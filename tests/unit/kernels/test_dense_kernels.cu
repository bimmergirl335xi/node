#include "kernels/dense_kernels.cuh"

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

void require(const bool condition, const std::string& message)
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
            std::abs(expected)
        });

    return std::abs(actual - expected) <= tolerance * scale;
}

template <typename T>
std::vector<T> cpu_dense(
    const std::vector<T>& input,
    const std::vector<T>& weights,
    const std::vector<T>& bias,
    const std::size_t batch_count,
    const std::size_t input_dim,
    const std::size_t output_dim)
{
    std::vector<T> output(batch_count * output_dim);

    for (std::size_t batch = 0; batch < batch_count; ++batch) {
        for (std::size_t row = 0; row < output_dim; ++row) {
            T sum = bias[row];

            for (std::size_t column = 0; column < input_dim; ++column) {
                sum +=
                    input[batch * input_dim + column] *
                    weights[row * input_dim + column];
            }

            output[batch * output_dim + row] = sum;
        }
    }

    return output;
}

template <typename T>
void run_floating_case(
    const prometheus::cuda::kernels::DenseAlgorithm algorithm,
    const T tolerance)
{
    using namespace prometheus::cuda::kernels;

    constexpr std::size_t batch_count = 2U;
    constexpr std::size_t input_dim = 5U;
    constexpr std::size_t output_dim = 3U;

    const DenseShape shape{
        batch_count,
        input_dim,
        output_dim,
    };

    const std::vector<T> input{
        static_cast<T>(1), static_cast<T>(2), static_cast<T>(3),
        static_cast<T>(4), static_cast<T>(5),

        static_cast<T>(-2), static_cast<T>(1), static_cast<T>(0.5),
        static_cast<T>(3), static_cast<T>(-1),
    };

    const std::vector<T> weights{
        static_cast<T>(1), static_cast<T>(0), static_cast<T>(-1),
        static_cast<T>(0.5), static_cast<T>(2),

        static_cast<T>(-2), static_cast<T>(1), static_cast<T>(0),
        static_cast<T>(1), static_cast<T>(-0.5),

        static_cast<T>(0.25), static_cast<T>(0.5), static_cast<T>(0.75),
        static_cast<T>(1), static_cast<T>(1.25),
    };

    const std::vector<T> bias{
        static_cast<T>(0.25),
        static_cast<T>(-0.5),
        static_cast<T>(1.0),
    };

    const auto expected =
        cpu_dense(
            input,
            weights,
            bias,
            batch_count,
            input_dim,
            output_dim);

    T* device_input = nullptr;
    T* device_weights = nullptr;
    T* device_bias = nullptr;
    T* device_output = nullptr;

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_input),
            input.size() * sizeof(T)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_weights),
            weights.size() * sizeof(T)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_bias),
            bias.size() * sizeof(T)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_output),
            expected.size() * sizeof(T)));

    CHECK_CUDA(
        cudaMemcpy(
            device_input,
            input.data(),
            input.size() * sizeof(T),
            cudaMemcpyHostToDevice));

    CHECK_CUDA(
        cudaMemcpy(
            device_weights,
            weights.data(),
            weights.size() * sizeof(T),
            cudaMemcpyHostToDevice));

    CHECK_CUDA(
        cudaMemcpy(
            device_bias,
            bias.data(),
            bias.size() * sizeof(T),
            cudaMemcpyHostToDevice));

    if constexpr (std::is_same_v<T, float>) {
        CHECK_CUDA(
            launch_dense_f32(
                device_input,
                device_weights,
                device_bias,
                device_output,
                shape,
                algorithm,
                ActivationType::Identity,
                0.01F));
    } else {
        CHECK_CUDA(
            launch_dense_f64(
                device_input,
                device_weights,
                device_bias,
                device_output,
                shape,
                algorithm,
                ActivationType::Identity,
                0.01));
    }

    std::vector<T> actual(expected.size());

    CHECK_CUDA(
        cudaMemcpy(
            actual.data(),
            device_output,
            actual.size() * sizeof(T),
            cudaMemcpyDeviceToHost));

    for (std::size_t index = 0; index < actual.size(); ++index) {
        require(
            nearly_equal(actual[index], expected[index], tolerance),
            "floating dense mismatch at index " + std::to_string(index));
    }

    CHECK_CUDA(cudaFree(device_output));
    CHECK_CUDA(cudaFree(device_bias));
    CHECK_CUDA(cudaFree(device_weights));
    CHECK_CUDA(cudaFree(device_input));
}

void run_integer_case(
    const prometheus::cuda::kernels::DenseAlgorithm algorithm)
{
    using namespace prometheus::cuda::kernels;

    constexpr std::size_t batch_count = 2U;
    constexpr std::size_t input_dim = 4U;
    constexpr std::size_t output_dim = 2U;

    const DenseShape shape{
        batch_count,
        input_dim,
        output_dim,
    };

    const std::vector<std::int32_t> input{
        100000, -200000, 300000, 400000,
        -500000, 600000, 700000, -800000,
    };

    const std::vector<std::int32_t> weights{
        20000, -30000, 40000, -50000,
        -60000, 70000, 80000, 90000,
    };

    const std::vector<std::int64_t> bias{
        123456789LL,
        -987654321LL,
    };

    std::vector<std::int64_t> expected(batch_count * output_dim);

    for (std::size_t batch = 0; batch < batch_count; ++batch) {
        for (std::size_t row = 0; row < output_dim; ++row) {
            std::int64_t sum = bias[row];

            for (std::size_t column = 0; column < input_dim; ++column) {
                sum +=
                    static_cast<std::int64_t>(
                        input[batch * input_dim + column]) *
                    static_cast<std::int64_t>(
                        weights[row * input_dim + column]);
            }

            expected[batch * output_dim + row] = sum;
        }
    }

    std::int32_t* device_input = nullptr;
    std::int32_t* device_weights = nullptr;
    std::int64_t* device_bias = nullptr;
    std::int64_t* device_output = nullptr;

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_input),
            input.size() * sizeof(std::int32_t)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_weights),
            weights.size() * sizeof(std::int32_t)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_bias),
            bias.size() * sizeof(std::int64_t)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_output),
            expected.size() * sizeof(std::int64_t)));

    CHECK_CUDA(
        cudaMemcpy(
            device_input,
            input.data(),
            input.size() * sizeof(std::int32_t),
            cudaMemcpyHostToDevice));

    CHECK_CUDA(
        cudaMemcpy(
            device_weights,
            weights.data(),
            weights.size() * sizeof(std::int32_t),
            cudaMemcpyHostToDevice));

    CHECK_CUDA(
        cudaMemcpy(
            device_bias,
            bias.data(),
            bias.size() * sizeof(std::int64_t),
            cudaMemcpyHostToDevice));

    CHECK_CUDA(
        launch_dense_i32_i64(
            device_input,
            device_weights,
            device_bias,
            device_output,
            shape,
            algorithm,
            false));

    std::vector<std::int64_t> actual(expected.size());

    CHECK_CUDA(
        cudaMemcpy(
            actual.data(),
            device_output,
            actual.size() * sizeof(std::int64_t),
            cudaMemcpyDeviceToHost));

    require(actual == expected, "integer dense output mismatch");

    CHECK_CUDA(cudaFree(device_output));
    CHECK_CUDA(cudaFree(device_bias));
    CHECK_CUDA(cudaFree(device_weights));
    CHECK_CUDA(cudaFree(device_input));
}

}  // namespace

int main()
{
    using prometheus::cuda::kernels::DenseAlgorithm;

    run_floating_case<float>(
        DenseAlgorithm::WarpPerOutput,
        1.0e-5F);

    run_floating_case<float>(
        DenseAlgorithm::BlockPerOutput,
        1.0e-5F);

    run_floating_case<double>(
        DenseAlgorithm::WarpPerOutput,
        1.0e-11);

    run_floating_case<double>(
        DenseAlgorithm::BlockPerOutput,
        1.0e-11);

    run_integer_case(DenseAlgorithm::WarpPerOutput);
    run_integer_case(DenseAlgorithm::BlockPerOutput);

    std::cout << "Prometheus dense CUDA kernels passed\n";
    return EXIT_SUCCESS;
}
