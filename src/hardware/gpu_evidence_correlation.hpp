#ifndef SRC_HARDWARE_GPU_EVIDENCE_CORRELATION_HPP
#define SRC_HARDWARE_GPU_EVIDENCE_CORRELATION_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "linux_pci_inventory.hpp"

namespace prometheus::hardware {

inline constexpr std::size_t kDefaultMaximumGpuHardwareEvidence = 256;
inline constexpr std::size_t kDefaultMaximumCudaDeviceEvidence = 256;
inline constexpr std::size_t kDefaultMaximumGpuCorrelations = 256;
inline constexpr std::size_t kDefaultMaximumCudaAssociations = 64;
inline constexpr std::size_t kDefaultMaximumRuntimeOnlyEvidence = 256;
inline constexpr std::size_t kDefaultMaximumGpuEvidenceIssues = 256;
inline constexpr std::size_t kDefaultMaximumStableIdentityBytes = 128;
inline constexpr std::size_t kDefaultMaximumGpuDiagnosticBytes = 256;

inline constexpr std::size_t kAbsoluteMaximumGpuHardwareEvidence = 4096;
inline constexpr std::size_t kAbsoluteMaximumCudaDeviceEvidence = 4096;
inline constexpr std::size_t kAbsoluteMaximumGpuCorrelations = 4096;
inline constexpr std::size_t kAbsoluteMaximumCudaAssociations = 1024;
inline constexpr std::size_t kAbsoluteMaximumRuntimeOnlyEvidence = 4096;
inline constexpr std::size_t kAbsoluteMaximumGpuEvidenceIssues = 4096;
inline constexpr std::size_t kAbsoluteMaximumStableIdentityBytes = 512;
inline constexpr std::size_t kAbsoluteMaximumGpuDiagnosticBytes = 1024;

enum class GpuPhysicalInventoryState : std::uint8_t {
    not_evaluated = 0,
    unavailable,
    failed,
    complete,
    partial,
};

enum class GpuCudaEnumerationState : std::uint8_t {
    not_evaluated = 0,
    success_with_devices,
    success_no_devices,
    partial,
    failed,
};

enum class GpuEvidenceSupportState : std::uint8_t {
    unknown = 0,
    unsupported,
    supported,
};

enum class GpuCudaReleaseFamily : std::uint8_t {
    unknown = 0,
    cuda_12_legacy,
    cuda_13_modern,
};

enum class GpuCorrelationState : std::uint8_t {
    not_evaluated = 0,
    exact_match,
    inferred_unique_match,
    hardware_only,
    runtime_only,
    ambiguous,
    invalid_identity,
    unavailable,
};

enum class GpuReadinessState : std::uint8_t {
    not_evaluated = 0,
    unknown,
    not_ready,
    ready,
};

enum class GpuRuntimeBindingState : std::uint8_t {
    not_evaluated = 0,
    unknown,
    unusable,
    usable,
};

enum class GpuKernelCoverageState : std::uint8_t {
    not_evaluated = 0,
    unknown,
    unavailable,
    partial,
    complete,
};

enum class GpuAdmissionState : std::uint8_t {
    not_evaluated = 0,
};

struct GpuVersionEvidence {
    int encoded = 0;
    int major = 0;
    int minor = 0;
    int patch = 0;
};

struct GpuCudaSoftwareEvidence {
    bool compile_time_toolkit_available = false;
    GpuVersionEvidence compile_time_toolkit{};
    GpuCudaReleaseFamily compile_time_release_family =
        GpuCudaReleaseFamily::unknown;

    bool runtime_version_query_available = false;
    GpuVersionEvidence runtime{};
    GpuCudaReleaseFamily runtime_release_family =
        GpuCudaReleaseFamily::unknown;

    bool driver_version_query_available = false;
    GpuVersionEvidence driver{};
    GpuEvidenceSupportState driver_supports_runtime =
        GpuEvidenceSupportState::unknown;
};

struct GpuCudaPciIdentity {
    std::uint32_t domain = 0;
    std::uint32_t bus = 0;
    std::uint32_t device = 0;
    std::uint32_t function = 0;
    bool domain_present = false;
    bool valid = false;
};

struct GpuCompatibilityEvidence {
    GpuEvidenceSupportState project_architecture =
        GpuEvidenceSupportState::unknown;
    GpuEvidenceSupportState compile_time_release =
        GpuEvidenceSupportState::unknown;
    GpuEvidenceSupportState runtime_release =
        GpuEvidenceSupportState::unknown;
    GpuEvidenceSupportState binary_architecture_image =
        GpuEvidenceSupportState::unknown;
    GpuKernelCoverageState registered_kernel_coverage =
        GpuKernelCoverageState::not_evaluated;
};

struct GpuCudaRuntimeDeviceEvidence {
    int runtime_ordinal = -1;
    bool stable_identity_available = false;
    std::string stable_uuid{};
    std::string persistent_key{};

    bool pci_identity_available = false;
    GpuCudaPciIdentity pci_identity{};
    bool capabilities_available = false;
    GpuCompatibilityEvidence compatibility{};

    GpuReadinessState backend_registration =
        GpuReadinessState::not_evaluated;
    GpuRuntimeBindingState runtime_binding =
        GpuRuntimeBindingState::not_evaluated;
    GpuReadinessState execution = GpuReadinessState::not_evaluated;
    int native_error_code = 0;
};

struct GpuHardwareInventoryEvidence {
    GpuPhysicalInventoryState state =
        GpuPhysicalInventoryState::not_evaluated;
    bool resource_exhausted = false;
    bool malformed_observations = false;
    LinuxPciInventoryResult inventory{};
};

struct GpuCudaEnumerationEvidence {
    GpuCudaEnumerationState state =
        GpuCudaEnumerationState::not_evaluated;
    int reported_device_count = 0;
    std::size_t reported_stable_identity_count = 0;
    std::size_t reported_capability_count = 0;
    std::size_t reported_registration_ready_count = 0;
    int native_error_code = 0;
    std::vector<GpuCudaRuntimeDeviceEvidence> devices{};
};

struct GpuEvidenceInput {
    GpuHardwareInventoryEvidence hardware{};
    GpuCudaSoftwareEvidence software{};
    GpuCudaEnumerationEvidence cuda_enumeration{};
};

struct GpuEvidenceCorrelationOptions {
    std::size_t maximum_hardware_input_records =
        kDefaultMaximumGpuHardwareEvidence;
    std::size_t maximum_cuda_input_records =
        kDefaultMaximumCudaDeviceEvidence;
    std::size_t maximum_correlated_records =
        kDefaultMaximumGpuCorrelations;
    std::size_t maximum_cuda_associations_per_hardware =
        kDefaultMaximumCudaAssociations;
    std::size_t maximum_runtime_only_records =
        kDefaultMaximumRuntimeOnlyEvidence;
    std::size_t maximum_issues = kDefaultMaximumGpuEvidenceIssues;
    std::size_t maximum_stable_identity_bytes =
        kDefaultMaximumStableIdentityBytes;
    std::size_t maximum_diagnostic_bytes =
        kDefaultMaximumGpuDiagnosticBytes;
};

enum class GpuEvidenceIssueCode : std::uint8_t {
    invalid_options = 0,
    hardware_input_capacity_exhausted,
    cuda_input_capacity_exhausted,
    correlated_capacity_exhausted,
    association_capacity_exhausted,
    runtime_only_capacity_exhausted,
    issue_capacity_exhausted,
    invalid_hardware_identity,
    duplicate_hardware_pci_identity,
    invalid_cuda_pci_identity,
    invalid_cuda_stable_identity,
    inconsistent_cuda_enumeration,
    duplicate_cuda_stable_identity,
    duplicate_cuda_runtime_ordinal,
    ambiguous_pci_identity,
    unstable_one_to_many_identity,
};

struct GpuEvidenceIssue {
    GpuEvidenceIssueCode code = GpuEvidenceIssueCode::invalid_options;
    std::string identity{};
    int runtime_ordinal = -1;
};

enum class GpuEvidenceResultCode : std::uint8_t {
    success = 0,
    partial,
    invalid_input,
    identity_conflict,
    resource_exhausted,
    hardware_inventory_unavailable,
    cuda_not_evaluated,
    cuda_enumeration_failed,
};

struct GpuHardwareEvidenceSummary {
    GpuPhysicalInventoryState state =
        GpuPhysicalInventoryState::not_evaluated;
    std::size_t input_records = 0;
    std::size_t retained_records = 0;
    bool zero_matching_hardware = false;
    bool resource_exhausted = false;
    bool malformed_observations = false;
};

struct GpuCudaEvidenceSummary {
    GpuCudaEnumerationState state =
        GpuCudaEnumerationState::not_evaluated;
    int reported_device_count = 0;
    std::size_t input_records = 0;
    std::size_t retained_records = 0;
    std::size_t reported_stable_identity_count = 0;
    std::size_t reported_capability_count = 0;
    std::size_t reported_registration_ready_count = 0;
    std::size_t stable_identity_count = 0;
    std::size_t capability_count = 0;
    std::size_t registration_ready_count = 0;
    int native_error_code = 0;
};

struct GpuCudaAssociation {
    GpuCudaRuntimeDeviceEvidence device{};
    GpuCorrelationState correlation = GpuCorrelationState::not_evaluated;
};

struct GpuCorrelatedHardwareRecord {
    LinuxPciDevice hardware{};
    GpuCorrelationState correlation = GpuCorrelationState::hardware_only;
    std::vector<GpuCudaAssociation> cuda_devices{};
    GpuAdmissionState admission = GpuAdmissionState::not_evaluated;
    std::vector<GpuEvidenceIssue> issues{};
};

struct GpuRuntimeOnlyRecord {
    GpuCudaRuntimeDeviceEvidence device{};
    GpuCorrelationState correlation = GpuCorrelationState::runtime_only;
    GpuAdmissionState admission = GpuAdmissionState::not_evaluated;
};

struct GpuEvidenceCapacityCounters {
    std::size_t hardware_input_records = 0;
    std::size_t cuda_input_records = 0;
    std::size_t retained_correlated_records = 0;
    std::size_t retained_cuda_associations = 0;
    std::size_t retained_runtime_only_records = 0;
    std::size_t retained_issues = 0;
};

struct GpuEvidenceCorrelationResult {
    GpuEvidenceResultCode code = GpuEvidenceResultCode::success;
    bool complete = true;
    GpuHardwareEvidenceSummary hardware{};
    GpuCudaSoftwareEvidence software{};
    GpuCudaEvidenceSummary cuda_enumeration{};
    std::vector<GpuCorrelatedHardwareRecord> correlated_hardware{};
    std::vector<GpuRuntimeOnlyRecord> runtime_only{};
    std::vector<GpuEvidenceIssue> issues{};
    std::size_t dropped_issues = 0;
    GpuEvidenceCapacityCounters capacity{};
};

// Pure metadata correlation. Inputs are not mutated and no hardware, runtime,
// backend, registry, admission, filesystem, or executable operation is invoked.
[[nodiscard]] GpuEvidenceCorrelationResult correlate_gpu_evidence(
    const GpuEvidenceInput& input,
    const GpuEvidenceCorrelationOptions& options = {}) noexcept;

[[nodiscard]] const char* to_string(GpuPhysicalInventoryState value) noexcept;
[[nodiscard]] const char* to_string(GpuCudaEnumerationState value) noexcept;
[[nodiscard]] const char* to_string(GpuEvidenceSupportState value) noexcept;
[[nodiscard]] const char* to_string(GpuCudaReleaseFamily value) noexcept;
[[nodiscard]] const char* to_string(GpuCorrelationState value) noexcept;
[[nodiscard]] const char* to_string(GpuReadinessState value) noexcept;
[[nodiscard]] const char* to_string(GpuRuntimeBindingState value) noexcept;
[[nodiscard]] const char* to_string(GpuKernelCoverageState value) noexcept;
[[nodiscard]] const char* to_string(GpuAdmissionState value) noexcept;
[[nodiscard]] const char* to_string(GpuEvidenceIssueCode value) noexcept;
[[nodiscard]] const char* to_string(GpuEvidenceResultCode value) noexcept;

}  // namespace prometheus::hardware

#endif  // SRC_HARDWARE_GPU_EVIDENCE_CORRELATION_HPP
