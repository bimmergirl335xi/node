#include "kernels/activation_kernels.cuh"

#include <cuda_runtime.h>

#include <cmath>
#include <cstddef>

namespace prometheus::cuda::kernels {
namespace {

constexpr int kElementwiseBlockSize = 256;

__global__ void activation_kernel(
    const float* input,
    float* output,
    const std::size_t count,
    const ActivationType activation,
    const float parameter)
{
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;

    if (index >= count) {
        return;
    }

    output[index] =
        detail::apply_activation_device<float>(
            input[index],
            activation,
            parameter);
}

__global__ void residual_add_kernel(
    const float* a,
    const float* b,
    float* output,
    const std::size_t count,
    const float scale_a,
    const float scale_b)
{
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;

    if (index >= count) {
        return;
    }

    output[index] = scale_a * a[index] + scale_b * b[index];
}

__global__ void sanitize_kernel(
    const float* input,
    float* output,
    const std::size_t count,
    const float replacement)
{
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;

    if (index >= count) {
        return;
    }

    const float value = input[index];
    output[index] = isfinite(value) ? value : replacement;
}

int elementwise_grid_size(const std::size_t count) noexcept
{
    return static_cast<int>(
        (count + static_cast<std::size_t>(kElementwiseBlockSize) - 1U) /
        static_cast<std::size_t>(kElementwiseBlockSize));
}

}  // namespace

cudaError_t launch_activation_f32(
    const float* input,
    float* output,
    const std::size_t count,
    const ActivationType activation,
    const float parameter,
    cudaStream_t stream) noexcept
{
    if (
        input == nullptr ||
        output == nullptr ||
        count == 0U ||
        !std::isfinite(parameter))
    {
        return cudaErrorInvalidValue;
    }

    if (
        activation == ActivationType::LeakyRelu &&
        parameter < 0.0F)
    {
        return cudaErrorInvalidValue;
    }

    activation_kernel
        <<<elementwise_grid_size(count), kElementwiseBlockSize, 0, stream>>>(
            input,
            output,
            count,
            activation,
            parameter);

    return cudaPeekAtLastError();
}

cudaError_t launch_residual_add_f32(
    const float* a,
    const float* b,
    float* output,
    const std::size_t count,
    const float scale_a,
    const float scale_b,
    cudaStream_t stream) noexcept
{
    if (
        a == nullptr ||
        b == nullptr ||
        output == nullptr ||
        count == 0U ||
        !std::isfinite(scale_a) ||
        !std::isfinite(scale_b))
    {
        return cudaErrorInvalidValue;
    }

    residual_add_kernel
        <<<elementwise_grid_size(count), kElementwiseBlockSize, 0, stream>>>(
            a,
            b,
            output,
            count,
            scale_a,
            scale_b);

    return cudaPeekAtLastError();
}

cudaError_t launch_sanitize_f32(
    const float* input,
    float* output,
    const std::size_t count,
    const float replacement,
    cudaStream_t stream) noexcept
{
    if (
        input == nullptr ||
        output == nullptr ||
        count == 0U ||
        !std::isfinite(replacement))
    {
        return cudaErrorInvalidValue;
    }

    sanitize_kernel
        <<<elementwise_grid_size(count), kElementwiseBlockSize, 0, stream>>>(
            input,
            output,
            count,
            replacement);

    return cudaPeekAtLastError();
}

}  // namespace prometheus::cuda::kernels
