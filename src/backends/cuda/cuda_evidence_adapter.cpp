#include "cuda_evidence_adapter.hpp"

#include <algorithm>

namespace prometheus::backends::cuda {
namespace {

hardware::GpuEvidenceSupportState map_support(
    CudaSupportState value) noexcept {
    switch (value) {
        case CudaSupportState::supported:
            return hardware::GpuEvidenceSupportState::supported;
        case CudaSupportState::unsupported:
            return hardware::GpuEvidenceSupportState::unsupported;
        case CudaSupportState::unknown:
            return hardware::GpuEvidenceSupportState::unknown;
    }
    return hardware::GpuEvidenceSupportState::unknown;
}

hardware::GpuCudaReleaseFamily map_release(
    CudaReleaseFamily value) noexcept {
    switch (value) {
        case CudaReleaseFamily::cuda_12_legacy:
            return hardware::GpuCudaReleaseFamily::cuda_12_legacy;
        case CudaReleaseFamily::cuda_13_modern:
            return hardware::GpuCudaReleaseFamily::cuda_13_modern;
        case CudaReleaseFamily::unknown:
            return hardware::GpuCudaReleaseFamily::unknown;
    }
    return hardware::GpuCudaReleaseFamily::unknown;
}

hardware::GpuVersionEvidence map_version(const CudaVersion& value) noexcept {
    hardware::GpuVersionEvidence mapped{};
    mapped.encoded = value.encoded;
    mapped.major = value.major;
    mapped.minor = value.minor;
    mapped.patch = value.patch;
    return mapped;
}

hardware::GpuCudaSoftwareEvidence map_software(
    const CudaSoftwareVersions& value) noexcept {
    hardware::GpuCudaSoftwareEvidence mapped{};
    mapped.compile_time_toolkit_available =
        value.compile_time_toolkit.valid();
    mapped.compile_time_toolkit = map_version(value.compile_time_toolkit);
    mapped.compile_time_release_family =
        map_release(value.compile_time_release_family);
    mapped.runtime_version_query_available =
        value.runtime_version_available;
    mapped.runtime = map_version(value.runtime);
    mapped.runtime_release_family = map_release(value.runtime_release_family);
    mapped.driver_version_query_available = value.driver_version_available;
    mapped.driver = map_version(value.driver);
    mapped.driver_supports_runtime = map_support(value.driver_supports_runtime);
    return mapped;
}

hardware::GpuPhysicalInventoryState map_hardware_state(
    const hardware::LinuxPciInventoryResult& inventory) noexcept {
    using Code = hardware::LinuxPciInventoryCode;
    switch (inventory.code) {
        case Code::success:
            return hardware::GpuPhysicalInventoryState::complete;
        case Code::partial:
        case Code::malformed_entry:
        case Code::resource_exhausted:
            return hardware::GpuPhysicalInventoryState::partial;
        case Code::inventory_root_unavailable:
            return hardware::GpuPhysicalInventoryState::unavailable;
        case Code::unsupported_platform:
        case Code::permission_denied:
        case Code::io_failure:
        case Code::invalid_options:
            return hardware::GpuPhysicalInventoryState::failed;
    }
    return hardware::GpuPhysicalInventoryState::failed;
}

hardware::GpuCudaEnumerationState map_discovery_state(
    const CudaDeviceDiscoveryResult& discovery) noexcept {
    switch (discovery.status.code) {
        case CudaDiscoveryCode::success:
            return discovery.devices.empty()
                       ? hardware::GpuCudaEnumerationState::success_no_devices
                       : hardware::GpuCudaEnumerationState::success_with_devices;
        case CudaDiscoveryCode::no_devices:
            return hardware::GpuCudaEnumerationState::success_no_devices;
        case CudaDiscoveryCode::partial_success:
            return hardware::GpuCudaEnumerationState::partial;
        case CudaDiscoveryCode::runtime_query_failed:
            return hardware::GpuCudaEnumerationState::failed;
    }
    return hardware::GpuCudaEnumerationState::failed;
}

hardware::GpuKernelCoverageState map_coverage(
    CudaKernelCoverageState value) noexcept {
    switch (value) {
        case CudaKernelCoverageState::unknown:
            return hardware::GpuKernelCoverageState::unknown;
        case CudaKernelCoverageState::unavailable:
            return hardware::GpuKernelCoverageState::unavailable;
        case CudaKernelCoverageState::partial:
            return hardware::GpuKernelCoverageState::partial;
        case CudaKernelCoverageState::complete:
            return hardware::GpuKernelCoverageState::complete;
    }
    return hardware::GpuKernelCoverageState::unknown;
}

const CudaPoolDevice* find_pool_device(
    const CudaDevicePool* pool,
    const CudaDiscoveredDevice& discovered) noexcept {
    if (pool == nullptr) return nullptr;
    if (discovered.has_stable_identity()) {
        return pool->find_device(discovered.identity.persistent_key);
    }
    for (const CudaPoolDevice& device : pool->devices) {
        if (device.discovered.identity.runtime_ordinal ==
            discovered.identity.runtime_ordinal) {
            return &device;
        }
    }
    return nullptr;
}

const CudaDeviceExecutionReport* find_execution_report(
    const std::vector<CudaDeviceExecutionReport>& reports,
    const CudaDiscoveredDevice& discovered) noexcept {
    if (!discovered.has_stable_identity()) return nullptr;
    const auto iterator = std::find_if(
        reports.begin(), reports.end(), [&](const auto& report) {
            return report.persistent_key ==
                   discovered.identity.persistent_key;
        });
    return iterator == reports.end() ? nullptr : &*iterator;
}

hardware::GpuCudaRuntimeDeviceEvidence map_device(
    const CudaDiscoveredDevice& discovered,
    const CudaPoolDevice* pool_device,
    const CudaDeviceExecutionReport* execution) {
    hardware::GpuCudaRuntimeDeviceEvidence mapped{};
    mapped.runtime_ordinal = discovered.identity.runtime_ordinal;
    mapped.stable_identity_available = discovered.has_stable_identity();
    mapped.stable_uuid = discovered.identity.uuid;
    mapped.persistent_key = discovered.identity.persistent_key;
    mapped.pci_identity_available =
        discovered.identity.pci_bus_id_available &&
        discovered.identity.pci_address.valid;
    mapped.pci_identity.domain = discovered.identity.pci_address.domain;
    mapped.pci_identity.bus = discovered.identity.pci_address.bus;
    mapped.pci_identity.device = discovered.identity.pci_address.device;
    mapped.pci_identity.function = discovered.identity.pci_address.function;
    mapped.pci_identity.domain_present =
        discovered.identity.pci_address.domain_was_present;
    mapped.pci_identity.valid = discovered.identity.pci_address.valid;
    mapped.capabilities_available = discovered.capabilities_available;
    mapped.backend_registration = discovered.ready_for_backend_registration()
                                      ? hardware::GpuReadinessState::ready
                                      : hardware::GpuReadinessState::not_ready;

    if (discovered.capabilities_available) {
        const CudaCompatibilityReport& compatibility =
            discovered.capabilities.compatibility;
        mapped.compatibility.project_architecture =
            map_support(compatibility.prometheus_project);
        mapped.compatibility.compile_time_release =
            map_support(compatibility.compile_time_release_family);
        mapped.compatibility.runtime_release =
            map_support(compatibility.runtime_release_family);
        mapped.compatibility.binary_architecture_image =
            map_support(compatibility.binary_architecture_image);
        mapped.compatibility.registered_kernel_coverage =
            hardware::GpuKernelCoverageState::not_evaluated;
    }

    if (pool_device != nullptr) {
        if (!pool_device->health_was_requested) {
            mapped.runtime_binding =
                hardware::GpuRuntimeBindingState::not_evaluated;
        } else if (!pool_device->health_available) {
            mapped.runtime_binding = hardware::GpuRuntimeBindingState::unknown;
        } else {
            mapped.runtime_binding = pool_device->runtime_binding_usable()
                                         ? hardware::GpuRuntimeBindingState::usable
                                         : hardware::GpuRuntimeBindingState::unusable;
        }
    }

    if (execution != nullptr) {
        mapped.backend_registration = execution->registration_ready
                                          ? hardware::GpuReadinessState::ready
                                          : hardware::GpuReadinessState::not_ready;
        mapped.runtime_binding = execution->runtime_binding_usable
                                     ? hardware::GpuRuntimeBindingState::usable
                                     : hardware::GpuRuntimeBindingState::unusable;
        mapped.compatibility.registered_kernel_coverage =
            map_coverage(execution->kernel_coverage.state);
        mapped.execution = execution->execution_ready()
                               ? hardware::GpuReadinessState::ready
                               : hardware::GpuReadinessState::not_ready;
    }
    return mapped;
}

}  // namespace

hardware::GpuEvidenceInput adapt_cuda_gpu_evidence(
    const hardware::LinuxPciInventoryResult& hardware_inventory,
    const CudaSoftwareVersions& software,
    const CudaDeviceDiscoveryResult& discovery,
    const CudaDevicePool* pool,
    const std::vector<CudaDeviceExecutionReport>& execution_reports) {
    hardware::GpuEvidenceInput input{};
    input.hardware.state = map_hardware_state(hardware_inventory);
    input.hardware.resource_exhausted =
        hardware_inventory.code ==
        hardware::LinuxPciInventoryCode::resource_exhausted;
    input.hardware.malformed_observations =
        hardware_inventory.code ==
        hardware::LinuxPciInventoryCode::malformed_entry;
    input.hardware.inventory = hardware_inventory;
    input.software = map_software(software);
    input.cuda_enumeration.state = map_discovery_state(discovery);
    input.cuda_enumeration.reported_device_count =
        discovery.reported_device_count;
    input.cuda_enumeration.reported_stable_identity_count =
        discovery.stable_identity_count;
    input.cuda_enumeration.reported_capability_count =
        discovery.capability_count;
    input.cuda_enumeration.reported_registration_ready_count =
        discovery.registration_ready_count;
    input.cuda_enumeration.native_error_code =
        discovery.status.native_error_code;
    input.cuda_enumeration.devices.reserve(discovery.devices.size());

    for (const CudaDiscoveredDevice& device : discovery.devices) {
        input.cuda_enumeration.devices.push_back(map_device(
            device,
            find_pool_device(pool, device),
            find_execution_report(execution_reports, device)));
    }
    return input;
}

CudaGpuEvidenceCollectionResult collect_cuda_gpu_evidence(
    const hardware::LinuxPciInventoryResult& hardware_inventory,
    const CudaGpuEvidenceCollectionOptions& options) {
    CudaGpuEvidenceCollectionResult result{};
    result.software = query_cuda_software_versions();
    result.pool = discover_cuda_device_pool(options.pool_options);
    result.evidence = adapt_cuda_gpu_evidence(
        hardware_inventory,
        result.software,
        result.pool.pool.discovery,
        &result.pool.pool);
    result.correlation = hardware::correlate_gpu_evidence(
        result.evidence,
        options.correlation_options);
    return result;
}

}  // namespace prometheus::backends::cuda
