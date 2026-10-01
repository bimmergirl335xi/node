#include "acs_discovery.hpp"

#include <algorithm>
#include <utility>

namespace prometheus::core::acs {
namespace {

bool valid_options(const DiscoveryStoreOptions& options) noexcept {
    return options.maximum_observations > 0 &&
           options.maximum_observations <= kAbsoluteDiscoveryObservations &&
           options.maximum_binding_bytes > 0 &&
           options.maximum_binding_bytes <= kAbsoluteDiscoveryBindingBytes;
}

bool valid_binding(
    const DiscoveryTransportBinding& binding,
    std::size_t maximum_bytes) noexcept {
    return binding.transport_profile.valid() && !binding.address.empty() &&
           binding.address.size() <= maximum_bytes && binding.port != 0;
}

bool same_binding(
    const DiscoveryTransportBinding& lhs,
    const DiscoveryTransportBinding& rhs) noexcept {
    return lhs.transport_profile.value == rhs.transport_profile.value &&
           lhs.address == rhs.address && lhs.port == rhs.port;
}

DiscoveryObservationResult failure(DiscoveryObservationCode code) noexcept {
    DiscoveryObservationResult result{};
    result.code = code;
    result.evidence_required = code != DiscoveryObservationCode::malformed;
    return result;
}

}  // namespace

DiscoveryObservationStore::DiscoveryObservationStore(
    ParticipantId self,
    DiscoveryStoreOptions options)
    : self_(std::move(self)),
      options_(std::move(options)),
      options_valid_(self_.valid() && valid_options(options_)) {
    if (!options_valid_) return;
    try {
        observations_.reserve(options_.maximum_observations);
    } catch (...) {
        options_valid_ = false;
    }
}

DiscoveryObservationResult DiscoveryObservationStore::observe_direct(
    ParticipantId participant,
    DiscoveryTransportBinding binding,
    std::uint16_t protocol_revision,
    std::uint64_t boot_epoch,
    std::uint64_t now_ms) noexcept {
    return observe(std::move(participant), std::move(binding),
                   DiscoveryObservationKind::direct, protocol_revision,
                   boot_epoch, now_ms);
}

DiscoveryObservationResult DiscoveryObservationStore::observe_hint(
    ParticipantId participant,
    DiscoveryTransportBinding binding,
    std::uint16_t protocol_revision,
    std::uint64_t boot_epoch,
    std::uint64_t now_ms) noexcept {
    return observe(std::move(participant), std::move(binding),
                   DiscoveryObservationKind::hint, protocol_revision,
                   boot_epoch, now_ms);
}

DiscoveryObservationResult DiscoveryObservationStore::observe(
    ParticipantId participant,
    DiscoveryTransportBinding binding,
    DiscoveryObservationKind kind,
    std::uint16_t protocol_revision,
    std::uint64_t boot_epoch,
    std::uint64_t now_ms) noexcept {
    try {
        if (!options_valid_) return failure(DiscoveryObservationCode::invalid_configuration);
        if (!participant.valid() || protocol_revision == 0 || boot_epoch == 0 ||
            !valid_binding(binding, options_.maximum_binding_bytes)) {
            return failure(DiscoveryObservationCode::malformed);
        }
        if (participant == self_) return failure(DiscoveryObservationCode::self_rejected);

        auto by_identity = std::find_if(
            observations_.begin(), observations_.end(),
            [&](const auto& value) { return value.participant == participant; });
        auto by_binding = std::find_if(
            observations_.begin(), observations_.end(),
            [&](const auto& value) {
                return value.participant != participant &&
                       same_binding(value.binding, binding);
            });

        if (by_identity != observations_.end()) {
            const bool binding_changed = !same_binding(by_identity->binding, binding);
            const bool epoch_changed = by_identity->boot_epoch != boot_epoch;
            const bool restart_after_stale =
                by_identity->state == DiscoveryObservationState::stale &&
                kind == DiscoveryObservationKind::direct;
            if (binding_changed || (epoch_changed && !restart_after_stale) ||
                by_binding != observations_.end()) {
                by_identity->state = DiscoveryObservationState::conflict;
                DiscoveryObservationResult result{};
                result.code = DiscoveryObservationCode::conflict;
                result.observation = *by_identity;
                result.evidence_required = true;
                return result;
            }
            if (by_identity->state == DiscoveryObservationState::conflict) {
                DiscoveryObservationResult result{};
                result.code = DiscoveryObservationCode::conflict;
                result.observation = *by_identity;
                result.evidence_required = false;
                return result;
            }

            const bool promoted = by_identity->kind == DiscoveryObservationKind::hint &&
                                  kind == DiscoveryObservationKind::direct;
            const bool rediscovered = by_identity->state == DiscoveryObservationState::stale &&
                                      kind == DiscoveryObservationKind::direct;
            if (restart_after_stale) {
                by_identity->binding = std::move(binding);
                by_identity->boot_epoch = boot_epoch;
            }
            if (promoted) by_identity->kind = DiscoveryObservationKind::direct;
            by_identity->protocol_revision = protocol_revision;
            by_identity->last_seen_ms = now_ms;
            if (kind == DiscoveryObservationKind::direct) {
                by_identity->state = DiscoveryObservationState::observed;
            }
            DiscoveryObservationResult result{};
            result.code = rediscovered
                ? DiscoveryObservationCode::rediscovered
                : promoted
                    ? DiscoveryObservationCode::promoted_to_direct
                    : kind == DiscoveryObservationKind::direct
                        ? DiscoveryObservationCode::refreshed
                        : DiscoveryObservationCode::hint_refreshed;
            result.observation = *by_identity;
            result.evidence_required = rediscovered || promoted;
            return result;
        }

        if (by_binding != observations_.end()) {
            by_binding->state = DiscoveryObservationState::conflict;
            DiscoveryObservationResult result{};
            result.code = DiscoveryObservationCode::conflict;
            result.observation = *by_binding;
            result.evidence_required = true;
            return result;
        }
        if (observations_.size() >= options_.maximum_observations) {
            return failure(DiscoveryObservationCode::capacity_exhausted);
        }

        DiscoveryObservation observation{};
        observation.participant = std::move(participant);
        observation.binding = std::move(binding);
        observation.kind = kind;
        observation.protocol_revision = protocol_revision;
        observation.boot_epoch = boot_epoch;
        observation.first_seen_ms = now_ms;
        observation.last_seen_ms = now_ms;
        observation.state = DiscoveryObservationState::observed;
        observations_.push_back(observation);
        peak_size_ = std::max(peak_size_, observations_.size());
        DiscoveryObservationResult result{};
        result.code = kind == DiscoveryObservationKind::direct
            ? DiscoveryObservationCode::observed
            : DiscoveryObservationCode::hinted;
        result.observation = std::move(observation);
        result.evidence_required = true;
        return result;
    } catch (...) {
        return failure(DiscoveryObservationCode::resource_exhausted);
    }
}

std::vector<DiscoveryStaleTransition> DiscoveryObservationStore::mark_stale(
    std::uint64_t now_ms,
    std::uint64_t stale_after_ms) noexcept {
    std::vector<DiscoveryStaleTransition> transitions;
    if (!options_valid_ || stale_after_ms == 0) return transitions;
    try {
        transitions.reserve(observations_.size());
        for (auto& observation : observations_) {
            if (observation.state != DiscoveryObservationState::observed ||
                now_ms < observation.last_seen_ms ||
                now_ms - observation.last_seen_ms < stale_after_ms) {
                continue;
            }
            observation.state = DiscoveryObservationState::stale;
            transitions.push_back({observation.participant, observation.kind,
                                   observation.last_seen_ms});
        }
    } catch (...) {
        transitions.clear();
    }
    return transitions;
}

std::vector<DiscoveryObservation> DiscoveryObservationStore::snapshot() const {
    auto result = observations_;
    std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.participant < rhs.participant;
    });
    return result;
}

const char* to_string(DiscoveryObservationCode code) noexcept {
    static constexpr const char* names[] = {
        "observed", "refreshed", "hinted", "hint_refreshed",
        "promoted_to_direct", "rediscovered", "conflict", "self_rejected",
        "malformed", "capacity_exhausted", "invalid_configuration",
        "resource_exhausted",
    };
    const auto index = static_cast<std::size_t>(code);
    return index < sizeof(names) / sizeof(names[0]) ? names[index] : "malformed";
}

const char* to_string(DiscoveryObservationKind kind) noexcept {
    return kind == DiscoveryObservationKind::direct ? "direct" : "hint";
}

const char* to_string(DiscoveryObservationState state) noexcept {
    switch (state) {
        case DiscoveryObservationState::observed: return "observed";
        case DiscoveryObservationState::stale: return "stale";
        case DiscoveryObservationState::conflict: return "conflict";
    }
    return "conflict";
}

}  // namespace prometheus::core::acs
