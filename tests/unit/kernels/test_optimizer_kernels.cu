#include "kernels/optimizer_kernels.cuh"

#include <cuda_runtime.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace {

void check_cuda(
    const cudaError_t error,
    const char* expression,
    const char* file,
    const int line)
{
    if (error == cudaSuccess) {
        return;
    }

    std::cerr
        << "CUDA failure at " << file << ':' << line
        << " for " << expression << ": "
        << cudaGetErrorString(error) << '\n';
    std::exit(EXIT_FAILURE);
}

#define CHECK_CUDA(expression) \
    check_cuda((expression), #expression, __FILE__, __LINE__)

void require(const bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

template <typename T>
bool nearly_equal(const T actual, const T expected, const T tolerance)
{
    const T scale =
        std::max({static_cast<T>(1), std::abs(actual), std::abs(expected)});
    return std::abs(actual - expected) <= tolerance * scale;
}

template <typename T>
void compare_vectors(
    const std::vector<T>& actual,
    const std::vector<T>& expected,
    const T tolerance,
    const std::string& label)
{
    require(actual.size() == expected.size(), label + " size mismatch");

    for (std::size_t index = 0; index < actual.size(); ++index) {
        require(
            nearly_equal(actual[index], expected[index], tolerance),
            label + " mismatch at index " + std::to_string(index));
    }
}

template <typename T>
T* allocate_and_copy(const std::vector<T>& values)
{
    T* device = nullptr;
    CHECK_CUDA(cudaMalloc(
        reinterpret_cast<void**>(&device),
        values.size() * sizeof(T)));
    CHECK_CUDA(cudaMemcpy(
        device,
        values.data(),
        values.size() * sizeof(T),
        cudaMemcpyHostToDevice));
    return device;
}

template <typename T>
std::vector<T> copy_back(const T* device, const std::size_t count)
{
    std::vector<T> values(count);
    CHECK_CUDA(cudaMemcpy(
        values.data(),
        device,
        count * sizeof(T),
        cudaMemcpyDeviceToHost));
    return values;
}

template <typename T>
T clamp_symmetric(const T value, const T limit)
{
    if (limit <= static_cast<T>(0)) {
        return value;
    }
    return std::max(-limit, std::min(value, limit));
}

template <typename T>
struct Reference {
    std::vector<T> parameters;
    std::vector<T> first;
    std::vector<T> second;
    std::vector<T> delta;
};

template <typename T>
Reference<T> reference_step(
    const std::vector<T>& parameters,
    const std::vector<T>& signal,
    const std::vector<T>& first,
    const std::vector<T>& second,
    const std::vector<std::uint8_t>& mask,
    const prometheus::cuda::kernels::OptimizerType optimizer,
    const prometheus::cuda::kernels::OptimizationDirection direction,
    const std::uint64_t step,
    const T learning_rate,
    const T beta1,
    const T beta2,
    const T epsilon,
    const T weight_decay,
    const T signal_scale,
    const T signal_clip,
    const T delta_clip,
    const T parameter_clip,
    const bool apply_update)
{
    using namespace prometheus::cuda::kernels;

    Reference<T> result{
        parameters,
        first,
        second,
        std::vector<T>(parameters.size(), static_cast<T>(0)),
    };

    const T correction1 =
        optimizer == OptimizerType::Adam
            ? static_cast<T>(1) - std::pow(beta1, static_cast<T>(step))
            : static_cast<T>(1);

    const T correction2 =
        optimizer == OptimizerType::Adam
            ? static_cast<T>(1) - std::pow(beta2, static_cast<T>(step))
            : static_cast<T>(1);

    for (std::size_t index = 0; index < parameters.size(); ++index) {
        if (mask[index] == 0U) {
            continue;
        }

        T scaled_signal =
            clamp_symmetric(signal[index] * signal_scale, signal_clip);

        T proposed_first = first[index];
        T proposed_second = second[index];
        T optimized_signal = scaled_signal;

        if (optimizer == OptimizerType::Momentum) {
            proposed_first =
                beta1 * proposed_first +
                (static_cast<T>(1) - beta1) * scaled_signal;
            optimized_signal = proposed_first;
        } else if (optimizer == OptimizerType::RmsProp) {
            proposed_second =
                beta2 * proposed_second +
                (static_cast<T>(1) - beta2) *
                    scaled_signal * scaled_signal;
            optimized_signal =
                scaled_signal / (std::sqrt(proposed_second) + epsilon);
        } else if (optimizer == OptimizerType::Adam) {
            proposed_first =
                beta1 * proposed_first +
                (static_cast<T>(1) - beta1) * scaled_signal;
            proposed_second =
                beta2 * proposed_second +
                (static_cast<T>(1) - beta2) *
                    scaled_signal * scaled_signal;
            optimized_signal =
                (proposed_first / correction1) /
                (std::sqrt(proposed_second / correction2) + epsilon);
        }

        const T sign =
            direction == OptimizationDirection::Ascent
                ? static_cast<T>(1)
                : static_cast<T>(-1);

        T proposed_delta =
            sign * learning_rate * optimized_signal -
            learning_rate * weight_decay * parameters[index];

        proposed_delta = clamp_symmetric(proposed_delta, delta_clip);

        const T proposed_parameter =
            clamp_symmetric(
                parameters[index] + proposed_delta,
                parameter_clip);

        result.delta[index] = proposed_parameter - parameters[index];

        if (apply_update) {
            result.parameters[index] = proposed_parameter;

            if (
                optimizer == OptimizerType::Momentum ||
                optimizer == OptimizerType::Adam)
            {
                result.first[index] = proposed_first;
            }

            if (
                optimizer == OptimizerType::RmsProp ||
                optimizer == OptimizerType::Adam)
            {
                result.second[index] = proposed_second;
            }
        }
    }

    return result;
}

template <typename T>
void launch_step(
    T* parameters,
    const T* signal,
    T* first,
    T* second,
    T* delta,
    const std::size_t count,
    const prometheus::cuda::kernels::OptimizerType optimizer,
    const bool apply_update,
    const std::uint8_t* mask)
{
    using namespace prometheus::cuda::kernels;

    if constexpr (std::is_same_v<T, float>) {
        CHECK_CUDA(launch_optimizer_step_f32(
            parameters,
            signal,
            first,
            second,
            delta,
            count,
            optimizer,
            OptimizationDirection::Ascent,
            3U,
            mask,
            OptimizerParametersF32{
                0.02F, 0.8F, 0.9F, 1.0e-6F,
                0.01F, 0.5F, 1.25F, 0.03F, 0.9F,
            },
            apply_update));
    } else {
        CHECK_CUDA(launch_optimizer_step_f64(
            parameters,
            signal,
            first,
            second,
            delta,
            count,
            optimizer,
            OptimizationDirection::Ascent,
            3U,
            mask,
            OptimizerParametersF64{
                0.02, 0.8, 0.9, 1.0e-12,
                0.01, 0.5, 1.25, 0.03, 0.9,
            },
            apply_update));
    }
}

template <typename T>
void run_accumulation_case(const T tolerance)
{
    using namespace prometheus::cuda::kernels;

    const std::vector<T> first{
        static_cast<T>(1), static_cast<T>(2),
        static_cast<T>(3), static_cast<T>(4),
    };
    const std::vector<T> second{
        static_cast<T>(-2), static_cast<T>(4),
        static_cast<T>(6), static_cast<T>(-8),
    };
    const std::vector<std::uint8_t> mask{1U, 0U, 1U, 1U};
    const std::vector<T> zeros(first.size(), static_cast<T>(0));

    T* d_first = allocate_and_copy(first);
    T* d_second = allocate_and_copy(second);
    T* d_accumulator = allocate_and_copy(zeros);

    std::uint8_t* d_mask = nullptr;
    CHECK_CUDA(cudaMalloc(
        reinterpret_cast<void**>(&d_mask),
        mask.size() * sizeof(std::uint8_t)));
    CHECK_CUDA(cudaMemcpy(
        d_mask,
        mask.data(),
        mask.size() * sizeof(std::uint8_t),
        cudaMemcpyHostToDevice));

    if constexpr (std::is_same_v<T, float>) {
        CHECK_CUDA(launch_accumulate_signal_f32(
            d_first, d_accumulator, first.size(), 1.0F, d_mask));
        CHECK_CUDA(launch_accumulate_signal_f32(
            d_second, d_accumulator, second.size(), 0.5F, d_mask));
    } else {
        CHECK_CUDA(launch_accumulate_signal_f64(
            d_first, d_accumulator, first.size(), 1.0, d_mask));
        CHECK_CUDA(launch_accumulate_signal_f64(
            d_second, d_accumulator, second.size(), 0.5, d_mask));
    }

    const std::vector<T> expected{
        static_cast<T>(0),
        static_cast<T>(0),
        static_cast<T>(6),
        static_cast<T>(0),
    };

    compare_vectors(
        copy_back(d_accumulator, expected.size()),
        expected,
        tolerance,
        "accumulation");

    CHECK_CUDA(cudaFree(d_mask));
    CHECK_CUDA(cudaFree(d_accumulator));
    CHECK_CUDA(cudaFree(d_second));
    CHECK_CUDA(cudaFree(d_first));
}

template <typename T>
void run_optimizer_case(
    const prometheus::cuda::kernels::OptimizerType optimizer,
    const bool apply_update,
    const T tolerance)
{
    using namespace prometheus::cuda::kernels;

    const std::vector<T> parameters{
        static_cast<T>(0.2), static_cast<T>(-0.4),
        static_cast<T>(0.6), static_cast<T>(-0.8),
        static_cast<T>(1.0),
    };
    const std::vector<T> signal{
        static_cast<T>(0.5), static_cast<T>(-1.0),
        static_cast<T>(2.0), static_cast<T>(-4.0),
        static_cast<T>(0.25),
    };
    const std::vector<T> first{
        static_cast<T>(0.1), static_cast<T>(0.2),
        static_cast<T>(-0.1), static_cast<T>(0.0),
        static_cast<T>(0.05),
    };
    const std::vector<T> second{
        static_cast<T>(0.5), static_cast<T>(0.25),
        static_cast<T>(1.0), static_cast<T>(2.0),
        static_cast<T>(0.125),
    };
    const std::vector<std::uint8_t> mask{1U, 1U, 0U, 1U, 1U};

    const auto expected = reference_step(
        parameters,
        signal,
        first,
        second,
        mask,
        optimizer,
        OptimizationDirection::Ascent,
        3U,
        static_cast<T>(0.02),
        static_cast<T>(0.8),
        static_cast<T>(0.9),
        std::is_same_v<T, float>
            ? static_cast<T>(1.0e-6F)
            : static_cast<T>(1.0e-12),
        static_cast<T>(0.01),
        static_cast<T>(0.5),
        static_cast<T>(1.25),
        static_cast<T>(0.03),
        static_cast<T>(0.9),
        apply_update);

    T* d_parameters = allocate_and_copy(parameters);
    T* d_signal = allocate_and_copy(signal);
    T* d_first = allocate_and_copy(first);
    T* d_second = allocate_and_copy(second);

    T* d_delta = nullptr;
    CHECK_CUDA(cudaMalloc(
        reinterpret_cast<void**>(&d_delta),
        parameters.size() * sizeof(T)));

    std::uint8_t* d_mask = nullptr;
    CHECK_CUDA(cudaMalloc(
        reinterpret_cast<void**>(&d_mask),
        mask.size() * sizeof(std::uint8_t)));
    CHECK_CUDA(cudaMemcpy(
        d_mask,
        mask.data(),
        mask.size() * sizeof(std::uint8_t),
        cudaMemcpyHostToDevice));

    launch_step(
        d_parameters,
        d_signal,
        d_first,
        d_second,
        d_delta,
        parameters.size(),
        optimizer,
        apply_update,
        d_mask);

    compare_vectors(
        copy_back(d_parameters, parameters.size()),
        expected.parameters,
        tolerance,
        "parameters");
    compare_vectors(
        copy_back(d_first, first.size()),
        expected.first,
        tolerance,
        "first moment");
    compare_vectors(
        copy_back(d_second, second.size()),
        expected.second,
        tolerance,
        "second moment");
    compare_vectors(
        copy_back(d_delta, parameters.size()),
        expected.delta,
        tolerance,
        "effective delta");

    CHECK_CUDA(cudaFree(d_mask));
    CHECK_CUDA(cudaFree(d_delta));
    CHECK_CUDA(cudaFree(d_second));
    CHECK_CUDA(cudaFree(d_first));
    CHECK_CUDA(cudaFree(d_signal));
    CHECK_CUDA(cudaFree(d_parameters));
}

void run_step_counter_case()
{
    using namespace prometheus::cuda::kernels;

    std::uint64_t* d_step = nullptr;
    CHECK_CUDA(cudaMalloc(
        reinterpret_cast<void**>(&d_step),
        sizeof(std::uint64_t)));

    CHECK_CUDA(launch_reset_optimizer_step(d_step, 7U));
    CHECK_CUDA(launch_increment_optimizer_step(d_step));

    std::uint64_t host_step = 0U;
    CHECK_CUDA(cudaMemcpy(
        &host_step,
        d_step,
        sizeof(std::uint64_t),
        cudaMemcpyDeviceToHost));

    require(host_step == 8U, "step counter mismatch");
    CHECK_CUDA(cudaFree(d_step));
}

}  // namespace

int main()
{
    using prometheus::cuda::kernels::OptimizerType;

    run_accumulation_case<float>(2.0e-6F);
    run_accumulation_case<double>(1.0e-12);

    const OptimizerType optimizers[]{
        OptimizerType::Sgd,
        OptimizerType::Momentum,
        OptimizerType::RmsProp,
        OptimizerType::Adam,
    };

    for (const OptimizerType optimizer : optimizers) {
        run_optimizer_case<float>(optimizer, false, 4.0e-5F);
        run_optimizer_case<float>(optimizer, true, 4.0e-5F);
        run_optimizer_case<double>(optimizer, false, 1.0e-10);
        run_optimizer_case<double>(optimizer, true, 1.0e-10);
    }

    run_step_counter_case();

    std::cout << "Prometheus optimizer CUDA kernels passed\n";
    return EXIT_SUCCESS;
}
