#ifndef PROMETHEUS_KERNELS_DENSE_KERNELS_CUH
#define PROMETHEUS_KERNELS_DENSE_KERNELS_CUH

#include "kernels/activation_kernels.cuh"

#include <cuda_runtime_api.h>

#include <cstddef>
#include <cstdint>

namespace prometheus::cuda::kernels {

/*
 * The dense operation is row-major:
 *
 *   input:   [batch_count, input_dim]
 *   weights: [output_dim, input_dim]
 *   bias:    [output_dim] or nullptr
 *   output:  [batch_count, output_dim]
 */
enum class DenseAlgorithm : std::uint8_t {
    Auto = 0,
    WarpPerOutput = 1,
    BlockPerOutput = 2,
};

struct DenseShape {
    std::size_t batch_count;
    std::size_t input_dim;
    std::size_t output_dim;
};

/*
 * FP32 baseline used by Pascal, Volta, modern CUDA devices, and Kepler builds.
 */
cudaError_t launch_dense_f32(
    const float* input,
    const float* weights,
    const float* bias,
    float* output,
    DenseShape shape,
    DenseAlgorithm algorithm = DenseAlgorithm::Auto,
    ActivationType activation = ActivationType::Identity,
    float activation_parameter = 0.01F,
    cudaStream_t stream = nullptr) noexcept;

/*
 * FP64 path for hardware and workloads where precision matters.
 *
 * This is intentionally a first-class operation. The runtime scheduler can
 * benchmark and prefer hardware with strong FP64 throughput.
 */
cudaError_t launch_dense_f64(
    const double* input,
    const double* weights,
    const double* bias,
    double* output,
    DenseShape shape,
    DenseAlgorithm algorithm = DenseAlgorithm::Auto,
    ActivationType activation = ActivationType::Identity,
    double activation_parameter = 0.01,
    cudaStream_t stream = nullptr) noexcept;

/*
 * Exact integer accumulation while the result remains representable in int64.
 *
 * Each output is:
 *
 *   bias[row] + sum(input[col] * weights[row, col])
 *
 * where input and weights are int32 and accumulation/output are int64.
 *
 * This is not arbitrary-precision arithmetic. Multi-limb big-integer kernels
 * will be a separate operation family.
 */
cudaError_t launch_dense_i32_i64(
    const std::int32_t* input,
    const std::int32_t* weights,
    const std::int64_t* bias,
    std::int64_t* output,
    DenseShape shape,
    DenseAlgorithm algorithm = DenseAlgorithm::Auto,
    bool apply_relu = false,
    cudaStream_t stream = nullptr) noexcept;

/*
 * Exposes the current heuristic so benchmarking and the future kernel registry
 * can compare it with measured choices.
 */
DenseAlgorithm choose_dense_algorithm(
    DenseShape shape,
    std::size_t accumulator_bytes) noexcept;

}  // namespace prometheus::cuda::kernels

#endif  // PROMETHEUS_KERNELS_DENSE_KERNELS_CUH
