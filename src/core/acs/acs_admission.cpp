#include "acs_admission.hpp"

#include <algorithm>
#include <limits>

namespace prometheus::core::acs {
namespace {

template <typename T, typename Id>
const T* locate(const std::vector<T>& values, const Id& id) {
    const auto found = std::find_if(values.begin(), values.end(), [&](const auto& value) { return value.id == id; });
    return found == values.end() ? nullptr : &*found;
}

int precedence(AcsAdmissionOutcome value) {
    switch (value) {
        case AcsAdmissionOutcome::rejected: return 0;
        case AcsAdmissionOutcome::denied: return 1;
        case AcsAdmissionOutcome::conflicting: return 2;
        case AcsAdmissionOutcome::unavailable: return 3;
        case AcsAdmissionOutcome::unknown: return 4;
        case AcsAdmissionOutcome::deferred: return 5;
        case AcsAdmissionOutcome::admitted_with_restrictions: return 6;
        case AcsAdmissionOutcome::admitted: return 7;
    }
    return 4;
}

void promote(AcsAdmissionResult& result, AcsAdmissionOutcome candidate) {
    if (precedence(candidate) < precedence(result.outcome)) result.outcome = candidate;
}

void issue(AcsAdmissionResult& result, const AcsAdmissionRequest& request,
           AcsAdmissionOutcome outcome, AcsAdmissionIssueCode code,
           std::string subject, std::string message) {
    promote(result, outcome);
    const std::size_t limit = std::min<std::size_t>(request.maximum_issues, 64);
    if (result.issues.size() < limit) result.issues.push_back({code, std::move(subject), std::move(message)});
    else result.issues_truncated = true;
}

bool listed(const std::vector<EvidenceId>& values, const EvidenceId& id) {
    return std::find(values.begin(), values.end(), id) != values.end();
}
bool listed(const std::vector<CapabilityId>& values, const CapabilityId& id) {
    return std::find(values.begin(), values.end(), id) != values.end();
}
}  // namespace

AcsAdmissionResult evaluate_admission_impl(
    const AcsRegistrySnapshot& registry, const AcsAdmissionRequest& request) {
    AcsAdmissionResult result{};
    result.outcome = AcsAdmissionOutcome::admitted;
    result.request_id = request.request_id;
    result.connection_id = request.connection_id;
    result.evaluated_generation = registry.generation;

    if (!request.request_id.valid() || !request.connection_id.valid() ||
        !request.participant.valid() || !request.authority.valid() ||
        request.maximum_issues == 0 || request.maximum_issues > 64 ||
        request.evidence.size() > kAbsoluteEvidenceReferences ||
        request.capabilities.size() > kAbsoluteEvidenceReferences ||
        request.delegations.size() > kAbsoluteEvidenceReferences ||
        request.restrictions.size() > kAbsoluteEvidenceReferences ||
        request.budgets.size() > kAbsoluteBudgetDimensions) {
        issue(result, request, AcsAdmissionOutcome::rejected,
              AcsAdmissionIssueCode::malformed_request, request.request_id.canonical(),
              "request structure exceeds public bounds");
        return result;
    }

    const auto* connection = locate(registry.connections, request.connection_id);
    const auto* participant = locate(registry.participants, request.participant);
    const auto* authority = locate(registry.authorities, request.authority);
    if (!connection || !participant || !authority) {
        issue(result, request, AcsAdmissionOutcome::rejected,
              AcsAdmissionIssueCode::broken_reference, request.connection_id.canonical(),
              "required descriptor is absent");
        return result;
    }
    if (registry.generation != request.expected_registry_generation)
        issue(result, request, AcsAdmissionOutcome::conflicting,
              AcsAdmissionIssueCode::generation_conflict, request.connection_id.canonical(),
              "registry generation differs from request");
    if (connection->revision != request.expected_connection_revision)
        issue(result, request, AcsAdmissionOutcome::conflicting,
              AcsAdmissionIssueCode::descriptor_revision_conflict, request.connection_id.canonical(),
              "connection descriptor revision differs from request");

    switch (request.identity_condition) {
        case IdentityCondition::valid: break;
        case IdentityCondition::stale:
            issue(result, request, AcsAdmissionOutcome::deferred, AcsAdmissionIssueCode::identity_stale, request.participant.canonical(), "participant identity is stale"); break;
        case IdentityCondition::conflicting:
            issue(result, request, AcsAdmissionOutcome::conflicting, AcsAdmissionIssueCode::identity_conflicting, request.participant.canonical(), "participant identity conflicts"); break;
        case IdentityCondition::unavailable:
            issue(result, request, AcsAdmissionOutcome::unavailable, AcsAdmissionIssueCode::identity_unavailable, request.participant.canonical(), "participant identity evaluator unavailable"); break;
        case IdentityCondition::unknown:
            issue(result, request, AcsAdmissionOutcome::unknown, AcsAdmissionIssueCode::identity_unknown, request.participant.canonical(), "participant identity unknown"); break;
    }

    for (const auto& required : connection->required_evidence) {
        const auto* value = locate(registry.evidence, required);
        if (!value || !listed(request.evidence, required)) {
            issue(result, request, AcsAdmissionOutcome::rejected, AcsAdmissionIssueCode::broken_reference, required.canonical(), "required evidence absent");
            continue;
        }
        switch (value->freshness) {
            case FreshnessCondition::current: break;
            case FreshnessCondition::stale: case FreshnessCondition::expired:
                issue(result, request, AcsAdmissionOutcome::deferred, AcsAdmissionIssueCode::evidence_stale, required.canonical(), "required evidence is stale"); break;
            case FreshnessCondition::conflicting:
                issue(result, request, AcsAdmissionOutcome::conflicting, AcsAdmissionIssueCode::evidence_conflicting, required.canonical(), "required evidence conflicts"); break;
            case FreshnessCondition::unavailable:
                issue(result, request, AcsAdmissionOutcome::unavailable, AcsAdmissionIssueCode::evidence_unavailable, required.canonical(), "required evidence unavailable"); break;
            case FreshnessCondition::unknown:
                issue(result, request, AcsAdmissionOutcome::unknown, AcsAdmissionIssueCode::evidence_unknown, required.canonical(), "required evidence freshness unknown"); break;
        }
    }

    switch (authority->condition) {
        case AuthorityCondition::valid: break;
        case AuthorityCondition::revoked: case AuthorityCondition::expired: case AuthorityCondition::suspended:
            issue(result, request, AcsAdmissionOutcome::denied, AcsAdmissionIssueCode::authority_insufficient, request.authority.canonical(), "authority is not valid"); break;
        case AuthorityCondition::stale:
            issue(result, request, AcsAdmissionOutcome::deferred, AcsAdmissionIssueCode::authority_stale, request.authority.canonical(), "authority is stale"); break;
        case AuthorityCondition::conflicting:
            issue(result, request, AcsAdmissionOutcome::conflicting, AcsAdmissionIssueCode::authority_conflicting, request.authority.canonical(), "authority conflicts"); break;
        case AuthorityCondition::unavailable:
            issue(result, request, AcsAdmissionOutcome::unavailable, AcsAdmissionIssueCode::authority_unavailable, request.authority.canonical(), "authority evaluator unavailable"); break;
        case AuthorityCondition::unknown:
            issue(result, request, AcsAdmissionOutcome::unknown, AcsAdmissionIssueCode::authority_insufficient, request.authority.canonical(), "authority unknown"); break;
    }

    for (const auto& required : connection->required_capabilities) {
        const auto* value = locate(registry.capabilities, required);
        if (!value || !listed(request.capabilities, required) || value->subject != request.participant || value->condition != AuthorityCondition::valid)
            issue(result, request, AcsAdmissionOutcome::denied, AcsAdmissionIssueCode::capability_insufficient, required.canonical(), "required capability is not current for participant");
    }
    for (const auto& delegation : request.delegations) {
        const auto* value = locate(registry.delegations, delegation);
        if (!value || value->delegate != request.participant || value->condition != AuthorityCondition::valid)
            issue(result, request, AcsAdmissionOutcome::denied, AcsAdmissionIssueCode::delegation_insufficient, delegation.canonical(), "delegation is not valid for participant");
    }
    for (const auto& revocation : registry.revocations) {
        if (revocation.subject == request.participant && revocation.condition == AuthorityCondition::revoked)
            issue(result, request, AcsAdmissionOutcome::denied, AcsAdmissionIssueCode::explicitly_revoked, revocation.id.canonical(), "explicit revocation applies");
    }

    const auto* relationship = locate(registry.relationships, connection->relationship);
    const auto* source_port = locate(registry.ports, connection->source_port);
    const auto* target_port = locate(registry.ports, connection->target_port);
    if (!relationship || !relationship->enabled)
        issue(result, request, AcsAdmissionOutcome::rejected, AcsAdmissionIssueCode::relationship_incompatible, connection->relationship.canonical(), "relationship unavailable or disabled");
    if (!source_port || !target_port || !source_port->enabled || !target_port->enabled)
        issue(result, request, AcsAdmissionOutcome::rejected, AcsAdmissionIssueCode::port_incompatible, request.connection_id.canonical(), "port contract unavailable or disabled");

    if (request.state.connection_id != request.connection_id ||
        (request.state.lifecycle != ConnectionLifecycle::proposed && request.state.lifecycle != ConnectionLifecycle::admission_pending))
        issue(result, request, AcsAdmissionOutcome::rejected, AcsAdmissionIssueCode::lifecycle_incompatible, request.connection_id.canonical(), "lifecycle is not admission eligible");
    switch (request.state.operational) {
        case OperationalCondition::nominal: break;
        case OperationalCondition::degraded:
            issue(result, request, AcsAdmissionOutcome::admitted_with_restrictions, AcsAdmissionIssueCode::operational_degraded, request.connection_id.canonical(), "connection context is degraded"); break;
        case OperationalCondition::unavailable: case OperationalCondition::failed:
            issue(result, request, AcsAdmissionOutcome::unavailable, AcsAdmissionIssueCode::operational_unavailable, request.connection_id.canonical(), "connection context unavailable"); break;
        case OperationalCondition::unknown:
            issue(result, request, AcsAdmissionOutcome::unknown, AcsAdmissionIssueCode::operational_unknown, request.connection_id.canonical(), "operational condition unknown"); break;
    }
    switch (request.state.enforcement) {
        case EnforcementCondition::unrestricted: break;
        case EnforcementCondition::restricted:
            issue(result, request, AcsAdmissionOutcome::admitted_with_restrictions, AcsAdmissionIssueCode::enforcement_restricted, request.connection_id.canonical(), "connection is restricted"); break;
        case EnforcementCondition::quarantined:
            issue(result, request, AcsAdmissionOutcome::denied, AcsAdmissionIssueCode::enforcement_quarantined, request.connection_id.canonical(), "connection is quarantined"); break;
        case EnforcementCondition::revoked:
            issue(result, request, AcsAdmissionOutcome::denied, AcsAdmissionIssueCode::enforcement_revoked, request.connection_id.canonical(), "connection enforcement is revoked"); break;
        case EnforcementCondition::unknown:
            issue(result, request, AcsAdmissionOutcome::unknown, AcsAdmissionIssueCode::enforcement_restricted, request.connection_id.canonical(), "enforcement condition unknown"); break;
    }
    for (const auto& restriction : request.restrictions) {
        const auto* value = locate(registry.restrictions, restriction);
        if (!value) issue(result, request, AcsAdmissionOutcome::rejected, AcsAdmissionIssueCode::broken_reference, restriction.canonical(), "restriction reference absent");
        else if (value->condition == PublicReferenceCondition::restricted || value->condition == PublicReferenceCondition::contained)
            issue(result, request, AcsAdmissionOutcome::admitted_with_restrictions, AcsAdmissionIssueCode::immune_restriction, restriction.canonical(), "public restriction applies");
    }

    using ExecutionOutcome = prometheus::core::AdmissionOutcome;
    switch (request.execution_policy_outcome) {
        case ExecutionOutcome::accepted: break;
        case ExecutionOutcome::rejected: case ExecutionOutcome::unsupported: case ExecutionOutcome::policy_conflict:
            issue(result, request, AcsAdmissionOutcome::rejected, AcsAdmissionIssueCode::execution_policy_conflict, request.connection_id.canonical(), "execution policy incompatible"); break;
        case ExecutionOutcome::unavailable:
            issue(result, request, AcsAdmissionOutcome::unavailable, AcsAdmissionIssueCode::execution_policy_unavailable, request.connection_id.canonical(), "execution policy evidence unavailable"); break;
        case ExecutionOutcome::deferred: case ExecutionOutcome::resource_exhausted:
            issue(result, request, AcsAdmissionOutcome::deferred, AcsAdmissionIssueCode::execution_policy_conflict, request.connection_id.canonical(), "execution policy deferred"); break;
        case ExecutionOutcome::unknown:
            issue(result, request, AcsAdmissionOutcome::unknown, AcsAdmissionIssueCode::execution_policy_unknown, request.connection_id.canonical(), "execution policy evidence unknown"); break;
    }

    for (const auto& budget : request.budgets) {
        if (!budget.dimension.valid() || budget.requested_unit == BudgetUnit::unknown || budget.available_unit == BudgetUnit::unknown) {
            issue(result, request, AcsAdmissionOutcome::rejected, AcsAdmissionIssueCode::malformed_request, budget.dimension.value, "invalid budget evidence"); continue;
        }
        if (budget.requested_unit != budget.available_unit) {
            issue(result, request, AcsAdmissionOutcome::rejected, AcsAdmissionIssueCode::budget_unit_mismatch, budget.dimension.value, "budget units are incompatible"); continue;
        }
        if (!budget.cost_known) {
            issue(result, request, AcsAdmissionOutcome::unknown, AcsAdmissionIssueCode::budget_unknown_cost, budget.dimension.value, "budget cost unknown"); continue;
        }
        std::uint64_t required = 0;
        if (!checked_add(budget.cost, budget.headroom, required)) {
            issue(result, request, AcsAdmissionOutcome::rejected, AcsAdmissionIssueCode::budget_overflow, budget.dimension.value, "budget addition overflow"); continue;
        }
        if (!budget.availability_known) issue(result, request, AcsAdmissionOutcome::unknown, AcsAdmissionIssueCode::budget_unknown_availability, budget.dimension.value, "budget availability unknown");
        else if (required > budget.available) issue(result, request, AcsAdmissionOutcome::deferred, AcsAdmissionIssueCode::budget_insufficient, budget.dimension.value, "budget insufficient");
    }
    return result;
}

AcsAdmissionResult evaluate_admission(
    const AcsRegistrySnapshot& registry, const AcsAdmissionRequest& request) noexcept {
    try {
        return evaluate_admission_impl(registry, request);
    } catch (...) {
        AcsAdmissionResult result{};
        result.outcome = AcsAdmissionOutcome::unavailable;
        result.issues_truncated = true;
        return result;
    }
}
}  // namespace prometheus::core::acs
