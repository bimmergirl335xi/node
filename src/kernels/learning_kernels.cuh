#ifndef PROMETHEUS_KERNELS_LEARNING_KERNELS_CUH
#define PROMETHEUS_KERNELS_LEARNING_KERNELS_CUH

#include <cuda_runtime_api.h>

#include <cstddef>
#include <cstdint>

namespace prometheus::cuda::kernels {

/*
 * Local learning rules for a row-major dense weight matrix:
 *
 *   presynaptic:    [batch_count, input_dim]
 *   teaching_signal:[batch_count, output_dim]
 *   weights:        [output_dim, input_dim]
 *
 * For Hebbian and Oja rules, teaching_signal represents postsynaptic activity.
 * For PredictiveDelta, it represents output prediction error.
 */
enum class LearningRule : std::uint8_t {
    Hebbian = 0,
    Oja = 1,
    PredictiveDelta = 2,
};

enum class LearningBatchReduction : std::uint8_t {
    Sum = 0,
    Mean = 1,
};

struct LearningShape {
    std::size_t batch_count;
    std::size_t input_dim;
    std::size_t output_dim;
};

struct LearningParametersF32 {
    float learning_rate = 1.0e-4F;
    float weight_decay = 0.0F;
    float update_clip = 0.0F;
    float weight_clip = 0.0F;
};

struct LearningParametersF64 {
    double learning_rate = 1.0e-6;
    double weight_decay = 0.0;
    double update_clip = 0.0;
    double weight_clip = 0.0;
};

/*
 * Proposed update formulas:
 *
 * Hebbian:
 *   correlation = post * pre
 *
 * Oja:
 *   correlation = post * pre - post^2 * weight
 *
 * PredictiveDelta:
 *   correlation = prediction_error * pre
 *
 * Batch samples are summed or averaged, then:
 *
 *   delta = learning_rate * (correlation - weight_decay * weight)
 *
 * update_clip > 0 clips delta.
 * weight_clip > 0 clips the proposed final weight.
 *
 * Shadow mode:
 *   apply_update == false
 *
 * In shadow mode, weights remain unchanged while update_output and statistics
 * describe the proposed effective update after clipping.
 *
 * Optional outputs:
 *
 *   update_output: [output_dim, input_dim]
 *   row_l1_output: [output_dim]
 *   row_l2_output: [output_dim]
 *   row_max_abs_output: [output_dim]
 *
 * Optional masks:
 *
 *   row_mask:    [output_dim]
 *   weight_mask: [output_dim, input_dim]
 *
 * Non-zero mask entries permit updates. Masked updates are written as zero.
 *
 * Launches are asynchronous, allocate no memory, and never synchronize.
 */
cudaError_t launch_local_learning_f32(
    const float* presynaptic,
    const float* teaching_signal,
    float* weights,
    float* update_output,
    float* row_l1_output,
    float* row_l2_output,
    float* row_max_abs_output,
    LearningShape shape,
    LearningRule rule,
    LearningBatchReduction batch_reduction,
    const std::uint8_t* row_mask,
    const std::uint8_t* weight_mask,
    LearningParametersF32 parameters,
    bool apply_update,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_local_learning_f64(
    const double* presynaptic,
    const double* teaching_signal,
    double* weights,
    double* update_output,
    double* row_l1_output,
    double* row_l2_output,
    double* row_max_abs_output,
    LearningShape shape,
    LearningRule rule,
    LearningBatchReduction batch_reduction,
    const std::uint8_t* row_mask,
    const std::uint8_t* weight_mask,
    LearningParametersF64 parameters,
    bool apply_update,
    cudaStream_t stream = nullptr) noexcept;

/*
 * Convenience wrappers.
 */
cudaError_t launch_hebbian_update_f32(
    const float* presynaptic,
    const float* postsynaptic,
    float* weights,
    float* update_output,
    LearningShape shape,
    LearningBatchReduction batch_reduction,
    LearningParametersF32 parameters,
    bool apply_update,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_oja_update_f32(
    const float* presynaptic,
    const float* postsynaptic,
    float* weights,
    float* update_output,
    LearningShape shape,
    LearningBatchReduction batch_reduction,
    LearningParametersF32 parameters,
    bool apply_update,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_predictive_update_f32(
    const float* presynaptic,
    const float* prediction_error,
    float* weights,
    float* update_output,
    LearningShape shape,
    LearningBatchReduction batch_reduction,
    LearningParametersF32 parameters,
    bool apply_update,
    cudaStream_t stream = nullptr) noexcept;

}  // namespace prometheus::cuda::kernels

#endif  // PROMETHEUS_KERNELS_LEARNING_KERNELS_CUH
