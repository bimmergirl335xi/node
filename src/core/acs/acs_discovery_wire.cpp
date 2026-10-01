#include "acs_discovery_wire.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <utility>

namespace prometheus::core::acs {
namespace {

constexpr std::array<std::uint8_t, 4> kMagic{{'A', 'C', 'S', 'D'}};

bool valid_kind(DiscoveryMessageKind kind) noexcept {
    return kind == DiscoveryMessageKind::presence ||
           kind == DiscoveryMessageKind::response ||
           kind == DiscoveryMessageKind::hint_query ||
           kind == DiscoveryMessageKind::hint_response;
}

bool valid_binding(const DiscoveryTransportBinding& binding) noexcept {
    return binding.transport_profile.valid() && !binding.address.empty() &&
           binding.address.size() <= kDefaultDiscoveryBindingBytes &&
           binding.port != 0;
}

bool valid_message(const DiscoveryMessage& message) noexcept {
    if (!valid_kind(message.kind) || !message.participant.valid() ||
        !valid_binding(message.binding) || message.boot_epoch == 0 ||
        message.hints.size() > kDiscoveryWireMaximumHints) {
        return false;
    }
    if (message.kind != DiscoveryMessageKind::hint_response &&
        !message.hints.empty()) {
        return false;
    }
    for (const auto& hint : message.hints) {
        if (!hint.participant.valid() || !valid_binding(hint.binding) ||
            hint.boot_epoch == 0) {
            return false;
        }
    }
    return true;
}

void put_u16(std::uint8_t* output, std::size_t& offset, std::uint16_t value) {
    output[offset++] = static_cast<std::uint8_t>(value >> 8U);
    output[offset++] = static_cast<std::uint8_t>(value);
}

void put_u32(std::uint8_t* output, std::size_t& offset, std::uint32_t value) {
    output[offset++] = static_cast<std::uint8_t>(value >> 24U);
    output[offset++] = static_cast<std::uint8_t>(value >> 16U);
    output[offset++] = static_cast<std::uint8_t>(value >> 8U);
    output[offset++] = static_cast<std::uint8_t>(value);
}

void put_u64(std::uint8_t* output, std::size_t& offset, std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        output[offset++] = static_cast<std::uint8_t>(value >> shift);
    }
}

bool put_text(
    std::uint8_t* output,
    std::size_t capacity,
    std::size_t& offset,
    const std::string& value) {
    if (value.empty() || value.size() > UINT16_MAX ||
        offset > capacity || capacity - offset < 2U + value.size()) {
        return false;
    }
    put_u16(output, offset, static_cast<std::uint16_t>(value.size()));
    std::memcpy(output + offset, value.data(), value.size());
    offset += value.size();
    return true;
}

template <typename Id>
bool put_id(
    std::uint8_t* output,
    std::size_t capacity,
    std::size_t& offset,
    const Id& id) {
    return put_text(output, capacity, offset, id.name_space()) &&
           put_text(output, capacity, offset, id.value());
}

bool put_binding(
    std::uint8_t* output,
    std::size_t capacity,
    std::size_t& offset,
    const DiscoveryTransportBinding& binding) {
    if (!put_text(output, capacity, offset, binding.transport_profile.value) ||
        !put_text(output, capacity, offset, binding.address) ||
        offset > capacity || capacity - offset < 2) {
        return false;
    }
    put_u16(output, offset, binding.port);
    return true;
}

bool get_u16(
    const std::uint8_t* input,
    std::size_t size,
    std::size_t& offset,
    std::uint16_t& value) {
    if (offset > size || size - offset < 2) return false;
    value = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(input[offset]) << 8U) |
        input[offset + 1]);
    offset += 2;
    return true;
}

bool get_u32(
    const std::uint8_t* input,
    std::size_t size,
    std::size_t& offset,
    std::uint32_t& value) {
    if (offset > size || size - offset < 4) return false;
    value = (static_cast<std::uint32_t>(input[offset]) << 24U) |
            (static_cast<std::uint32_t>(input[offset + 1]) << 16U) |
            (static_cast<std::uint32_t>(input[offset + 2]) << 8U) |
            input[offset + 3];
    offset += 4;
    return true;
}

bool get_u64(
    const std::uint8_t* input,
    std::size_t size,
    std::size_t& offset,
    std::uint64_t& value) {
    if (offset > size || size - offset < 8) return false;
    value = 0;
    for (int index = 0; index < 8; ++index) {
        value = (value << 8U) | input[offset++];
    }
    return true;
}

bool get_text(
    const std::uint8_t* input,
    std::size_t size,
    std::size_t& offset,
    std::string& value,
    std::size_t maximum) {
    std::uint16_t length = 0;
    if (!get_u16(input, size, offset, length) || length == 0 ||
        length > maximum || offset > size || size - offset < length) {
        return false;
    }
    value.assign(reinterpret_cast<const char*>(input + offset), length);
    offset += length;
    return true;
}

template <typename Id>
bool get_id(
    const std::uint8_t* input,
    std::size_t size,
    std::size_t& offset,
    Id& id) {
    std::string name_space;
    std::string value;
    if (!get_text(input, size, offset, name_space, kDefaultIdentifierBytes) ||
        !get_text(input, size, offset, value, kDefaultIdentifierBytes)) {
        return false;
    }
    auto parsed = Id::parse(std::move(name_space), std::move(value));
    if (!parsed) return false;
    id = std::move(*parsed);
    return true;
}

bool get_binding(
    const std::uint8_t* input,
    std::size_t size,
    std::size_t& offset,
    DiscoveryTransportBinding& binding) {
    if (!get_text(input, size, offset, binding.transport_profile.value,
                  kDefaultIdentifierBytes) ||
        !get_text(input, size, offset, binding.address,
                  kDefaultDiscoveryBindingBytes) ||
        !get_u16(input, size, offset, binding.port)) {
        return false;
    }
    return valid_binding(binding);
}

}  // namespace

DiscoveryWireResult encode_discovery_message(
    const DiscoveryMessage& message,
    std::uint8_t* output,
    std::size_t output_capacity) noexcept {
    if (output == nullptr) return {DiscoveryWireCode::invalid_argument, 0};
    if (!valid_message(message)) return {DiscoveryWireCode::invalid_message, 0};
    if (output_capacity < kDiscoveryWireHeaderBytes) {
        return {DiscoveryWireCode::output_too_small, 0};
    }
    try {
        std::size_t offset = kDiscoveryWireHeaderBytes;
        if (!put_id(output, output_capacity, offset, message.participant) ||
            !put_binding(output, output_capacity, offset, message.binding) ||
            offset > output_capacity || output_capacity - offset < 16) {
            return {DiscoveryWireCode::output_too_small, 0};
        }
        put_u64(output, offset, message.boot_epoch);
        put_u64(output, offset, message.nonce);
        for (const auto& hint : message.hints) {
            if (!put_id(output, output_capacity, offset, hint.participant) ||
                !put_binding(output, output_capacity, offset, hint.binding) ||
                offset > output_capacity || output_capacity - offset < 8) {
                return {DiscoveryWireCode::output_too_small, 0};
            }
            put_u64(output, offset, hint.boot_epoch);
        }
        if (offset > kDiscoveryWireMaximumBytes || offset > UINT32_MAX) {
            return {DiscoveryWireCode::oversized, 0};
        }
        std::copy(kMagic.begin(), kMagic.end(), output);
        std::size_t header = 4;
        put_u16(output, header, kDiscoveryWireVersion);
        output[header++] = static_cast<std::uint8_t>(message.kind);
        output[header++] = 0;
        put_u32(output, header, static_cast<std::uint32_t>(offset));
        put_u16(output, header, static_cast<std::uint16_t>(message.hints.size()));
        put_u16(output, header, 0);
        return {DiscoveryWireCode::success, offset};
    } catch (...) {
        return {DiscoveryWireCode::resource_exhausted, 0};
    }
}

DiscoveryWireResult decode_discovery_message(
    const std::uint8_t* input,
    std::size_t input_size,
    DiscoveryMessage& message) noexcept {
    if (input == nullptr) return {DiscoveryWireCode::invalid_argument, 0};
    if (input_size < kDiscoveryWireHeaderBytes) {
        return {DiscoveryWireCode::truncated, 0};
    }
    if (input_size > kDiscoveryWireMaximumBytes) {
        return {DiscoveryWireCode::oversized, 0};
    }
    if (!std::equal(kMagic.begin(), kMagic.end(), input)) {
        return {DiscoveryWireCode::invalid_header, 0};
    }
    std::size_t offset = 4;
    std::uint16_t version = 0;
    std::uint32_t declared_size = 0;
    std::uint16_t hint_count = 0;
    std::uint16_t reserved = 0;
    if (!get_u16(input, input_size, offset, version)) {
        return {DiscoveryWireCode::truncated, 0};
    }
    if (version != kDiscoveryWireVersion) {
        return {DiscoveryWireCode::unsupported_version, 0};
    }
    const auto raw_kind = input[offset++];
    if (input[offset++] != 0 ||
        !get_u32(input, input_size, offset, declared_size) ||
        !get_u16(input, input_size, offset, hint_count) ||
        !get_u16(input, input_size, offset, reserved) || reserved != 0) {
        return {DiscoveryWireCode::invalid_header, 0};
    }
    if (declared_size > kDiscoveryWireMaximumBytes) {
        return {DiscoveryWireCode::oversized, 0};
    }
    if (declared_size > input_size) return {DiscoveryWireCode::truncated, 0};
    if (declared_size < input_size) return {DiscoveryWireCode::trailing_data, 0};
    if (hint_count > kDiscoveryWireMaximumHints) {
        return {DiscoveryWireCode::too_many_hints, 0};
    }
    const auto kind = static_cast<DiscoveryMessageKind>(raw_kind);
    if (!valid_kind(kind)) return {DiscoveryWireCode::unsupported_kind, 0};
    if (kind != DiscoveryMessageKind::hint_response && hint_count != 0) {
        return {DiscoveryWireCode::invalid_field, 0};
    }
    try {
        DiscoveryMessage decoded{};
        decoded.kind = kind;
        if (!get_id(input, input_size, offset, decoded.participant) ||
            !get_binding(input, input_size, offset, decoded.binding) ||
            !get_u64(input, input_size, offset, decoded.boot_epoch) ||
            !get_u64(input, input_size, offset, decoded.nonce) ||
            decoded.boot_epoch == 0) {
            return {DiscoveryWireCode::invalid_field, 0};
        }
        decoded.hints.reserve(hint_count);
        for (std::uint16_t index = 0; index < hint_count; ++index) {
            DiscoveryHint hint{};
            if (!get_id(input, input_size, offset, hint.participant) ||
                !get_binding(input, input_size, offset, hint.binding) ||
                !get_u64(input, input_size, offset, hint.boot_epoch) ||
                hint.boot_epoch == 0) {
                return {DiscoveryWireCode::invalid_field, 0};
            }
            decoded.hints.push_back(std::move(hint));
        }
        if (offset != input_size) return {DiscoveryWireCode::trailing_data, 0};
        message = std::move(decoded);
        return {DiscoveryWireCode::success, input_size};
    } catch (...) {
        return {DiscoveryWireCode::resource_exhausted, 0};
    }
}

const char* to_string(DiscoveryWireCode code) noexcept {
    static constexpr const char* names[] = {
        "success", "invalid_argument", "invalid_message", "output_too_small",
        "invalid_header", "truncated", "oversized", "unsupported_version",
        "unsupported_kind", "invalid_field", "too_many_hints",
        "trailing_data", "resource_exhausted",
    };
    const auto index = static_cast<std::size_t>(code);
    return index < sizeof(names) / sizeof(names[0]) ? names[index] : "invalid_field";
}

const char* to_string(DiscoveryMessageKind kind) noexcept {
    switch (kind) {
        case DiscoveryMessageKind::presence: return "presence";
        case DiscoveryMessageKind::response: return "response";
        case DiscoveryMessageKind::hint_query: return "hint_query";
        case DiscoveryMessageKind::hint_response: return "hint_response";
    }
    return "unsupported";
}

}  // namespace prometheus::core::acs
