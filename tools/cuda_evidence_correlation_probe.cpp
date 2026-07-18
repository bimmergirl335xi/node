#include <cstdlib>
#include <iostream>

#include "cuda_evidence_adapter.hpp"
#include "linux_pci_inventory.hpp"

namespace pc = prometheus::backends::cuda;
namespace ph = prometheus::hardware;

namespace {

void print_version(
    const ph::GpuVersionEvidence& version,
    bool available) {
    if (!available) {
        std::cout << "unavailable";
        return;
    }
    std::cout << version.major << '.' << version.minor;
    if (version.patch != 0) std::cout << '.' << version.patch;
}

}  // namespace

int main() {
    const ph::LinuxPciInventoryResult hardware =
        ph::inventory_linux_pci_devices();
    pc::CudaGpuEvidenceCollectionOptions options{};
    options.pool_options.query_health = true;
    options.pool_options.query_peer_access = false;
    const pc::CudaGpuEvidenceCollectionResult observed =
        pc::collect_cuda_gpu_evidence(hardware, options);
    const ph::GpuEvidenceCorrelationResult& result = observed.correlation;

    std::size_t exact = 0;
    std::size_t inferred = 0;
    std::size_t hardware_only = 0;
    std::size_t ambiguous = 0;
    for (const auto& record : result.correlated_hardware) {
        if (record.correlation == ph::GpuCorrelationState::exact_match) {
            ++exact;
        } else if (record.correlation ==
                   ph::GpuCorrelationState::inferred_unique_match) {
            ++inferred;
        } else if (record.correlation ==
                   ph::GpuCorrelationState::hardware_only) {
            ++hardware_only;
        } else if (record.correlation ==
                   ph::GpuCorrelationState::ambiguous) {
            ++ambiguous;
        }
    }
    for (const auto& record : result.runtime_only) {
        if (record.correlation == ph::GpuCorrelationState::ambiguous) {
            ++ambiguous;
        }
    }

    std::cout << "GPU_EVIDENCE status=" << ph::to_string(result.code)
              << " complete=" << (result.complete ? "true" : "false")
              << " hardware_state=" << ph::to_string(result.hardware.state)
              << " hardware=" << result.correlated_hardware.size()
              << " cuda_state="
              << ph::to_string(result.cuda_enumeration.state)
              << " cuda_visible=" << result.cuda_enumeration.retained_records
              << " exact=" << exact
              << " inferred=" << inferred
              << " hardware_only=" << hardware_only
              << " runtime_only=" << result.runtime_only.size()
              << " ambiguous=" << ambiguous
              << " issues=" << result.issues.size()
              << " dropped_issues=" << result.dropped_issues << '\n';

    for (const auto& record : result.correlated_hardware) {
        std::cout << "GPU_CORRELATION pci="
                  << record.hardware.address.canonical
                  << " correlation=" << ph::to_string(record.correlation)
                  << " driver_state="
                  << ph::to_string(record.hardware.driver_state)
                  << " driver=";
        if (record.hardware.driver_state == ph::DriverBindingState::bound) {
            std::cout << record.hardware.driver_name;
        } else if (record.hardware.driver_state ==
                   ph::DriverBindingState::unbound) {
            std::cout << "none";
        } else {
            std::cout << "unknown";
        }
        std::cout << " cuda_devices=" << record.cuda_devices.size()
                  << " admission=" << ph::to_string(record.admission) << '\n';

        for (const auto& association : record.cuda_devices) {
            const auto& device = association.device;
            std::cout << "CUDA_DEVICE uuid="
                      << (device.stable_identity_available
                              ? device.stable_uuid
                              : "unavailable")
                      << " persistent_key="
                      << (device.stable_identity_available
                              ? device.persistent_key
                              : "unavailable")
                      << " ordinal=" << device.runtime_ordinal
                      << " correlation="
                      << ph::to_string(association.correlation)
                      << " capabilities="
                      << (device.capabilities_available
                              ? "available" : "unavailable")
                      << " project_support="
                      << ph::to_string(
                             device.compatibility.project_architecture)
                      << " compile_release_support="
                      << ph::to_string(
                             device.compatibility.compile_time_release)
                      << " runtime_release_support="
                      << ph::to_string(
                             device.compatibility.runtime_release)
                      << " binary_image="
                      << ph::to_string(
                             device.compatibility.binary_architecture_image)
                      << " kernel_coverage="
                      << ph::to_string(
                             device.compatibility.registered_kernel_coverage)
                      << " registration="
                      << ph::to_string(device.backend_registration)
                      << " runtime_binding="
                      << ph::to_string(device.runtime_binding)
                      << " execution=" << ph::to_string(device.execution)
                      << '\n';
        }
    }

    for (const auto& record : result.runtime_only) {
        std::cout << "CUDA_RUNTIME_ONLY uuid="
                  << (record.device.stable_identity_available
                          ? record.device.stable_uuid
                          : "unavailable")
                  << " ordinal=" << record.device.runtime_ordinal
                  << " correlation=" << ph::to_string(record.correlation)
                  << " admission=" << ph::to_string(record.admission) << '\n';
    }

    const ph::GpuCudaSoftwareEvidence& software = result.software;
    std::cout << "CUDA_SOFTWARE compile_toolkit=";
    print_version(
        software.compile_time_toolkit,
        software.compile_time_toolkit_available);
    std::cout << " runtime=";
    print_version(software.runtime, software.runtime_version_query_available);
    std::cout << " driver=";
    print_version(software.driver, software.driver_version_query_available);
    std::cout << " runtime_query="
              << (software.runtime_version_query_available
                      ? "available" : "unavailable")
              << " driver_query="
              << (software.driver_version_query_available
                      ? "available" : "unavailable")
              << " driver_supports_runtime="
              << ph::to_string(software.driver_supports_runtime) << '\n';
    std::cout << "NODE_ADMISSION status=not_evaluated\n";

    return result.code == ph::GpuEvidenceResultCode::invalid_input ||
                   result.code == ph::GpuEvidenceResultCode::identity_conflict ||
                   result.code == ph::GpuEvidenceResultCode::resource_exhausted
               ? EXIT_FAILURE
               : EXIT_SUCCESS;
}
