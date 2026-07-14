#include "kernels/normalization_kernels.cuh"

#include <cuda_runtime.h>

#include <cstdlib>
#include <iomanip>
#include <iostream>
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

const char* algorithm_name(
    const prometheus::cuda::kernels::NormalizationAlgorithm algorithm)
{
    using prometheus::cuda::kernels::NormalizationAlgorithm;

    switch (algorithm) {
        case NormalizationAlgorithm::Auto:
            return "auto";
        case NormalizationAlgorithm::WarpPerRow:
            return "warp";
        case NormalizationAlgorithm::BlockPerRow:
            return "block";
    }

    return "unknown";
}

const char* type_name(
    const prometheus::cuda::kernels::NormalizationType type)
{
    using prometheus::cuda::kernels::NormalizationType;

    return type == NormalizationType::LayerNorm
        ? "layer"
        : "rms";
}

float benchmark_case(
    const prometheus::cuda::kernels::NormalizationShape shape,
    const prometheus::cuda::kernels::NormalizationType type,
    const prometheus::cuda::kernels::NormalizationAlgorithm algorithm,
    const int iterations)
{
    using namespace prometheus::cuda::kernels;

    const std::size_t value_count =
        shape.row_count * shape.feature_count;

    float* input = nullptr;
    float* output = nullptr;
    float* gamma = nullptr;
    float* beta = nullptr;

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&input),
            value_count * sizeof(float)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&output),
            value_count * sizeof(float)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&gamma),
            shape.feature_count * sizeof(float)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&beta),
            shape.feature_count * sizeof(float)));

    CHECK_CUDA(
        cudaMemset(
            input,
            1,
            value_count * sizeof(float)));

    CHECK_CUDA(
        cudaMemset(
            gamma,
            1,
            shape.feature_count * sizeof(float)));

    CHECK_CUDA(
        cudaMemset(
            beta,
            0,
            shape.feature_count * sizeof(float)));

    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;

    CHECK_CUDA(cudaEventCreate(&start));
    CHECK_CUDA(cudaEventCreate(&stop));

    for (int warmup = 0; warmup < 10; ++warmup) {
        CHECK_CUDA(
            launch_normalization_f32(
                input,
                gamma,
                beta,
                output,
                nullptr,
                nullptr,
                shape,
                type,
                algorithm,
                1.0e-5F));
    }

    CHECK_CUDA(cudaDeviceSynchronize());
    CHECK_CUDA(cudaEventRecord(start));

    for (int iteration = 0; iteration < iterations; ++iteration) {
        CHECK_CUDA(
            launch_normalization_f32(
                input,
                gamma,
                beta,
                output,
                nullptr,
                nullptr,
                shape,
                type,
                algorithm,
                1.0e-5F));
    }

    CHECK_CUDA(cudaEventRecord(stop));
    CHECK_CUDA(cudaEventSynchronize(stop));

    float elapsed_ms = 0.0F;
    CHECK_CUDA(
        cudaEventElapsedTime(
            &elapsed_ms,
            start,
            stop));

    CHECK_CUDA(cudaEventDestroy(stop));
    CHECK_CUDA(cudaEventDestroy(start));

    CHECK_CUDA(cudaFree(beta));
    CHECK_CUDA(cudaFree(gamma));
    CHECK_CUDA(cudaFree(output));
    CHECK_CUDA(cudaFree(input));

    return elapsed_ms / static_cast<float>(iterations);
}

}  // namespace

int main()
{
    using namespace prometheus::cuda::kernels;

    int device = 0;
    CHECK_CUDA(cudaGetDevice(&device));

    cudaDeviceProp properties{};
    CHECK_CUDA(
        cudaGetDeviceProperties(
            &properties,
            device));

    std::cout
        << "# device=" << properties.name
        << " compute_capability="
        << properties.major
        << '.'
        << properties.minor
        << '\n';

    std::cout
        << "rows,features,type,algorithm,average_ms\n";

    const std::vector<NormalizationShape> cases{
        {1U, 128U},
        {8U, 256U},
        {32U, 512U},
        {32U, 1024U},
        {64U, 4096U},
        {256U, 8192U},
    };

    const std::vector<NormalizationType> types{
        NormalizationType::LayerNorm,
        NormalizationType::RmsNorm,
    };

    const std::vector<NormalizationAlgorithm> algorithms{
        NormalizationAlgorithm::Auto,
        NormalizationAlgorithm::WarpPerRow,
        NormalizationAlgorithm::BlockPerRow,
    };

    for (const NormalizationShape shape : cases) {
        for (const NormalizationType type : types) {
            for (
                const NormalizationAlgorithm algorithm :
                algorithms)
            {
                const float milliseconds =
                    benchmark_case(
                        shape,
                        type,
                        algorithm,
                        200);

                std::cout
                    << shape.row_count << ','
                    << shape.feature_count << ','
                    << type_name(type) << ','
                    << algorithm_name(algorithm) << ','
                    << std::fixed
                    << std::setprecision(6)
                    << milliseconds
                    << '\n';
            }
        }
    }

    return EXIT_SUCCESS;
}
