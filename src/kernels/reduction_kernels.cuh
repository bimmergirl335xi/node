#ifndef PROMETHEUS_KERNELS_REDUCTION_KERNELS_CUH
#define PROMETHEUS_KERNELS_REDUCTION_KERNELS_CUH

#include <cuda_runtime_api.h>

#include <cstddef>

namespace prometheus::cuda::kernels {

/*
 * Returns the caller-owned device workspace required by the FP32 reduction
 * launchers below.
 *
 * Reusing workspace avoids cudaMalloc/cudaFree calls in the inference path.
 */
std::size_t reduction_workspace_bytes_f32(std::size_t count) noexcept;

/*
 * All reduction outputs are one FP32 value stored in device memory.
 *
 * workspace must be device-accessible storage containing at least
 * reduction_workspace_bytes_f32(count) bytes.
 *
 * Launches are asynchronous and never synchronize stream.
 */
cudaError_t launch_sum_f32(
    const float* input,
    std::size_t count,
    float* output,
    void* workspace,
    std::size_t workspace_bytes,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_mean_abs_f32(
    const float* input,
    std::size_t count,
    float* output,
    void* workspace,
    std::size_t workspace_bytes,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_l2_norm_f32(
    const float* input,
    std::size_t count,
    float* output,
    void* workspace,
    std::size_t workspace_bytes,
    cudaStream_t stream = nullptr) noexcept;

}  // namespace prometheus::cuda::kernels

#endif  // PROMETHEUS_KERNELS_REDUCTION_KERNELS_CUH
