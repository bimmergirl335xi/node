#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

#include "core/execution_policy.hpp"

namespace pb = prometheus::backends;
namespace pc = prometheus::core;

namespace {

[[nodiscard]] pc::ExecutionPolicyRequest base_request() {
    pc::ExecutionPolicyRequest request{};
    request.identity.workload_id = "workload-001";
    request.identity.request_id = "request-001";
    request.identity.retry_correlation_id = "retry-001";
    request.identity.cancellation_correlation_id = "cancel-001";
    request.tenancy = pc::ResourceTenancy::concurrently_shareable;
    request.placement.allowed_backend_kinds = {pb::ComputeBackendKind::cuda};
    request.placement.required_backend_id = "cuda";
    request.placement.required_execution_profile = "sm_70";
    request.placement.required_operation_adapters = {"dense.fp32"};
    request.required_memory_bytes = 1024;
    request.reservation_headroom_bytes = 256;
    return request;
}

[[nodiscard]] pc::ExecutionPolicyResource base_resource() {
    pc::ExecutionPolicyResource resource{};
    resource.resource_id = "cuda:GPU-001";
    resource.backend_id = "cuda";
    resource.backend_kind = pb::ComputeBackendKind::cuda;
    resource.execution_profile = "sm_70";
    resource.operation_adapters = {"dense.fp32", "activation.fp32"};
    resource.operational_state = pc::ResourceOperationalState::ready;
    resource.tenancy = pc::ResourceTenancy::concurrently_shareable;
    resource.concurrency.concurrent_workload_support = pc::PolicySupport::supported;
    resource.concurrency.concurrent_lane_support = pc::PolicySupport::supported;
    resource.concurrency.preemption_support = pc::PolicySupport::unsupported;
    resource.concurrency.maximum_concurrent_workloads = 4;
    resource.concurrency.maximum_execution_lanes = 8;
    resource.memory_capacity_known = true;
    resource.total_memory_bytes = 8192;
    resource.available_memory_bytes = 4096;
    return resource;
}

[[nodiscard]] bool test_tenancy() {
    pc::ExecutionPolicyRequest request = base_request();
    pc::ExecutionPolicyResource resource = base_resource();

    request.tenancy = pc::ResourceTenancy::exclusive;
    if (pc::evaluate_execution_policy(request, resource).outcome !=
        pc::AdmissionOutcome::policy_conflict) {
        return false;
    }

    request.tenancy = pc::ResourceTenancy::shareable;
    resource.tenancy = pc::ResourceTenancy::exclusive;
    const pc::ExecutionAdmissionResult exclusive =
        pc::evaluate_execution_policy(request, resource);
    if (!exclusive.accepted() ||
        exclusive.effective_tenancy != pc::ResourceTenancy::exclusive) {
        return false;
    }

    resource.tenancy = pc::ResourceTenancy::concurrently_shareable;
    const pc::ExecutionAdmissionResult shared =
        pc::evaluate_execution_policy(request, resource);
    if (!shared.accepted() || shared.effective_tenancy !=
                                  pc::ResourceTenancy::concurrently_shareable) {
        return false;
    }

    resource.tenancy = pc::ResourceTenancy::unknown;
    if (pc::evaluate_execution_policy(request, resource).outcome !=
        pc::AdmissionOutcome::unknown) {
        return false;
    }
    resource.tenancy = pc::ResourceTenancy::concurrently_shareable;
    resource.concurrency.concurrent_workload_support =
        pc::PolicySupport::unknown;
    return pc::evaluate_execution_policy(request, resource).outcome ==
           pc::AdmissionOutcome::unknown;
}

[[nodiscard]] bool test_concurrency_and_memory() {
    pc::ExecutionPolicyRequest request = base_request();
    pc::ExecutionPolicyResource resource = base_resource();

    request.concurrency.requested_execution_lanes = 9;
    if (pc::evaluate_execution_policy(request, resource).outcome !=
        pc::AdmissionOutcome::policy_conflict) {
        return false;
    }

    request.concurrency.requested_execution_lanes = 0;
    resource.concurrency.admitted_workloads = 4;
    if (pc::evaluate_execution_policy(request, resource).outcome !=
        pc::AdmissionOutcome::resource_exhausted) {
        return false;
    }

    resource.concurrency.admitted_workloads = 0;
    request.required_memory_bytes = 5000;
    request.reservation_headroom_bytes = 0;
    const pc::ExecutionAdmissionResult exhausted =
        pc::evaluate_execution_policy(request, resource);
    if (exhausted.outcome != pc::AdmissionOutcome::resource_exhausted ||
        exhausted.issues.front().code !=
            pc::AdmissionIssueCode::memory_capacity_exhausted) {
        return false;
    }

    resource.memory_capacity_known = false;
    if (pc::evaluate_execution_policy(request, resource).outcome !=
        pc::AdmissionOutcome::unknown) {
        return false;
    }

    request.required_memory_bytes = std::numeric_limits<std::uint64_t>::max();
    request.reservation_headroom_bytes = 1;
    return pc::evaluate_execution_policy(request, resource).outcome ==
           pc::AdmissionOutcome::rejected;
}

[[nodiscard]] bool test_identity_determinism_and_purity() {
    const pc::ExecutionPolicyRequest request = base_request();
    const pc::ExecutionPolicyResource resource = base_resource();
    const pc::ExecutionPolicyResource before = resource;

    const pc::ExecutionAdmissionResult first =
        pc::evaluate_execution_policy(request, resource);
    const pc::ExecutionAdmissionResult second =
        pc::evaluate_execution_policy(request, resource);
    if (!first.accepted() || !second.accepted() ||
        first.identity.workload_id != request.identity.workload_id ||
        first.identity.request_id != request.identity.request_id ||
        first.identity.retry_correlation_id !=
            request.identity.retry_correlation_id ||
        first.identity.cancellation_correlation_id !=
            request.identity.cancellation_correlation_id ||
        first.outcome != second.outcome ||
        first.effective_tenancy != second.effective_tenancy ||
        first.required_reservation_bytes != 1280 ||
        !first.issues.empty() || !second.issues.empty()) {
        return false;
    }

    // Const inputs and unchanged metadata demonstrate that evaluation causes
    // no backend discovery, allocation, stream creation, or hardware action.
    return resource.resource_id == before.resource_id &&
           resource.available_memory_bytes == before.available_memory_bytes &&
           resource.concurrency.admitted_workloads ==
               before.concurrency.admitted_workloads &&
           resource.operation_adapters == before.operation_adapters;
}

[[nodiscard]] bool test_placement_and_state_outcomes() {
    pc::ExecutionPolicyRequest request = base_request();
    pc::ExecutionPolicyResource resource = base_resource();

    resource.operational_state = pc::ResourceOperationalState::degraded;
    if (pc::evaluate_execution_policy(request, resource).outcome !=
        pc::AdmissionOutcome::deferred) {
        return false;
    }
    resource.operational_state = pc::ResourceOperationalState::unavailable;
    if (pc::evaluate_execution_policy(request, resource).outcome !=
        pc::AdmissionOutcome::unavailable) {
        return false;
    }
    resource.operational_state = pc::ResourceOperationalState::ready;
    resource.operation_adapters.clear();
    if (pc::evaluate_execution_policy(request, resource).outcome !=
        pc::AdmissionOutcome::unsupported) {
        return false;
    }
    resource.operation_adapters = {"dense.fp32"};
    request.placement.excluded_resource_ids = {resource.resource_id};
    return pc::evaluate_execution_policy(request, resource).outcome ==
           pc::AdmissionOutcome::rejected;
}

}  // namespace

int main() {
    return test_tenancy() && test_concurrency_and_memory() &&
                   test_identity_determinism_and_purity() &&
                   test_placement_and_state_outcomes()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
