#ifndef SRC_CORE_ACS_ACS_DISCOVERY_HPP
#define SRC_CORE_ACS_ACS_DISCOVERY_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "acs_types.hpp"

namespace prometheus::core::acs {

inline constexpr std::size_t kDefaultDiscoveryObservations = 256;
inline constexpr std::size_t kAbsoluteDiscoveryObservations = 4096;
inline constexpr std::size_t kDefaultDiscoveryBindingBytes = 64;
inline constexpr std::size_t kAbsoluteDiscoveryBindingBytes = 256;

enum class DiscoveryObservationKind : std::uint8_t {
    direct = 0,
    hint,
};

enum class DiscoveryObservationState : std::uint8_t {
    observed = 0,
    stale,
    conflict,
};

struct DiscoveryTransportBinding {
    ScopeReference transport_profile{};
    std::string address{};
    std::uint16_t port = 0;
};

struct DiscoveryObservation {
    ParticipantId participant{};
    DiscoveryTransportBinding binding{};
    DiscoveryObservationKind kind = DiscoveryObservationKind::hint;
    std::uint16_t protocol_revision = 0;
    std::uint64_t boot_epoch = 0;
    std::uint64_t first_seen_ms = 0;
    std::uint64_t last_seen_ms = 0;
    DiscoveryObservationState state = DiscoveryObservationState::observed;
};

struct DiscoveryStoreOptions {
    std::size_t maximum_observations = kDefaultDiscoveryObservations;
    std::size_t maximum_binding_bytes = kDefaultDiscoveryBindingBytes;
};

enum class DiscoveryObservationCode : std::uint8_t {
    observed = 0,
    refreshed,
    hinted,
    hint_refreshed,
    promoted_to_direct,
    rediscovered,
    conflict,
    self_rejected,
    malformed,
    capacity_exhausted,
    invalid_configuration,
    resource_exhausted,
};

struct DiscoveryObservationResult {
    DiscoveryObservationCode code = DiscoveryObservationCode::malformed;
    DiscoveryObservation observation{};
    bool evidence_required = false;
    [[nodiscard]] bool accepted() const noexcept {
        return code == DiscoveryObservationCode::observed ||
               code == DiscoveryObservationCode::refreshed ||
               code == DiscoveryObservationCode::hinted ||
               code == DiscoveryObservationCode::hint_refreshed ||
               code == DiscoveryObservationCode::promoted_to_direct ||
               code == DiscoveryObservationCode::rediscovered;
    }
};

struct DiscoveryStaleTransition {
    ParticipantId participant{};
    DiscoveryObservationKind kind = DiscoveryObservationKind::hint;
    std::uint64_t last_seen_ms = 0;
};

// Volatile current-boot observation state. It does not register participants,
// relationships, connections, attachments, capabilities, or authorities.
class DiscoveryObservationStore {
public:
    explicit DiscoveryObservationStore(
        ParticipantId self,
        DiscoveryStoreOptions options = {});

    [[nodiscard]] DiscoveryObservationResult observe_direct(
        ParticipantId participant,
        DiscoveryTransportBinding binding,
        std::uint16_t protocol_revision,
        std::uint64_t boot_epoch,
        std::uint64_t now_ms) noexcept;

    [[nodiscard]] DiscoveryObservationResult observe_hint(
        ParticipantId participant,
        DiscoveryTransportBinding binding,
        std::uint16_t protocol_revision,
        std::uint64_t boot_epoch,
        std::uint64_t now_ms) noexcept;

    [[nodiscard]] std::vector<DiscoveryStaleTransition> mark_stale(
        std::uint64_t now_ms,
        std::uint64_t stale_after_ms) noexcept;

    [[nodiscard]] std::vector<DiscoveryObservation> snapshot() const;
    [[nodiscard]] std::size_t size() const noexcept { return observations_.size(); }
    [[nodiscard]] std::size_t peak_size() const noexcept { return peak_size_; }
    [[nodiscard]] bool valid() const noexcept { return options_valid_; }

private:
    [[nodiscard]] DiscoveryObservationResult observe(
        ParticipantId participant,
        DiscoveryTransportBinding binding,
        DiscoveryObservationKind kind,
        std::uint16_t protocol_revision,
        std::uint64_t boot_epoch,
        std::uint64_t now_ms) noexcept;

    ParticipantId self_{};
    DiscoveryStoreOptions options_{};
    bool options_valid_ = false;
    std::vector<DiscoveryObservation> observations_{};
    std::size_t peak_size_ = 0;
};

[[nodiscard]] const char* to_string(DiscoveryObservationCode code) noexcept;
[[nodiscard]] const char* to_string(DiscoveryObservationKind kind) noexcept;
[[nodiscard]] const char* to_string(DiscoveryObservationState state) noexcept;

}  // namespace prometheus::core::acs

#endif
