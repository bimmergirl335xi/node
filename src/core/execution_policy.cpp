#include "execution_policy.hpp"

#include <algorithm>
#include <limits>
#include <string_view>
#include <utility>

namespace prometheus::core {
namespace {

[[nodiscard]] bool valid_identity_component(
    std::string_view value,
    bool required) noexcept {
    return (!required || !value.empty()) &&
           value.size() <= kMaximumExecutionIdentityLength;
}

[[nodiscard]] bool valid_identity(const WorkloadIdentity& identity) noexcept {
    return valid_identity_component(identity.workload_id, true) &&
           valid_identity_component(identity.request_id, true) &&
           valid_identity_component(identity.retry_correlation_id, false) &&
           valid_identity_component(identity.cancellation_correlation_id,
                                    false);
}

[[nodiscard]] bool bounded_strings(
    const std::vector<std::string>& values) noexcept {
    if (values.size() > kMaximumPlacementListEntries) {
        return false;
    }
    return std::all_of(values.begin(), values.end(), [](const std::string& value) {
        return !value.empty() &&
               value.size() <= kMaximumExecutionIdentityLength;
    });
}

template <typename Value>
[[nodiscard]] bool contains(const std::vector<Value>& values,
                            const Value& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

void add_issue(ExecutionAdmissionResult& result,
               AdmissionIssueCode code,
               std::string message) {
    result.issues.push_back({code, std::move(message)});
}

[[nodiscard]] ResourceTenancy effective_tenancy(
    ResourceTenancy requested,
    ResourceTenancy available) noexcept {
    if (requested == ResourceTenancy::unknown ||
        available == ResourceTenancy::unknown) {
        return ResourceTenancy::unknown;
    }
    if (requested == ResourceTenancy::exclusive) {
        return available == ResourceTenancy::exclusive
                   ? ResourceTenancy::exclusive
                   : ResourceTenancy::unknown;
    }
    if (requested == ResourceTenancy::partitioned) {
        return available == ResourceTenancy::partitioned
                   ? ResourceTenancy::partitioned
                   : ResourceTenancy::unknown;
    }
    return available;
}

[[nodiscard]] bool tenancy_conflicts(ResourceTenancy requested,
                                     ResourceTenancy available) noexcept {
    if (requested == ResourceTenancy::exclusive) {
        return available != ResourceTenancy::unknown &&
               available != ResourceTenancy::exclusive;
    }
    if (requested == ResourceTenancy::partitioned) {
        return available != ResourceTenancy::unknown &&
               available != ResourceTenancy::partitioned;
    }
    return false;
}

}  // namespace

ExecutionAdmissionResult evaluate_execution_policy(
    const ExecutionPolicyRequest& request,
    const ExecutionPolicyResource& resource) {
    ExecutionAdmissionResult result{};
    result.identity = request.identity;
    result.resource_id = resource.resource_id;

    if (!valid_identity(request.identity)) {
        result.outcome = AdmissionOutcome::rejected;
        add_issue(result,
                  AdmissionIssueCode::invalid_workload_identity,
                  "Workload and request identities are required and bounded");
        return result;
    }
    if (resource.resource_id.empty() ||
        resource.resource_id.size() > kMaximumExecutionIdentityLength ||
        resource.backend_id.size() > kMaximumExecutionIdentityLength ||
        resource.execution_profile.size() > kMaximumExecutionProfileLength ||
        !bounded_strings(resource.operation_adapters) ||
        (resource.memory_capacity_known &&
         resource.available_memory_bytes > resource.total_memory_bytes)) {
        result.outcome = AdmissionOutcome::rejected;
        add_issue(result,
                  AdmissionIssueCode::invalid_resource_identity,
                  "Resource identity and capacity metadata must be bounded and consistent");
        return result;
    }
    const PlacementConstraints& placement = request.placement;
    if (placement.allowed_backend_kinds.size() > kMaximumPlacementListEntries ||
        !bounded_strings(placement.allowed_resource_ids) ||
        !bounded_strings(placement.excluded_resource_ids) ||
        !bounded_strings(placement.required_operation_adapters) ||
        placement.required_backend_id.size() >
            kMaximumExecutionIdentityLength ||
        placement.required_execution_profile.size() >
            kMaximumExecutionProfileLength ||
        request.required_memory_bytes >
            std::numeric_limits<std::uint64_t>::max() -
                request.reservation_headroom_bytes) {
        result.outcome = AdmissionOutcome::rejected;
        add_issue(result,
                  AdmissionIssueCode::invalid_constraint,
                  "Execution constraints exceed their declared bounds");
        return result;
    }
    result.required_reservation_bytes = request.required_memory_bytes +
                                        request.reservation_headroom_bytes;

    if (!placement.allowed_backend_kinds.empty()) {
        if (resource.backend_kind == backends::ComputeBackendKind::unknown) {
            add_issue(result,
                      AdmissionIssueCode::backend_kind_mismatch,
                      "Resource backend kind is unknown");
            result.outcome = AdmissionOutcome::unknown;
            return result;
        }
        if (!contains(placement.allowed_backend_kinds, resource.backend_kind)) {
            add_issue(result,
                      AdmissionIssueCode::backend_kind_mismatch,
                      "Resource backend kind is not permitted");
            result.outcome = AdmissionOutcome::unsupported;
            return result;
        }
    }
    if (!placement.required_backend_id.empty() &&
        placement.required_backend_id != resource.backend_id) {
        add_issue(result,
                  AdmissionIssueCode::backend_identity_mismatch,
                  "Resource backend identity does not match the requirement");
        result.outcome = AdmissionOutcome::unsupported;
        return result;
    }
    if (!placement.allowed_resource_ids.empty() &&
        !contains(placement.allowed_resource_ids, resource.resource_id)) {
        add_issue(result,
                  AdmissionIssueCode::resource_identity_mismatch,
                  "Resource identity is not in the allowed set");
        result.outcome = AdmissionOutcome::unsupported;
        return result;
    }
    if (contains(placement.excluded_resource_ids, resource.resource_id)) {
        add_issue(result,
                  AdmissionIssueCode::resource_excluded,
                  "Resource identity is explicitly excluded");
        result.outcome = AdmissionOutcome::rejected;
        return result;
    }
    if (!placement.required_execution_profile.empty() &&
        placement.required_execution_profile != resource.execution_profile) {
        add_issue(result,
                  AdmissionIssueCode::execution_profile_mismatch,
                  "Resource execution profile does not match the requirement");
        result.outcome = AdmissionOutcome::unsupported;
        return result;
    }
    for (const std::string& adapter : placement.required_operation_adapters) {
        if (!contains(resource.operation_adapters, adapter)) {
            add_issue(result,
                      AdmissionIssueCode::operation_adapter_unavailable,
                      "Required typed operation adapter is unavailable: '" +
                          adapter + "'");
            result.outcome = AdmissionOutcome::unsupported;
            return result;
        }
    }

    if (placement.require_ready) {
        switch (resource.operational_state) {
            case ResourceOperationalState::unknown:
                add_issue(result,
                          AdmissionIssueCode::resource_state_unknown,
                          "Resource readiness is unknown");
                result.outcome = AdmissionOutcome::unknown;
                return result;
            case ResourceOperationalState::degraded:
                add_issue(result,
                          AdmissionIssueCode::resource_degraded,
                          "Resource is degraded and requires later admission");
                result.outcome = AdmissionOutcome::deferred;
                return result;
            case ResourceOperationalState::unavailable:
            case ResourceOperationalState::failed:
                add_issue(result,
                          AdmissionIssueCode::resource_unavailable,
                          "Resource is unavailable for admission");
                result.outcome = AdmissionOutcome::unavailable;
                return result;
            case ResourceOperationalState::ready:
                break;
        }
    }

    result.effective_tenancy =
        effective_tenancy(request.tenancy, resource.tenancy);
    if (request.tenancy == ResourceTenancy::unknown ||
        resource.tenancy == ResourceTenancy::unknown) {
        add_issue(result,
                  AdmissionIssueCode::tenancy_unknown,
                  "Workload or resource tenancy policy is unknown");
        result.outcome = AdmissionOutcome::unknown;
        return result;
    }
    if (tenancy_conflicts(request.tenancy, resource.tenancy)) {
        add_issue(result,
                  AdmissionIssueCode::tenancy_conflict,
                  "Workload tenancy requirement conflicts with resource policy");
        result.outcome = AdmissionOutcome::policy_conflict;
        return result;
    }

    const ResourceConcurrencyPolicy& concurrency = resource.concurrency;
    const bool sharing_requested =
        request.tenancy == ResourceTenancy::shareable ||
        request.tenancy == ResourceTenancy::concurrently_shareable;
    if (sharing_requested && resource.tenancy != ResourceTenancy::exclusive) {
        if (concurrency.concurrent_workload_support == PolicySupport::unknown) {
            add_issue(result,
                      AdmissionIssueCode::concurrency_support_unknown,
                      "Concurrent workload support is unknown");
            result.outcome = AdmissionOutcome::unknown;
            return result;
        }
        if (concurrency.concurrent_workload_support ==
            PolicySupport::unsupported) {
            add_issue(result,
                      AdmissionIssueCode::tenancy_conflict,
                      "Resource does not support concurrent workloads");
            result.outcome = AdmissionOutcome::policy_conflict;
            return result;
        }
        if (concurrency.maximum_concurrent_workloads == 0) {
            add_issue(result,
                      AdmissionIssueCode::concurrency_support_unknown,
                      "Concurrent workload limit is unknown");
            result.outcome = AdmissionOutcome::unknown;
            return result;
        }
    }
    if (request.concurrency.maximum_co_resident_workloads != 0 &&
        concurrency.admitted_workloads >=
            request.concurrency.maximum_co_resident_workloads) {
        add_issue(result,
                  AdmissionIssueCode::concurrency_limit_conflict,
                  "Existing co-resident workload count exceeds workload policy");
        result.outcome = AdmissionOutcome::policy_conflict;
        return result;
    }
    if (concurrency.maximum_concurrent_workloads != 0 &&
        concurrency.admitted_workloads >=
            concurrency.maximum_concurrent_workloads) {
        add_issue(result,
                  AdmissionIssueCode::concurrency_capacity_exhausted,
                  "Resource concurrent-workload capacity is exhausted");
        result.outcome = AdmissionOutcome::resource_exhausted;
        return result;
    }
    if (request.concurrency.requested_execution_lanes != 0) {
        if (concurrency.concurrent_lane_support == PolicySupport::unknown ||
            concurrency.maximum_execution_lanes == 0) {
            add_issue(result,
                      AdmissionIssueCode::concurrency_support_unknown,
                      "Execution-lane support or limit is unknown");
            result.outcome = AdmissionOutcome::unknown;
            return result;
        }
        if (concurrency.concurrent_lane_support == PolicySupport::unsupported ||
            request.concurrency.requested_execution_lanes >
                concurrency.maximum_execution_lanes) {
            add_issue(result,
                      AdmissionIssueCode::concurrency_limit_conflict,
                      "Requested execution lanes conflict with resource policy");
            result.outcome = AdmissionOutcome::policy_conflict;
            return result;
        }
    }
    if (request.concurrency.preemption == PreemptionAssumption::required) {
        if (concurrency.preemption_support == PolicySupport::unknown) {
            add_issue(result,
                      AdmissionIssueCode::preemption_support_unknown,
                      "Required preemption support is unknown");
            result.outcome = AdmissionOutcome::unknown;
            return result;
        }
        if (concurrency.preemption_support == PolicySupport::unsupported) {
            add_issue(result,
                      AdmissionIssueCode::preemption_unsupported,
                      "Required preemption is unsupported");
            result.outcome = AdmissionOutcome::unsupported;
            return result;
        }
    }

    if (result.required_reservation_bytes != 0) {
        if (!resource.memory_capacity_known) {
            add_issue(result,
                      AdmissionIssueCode::memory_capacity_unknown,
                      "Resource memory capacity is unknown");
            result.outcome = AdmissionOutcome::unknown;
            return result;
        }
        if (result.required_reservation_bytes >
            resource.available_memory_bytes) {
            add_issue(result,
                      AdmissionIssueCode::memory_capacity_exhausted,
                      "Required memory reservation exceeds available capacity");
            result.outcome = AdmissionOutcome::resource_exhausted;
            return result;
        }
    }

    result.outcome = AdmissionOutcome::accepted;
    return result;
}

const char* to_string(ResourceTenancy value) noexcept {
    switch (value) {
        case ResourceTenancy::unknown: return "unknown";
        case ResourceTenancy::exclusive: return "exclusive";
        case ResourceTenancy::shareable: return "shareable";
        case ResourceTenancy::concurrently_shareable: return "concurrently_shareable";
        case ResourceTenancy::partitioned: return "partitioned";
    }
    return "unknown";
}

const char* to_string(PolicySupport value) noexcept {
    switch (value) {
        case PolicySupport::unknown: return "unknown";
        case PolicySupport::unsupported: return "unsupported";
        case PolicySupport::supported: return "supported";
    }
    return "unknown";
}

const char* to_string(WorkloadCooperation value) noexcept {
    switch (value) {
        case WorkloadCooperation::unknown: return "unknown";
        case WorkloadCooperation::cooperative: return "cooperative";
        case WorkloadCooperation::non_cooperative: return "non_cooperative";
    }
    return "unknown";
}

const char* to_string(LatencySensitivity value) noexcept {
    switch (value) {
        case LatencySensitivity::unknown: return "unknown";
        case LatencySensitivity::best_effort: return "best_effort";
        case LatencySensitivity::latency_sensitive: return "latency_sensitive";
    }
    return "unknown";
}

const char* to_string(ExecutionPreference value) noexcept {
    switch (value) {
        case ExecutionPreference::balanced: return "balanced";
        case ExecutionPreference::latency: return "latency";
        case ExecutionPreference::throughput: return "throughput";
    }
    return "unknown";
}

const char* to_string(PreemptionAssumption value) noexcept {
    switch (value) {
        case PreemptionAssumption::unknown: return "unknown";
        case PreemptionAssumption::not_required: return "not_required";
        case PreemptionAssumption::required: return "required";
    }
    return "unknown";
}

const char* to_string(ResourceOperationalState value) noexcept {
    switch (value) {
        case ResourceOperationalState::unknown: return "unknown";
        case ResourceOperationalState::ready: return "ready";
        case ResourceOperationalState::degraded: return "degraded";
        case ResourceOperationalState::unavailable: return "unavailable";
        case ResourceOperationalState::failed: return "failed";
    }
    return "unknown";
}

const char* to_string(AdmissionOutcome value) noexcept {
    switch (value) {
        case AdmissionOutcome::accepted: return "accepted";
        case AdmissionOutcome::deferred: return "deferred";
        case AdmissionOutcome::rejected: return "rejected";
        case AdmissionOutcome::unsupported: return "unsupported";
        case AdmissionOutcome::unavailable: return "unavailable";
        case AdmissionOutcome::resource_exhausted: return "resource_exhausted";
        case AdmissionOutcome::policy_conflict: return "policy_conflict";
        case AdmissionOutcome::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(AdmissionIssueCode value) noexcept {
    switch (value) {
        case AdmissionIssueCode::none: return "none";
        case AdmissionIssueCode::invalid_workload_identity: return "invalid_workload_identity";
        case AdmissionIssueCode::invalid_resource_identity: return "invalid_resource_identity";
        case AdmissionIssueCode::invalid_constraint: return "invalid_constraint";
        case AdmissionIssueCode::backend_kind_mismatch: return "backend_kind_mismatch";
        case AdmissionIssueCode::backend_identity_mismatch: return "backend_identity_mismatch";
        case AdmissionIssueCode::resource_identity_mismatch: return "resource_identity_mismatch";
        case AdmissionIssueCode::resource_excluded: return "resource_excluded";
        case AdmissionIssueCode::execution_profile_mismatch: return "execution_profile_mismatch";
        case AdmissionIssueCode::operation_adapter_unavailable: return "operation_adapter_unavailable";
        case AdmissionIssueCode::resource_state_unknown: return "resource_state_unknown";
        case AdmissionIssueCode::resource_degraded: return "resource_degraded";
        case AdmissionIssueCode::resource_unavailable: return "resource_unavailable";
        case AdmissionIssueCode::tenancy_unknown: return "tenancy_unknown";
        case AdmissionIssueCode::tenancy_conflict: return "tenancy_conflict";
        case AdmissionIssueCode::concurrency_support_unknown: return "concurrency_support_unknown";
        case AdmissionIssueCode::concurrency_limit_conflict: return "concurrency_limit_conflict";
        case AdmissionIssueCode::concurrency_capacity_exhausted: return "concurrency_capacity_exhausted";
        case AdmissionIssueCode::preemption_support_unknown: return "preemption_support_unknown";
        case AdmissionIssueCode::preemption_unsupported: return "preemption_unsupported";
        case AdmissionIssueCode::memory_capacity_unknown: return "memory_capacity_unknown";
        case AdmissionIssueCode::memory_capacity_exhausted: return "memory_capacity_exhausted";
    }
    return "unknown";
}

}  // namespace prometheus::core
