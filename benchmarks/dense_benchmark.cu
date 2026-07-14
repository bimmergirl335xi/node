#include "kernels/dense_kernels.cuh"

#include <cuda_runtime.h>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
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
    const prometheus::cuda::kernels::DenseAlgorithm algorithm)
{
    using prometheus::cuda::kernels::DenseAlgorithm;

    switch (algorithm) {
        case DenseAlgorithm::Auto:
            return "auto";
        case DenseAlgorithm::WarpPerOutput:
            return "warp";
        case DenseAlgorithm::BlockPerOutput:
            return "block";
    }

    return "unknown";
}

float benchmark_case(
    const prometheus::cuda::kernels::DenseShape shape,
    const prometheus::cuda::kernels::DenseAlgorithm algorithm,
    const int iterations)
{
    using namespace prometheus::cuda::kernels;

    const std::size_t input_count =
        shape.batch_count * shape.input_dim;

    const std::size_t weight_count =
        shape.output_dim * shape.input_dim;

    const std::size_t output_count =
        shape.batch_count * shape.output_dim;

    float* input = nullptr;
    float* weights = nullptr;
    float* bias = nullptr;
    float* output = nullptr;

    CHECK_CUDA(cudaMalloc(&input, input_count * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&weights, weight_count * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&bias, shape.output_dim * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&output, output_count * sizeof(float)));

    CHECK_CUDA(cudaMemset(input, 1, input_count * sizeof(float)));
    CHECK_CUDA(cudaMemset(weights, 1, weight_count * sizeof(float)));
    CHECK_CUDA(cudaMemset(bias, 0, shape.output_dim * sizeof(float)));

    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;

    CHECK_CUDA(cudaEventCreate(&start));
    CHECK_CUDA(cudaEventCreate(&stop));

    for (int warmup = 0; warmup < 10; ++warmup) {
        CHECK_CUDA(
            launch_dense_f32(
                input,
                weights,
                bias,
                output,
                shape,
                algorithm,
                ActivationType::Identity,
                0.01F));
    }

    CHECK_CUDA(cudaDeviceSynchronize());
    CHECK_CUDA(cudaEventRecord(start));

    for (int iteration = 0; iteration < iterations; ++iteration) {
        CHECK_CUDA(
            launch_dense_f32(
                input,
                weights,
                bias,
                output,
                shape,
                algorithm,
                ActivationType::Identity,
                0.01F));
    }

    CHECK_CUDA(cudaEventRecord(stop));
    CHECK_CUDA(cudaEventSynchronize(stop));

    float elapsed_ms = 0.0F;
    CHECK_CUDA(cudaEventElapsedTime(&elapsed_ms, start, stop));

    CHECK_CUDA(cudaEventDestroy(stop));
    CHECK_CUDA(cudaEventDestroy(start));

    CHECK_CUDA(cudaFree(output));
    CHECK_CUDA(cudaFree(bias));
    CHECK_CUDA(cudaFree(weights));
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
    CHECK_CUDA(cudaGetDeviceProperties(&properties, device));

    std::cout
        << "# device=" << properties.name
        << " compute_capability="
        << properties.major << '.' << properties.minor
        << '\n';

    std::cout
        << "batch,input_dim,output_dim,algorithm,average_ms\n";

    const std::vector<DenseShape> cases{
        {1U, 256U, 256U},
        {1U, 1024U, 512U},
        {1U, 4096U, 1024U},
        {8U, 1024U, 1024U},
        {32U, 512U, 512U},
    };

    const std::vector<DenseAlgorithm> algorithms{
        DenseAlgorithm::Auto,
        DenseAlgorithm::WarpPerOutput,
        DenseAlgorithm::BlockPerOutput,
    };

    for (const DenseShape shape : cases) {
        for (const DenseAlgorithm algorithm : algorithms) {
            const float milliseconds =
                benchmark_case(shape, algorithm, 100);

            std::cout
                << shape.batch_count << ','
                << shape.input_dim << ','
                << shape.output_dim << ','
                << algorithm_name(algorithm) << ','
                << std::fixed << std::setprecision(6)
                << milliseconds
                << '\n';
        }
    }

    return EXIT_SUCCESS;
}
