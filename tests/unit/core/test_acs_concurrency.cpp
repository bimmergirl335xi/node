#include <atomic>
#include <cstdlib>
#include <thread>
#include <vector>

#include "acs_test_fixture.hpp"
#include "core/acs/acs_lifecycle.hpp"

namespace acs = prometheus::core::acs;

int main() {
    acs::AcsRegistry registry{}; acs_test::FixtureIds ids{};
    if (!acs_test::populate(registry, ids)) return EXIT_FAILURE;
    acs::ConnectionStateStore store{registry};
    const auto first = *acs::TransitionId::parse("transition", "initial");
    if (!store.transition_lifecycle({first, ids.connection, acs::LifecycleRevision{0}, acs::ConnectionLifecycle::proposed, 0}).ok()) return EXIT_FAILURE;
    std::atomic<int> successes{0}; std::atomic<int> conflicts{0};
    std::vector<std::thread> threads;
    for (int index = 0; index < 16; ++index) {
        threads.emplace_back([&, index] {
            const auto transition = *acs::TransitionId::parse("transition", "race-" + std::to_string(index));
            const auto result = store.transition_lifecycle({transition, ids.connection, acs::LifecycleRevision{1}, acs::ConnectionLifecycle::admission_pending, store.retained_idempotency_horizon()});
            if (result.ok()) ++successes;
            else if (result.code == acs::TransitionCode::revision_conflict) ++conflicts;
        });
    }
    for (auto& thread : threads) thread.join();
    const auto snapshots = store.snapshots();
    return successes == 1 && conflicts == 15 && snapshots.size() == 1 &&
           snapshots[0].lifecycle == acs::ConnectionLifecycle::admission_pending &&
           snapshots[0].lifecycle_revision.value() == 2 ? EXIT_SUCCESS : EXIT_FAILURE;
}
