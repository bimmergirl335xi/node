#include "gpu_evidence_correlation.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace prometheus::hardware {
namespace {

struct PlannedCudaMatch {
    GpuCudaRuntimeDeviceEvidence device{};
    GpuCorrelationState state = GpuCorrelationState::runtime_only;
    std::size_t hardware_index = std::numeric_limits<std::size_t>::max();
    bool has_hardware = false;
};

bool valid_options(const GpuEvidenceCorrelationOptions& options) noexcept {
    return options.maximum_hardware_input_records > 0 &&
           options.maximum_hardware_input_records <=
               kAbsoluteMaximumGpuHardwareEvidence &&
           options.maximum_cuda_input_records > 0 &&
           options.maximum_cuda_input_records <=
               kAbsoluteMaximumCudaDeviceEvidence &&
           options.maximum_correlated_records > 0 &&
           options.maximum_correlated_records <=
               kAbsoluteMaximumGpuCorrelations &&
           options.maximum_cuda_associations_per_hardware > 0 &&
           options.maximum_cuda_associations_per_hardware <=
               kAbsoluteMaximumCudaAssociations &&
           options.maximum_runtime_only_records > 0 &&
           options.maximum_runtime_only_records <=
               kAbsoluteMaximumRuntimeOnlyEvidence &&
           options.maximum_issues > 0 &&
           options.maximum_issues <= kAbsoluteMaximumGpuEvidenceIssues &&
           options.maximum_stable_identity_bytes > 0 &&
           options.maximum_stable_identity_bytes <=
               kAbsoluteMaximumStableIdentityBytes &&
           options.maximum_diagnostic_bytes > 0 &&
           options.maximum_diagnostic_bytes <=
               kAbsoluteMaximumGpuDiagnosticBytes;
}

void retain_issue(
    GpuEvidenceCorrelationResult& result,
    const GpuEvidenceCorrelationOptions& options,
    GpuEvidenceIssueCode code,
    const std::string& identity = {},
    int runtime_ordinal = -1) {
    result.complete = false;
    if (result.issues.size() >= options.maximum_issues) {
        if (result.dropped_issues < std::numeric_limits<std::size_t>::max()) {
            ++result.dropped_issues;
        }
        return;
    }
    GpuEvidenceIssue issue{};
    issue.code = code;
    if (identity.size() <= options.maximum_diagnostic_bytes) {
        issue.identity = identity;
    }
    issue.runtime_ordinal = runtime_ordinal;
    result.issues.push_back(std::move(issue));
}

bool bounded_identity_syntax(const std::string& value) noexcept {
    if (value.empty()) return false;
    for (const unsigned char character : value) {
        if (std::isalnum(character) == 0 && character != '-' &&
            character != '_' && character != ':' && character != '.') {
            return false;
        }
    }
    return true;
}

bool valid_cuda_pci_identity(const GpuCudaPciIdentity& value) noexcept {
    return value.valid && value.domain <= 0xffffU && value.bus <= 0xffU &&
           value.device <= 0x1fU && value.function <= 0x07U;
}

bool same_hardware_identity(
    const LinuxPciDevice& left,
    const LinuxPciDevice& right) noexcept {
    return left.address.domain == right.address.domain &&
           left.address.bus == right.address.bus &&
           left.address.device == right.address.device &&
           left.address.function == right.address.function;
}

bool exact_pci_match(
    const LinuxPciDevice& hardware,
    const GpuCudaPciIdentity& cuda) noexcept {
    return cuda.domain_present &&
           hardware.address.domain == cuda.domain &&
           hardware.address.bus == cuda.bus &&
           hardware.address.device == cuda.device &&
           hardware.address.function == cuda.function;
}

bool partial_pci_match(
    const LinuxPciDevice& hardware,
    const GpuCudaPciIdentity& cuda) noexcept {
    return hardware.address.bus == cuda.bus &&
           hardware.address.device == cuda.device &&
           hardware.address.function == cuda.function;
}

bool runtime_device_less(
    const GpuCudaRuntimeDeviceEvidence& left,
    const GpuCudaRuntimeDeviceEvidence& right) {
    if (left.stable_identity_available != right.stable_identity_available) {
        return left.stable_identity_available;
    }
    if (left.stable_uuid != right.stable_uuid) {
        return left.stable_uuid < right.stable_uuid;
    }
    if (left.persistent_key != right.persistent_key) {
        return left.persistent_key < right.persistent_key;
    }
    if (left.pci_identity.domain_present != right.pci_identity.domain_present) {
        return left.pci_identity.domain_present;
    }
    if (left.pci_identity.domain != right.pci_identity.domain) {
        return left.pci_identity.domain < right.pci_identity.domain;
    }
    if (left.pci_identity.bus != right.pci_identity.bus) {
        return left.pci_identity.bus < right.pci_identity.bus;
    }
    if (left.pci_identity.device != right.pci_identity.device) {
        return left.pci_identity.device < right.pci_identity.device;
    }
    if (left.pci_identity.function != right.pci_identity.function) {
        return left.pci_identity.function < right.pci_identity.function;
    }
    return left.runtime_ordinal < right.runtime_ordinal;
}

bool hardware_less(const LinuxPciDevice& left, const LinuxPciDevice& right) {
    if (left.address.domain != right.address.domain) {
        return left.address.domain < right.address.domain;
    }
    if (left.address.bus != right.address.bus) {
        return left.address.bus < right.address.bus;
    }
    if (left.address.device != right.address.device) {
        return left.address.device < right.address.device;
    }
    if (left.address.function != right.address.function) {
        return left.address.function < right.address.function;
    }
    return left.address.canonical < right.address.canonical;
}

GpuEvidenceCorrelationResult resource_failure(
    GpuEvidenceCorrelationResult result) noexcept {
    result.code = GpuEvidenceResultCode::resource_exhausted;
    result.complete = false;
    result.correlated_hardware.clear();
    result.runtime_only.clear();
    result.capacity.retained_correlated_records = 0;
    result.capacity.retained_cuda_associations = 0;
    result.capacity.retained_runtime_only_records = 0;
    result.capacity.retained_issues = result.issues.size();
    return result;
}

}  // namespace

GpuEvidenceCorrelationResult correlate_gpu_evidence(
    const GpuEvidenceInput& input,
    const GpuEvidenceCorrelationOptions& options) noexcept {
    try {
        GpuEvidenceCorrelationResult result{};
        result.software = input.software;
        result.hardware.state = input.hardware.state;
        result.hardware.input_records = input.hardware.inventory.devices.size();
        result.hardware.zero_matching_hardware =
            input.hardware.inventory.devices.empty();
        result.hardware.resource_exhausted =
            input.hardware.resource_exhausted;
        result.hardware.malformed_observations =
            input.hardware.malformed_observations;
        result.cuda_enumeration.state = input.cuda_enumeration.state;
        result.cuda_enumeration.reported_device_count =
            input.cuda_enumeration.reported_device_count;
        result.cuda_enumeration.reported_stable_identity_count =
            input.cuda_enumeration.reported_stable_identity_count;
        result.cuda_enumeration.reported_capability_count =
            input.cuda_enumeration.reported_capability_count;
        result.cuda_enumeration.reported_registration_ready_count =
            input.cuda_enumeration.reported_registration_ready_count;
        result.cuda_enumeration.input_records =
            input.cuda_enumeration.devices.size();
        result.cuda_enumeration.native_error_code =
            input.cuda_enumeration.native_error_code;
        result.capacity.hardware_input_records =
            input.hardware.inventory.devices.size();
        result.capacity.cuda_input_records =
            input.cuda_enumeration.devices.size();

        if (!valid_options(options)) {
            result.code = GpuEvidenceResultCode::invalid_input;
            retain_issue(result, options, GpuEvidenceIssueCode::invalid_options);
            return result;
        }
        if (input.hardware.inventory.devices.size() >
            options.maximum_hardware_input_records) {
            retain_issue(
                result, options,
                GpuEvidenceIssueCode::hardware_input_capacity_exhausted);
            return resource_failure(std::move(result));
        }
        if (input.cuda_enumeration.devices.size() >
            options.maximum_cuda_input_records) {
            retain_issue(
                result, options,
                GpuEvidenceIssueCode::cuda_input_capacity_exhausted);
            return resource_failure(std::move(result));
        }
        if (input.hardware.inventory.devices.size() >
            options.maximum_correlated_records) {
            retain_issue(
                result, options,
                GpuEvidenceIssueCode::correlated_capacity_exhausted);
            return resource_failure(std::move(result));
        }

        std::vector<LinuxPciDevice> hardware =
            input.hardware.inventory.devices;
        std::sort(hardware.begin(), hardware.end(), hardware_less);
        std::vector<bool> invalid_hardware(hardware.size(), false);
        bool invalid_input = false;
        bool identity_conflict = false;

        for (std::size_t index = 0; index < hardware.size(); ++index) {
            const PciAddress reparsed =
                parse_pci_address(hardware[index].address.canonical);
            if (!hardware[index].address.valid || !reparsed.valid ||
                reparsed.domain != hardware[index].address.domain ||
                reparsed.bus != hardware[index].address.bus ||
                reparsed.device != hardware[index].address.device ||
                reparsed.function != hardware[index].address.function ||
                reparsed.canonical != hardware[index].address.canonical) {
                invalid_hardware[index] = true;
                invalid_input = true;
                retain_issue(
                    result, options,
                    GpuEvidenceIssueCode::invalid_hardware_identity,
                    hardware[index].address.canonical);
            }
            if (index > 0 && !invalid_hardware[index - 1] &&
                !invalid_hardware[index] &&
                same_hardware_identity(hardware[index - 1], hardware[index])) {
                invalid_hardware[index - 1] = true;
                invalid_hardware[index] = true;
                identity_conflict = true;
                retain_issue(
                    result, options,
                    GpuEvidenceIssueCode::duplicate_hardware_pci_identity,
                    hardware[index].address.canonical);
            }
        }

        const bool enumeration_shape_invalid =
            ((input.cuda_enumeration.state ==
                  GpuCudaEnumerationState::not_evaluated ||
              input.cuda_enumeration.state ==
                  GpuCudaEnumerationState::success_no_devices) &&
             !input.cuda_enumeration.devices.empty()) ||
            (input.cuda_enumeration.state ==
                 GpuCudaEnumerationState::success_with_devices &&
             input.cuda_enumeration.devices.empty());
        if (enumeration_shape_invalid) {
            invalid_input = true;
            retain_issue(
                result, options,
                GpuEvidenceIssueCode::inconsistent_cuda_enumeration);
        }

        std::vector<GpuCudaRuntimeDeviceEvidence> cuda_devices{};
        cuda_devices.reserve(input.cuda_enumeration.devices.size());
        std::vector<int> preinvalid_ordinals{};
        preinvalid_ordinals.reserve(input.cuda_enumeration.devices.size());
        for (const auto& source : input.cuda_enumeration.devices) {
            GpuCudaRuntimeDeviceEvidence bounded{};
            bounded.runtime_ordinal = source.runtime_ordinal;
            bounded.pci_identity_available = source.pci_identity_available;
            bounded.pci_identity = source.pci_identity;
            bounded.capabilities_available = source.capabilities_available;
            bounded.compatibility = source.compatibility;
            bounded.backend_registration = source.backend_registration;
            bounded.runtime_binding = source.runtime_binding;
            bounded.execution = source.execution;
            bounded.native_error_code = source.native_error_code;

            const bool stable_identity_valid =
                source.stable_identity_available &&
                source.stable_uuid.size() <=
                    options.maximum_stable_identity_bytes &&
                source.persistent_key.size() <=
                    options.maximum_stable_identity_bytes &&
                bounded_identity_syntax(source.stable_uuid) &&
                bounded_identity_syntax(source.persistent_key);
            const bool stable_identity_absent =
                !source.stable_identity_available &&
                source.stable_uuid.empty() && source.persistent_key.empty();
            if (stable_identity_valid) {
                bounded.stable_identity_available = true;
                bounded.stable_uuid = source.stable_uuid;
                bounded.persistent_key = source.persistent_key;
            } else if (!stable_identity_absent) {
                preinvalid_ordinals.push_back(source.runtime_ordinal);
                invalid_input = true;
            }
            cuda_devices.push_back(std::move(bounded));
        }
        std::sort(cuda_devices.begin(), cuda_devices.end(), runtime_device_less);
        std::vector<bool> invalid_cuda(cuda_devices.size(), false);

        for (std::size_t index = 0; index < cuda_devices.size(); ++index) {
            const auto& device = cuda_devices[index];
            invalid_cuda[index] = std::find(
                preinvalid_ordinals.begin(), preinvalid_ordinals.end(),
                device.runtime_ordinal) != preinvalid_ordinals.end();
            if (invalid_cuda[index]) {
                retain_issue(
                    result, options,
                    GpuEvidenceIssueCode::invalid_cuda_stable_identity,
                    {}, device.runtime_ordinal);
            }
            if (device.runtime_ordinal < 0) {
                invalid_cuda[index] = true;
                invalid_input = true;
                retain_issue(
                    result, options,
                    GpuEvidenceIssueCode::duplicate_cuda_runtime_ordinal,
                    {}, device.runtime_ordinal);
            }
            if (device.pci_identity_available &&
                !valid_cuda_pci_identity(device.pci_identity)) {
                invalid_cuda[index] = true;
                invalid_input = true;
                retain_issue(
                    result, options,
                    GpuEvidenceIssueCode::invalid_cuda_pci_identity,
                    device.persistent_key, device.runtime_ordinal);
            }
        }

        for (std::size_t left = 0; left < cuda_devices.size(); ++left) {
            for (std::size_t right = left + 1;
                 right < cuda_devices.size(); ++right) {
                if (cuda_devices[left].runtime_ordinal ==
                    cuda_devices[right].runtime_ordinal) {
                    invalid_cuda[left] = true;
                    invalid_cuda[right] = true;
                    identity_conflict = true;
                    retain_issue(
                        result, options,
                        GpuEvidenceIssueCode::duplicate_cuda_runtime_ordinal,
                        {}, cuda_devices[right].runtime_ordinal);
                }
                if (cuda_devices[left].stable_identity_available &&
                    cuda_devices[right].stable_identity_available &&
                    (cuda_devices[left].stable_uuid ==
                         cuda_devices[right].stable_uuid ||
                     cuda_devices[left].persistent_key ==
                         cuda_devices[right].persistent_key)) {
                    invalid_cuda[left] = true;
                    invalid_cuda[right] = true;
                    identity_conflict = true;
                    retain_issue(
                        result, options,
                        GpuEvidenceIssueCode::duplicate_cuda_stable_identity,
                        cuda_devices[right].stable_uuid,
                        cuda_devices[right].runtime_ordinal);
                }
            }
        }

        std::vector<PlannedCudaMatch> planned{};
        planned.reserve(cuda_devices.size());
        for (std::size_t cuda_index = 0;
             cuda_index < cuda_devices.size(); ++cuda_index) {
            PlannedCudaMatch match{};
            match.device = cuda_devices[cuda_index];
            if (invalid_cuda[cuda_index]) {
                match.state = GpuCorrelationState::invalid_identity;
                planned.push_back(std::move(match));
                continue;
            }
            if (!match.device.pci_identity_available) {
                match.state = GpuCorrelationState::unavailable;
                planned.push_back(std::move(match));
                continue;
            }

            std::vector<std::size_t> candidates{};
            candidates.reserve(hardware.size());
            for (std::size_t hardware_index = 0;
                 hardware_index < hardware.size(); ++hardware_index) {
                if (invalid_hardware[hardware_index]) continue;
                const bool matched = match.device.pci_identity.domain_present
                    ? exact_pci_match(
                          hardware[hardware_index],
                          match.device.pci_identity)
                    : partial_pci_match(
                          hardware[hardware_index],
                          match.device.pci_identity);
                if (matched) candidates.push_back(hardware_index);
            }

            if (candidates.size() == 1) {
                match.has_hardware = true;
                match.hardware_index = candidates.front();
                match.state = match.device.pci_identity.domain_present
                                  ? GpuCorrelationState::exact_match
                                  : GpuCorrelationState::inferred_unique_match;
            } else if (candidates.size() > 1) {
                match.state = GpuCorrelationState::ambiguous;
                identity_conflict = true;
                retain_issue(
                    result, options,
                    GpuEvidenceIssueCode::ambiguous_pci_identity,
                    match.device.persistent_key,
                    match.device.runtime_ordinal);
            }
            planned.push_back(std::move(match));
        }

        for (std::size_t hardware_index = 0;
             hardware_index < hardware.size(); ++hardware_index) {
            std::size_t unstable_matches = 0;
            for (const PlannedCudaMatch& match : planned) {
                if (match.has_hardware &&
                    match.hardware_index == hardware_index &&
                    !match.device.stable_identity_available) {
                    ++unstable_matches;
                }
            }
            if (unstable_matches > 1) {
                identity_conflict = true;
                retain_issue(
                    result, options,
                    GpuEvidenceIssueCode::unstable_one_to_many_identity,
                    hardware[hardware_index].address.canonical);
                for (PlannedCudaMatch& match : planned) {
                    if (match.has_hardware &&
                        match.hardware_index == hardware_index &&
                        !match.device.stable_identity_available) {
                        match.has_hardware = false;
                        match.state = GpuCorrelationState::ambiguous;
                    }
                }
            }
        }

        std::vector<std::size_t> association_counts(hardware.size(), 0);
        std::size_t runtime_only_count = 0;
        for (const PlannedCudaMatch& match : planned) {
            if (match.has_hardware) {
                ++association_counts[match.hardware_index];
            } else {
                ++runtime_only_count;
            }
        }
        for (std::size_t count : association_counts) {
            if (count > options.maximum_cuda_associations_per_hardware) {
                retain_issue(
                    result, options,
                    GpuEvidenceIssueCode::association_capacity_exhausted);
                return resource_failure(std::move(result));
            }
        }
        if (runtime_only_count > options.maximum_runtime_only_records) {
            retain_issue(
                result, options,
                GpuEvidenceIssueCode::runtime_only_capacity_exhausted);
            return resource_failure(std::move(result));
        }

        result.correlated_hardware.reserve(hardware.size());
        for (std::size_t hardware_index = 0;
             hardware_index < hardware.size(); ++hardware_index) {
            GpuCorrelatedHardwareRecord record{};
            record.hardware = hardware[hardware_index];
            if (invalid_hardware[hardware_index]) {
                record.correlation = GpuCorrelationState::invalid_identity;
            } else if (input.cuda_enumeration.state ==
                       GpuCudaEnumerationState::not_evaluated) {
                record.correlation = GpuCorrelationState::not_evaluated;
            } else if (input.cuda_enumeration.state ==
                       GpuCudaEnumerationState::failed) {
                record.correlation = GpuCorrelationState::unavailable;
            }

            for (const PlannedCudaMatch& match : planned) {
                if (!match.has_hardware ||
                    match.hardware_index != hardware_index) {
                    continue;
                }
                GpuCudaAssociation association{};
                association.device = match.device;
                association.correlation = match.state;
                record.cuda_devices.push_back(std::move(association));
                if (record.correlation == GpuCorrelationState::hardware_only ||
                    record.correlation == GpuCorrelationState::not_evaluated ||
                    record.correlation == GpuCorrelationState::unavailable) {
                    record.correlation = match.state;
                } else if (record.correlation == GpuCorrelationState::exact_match &&
                           match.state ==
                               GpuCorrelationState::inferred_unique_match) {
                    record.correlation =
                        GpuCorrelationState::inferred_unique_match;
                }
            }
            result.capacity.retained_cuda_associations +=
                record.cuda_devices.size();
            result.correlated_hardware.push_back(std::move(record));
        }

        result.runtime_only.reserve(runtime_only_count);
        for (const PlannedCudaMatch& match : planned) {
            if (match.has_hardware) continue;
            GpuRuntimeOnlyRecord record{};
            record.device = match.device;
            record.correlation = match.state;
            result.runtime_only.push_back(std::move(record));
        }

        result.hardware.retained_records = result.correlated_hardware.size();
        result.cuda_enumeration.retained_records =
            input.cuda_enumeration.devices.size();
        for (const auto& device : input.cuda_enumeration.devices) {
            if (device.stable_identity_available) {
                ++result.cuda_enumeration.stable_identity_count;
            }
            if (device.capabilities_available) {
                ++result.cuda_enumeration.capability_count;
            }
            if (device.backend_registration == GpuReadinessState::ready) {
                ++result.cuda_enumeration.registration_ready_count;
            }
        }
        result.capacity.retained_correlated_records =
            result.correlated_hardware.size();
        result.capacity.retained_runtime_only_records =
            result.runtime_only.size();
        result.capacity.retained_issues = result.issues.size();

        const bool hardware_unavailable =
            input.hardware.state == GpuPhysicalInventoryState::unavailable ||
            input.hardware.state == GpuPhysicalInventoryState::failed ||
            input.hardware.state == GpuPhysicalInventoryState::not_evaluated;
        const bool cuda_not_evaluated =
            input.cuda_enumeration.state ==
            GpuCudaEnumerationState::not_evaluated;
        const bool cuda_failed =
            input.cuda_enumeration.state == GpuCudaEnumerationState::failed;
        const bool partial_source =
            input.hardware.state == GpuPhysicalInventoryState::partial ||
            input.cuda_enumeration.state == GpuCudaEnumerationState::partial ||
            input.hardware.resource_exhausted ||
            input.hardware.malformed_observations;

        if (result.dropped_issues != 0) {
            return resource_failure(std::move(result));
        }
        if (identity_conflict) {
            result.code = GpuEvidenceResultCode::identity_conflict;
            result.complete = false;
        } else if (invalid_input) {
            result.code = GpuEvidenceResultCode::invalid_input;
            result.complete = false;
        } else if (hardware_unavailable) {
            result.code =
                GpuEvidenceResultCode::hardware_inventory_unavailable;
            result.complete = false;
        } else if (cuda_not_evaluated) {
            result.code = GpuEvidenceResultCode::cuda_not_evaluated;
            result.complete = false;
        } else if (cuda_failed) {
            result.code = GpuEvidenceResultCode::cuda_enumeration_failed;
            result.complete = false;
        } else if (partial_source || !result.runtime_only.empty()) {
            result.code = GpuEvidenceResultCode::partial;
            result.complete = false;
        } else {
            const bool hardware_only_with_visible_cuda =
                !input.cuda_enumeration.devices.empty() &&
                std::any_of(
                    result.correlated_hardware.begin(),
                    result.correlated_hardware.end(),
                    [](const GpuCorrelatedHardwareRecord& record) {
                        return record.correlation ==
                               GpuCorrelationState::hardware_only;
                    });
            if (hardware_only_with_visible_cuda) {
                result.code = GpuEvidenceResultCode::partial;
                result.complete = false;
            }
        }
        return result;
    } catch (...) {
        GpuEvidenceCorrelationResult result{};
        result.code = GpuEvidenceResultCode::resource_exhausted;
        result.complete = false;
        return result;
    }
}

#define GPU_EVIDENCE_TO_STRING_CASE(type, name) case type::name: return #name

const char* to_string(GpuPhysicalInventoryState value) noexcept {
    switch (value) {
        GPU_EVIDENCE_TO_STRING_CASE(GpuPhysicalInventoryState, not_evaluated);
        GPU_EVIDENCE_TO_STRING_CASE(GpuPhysicalInventoryState, unavailable);
        GPU_EVIDENCE_TO_STRING_CASE(GpuPhysicalInventoryState, failed);
        GPU_EVIDENCE_TO_STRING_CASE(GpuPhysicalInventoryState, complete);
        GPU_EVIDENCE_TO_STRING_CASE(GpuPhysicalInventoryState, partial);
    }
    return "unknown";
}

const char* to_string(GpuCudaEnumerationState value) noexcept {
    switch (value) {
        GPU_EVIDENCE_TO_STRING_CASE(GpuCudaEnumerationState, not_evaluated);
        GPU_EVIDENCE_TO_STRING_CASE(GpuCudaEnumerationState, success_with_devices);
        GPU_EVIDENCE_TO_STRING_CASE(GpuCudaEnumerationState, success_no_devices);
        GPU_EVIDENCE_TO_STRING_CASE(GpuCudaEnumerationState, partial);
        GPU_EVIDENCE_TO_STRING_CASE(GpuCudaEnumerationState, failed);
    }
    return "unknown";
}

const char* to_string(GpuEvidenceSupportState value) noexcept {
    switch (value) {
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceSupportState, unknown);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceSupportState, unsupported);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceSupportState, supported);
    }
    return "unknown";
}

const char* to_string(GpuCudaReleaseFamily value) noexcept {
    switch (value) {
        GPU_EVIDENCE_TO_STRING_CASE(GpuCudaReleaseFamily, unknown);
        GPU_EVIDENCE_TO_STRING_CASE(GpuCudaReleaseFamily, cuda_12_legacy);
        GPU_EVIDENCE_TO_STRING_CASE(GpuCudaReleaseFamily, cuda_13_modern);
    }
    return "unknown";
}

const char* to_string(GpuCorrelationState value) noexcept {
    switch (value) {
        GPU_EVIDENCE_TO_STRING_CASE(GpuCorrelationState, not_evaluated);
        GPU_EVIDENCE_TO_STRING_CASE(GpuCorrelationState, exact_match);
        GPU_EVIDENCE_TO_STRING_CASE(GpuCorrelationState, inferred_unique_match);
        GPU_EVIDENCE_TO_STRING_CASE(GpuCorrelationState, hardware_only);
        GPU_EVIDENCE_TO_STRING_CASE(GpuCorrelationState, runtime_only);
        GPU_EVIDENCE_TO_STRING_CASE(GpuCorrelationState, ambiguous);
        GPU_EVIDENCE_TO_STRING_CASE(GpuCorrelationState, invalid_identity);
        GPU_EVIDENCE_TO_STRING_CASE(GpuCorrelationState, unavailable);
    }
    return "unknown";
}

const char* to_string(GpuReadinessState value) noexcept {
    switch (value) {
        GPU_EVIDENCE_TO_STRING_CASE(GpuReadinessState, not_evaluated);
        GPU_EVIDENCE_TO_STRING_CASE(GpuReadinessState, unknown);
        GPU_EVIDENCE_TO_STRING_CASE(GpuReadinessState, not_ready);
        GPU_EVIDENCE_TO_STRING_CASE(GpuReadinessState, ready);
    }
    return "unknown";
}

const char* to_string(GpuRuntimeBindingState value) noexcept {
    switch (value) {
        GPU_EVIDENCE_TO_STRING_CASE(GpuRuntimeBindingState, not_evaluated);
        GPU_EVIDENCE_TO_STRING_CASE(GpuRuntimeBindingState, unknown);
        GPU_EVIDENCE_TO_STRING_CASE(GpuRuntimeBindingState, unusable);
        GPU_EVIDENCE_TO_STRING_CASE(GpuRuntimeBindingState, usable);
    }
    return "unknown";
}

const char* to_string(GpuKernelCoverageState value) noexcept {
    switch (value) {
        GPU_EVIDENCE_TO_STRING_CASE(GpuKernelCoverageState, not_evaluated);
        GPU_EVIDENCE_TO_STRING_CASE(GpuKernelCoverageState, unknown);
        GPU_EVIDENCE_TO_STRING_CASE(GpuKernelCoverageState, unavailable);
        GPU_EVIDENCE_TO_STRING_CASE(GpuKernelCoverageState, partial);
        GPU_EVIDENCE_TO_STRING_CASE(GpuKernelCoverageState, complete);
    }
    return "unknown";
}

const char* to_string(GpuAdmissionState value) noexcept {
    switch (value) {
        GPU_EVIDENCE_TO_STRING_CASE(GpuAdmissionState, not_evaluated);
    }
    return "unknown";
}

const char* to_string(GpuEvidenceIssueCode value) noexcept {
    switch (value) {
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, invalid_options);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, hardware_input_capacity_exhausted);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, cuda_input_capacity_exhausted);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, correlated_capacity_exhausted);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, association_capacity_exhausted);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, runtime_only_capacity_exhausted);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, issue_capacity_exhausted);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, invalid_hardware_identity);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, duplicate_hardware_pci_identity);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, invalid_cuda_pci_identity);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, invalid_cuda_stable_identity);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, inconsistent_cuda_enumeration);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, duplicate_cuda_stable_identity);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, duplicate_cuda_runtime_ordinal);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, ambiguous_pci_identity);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceIssueCode, unstable_one_to_many_identity);
    }
    return "unknown";
}

const char* to_string(GpuEvidenceResultCode value) noexcept {
    switch (value) {
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceResultCode, success);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceResultCode, partial);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceResultCode, invalid_input);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceResultCode, identity_conflict);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceResultCode, resource_exhausted);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceResultCode, hardware_inventory_unavailable);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceResultCode, cuda_not_evaluated);
        GPU_EVIDENCE_TO_STRING_CASE(GpuEvidenceResultCode, cuda_enumeration_failed);
    }
    return "unknown";
}

#undef GPU_EVIDENCE_TO_STRING_CASE

}  // namespace prometheus::hardware
