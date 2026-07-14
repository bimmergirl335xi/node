#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "backend_registry.hpp"
#include "cuda_backend.hpp"
#include "cpu_backend.hpp"

namespace pb = prometheus::backends;
namespace pc = prometheus::backends::cuda;
namespace p_cpu = prometheus::backends::cpu;

int main() {
    auto backend = std::make_shared<pc::CudaBackend>();
    auto cpu_backend = std::make_shared<p_cpu::CpuBackend>();
    pb::BackendRegistry registry{};
    if (!registry.register_backend(backend).ok()) {
        return EXIT_FAILURE;
    }
    if (registry.register_backend(backend).code !=
        pb::BackendRegistrationCode::duplicate_backend_id) {
        return EXIT_FAILURE;
    }
    if (!registry.register_backend(cpu_backend).ok() || registry.size() != 2) {
        return EXIT_FAILURE;
    }

    const pb::ComputeBackendStatus cpu_status = cpu_backend->initialize();
    if (!cpu_status.usable) {
        return EXIT_FAILURE;
    }

    const pb::ComputeBackendStatus status = backend->initialize();
    if (!status.usable) {
        return EXIT_FAILURE;
    }

    pc::CudaKernelDescriptor descriptor{};
    descriptor.kernel_id = "test.backend_registry.fp32";
    descriptor.family = pc::CudaKernelFamily::activation;
    descriptor.scalar_types = {pc::CudaScalarType::fp32};
    descriptor.minimum_compute_capability = {6, 1};
    descriptor.cuda_12_release_support = pc::CudaSupportState::supported;
    descriptor.cuda_13_release_support = pc::CudaSupportState::supported;
    descriptor.binary_state = pc::CudaKernelBinaryState::available;
    descriptor.binding_state = pc::CudaKernelBindingState::adapter_available;

    if (!backend->kernel_registry().register_kernel(descriptor).ok()) {
        return EXIT_FAILURE;
    }
    if (backend->kernel_registry().register_kernel(descriptor).code !=
        pc::CudaKernelRegistrationCode::duplicate_kernel_id) {
        return EXIT_FAILURE;
    }

    const pc::CudaDevicePool pool = backend->pool_snapshot();
    if (pool.devices.empty()) {
        return EXIT_FAILURE;
    }

    for (const pc::CudaPoolDevice& device : pool.devices) {
        const pc::CudaDeviceExecutionReport report =
            backend->evaluate_device_execution(
                device.persistent_key(),
                {descriptor.kernel_id});
        if (!report.execution_ready()) {
            return EXIT_FAILURE;
        }
    }

    backend->shutdown();
    cpu_backend->shutdown();
    return backend->status().state == pb::ComputeBackendState::stopped &&
                   cpu_backend->status().state ==
                       pb::ComputeBackendState::stopped
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
