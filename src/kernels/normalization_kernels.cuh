#ifndef PROMETHEUS_KERNELS_NORMALIZATION_KERNELS_CUH
#define PROMETHEUS_KERNELS_NORMALIZATION_KERNELS_CUH

#include <cuda_runtime_api.h>

#include <cstddef>
#include <cstdint>

namespace prometheus::cuda::kernels {

enum class NormalizationType : std::uint8_t {
    LayerNorm = 0,
    RmsNorm = 1,
};

enum class NormalizationAlgorithm : std::uint8_t {
    Auto = 0,
    WarpPerRow = 1,
    BlockPerRow = 2,
};

struct NormalizationShape {
    std::size_t row_count;
    std::size_t feature_count;
};

/*
 * Row-major normalization:
 *
 *   input/output: [row_count, feature_count]
 *   gamma:        [feature_count] or nullptr
 *   beta:         [feature_count] or nullptr
 *   mean_output:  [row_count] or nullptr
 *   inv_output:   [row_count] or nullptr
 *
 * LayerNorm:
 *   normalized = (x - mean) / sqrt(variance + epsilon)
 *
 * RMSNorm:
 *   normalized = x / sqrt(mean(x^2) + epsilon)
 *
 * gamma and beta are applied after normalization when provided.
 *
 * input and output may alias. The launch is asynchronous and does not allocate
 * or synchronize. Statistics outputs remain on the device.
 */
cudaError_t launch_normalization_f32(
    const float* input,
    const float* gamma,
    const float* beta,
    float* output,
    float* mean_output,
    float* inverse_scale_output,
    NormalizationShape shape,
    NormalizationType type,
    NormalizationAlgorithm algorithm = NormalizationAlgorithm::Auto,
    float epsilon = 1.0e-5F,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_normalization_f64(
    const double* input,
    const double* gamma,
    const double* beta,
    double* output,
    double* mean_output,
    double* inverse_scale_output,
    NormalizationShape shape,
    NormalizationType type,
    NormalizationAlgorithm algorithm = NormalizationAlgorithm::Auto,
    double epsilon = 1.0e-12,
    cudaStream_t stream = nullptr) noexcept;

/*
 * Convenience wrappers.
 */
cudaError_t launch_layer_norm_f32(
    const float* input,
    const float* gamma,
    const float* beta,
    float* output,
    float* mean_output,
    float* inverse_std_output,
    NormalizationShape shape,
    NormalizationAlgorithm algorithm = NormalizationAlgorithm::Auto,
    float epsilon = 1.0e-5F,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_rms_norm_f32(
    const float* input,
    const float* gamma,
    const float* beta,
    float* output,
    float* inverse_rms_output,
    NormalizationShape shape,
    NormalizationAlgorithm algorithm = NormalizationAlgorithm::Auto,
    float epsilon = 1.0e-5F,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_layer_norm_f64(
    const double* input,
    const double* gamma,
    const double* beta,
    double* output,
    double* mean_output,
    double* inverse_std_output,
    NormalizationShape shape,
    NormalizationAlgorithm algorithm = NormalizationAlgorithm::Auto,
    double epsilon = 1.0e-12,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_rms_norm_f64(
    const double* input,
    const double* gamma,
    const double* beta,
    double* output,
    double* inverse_rms_output,
    NormalizationShape shape,
    NormalizationAlgorithm algorithm = NormalizationAlgorithm::Auto,
    double epsilon = 1.0e-12,
    cudaStream_t stream = nullptr) noexcept;

NormalizationAlgorithm choose_normalization_algorithm(
    NormalizationShape shape,
    std::size_t accumulator_bytes) noexcept;

}  // namespace prometheus::cuda::kernels

#endif  // PROMETHEUS_KERNELS_NORMALIZATION_KERNELS_CUH
