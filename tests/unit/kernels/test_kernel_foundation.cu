#include "kernels/activation_kernels.cuh"
#include "kernels/reduction_kernels.cuh"

#include <cuda_runtime.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
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

bool nearly_equal(
    const float actual,
    const float expected,
    const float tolerance = 1.0e-5F)
{
    const float scale =
        std::max({1.0F, std::fabs(actual), std::fabs(expected)});

    return std::fabs(actual - expected) <= tolerance * scale;
}

void require(
    const bool condition,
    const char* message)
{
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main()
{
    using prometheus::cuda::kernels::ActivationType;
    using prometheus::cuda::kernels::launch_activation_f32;
    using prometheus::cuda::kernels::launch_l2_norm_f32;
    using prometheus::cuda::kernels::launch_mean_abs_f32;
    using prometheus::cuda::kernels::launch_residual_add_f32;
    using prometheus::cuda::kernels::launch_sum_f32;
    using prometheus::cuda::kernels::reduction_workspace_bytes_f32;

    const std::vector<float> host_input{
        -2.0F,
        -1.0F,
        0.0F,
        1.0F,
        2.0F,
        3.0F,
        -4.0F,
        5.0F,
    };

    const std::size_t count = host_input.size();
    const std::size_t vector_bytes = count * sizeof(float);

    float* device_input = nullptr;
    float* device_output = nullptr;
    float* device_second = nullptr;
    float* device_scalar = nullptr;
    void* device_workspace = nullptr;

    CHECK_CUDA(cudaMalloc(&device_input, vector_bytes));
    CHECK_CUDA(cudaMalloc(&device_output, vector_bytes));
    CHECK_CUDA(cudaMalloc(&device_second, vector_bytes));
    CHECK_CUDA(cudaMalloc(&device_scalar, sizeof(float)));

    const std::size_t workspace_bytes =
        reduction_workspace_bytes_f32(count);

    CHECK_CUDA(cudaMalloc(&device_workspace, workspace_bytes));

    CHECK_CUDA(
        cudaMemcpy(
            device_input,
            host_input.data(),
            vector_bytes,
            cudaMemcpyHostToDevice));

    /*
     * Leaky ReLU test.
     */
    CHECK_CUDA(
        launch_activation_f32(
            device_input,
            device_output,
            count,
            ActivationType::LeakyRelu,
            0.1F));

    std::vector<float> host_output(count);

    CHECK_CUDA(
        cudaMemcpy(
            host_output.data(),
            device_output,
            vector_bytes,
            cudaMemcpyDeviceToHost));

    const std::vector<float> expected_activation{
        -0.2F,
        -0.1F,
        0.0F,
        1.0F,
        2.0F,
        3.0F,
        -0.4F,
        5.0F,
    };

    for (std::size_t index = 0; index < count; ++index) {
        require(
            nearly_equal(
                host_output[index],
                expected_activation[index]),
            "Leaky ReLU output mismatch");
    }

    /*
     * Residual blend test:
     * second is a copy of input, so 0.5*x + 0.5*x == x.
     */
    CHECK_CUDA(
        cudaMemcpy(
            device_second,
            device_input,
            vector_bytes,
            cudaMemcpyDeviceToDevice));

    CHECK_CUDA(
        launch_residual_add_f32(
            device_input,
            device_second,
            device_output,
            count,
            0.5F,
            0.5F));

    CHECK_CUDA(
        cudaMemcpy(
            host_output.data(),
            device_output,
            vector_bytes,
            cudaMemcpyDeviceToHost));

    for (std::size_t index = 0; index < count; ++index) {
        require(
            nearly_equal(host_output[index], host_input[index]),
            "Residual add output mismatch");
    }

    auto read_scalar = [&]() {
        float value = 0.0F;

        CHECK_CUDA(
            cudaMemcpy(
                &value,
                device_scalar,
                sizeof(float),
                cudaMemcpyDeviceToHost));

        return value;
    };

    CHECK_CUDA(
        launch_sum_f32(
            device_input,
            count,
            device_scalar,
            device_workspace,
            workspace_bytes));

    require(
        nearly_equal(read_scalar(), 4.0F),
        "Sum reduction mismatch");

    CHECK_CUDA(
        launch_mean_abs_f32(
            device_input,
            count,
            device_scalar,
            device_workspace,
            workspace_bytes));

    require(
        nearly_equal(read_scalar(), 2.25F),
        "Mean absolute reduction mismatch");

    CHECK_CUDA(
        launch_l2_norm_f32(
            device_input,
            count,
            device_scalar,
            device_workspace,
            workspace_bytes));

    require(
        nearly_equal(read_scalar(), std::sqrt(60.0F)),
        "L2 reduction mismatch");

    CHECK_CUDA(cudaFree(device_workspace));
    CHECK_CUDA(cudaFree(device_scalar));
    CHECK_CUDA(cudaFree(device_second));
    CHECK_CUDA(cudaFree(device_output));
    CHECK_CUDA(cudaFree(device_input));

    std::cout << "Prometheus CUDA kernel foundation passed\n";
    return EXIT_SUCCESS;
}
