#include <cstdlib>
#include <limits>

#include "acs_test_fixture.hpp"
#include "core/acs/acs_admission.hpp"

namespace acs = prometheus::core::acs;

acs::AcsAdmissionRequest request_for(
    const acs::AcsRegistrySnapshot& snapshot,
    const acs_test::FixtureIds& ids) {
    acs::AcsAdmissionRequest request{};
    request.request_id = *acs::AdmissionRequestId::parse("admission", "one");
    request.connection_id = ids.connection; request.participant = ids.first;
    request.authority = ids.authority; request.expected_registry_generation = snapshot.generation;
    request.expected_connection_revision = acs::DescriptorRevision{1};
    request.identity_condition = acs::IdentityCondition::valid;
    request.evidence = {ids.evidence}; request.capabilities = {ids.capability};
    request.state.connection_id = ids.connection; request.state.lifecycle = acs::ConnectionLifecycle::admission_pending;
    request.state.operational = acs::OperationalCondition::nominal;
    request.state.enforcement = acs::EnforcementCondition::unrestricted;
    request.execution_policy_outcome = prometheus::core::AdmissionOutcome::accepted;
    request.budgets = {{{"messages"}, acs::BudgetUnit::messages, acs::BudgetUnit::messages, true, 4, 1, true, 10}};
    return request;
}

int main() {
    acs::AcsRegistry registry{}; acs_test::FixtureIds ids{};
    if (!acs_test::populate(registry, ids)) return EXIT_FAILURE;
    const auto snapshot = registry.snapshot(); const auto request = request_for(snapshot, ids);
    const auto snapshot_before = snapshot; const auto request_before = request;
    const auto admitted = acs::evaluate_admission(snapshot, request);
    if (!admitted.admitted() || admitted.outcome != acs::AcsAdmissionOutcome::admitted || !admitted.issues.empty() ||
        snapshot.generation != snapshot_before.generation || request.budgets[0].available != request_before.budgets[0].available)
        return EXIT_FAILURE;

    auto restricted = request; restricted.state.enforcement = acs::EnforcementCondition::restricted;
    if (acs::evaluate_admission(snapshot, restricted).outcome != acs::AcsAdmissionOutcome::admitted_with_restrictions) return EXIT_FAILURE;
    auto unavailable = request; unavailable.state.operational = acs::OperationalCondition::unavailable;
    if (acs::evaluate_admission(snapshot, unavailable).outcome != acs::AcsAdmissionOutcome::unavailable) return EXIT_FAILURE;
    auto unknown = request; unknown.identity_condition = acs::IdentityCondition::unknown;
    if (acs::evaluate_admission(snapshot, unknown).outcome != acs::AcsAdmissionOutcome::unknown) return EXIT_FAILURE;
    auto conflicting = request; conflicting.expected_registry_generation = acs::RegistryGeneration{0};
    if (acs::evaluate_admission(snapshot, conflicting).outcome != acs::AcsAdmissionOutcome::conflicting) return EXIT_FAILURE;

    auto stale_snapshot = snapshot; stale_snapshot.evidence[0].freshness = acs::FreshnessCondition::stale;
    if (acs::evaluate_admission(stale_snapshot, request).outcome != acs::AcsAdmissionOutcome::deferred) return EXIT_FAILURE;
    auto budget = request; budget.budgets[0].available = 1;
    if (acs::evaluate_admission(snapshot, budget).outcome != acs::AcsAdmissionOutcome::deferred) return EXIT_FAILURE;
    budget = request; budget.budgets[0].cost_known = false;
    if (acs::evaluate_admission(snapshot, budget).outcome != acs::AcsAdmissionOutcome::unknown) return EXIT_FAILURE;
    budget = request; budget.budgets[0].available_unit = acs::BudgetUnit::bytes;
    if (acs::evaluate_admission(snapshot, budget).outcome != acs::AcsAdmissionOutcome::rejected) return EXIT_FAILURE;
    budget = request; budget.budgets[0].cost = std::numeric_limits<std::uint64_t>::max(); budget.budgets[0].headroom = 1;
    if (acs::evaluate_admission(snapshot, budget).outcome != acs::AcsAdmissionOutcome::rejected) return EXIT_FAILURE;

    const auto revocation_id = *acs::RevocationId::parse("revocation", "one");
    if (!registry.register_revocation({revocation_id, ids.authority, ids.capability, ids.first, {"connection.use"}, acs::AuthorityCondition::revoked, acs::DescriptorRevision{1}}).ok()) return EXIT_FAILURE;
    auto revoked_request = request_for(registry.snapshot(), ids);
    const auto revoked = acs::evaluate_admission(registry.snapshot(), revoked_request);
    if (revoked.outcome != acs::AcsAdmissionOutcome::denied) return EXIT_FAILURE;
    revoked_request.connection_id = *acs::ConnectionId::parse("connection", "missing");
    return acs::evaluate_admission(registry.snapshot(), revoked_request).outcome == acs::AcsAdmissionOutcome::rejected ? EXIT_SUCCESS : EXIT_FAILURE;
}
