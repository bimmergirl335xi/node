#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "backend_registry.hpp"
#include "cuda_backend.hpp"

namespace pb = prometheus::backends;
namespace pc = prometheus::backends::cuda;

namespace {

pc::CudaKernelDescriptor make_probe_descriptor(
    std::string id,
    pc::CudaKernelBinaryState binary,
    pc::CudaKernelBindingState binding) {
    pc::CudaKernelDescriptor descriptor{};
    descriptor.kernel_id = std::move(id);
    descriptor.display_name = "Phase 6.5 synthetic registry probe";
    descriptor.family = pc::CudaKernelFamily::activation;
    descriptor.scalar_types = {pc::CudaScalarType::fp32};
    descriptor.minimum_compute_capability = {6, 1};
    descriptor.cuda_12_release_support = pc::CudaSupportState::supported;
    descriptor.cuda_13_release_support = pc::CudaSupportState::supported;
    descriptor.binary_state = binary;
    descriptor.binding_state = binding;
    descriptor.source_file = "tools/cuda_backend_registry_probe.cu";
    descriptor.adapter_name = "synthetic_probe_only";
    return descriptor;
}

void print_coverage(
    const std::string& label,
    const pc::CudaDeviceExecutionReport& report) {
    std::cout << label << "\n"
              << "  device found: " << (report.device_found ? "yes" : "no") << "\n"
              << "  registration ready: "
              << (report.registration_ready ? "yes" : "no") << "\n"
              << "  runtime binding usable: "
              << (report.runtime_binding_usable ? "yes" : "no") << "\n"
              << "  kernel coverage: "
              << pc::to_string(report.kernel_coverage.state) << "\n"
              << "  execution ready: "
              << (report.execution_ready() ? "yes" : "no") << "\n";

    for (const pc::CudaKernelCoverageItem& item :
         report.kernel_coverage.items) {
        std::cout << "    " << item.kernel_id
                  << ": " << pc::to_string(item.compatibility)
                  << ", binary=" << pc::to_string(item.binary_state)
                  << ", binding=" << pc::to_string(item.binding_state);
        if (!item.message.empty()) {
            std::cout << " (" << item.message << ")";
        }
        std::cout << "\n";
    }
}

}  // namespace

int main() {
    std::cout << "Prometheus CUDA backend and registry probe\n";

    auto cuda_backend = std::make_shared<pc::CudaBackend>();
    pb::BackendRegistry backends{};
    const pb::BackendRegistrationResult registration =
        backends.register_backend(cuda_backend);
    std::cout << "Backend registration: "
              << pb::to_string(registration.code) << "\n";
    if (!registration.ok()) {
        std::cerr << registration.message << "\n";
        return EXIT_FAILURE;
    }

    const pb::ComputeBackendStatus initialization =
        cuda_backend->initialize();
    std::cout << "Backend state: "
              << pb::to_string(initialization.state) << "\n"
              << "Backend usable: "
              << (initialization.usable ? "yes" : "no") << "\n";
    if (!initialization.message.empty()) {
        std::cout << "Backend message: " << initialization.message << "\n";
    }

    const std::vector<pb::ComputeBackendSnapshot> snapshots =
        backends.snapshots();
    std::cout << "Registered backends: " << snapshots.size() << "\n";
    for (const pb::ComputeBackendSnapshot& snapshot : snapshots) {
        std::cout << "  " << snapshot.backend_id
                  << ": kind=" << pb::to_string(snapshot.kind)
                  << ", state=" << pb::to_string(snapshot.state)
                  << ", visible=" << snapshot.visible_resource_count
                  << ", usable=" << snapshot.usable_resource_count << "\n";
    }

    if (!initialization.usable) {
        std::cerr << "CUDA backend did not expose a usable runtime binding\n";
        return EXIT_FAILURE;
    }

    const pc::CudaKernelRegistrationResult ready_registration =
        cuda_backend->kernel_registry().register_kernel(
            make_probe_descriptor(
                "probe.synthetic.ready.fp32",
                pc::CudaKernelBinaryState::available,
                pc::CudaKernelBindingState::adapter_available));
    const pc::CudaKernelRegistrationResult metadata_registration =
        cuda_backend->kernel_registry().register_kernel(
            make_probe_descriptor(
                "probe.synthetic.metadata_only.fp32",
                pc::CudaKernelBinaryState::unknown,
                pc::CudaKernelBindingState::metadata_only));

    std::cout << "Ready descriptor registration: "
              << pc::to_string(ready_registration.code) << "\n"
              << "Metadata descriptor registration: "
              << pc::to_string(metadata_registration.code) << "\n"
              << "Kernel registry size: "
              << cuda_backend->kernel_registry().size() << "\n";
    if (!ready_registration.ok() || !metadata_registration.ok()) {
        return EXIT_FAILURE;
    }

    const pc::CudaDevicePool pool = cuda_backend->pool_snapshot();
    std::cout << "CUDA devices in backend snapshot: "
              << pool.devices.size() << "\n";

    bool all_ready_reports_complete = true;
    bool all_metadata_reports_unresolved = true;
    for (const pc::CudaPoolDevice& device : pool.devices) {
        const std::string& key = device.persistent_key();
        std::cout << "\nDevice " << key << "\n";

        const pc::CudaDeviceExecutionReport ready_report =
            cuda_backend->evaluate_device_execution(
                key,
                {"probe.synthetic.ready.fp32"});
        print_coverage("  Ready requirement", ready_report);
        all_ready_reports_complete =
            all_ready_reports_complete && ready_report.execution_ready();

        const pc::CudaDeviceExecutionReport metadata_report =
            cuda_backend->evaluate_device_execution(
                key,
                {"probe.synthetic.metadata_only.fp32"});
        print_coverage("  Metadata-only requirement", metadata_report);
        all_metadata_reports_unresolved =
            all_metadata_reports_unresolved &&
            metadata_report.kernel_coverage.state ==
                pc::CudaKernelCoverageState::unknown &&
            !metadata_report.execution_ready();
    }

    // Prove that the backend remains an advisory planner for whole specialist
    // instances. No allocation or kernel launch occurs here.
    pc::CudaTaskInstanceRequest request{};
    request.request_id = "phase-6-5-two-specialists";
    request.instance_count = 2;
    request.memory_bytes_per_instance = 64ULL * 1024ULL * 1024ULL;
    request.memory_headroom_bytes = 128ULL * 1024ULL * 1024ULL;
    request.minimum_compute_capability = {6, 1};
    request.policy = pc::CudaTaskPlacementPolicy::spread;

    const pc::CudaTaskPlacementPlan plan =
        cuda_backend->plan_task_instances(request);
    std::cout << "\nBackend placement plan: "
              << pc::to_string(plan.code) << "\n"
              << "Assignments: " << plan.assignments.size() << "\n"
              << "Unplaced: " << plan.unplaced_instance_indices.size() << "\n";

    const bool planner_ok = plan.fully_satisfied() &&
                            plan.assignments.size() == request.instance_count;
    const bool passed = all_ready_reports_complete &&
                        all_metadata_reports_unresolved && planner_ok;

    cuda_backend->shutdown();
    std::cout << "Final backend state: "
              << pb::to_string(cuda_backend->status().state) << "\n";
    std::cout << (passed ? "Phase 6.5 probe passed\n"
                         : "Phase 6.5 probe failed\n");
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
