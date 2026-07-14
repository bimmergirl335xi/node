#include "kernels/dense_kernels.cuh"

#include <cuda_runtime.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace prometheus::cuda::kernels {
namespace {

constexpr int kDenseBlockSize = 256;
constexpr int kWarpSize = 32;
constexpr int kWarpsPerBlock = kDenseBlockSize / kWarpSize;

template <typename T>
__device__ __forceinline__ T warp_sum(T value) noexcept
{
    for (int offset = kWarpSize / 2; offset > 0; offset /= 2) {
#if defined(__CUDA_ARCH__) && (__CUDA_ARCH__ < 700)
        value += __shfl_down(value, offset);
#else
        value += __shfl_down_sync(0xFFFFFFFFU, value, offset);
#endif
    }

    return value;
}

template <typename T>
__device__ __forceinline__ T block_sum(T value, T* scratch) noexcept
{
    scratch[threadIdx.x] = value;
    __syncthreads();

    for (int stride = blockDim.x / 2; stride > 0; stride /= 2) {
        if (threadIdx.x < stride) {
            scratch[threadIdx.x] += scratch[threadIdx.x + stride];
        }

        __syncthreads();
    }

    return scratch[0];
}

template <typename T>
__global__ void dense_warp_kernel(
    const T* input,
    const T* weights,
    const T* bias,
    T* output,
    const std::size_t input_dim,
    const std::size_t output_dim,
    const ActivationType activation,
    const T activation_parameter)
{
    const int warp_in_block = threadIdx.x / kWarpSize;
    const int lane = threadIdx.x % kWarpSize;

    const std::size_t output_row =
        static_cast<std::size_t>(blockIdx.x) *
            static_cast<std::size_t>(kWarpsPerBlock) +
        static_cast<std::size_t>(warp_in_block);

    if (output_row >= output_dim) {
        return;
    }

    const std::size_t batch = blockIdx.y;

    const T* input_row = input + batch * input_dim;
    const T* weight_row = weights + output_row * input_dim;

    T partial = static_cast<T>(0);

    for (
        std::size_t column = static_cast<std::size_t>(lane);
        column < input_dim;
        column += static_cast<std::size_t>(kWarpSize))
    {
        partial += weight_row[column] * input_row[column];
    }

    partial = warp_sum(partial);

    if (lane == 0) {
        if (bias != nullptr) {
            partial += bias[output_row];
        }

        output[batch * output_dim + output_row] =
            detail::apply_activation_device<T>(
                partial,
                activation,
                activation_parameter);
    }
}

template <typename T>
__global__ void dense_block_kernel(
    const T* input,
    const T* weights,
    const T* bias,
    T* output,
    const std::size_t input_dim,
    const std::size_t output_dim,
    const ActivationType activation,
    const T activation_parameter)
{
    __shared__ T scratch[kDenseBlockSize];

    const std::size_t output_row = blockIdx.x;
    const std::size_t batch = blockIdx.y;

    if (output_row >= output_dim) {
        return;
    }

    const T* input_row = input + batch * input_dim;
    const T* weight_row = weights + output_row * input_dim;

    T partial = static_cast<T>(0);

    for (
        std::size_t column = threadIdx.x;
        column < input_dim;
        column += blockDim.x)
    {
        partial += weight_row[column] * input_row[column];
    }

    const T total = block_sum(partial, scratch);

    if (threadIdx.x == 0) {
        T result = total;

        if (bias != nullptr) {
            result += bias[output_row];
        }

        output[batch * output_dim + output_row] =
            detail::apply_activation_device<T>(
                result,
                activation,
                activation_parameter);
    }
}

__global__ void dense_i32_i64_warp_kernel(
    const std::int32_t* input,
    const std::int32_t* weights,
    const std::int64_t* bias,
    std::int64_t* output,
    const std::size_t input_dim,
    const std::size_t output_dim,
    const bool apply_relu)
{
    const int warp_in_block = threadIdx.x / kWarpSize;
    const int lane = threadIdx.x % kWarpSize;

    const std::size_t output_row =
        static_cast<std::size_t>(blockIdx.x) *
            static_cast<std::size_t>(kWarpsPerBlock) +
        static_cast<std::size_t>(warp_in_block);

    if (output_row >= output_dim) {
        return;
    }

    const std::size_t batch = blockIdx.y;

    const std::int32_t* input_row = input + batch * input_dim;
    const std::int32_t* weight_row = weights + output_row * input_dim;

    std::int64_t partial = 0;

    for (
        std::size_t column = static_cast<std::size_t>(lane);
        column < input_dim;
        column += static_cast<std::size_t>(kWarpSize))
    {
        partial +=
            static_cast<std::int64_t>(weight_row[column]) *
            static_cast<std::int64_t>(input_row[column]);
    }

    partial = warp_sum(partial);

    if (lane == 0) {
        if (bias != nullptr) {
            partial += bias[output_row];
        }

        if (apply_relu && partial < 0) {
            partial = 0;
        }

        output[batch * output_dim + output_row] = partial;
    }
}

__global__ void dense_i32_i64_block_kernel(
    const std::int32_t* input,
    const std::int32_t* weights,
    const std::int64_t* bias,
    std::int64_t* output,
    const std::size_t input_dim,
    const std::size_t output_dim,
    const bool apply_relu)
{
    __shared__ std::int64_t scratch[kDenseBlockSize];

    const std::size_t output_row = blockIdx.x;
    const std::size_t batch = blockIdx.y;

    if (output_row >= output_dim) {
        return;
    }

    const std::int32_t* input_row = input + batch * input_dim;
    const std::int32_t* weight_row = weights + output_row * input_dim;

    std::int64_t partial = 0;

    for (
        std::size_t column = threadIdx.x;
        column < input_dim;
        column += blockDim.x)
    {
        partial +=
            static_cast<std::int64_t>(weight_row[column]) *
            static_cast<std::int64_t>(input_row[column]);
    }

    std::int64_t total = block_sum(partial, scratch);

    if (threadIdx.x == 0) {
        if (bias != nullptr) {
            total += bias[output_row];
        }

        if (apply_relu && total < 0) {
            total = 0;
        }

        output[batch * output_dim + output_row] = total;
    }
}

bool valid_shape(const DenseShape shape) noexcept
{
    return
        shape.batch_count > 0U &&
        shape.input_dim > 0U &&
        shape.output_dim > 0U &&
        shape.batch_count <= 65535U &&
        shape.output_dim <=
            static_cast<std::size_t>(std::numeric_limits<unsigned int>::max());
}

bool valid_algorithm(const DenseAlgorithm algorithm) noexcept
{
    return
        algorithm == DenseAlgorithm::Auto ||
        algorithm == DenseAlgorithm::WarpPerOutput ||
        algorithm == DenseAlgorithm::BlockPerOutput;
}

template <typename T>
cudaError_t launch_dense_floating(
    const T* input,
    const T* weights,
    const T* bias,
    T* output,
    const DenseShape shape,
    DenseAlgorithm algorithm,
    const ActivationType activation,
    const T activation_parameter,
    cudaStream_t stream) noexcept
{
    if (
        input == nullptr ||
        weights == nullptr ||
        output == nullptr ||
        !valid_shape(shape) ||
        !valid_algorithm(algorithm) ||
        !std::isfinite(activation_parameter) ||
        (
            activation == ActivationType::LeakyRelu &&
            activation_parameter < static_cast<T>(0)
        ))
    {
        return cudaErrorInvalidValue;
    }

    if (algorithm == DenseAlgorithm::Auto) {
        algorithm =
            choose_dense_algorithm(shape, sizeof(T));
    }

    const dim3 block(kDenseBlockSize, 1U, 1U);

    if (algorithm == DenseAlgorithm::WarpPerOutput) {
        const std::size_t grid_x =
            (
                shape.output_dim +
                static_cast<std::size_t>(kWarpsPerBlock) -
                1U
            ) /
            static_cast<std::size_t>(kWarpsPerBlock);

        const dim3 grid(
            static_cast<unsigned int>(grid_x),
            static_cast<unsigned int>(shape.batch_count),
            1U);

        dense_warp_kernel<T>
            <<<grid, block, 0, stream>>>(
                input,
                weights,
                bias,
                output,
                shape.input_dim,
                shape.output_dim,
                activation,
                activation_parameter);
    } else {
        const dim3 grid(
            static_cast<unsigned int>(shape.output_dim),
            static_cast<unsigned int>(shape.batch_count),
            1U);

        dense_block_kernel<T>
            <<<grid, block, 0, stream>>>(
                input,
                weights,
                bias,
                output,
                shape.input_dim,
                shape.output_dim,
                activation,
                activation_parameter);
    }

    return cudaPeekAtLastError();
}

}  // namespace

DenseAlgorithm choose_dense_algorithm(
    const DenseShape shape,
    const std::size_t accumulator_bytes) noexcept
{
    /*
     * Starting heuristic, not a permanent truth.
     *
     * Warp-per-output generally reduces synchronization cost for modest-width
     * rows and many outputs. Block-per-output offers more lanes per long row
     * and is intentionally preferred for larger accumulators.
     */
    if (
        accumulator_bytes <= sizeof(float) &&
        shape.input_dim <= 4096U &&
        shape.output_dim >= 8U)
    {
        return DenseAlgorithm::WarpPerOutput;
    }

    if (
        accumulator_bytes <= sizeof(double) &&
        shape.input_dim <= 2048U &&
        shape.output_dim >= 16U)
    {
        return DenseAlgorithm::WarpPerOutput;
    }

    return DenseAlgorithm::BlockPerOutput;
}

cudaError_t launch_dense_f32(
    const float* input,
    const float* weights,
    const float* bias,
    float* output,
    const DenseShape shape,
    const DenseAlgorithm algorithm,
    const ActivationType activation,
    const float activation_parameter,
    cudaStream_t stream) noexcept
{
    return launch_dense_floating<float>(
        input,
        weights,
        bias,
        output,
        shape,
        algorithm,
        activation,
        activation_parameter,
        stream);
}

cudaError_t launch_dense_f64(
    const double* input,
    const double* weights,
    const double* bias,
    double* output,
    const DenseShape shape,
    const DenseAlgorithm algorithm,
    const ActivationType activation,
    const double activation_parameter,
    cudaStream_t stream) noexcept
{
    return launch_dense_floating<double>(
        input,
        weights,
        bias,
        output,
        shape,
        algorithm,
        activation,
        activation_parameter,
        stream);
}

cudaError_t launch_dense_i32_i64(
    const std::int32_t* input,
    const std::int32_t* weights,
    const std::int64_t* bias,
    std::int64_t* output,
    const DenseShape shape,
    DenseAlgorithm algorithm,
    const bool apply_relu,
    cudaStream_t stream) noexcept
{
    if (
        input == nullptr ||
        weights == nullptr ||
        output == nullptr ||
        !valid_shape(shape) ||
        !valid_algorithm(algorithm))
    {
        return cudaErrorInvalidValue;
    }

    if (algorithm == DenseAlgorithm::Auto) {
        algorithm =
            choose_dense_algorithm(shape, sizeof(std::int64_t));
    }

    const dim3 block(kDenseBlockSize, 1U, 1U);

    if (algorithm == DenseAlgorithm::WarpPerOutput) {
        const std::size_t grid_x =
            (
                shape.output_dim +
                static_cast<std::size_t>(kWarpsPerBlock) -
                1U
            ) /
            static_cast<std::size_t>(kWarpsPerBlock);

        const dim3 grid(
            static_cast<unsigned int>(grid_x),
            static_cast<unsigned int>(shape.batch_count),
            1U);

        dense_i32_i64_warp_kernel
            <<<grid, block, 0, stream>>>(
                input,
                weights,
                bias,
                output,
                shape.input_dim,
                shape.output_dim,
                apply_relu);
    } else {
        const dim3 grid(
            static_cast<unsigned int>(shape.output_dim),
            static_cast<unsigned int>(shape.batch_count),
            1U);

        dense_i32_i64_block_kernel
            <<<grid, block, 0, stream>>>(
                input,
                weights,
                bias,
                output,
                shape.input_dim,
                shape.output_dim,
                apply_relu);
    }

    return cudaPeekAtLastError();
}

}  // namespace prometheus::cuda::kernels
