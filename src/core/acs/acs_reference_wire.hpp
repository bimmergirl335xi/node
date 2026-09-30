#ifndef SRC_CORE_ACS_ACS_REFERENCE_WIRE_HPP
#define SRC_CORE_ACS_ACS_REFERENCE_WIRE_HPP

#include <cstddef>
#include <cstdint>

#include "acs_transport_contract.hpp"

namespace prometheus::core::acs {

inline constexpr std::uint16_t kReferenceWireVersion = 1;
inline constexpr std::size_t kReferenceWireHeaderBytes = 16;
inline constexpr std::size_t kReferenceWireMaximumBytes = 8192;
inline constexpr std::size_t kReferenceWireMaximumInlineBytes = 256;
inline constexpr std::size_t kReferenceWireMaximumProvenance = 4;

enum class ReferenceWireCode : std::uint8_t {
    success = 0,
    invalid_argument,
    invalid_envelope,
    output_too_small,
    invalid_header,
    truncated,
    oversized,
    unsupported_version,
    invalid_field,
    trailing_data,
    resource_exhausted,
};

struct ReferenceWireResult {
    ReferenceWireCode code = ReferenceWireCode::success;
    std::size_t bytes = 0;
    [[nodiscard]] bool ok() const noexcept {
        return code == ReferenceWireCode::success;
    }
};

// Deterministic DEV-002C conformance representation. This is not a general
// ACS serialization framework or a production-secure session protocol.
[[nodiscard]] ReferenceWireResult encode_reference_signal(
    const SignalEnvelope& envelope,
    std::uint8_t* output,
    std::size_t output_capacity) noexcept;

[[nodiscard]] ReferenceWireResult decode_reference_signal(
    const std::uint8_t* input,
    std::size_t input_size,
    SignalEnvelope& envelope) noexcept;

[[nodiscard]] const char* to_string(ReferenceWireCode code) noexcept;

}  // namespace prometheus::core::acs

#endif
