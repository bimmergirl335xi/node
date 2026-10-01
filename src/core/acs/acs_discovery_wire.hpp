#ifndef SRC_CORE_ACS_ACS_DISCOVERY_WIRE_HPP
#define SRC_CORE_ACS_ACS_DISCOVERY_WIRE_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include "acs_discovery.hpp"

namespace prometheus::core::acs {

inline constexpr std::uint16_t kDiscoveryWireVersion = 1;
inline constexpr std::size_t kDiscoveryWireHeaderBytes = 16;
inline constexpr std::size_t kDiscoveryWireMaximumBytes = 1024;
inline constexpr std::size_t kDiscoveryWireMaximumHints = 8;

enum class DiscoveryMessageKind : std::uint8_t {
    presence = 1,
    response = 2,
    hint_query = 3,
    hint_response = 4,
};

struct DiscoveryHint {
    ParticipantId participant{};
    DiscoveryTransportBinding binding{};
    std::uint64_t boot_epoch = 0;
};

struct DiscoveryMessage {
    DiscoveryMessageKind kind = DiscoveryMessageKind::presence;
    ParticipantId participant{};
    DiscoveryTransportBinding binding{};
    std::uint64_t boot_epoch = 0;
    std::uint64_t nonce = 0;
    std::vector<DiscoveryHint> hints{};
};

enum class DiscoveryWireCode : std::uint8_t {
    success = 0,
    invalid_argument,
    invalid_message,
    output_too_small,
    invalid_header,
    truncated,
    oversized,
    unsupported_version,
    unsupported_kind,
    invalid_field,
    too_many_hints,
    trailing_data,
    resource_exhausted,
};

struct DiscoveryWireResult {
    DiscoveryWireCode code = DiscoveryWireCode::success;
    std::size_t bytes = 0;
    [[nodiscard]] bool ok() const noexcept {
        return code == DiscoveryWireCode::success;
    }
};

// Public reference discovery representation only. Claimed identities and boot
// epochs are not authenticated, trusted, or authoritative by this codec.
[[nodiscard]] DiscoveryWireResult encode_discovery_message(
    const DiscoveryMessage& message,
    std::uint8_t* output,
    std::size_t output_capacity) noexcept;

[[nodiscard]] DiscoveryWireResult decode_discovery_message(
    const std::uint8_t* input,
    std::size_t input_size,
    DiscoveryMessage& message) noexcept;

[[nodiscard]] const char* to_string(DiscoveryWireCode code) noexcept;
[[nodiscard]] const char* to_string(DiscoveryMessageKind kind) noexcept;

}  // namespace prometheus::core::acs

#endif
