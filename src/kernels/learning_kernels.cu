#include "kernels/learning_kernels.cuh"

#include <cuda_runtime.h>

#include <cmath>
#include <cstddef>
#include <limits>

namespace prometheus::cuda::kernels {
namespace {

constexpr int kLearningBlockSize = 256;

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
__device__ __forceinline__ T learning_correlation(
    const LearningRule rule,
    const T pre,
    const T teaching,
    const T current_weight) noexcept
{
    if (rule == LearningRule::Oja) {
        return
            teaching * pre -
            teaching * teaching * current_weight;
    }

    /*
     * Hebbian and PredictiveDelta have the same outer-product mathematics.
     * Their semantic distinction is carried by the teaching signal.
     */
    return teaching * pre;
}

template <typename T>
__global__ void local_learning_kernel(
    const T* presynaptic,
    const T* teaching_signal,
    T* weights,
    T* update_output,
    T* row_l1_output,
    T* row_l2_output,
    T* row_max_abs_output,
    const std::size_t batch_count,
    const std::size_t input_dim,
    const std::size_t output_dim,
    const LearningRule rule,
    const LearningBatchReduction batch_reduction,
    const std::uint8_t* row_mask,
    const std::uint8_t* weight_mask,
    const T learning_rate,
    const T weight_decay,
    const T update_clip,
    const T weight_clip,
    const bool apply_update)
{
    __shared__ T l1_scratch[kLearningBlockSize];
    __shared__ T l2_scratch[kLearningBlockSize];
    __shared__ T max_scratch[kLearningBlockSize];

    const std::size_t row = blockIdx.x;

    if (row >= output_dim) {
        return;
    }

    const bool row_enabled =
        mask_enabled(row_mask, row);

    T local_l1 = static_cast<T>(0);
    T local_l2 = static_cast<T>(0);
    T local_max = static_cast<T>(0);

    for (
        std::size_t column = threadIdx.x;
        column < input_dim;
        column += blockDim.x)
    {
        const std::size_t weight_index =
            row * input_dim + column;

        T effective_delta = static_cast<T>(0);

        if (
            row_enabled &&
            mask_enabled(weight_mask, weight_index))
        {
            const T current_weight =
                weights[weight_index];

            T correlation = static_cast<T>(0);

            for (
                std::size_t batch = 0;
                batch < batch_count;
                ++batch)
            {
                const T pre =
                    presynaptic[
                        batch * input_dim + column
                    ];

                const T teaching =
                    teaching_signal[
                        batch * output_dim + row
                    ];

                correlation +=
                    learning_correlation(
                        rule,
                        pre,
                        teaching,
                        current_weight);
            }

            if (
                batch_reduction ==
                LearningBatchReduction::Mean)
            {
                correlation /=
                    static_cast<T>(batch_count);
            }

            T proposed_delta =
                learning_rate *
                (
                    correlation -
                    weight_decay * current_weight
                );

            proposed_delta =
                clamp_symmetric(
                    proposed_delta,
                    update_clip);

            const T proposed_weight =
                clamp_symmetric(
                    current_weight + proposed_delta,
                    weight_clip);

            effective_delta =
                proposed_weight - current_weight;

            if (apply_update) {
                weights[weight_index] =
                    proposed_weight;
            }
        }

        if (update_output != nullptr) {
            update_output[weight_index] =
                effective_delta;
        }

        const T absolute_delta =
            device_abs<T>(effective_delta);

        local_l1 += absolute_delta;
        local_l2 +=
            effective_delta * effective_delta;

        if (absolute_delta > local_max) {
            local_max = absolute_delta;
        }
    }

    l1_scratch[threadIdx.x] = local_l1;
    l2_scratch[threadIdx.x] = local_l2;
    max_scratch[threadIdx.x] = local_max;

    __syncthreads();

    for (
        int stride = blockDim.x / 2;
        stride > 0;
        stride /= 2)
    {
        if (threadIdx.x < stride) {
            l1_scratch[threadIdx.x] +=
                l1_scratch[threadIdx.x + stride];

            l2_scratch[threadIdx.x] +=
                l2_scratch[threadIdx.x + stride];

            const T candidate =
                max_scratch[threadIdx.x + stride];

            if (candidate > max_scratch[threadIdx.x]) {
                max_scratch[threadIdx.x] = candidate;
            }
        }

        __syncthreads();
    }

    if (threadIdx.x == 0) {
        if (row_l1_output != nullptr) {
            row_l1_output[row] =
                l1_scratch[0];
        }

        if (row_l2_output != nullptr) {
            row_l2_output[row] =
                device_sqrt<T>(l2_scratch[0]);
        }

        if (row_max_abs_output != nullptr) {
            row_max_abs_output[row] =
                max_scratch[0];
        }
    }
}

bool valid_shape(const LearningShape shape) noexcept
{
    if (
        shape.batch_count == 0U ||
        shape.input_dim == 0U ||
        shape.output_dim == 0U)
    {
        return false;
    }

    if (
        shape.batch_count >
        std::numeric_limits<std::size_t>::max() /
            shape.input_dim)
    {
        return false;
    }

    if (
        shape.batch_count >
        std::numeric_limits<std::size_t>::max() /
            shape.output_dim)
    {
        return false;
    }

    if (
        shape.output_dim >
        std::numeric_limits<std::size_t>::max() /
            shape.input_dim)
    {
        return false;
    }

    return
        shape.output_dim <=
        static_cast<std::size_t>(
            std::numeric_limits<unsigned int>::max());
}

bool valid_rule(const LearningRule rule) noexcept
{
    return
        rule == LearningRule::Hebbian ||
        rule == LearningRule::Oja ||
        rule == LearningRule::PredictiveDelta;
}

bool valid_batch_reduction(
    const LearningBatchReduction reduction) noexcept
{
    return
        reduction == LearningBatchReduction::Sum ||
        reduction == LearningBatchReduction::Mean;
}

template <typename T>
bool valid_parameters(
    const T learning_rate,
    const T weight_decay,
    const T update_clip,
    const T weight_clip) noexcept
{
    return
        std::isfinite(learning_rate) &&
        learning_rate >= static_cast<T>(0) &&
        std::isfinite(weight_decay) &&
        weight_decay >= static_cast<T>(0) &&
        std::isfinite(update_clip) &&
        update_clip >= static_cast<T>(0) &&
        std::isfinite(weight_clip) &&
        weight_clip >= static_cast<T>(0);
}

template <typename T>
cudaError_t launch_local_learning(
    const T* presynaptic,
    const T* teaching_signal,
    T* weights,
    T* update_output,
    T* row_l1_output,
    T* row_l2_output,
    T* row_max_abs_output,
    const LearningShape shape,
    const LearningRule rule,
    const LearningBatchReduction batch_reduction,
    const std::uint8_t* row_mask,
    const std::uint8_t* weight_mask,
    const T learning_rate,
    const T weight_decay,
    const T update_clip,
    const T weight_clip,
    const bool apply_update,
    cudaStream_t stream) noexcept
{
    if (
        presynaptic == nullptr ||
        teaching_signal == nullptr ||
        weights == nullptr ||
        !valid_shape(shape) ||
        !valid_rule(rule) ||
        !valid_batch_reduction(batch_reduction) ||
        !valid_parameters(
            learning_rate,
            weight_decay,
            update_clip,
            weight_clip))
    {
        return cudaErrorInvalidValue;
    }

    local_learning_kernel<T>
        <<<static_cast<unsigned int>(shape.output_dim),
           kLearningBlockSize,
           0,
           stream>>>(
            presynaptic,
            teaching_signal,
            weights,
            update_output,
            row_l1_output,
            row_l2_output,
            row_max_abs_output,
            shape.batch_count,
            shape.input_dim,
            shape.output_dim,
            rule,
            batch_reduction,
            row_mask,
            weight_mask,
            learning_rate,
            weight_decay,
            update_clip,
            weight_clip,
            apply_update);

    return cudaPeekAtLastError();
}

}  // namespace

cudaError_t launch_local_learning_f32(
    const float* presynaptic,
    const float* teaching_signal,
    float* weights,
    float* update_output,
    float* row_l1_output,
    float* row_l2_output,
    float* row_max_abs_output,
    const LearningShape shape,
    const LearningRule rule,
    const LearningBatchReduction batch_reduction,
    const std::uint8_t* row_mask,
    const std::uint8_t* weight_mask,
    const LearningParametersF32 parameters,
    const bool apply_update,
    cudaStream_t stream) noexcept
{
    return launch_local_learning(
        presynaptic,
        teaching_signal,
        weights,
        update_output,
        row_l1_output,
        row_l2_output,
        row_max_abs_output,
        shape,
        rule,
        batch_reduction,
        row_mask,
        weight_mask,
        parameters.learning_rate,
        parameters.weight_decay,
        parameters.update_clip,
        parameters.weight_clip,
        apply_update,
        stream);
}

cudaError_t launch_local_learning_f64(
    const double* presynaptic,
    const double* teaching_signal,
    double* weights,
    double* update_output,
    double* row_l1_output,
    double* row_l2_output,
    double* row_max_abs_output,
    const LearningShape shape,
    const LearningRule rule,
    const LearningBatchReduction batch_reduction,
    const std::uint8_t* row_mask,
    const std::uint8_t* weight_mask,
    const LearningParametersF64 parameters,
    const bool apply_update,
    cudaStream_t stream) noexcept
{
    return launch_local_learning(
        presynaptic,
        teaching_signal,
        weights,
        update_output,
        row_l1_output,
        row_l2_output,
        row_max_abs_output,
        shape,
        rule,
        batch_reduction,
        row_mask,
        weight_mask,
        parameters.learning_rate,
        parameters.weight_decay,
        parameters.update_clip,
        parameters.weight_clip,
        apply_update,
        stream);
}

cudaError_t launch_hebbian_update_f32(
    const float* presynaptic,
    const float* postsynaptic,
    float* weights,
    float* update_output,
    const LearningShape shape,
    const LearningBatchReduction batch_reduction,
    const LearningParametersF32 parameters,
    const bool apply_update,
    cudaStream_t stream) noexcept
{
    return launch_local_learning_f32(
        presynaptic,
        postsynaptic,
        weights,
        update_output,
        nullptr,
        nullptr,
        nullptr,
        shape,
        LearningRule::Hebbian,
        batch_reduction,
        nullptr,
        nullptr,
        parameters,
        apply_update,
        stream);
}

cudaError_t launch_oja_update_f32(
    const float* presynaptic,
    const float* postsynaptic,
    float* weights,
    float* update_output,
    const LearningShape shape,
    const LearningBatchReduction batch_reduction,
    const LearningParametersF32 parameters,
    const bool apply_update,
    cudaStream_t stream) noexcept
{
    return launch_local_learning_f32(
        presynaptic,
        postsynaptic,
        weights,
        update_output,
        nullptr,
        nullptr,
        nullptr,
        shape,
        LearningRule::Oja,
        batch_reduction,
        nullptr,
        nullptr,
        parameters,
        apply_update,
        stream);
}

cudaError_t launch_predictive_update_f32(
    const float* presynaptic,
    const float* prediction_error,
    float* weights,
    float* update_output,
    const LearningShape shape,
    const LearningBatchReduction batch_reduction,
    const LearningParametersF32 parameters,
    const bool apply_update,
    cudaStream_t stream) noexcept
{
    return launch_local_learning_f32(
        presynaptic,
        prediction_error,
        weights,
        update_output,
        nullptr,
        nullptr,
        nullptr,
        shape,
        LearningRule::PredictiveDelta,
        batch_reduction,
        nullptr,
        nullptr,
        parameters,
        apply_update,
        stream);
}

}  // namespace prometheus::cuda::kernels
