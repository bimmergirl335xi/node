#ifndef SRC_CORE_EXECUTION_POLICY_HPP
#define SRC_CORE_EXECUTION_POLICY_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "backends/compute_backend.hpp"

namespace prometheus::core {

inline constexpr std::size_t kMaximumExecutionIdentityLength = 128;
inline constexpr std::size_t kMaximumPlacementListEntries = 64;
inline constexpr std::size_t kMaximumExecutionProfileLength = 128;

struct WorkloadIdentity {
    std::string workload_id{};
    std::string request_id{};
    std::string retry_correlation_id{};
    std::string cancellation_correlation_id{};
};

enum class ResourceTenancy : std::uint8_t {
    unknown = 0,
    exclusive,
    shareable,
    concurrently_shareable,
    partitioned,
};

enum class PolicySupport : std::uint8_t {
    unknown = 0,
    unsupported,
    supported,
};

enum class WorkloadCooperation : std::uint8_t {
    unknown = 0,
    cooperative,
    non_cooperative,
};

enum class LatencySensitivity : std::uint8_t {
    unknown = 0,
    best_effort,
    latency_sensitive,
};

enum class ExecutionPreference : std::uint8_t {
    balanced = 0,
    latency,
    throughput,
};

enum class PreemptionAssumption : std::uint8_t {
    unknown = 0,
    not_required,
    required,
};

enum class ResourceOperationalState : std::uint8_t {
    unknown = 0,
    ready,
    degraded,
    unavailable,
    failed,
};

struct WorkloadConcurrencyConstraints {
    // Zero means that no explicit lane count is requested.
    std::size_t requested_execution_lanes = 0;

    // Zero means no workload-side co-residency ceiling.
    std::size_t maximum_co_resident_workloads = 0;

    WorkloadCooperation cooperation = WorkloadCooperation::unknown;
    LatencySensitivity latency_sensitivity = LatencySensitivity::unknown;
    ExecutionPreference preference = ExecutionPreference::balanced;
    PreemptionAssumption preemption = PreemptionAssumption::unknown;
};

struct PlacementConstraints {
    std::vector<backends::ComputeBackendKind> allowed_backend_kinds{};
    std::vector<std::string> allowed_resource_ids{};
    std::vector<std::string> excluded_resource_ids{};
    std::string required_backend_id{};
    std::string required_execution_profile{};
    std::vector<std::string> required_operation_adapters{};
    bool require_ready = true;
};

struct ExecutionPolicyRequest {
    WorkloadIdentity identity{};
    ResourceTenancy tenancy = ResourceTenancy::unknown;
    WorkloadConcurrencyConstraints concurrency{};
    PlacementConstraints placement{};
    std::uint64_t required_memory_bytes = 0;
    std::uint64_t reservation_headroom_bytes = 0;
};

struct ResourceConcurrencyPolicy {
    PolicySupport concurrent_workload_support = PolicySupport::unknown;
    PolicySupport concurrent_lane_support = PolicySupport::unknown;
    PolicySupport preemption_support = PolicySupport::unknown;

    // Zero means the limit is unknown, not unlimited.
    std::size_t maximum_concurrent_workloads = 0;
    std::size_t maximum_execution_lanes = 0;
    std::size_t admitted_workloads = 0;
};

struct ExecutionPolicyResource {
    std::string resource_id{};
    std::string backend_id{};
    backends::ComputeBackendKind backend_kind =
        backends::ComputeBackendKind::unknown;
    std::string execution_profile{};
    std::vector<std::string> operation_adapters{};

    ResourceOperationalState operational_state =
        ResourceOperationalState::unknown;
    ResourceTenancy tenancy = ResourceTenancy::unknown;
    ResourceConcurrencyPolicy concurrency{};

    bool memory_capacity_known = false;
    std::uint64_t total_memory_bytes = 0;
    std::uint64_t available_memory_bytes = 0;
};

enum class AdmissionOutcome : std::uint8_t {
    accepted = 0,
    deferred,
    rejected,
    unsupported,
    unavailable,
    resource_exhausted,
    policy_conflict,
    unknown,
};

enum class AdmissionIssueCode : std::uint8_t {
    none = 0,
    invalid_workload_identity,
    invalid_resource_identity,
    invalid_constraint,
    backend_kind_mismatch,
    backend_identity_mismatch,
    resource_identity_mismatch,
    resource_excluded,
    execution_profile_mismatch,
    operation_adapter_unavailable,
    resource_state_unknown,
    resource_degraded,
    resource_unavailable,
    tenancy_unknown,
    tenancy_conflict,
    concurrency_support_unknown,
    concurrency_limit_conflict,
    concurrency_capacity_exhausted,
    preemption_support_unknown,
    preemption_unsupported,
    memory_capacity_unknown,
    memory_capacity_exhausted,
};

struct AdmissionIssue {
    AdmissionIssueCode code = AdmissionIssueCode::none;
    std::string message{};
};

struct ExecutionAdmissionResult {
    AdmissionOutcome outcome = AdmissionOutcome::unknown;
    WorkloadIdentity identity{};
    std::string resource_id{};
    ResourceTenancy effective_tenancy = ResourceTenancy::unknown;
    std::uint64_t required_reservation_bytes = 0;
    std::vector<AdmissionIssue> issues{};

    [[nodiscard]] bool accepted() const noexcept {
        return outcome == AdmissionOutcome::accepted;
    }
};

// Evaluates one immutable request against one immutable resource snapshot. It
// performs no discovery, allocation, device selection, stream creation, or
// work launch. A future scheduler may use this result as one admission input.
[[nodiscard]] ExecutionAdmissionResult evaluate_execution_policy(
    const ExecutionPolicyRequest& request,
    const ExecutionPolicyResource& resource);

[[nodiscard]] const char* to_string(ResourceTenancy value) noexcept;
[[nodiscard]] const char* to_string(PolicySupport value) noexcept;
[[nodiscard]] const char* to_string(WorkloadCooperation value) noexcept;
[[nodiscard]] const char* to_string(LatencySensitivity value) noexcept;
[[nodiscard]] const char* to_string(ExecutionPreference value) noexcept;
[[nodiscard]] const char* to_string(PreemptionAssumption value) noexcept;
[[nodiscard]] const char* to_string(ResourceOperationalState value) noexcept;
[[nodiscard]] const char* to_string(AdmissionOutcome value) noexcept;
[[nodiscard]] const char* to_string(AdmissionIssueCode value) noexcept;

}  // namespace prometheus::core

#endif  // SRC_CORE_EXECUTION_POLICY_HPP
