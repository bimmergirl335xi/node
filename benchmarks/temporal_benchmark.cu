#include "kernels/temporal_kernels.cuh"

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

struct DeviceBuffers {
    float* observation = nullptr;
    float* gate = nullptr;
    float* elapsed = nullptr;
    float* previous = nullptr;
    float* state = nullptr;
};

DeviceBuffers allocate_buffers(
    const prometheus::cuda::kernels::TemporalShape shape)
{
    const std::size_t count =
        shape.stream_count * shape.feature_count;

    DeviceBuffers buffers;

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&buffers.observation),
            count * sizeof(float)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&buffers.gate),
            count * sizeof(float)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&buffers.elapsed),
            shape.stream_count * sizeof(float)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&buffers.previous),
            count * sizeof(float)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&buffers.state),
            count * sizeof(float)));

    CHECK_CUDA(
        cudaMemset(
            buffers.observation,
            1,
            count * sizeof(float)));

    CHECK_CUDA(
        cudaMemset(
            buffers.gate,
            0,
            count * sizeof(float)));

    CHECK_CUDA(
        cudaMemset(
            buffers.elapsed,
            0,
            shape.stream_count * sizeof(float)));

    CHECK_CUDA(
        cudaMemset(
            buffers.previous,
            0,
            count * sizeof(float)));

    CHECK_CUDA(
        cudaMemset(
            buffers.state,
            0,
            count * sizeof(float)));

    return buffers;
}

void free_buffers(DeviceBuffers& buffers)
{
    CHECK_CUDA(cudaFree(buffers.state));
    CHECK_CUDA(cudaFree(buffers.previous));
    CHECK_CUDA(cudaFree(buffers.elapsed));
    CHECK_CUDA(cudaFree(buffers.gate));
    CHECK_CUDA(cudaFree(buffers.observation));
}

float benchmark_fused(
    const prometheus::cuda::kernels::TemporalShape shape,
    const int iterations)
{
    using namespace prometheus::cuda::kernels;

    DeviceBuffers buffers = allocate_buffers(shape);

    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;

    CHECK_CUDA(cudaEventCreate(&start));
    CHECK_CUDA(cudaEventCreate(&stop));

    for (int warmup = 0; warmup < 10; ++warmup) {
        CHECK_CUDA(
            launch_fused_temporal_step_f32(
                buffers.observation,
                buffers.gate,
                buffers.elapsed,
                nullptr,
                buffers.previous,
                buffers.state,
                shape,
                0.5F,
                0.033F,
                1.0F,
                0.0F));
    }

    CHECK_CUDA(cudaDeviceSynchronize());
    CHECK_CUDA(cudaEventRecord(start));

    for (int iteration = 0; iteration < iterations; ++iteration) {
        CHECK_CUDA(
            launch_fused_temporal_step_f32(
                buffers.observation,
                buffers.gate,
                buffers.elapsed,
                nullptr,
                buffers.previous,
                buffers.state,
                shape,
                0.5F,
                0.033F,
                1.0F,
                0.0F));
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

    free_buffers(buffers);

    return elapsed_ms / static_cast<float>(iterations);
}

float benchmark_split(
    const prometheus::cuda::kernels::TemporalShape shape,
    const int iterations)
{
    using namespace prometheus::cuda::kernels;

    DeviceBuffers buffers = allocate_buffers(shape);

    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;

    CHECK_CUDA(cudaEventCreate(&start));
    CHECK_CUDA(cudaEventCreate(&stop));

    for (int warmup = 0; warmup < 10; ++warmup) {
        CHECK_CUDA(
            launch_temporal_copy_f32(
                buffers.state,
                buffers.previous,
                shape.stream_count * shape.feature_count));

        CHECK_CUDA(
            launch_temporal_decay_f32(
                buffers.state,
                shape,
                buffers.elapsed,
                nullptr,
                0.033F,
                1.0F,
                0.0F));

        CHECK_CUDA(
            launch_gated_state_update_f32(
                buffers.observation,
                buffers.gate,
                buffers.state,
                shape,
                nullptr,
                0.5F));
    }

    CHECK_CUDA(cudaDeviceSynchronize());
    CHECK_CUDA(cudaEventRecord(start));

    for (int iteration = 0; iteration < iterations; ++iteration) {
        CHECK_CUDA(
            launch_temporal_copy_f32(
                buffers.state,
                buffers.previous,
                shape.stream_count * shape.feature_count));

        CHECK_CUDA(
            launch_temporal_decay_f32(
                buffers.state,
                shape,
                buffers.elapsed,
                nullptr,
                0.033F,
                1.0F,
                0.0F));

        CHECK_CUDA(
            launch_gated_state_update_f32(
                buffers.observation,
                buffers.gate,
                buffers.state,
                shape,
                nullptr,
                0.5F));
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

    free_buffers(buffers);

    return elapsed_ms / static_cast<float>(iterations);
}

}  // namespace

int main()
{
    using prometheus::cuda::kernels::TemporalShape;

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
        << "streams,features,implementation,average_ms\n";

    const std::vector<TemporalShape> cases{
        {1U, 256U},
        {2U, 512U},
        {8U, 1024U},
        {32U, 4096U},
        {128U, 8192U},
    };

    for (const TemporalShape shape : cases) {
        const float fused =
            benchmark_fused(shape, 500);

        const float split =
            benchmark_split(shape, 500);

        std::cout
            << shape.stream_count << ','
            << shape.feature_count << ','
            << "fused,"
            << std::fixed
            << std::setprecision(6)
            << fused
            << '\n';

        std::cout
            << shape.stream_count << ','
            << shape.feature_count << ','
            << "split,"
            << std::fixed
            << std::setprecision(6)
            << split
            << '\n';
    }

    return EXIT_SUCCESS;
}
