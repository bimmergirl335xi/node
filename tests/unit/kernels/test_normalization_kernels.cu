#include "kernels/normalization_kernels.cuh"

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
struct ReferenceResult {
    std::vector<T> output;
    std::vector<T> mean;
    std::vector<T> inverse_scale;
};

template <typename T>
ReferenceResult<T> reference_normalization(
    const std::vector<T>& input,
    const std::vector<T>* gamma,
    const std::vector<T>* beta,
    const std::size_t row_count,
    const std::size_t feature_count,
    const prometheus::cuda::kernels::NormalizationType type,
    const T epsilon)
{
    using prometheus::cuda::kernels::NormalizationType;

    ReferenceResult<T> result;
    result.output.resize(input.size());
    result.mean.resize(row_count);
    result.inverse_scale.resize(row_count);

    for (std::size_t row = 0; row < row_count; ++row) {
        long double mean = 0.0L;

        for (std::size_t feature = 0; feature < feature_count; ++feature) {
            mean += static_cast<long double>(
                input[row * feature_count + feature]);
        }

        mean /= static_cast<long double>(feature_count);

        long double variance = 0.0L;
        long double mean_square = 0.0L;

        for (std::size_t feature = 0; feature < feature_count; ++feature) {
            const long double value =
                static_cast<long double>(
                    input[row * feature_count + feature]);

            const long double delta = value - mean;
            variance += delta * delta;
            mean_square += value * value;
        }

        variance /= static_cast<long double>(feature_count);
        mean_square /= static_cast<long double>(feature_count);

        const long double center =
            type == NormalizationType::LayerNorm
                ? mean
                : 0.0L;

        const long double denominator =
            type == NormalizationType::LayerNorm
                ? variance
                : mean_square;

        const long double inverse_scale =
            1.0L /
            std::sqrt(
                denominator +
                static_cast<long double>(epsilon));

        result.mean[row] =
            static_cast<T>(center);

        result.inverse_scale[row] =
            static_cast<T>(inverse_scale);

        for (std::size_t feature = 0; feature < feature_count; ++feature) {
            const T value =
                input[row * feature_count + feature];

            T normalized =
                type == NormalizationType::LayerNorm
                    ? (
                        value -
                        static_cast<T>(center)
                      ) * static_cast<T>(inverse_scale)
                    : value *
                        static_cast<T>(inverse_scale);

            if (gamma != nullptr) {
                normalized *= (*gamma)[feature];
            }

            if (beta != nullptr) {
                normalized += (*beta)[feature];
            }

            result.output[row * feature_count + feature] =
                normalized;
        }
    }

    return result;
}

template <typename T>
void run_case(
    const prometheus::cuda::kernels::NormalizationType type,
    const prometheus::cuda::kernels::NormalizationAlgorithm algorithm,
    const bool affine,
    const bool in_place,
    const T epsilon,
    const T tolerance)
{
    using namespace prometheus::cuda::kernels;

    constexpr std::size_t row_count = 3U;
    constexpr std::size_t feature_count = 7U;

    const NormalizationShape shape{
        row_count,
        feature_count,
    };

    const std::vector<T> input{
        static_cast<T>(1), static_cast<T>(2), static_cast<T>(3),
        static_cast<T>(4), static_cast<T>(5), static_cast<T>(6),
        static_cast<T>(7),

        static_cast<T>(-3), static_cast<T>(-1), static_cast<T>(0),
        static_cast<T>(2), static_cast<T>(4), static_cast<T>(6),
        static_cast<T>(8),

        static_cast<T>(5), static_cast<T>(5), static_cast<T>(5),
        static_cast<T>(5), static_cast<T>(5), static_cast<T>(5),
        static_cast<T>(5),
    };

    const std::vector<T> gamma{
        static_cast<T>(0.5), static_cast<T>(0.75),
        static_cast<T>(1.0), static_cast<T>(1.25),
        static_cast<T>(1.5), static_cast<T>(1.75),
        static_cast<T>(2.0),
    };

    const std::vector<T> beta{
        static_cast<T>(-0.3), static_cast<T>(-0.2),
        static_cast<T>(-0.1), static_cast<T>(0.0),
        static_cast<T>(0.1), static_cast<T>(0.2),
        static_cast<T>(0.3),
    };

    const std::vector<T>* gamma_reference =
        affine ? &gamma : nullptr;

    const std::vector<T>* beta_reference =
        affine ? &beta : nullptr;

    const ReferenceResult<T> expected =
        reference_normalization(
            input,
            gamma_reference,
            beta_reference,
            row_count,
            feature_count,
            type,
            epsilon);

    T* device_input = nullptr;
    T* device_output = nullptr;
    T* device_gamma = nullptr;
    T* device_beta = nullptr;
    T* device_mean = nullptr;
    T* device_inverse = nullptr;

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_input),
            input.size() * sizeof(T)));

    if (!in_place) {
        CHECK_CUDA(
            cudaMalloc(
                reinterpret_cast<void**>(&device_output),
                input.size() * sizeof(T)));
    } else {
        device_output = device_input;
    }

    if (affine) {
        CHECK_CUDA(
            cudaMalloc(
                reinterpret_cast<void**>(&device_gamma),
                gamma.size() * sizeof(T)));

        CHECK_CUDA(
            cudaMalloc(
                reinterpret_cast<void**>(&device_beta),
                beta.size() * sizeof(T)));
    }

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_mean),
            row_count * sizeof(T)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_inverse),
            row_count * sizeof(T)));

    CHECK_CUDA(
        cudaMemcpy(
            device_input,
            input.data(),
            input.size() * sizeof(T),
            cudaMemcpyHostToDevice));

    if (affine) {
        CHECK_CUDA(
            cudaMemcpy(
                device_gamma,
                gamma.data(),
                gamma.size() * sizeof(T),
                cudaMemcpyHostToDevice));

        CHECK_CUDA(
            cudaMemcpy(
                device_beta,
                beta.data(),
                beta.size() * sizeof(T),
                cudaMemcpyHostToDevice));
    }

    if constexpr (std::is_same_v<T, float>) {
        CHECK_CUDA(
            launch_normalization_f32(
                device_input,
                device_gamma,
                device_beta,
                device_output,
                device_mean,
                device_inverse,
                shape,
                type,
                algorithm,
                epsilon));
    } else {
        CHECK_CUDA(
            launch_normalization_f64(
                device_input,
                device_gamma,
                device_beta,
                device_output,
                device_mean,
                device_inverse,
                shape,
                type,
                algorithm,
                epsilon));
    }

    std::vector<T> actual_output(input.size());
    std::vector<T> actual_mean(row_count);
    std::vector<T> actual_inverse(row_count);

    CHECK_CUDA(
        cudaMemcpy(
            actual_output.data(),
            device_output,
            actual_output.size() * sizeof(T),
            cudaMemcpyDeviceToHost));

    CHECK_CUDA(
        cudaMemcpy(
            actual_mean.data(),
            device_mean,
            actual_mean.size() * sizeof(T),
            cudaMemcpyDeviceToHost));

    CHECK_CUDA(
        cudaMemcpy(
            actual_inverse.data(),
            device_inverse,
            actual_inverse.size() * sizeof(T),
            cudaMemcpyDeviceToHost));

    for (std::size_t index = 0; index < actual_output.size(); ++index) {
        require(
            nearly_equal(
                actual_output[index],
                expected.output[index],
                tolerance),
            "normalization output mismatch at index " +
                std::to_string(index));
    }

    for (std::size_t row = 0; row < row_count; ++row) {
        require(
            nearly_equal(
                actual_mean[row],
                expected.mean[row],
                tolerance),
            "normalization mean mismatch at row " +
                std::to_string(row));

        require(
            nearly_equal(
                actual_inverse[row],
                expected.inverse_scale[row],
                tolerance),
            "normalization inverse scale mismatch at row " +
                std::to_string(row));
    }

    CHECK_CUDA(cudaFree(device_inverse));
    CHECK_CUDA(cudaFree(device_mean));

    if (affine) {
        CHECK_CUDA(cudaFree(device_beta));
        CHECK_CUDA(cudaFree(device_gamma));
    }

    if (!in_place) {
        CHECK_CUDA(cudaFree(device_output));
    }

    CHECK_CUDA(cudaFree(device_input));
}

}  // namespace

int main()
{
    using namespace prometheus::cuda::kernels;

    const NormalizationAlgorithm algorithms[]{
        NormalizationAlgorithm::WarpPerRow,
        NormalizationAlgorithm::BlockPerRow,
        NormalizationAlgorithm::Auto,
    };

    for (const NormalizationAlgorithm algorithm : algorithms) {
        run_case<float>(
            NormalizationType::LayerNorm,
            algorithm,
            true,
            false,
            1.0e-5F,
            2.0e-5F);

        run_case<float>(
            NormalizationType::RmsNorm,
            algorithm,
            false,
            true,
            1.0e-5F,
            2.0e-5F);

        run_case<double>(
            NormalizationType::LayerNorm,
            algorithm,
            true,
            true,
            1.0e-12,
            1.0e-10);

        run_case<double>(
            NormalizationType::RmsNorm,
            algorithm,
            false,
            false,
            1.0e-12,
            1.0e-10);
    }

    std::cout
        << "Prometheus normalization CUDA kernels passed\n";

    return EXIT_SUCCESS;
}
