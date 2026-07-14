#include "kernels/reduction_kernels.cuh"

#include <cuda_runtime.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace prometheus::cuda::kernels {
namespace {

constexpr int kReductionBlockSize = 256;
constexpr int kMaximumReductionBlocks = 1024;
constexpr unsigned int kFullWarpMask = 0xFFFFFFFFU;

enum class Transform : int {
    Identity,
    AbsoluteValue,
    Square,
};

enum class Finalize : int {
    Identity,
    Mean,
    SquareRoot,
};

__device__ __forceinline__ float warp_reduce_sum(float value) noexcept
{
    for (int offset = warpSize / 2; offset > 0; offset /= 2) {
        value += __shfl_down_sync(kFullWarpMask, value, offset);
    }

    return value;
}

__device__ __forceinline__ float block_reduce_sum(float value) noexcept
{
    __shared__ float warp_sums[32];

    const int lane = threadIdx.x & (warpSize - 1);
    const int warp = threadIdx.x / warpSize;

    value = warp_reduce_sum(value);

    if (lane == 0) {
        warp_sums[warp] = value;
    }

    __syncthreads();

    const int warp_count =
        (blockDim.x + warpSize - 1) / warpSize;

    value =
        (threadIdx.x < warp_count)
            ? warp_sums[lane]
            : 0.0F;

    if (warp == 0) {
        value = warp_reduce_sum(value);
    }

    return value;
}

template <Transform Operation>
__device__ __forceinline__ float transform_value(
    const float value) noexcept
{
    if constexpr (Operation == Transform::Identity) {
        return value;
    } else if constexpr (Operation == Transform::AbsoluteValue) {
        return fabsf(value);
    } else {
        return value * value;
    }
}

template <Transform Operation>
__global__ void reduction_stage_one_kernel(
    const float* input,
    const std::size_t count,
    float* partials)
{
    const std::size_t block_base =
        static_cast<std::size_t>(blockIdx.x) *
        static_cast<std::size_t>(blockDim.x) *
        2U;

    std::size_t index = block_base + threadIdx.x;

    const std::size_t stride =
        static_cast<std::size_t>(gridDim.x) *
        static_cast<std::size_t>(blockDim.x) *
        2U;

    float local_sum = 0.0F;

    while (index < count) {
        local_sum += transform_value<Operation>(input[index]);

        const std::size_t second = index + blockDim.x;
        if (second < count) {
            local_sum += transform_value<Operation>(input[second]);
        }

        index += stride;
    }

    const float block_sum = block_reduce_sum(local_sum);

    if (threadIdx.x == 0) {
        partials[blockIdx.x] = block_sum;
    }
}

template <Finalize FinalOperation>
__global__ void reduction_finalize_kernel(
    const float* partials,
    const int partial_count,
    const std::size_t original_count,
    float* output)
{
    float local_sum = 0.0F;

    for (
        int index = threadIdx.x;
        index < partial_count;
        index += blockDim.x)
    {
        local_sum += partials[index];
    }

    float total = block_reduce_sum(local_sum);

    if (threadIdx.x != 0) {
        return;
    }

    if constexpr (FinalOperation == Finalize::Mean) {
        total /= static_cast<float>(original_count);
    } else if constexpr (FinalOperation == Finalize::SquareRoot) {
        total = sqrtf(fmaxf(total, 0.0F));
    }

    *output = total;
}

int reduction_block_count(const std::size_t count) noexcept
{
    const std::size_t elements_per_block =
        static_cast<std::size_t>(kReductionBlockSize) * 2U;

    const std::size_t requested =
        (count + elements_per_block - 1U) / elements_per_block;

    return static_cast<int>(
        std::clamp<std::size_t>(
            requested,
            1U,
            static_cast<std::size_t>(kMaximumReductionBlocks)));
}

template <Transform Operation, Finalize FinalOperation>
cudaError_t launch_reduction(
    const float* input,
    const std::size_t count,
    float* output,
    void* workspace,
    const std::size_t workspace_bytes,
    cudaStream_t stream) noexcept
{
    if (
        input == nullptr ||
        output == nullptr ||
        workspace == nullptr ||
        count == 0U)
    {
        return cudaErrorInvalidValue;
    }

    const std::size_t required =
        reduction_workspace_bytes_f32(count);

    if (workspace_bytes < required) {
        return cudaErrorInvalidValue;
    }

    const int blocks = reduction_block_count(count);
    auto* partials = static_cast<float*>(workspace);

    reduction_stage_one_kernel<Operation>
        <<<blocks, kReductionBlockSize, 0, stream>>>(
            input,
            count,
            partials);

    cudaError_t error = cudaPeekAtLastError();
    if (error != cudaSuccess) {
        return error;
    }

    reduction_finalize_kernel<FinalOperation>
        <<<1, kReductionBlockSize, 0, stream>>>(
            partials,
            blocks,
            count,
            output);

    return cudaPeekAtLastError();
}

}  // namespace

std::size_t reduction_workspace_bytes_f32(
    const std::size_t count) noexcept
{
    if (count == 0U) {
        return 0U;
    }

    return
        static_cast<std::size_t>(reduction_block_count(count)) *
        sizeof(float);
}

cudaError_t launch_sum_f32(
    const float* input,
    const std::size_t count,
    float* output,
    void* workspace,
    const std::size_t workspace_bytes,
    cudaStream_t stream) noexcept
{
    return launch_reduction<Transform::Identity, Finalize::Identity>(
        input,
        count,
        output,
        workspace,
        workspace_bytes,
        stream);
}

cudaError_t launch_mean_abs_f32(
    const float* input,
    const std::size_t count,
    float* output,
    void* workspace,
    const std::size_t workspace_bytes,
    cudaStream_t stream) noexcept
{
    return launch_reduction<Transform::AbsoluteValue, Finalize::Mean>(
        input,
        count,
        output,
        workspace,
        workspace_bytes,
        stream);
}

cudaError_t launch_l2_norm_f32(
    const float* input,
    const std::size_t count,
    float* output,
    void* workspace,
    const std::size_t workspace_bytes,
    cudaStream_t stream) noexcept
{
    return launch_reduction<Transform::Square, Finalize::SquareRoot>(
        input,
        count,
        output,
        workspace,
        workspace_bytes,
        stream);
}

}  // namespace prometheus::cuda::kernels
