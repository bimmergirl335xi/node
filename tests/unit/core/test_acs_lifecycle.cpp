#include <cstdlib>

#include "acs_test_fixture.hpp"
#include "core/acs/acs_lifecycle.hpp"

namespace acs = prometheus::core::acs;

template <typename Id>
Id tid(const char* value) { return *Id::parse("transition", value); }

int main() {
    acs::AcsRegistry registry{}; acs_test::FixtureIds ids{};
    if (!acs_test::populate(registry, ids)) return EXIT_FAILURE;
    acs::StateStoreOptions options{}; options.maximum_history_per_connection = 3; options.maximum_idempotency_records = 2;
    acs::ConnectionStateStore store{registry, options};
    acs::LifecycleTransitionRequest lifecycle{tid<acs::TransitionId>("propose"), ids.connection, acs::LifecycleRevision{0}, acs::ConnectionLifecycle::proposed, 0};
    const auto proposed = store.transition_lifecycle(lifecycle);
    if (!proposed.ok() || proposed.state.lifecycle != acs::ConnectionLifecycle::proposed || proposed.state.operational != acs::OperationalCondition::unknown || proposed.state.enforcement != acs::EnforcementCondition::unknown) return EXIT_FAILURE;
    if (!store.transition_lifecycle(lifecycle).replayed) return EXIT_FAILURE;
    lifecycle.desired = acs::ConnectionLifecycle::failed;
    if (store.transition_lifecycle(lifecycle).code != acs::TransitionCode::transition_identity_conflict) return EXIT_FAILURE;
    lifecycle.transition_id = tid<acs::TransitionId>("illegal"); lifecycle.desired = acs::ConnectionLifecycle::active; lifecycle.expected_revision = acs::LifecycleRevision{1};
    if (store.transition_lifecycle(lifecycle).code != acs::TransitionCode::invalid_transition) return EXIT_FAILURE;
    lifecycle.transition_id = tid<acs::TransitionId>("pending"); lifecycle.desired = acs::ConnectionLifecycle::admission_pending;
    if (!store.transition_lifecycle(lifecycle).ok()) return EXIT_FAILURE;

    acs::OperationalTransitionRequest operational{tid<acs::TransitionId>("nominal"), ids.connection, acs::OperationalRevision{0}, acs::OperationalCondition::nominal, store.retained_idempotency_horizon()};
    if (!store.transition_operational(operational).ok()) return EXIT_FAILURE;

    acs::EnforcementTransitionRequest enforcement{}; enforcement.transition_id = tid<acs::TransitionId>("restrict");
    enforcement.connection_id = ids.connection; enforcement.desired = acs::EnforcementCondition::restricted;
    enforcement.subject = ids.first; enforcement.scope = {"connection.use"}; enforcement.authority = ids.authority;
    enforcement.known_idempotency_horizon = store.retained_idempotency_horizon();
    if (!store.transition_enforcement(enforcement).ok()) return EXIT_FAILURE;
    enforcement.transition_id = tid<acs::TransitionId>("quarantine"); enforcement.expected_revision = acs::EnforcementRevision{1};
    enforcement.desired = acs::EnforcementCondition::quarantined; enforcement.containment_authorization = ids.containment;
    enforcement.known_idempotency_horizon = store.retained_idempotency_horizon();
    if (!store.transition_enforcement(enforcement).ok()) return EXIT_FAILURE;
    enforcement.transition_id = tid<acs::TransitionId>("direct-unrestricted"); enforcement.expected_revision = acs::EnforcementRevision{2};
    enforcement.desired = acs::EnforcementCondition::unrestricted; enforcement.known_idempotency_horizon = store.retained_idempotency_horizon();
    if (store.transition_enforcement(enforcement).code != acs::TransitionCode::invalid_transition) return EXIT_FAILURE;
    enforcement.transition_id = tid<acs::TransitionId>("restore-restricted"); enforcement.desired = acs::EnforcementCondition::restricted;
    enforcement.restoration_clearance = ids.clearance; enforcement.known_idempotency_horizon = store.retained_idempotency_horizon();
    if (!store.transition_enforcement(enforcement).ok()) return EXIT_FAILURE;
    enforcement.transition_id = tid<acs::TransitionId>("restore-open"); enforcement.expected_revision = acs::EnforcementRevision{3};
    enforcement.desired = acs::EnforcementCondition::unrestricted; enforcement.capability = ids.capability;
    enforcement.admission_reference = ids.evidence; enforcement.known_idempotency_horizon = store.retained_idempotency_horizon();
    if (!store.transition_enforcement(enforcement).ok()) return EXIT_FAILURE;

    const auto state = store.find(ids.connection);
    if (!state || state->history.size() != 3 || state->lifecycle_revision.value() != 2 ||
        state->operational_revision.value() != 1 || state->enforcement_revision.value() != 4) return EXIT_FAILURE;
    const auto expired = store.transition_lifecycle({tid<acs::TransitionId>("forgotten"), ids.connection, acs::LifecycleRevision{2}, acs::ConnectionLifecycle::admitted, 0});
    return expired.code == acs::TransitionCode::idempotency_expired ? EXIT_SUCCESS : EXIT_FAILURE;
}
