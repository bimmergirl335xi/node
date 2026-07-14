#ifndef PROMETHEUS_KERNELS_OPTIMIZER_KERNELS_CUH
#define PROMETHEUS_KERNELS_OPTIMIZER_KERNELS_CUH

#include <cuda_runtime_api.h>

#include <cstddef>
#include <cstdint>

namespace prometheus::cuda::kernels {

enum class OptimizerType : std::uint8_t {
    Sgd = 0,
    Momentum = 1,
    RmsProp = 2,
    Adam = 3,
};

enum class OptimizationDirection : std::uint8_t {
    Descent = 0,
    Ascent = 1,
};

struct OptimizerParametersF32 {
    float learning_rate = 1.0e-4F;
    float beta1 = 0.9F;
    float beta2 = 0.999F;
    float epsilon = 1.0e-8F;
    float weight_decay = 0.0F;
    float signal_scale = 1.0F;
    float signal_clip = 0.0F;
    float delta_clip = 0.0F;
    float parameter_clip = 0.0F;
};

struct OptimizerParametersF64 {
    double learning_rate = 1.0e-6;
    double beta1 = 0.9;
    double beta2 = 0.999;
    double epsilon = 1.0e-12;
    double weight_decay = 0.0;
    double signal_scale = 1.0;
    double signal_clip = 0.0;
    double delta_clip = 0.0;
    double parameter_clip = 0.0;
};

cudaError_t launch_accumulate_signal_f32(
    const float* source,
    float* accumulator,
    std::size_t count,
    float scale = 1.0F,
    const std::uint8_t* mask = nullptr,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_accumulate_signal_f64(
    const double* source,
    double* accumulator,
    std::size_t count,
    double scale = 1.0,
    const std::uint8_t* mask = nullptr,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_reset_optimizer_state_f32(
    float* first_state,
    float* second_state,
    std::size_t count,
    float value = 0.0F,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_reset_optimizer_state_f64(
    double* first_state,
    double* second_state,
    std::size_t count,
    double value = 0.0,
    cudaStream_t stream = nullptr) noexcept;

/*
 * Generic flat-parameter optimizer.
 *
 * Required state:
 *   SGD:      neither moment required
 *   Momentum: first_moment required
 *   RMSProp:  second_moment required
 *   Adam:     both moments required and step >= 1
 *
 * Shadow mode (apply_update == false) writes effective_update but leaves
 * parameters and optimizer state unchanged.
 *
 * Decoupled weight decay always moves parameters toward zero.
 */
cudaError_t launch_optimizer_step_f32(
    float* parameters,
    const float* update_signal,
    float* first_moment,
    float* second_moment,
    float* effective_update,
    std::size_t count,
    OptimizerType optimizer,
    OptimizationDirection direction,
    std::uint64_t step,
    const std::uint8_t* mask,
    OptimizerParametersF32 settings,
    bool apply_update,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_optimizer_step_f64(
    double* parameters,
    const double* update_signal,
    double* first_moment,
    double* second_moment,
    double* effective_update,
    std::size_t count,
    OptimizerType optimizer,
    OptimizationDirection direction,
    std::uint64_t step,
    const std::uint8_t* mask,
    OptimizerParametersF64 settings,
    bool apply_update,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_increment_optimizer_step(
    std::uint64_t* device_step,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_reset_optimizer_step(
    std::uint64_t* device_step,
    std::uint64_t value = 0U,
    cudaStream_t stream = nullptr) noexcept;

}  // namespace prometheus::cuda::kernels

#endif  // PROMETHEUS_KERNELS_OPTIMIZER_KERNELS_CUH
