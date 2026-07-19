#include <cstdlib>
#include <iostream>

#include "cuda_evidence_adapter.hpp"
#include "linux_pci_inventory.hpp"

namespace pc = prometheus::backends::cuda;
namespace ph = prometheus::hardware;

namespace {

bool test_execution_report_mapping() {
    ph::LinuxPciInventoryResult inventory{};
    ph::LinuxPciDevice hardware{};
    hardware.address = ph::parse_pci_address("0000:01:00.0");
    inventory.devices.push_back(hardware);

    pc::CudaDeviceDiscoveryResult discovery{};
    pc::CudaDiscoveredDevice cuda{};
    cuda.identity.runtime_ordinal = 0;
    cuda.identity.uuid_available = true;
    cuda.identity.uuid = "GPU-adapter-test";
    cuda.identity.persistent_key = "cuda:GPU-adapter-test";
    cuda.identity.pci_bus_id_available = true;
    cuda.identity.pci_address.domain = 0;
    cuda.identity.pci_address.bus = 1;
    cuda.identity.pci_address.device = 0;
    cuda.identity.pci_address.function = 0;
    cuda.identity.pci_address.domain_was_present = true;
    cuda.identity.pci_address.valid = true;
    cuda.capabilities_available = true;
    discovery.devices.push_back(cuda);
    discovery.reported_device_count = 1;
    discovery.stable_identity_count = 1;
    discovery.capability_count = 1;
    discovery.registration_ready_count = 1;

    pc::CudaDeviceExecutionReport execution{};
    execution.persistent_key = cuda.identity.persistent_key;
    execution.device_found = true;
    execution.registration_ready = true;
    execution.runtime_binding_usable = true;
    execution.kernel_coverage.state = pc::CudaKernelCoverageState::complete;

    const ph::GpuEvidenceInput adapted = pc::adapt_cuda_gpu_evidence(
        inventory, {}, discovery, nullptr, {execution});
    const auto result = ph::correlate_gpu_evidence(adapted);
    if (result.code != ph::GpuEvidenceResultCode::success ||
        result.correlated_hardware.size() != 1 ||
        result.correlated_hardware[0].cuda_devices.size() != 1) {
        return false;
    }
    const auto& retained =
        result.correlated_hardware[0].cuda_devices[0].device;
    return retained.backend_registration == ph::GpuReadinessState::ready &&
           retained.runtime_binding == ph::GpuRuntimeBindingState::usable &&
           retained.compatibility.registered_kernel_coverage ==
               ph::GpuKernelCoverageState::complete &&
           retained.execution == ph::GpuReadinessState::ready &&
           result.correlated_hardware[0].admission ==
               ph::GpuAdmissionState::not_evaluated;
}

}  // namespace

int main() {
    if (!test_execution_report_mapping()) {
        std::cerr << "CUDA execution evidence mapping failed\n";
        return EXIT_FAILURE;
    }

    const ph::LinuxPciInventoryResult inventory =
        ph::inventory_linux_pci_devices();
    pc::CudaGpuEvidenceCollectionOptions options{};
    options.pool_options.query_health = true;
    options.pool_options.query_peer_access = false;
    const pc::CudaGpuEvidenceCollectionResult observed =
        pc::collect_cuda_gpu_evidence(inventory, options);
    const ph::GpuEvidenceCorrelationResult& result = observed.correlation;

    if (result.hardware.input_records != inventory.devices.size() ||
        result.cuda_enumeration.input_records !=
            observed.pool.pool.discovery.devices.size() ||
        result.cuda_enumeration.reported_stable_identity_count !=
            observed.pool.pool.discovery.stable_identity_count ||
        result.cuda_enumeration.reported_capability_count !=
            observed.pool.pool.discovery.capability_count ||
        result.cuda_enumeration.reported_registration_ready_count !=
            observed.pool.pool.discovery.registration_ready_count ||
        result.correlated_hardware.size() > inventory.devices.size() ||
        result.capacity.retained_cuda_associations +
                result.runtime_only.size() !=
            observed.pool.pool.discovery.devices.size()) {
        std::cerr << "CUDA evidence adapter lost or fabricated records\n";
        return EXIT_FAILURE;
    }

    for (const auto& hardware : result.correlated_hardware) {
        if (hardware.admission != ph::GpuAdmissionState::not_evaluated) {
            std::cerr << "CUDA evidence adapter evaluated admission\n";
            return EXIT_FAILURE;
        }
        for (const auto& association : hardware.cuda_devices) {
            const auto& pci = association.device.pci_identity;
            if (association.correlation ==
                    ph::GpuCorrelationState::exact_match &&
                (!pci.domain_present || !pci.valid ||
                 hardware.hardware.address.domain != pci.domain ||
                 hardware.hardware.address.bus != pci.bus ||
                 hardware.hardware.address.device != pci.device ||
                 hardware.hardware.address.function != pci.function)) {
                std::cerr << "Exact CUDA correlation has unequal PCI identity\n";
                return EXIT_FAILURE;
            }
            if (association.device.execution !=
                ph::GpuReadinessState::not_evaluated) {
                std::cerr << "Execution was inferred without kernel evidence\n";
                return EXIT_FAILURE;
            }
        }
    }

    if (inventory.code == ph::LinuxPciInventoryCode::success &&
        observed.pool.pool.discovery.status.completed() &&
        (result.code == ph::GpuEvidenceResultCode::invalid_input ||
         result.code == ph::GpuEvidenceResultCode::identity_conflict ||
         result.code == ph::GpuEvidenceResultCode::resource_exhausted)) {
        std::cerr << "Valid host evidence produced a structural failure\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
