#include <array>
#include <cstdlib>
#include <cstring>

#include "core/acs/acs_discovery_wire.hpp"

namespace acs = prometheus::core::acs;

template <typename Id>
Id id(const char* name_space, const char* value) {
    return *Id::parse(name_space, value);
}

acs::DiscoveryMessage message(acs::DiscoveryMessageKind kind) {
    acs::DiscoveryMessage value{};
    value.kind = kind;
    value.participant = id<acs::ParticipantId>("node", "node-001");
    value.binding = {{"isolated.udp.ipv4.discovery.v1"}, "10.77.0.1", 39002};
    value.boot_epoch = 123;
    value.nonce = 456;
    return value;
}

int main() {
    std::array<std::uint8_t, acs::kDiscoveryWireMaximumBytes> encoded{};
    auto original = message(acs::DiscoveryMessageKind::hint_response);
    for (std::size_t index = 0; index < acs::kDiscoveryWireMaximumHints; ++index) {
        original.hints.push_back({
            id<acs::ParticipantId>("node", ("node-" + std::to_string(index + 2)).c_str()),
            {{"isolated.udp.ipv4.discovery.v1"},
             "10.77.0." + std::to_string(index + 2), 39002},
            1000 + index});
    }
    const auto result = acs::encode_discovery_message(
        original, encoded.data(), encoded.size());
    acs::DiscoveryMessage decoded{};
    if (!result.ok() ||
        !acs::decode_discovery_message(
             encoded.data(), result.bytes, decoded).ok() ||
        decoded.kind != original.kind ||
        decoded.participant != original.participant ||
        decoded.binding.address != original.binding.address ||
        decoded.boot_epoch != original.boot_epoch ||
        decoded.nonce != original.nonce ||
        decoded.hints.size() != acs::kDiscoveryWireMaximumHints) {
        return EXIT_FAILURE;
    }

    auto too_many = original;
    too_many.hints.push_back(original.hints.front());
    if (acs::encode_discovery_message(
            too_many, encoded.data(), encoded.size()).code !=
        acs::DiscoveryWireCode::invalid_message) {
        return EXIT_FAILURE;
    }
    auto invalid_family = message(acs::DiscoveryMessageKind::presence);
    invalid_family.hints.push_back(original.hints.front());
    if (acs::encode_discovery_message(
            invalid_family, encoded.data(), encoded.size()).code !=
        acs::DiscoveryWireCode::invalid_message) {
        return EXIT_FAILURE;
    }
    if (acs::decode_discovery_message(
            encoded.data(), 8, decoded).code != acs::DiscoveryWireCode::truncated) {
        return EXIT_FAILURE;
    }
    auto malformed = encoded;
    malformed[0] = 0;
    if (acs::decode_discovery_message(
            malformed.data(), result.bytes, decoded).code !=
        acs::DiscoveryWireCode::invalid_header) {
        return EXIT_FAILURE;
    }
    malformed = encoded;
    malformed[5] = 2;
    if (acs::decode_discovery_message(
            malformed.data(), result.bytes, decoded).code !=
        acs::DiscoveryWireCode::unsupported_version) {
        return EXIT_FAILURE;
    }
    malformed = encoded;
    malformed[6] = 99;
    if (acs::decode_discovery_message(
            malformed.data(), result.bytes, decoded).code !=
        acs::DiscoveryWireCode::unsupported_kind) {
        return EXIT_FAILURE;
    }
    malformed = encoded;
    malformed[8] = 0;
    malformed[9] = 0;
    malformed[10] = 8;
    malformed[11] = 0;
    if (acs::decode_discovery_message(
            malformed.data(), result.bytes, decoded).code !=
        acs::DiscoveryWireCode::oversized) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
