#include "kernels/temporal_kernels.cuh"

#include <cuda_runtime.h>

#include <cmath>
#include <cstddef>
#include <limits>

namespace prometheus::cuda::kernels {
namespace {

constexpr int kTemporalBlockSize = 256;

template <typename T>
__device__ __forceinline__ T device_abs(const T value) noexcept;

template <>
__device__ __forceinline__ float device_abs<float>(
    const float value) noexcept
{
    return fabsf(value);
}

template <>
__device__ __forceinline__ double device_abs<double>(
    const double value) noexcept
{
    return fabs(value);
}

template <typename T>
__device__ __forceinline__ T device_exp2(const T value) noexcept;

template <>
__device__ __forceinline__ float device_exp2<float>(
    const float value) noexcept
{
    return exp2f(value);
}

template <>
__device__ __forceinline__ double device_exp2<double>(
    const double value) noexcept
{
    return exp2(value);
}

template <typename T>
__device__ __forceinline__ T clamp_unit(const T value) noexcept
{
    if (value < static_cast<T>(0)) {
        return static_cast<T>(0);
    }

    if (value > static_cast<T>(1)) {
        return static_cast<T>(1);
    }

    return value;
}

template <typename T>
__device__ __forceinline__ T decay_multiplier(
    const T elapsed_seconds,
    const T half_life_seconds,
    const T minimum_multiplier) noexcept
{
    const T elapsed =
        elapsed_seconds > static_cast<T>(0)
            ? elapsed_seconds
            : static_cast<T>(0);

    T multiplier =
        device_exp2<T>(
            -elapsed / half_life_seconds);

    if (multiplier < minimum_multiplier) {
        multiplier = minimum_multiplier;
    }

    return multiplier;
}

__device__ __forceinline__ bool stream_enabled(
    const std::uint8_t* stream_mask,
    const std::size_t stream_index) noexcept
{
    return
        stream_mask == nullptr ||
        stream_mask[stream_index] != 0U;
}

__device__ __forceinline__ bool element_enabled(
    const std::uint8_t* element_mask,
    const std::size_t index) noexcept
{
    return
        element_mask == nullptr ||
        element_mask[index] != 0U;
}

template <typename T>
__global__ void temporal_ema_kernel(
    const T* observation,
    T* state,
    const std::size_t feature_count,
    const std::size_t value_count,
    const T* alpha_by_stream,
    const std::uint8_t* stream_mask,
    const T scalar_alpha)
{
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) *
            blockDim.x +
        threadIdx.x;

    if (index >= value_count) {
        return;
    }

    const std::size_t stream_index =
        index / feature_count;

    if (!stream_enabled(stream_mask, stream_index)) {
        return;
    }

    const T alpha =
        clamp_unit(
            alpha_by_stream != nullptr
                ? alpha_by_stream[stream_index]
                : scalar_alpha);

    const T previous = state[index];
    state[index] =
        previous +
        alpha * (observation[index] - previous);
}

template <typename T>
__global__ void temporal_decay_kernel(
    T* state,
    const std::size_t feature_count,
    const std::size_t value_count,
    const T* elapsed_seconds_by_stream,
    const std::uint8_t* stream_mask,
    const T scalar_elapsed_seconds,
    const T half_life_seconds,
    const T minimum_multiplier)
{
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) *
            blockDim.x +
        threadIdx.x;

    if (index >= value_count) {
        return;
    }

    const std::size_t stream_index =
        index / feature_count;

    if (!stream_enabled(stream_mask, stream_index)) {
        return;
    }

    const T elapsed_seconds =
        elapsed_seconds_by_stream != nullptr
            ? elapsed_seconds_by_stream[stream_index]
            : scalar_elapsed_seconds;

    state[index] *=
        decay_multiplier(
            elapsed_seconds,
            half_life_seconds,
            minimum_multiplier);
}

template <typename T>
__global__ void gated_state_update_kernel(
    const T* candidate,
    const T* gate_by_element,
    T* state,
    const std::size_t feature_count,
    const std::size_t value_count,
    const std::uint8_t* stream_mask,
    const T scalar_gate)
{
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) *
            blockDim.x +
        threadIdx.x;

    if (index >= value_count) {
        return;
    }

    const std::size_t stream_index =
        index / feature_count;

    if (!stream_enabled(stream_mask, stream_index)) {
        return;
    }

    const T gate =
        clamp_unit(
            gate_by_element != nullptr
                ? gate_by_element[index]
                : scalar_gate);

    const T previous = state[index];
    state[index] =
        previous +
        gate * (candidate[index] - previous);
}

template <typename T>
__global__ void fused_temporal_step_kernel(
    const T* observation,
    const T* gate_by_element,
    const T* elapsed_seconds_by_stream,
    const std::uint8_t* stream_mask,
    T* previous_state,
    T* state,
    const std::size_t feature_count,
    const std::size_t value_count,
    const T scalar_gate,
    const T scalar_elapsed_seconds,
    const T half_life_seconds,
    const T minimum_multiplier)
{
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) *
            blockDim.x +
        threadIdx.x;

    if (index >= value_count) {
        return;
    }

    const std::size_t stream_index =
        index / feature_count;

    const T previous = state[index];

    if (previous_state != nullptr) {
        previous_state[index] = previous;
    }

    if (!stream_enabled(stream_mask, stream_index)) {
        return;
    }

    const T elapsed_seconds =
        elapsed_seconds_by_stream != nullptr
            ? elapsed_seconds_by_stream[stream_index]
            : scalar_elapsed_seconds;

    const T multiplier =
        decay_multiplier(
            elapsed_seconds,
            half_life_seconds,
            minimum_multiplier);

    const T decayed = previous * multiplier;

    const T gate =
        clamp_unit(
            gate_by_element != nullptr
                ? gate_by_element[index]
                : scalar_gate);

    state[index] =
        decayed +
        gate * (observation[index] - decayed);
}

template <typename T>
__global__ void prediction_error_kernel(
    const T* observed,
    const T* predicted,
    T* output,
    const std::size_t count,
    const PredictionErrorMode mode,
    const std::uint8_t* element_mask)
{
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) *
            blockDim.x +
        threadIdx.x;

    if (index >= count) {
        return;
    }

    if (!element_enabled(element_mask, index)) {
        output[index] = static_cast<T>(0);
        return;
    }

    const T residual =
        observed[index] - predicted[index];

    if (mode == PredictionErrorMode::Residual) {
        output[index] = residual;
    } else if (mode == PredictionErrorMode::Absolute) {
        output[index] = device_abs<T>(residual);
    } else {
        output[index] = residual * residual;
    }
}

template <typename T>
__global__ void surprise_kernel(
    const T* observed,
    const T* predicted,
    const T* uncertainty,
    T* surprise,
    const std::size_t count,
    const T epsilon,
    const std::uint8_t* element_mask)
{
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) *
            blockDim.x +
        threadIdx.x;

    if (index >= count) {
        return;
    }

    if (!element_enabled(element_mask, index)) {
        surprise[index] = static_cast<T>(0);
        return;
    }

    const T absolute_error =
        device_abs<T>(
            observed[index] - predicted[index]);

    if (uncertainty == nullptr) {
        surprise[index] = absolute_error;
        return;
    }

    T denominator =
        device_abs<T>(uncertainty[index]);

    if (denominator < epsilon) {
        denominator = epsilon;
    }

    surprise[index] =
        absolute_error / denominator;
}

template <typename T>
__global__ void copy_kernel(
    const T* source,
    T* destination,
    const std::size_t count)
{
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) *
            blockDim.x +
        threadIdx.x;

    if (index < count) {
        destination[index] = source[index];
    }
}

template <typename T>
__global__ void reset_kernel(
    T* state,
    const std::size_t feature_count,
    const std::size_t value_count,
    const std::uint8_t* stream_mask,
    const T value)
{
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) *
            blockDim.x +
        threadIdx.x;

    if (index >= value_count) {
        return;
    }

    const std::size_t stream_index =
        index / feature_count;

    if (stream_enabled(stream_mask, stream_index)) {
        state[index] = value;
    }
}

template <typename T>
__global__ void stream_mask_kernel(
    const T* input,
    T* output,
    const std::size_t feature_count,
    const std::size_t value_count,
    const std::uint8_t* stream_mask,
    const T masked_value)
{
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) *
            blockDim.x +
        threadIdx.x;

    if (index >= value_count) {
        return;
    }

    const std::size_t stream_index =
        index / feature_count;

    output[index] =
        stream_enabled(stream_mask, stream_index)
            ? input[index]
            : masked_value;
}

bool valid_shape(const TemporalShape shape) noexcept
{
    if (
        shape.stream_count == 0U ||
        shape.feature_count == 0U)
    {
        return false;
    }

    return
        shape.stream_count <=
            std::numeric_limits<std::size_t>::max() /
            shape.feature_count;
}

bool valid_prediction_mode(
    const PredictionErrorMode mode) noexcept
{
    return
        mode == PredictionErrorMode::Residual ||
        mode == PredictionErrorMode::Absolute ||
        mode == PredictionErrorMode::Squared;
}

std::size_t value_count(
    const TemporalShape shape) noexcept
{
    return shape.stream_count * shape.feature_count;
}

int grid_size(const std::size_t count) noexcept
{
    return static_cast<int>(
        (
            count +
            static_cast<std::size_t>(kTemporalBlockSize) -
            1U
        ) /
        static_cast<std::size_t>(kTemporalBlockSize));
}

template <typename T>
bool valid_unit_scalar(const T value) noexcept
{
    return
        std::isfinite(value) &&
        value >= static_cast<T>(0) &&
        value <= static_cast<T>(1);
}

template <typename T>
bool valid_decay_parameters(
    const T scalar_elapsed_seconds,
    const T half_life_seconds,
    const T minimum_multiplier) noexcept
{
    return
        std::isfinite(scalar_elapsed_seconds) &&
        scalar_elapsed_seconds >= static_cast<T>(0) &&
        std::isfinite(half_life_seconds) &&
        half_life_seconds > static_cast<T>(0) &&
        valid_unit_scalar(minimum_multiplier);
}

template <typename T>
cudaError_t launch_temporal_ema(
    const T* observation,
    T* state,
    const TemporalShape shape,
    const T* alpha_by_stream,
    const std::uint8_t* stream_mask,
    const T scalar_alpha,
    cudaStream_t stream) noexcept
{
    if (
        observation == nullptr ||
        state == nullptr ||
        !valid_shape(shape) ||
        !valid_unit_scalar(scalar_alpha))
    {
        return cudaErrorInvalidValue;
    }

    const std::size_t count = value_count(shape);

    temporal_ema_kernel<T>
        <<<grid_size(count), kTemporalBlockSize, 0, stream>>>(
            observation,
            state,
            shape.feature_count,
            count,
            alpha_by_stream,
            stream_mask,
            scalar_alpha);

    return cudaPeekAtLastError();
}

template <typename T>
cudaError_t launch_temporal_decay(
    T* state,
    const TemporalShape shape,
    const T* elapsed_seconds_by_stream,
    const std::uint8_t* stream_mask,
    const T scalar_elapsed_seconds,
    const T half_life_seconds,
    const T minimum_multiplier,
    cudaStream_t stream) noexcept
{
    if (
        state == nullptr ||
        !valid_shape(shape) ||
        !valid_decay_parameters(
            scalar_elapsed_seconds,
            half_life_seconds,
            minimum_multiplier))
    {
        return cudaErrorInvalidValue;
    }

    const std::size_t count = value_count(shape);

    temporal_decay_kernel<T>
        <<<grid_size(count), kTemporalBlockSize, 0, stream>>>(
            state,
            shape.feature_count,
            count,
            elapsed_seconds_by_stream,
            stream_mask,
            scalar_elapsed_seconds,
            half_life_seconds,
            minimum_multiplier);

    return cudaPeekAtLastError();
}

template <typename T>
cudaError_t launch_gated_state_update(
    const T* candidate,
    const T* gate_by_element,
    T* state,
    const TemporalShape shape,
    const std::uint8_t* stream_mask,
    const T scalar_gate,
    cudaStream_t stream) noexcept
{
    if (
        candidate == nullptr ||
        state == nullptr ||
        !valid_shape(shape) ||
        !valid_unit_scalar(scalar_gate))
    {
        return cudaErrorInvalidValue;
    }

    const std::size_t count = value_count(shape);

    gated_state_update_kernel<T>
        <<<grid_size(count), kTemporalBlockSize, 0, stream>>>(
            candidate,
            gate_by_element,
            state,
            shape.feature_count,
            count,
            stream_mask,
            scalar_gate);

    return cudaPeekAtLastError();
}

template <typename T>
cudaError_t launch_fused_temporal_step(
    const T* observation,
    const T* gate_by_element,
    const T* elapsed_seconds_by_stream,
    const std::uint8_t* stream_mask,
    T* previous_state,
    T* state,
    const TemporalShape shape,
    const T scalar_gate,
    const T scalar_elapsed_seconds,
    const T half_life_seconds,
    const T minimum_multiplier,
    cudaStream_t stream) noexcept
{
    if (
        observation == nullptr ||
        state == nullptr ||
        !valid_shape(shape) ||
        !valid_unit_scalar(scalar_gate) ||
        !valid_decay_parameters(
            scalar_elapsed_seconds,
            half_life_seconds,
            minimum_multiplier))
    {
        return cudaErrorInvalidValue;
    }

    const std::size_t count = value_count(shape);

    fused_temporal_step_kernel<T>
        <<<grid_size(count), kTemporalBlockSize, 0, stream>>>(
            observation,
            gate_by_element,
            elapsed_seconds_by_stream,
            stream_mask,
            previous_state,
            state,
            shape.feature_count,
            count,
            scalar_gate,
            scalar_elapsed_seconds,
            half_life_seconds,
            minimum_multiplier);

    return cudaPeekAtLastError();
}

template <typename T>
cudaError_t launch_prediction_error(
    const T* observed,
    const T* predicted,
    T* output,
    const std::size_t count,
    const PredictionErrorMode mode,
    const std::uint8_t* element_mask,
    cudaStream_t stream) noexcept
{
    if (
        observed == nullptr ||
        predicted == nullptr ||
        output == nullptr ||
        count == 0U ||
        !valid_prediction_mode(mode))
    {
        return cudaErrorInvalidValue;
    }

    prediction_error_kernel<T>
        <<<grid_size(count), kTemporalBlockSize, 0, stream>>>(
            observed,
            predicted,
            output,
            count,
            mode,
            element_mask);

    return cudaPeekAtLastError();
}

template <typename T>
cudaError_t launch_surprise(
    const T* observed,
    const T* predicted,
    const T* uncertainty,
    T* surprise,
    const std::size_t count,
    const T epsilon,
    const std::uint8_t* element_mask,
    cudaStream_t stream) noexcept
{
    if (
        observed == nullptr ||
        predicted == nullptr ||
        surprise == nullptr ||
        count == 0U ||
        !std::isfinite(epsilon) ||
        epsilon <= static_cast<T>(0))
    {
        return cudaErrorInvalidValue;
    }

    surprise_kernel<T>
        <<<grid_size(count), kTemporalBlockSize, 0, stream>>>(
            observed,
            predicted,
            uncertainty,
            surprise,
            count,
            epsilon,
            element_mask);

    return cudaPeekAtLastError();
}

template <typename T>
cudaError_t launch_copy(
    const T* source,
    T* destination,
    const std::size_t count,
    cudaStream_t stream) noexcept
{
    if (
        source == nullptr ||
        destination == nullptr ||
        count == 0U)
    {
        return cudaErrorInvalidValue;
    }

    copy_kernel<T>
        <<<grid_size(count), kTemporalBlockSize, 0, stream>>>(
            source,
            destination,
            count);

    return cudaPeekAtLastError();
}

template <typename T>
cudaError_t launch_reset(
    T* state,
    const TemporalShape shape,
    const std::uint8_t* stream_mask,
    const T value,
    cudaStream_t stream) noexcept
{
    if (
        state == nullptr ||
        !valid_shape(shape) ||
        !std::isfinite(value))
    {
        return cudaErrorInvalidValue;
    }

    const std::size_t count = value_count(shape);

    reset_kernel<T>
        <<<grid_size(count), kTemporalBlockSize, 0, stream>>>(
            state,
            shape.feature_count,
            count,
            stream_mask,
            value);

    return cudaPeekAtLastError();
}

template <typename T>
cudaError_t launch_stream_mask(
    const T* input,
    T* output,
    const TemporalShape shape,
    const std::uint8_t* stream_mask,
    const T masked_value,
    cudaStream_t stream) noexcept
{
    if (
        input == nullptr ||
        output == nullptr ||
        stream_mask == nullptr ||
        !valid_shape(shape) ||
        !std::isfinite(masked_value))
    {
        return cudaErrorInvalidValue;
    }

    const std::size_t count = value_count(shape);

    stream_mask_kernel<T>
        <<<grid_size(count), kTemporalBlockSize, 0, stream>>>(
            input,
            output,
            shape.feature_count,
            count,
            stream_mask,
            masked_value);

    return cudaPeekAtLastError();
}

}  // namespace

cudaError_t launch_temporal_ema_f32(
    const float* observation,
    float* state,
    const TemporalShape shape,
    const float* alpha_by_stream,
    const std::uint8_t* stream_mask,
    const float scalar_alpha,
    cudaStream_t stream) noexcept
{
    return launch_temporal_ema(
        observation,
        state,
        shape,
        alpha_by_stream,
        stream_mask,
        scalar_alpha,
        stream);
}

cudaError_t launch_temporal_ema_f64(
    const double* observation,
    double* state,
    const TemporalShape shape,
    const double* alpha_by_stream,
    const std::uint8_t* stream_mask,
    const double scalar_alpha,
    cudaStream_t stream) noexcept
{
    return launch_temporal_ema(
        observation,
        state,
        shape,
        alpha_by_stream,
        stream_mask,
        scalar_alpha,
        stream);
}

cudaError_t launch_temporal_decay_f32(
    float* state,
    const TemporalShape shape,
    const float* elapsed_seconds_by_stream,
    const std::uint8_t* stream_mask,
    const float scalar_elapsed_seconds,
    const float half_life_seconds,
    const float minimum_multiplier,
    cudaStream_t stream) noexcept
{
    return launch_temporal_decay(
        state,
        shape,
        elapsed_seconds_by_stream,
        stream_mask,
        scalar_elapsed_seconds,
        half_life_seconds,
        minimum_multiplier,
        stream);
}

cudaError_t launch_temporal_decay_f64(
    double* state,
    const TemporalShape shape,
    const double* elapsed_seconds_by_stream,
    const std::uint8_t* stream_mask,
    const double scalar_elapsed_seconds,
    const double half_life_seconds,
    const double minimum_multiplier,
    cudaStream_t stream) noexcept
{
    return launch_temporal_decay(
        state,
        shape,
        elapsed_seconds_by_stream,
        stream_mask,
        scalar_elapsed_seconds,
        half_life_seconds,
        minimum_multiplier,
        stream);
}

cudaError_t launch_gated_state_update_f32(
    const float* candidate,
    const float* gate_by_element,
    float* state,
    const TemporalShape shape,
    const std::uint8_t* stream_mask,
    const float scalar_gate,
    cudaStream_t stream) noexcept
{
    return launch_gated_state_update(
        candidate,
        gate_by_element,
        state,
        shape,
        stream_mask,
        scalar_gate,
        stream);
}

cudaError_t launch_gated_state_update_f64(
    const double* candidate,
    const double* gate_by_element,
    double* state,
    const TemporalShape shape,
    const std::uint8_t* stream_mask,
    const double scalar_gate,
    cudaStream_t stream) noexcept
{
    return launch_gated_state_update(
        candidate,
        gate_by_element,
        state,
        shape,
        stream_mask,
        scalar_gate,
        stream);
}

cudaError_t launch_fused_temporal_step_f32(
    const float* observation,
    const float* gate_by_element,
    const float* elapsed_seconds_by_stream,
    const std::uint8_t* stream_mask,
    float* previous_state,
    float* state,
    const TemporalShape shape,
    const float scalar_gate,
    const float scalar_elapsed_seconds,
    const float half_life_seconds,
    const float minimum_multiplier,
    cudaStream_t stream) noexcept
{
    return launch_fused_temporal_step(
        observation,
        gate_by_element,
        elapsed_seconds_by_stream,
        stream_mask,
        previous_state,
        state,
        shape,
        scalar_gate,
        scalar_elapsed_seconds,
        half_life_seconds,
        minimum_multiplier,
        stream);
}

cudaError_t launch_fused_temporal_step_f64(
    const double* observation,
    const double* gate_by_element,
    const double* elapsed_seconds_by_stream,
    const std::uint8_t* stream_mask,
    double* previous_state,
    double* state,
    const TemporalShape shape,
    const double scalar_gate,
    const double scalar_elapsed_seconds,
    const double half_life_seconds,
    const double minimum_multiplier,
    cudaStream_t stream) noexcept
{
    return launch_fused_temporal_step(
        observation,
        gate_by_element,
        elapsed_seconds_by_stream,
        stream_mask,
        previous_state,
        state,
        shape,
        scalar_gate,
        scalar_elapsed_seconds,
        half_life_seconds,
        minimum_multiplier,
        stream);
}

cudaError_t launch_prediction_error_f32(
    const float* observed,
    const float* predicted,
    float* output,
    const std::size_t count,
    const PredictionErrorMode mode,
    const std::uint8_t* element_mask,
    cudaStream_t stream) noexcept
{
    return launch_prediction_error(
        observed,
        predicted,
        output,
        count,
        mode,
        element_mask,
        stream);
}

cudaError_t launch_prediction_error_f64(
    const double* observed,
    const double* predicted,
    double* output,
    const std::size_t count,
    const PredictionErrorMode mode,
    const std::uint8_t* element_mask,
    cudaStream_t stream) noexcept
{
    return launch_prediction_error(
        observed,
        predicted,
        output,
        count,
        mode,
        element_mask,
        stream);
}

cudaError_t launch_surprise_f32(
    const float* observed,
    const float* predicted,
    const float* uncertainty,
    float* surprise,
    const std::size_t count,
    const float epsilon,
    const std::uint8_t* element_mask,
    cudaStream_t stream) noexcept
{
    return launch_surprise(
        observed,
        predicted,
        uncertainty,
        surprise,
        count,
        epsilon,
        element_mask,
        stream);
}

cudaError_t launch_surprise_f64(
    const double* observed,
    const double* predicted,
    const double* uncertainty,
    double* surprise,
    const std::size_t count,
    const double epsilon,
    const std::uint8_t* element_mask,
    cudaStream_t stream) noexcept
{
    return launch_surprise(
        observed,
        predicted,
        uncertainty,
        surprise,
        count,
        epsilon,
        element_mask,
        stream);
}

cudaError_t launch_temporal_copy_f32(
    const float* source,
    float* destination,
    const std::size_t count,
    cudaStream_t stream) noexcept
{
    return launch_copy(
        source,
        destination,
        count,
        stream);
}

cudaError_t launch_temporal_copy_f64(
    const double* source,
    double* destination,
    const std::size_t count,
    cudaStream_t stream) noexcept
{
    return launch_copy(
        source,
        destination,
        count,
        stream);
}

cudaError_t launch_temporal_reset_f32(
    float* state,
    const TemporalShape shape,
    const std::uint8_t* stream_mask,
    const float value,
    cudaStream_t stream) noexcept
{
    return launch_reset(
        state,
        shape,
        stream_mask,
        value,
        stream);
}

cudaError_t launch_temporal_reset_f64(
    double* state,
    const TemporalShape shape,
    const std::uint8_t* stream_mask,
    const double value,
    cudaStream_t stream) noexcept
{
    return launch_reset(
        state,
        shape,
        stream_mask,
        value,
        stream);
}

cudaError_t launch_apply_stream_mask_f32(
    const float* input,
    float* output,
    const TemporalShape shape,
    const std::uint8_t* stream_mask,
    const float masked_value,
    cudaStream_t stream) noexcept
{
    return launch_stream_mask(
        input,
        output,
        shape,
        stream_mask,
        masked_value,
        stream);
}

cudaError_t launch_apply_stream_mask_f64(
    const double* input,
    double* output,
    const TemporalShape shape,
    const std::uint8_t* stream_mask,
    const double masked_value,
    cudaStream_t stream) noexcept
{
    return launch_stream_mask(
        input,
        output,
        shape,
        stream_mask,
        masked_value,
        stream);
}

}  // namespace prometheus::cuda::kernels
