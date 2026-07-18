#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "gpu_evidence_correlation.hpp"

namespace ph = prometheus::hardware;

namespace {

ph::LinuxPciDevice make_hardware(
    const std::string& address,
    const std::string& driver = {}) {
    ph::LinuxPciDevice device{};
    device.address = ph::parse_pci_address(address);
    device.identity.vendor_id = 0x10deU;
    device.identity.device_id = 0x1eb1U;
    device.identity.class_code = 0x030000U;
    device.vendor_family = ph::PciVendorFamily::nvidia;
    device.device_class = ph::PciDeviceClass::vga_compatible_controller;
    if (driver.empty()) {
        device.driver_state = ph::DriverBindingState::unbound;
    } else {
        device.driver_state = ph::DriverBindingState::bound;
        device.driver_name = driver;
    }
    return device;
}

ph::GpuCudaRuntimeDeviceEvidence make_cuda(
    int ordinal,
    const std::string& uuid,
    std::uint32_t domain,
    std::uint32_t bus,
    std::uint32_t device,
    std::uint32_t function,
    bool domain_present = true) {
    ph::GpuCudaRuntimeDeviceEvidence evidence{};
    evidence.runtime_ordinal = ordinal;
    evidence.stable_identity_available = !uuid.empty();
    evidence.stable_uuid = uuid;
    evidence.persistent_key = uuid.empty() ? std::string{} : "cuda:" + uuid;
    evidence.pci_identity_available = true;
    evidence.pci_identity.domain = domain;
    evidence.pci_identity.bus = bus;
    evidence.pci_identity.device = device;
    evidence.pci_identity.function = function;
    evidence.pci_identity.domain_present = domain_present;
    evidence.pci_identity.valid = true;
    evidence.capabilities_available = true;
    evidence.compatibility.project_architecture =
        ph::GpuEvidenceSupportState::supported;
    evidence.compatibility.compile_time_release =
        ph::GpuEvidenceSupportState::supported;
    evidence.compatibility.runtime_release =
        ph::GpuEvidenceSupportState::supported;
    evidence.compatibility.binary_architecture_image =
        ph::GpuEvidenceSupportState::unknown;
    evidence.compatibility.registered_kernel_coverage =
        ph::GpuKernelCoverageState::complete;
    evidence.backend_registration = ph::GpuReadinessState::ready;
    evidence.runtime_binding = ph::GpuRuntimeBindingState::usable;
    evidence.execution = ph::GpuReadinessState::ready;
    return evidence;
}

ph::GpuEvidenceInput complete_input(
    std::vector<ph::LinuxPciDevice> hardware,
    std::vector<ph::GpuCudaRuntimeDeviceEvidence> cuda) {
    ph::GpuEvidenceInput input{};
    input.hardware.state = ph::GpuPhysicalInventoryState::complete;
    input.hardware.inventory.code = ph::LinuxPciInventoryCode::success;
    input.hardware.inventory.complete = true;
    input.hardware.inventory.devices = std::move(hardware);
    input.cuda_enumeration.state = cuda.empty()
        ? ph::GpuCudaEnumerationState::success_no_devices
        : ph::GpuCudaEnumerationState::success_with_devices;
    input.cuda_enumeration.reported_device_count =
        static_cast<int>(cuda.size());
    input.cuda_enumeration.devices = std::move(cuda);
    return input;
}

bool has_issue(
    const ph::GpuEvidenceCorrelationResult& result,
    ph::GpuEvidenceIssueCode code) {
    return std::any_of(
        result.issues.begin(), result.issues.end(),
        [code](const auto& issue) { return issue.code == code; });
}

bool test_exact_one_to_one() {
    auto input = complete_input(
        {make_hardware("0000:01:00.0", "nvidia")},
        {make_cuda(0, "GPU-aaaaaaaa", 0, 1, 0, 0)});
    const auto result = ph::correlate_gpu_evidence(input);
    return result.code == ph::GpuEvidenceResultCode::success &&
           result.complete && result.correlated_hardware.size() == 1 &&
           result.runtime_only.empty() &&
           result.correlated_hardware[0].correlation ==
               ph::GpuCorrelationState::exact_match &&
           result.correlated_hardware[0].cuda_devices.size() == 1 &&
           result.correlated_hardware[0].admission ==
               ph::GpuAdmissionState::not_evaluated;
}

bool test_hardware_cuda_not_evaluated() {
    auto input = complete_input(
        {make_hardware("0000:02:00.0")}, {});
    input.cuda_enumeration.state =
        ph::GpuCudaEnumerationState::not_evaluated;
    const auto result = ph::correlate_gpu_evidence(input);
    return result.code == ph::GpuEvidenceResultCode::cuda_not_evaluated &&
           !result.complete && result.correlated_hardware.size() == 1 &&
           result.correlated_hardware[0].correlation ==
               ph::GpuCorrelationState::not_evaluated;
}

bool test_hardware_cuda_zero() {
    const auto result = ph::correlate_gpu_evidence(complete_input(
        {make_hardware("0000:03:00.0")}, {}));
    return result.code == ph::GpuEvidenceResultCode::success &&
           result.complete && result.cuda_enumeration.state ==
               ph::GpuCudaEnumerationState::success_no_devices &&
           result.correlated_hardware[0].correlation ==
               ph::GpuCorrelationState::hardware_only;
}

bool test_non_cuda_driver(const std::string& driver) {
    auto input = complete_input(
        {make_hardware("0000:04:00.0", driver)}, {});
    input.cuda_enumeration.state =
        ph::GpuCudaEnumerationState::not_evaluated;
    const auto result = ph::correlate_gpu_evidence(input);
    const auto& record = result.correlated_hardware[0];
    return record.hardware.driver_state == ph::DriverBindingState::bound &&
           record.hardware.driver_name == driver &&
           record.correlation == ph::GpuCorrelationState::not_evaluated &&
           record.cuda_devices.empty();
}

bool test_runtime_only() {
    auto cuda = make_cuda(0, "GPU-bbbbbbbb", 0, 5, 0, 0);
    cuda.pci_identity_available = false;
    cuda.pci_identity = {};
    const auto result = ph::correlate_gpu_evidence(
        complete_input({}, {cuda}));
    return result.code == ph::GpuEvidenceResultCode::partial &&
           result.correlated_hardware.empty() &&
           result.runtime_only.size() == 1 &&
           result.runtime_only[0].device.stable_uuid == "GPU-bbbbbbbb" &&
           result.runtime_only[0].correlation ==
               ph::GpuCorrelationState::unavailable;
}

bool test_inferred_unique_match() {
    const auto result = ph::correlate_gpu_evidence(complete_input(
        {make_hardware("0001:06:00.0")},
        {make_cuda(0, "GPU-cccccccc", 0, 6, 0, 0, false)}));
    return result.code == ph::GpuEvidenceResultCode::success &&
           result.correlated_hardware[0].correlation ==
               ph::GpuCorrelationState::inferred_unique_match &&
           result.correlated_hardware[0].cuda_devices[0].correlation ==
               ph::GpuCorrelationState::inferred_unique_match;
}

bool test_ambiguous_missing_domain() {
    const auto result = ph::correlate_gpu_evidence(complete_input(
        {make_hardware("0000:07:00.0"),
         make_hardware("0001:07:00.0")},
        {make_cuda(0, "GPU-dddddddd", 0, 7, 0, 0, false)}));
    return result.code == ph::GpuEvidenceResultCode::identity_conflict &&
           !result.complete && result.runtime_only.size() == 1 &&
           result.runtime_only[0].correlation ==
               ph::GpuCorrelationState::ambiguous &&
           has_issue(result, ph::GpuEvidenceIssueCode::ambiguous_pci_identity);
}

bool test_mismatched_function() {
    const auto result = ph::correlate_gpu_evidence(complete_input(
        {make_hardware("0000:08:00.0")},
        {make_cuda(0, "GPU-eeeeeeee", 0, 8, 0, 1)}));
    return result.code == ph::GpuEvidenceResultCode::partial &&
           result.correlated_hardware[0].correlation ==
               ph::GpuCorrelationState::hardware_only &&
           result.runtime_only.size() == 1 &&
           result.runtime_only[0].correlation ==
               ph::GpuCorrelationState::runtime_only;
}

bool test_one_to_many() {
    const auto result = ph::correlate_gpu_evidence(complete_input(
        {make_hardware("0000:09:00.0")},
        {make_cuda(1, "GPU-22222222", 0, 9, 0, 0),
         make_cuda(0, "GPU-11111111", 0, 9, 0, 0)}));
    const auto& associations = result.correlated_hardware[0].cuda_devices;
    return result.code == ph::GpuEvidenceResultCode::success &&
           associations.size() == 2 &&
           associations[0].device.stable_uuid == "GPU-11111111" &&
           associations[1].device.stable_uuid == "GPU-22222222";
}

bool test_duplicate_cuda_uuid() {
    const auto result = ph::correlate_gpu_evidence(complete_input(
        {make_hardware("0000:0a:00.0")},
        {make_cuda(0, "GPU-duplicate", 0, 10, 0, 0),
         make_cuda(1, "GPU-duplicate", 0, 10, 0, 0)}));
    return result.code == ph::GpuEvidenceResultCode::identity_conflict &&
           !result.complete &&
           has_issue(
               result,
               ph::GpuEvidenceIssueCode::duplicate_cuda_stable_identity);
}

bool test_duplicate_hardware_identity() {
    const auto duplicate = make_hardware("0000:0b:00.0");
    const auto result = ph::correlate_gpu_evidence(complete_input(
        {duplicate, duplicate}, {}));
    return result.code == ph::GpuEvidenceResultCode::identity_conflict &&
           !result.complete && result.correlated_hardware.size() == 2 &&
           has_issue(
               result,
               ph::GpuEvidenceIssueCode::duplicate_hardware_pci_identity);
}

bool test_partial_hardware() {
    auto input = complete_input(
        {make_hardware("0000:0c:00.0")},
        {make_cuda(0, "GPU-ffffffff", 0, 12, 0, 0)});
    input.hardware.state = ph::GpuPhysicalInventoryState::partial;
    input.hardware.malformed_observations = true;
    const auto result = ph::correlate_gpu_evidence(input);
    return result.code == ph::GpuEvidenceResultCode::partial &&
           result.correlated_hardware[0].correlation ==
               ph::GpuCorrelationState::exact_match &&
           result.hardware.malformed_observations;
}

bool test_cuda_enumeration_failure() {
    auto input = complete_input(
        {make_hardware("0000:0d:00.0")}, {});
    input.cuda_enumeration.state = ph::GpuCudaEnumerationState::failed;
    input.cuda_enumeration.native_error_code = 35;
    const auto result = ph::correlate_gpu_evidence(input);
    return result.code ==
               ph::GpuEvidenceResultCode::cuda_enumeration_failed &&
           result.correlated_hardware.size() == 1 &&
           result.correlated_hardware[0].correlation ==
               ph::GpuCorrelationState::unavailable &&
           result.cuda_enumeration.native_error_code == 35;
}

bool test_compatibility_unsupported() {
    auto cuda = make_cuda(0, "GPU-unsupported", 0, 14, 0, 0);
    cuda.compatibility.project_architecture =
        ph::GpuEvidenceSupportState::unsupported;
    cuda.backend_registration = ph::GpuReadinessState::not_ready;
    cuda.execution = ph::GpuReadinessState::not_ready;
    const auto result = ph::correlate_gpu_evidence(complete_input(
        {make_hardware("0000:0e:00.0")}, {cuda}));
    const auto& retained =
        result.correlated_hardware[0].cuda_devices[0].device;
    return result.code == ph::GpuEvidenceResultCode::success &&
           retained.compatibility.project_architecture ==
               ph::GpuEvidenceSupportState::unsupported &&
           retained.backend_registration == ph::GpuReadinessState::not_ready &&
           retained.execution == ph::GpuReadinessState::not_ready;
}

bool test_compatibility_unknown() {
    auto cuda = make_cuda(0, "GPU-unknown", 0, 15, 0, 0);
    cuda.compatibility.project_architecture =
        ph::GpuEvidenceSupportState::unknown;
    cuda.compatibility.runtime_release =
        ph::GpuEvidenceSupportState::unknown;
    const auto result = ph::correlate_gpu_evidence(complete_input(
        {make_hardware("0000:0f:00.0")}, {cuda}));
    const auto& retained =
        result.correlated_hardware[0].cuda_devices[0].device.compatibility;
    return retained.project_architecture ==
               ph::GpuEvidenceSupportState::unknown &&
           retained.runtime_release == ph::GpuEvidenceSupportState::unknown;
}

bool test_registration_and_admission() {
    auto cuda = make_cuda(0, "GPU-registration", 0, 16, 0, 0);
    cuda.execution = ph::GpuReadinessState::not_evaluated;
    const auto result = ph::correlate_gpu_evidence(complete_input(
        {make_hardware("0000:10:00.0")}, {cuda}));
    const auto& record = result.correlated_hardware[0];
    return record.cuda_devices[0].device.backend_registration ==
               ph::GpuReadinessState::ready &&
           record.cuda_devices[0].device.execution ==
               ph::GpuReadinessState::not_evaluated &&
           record.admission == ph::GpuAdmissionState::not_evaluated;
}

bool test_execution_incomplete() {
    auto cuda = make_cuda(0, "GPU-incomplete", 0, 17, 0, 0);
    cuda.compatibility.registered_kernel_coverage =
        ph::GpuKernelCoverageState::unavailable;
    cuda.execution = ph::GpuReadinessState::not_ready;
    const auto result = ph::correlate_gpu_evidence(complete_input(
        {make_hardware("0000:11:00.0")}, {cuda}));
    const auto& retained =
        result.correlated_hardware[0].cuda_devices[0].device;
    return retained.backend_registration == ph::GpuReadinessState::ready &&
           retained.compatibility.registered_kernel_coverage ==
               ph::GpuKernelCoverageState::unavailable &&
           retained.execution == ph::GpuReadinessState::not_ready;
}

bool test_capacity_exhaustion() {
    auto input = complete_input(
        {make_hardware("0000:12:00.0"),
         make_hardware("0000:13:00.0")}, {});
    ph::GpuEvidenceCorrelationOptions options{};
    options.maximum_hardware_input_records = 1;
    const auto hardware_bound = ph::correlate_gpu_evidence(input, options);
    if (hardware_bound.code !=
            ph::GpuEvidenceResultCode::resource_exhausted ||
        hardware_bound.complete ||
        !has_issue(
            hardware_bound,
            ph::GpuEvidenceIssueCode::hardware_input_capacity_exhausted)) {
        return false;
    }

    input = complete_input(
        {make_hardware("0000:14:00.0")},
        {make_cuda(0, "GPU-capacity-1", 0, 20, 0, 0),
         make_cuda(1, "GPU-capacity-2", 0, 20, 0, 0)});
    options = {};
    options.maximum_cuda_associations_per_hardware = 1;
    const auto association_bound =
        ph::correlate_gpu_evidence(input, options);
    if (association_bound.code !=
            ph::GpuEvidenceResultCode::resource_exhausted ||
        !association_bound.correlated_hardware.empty() ||
        !has_issue(
            association_bound,
            ph::GpuEvidenceIssueCode::association_capacity_exhausted)) {
        return false;
    }

    auto oversized = make_cuda(0, "GPU-identity-too-long", 0, 20, 0, 0);
    input = complete_input(
        {make_hardware("0000:14:00.0")}, {oversized});
    options = {};
    options.maximum_stable_identity_bytes = 8;
    const auto identity_bound = ph::correlate_gpu_evidence(input, options);
    return identity_bound.code == ph::GpuEvidenceResultCode::invalid_input &&
           !identity_bound.complete && identity_bound.runtime_only.size() == 1 &&
           identity_bound.runtime_only[0].device.stable_uuid.empty() &&
           identity_bound.runtime_only[0].device.persistent_key.empty() &&
           identity_bound.runtime_only[0].correlation ==
               ph::GpuCorrelationState::invalid_identity &&
           has_issue(
               identity_bound,
               ph::GpuEvidenceIssueCode::invalid_cuda_stable_identity);
}

std::string signature(const ph::GpuEvidenceCorrelationResult& result) {
    std::ostringstream output{};
    output << ph::to_string(result.code) << ':' << result.complete;
    for (const auto& hardware : result.correlated_hardware) {
        output << "|H:" << hardware.hardware.address.canonical << ':'
               << ph::to_string(hardware.correlation);
        for (const auto& cuda : hardware.cuda_devices) {
            output << ":C:" << cuda.device.stable_uuid << ':'
                   << cuda.device.runtime_ordinal << ':'
                   << ph::to_string(cuda.correlation);
        }
    }
    for (const auto& runtime : result.runtime_only) {
        output << "|R:" << runtime.device.stable_uuid << ':'
               << runtime.device.runtime_ordinal << ':'
               << ph::to_string(runtime.correlation);
    }
    for (const auto& issue : result.issues) {
        output << "|I:" << ph::to_string(issue.code) << ':'
               << issue.identity << ':' << issue.runtime_ordinal;
    }
    return output.str();
}

bool test_deterministic_permutations() {
    std::vector<ph::LinuxPciDevice> hardware{
        make_hardware("0000:17:00.0"),
        make_hardware("0000:15:00.0"),
        make_hardware("0000:16:00.0")};
    std::vector<ph::GpuCudaRuntimeDeviceEvidence> cuda{
        make_cuda(2, "GPU-order-c", 0, 23, 0, 0),
        make_cuda(0, "GPU-order-a", 0, 21, 0, 0),
        make_cuda(1, "GPU-order-b", 0, 22, 0, 0)};
    std::sort(hardware.begin(), hardware.end(), [](const auto& a, const auto& b) {
        return a.address.canonical < b.address.canonical;
    });
    std::sort(cuda.begin(), cuda.end(), [](const auto& a, const auto& b) {
        return a.stable_uuid < b.stable_uuid;
    });

    std::string expected{};
    bool first = true;
    do {
        auto cuda_permutation = cuda;
        do {
            const std::string observed = signature(
                ph::correlate_gpu_evidence(complete_input(
                    hardware, cuda_permutation)));
            if (first) {
                expected = observed;
                first = false;
            } else if (observed != expected) {
                return false;
            }
        } while (std::next_permutation(
            cuda_permutation.begin(), cuda_permutation.end(),
            [](const auto& a, const auto& b) {
                return a.stable_uuid < b.stable_uuid;
            }));
    } while (std::next_permutation(
        hardware.begin(), hardware.end(), [](const auto& a, const auto& b) {
            return a.address.canonical < b.address.canonical;
        }));
    return true;
}

}  // namespace

int main() {
    const auto check = [](bool passed, const char* name) {
        if (!passed) std::cerr << "FAILED: " << name << '\n';
        return passed;
    };
    bool passed = true;
    passed &= check(test_exact_one_to_one(), "A exact_one_to_one");
    passed &= check(test_hardware_cuda_not_evaluated(), "B cuda_not_evaluated");
    passed &= check(test_hardware_cuda_zero(), "C cuda_zero");
    passed &= check(test_non_cuda_driver("nouveau"), "D nouveau");
    passed &= check(test_non_cuda_driver("vfio-pci"), "E vfio_pci");
    passed &= check(test_runtime_only(), "F runtime_only");
    passed &= check(test_inferred_unique_match(), "G inferred_unique");
    passed &= check(test_ambiguous_missing_domain(), "H ambiguous_domain");
    passed &= check(test_mismatched_function(), "I mismatched_function");
    passed &= check(test_one_to_many(), "J one_to_many");
    passed &= check(test_duplicate_cuda_uuid(), "K duplicate_uuid");
    passed &= check(test_duplicate_hardware_identity(), "L duplicate_hardware");
    passed &= check(test_partial_hardware(), "M partial_hardware");
    passed &= check(test_cuda_enumeration_failure(), "N cuda_failure");
    passed &= check(test_compatibility_unsupported(), "O unsupported");
    passed &= check(test_compatibility_unknown(), "P unknown");
    passed &= check(test_registration_and_admission(), "Q admission");
    passed &= check(test_execution_incomplete(), "R execution_incomplete");
    passed &= check(test_capacity_exhaustion(), "S capacity");
    passed &= check(test_deterministic_permutations(), "T deterministic");
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
