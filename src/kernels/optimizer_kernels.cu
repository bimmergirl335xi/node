#include "kernels/optimizer_kernels.cuh"

#include <cuda_runtime.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace prometheus::cuda::kernels {
namespace {

constexpr int kOptimizerBlockSize = 256;

template <typename T>
__device__ __forceinline__ T device_sqrt(const T value) noexcept;

template <>
__device__ __forceinline__ float device_sqrt<float>(
    const float value) noexcept
{
    return sqrtf(value);
}

template <>
__device__ __forceinline__ double device_sqrt<double>(
    const double value) noexcept
{
    return sqrt(value);
}

template <typename T>
__device__ __forceinline__ T clamp_symmetric(
    const T value,
    const T limit) noexcept
{
    if (limit <= static_cast<T>(0)) {
        return value;
    }

    if (value > limit) {
        return limit;
    }

    if (value < -limit) {
        return -limit;
    }

    return value;
}

__device__ __forceinline__ bool mask_enabled(
    const std::uint8_t* mask,
    const std::size_t index) noexcept
{
    return mask == nullptr || mask[index] != 0U;
}

template <typename T>
__global__ void accumulate_signal_kernel(
    const T* source,
    T* accumulator,
    const std::size_t count,
    const T scale,
    const std::uint8_t* mask)
{
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;

    if (index >= count || !mask_enabled(mask, index)) {
        return;
    }

    accumulator[index] += scale * source[index];
}

template <typename T>
__global__ void reset_state_kernel(
    T* first_state,
    T* second_state,
    const std::size_t count,
    const T value)
{
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;

    if (index >= count) {
        return;
    }

    if (first_state != nullptr) {
        first_state[index] = value;
    }

    if (second_state != nullptr) {
        second_state[index] = value;
    }
}

__global__ void set_step_kernel(
    std::uint64_t* step,
    const std::uint64_t value)
{
    if (blockIdx.x == 0 && threadIdx.x == 0) {
        *step = value;
    }
}

__global__ void increment_step_kernel(std::uint64_t* step)
{
    if (blockIdx.x == 0 && threadIdx.x == 0) {
        *step += 1U;
    }
}

template <typename T>
__global__ void optimizer_step_kernel(
    T* parameters,
    const T* update_signal,
    T* first_moment,
    T* second_moment,
    T* effective_update,
    const std::size_t count,
    const OptimizerType optimizer,
    const OptimizationDirection direction,
    const std::uint8_t* mask,
    const T learning_rate,
    const T beta1,
    const T beta2,
    const T epsilon,
    const T weight_decay,
    const T signal_scale,
    const T signal_clip,
    const T delta_clip,
    const T parameter_clip,
    const T inverse_bias_correction1,
    const T inverse_bias_correction2,
    const bool apply_update)
{
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;

    if (index >= count) {
        return;
    }

    if (!mask_enabled(mask, index)) {
        if (effective_update != nullptr) {
            effective_update[index] = static_cast<T>(0);
        }
        return;
    }

    const T parameter = parameters[index];

    T signal =
        clamp_symmetric(
            update_signal[index] * signal_scale,
            signal_clip);

    T proposed_first =
        first_moment != nullptr
            ? first_moment[index]
            : static_cast<T>(0);

    T proposed_second =
        second_moment != nullptr
            ? second_moment[index]
            : static_cast<T>(0);

    T optimized_signal = signal;

    if (optimizer == OptimizerType::Momentum) {
        proposed_first =
            beta1 * proposed_first +
            (static_cast<T>(1) - beta1) * signal;

        optimized_signal = proposed_first;
    } else if (optimizer == OptimizerType::RmsProp) {
        proposed_second =
            beta2 * proposed_second +
            (static_cast<T>(1) - beta2) * signal * signal;

        optimized_signal =
            signal / (device_sqrt<T>(proposed_second) + epsilon);
    } else if (optimizer == OptimizerType::Adam) {
        proposed_first =
            beta1 * proposed_first +
            (static_cast<T>(1) - beta1) * signal;

        proposed_second =
            beta2 * proposed_second +
            (static_cast<T>(1) - beta2) * signal * signal;

        const T corrected_first =
            proposed_first * inverse_bias_correction1;

        const T corrected_second =
            proposed_second * inverse_bias_correction2;

        optimized_signal =
            corrected_first /
            (device_sqrt<T>(corrected_second) + epsilon);
    }

    const T signed_learning_rate =
        direction == OptimizationDirection::Ascent
            ? learning_rate
            : -learning_rate;

    T proposed_delta =
        signed_learning_rate * optimized_signal -
        learning_rate * weight_decay * parameter;

    proposed_delta =
        clamp_symmetric(proposed_delta, delta_clip);

    const T proposed_parameter =
        clamp_symmetric(
            parameter + proposed_delta,
            parameter_clip);

    const T actual_delta =
        proposed_parameter - parameter;

    if (effective_update != nullptr) {
        effective_update[index] = actual_delta;
    }

    if (!apply_update) {
        return;
    }

    parameters[index] = proposed_parameter;

    if (
        optimizer == OptimizerType::Momentum ||
        optimizer == OptimizerType::Adam)
    {
        first_moment[index] = proposed_first;
    }

    if (
        optimizer == OptimizerType::RmsProp ||
        optimizer == OptimizerType::Adam)
    {
        second_moment[index] = proposed_second;
    }
}

int grid_size(const std::size_t count) noexcept
{
    return static_cast<int>(
        (count + static_cast<std::size_t>(kOptimizerBlockSize) - 1U) /
        static_cast<std::size_t>(kOptimizerBlockSize));
}

bool valid_optimizer(const OptimizerType optimizer) noexcept
{
    return
        optimizer == OptimizerType::Sgd ||
        optimizer == OptimizerType::Momentum ||
        optimizer == OptimizerType::RmsProp ||
        optimizer == OptimizerType::Adam;
}

bool valid_direction(
    const OptimizationDirection direction) noexcept
{
    return
        direction == OptimizationDirection::Descent ||
        direction == OptimizationDirection::Ascent;
}

template <typename T>
bool valid_settings(
    const T learning_rate,
    const T beta1,
    const T beta2,
    const T epsilon,
    const T weight_decay,
    const T signal_scale,
    const T signal_clip,
    const T delta_clip,
    const T parameter_clip) noexcept
{
    return
        std::isfinite(learning_rate) &&
        learning_rate >= static_cast<T>(0) &&
        std::isfinite(beta1) &&
        beta1 >= static_cast<T>(0) &&
        beta1 < static_cast<T>(1) &&
        std::isfinite(beta2) &&
        beta2 >= static_cast<T>(0) &&
        beta2 < static_cast<T>(1) &&
        std::isfinite(epsilon) &&
        epsilon > static_cast<T>(0) &&
        std::isfinite(weight_decay) &&
        weight_decay >= static_cast<T>(0) &&
        std::isfinite(signal_scale) &&
        std::isfinite(signal_clip) &&
        signal_clip >= static_cast<T>(0) &&
        std::isfinite(delta_clip) &&
        delta_clip >= static_cast<T>(0) &&
        std::isfinite(parameter_clip) &&
        parameter_clip >= static_cast<T>(0);
}

template <typename T>
cudaError_t launch_accumulate_signal(
    const T* source,
    T* accumulator,
    const std::size_t count,
    const T scale,
    const std::uint8_t* mask,
    cudaStream_t stream) noexcept
{
    if (
        source == nullptr ||
        accumulator == nullptr ||
        count == 0U ||
        !std::isfinite(scale))
    {
        return cudaErrorInvalidValue;
    }

    accumulate_signal_kernel<T>
        <<<grid_size(count), kOptimizerBlockSize, 0, stream>>>(
            source,
            accumulator,
            count,
            scale,
            mask);

    return cudaPeekAtLastError();
}

template <typename T>
cudaError_t launch_reset_state(
    T* first_state,
    T* second_state,
    const std::size_t count,
    const T value,
    cudaStream_t stream) noexcept
{
    if (
        (first_state == nullptr && second_state == nullptr) ||
        count == 0U ||
        !std::isfinite(value))
    {
        return cudaErrorInvalidValue;
    }

    reset_state_kernel<T>
        <<<grid_size(count), kOptimizerBlockSize, 0, stream>>>(
            first_state,
            second_state,
            count,
            value);

    return cudaPeekAtLastError();
}

template <typename T>
cudaError_t launch_optimizer_step(
    T* parameters,
    const T* update_signal,
    T* first_moment,
    T* second_moment,
    T* effective_update,
    const std::size_t count,
    const OptimizerType optimizer,
    const OptimizationDirection direction,
    const std::uint64_t step,
    const std::uint8_t* mask,
    const T learning_rate,
    const T beta1,
    const T beta2,
    const T epsilon,
    const T weight_decay,
    const T signal_scale,
    const T signal_clip,
    const T delta_clip,
    const T parameter_clip,
    const bool apply_update,
    cudaStream_t stream) noexcept
{
    if (
        parameters == nullptr ||
        update_signal == nullptr ||
        count == 0U ||
        !valid_optimizer(optimizer) ||
        !valid_direction(direction) ||
        !valid_settings(
            learning_rate,
            beta1,
            beta2,
            epsilon,
            weight_decay,
            signal_scale,
            signal_clip,
            delta_clip,
            parameter_clip))
    {
        return cudaErrorInvalidValue;
    }

    if (
        (optimizer == OptimizerType::Momentum ||
         optimizer == OptimizerType::Adam) &&
        first_moment == nullptr)
    {
        return cudaErrorInvalidValue;
    }

    if (
        (optimizer == OptimizerType::RmsProp ||
         optimizer == OptimizerType::Adam) &&
        second_moment == nullptr)
    {
        return cudaErrorInvalidValue;
    }

    if (optimizer == OptimizerType::Adam && step == 0U) {
        return cudaErrorInvalidValue;
    }

    T inverse_bias_correction1 = static_cast<T>(1);
    T inverse_bias_correction2 = static_cast<T>(1);

    if (optimizer == OptimizerType::Adam) {
        const T correction1 =
            static_cast<T>(1) -
            std::pow(beta1, static_cast<T>(step));

        const T correction2 =
            static_cast<T>(1) -
            std::pow(beta2, static_cast<T>(step));

        if (
            correction1 <= static_cast<T>(0) ||
            correction2 <= static_cast<T>(0))
        {
            return cudaErrorInvalidValue;
        }

        inverse_bias_correction1 =
            static_cast<T>(1) / correction1;

        inverse_bias_correction2 =
            static_cast<T>(1) / correction2;
    }

    optimizer_step_kernel<T>
        <<<grid_size(count), kOptimizerBlockSize, 0, stream>>>(
            parameters,
            update_signal,
            first_moment,
            second_moment,
            effective_update,
            count,
            optimizer,
            direction,
            mask,
            learning_rate,
            beta1,
            beta2,
            epsilon,
            weight_decay,
            signal_scale,
            signal_clip,
            delta_clip,
            parameter_clip,
            inverse_bias_correction1,
            inverse_bias_correction2,
            apply_update);

    return cudaPeekAtLastError();
}

}  // namespace

cudaError_t launch_accumulate_signal_f32(
    const float* source,
    float* accumulator,
    const std::size_t count,
    const float scale,
    const std::uint8_t* mask,
    cudaStream_t stream) noexcept
{
    return launch_accumulate_signal(
        source, accumulator, count, scale, mask, stream);
}

cudaError_t launch_accumulate_signal_f64(
    const double* source,
    double* accumulator,
    const std::size_t count,
    const double scale,
    const std::uint8_t* mask,
    cudaStream_t stream) noexcept
{
    return launch_accumulate_signal(
        source, accumulator, count, scale, mask, stream);
}

cudaError_t launch_reset_optimizer_state_f32(
    float* first_state,
    float* second_state,
    const std::size_t count,
    const float value,
    cudaStream_t stream) noexcept
{
    return launch_reset_state(
        first_state, second_state, count, value, stream);
}

cudaError_t launch_reset_optimizer_state_f64(
    double* first_state,
    double* second_state,
    const std::size_t count,
    const double value,
    cudaStream_t stream) noexcept
{
    return launch_reset_state(
        first_state, second_state, count, value, stream);
}

cudaError_t launch_optimizer_step_f32(
    float* parameters,
    const float* update_signal,
    float* first_moment,
    float* second_moment,
    float* effective_update,
    const std::size_t count,
    const OptimizerType optimizer,
    const OptimizationDirection direction,
    const std::uint64_t step,
    const std::uint8_t* mask,
    const OptimizerParametersF32 settings,
    const bool apply_update,
    cudaStream_t stream) noexcept
{
    return launch_optimizer_step(
        parameters,
        update_signal,
        first_moment,
        second_moment,
        effective_update,
        count,
        optimizer,
        direction,
        step,
        mask,
        settings.learning_rate,
        settings.beta1,
        settings.beta2,
        settings.epsilon,
        settings.weight_decay,
        settings.signal_scale,
        settings.signal_clip,
        settings.delta_clip,
        settings.parameter_clip,
        apply_update,
        stream);
}

cudaError_t launch_optimizer_step_f64(
    double* parameters,
    const double* update_signal,
    double* first_moment,
    double* second_moment,
    double* effective_update,
    const std::size_t count,
    const OptimizerType optimizer,
    const OptimizationDirection direction,
    const std::uint64_t step,
    const std::uint8_t* mask,
    const OptimizerParametersF64 settings,
    const bool apply_update,
    cudaStream_t stream) noexcept
{
    return launch_optimizer_step(
        parameters,
        update_signal,
        first_moment,
        second_moment,
        effective_update,
        count,
        optimizer,
        direction,
        step,
        mask,
        settings.learning_rate,
        settings.beta1,
        settings.beta2,
        settings.epsilon,
        settings.weight_decay,
        settings.signal_scale,
        settings.signal_clip,
        settings.delta_clip,
        settings.parameter_clip,
        apply_update,
        stream);
}

cudaError_t launch_increment_optimizer_step(
    std::uint64_t* device_step,
    cudaStream_t stream) noexcept
{
    if (device_step == nullptr) {
        return cudaErrorInvalidValue;
    }

    increment_step_kernel<<<1, 1, 0, stream>>>(device_step);
    return cudaPeekAtLastError();
}

cudaError_t launch_reset_optimizer_step(
    std::uint64_t* device_step,
    const std::uint64_t value,
    cudaStream_t stream) noexcept
{
    if (device_step == nullptr) {
        return cudaErrorInvalidValue;
    }

    set_step_kernel<<<1, 1, 0, stream>>>(device_step, value);
    return cudaPeekAtLastError();
}

}  // namespace prometheus::cuda::kernels
