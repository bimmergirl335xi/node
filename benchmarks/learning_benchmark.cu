#include "kernels/learning_kernels.cuh"

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

const char* rule_name(
    const prometheus::cuda::kernels::LearningRule rule)
{
    using prometheus::cuda::kernels::LearningRule;

    switch (rule) {
        case LearningRule::Hebbian:
            return "hebbian";
        case LearningRule::Oja:
            return "oja";
        case LearningRule::PredictiveDelta:
            return "predictive";
    }

    return "unknown";
}

struct DeviceBuffers {
    float* presynaptic = nullptr;
    float* teaching = nullptr;
    float* weights = nullptr;
    float* updates = nullptr;
    float* row_l1 = nullptr;
    float* row_l2 = nullptr;
    float* row_max = nullptr;
};

DeviceBuffers allocate_buffers(
    const prometheus::cuda::kernels::LearningShape shape)
{
    DeviceBuffers buffers;

    const std::size_t presynaptic_count =
        shape.batch_count * shape.input_dim;

    const std::size_t teaching_count =
        shape.batch_count * shape.output_dim;

    const std::size_t weight_count =
        shape.output_dim * shape.input_dim;

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&buffers.presynaptic),
            presynaptic_count * sizeof(float)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&buffers.teaching),
            teaching_count * sizeof(float)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&buffers.weights),
            weight_count * sizeof(float)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&buffers.updates),
            weight_count * sizeof(float)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&buffers.row_l1),
            shape.output_dim * sizeof(float)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&buffers.row_l2),
            shape.output_dim * sizeof(float)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&buffers.row_max),
            shape.output_dim * sizeof(float)));

    CHECK_CUDA(
        cudaMemset(
            buffers.presynaptic,
            1,
            presynaptic_count * sizeof(float)));

    CHECK_CUDA(
        cudaMemset(
            buffers.teaching,
            1,
            teaching_count * sizeof(float)));

    CHECK_CUDA(
        cudaMemset(
            buffers.weights,
            0,
            weight_count * sizeof(float)));

    return buffers;
}

void free_buffers(DeviceBuffers& buffers)
{
    CHECK_CUDA(cudaFree(buffers.row_max));
    CHECK_CUDA(cudaFree(buffers.row_l2));
    CHECK_CUDA(cudaFree(buffers.row_l1));
    CHECK_CUDA(cudaFree(buffers.updates));
    CHECK_CUDA(cudaFree(buffers.weights));
    CHECK_CUDA(cudaFree(buffers.teaching));
    CHECK_CUDA(cudaFree(buffers.presynaptic));
}

float benchmark_case(
    const prometheus::cuda::kernels::LearningShape shape,
    const prometheus::cuda::kernels::LearningRule rule,
    const bool apply_update,
    const int iterations)
{
    using namespace prometheus::cuda::kernels;

    DeviceBuffers buffers = allocate_buffers(shape);

    const LearningParametersF32 parameters{
        1.0e-4F,
        1.0e-5F,
        0.01F,
        2.0F,
    };

    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;

    CHECK_CUDA(cudaEventCreate(&start));
    CHECK_CUDA(cudaEventCreate(&stop));

    for (int warmup = 0; warmup < 10; ++warmup) {
        CHECK_CUDA(
            launch_local_learning_f32(
                buffers.presynaptic,
                buffers.teaching,
                buffers.weights,
                buffers.updates,
                buffers.row_l1,
                buffers.row_l2,
                buffers.row_max,
                shape,
                rule,
                LearningBatchReduction::Mean,
                nullptr,
                nullptr,
                parameters,
                apply_update));
    }

    CHECK_CUDA(cudaDeviceSynchronize());
    CHECK_CUDA(cudaEventRecord(start));

    for (int iteration = 0; iteration < iterations; ++iteration) {
        CHECK_CUDA(
            launch_local_learning_f32(
                buffers.presynaptic,
                buffers.teaching,
                buffers.weights,
                buffers.updates,
                buffers.row_l1,
                buffers.row_l2,
                buffers.row_max,
                shape,
                rule,
                LearningBatchReduction::Mean,
                nullptr,
                nullptr,
                parameters,
                apply_update));
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
        << "batch,input_dim,output_dim,rule,mode,average_ms\n";

    const std::vector<LearningShape> cases{
        {1U, 256U, 256U},
        {8U, 512U, 512U},
        {16U, 1024U, 512U},
        {32U, 2048U, 1024U},
        {64U, 4096U, 1024U},
    };

    const LearningRule rules[]{
        LearningRule::Hebbian,
        LearningRule::Oja,
        LearningRule::PredictiveDelta,
    };

    for (const LearningShape shape : cases) {
        for (const LearningRule rule : rules) {
            const float shadow =
                benchmark_case(
                    shape,
                    rule,
                    false,
                    100);

            const float applied =
                benchmark_case(
                    shape,
                    rule,
                    true,
                    100);

            std::cout
                << shape.batch_count << ','
                << shape.input_dim << ','
                << shape.output_dim << ','
                << rule_name(rule) << ','
                << "shadow,"
                << std::fixed
                << std::setprecision(6)
                << shadow
                << '\n';

            std::cout
                << shape.batch_count << ','
                << shape.input_dim << ','
                << shape.output_dim << ','
                << rule_name(rule) << ','
                << "apply,"
                << std::fixed
                << std::setprecision(6)
                << applied
                << '\n';
        }
    }

    return EXIT_SUCCESS;
}
