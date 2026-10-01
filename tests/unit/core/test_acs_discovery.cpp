#include <cstdlib>

#include "core/acs/acs_discovery.hpp"
#include "core/acs/acs_registry.hpp"

namespace acs = prometheus::core::acs;

template <typename Id>
Id id(const char* name_space, const char* value) {
    return *Id::parse(name_space, value);
}

acs::DiscoveryTransportBinding binding(const char* address, std::uint16_t port = 39002) {
    return {{"isolated.udp.ipv4.discovery.v1"}, address, port};
}

int main() {
    const auto self = id<acs::ParticipantId>("node", "node-001");
    const auto second = id<acs::ParticipantId>("node", "node-002");
    const auto third = id<acs::ParticipantId>("node", "node-003");
    acs::DiscoveryStoreOptions options{};
    options.maximum_observations = 2;
    acs::DiscoveryObservationStore store{self, options};
    acs::AcsRegistry registry{};

    if (!store.valid() || store.size() != 0 ||
        store.observe_direct(self, binding("10.77.0.1"), 1, 1, 10).code !=
            acs::DiscoveryObservationCode::self_rejected ||
        store.size() != 0) {
        return EXIT_FAILURE;
    }

    const auto hinted = store.observe_hint(
        second, binding("10.77.0.2"), 1, 22, 100);
    if (hinted.code != acs::DiscoveryObservationCode::hinted ||
        hinted.observation.kind != acs::DiscoveryObservationKind::hint ||
        !hinted.evidence_required) {
        return EXIT_FAILURE;
    }
    const auto promoted = store.observe_direct(
        second, binding("10.77.0.2"), 1, 22, 110);
    if (promoted.code != acs::DiscoveryObservationCode::promoted_to_direct ||
        promoted.observation.kind != acs::DiscoveryObservationKind::direct ||
        !promoted.evidence_required) {
        return EXIT_FAILURE;
    }
    const auto duplicate = store.observe_direct(
        second, binding("10.77.0.2"), 1, 22, 120);
    if (duplicate.code != acs::DiscoveryObservationCode::refreshed ||
        duplicate.evidence_required || duplicate.observation.last_seen_ms != 120) {
        return EXIT_FAILURE;
    }

    const auto direct = store.observe_direct(
        third, binding("10.77.0.3"), 1, 33, 120);
    if (direct.code != acs::DiscoveryObservationCode::observed ||
        store.peak_size() != 2) {
        return EXIT_FAILURE;
    }
    if (store.observe_hint(
            id<acs::ParticipantId>("node", "node-004"),
            binding("10.77.0.4"), 1, 44, 120).code !=
        acs::DiscoveryObservationCode::capacity_exhausted) {
        return EXIT_FAILURE;
    }

    const auto stale = store.mark_stale(1121, 1000);
    if (stale.size() != 2 ||
        store.snapshot().front().state != acs::DiscoveryObservationState::stale) {
        return EXIT_FAILURE;
    }
    const auto rediscovered = store.observe_direct(
        second, binding("10.77.0.2"), 1, 222, 1130);
    if (rediscovered.code != acs::DiscoveryObservationCode::rediscovered ||
        rediscovered.observation.state != acs::DiscoveryObservationState::observed ||
        rediscovered.observation.boot_epoch != 222) {
        return EXIT_FAILURE;
    }
    const auto identity_conflict = store.observe_direct(
        second, binding("10.77.0.9"), 1, 222, 1140);
    if (identity_conflict.code != acs::DiscoveryObservationCode::conflict ||
        identity_conflict.observation.state != acs::DiscoveryObservationState::conflict) {
        return EXIT_FAILURE;
    }

    acs::DiscoveryObservationStore binding_conflicts{self};
    if (!binding_conflicts.observe_direct(
             second, binding("10.77.0.2"), 1, 22, 1).accepted() ||
        binding_conflicts.observe_direct(
             third, binding("10.77.0.2"), 1, 33, 2).code !=
             acs::DiscoveryObservationCode::conflict) {
        return EXIT_FAILURE;
    }
    acs::DiscoveryObservationStore epoch_conflicts{self};
    if (!epoch_conflicts.observe_direct(
             second, binding("10.77.0.2"), 1, 22, 1).accepted() ||
        epoch_conflicts.observe_direct(
             second, binding("10.77.0.2"), 1, 23, 2).code !=
             acs::DiscoveryObservationCode::conflict) {
        return EXIT_FAILURE;
    }

    // Observation is intentionally separate from the canonical descriptor
    // graph and cannot manufacture authority or communication state.
    const auto canonical = registry.snapshot();
    return canonical.participants.empty() && canonical.authorities.empty() &&
           canonical.capabilities.empty() && canonical.relationships.empty() &&
           canonical.connections.empty()
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}
