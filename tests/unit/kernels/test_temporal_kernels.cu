#include "kernels/temporal_kernels.cuh"

#include <cuda_runtime.h>

#include <algorithm>
#include <cmath>
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
        << "CUDA failure at "
        << file << ':'
        << line << " for "
        << expression << ": "
        << cudaGetErrorString(error)
        << '\n';

    std::exit(EXIT_FAILURE);
}

#define CHECK_CUDA(expression) \
    check_cuda((expression), #expression, __FILE__, __LINE__)

void require(
    const bool condition,
    const std::string& message)
{
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

template <typename T>
bool nearly_equal(
    const T actual,
    const T expected,
    const T tolerance)
{
    const T scale =
        std::max({
            static_cast<T>(1),
            std::abs(actual),
            std::abs(expected),
        });

    return std::abs(actual - expected) <= tolerance * scale;
}

template <typename T>
void compare_vectors(
    const std::vector<T>& actual,
    const std::vector<T>& expected,
    const T tolerance,
    const std::string& label)
{
    require(
        actual.size() == expected.size(),
        label + " size mismatch");

    for (std::size_t index = 0; index < actual.size(); ++index) {
        require(
            nearly_equal(
                actual[index],
                expected[index],
                tolerance),
            label + " mismatch at index " +
                std::to_string(index));
    }
}

template <typename T>
T* allocate_and_copy(const std::vector<T>& values)
{
    T* device = nullptr;

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device),
            values.size() * sizeof(T)));

    CHECK_CUDA(
        cudaMemcpy(
            device,
            values.data(),
            values.size() * sizeof(T),
            cudaMemcpyHostToDevice));

    return device;
}

template <typename T>
std::vector<T> copy_back(
    const T* device,
    const std::size_t count)
{
    std::vector<T> values(count);

    CHECK_CUDA(
        cudaMemcpy(
            values.data(),
            device,
            count * sizeof(T),
            cudaMemcpyDeviceToHost));

    return values;
}

template <typename T>
void run_temporal_case(const T tolerance)
{
    using namespace prometheus::cuda::kernels;

    constexpr std::size_t stream_count = 2U;
    constexpr std::size_t feature_count = 4U;
    constexpr std::size_t count =
        stream_count * feature_count;

    const TemporalShape shape{
        stream_count,
        feature_count,
    };

    const std::vector<T> initial_state{
        static_cast<T>(1),
        static_cast<T>(2),
        static_cast<T>(3),
        static_cast<T>(4),
        static_cast<T>(10),
        static_cast<T>(20),
        static_cast<T>(30),
        static_cast<T>(40),
    };

    const std::vector<T> observation{
        static_cast<T>(5),
        static_cast<T>(6),
        static_cast<T>(7),
        static_cast<T>(8),
        static_cast<T>(50),
        static_cast<T>(60),
        static_cast<T>(70),
        static_cast<T>(80),
    };

    const std::vector<T> alpha{
        static_cast<T>(0.25),
        static_cast<T>(0.75),
    };

    const std::vector<std::uint8_t> stream_mask{
        1U,
        0U,
    };

    T* device_state = allocate_and_copy(initial_state);
    T* device_observation = allocate_and_copy(observation);
    T* device_alpha = allocate_and_copy(alpha);

    std::uint8_t* device_stream_mask = nullptr;
    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_stream_mask),
            stream_mask.size() * sizeof(std::uint8_t)));

    CHECK_CUDA(
        cudaMemcpy(
            device_stream_mask,
            stream_mask.data(),
            stream_mask.size() * sizeof(std::uint8_t),
            cudaMemcpyHostToDevice));

    if constexpr (std::is_same_v<T, float>) {
        CHECK_CUDA(
            launch_temporal_ema_f32(
                device_observation,
                device_state,
                shape,
                device_alpha,
                device_stream_mask,
                0.5F));
    } else {
        CHECK_CUDA(
            launch_temporal_ema_f64(
                device_observation,
                device_state,
                shape,
                device_alpha,
                device_stream_mask,
                0.5));
    }

    std::vector<T> expected_ema = initial_state;
    for (std::size_t feature = 0; feature < feature_count; ++feature) {
        expected_ema[feature] =
            initial_state[feature] +
            static_cast<T>(0.25) *
                (
                    observation[feature] -
                    initial_state[feature]
                );
    }

    compare_vectors(
        copy_back(device_state, count),
        expected_ema,
        tolerance,
        "EMA");

    const std::vector<T> elapsed{
        static_cast<T>(1),
        static_cast<T>(2),
    };

    T* device_elapsed = allocate_and_copy(elapsed);

    if constexpr (std::is_same_v<T, float>) {
        CHECK_CUDA(
            launch_temporal_decay_f32(
                device_state,
                shape,
                device_elapsed,
                nullptr,
                0.0F,
                1.0F,
                0.0F));
    } else {
        CHECK_CUDA(
            launch_temporal_decay_f64(
                device_state,
                shape,
                device_elapsed,
                nullptr,
                0.0,
                1.0,
                0.0));
    }

    std::vector<T> expected_decay = expected_ema;
    for (std::size_t feature = 0; feature < feature_count; ++feature) {
        expected_decay[feature] *= static_cast<T>(0.5);
        expected_decay[feature_count + feature] *=
            static_cast<T>(0.25);
    }

    compare_vectors(
        copy_back(device_state, count),
        expected_decay,
        tolerance,
        "decay");

    CHECK_CUDA(
        cudaMemcpy(
            device_state,
            initial_state.data(),
            count * sizeof(T),
            cudaMemcpyHostToDevice));

    T* device_previous = nullptr;
    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_previous),
            count * sizeof(T)));

    if constexpr (std::is_same_v<T, float>) {
        CHECK_CUDA(
            launch_fused_temporal_step_f32(
                device_observation,
                nullptr,
                device_elapsed,
                nullptr,
                device_previous,
                device_state,
                shape,
                0.5F,
                0.0F,
                1.0F,
                0.0F));
    } else {
        CHECK_CUDA(
            launch_fused_temporal_step_f64(
                device_observation,
                nullptr,
                device_elapsed,
                nullptr,
                device_previous,
                device_state,
                shape,
                0.5,
                0.0,
                1.0,
                0.0));
    }

    std::vector<T> expected_fused(count);

    for (std::size_t feature = 0; feature < feature_count; ++feature) {
        const T decayed_a =
            initial_state[feature] *
            static_cast<T>(0.5);

        expected_fused[feature] =
            decayed_a +
            static_cast<T>(0.5) *
                (observation[feature] - decayed_a);

        const std::size_t second =
            feature_count + feature;

        const T decayed_b =
            initial_state[second] *
            static_cast<T>(0.25);

        expected_fused[second] =
            decayed_b +
            static_cast<T>(0.5) *
                (observation[second] - decayed_b);
    }

    compare_vectors(
        copy_back(device_previous, count),
        initial_state,
        tolerance,
        "previous state");

    compare_vectors(
        copy_back(device_state, count),
        expected_fused,
        tolerance,
        "fused temporal step");

    const std::vector<T> predicted{
        static_cast<T>(4),
        static_cast<T>(7),
        static_cast<T>(7),
        static_cast<T>(10),
        static_cast<T>(40),
        static_cast<T>(65),
        static_cast<T>(75),
        static_cast<T>(100),
    };

    const std::vector<T> uncertainty(
        count,
        static_cast<T>(2));

    T* device_predicted = allocate_and_copy(predicted);
    T* device_uncertainty = allocate_and_copy(uncertainty);
    T* device_output = nullptr;

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_output),
            count * sizeof(T)));

    if constexpr (std::is_same_v<T, float>) {
        CHECK_CUDA(
            launch_prediction_error_f32(
                device_observation,
                device_predicted,
                device_output,
                count,
                PredictionErrorMode::Squared));

        std::vector<T> expected_squared(count);
        for (std::size_t index = 0; index < count; ++index) {
            const T residual =
                observation[index] - predicted[index];
            expected_squared[index] =
                residual * residual;
        }

        compare_vectors(
            copy_back(device_output, count),
            expected_squared,
            tolerance,
            "squared prediction error");

        CHECK_CUDA(
            launch_surprise_f32(
                device_observation,
                device_predicted,
                device_uncertainty,
                device_output,
                count,
                1.0e-6F));
    } else {
        CHECK_CUDA(
            launch_prediction_error_f64(
                device_observation,
                device_predicted,
                device_output,
                count,
                PredictionErrorMode::Squared));

        std::vector<T> expected_squared(count);
        for (std::size_t index = 0; index < count; ++index) {
            const T residual =
                observation[index] - predicted[index];
            expected_squared[index] =
                residual * residual;
        }

        compare_vectors(
            copy_back(device_output, count),
            expected_squared,
            tolerance,
            "squared prediction error");

        CHECK_CUDA(
            launch_surprise_f64(
                device_observation,
                device_predicted,
                device_uncertainty,
                device_output,
                count,
                1.0e-12));
    }

    std::vector<T> expected_surprise(count);
    for (std::size_t index = 0; index < count; ++index) {
        expected_surprise[index] =
            std::abs(
                observation[index] -
                predicted[index]
            ) / static_cast<T>(2);
    }

    compare_vectors(
        copy_back(device_output, count),
        expected_surprise,
        tolerance,
        "surprise");

    if constexpr (std::is_same_v<T, float>) {
        CHECK_CUDA(
            launch_temporal_reset_f32(
                device_state,
                shape,
                device_stream_mask,
                -3.0F));
    } else {
        CHECK_CUDA(
            launch_temporal_reset_f64(
                device_state,
                shape,
                device_stream_mask,
                -3.0));
    }

    std::vector<T> expected_reset = expected_fused;
    for (std::size_t feature = 0; feature < feature_count; ++feature) {
        expected_reset[feature] = static_cast<T>(-3);
    }

    compare_vectors(
        copy_back(device_state, count),
        expected_reset,
        tolerance,
        "selective reset");

    CHECK_CUDA(cudaFree(device_output));
    CHECK_CUDA(cudaFree(device_uncertainty));
    CHECK_CUDA(cudaFree(device_predicted));
    CHECK_CUDA(cudaFree(device_previous));
    CHECK_CUDA(cudaFree(device_elapsed));
    CHECK_CUDA(cudaFree(device_stream_mask));
    CHECK_CUDA(cudaFree(device_alpha));
    CHECK_CUDA(cudaFree(device_observation));
    CHECK_CUDA(cudaFree(device_state));
}

}  // namespace

int main()
{
    run_temporal_case<float>(2.0e-5F);
    run_temporal_case<double>(1.0e-10);

    std::cout
        << "Prometheus temporal CUDA kernels passed\n";

    return EXIT_SUCCESS;
}
