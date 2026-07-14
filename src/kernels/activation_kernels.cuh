#ifndef PROMETHEUS_KERNELS_ACTIVATION_KERNELS_CUH
#define PROMETHEUS_KERNELS_ACTIVATION_KERNELS_CUH

#include <cuda_runtime_api.h>

#include <cstddef>
#include <cstdint>

namespace prometheus::cuda::kernels {

enum class ActivationType : std::uint8_t {
    Identity = 0,
    Relu = 1,
    LeakyRelu = 2,
    Tanh = 3,
    Sigmoid = 4,
    Silu = 5,
    GeluTanhApprox = 6,
};

#ifdef __CUDACC__
namespace detail {

template <typename T>
__device__ __forceinline__ T device_exp(T value) noexcept;

template <>
__device__ __forceinline__ float device_exp<float>(float value) noexcept
{
    return expf(value);
}

template <>
__device__ __forceinline__ double device_exp<double>(double value) noexcept
{
    return exp(value);
}

template <typename T>
__device__ __forceinline__ T device_tanh(T value) noexcept;

template <>
__device__ __forceinline__ float device_tanh<float>(float value) noexcept
{
    return tanhf(value);
}

template <>
__device__ __forceinline__ double device_tanh<double>(double value) noexcept
{
    return tanh(value);
}

template <typename T>
__device__ __forceinline__ T apply_activation_device(
    const T value,
    const ActivationType activation,
    const T parameter) noexcept
{
    switch (activation) {
        case ActivationType::Identity:
            return value;

        case ActivationType::Relu:
            return value > static_cast<T>(0)
                ? value
                : static_cast<T>(0);

        case ActivationType::LeakyRelu:
            return value >= static_cast<T>(0)
                ? value
                : parameter * value;

        case ActivationType::Tanh:
            return device_tanh<T>(value);

        case ActivationType::Sigmoid: {
            if (value >= static_cast<T>(0)) {
                const T z = device_exp<T>(-value);
                return static_cast<T>(1) / (static_cast<T>(1) + z);
            }

            const T z = device_exp<T>(value);
            return z / (static_cast<T>(1) + z);
        }

        case ActivationType::Silu: {
            if (value >= static_cast<T>(0)) {
                const T z = device_exp<T>(-value);
                return value / (static_cast<T>(1) + z);
            }

            const T z = device_exp<T>(value);
            return value * z / (static_cast<T>(1) + z);
        }

        case ActivationType::GeluTanhApprox: {
            const T sqrt_two_over_pi =
                static_cast<T>(0.79788456080286535587989211986876);
            const T cubic_coefficient = static_cast<T>(0.044715);

            const T cubic = value * value * value;
            const T inner =
                sqrt_two_over_pi *
                (value + cubic_coefficient * cubic);

            return
                static_cast<T>(0.5) *
                value *
                (static_cast<T>(1) + device_tanh<T>(inner));
        }
    }

    return value;
}

}  // namespace detail
#endif  // __CUDACC__

cudaError_t launch_activation_f32(
    const float* input,
    float* output,
    std::size_t count,
    ActivationType activation,
    float parameter,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_residual_add_f32(
    const float* a,
    const float* b,
    float* output,
    std::size_t count,
    float scale_a = 1.0F,
    float scale_b = 1.0F,
    cudaStream_t stream = nullptr) noexcept;

cudaError_t launch_sanitize_f32(
    const float* input,
    float* output,
    std::size_t count,
    float replacement = 0.0F,
    cudaStream_t stream = nullptr) noexcept;

}  // namespace prometheus::cuda::kernels

#endif  // PROMETHEUS_KERNELS_ACTIVATION_KERNELS_CUH
