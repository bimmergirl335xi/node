#ifndef SRC_CORE_ACS_ACS_ADMISSION_HPP
#define SRC_CORE_ACS_ACS_ADMISSION_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "acs_lifecycle.hpp"
#include "core/execution_policy.hpp"

namespace prometheus::core::acs {

enum class AcsAdmissionOutcome : std::uint8_t {
    admitted = 0,
    admitted_with_restrictions,
    deferred,
    denied,
    rejected,
    unknown,
    conflicting,
    unavailable,
};

enum class AcsAdmissionIssueCode : std::uint8_t {
    none = 0,
    malformed_request,
    broken_reference,
    generation_conflict,
    descriptor_revision_conflict,
    identity_unknown,
    identity_stale,
    identity_conflicting,
    identity_unavailable,
    evidence_unknown,
    evidence_stale,
    evidence_conflicting,
    evidence_unavailable,
    authority_insufficient,
    authority_stale,
    authority_conflicting,
    authority_unavailable,
    capability_insufficient,
    delegation_insufficient,
    explicitly_revoked,
    relationship_incompatible,
    port_incompatible,
    lifecycle_incompatible,
    operational_unknown,
    operational_degraded,
    operational_unavailable,
    enforcement_restricted,
    enforcement_quarantined,
    enforcement_revoked,
    immune_restriction,
    execution_policy_conflict,
    execution_policy_unknown,
    execution_policy_unavailable,
    budget_unit_mismatch,
    budget_unknown_cost,
    budget_unknown_availability,
    budget_insufficient,
    budget_overflow,
};

struct AcsAdmissionIssue {
    AcsAdmissionIssueCode code = AcsAdmissionIssueCode::none;
    std::string subject{};
    std::string message{};
};

struct BudgetEvidence {
    ScopeReference dimension{};
    BudgetUnit requested_unit = BudgetUnit::unknown;
    BudgetUnit available_unit = BudgetUnit::unknown;
    bool cost_known = false;
    std::uint64_t cost = 0;
    std::uint64_t headroom = 0;
    bool availability_known = false;
    std::uint64_t available = 0;
};

struct AcsAdmissionRequest {
    AdmissionRequestId request_id{};
    ConnectionId connection_id{};
    ParticipantId participant{};
    AuthorityId authority{};
    RegistryGeneration expected_registry_generation{};
    DescriptorRevision expected_connection_revision{};
    IdentityCondition identity_condition = IdentityCondition::unknown;
    std::vector<EvidenceId> evidence{};
    std::vector<CapabilityId> capabilities{};
    std::vector<DelegationId> delegations{};
    std::vector<RestrictionId> restrictions{};
    ConnectionStateSnapshot state{};
    prometheus::core::AdmissionOutcome execution_policy_outcome =
        prometheus::core::AdmissionOutcome::unknown;
    std::vector<BudgetEvidence> budgets{};
    std::size_t maximum_issues = 32;
};

struct AcsAdmissionResult {
    AcsAdmissionOutcome outcome = AcsAdmissionOutcome::unknown;
    AdmissionRequestId request_id{};
    ConnectionId connection_id{};
    RegistryGeneration evaluated_generation{};
    std::vector<AcsAdmissionIssue> issues{};
    bool issues_truncated = false;
    [[nodiscard]] bool admitted() const noexcept {
        return outcome == AcsAdmissionOutcome::admitted ||
               outcome == AcsAdmissionOutcome::admitted_with_restrictions;
    }
};

// Pure metadata evaluation: no reservation, registry mutation, lifecycle
// mutation, provider call, authentication, discovery, or transport action.
[[nodiscard]] AcsAdmissionResult evaluate_admission(
    const AcsRegistrySnapshot& registry,
    const AcsAdmissionRequest& request);

}  // namespace prometheus::core::acs
#endif
