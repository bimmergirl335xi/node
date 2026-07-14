#include "kernels/optimizer_kernels.cuh"

#include <cuda_runtime.h>

#include <cstdint>
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
        << "CUDA failure at " << file << ':' << line
        << " for " << expression << ": "
        << cudaGetErrorString(error) << '\n';
    std::exit(EXIT_FAILURE);
}

#define CHECK_CUDA(expression) \
    check_cuda((expression), #expression, __FILE__, __LINE__)

const char* optimizer_name(
    const prometheus::cuda::kernels::OptimizerType optimizer)
{
    using prometheus::cuda::kernels::OptimizerType;

    switch (optimizer) {
        case OptimizerType::Sgd:
            return "sgd";
        case OptimizerType::Momentum:
            return "momentum";
        case OptimizerType::RmsProp:
            return "rmsprop";
        case OptimizerType::Adam:
            return "adam";
    }

    return "unknown";
}

float benchmark_case(
    const std::size_t count,
    const prometheus::cuda::kernels::OptimizerType optimizer,
    const bool apply_update,
    const int iterations)
{
    using namespace prometheus::cuda::kernels;

    float* parameters = nullptr;
    float* signal = nullptr;
    float* first = nullptr;
    float* second = nullptr;
    float* delta = nullptr;

    CHECK_CUDA(cudaMalloc(
        reinterpret_cast<void**>(&parameters),
        count * sizeof(float)));
    CHECK_CUDA(cudaMalloc(
        reinterpret_cast<void**>(&signal),
        count * sizeof(float)));
    CHECK_CUDA(cudaMalloc(
        reinterpret_cast<void**>(&first),
        count * sizeof(float)));
    CHECK_CUDA(cudaMalloc(
        reinterpret_cast<void**>(&second),
        count * sizeof(float)));
    CHECK_CUDA(cudaMalloc(
        reinterpret_cast<void**>(&delta),
        count * sizeof(float)));

    CHECK_CUDA(cudaMemset(parameters, 0, count * sizeof(float)));
    CHECK_CUDA(cudaMemset(signal, 1, count * sizeof(float)));
    CHECK_CUDA(cudaMemset(first, 0, count * sizeof(float)));
    CHECK_CUDA(cudaMemset(second, 0, count * sizeof(float)));

    const OptimizerParametersF32 settings{
        1.0e-4F,
        0.9F,
        0.999F,
        1.0e-8F,
        1.0e-5F,
        1.0F,
        0.0F,
        0.01F,
        2.0F,
    };

    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;

    CHECK_CUDA(cudaEventCreate(&start));
    CHECK_CUDA(cudaEventCreate(&stop));

    for (int warmup = 0; warmup < 10; ++warmup) {
        CHECK_CUDA(launch_optimizer_step_f32(
            parameters,
            signal,
            first,
            second,
            delta,
            count,
            optimizer,
            OptimizationDirection::Descent,
            10U,
            nullptr,
            settings,
            apply_update));
    }

    CHECK_CUDA(cudaDeviceSynchronize());
    CHECK_CUDA(cudaEventRecord(start));

    for (int iteration = 0; iteration < iterations; ++iteration) {
        CHECK_CUDA(launch_optimizer_step_f32(
            parameters,
            signal,
            first,
            second,
            delta,
            count,
            optimizer,
            OptimizationDirection::Descent,
            static_cast<std::uint64_t>(iteration + 1),
            nullptr,
            settings,
            apply_update));
    }

    CHECK_CUDA(cudaEventRecord(stop));
    CHECK_CUDA(cudaEventSynchronize(stop));

    float elapsed_ms = 0.0F;
    CHECK_CUDA(cudaEventElapsedTime(&elapsed_ms, start, stop));

    CHECK_CUDA(cudaEventDestroy(stop));
    CHECK_CUDA(cudaEventDestroy(start));

    CHECK_CUDA(cudaFree(delta));
    CHECK_CUDA(cudaFree(second));
    CHECK_CUDA(cudaFree(first));
    CHECK_CUDA(cudaFree(signal));
    CHECK_CUDA(cudaFree(parameters));

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

    std::cout << "parameters,optimizer,mode,average_ms\n";

    const std::vector<std::size_t> counts{
        256U,
        4096U,
        65536U,
        1048576U,
        4194304U,
    };

    const OptimizerType optimizers[]{
        OptimizerType::Sgd,
        OptimizerType::Momentum,
        OptimizerType::RmsProp,
        OptimizerType::Adam,
    };

    for (const std::size_t count : counts) {
        for (const OptimizerType optimizer : optimizers) {
            const float shadow =
                benchmark_case(count, optimizer, false, 250);
            const float applied =
                benchmark_case(count, optimizer, true, 250);

            std::cout
                << count << ','
                << optimizer_name(optimizer) << ','
                << "shadow,"
                << std::fixed << std::setprecision(6)
                << shadow << '\n';

            std::cout
                << count << ','
                << optimizer_name(optimizer) << ','
                << "apply,"
                << std::fixed << std::setprecision(6)
                << applied << '\n';
        }
    }

    return EXIT_SUCCESS;
}
