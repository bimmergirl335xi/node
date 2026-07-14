#ifndef PROMETHEUS_KERNELS_TEMPORAL_KERNELS_CUH
#define PROMETHEUS_KERNELS_TEMPORAL_KERNELS_CUH

#include <cuda_runtime_api.h>

#include <cstddef>
#include <cstdint>

namespace prometheus::cuda::kernels {

struct TemporalShape {
    std::size_t stream_count;
    std::size_t feature_count;
};

enum class PredictionErrorMode : std::uint8_t {
    Residual = 0,
    Absolute = 1,
    Squared = 2,
};

/*
 * Exponential moving average:
 *
 *   state = state + alpha * (observation - state)
 *
 * alpha_by_stream may be nullptr, in which case scalar_alpha is used.
 * stream_mask may be nullptr; non-zero entries permit updates.
 */
cudaError_t launch_temporal_ema_f32(
    const float* observation,
    float* state,
    TemporalShape shape,
    const float* alpha_by_stream = nullptr,
    const std::uint8_t* stream_mask = nullptr,
    float scalar_alpha = 0.1F,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_temporal_ema_f64(
    const double* observation,
    double* state,
    TemporalShape shape,
    const double* alpha_by_stream = nullptr,
    const std::uint8_t* stream_mask = nullptr,
    double scalar_alpha = 0.1,
    cudaStream_t stream = nullptr) noexcept;

/*
 * Timestamp-aware exponential decay:
 *
 *   factor = 2^(-elapsed_seconds / half_life_seconds)
 *   state *= max(factor, minimum_multiplier)
 *
 * elapsed_seconds_by_stream may be nullptr.
 */
cudaError_t launch_temporal_decay_f32(
    float* state,
    TemporalShape shape,
    const float* elapsed_seconds_by_stream = nullptr,
    const std::uint8_t* stream_mask = nullptr,
    float scalar_elapsed_seconds = 0.0F,
    float half_life_seconds = 1.0F,
    float minimum_multiplier = 0.0F,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_temporal_decay_f64(
    double* state,
    TemporalShape shape,
    const double* elapsed_seconds_by_stream = nullptr,
    const std::uint8_t* stream_mask = nullptr,
    double scalar_elapsed_seconds = 0.0,
    double half_life_seconds = 1.0,
    double minimum_multiplier = 0.0,
    cudaStream_t stream = nullptr) noexcept;

/*
 * Gated state update:
 *
 *   state = state + gate * (candidate - state)
 *
 * gate_by_element may be nullptr, in which case scalar_gate is used.
 */
cudaError_t launch_gated_state_update_f32(
    const float* candidate,
    const float* gate_by_element,
    float* state,
    TemporalShape shape,
    const std::uint8_t* stream_mask = nullptr,
    float scalar_gate = 0.5F,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_gated_state_update_f64(
    const double* candidate,
    const double* gate_by_element,
    double* state,
    TemporalShape shape,
    const std::uint8_t* stream_mask = nullptr,
    double scalar_gate = 0.5,
    cudaStream_t stream = nullptr) noexcept;

/*
 * Fused persistent-state step:
 *
 *   previous_state = state                    (optional)
 *   decayed = state * timestamp_decay
 *   state = decayed + gate * (observation - decayed)
 *
 * This is the preferred hot-path primitive for stream-owned temporal state.
 */
cudaError_t launch_fused_temporal_step_f32(
    const float* observation,
    const float* gate_by_element,
    const float* elapsed_seconds_by_stream,
    const std::uint8_t* stream_mask,
    float* previous_state,
    float* state,
    TemporalShape shape,
    float scalar_gate = 0.5F,
    float scalar_elapsed_seconds = 0.0F,
    float half_life_seconds = 1.0F,
    float minimum_multiplier = 0.0F,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_fused_temporal_step_f64(
    const double* observation,
    const double* gate_by_element,
    const double* elapsed_seconds_by_stream,
    const std::uint8_t* stream_mask,
    double* previous_state,
    double* state,
    TemporalShape shape,
    double scalar_gate = 0.5,
    double scalar_elapsed_seconds = 0.0,
    double half_life_seconds = 1.0,
    double minimum_multiplier = 0.0,
    cudaStream_t stream = nullptr) noexcept;

/*
 * Prediction error. element_mask may be nullptr.
 * Masked elements are written as zero.
 */
cudaError_t launch_prediction_error_f32(
    const float* observed,
    const float* predicted,
    float* output,
    std::size_t count,
    PredictionErrorMode mode = PredictionErrorMode::Residual,
    const std::uint8_t* element_mask = nullptr,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_prediction_error_f64(
    const double* observed,
    const double* predicted,
    double* output,
    std::size_t count,
    PredictionErrorMode mode = PredictionErrorMode::Residual,
    const std::uint8_t* element_mask = nullptr,
    cudaStream_t stream = nullptr) noexcept;

/*
 * Surprise:
 *
 *   abs(observed - predicted) / max(abs(uncertainty), epsilon)
 *
 * uncertainty may be nullptr, yielding unscaled absolute error.
 */
cudaError_t launch_surprise_f32(
    const float* observed,
    const float* predicted,
    const float* uncertainty,
    float* surprise,
    std::size_t count,
    float epsilon = 1.0e-6F,
    const std::uint8_t* element_mask = nullptr,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_surprise_f64(
    const double* observed,
    const double* predicted,
    const double* uncertainty,
    double* surprise,
    std::size_t count,
    double epsilon = 1.0e-12,
    const std::uint8_t* element_mask = nullptr,
    cudaStream_t stream = nullptr) noexcept;

/*
 * State utilities. All are asynchronous.
 */
cudaError_t launch_temporal_copy_f32(
    const float* source,
    float* destination,
    std::size_t count,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_temporal_copy_f64(
    const double* source,
    double* destination,
    std::size_t count,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_temporal_reset_f32(
    float* state,
    TemporalShape shape,
    const std::uint8_t* stream_mask = nullptr,
    float value = 0.0F,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_temporal_reset_f64(
    double* state,
    TemporalShape shape,
    const std::uint8_t* stream_mask = nullptr,
    double value = 0.0,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_apply_stream_mask_f32(
    const float* input,
    float* output,
    TemporalShape shape,
    const std::uint8_t* stream_mask,
    float masked_value = 0.0F,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_apply_stream_mask_f64(
    const double* input,
    double* output,
    TemporalShape shape,
    const std::uint8_t* stream_mask,
    double masked_value = 0.0,
    cudaStream_t stream = nullptr) noexcept;

}  // namespace prometheus::cuda::kernels

#endif  // PROMETHEUS_KERNELS_TEMPORAL_KERNELS_CUH
