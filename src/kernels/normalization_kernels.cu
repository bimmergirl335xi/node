#include "kernels/normalization_kernels.cuh"

#include <cuda_runtime.h>

#include <cmath>
#include <cstddef>
#include <limits>

namespace prometheus::cuda::kernels {
namespace {

constexpr int kWarpSize = 32;
constexpr int kNormalizationBlockSize = 256;
constexpr int kWarpsPerBlock =
    kNormalizationBlockSize / kWarpSize;

template <typename T>
struct WelfordState {
    unsigned int count;
    T mean;
    T m2;
};

template <typename T>
__device__ __forceinline__ WelfordState<T> welford_empty() noexcept
{
    return WelfordState<T>{
        0U,
        static_cast<T>(0),
        static_cast<T>(0),
    };
}

template <typename T>
__device__ __forceinline__ WelfordState<T> welford_add(
    WelfordState<T> state,
    const T value) noexcept
{
    const unsigned int new_count = state.count + 1U;
    const T delta = value - state.mean;
    const T new_mean =
        state.mean + delta / static_cast<T>(new_count);
    const T delta_two = value - new_mean;

    state.count = new_count;
    state.mean = new_mean;
    state.m2 += delta * delta_two;

    return state;
}

template <typename T>
__device__ __forceinline__ WelfordState<T> welford_combine(
    const WelfordState<T> left,
    const WelfordState<T> right) noexcept
{
    if (left.count == 0U) {
        return right;
    }

    if (right.count == 0U) {
        return left;
    }

    const unsigned int combined_count =
        left.count + right.count;

    const T delta = right.mean - left.mean;
    const T left_weight =
        static_cast<T>(left.count);
    const T right_weight =
        static_cast<T>(right.count);
    const T total_weight =
        static_cast<T>(combined_count);

    WelfordState<T> combined{};
    combined.count = combined_count;
    combined.mean =
        left.mean + delta * right_weight / total_weight;
    combined.m2 =
        left.m2 +
        right.m2 +
        delta * delta *
            left_weight *
            right_weight /
            total_weight;

    return combined;
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
__device__ __forceinline__ T shuffle_down(
    const T value,
    const int offset) noexcept
{
#if defined(__CUDA_ARCH__) && (__CUDA_ARCH__ < 700)
    return __shfl_down(value, offset);
#else
    return __shfl_down_sync(0xFFFFFFFFU, value, offset);
#endif
}

__device__ __forceinline__ unsigned int shuffle_down(
    const unsigned int value,
    const int offset) noexcept
{
#if defined(__CUDA_ARCH__) && (__CUDA_ARCH__ < 700)
    return __shfl_down(value, offset);
#else
    return __shfl_down_sync(0xFFFFFFFFU, value, offset);
#endif
}

template <typename T>
__device__ __forceinline__ T shuffle_from_lane_zero(
    const T value) noexcept
{
#if defined(__CUDA_ARCH__) && (__CUDA_ARCH__ < 700)
    return __shfl(value, 0);
#else
    return __shfl_sync(0xFFFFFFFFU, value, 0);
#endif
}

template <typename T>
__device__ __forceinline__ WelfordState<T> warp_welford_reduce(
    WelfordState<T> state) noexcept
{
    for (int offset = kWarpSize / 2; offset > 0; offset /= 2) {
        const WelfordState<T> other{
            shuffle_down(state.count, offset),
            shuffle_down(state.mean, offset),
            shuffle_down(state.m2, offset),
        };

        state = welford_combine(state, other);
    }

    return state;
}

template <typename T>
__device__ __forceinline__ void calculate_statistics(
    const WelfordState<T> state,
    const NormalizationType type,
    const T epsilon,
    T& center,
    T& inverse_scale) noexcept
{
    const T count = static_cast<T>(state.count);
    const T variance =
        state.count > 0U
            ? state.m2 / count
            : static_cast<T>(0);

    if (type == NormalizationType::LayerNorm) {
        center = state.mean;
        const T denominator =
            variance > static_cast<T>(0)
                ? variance + epsilon
                : epsilon;

        inverse_scale =
            static_cast<T>(1) /
            device_sqrt<T>(denominator);
    } else {
        center = static_cast<T>(0);

        T mean_square =
            variance + state.mean * state.mean;

        if (mean_square < static_cast<T>(0)) {
            mean_square = static_cast<T>(0);
        }

        inverse_scale =
            static_cast<T>(1) /
            device_sqrt<T>(mean_square + epsilon);
    }
}

template <typename T>
__device__ __forceinline__ T apply_affine(
    const T normalized,
    const T* gamma,
    const T* beta,
    const std::size_t feature) noexcept
{
    T result = normalized;

    if (gamma != nullptr) {
        result *= gamma[feature];
    }

    if (beta != nullptr) {
        result += beta[feature];
    }

    return result;
}

template <typename T>
__global__ void normalization_warp_kernel(
    const T* input,
    const T* gamma,
    const T* beta,
    T* output,
    T* mean_output,
    T* inverse_scale_output,
    const std::size_t row_count,
    const std::size_t feature_count,
    const NormalizationType type,
    const T epsilon)
{
    const int warp_in_block = threadIdx.x / kWarpSize;
    const int lane = threadIdx.x % kWarpSize;

    const std::size_t row =
        static_cast<std::size_t>(blockIdx.x) *
            static_cast<std::size_t>(kWarpsPerBlock) +
        static_cast<std::size_t>(warp_in_block);

    if (row >= row_count) {
        return;
    }

    const T* input_row = input + row * feature_count;
    T* output_row = output + row * feature_count;

    WelfordState<T> local = welford_empty<T>();

    for (
        std::size_t feature =
            static_cast<std::size_t>(lane);
        feature < feature_count;
        feature += static_cast<std::size_t>(kWarpSize))
    {
        local = welford_add(local, input_row[feature]);
    }

    const WelfordState<T> reduced =
        warp_welford_reduce(local);

    T center = static_cast<T>(0);
    T inverse_scale = static_cast<T>(0);

    if (lane == 0) {
        calculate_statistics(
            reduced,
            type,
            epsilon,
            center,
            inverse_scale);

        if (mean_output != nullptr) {
            mean_output[row] = center;
        }

        if (inverse_scale_output != nullptr) {
            inverse_scale_output[row] = inverse_scale;
        }
    }

    center = shuffle_from_lane_zero(center);
    inverse_scale = shuffle_from_lane_zero(inverse_scale);

    for (
        std::size_t feature =
            static_cast<std::size_t>(lane);
        feature < feature_count;
        feature += static_cast<std::size_t>(kWarpSize))
    {
        const T normalized =
            type == NormalizationType::LayerNorm
                ? (input_row[feature] - center) * inverse_scale
                : input_row[feature] * inverse_scale;

        output_row[feature] =
            apply_affine(
                normalized,
                gamma,
                beta,
                feature);
    }
}

template <typename T>
__global__ void normalization_block_kernel(
    const T* input,
    const T* gamma,
    const T* beta,
    T* output,
    T* mean_output,
    T* inverse_scale_output,
    const std::size_t row_count,
    const std::size_t feature_count,
    const NormalizationType type,
    const T epsilon)
{
    __shared__ WelfordState<T> states[kNormalizationBlockSize];
    __shared__ T shared_center;
    __shared__ T shared_inverse_scale;

    const std::size_t row = blockIdx.x;

    if (row >= row_count) {
        return;
    }

    const T* input_row = input + row * feature_count;
    T* output_row = output + row * feature_count;

    WelfordState<T> local = welford_empty<T>();

    for (
        std::size_t feature = threadIdx.x;
        feature < feature_count;
        feature += blockDim.x)
    {
        local = welford_add(local, input_row[feature]);
    }

    states[threadIdx.x] = local;
    __syncthreads();

    for (
        int stride = blockDim.x / 2;
        stride > 0;
        stride /= 2)
    {
        if (threadIdx.x < stride) {
            states[threadIdx.x] =
                welford_combine(
                    states[threadIdx.x],
                    states[threadIdx.x + stride]);
        }

        __syncthreads();
    }

    if (threadIdx.x == 0) {
        calculate_statistics(
            states[0],
            type,
            epsilon,
            shared_center,
            shared_inverse_scale);

        if (mean_output != nullptr) {
            mean_output[row] = shared_center;
        }

        if (inverse_scale_output != nullptr) {
            inverse_scale_output[row] =
                shared_inverse_scale;
        }
    }

    __syncthreads();

    for (
        std::size_t feature = threadIdx.x;
        feature < feature_count;
        feature += blockDim.x)
    {
        const T normalized =
            type == NormalizationType::LayerNorm
                ? (
                    input_row[feature] -
                    shared_center
                  ) * shared_inverse_scale
                : input_row[feature] *
                    shared_inverse_scale;

        output_row[feature] =
            apply_affine(
                normalized,
                gamma,
                beta,
                feature);
    }
}

bool valid_shape(const NormalizationShape shape) noexcept
{
    return
        shape.row_count > 0U &&
        shape.feature_count > 0U &&
        shape.feature_count <=
            static_cast<std::size_t>(
                std::numeric_limits<unsigned int>::max()) &&
        shape.row_count <=
            static_cast<std::size_t>(
                std::numeric_limits<unsigned int>::max());
}

bool valid_type(const NormalizationType type) noexcept
{
    return
        type == NormalizationType::LayerNorm ||
        type == NormalizationType::RmsNorm;
}

bool valid_algorithm(
    const NormalizationAlgorithm algorithm) noexcept
{
    return
        algorithm == NormalizationAlgorithm::Auto ||
        algorithm == NormalizationAlgorithm::WarpPerRow ||
        algorithm == NormalizationAlgorithm::BlockPerRow;
}

template <typename T>
cudaError_t launch_normalization(
    const T* input,
    const T* gamma,
    const T* beta,
    T* output,
    T* mean_output,
    T* inverse_scale_output,
    const NormalizationShape shape,
    const NormalizationType type,
    NormalizationAlgorithm algorithm,
    const T epsilon,
    cudaStream_t stream) noexcept
{
    if (
        input == nullptr ||
        output == nullptr ||
        !valid_shape(shape) ||
        !valid_type(type) ||
        !valid_algorithm(algorithm) ||
        !std::isfinite(epsilon) ||
        epsilon <= static_cast<T>(0))
    {
        return cudaErrorInvalidValue;
    }

    if (algorithm == NormalizationAlgorithm::Auto) {
        algorithm =
            choose_normalization_algorithm(
                shape,
                sizeof(T));
    }

    if (algorithm == NormalizationAlgorithm::WarpPerRow) {
        const std::size_t grid_x =
            (
                shape.row_count +
                static_cast<std::size_t>(kWarpsPerBlock) -
                1U
            ) /
            static_cast<std::size_t>(kWarpsPerBlock);

        normalization_warp_kernel<T>
            <<<static_cast<unsigned int>(grid_x),
               kNormalizationBlockSize,
               0,
               stream>>>(
                input,
                gamma,
                beta,
                output,
                mean_output,
                inverse_scale_output,
                shape.row_count,
                shape.feature_count,
                type,
                epsilon);
    } else {
        normalization_block_kernel<T>
            <<<static_cast<unsigned int>(shape.row_count),
               kNormalizationBlockSize,
               0,
               stream>>>(
                input,
                gamma,
                beta,
                output,
                mean_output,
                inverse_scale_output,
                shape.row_count,
                shape.feature_count,
                type,
                epsilon);
    }

    return cudaPeekAtLastError();
}

}  // namespace

NormalizationAlgorithm choose_normalization_algorithm(
    const NormalizationShape shape,
    const std::size_t accumulator_bytes) noexcept
{
    /*
     * Initial heuristic only. The benchmark and future kernel registry should
     * replace this decision with measured per-device profiles.
     */
    if (
        accumulator_bytes <= sizeof(float) &&
        shape.feature_count <= 1024U &&
        shape.row_count >= 2U)
    {
        return NormalizationAlgorithm::WarpPerRow;
    }

    if (
        accumulator_bytes <= sizeof(double) &&
        shape.feature_count <= 512U &&
        shape.row_count >= 4U)
    {
        return NormalizationAlgorithm::WarpPerRow;
    }

    return NormalizationAlgorithm::BlockPerRow;
}

cudaError_t launch_normalization_f32(
    const float* input,
    const float* gamma,
    const float* beta,
    float* output,
    float* mean_output,
    float* inverse_scale_output,
    const NormalizationShape shape,
    const NormalizationType type,
    const NormalizationAlgorithm algorithm,
    const float epsilon,
    cudaStream_t stream) noexcept
{
    return launch_normalization<float>(
        input,
        gamma,
        beta,
        output,
        mean_output,
        inverse_scale_output,
        shape,
        type,
        algorithm,
        epsilon,
        stream);
}

cudaError_t launch_normalization_f64(
    const double* input,
    const double* gamma,
    const double* beta,
    double* output,
    double* mean_output,
    double* inverse_scale_output,
    const NormalizationShape shape,
    const NormalizationType type,
    const NormalizationAlgorithm algorithm,
    const double epsilon,
    cudaStream_t stream) noexcept
{
    return launch_normalization<double>(
        input,
        gamma,
        beta,
        output,
        mean_output,
        inverse_scale_output,
        shape,
        type,
        algorithm,
        epsilon,
        stream);
}

cudaError_t launch_layer_norm_f32(
    const float* input,
    const float* gamma,
    const float* beta,
    float* output,
    float* mean_output,
    float* inverse_std_output,
    const NormalizationShape shape,
    const NormalizationAlgorithm algorithm,
    const float epsilon,
    cudaStream_t stream) noexcept
{
    return launch_normalization_f32(
        input,
        gamma,
        beta,
        output,
        mean_output,
        inverse_std_output,
        shape,
        NormalizationType::LayerNorm,
        algorithm,
        epsilon,
        stream);
}

cudaError_t launch_rms_norm_f32(
    const float* input,
    const float* gamma,
    const float* beta,
    float* output,
    float* inverse_rms_output,
    const NormalizationShape shape,
    const NormalizationAlgorithm algorithm,
    const float epsilon,
    cudaStream_t stream) noexcept
{
    return launch_normalization_f32(
        input,
        gamma,
        beta,
        output,
        nullptr,
        inverse_rms_output,
        shape,
        NormalizationType::RmsNorm,
        algorithm,
        epsilon,
        stream);
}

cudaError_t launch_layer_norm_f64(
    const double* input,
    const double* gamma,
    const double* beta,
    double* output,
    double* mean_output,
    double* inverse_std_output,
    const NormalizationShape shape,
    const NormalizationAlgorithm algorithm,
    const double epsilon,
    cudaStream_t stream) noexcept
{
    return launch_normalization_f64(
        input,
        gamma,
        beta,
        output,
        mean_output,
        inverse_std_output,
        shape,
        NormalizationType::LayerNorm,
        algorithm,
        epsilon,
        stream);
}

cudaError_t launch_rms_norm_f64(
    const double* input,
    const double* gamma,
    const double* beta,
    double* output,
    double* inverse_rms_output,
    const NormalizationShape shape,
    const NormalizationAlgorithm algorithm,
    const double epsilon,
    cudaStream_t stream) noexcept
{
    return launch_normalization_f64(
        input,
        gamma,
        beta,
        output,
        nullptr,
        inverse_rms_output,
        shape,
        NormalizationType::RmsNorm,
        algorithm,
        epsilon,
        stream);
}

}  // namespace prometheus::cuda::kernels
