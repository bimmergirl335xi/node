#include "acs_reference_wire.hpp"

#include <algorithm>
#include <cstring>
#include <string>
#include <utility>

namespace prometheus::core::acs {
namespace {

constexpr std::uint32_t kMagic = UINT32_C(0x4e414353);  // NACS

void store_u16(std::uint8_t* output, std::uint16_t value) noexcept {
    output[0] = static_cast<std::uint8_t>(value >> 8U);
    output[1] = static_cast<std::uint8_t>(value);
}

void store_u32(std::uint8_t* output, std::uint32_t value) noexcept {
    output[0] = static_cast<std::uint8_t>(value >> 24U);
    output[1] = static_cast<std::uint8_t>(value >> 16U);
    output[2] = static_cast<std::uint8_t>(value >> 8U);
    output[3] = static_cast<std::uint8_t>(value);
}

std::uint16_t load_u16(const std::uint8_t* input) noexcept {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(input[0]) << 8U) |
        static_cast<std::uint16_t>(input[1]));
}

std::uint32_t load_u32(const std::uint8_t* input) noexcept {
    return (static_cast<std::uint32_t>(input[0]) << 24U) |
           (static_cast<std::uint32_t>(input[1]) << 16U) |
           (static_cast<std::uint32_t>(input[2]) << 8U) |
           static_cast<std::uint32_t>(input[3]);
}

class Writer {
public:
    Writer(std::uint8_t* output, std::size_t capacity) noexcept
        : output_(output), capacity_(capacity), position_(kReferenceWireHeaderBytes) {}

    bool u8(std::uint8_t value) noexcept {
        if (!reserve(1)) return false;
        output_[position_++] = value;
        return true;
    }

    bool u16(std::uint16_t value) noexcept {
        if (!reserve(2)) return false;
        store_u16(output_ + position_, value);
        position_ += 2;
        return true;
    }

    bool u32(std::uint32_t value) noexcept {
        if (!reserve(4)) return false;
        store_u32(output_ + position_, value);
        position_ += 4;
        return true;
    }

    bool bytes(const std::uint8_t* value, std::size_t length) noexcept {
        if (!reserve(length)) return false;
        if (length != 0) std::memcpy(output_ + position_, value, length);
        position_ += length;
        return true;
    }

    bool text(const std::string& value) noexcept {
        if (value.size() > kAbsoluteIdentifierBytes ||
            !u16(static_cast<std::uint16_t>(value.size()))) {
            return false;
        }
        return bytes(reinterpret_cast<const std::uint8_t*>(value.data()),
                     value.size());
    }

    template <typename Id>
    bool id(const Id& value) noexcept {
        return value.valid() && text(value.name_space()) && text(value.value());
    }

    [[nodiscard]] std::size_t position() const noexcept { return position_; }
    [[nodiscard]] bool overflowed() const noexcept { return overflowed_; }

private:
    bool reserve(std::size_t length) noexcept {
        if (length > capacity_ - std::min(position_, capacity_)) {
            overflowed_ = true;
            return false;
        }
        return true;
    }

    std::uint8_t* output_ = nullptr;
    std::size_t capacity_ = 0;
    std::size_t position_ = 0;
    bool overflowed_ = false;
};

class Reader {
public:
    Reader(const std::uint8_t* input, std::size_t size) noexcept
        : input_(input), size_(size), position_(kReferenceWireHeaderBytes) {}

    bool u8(std::uint8_t& value) noexcept {
        if (!available(1)) return false;
        value = input_[position_++];
        return true;
    }

    bool u16(std::uint16_t& value) noexcept {
        if (!available(2)) return false;
        value = load_u16(input_ + position_);
        position_ += 2;
        return true;
    }

    bool u32(std::uint32_t& value) noexcept {
        if (!available(4)) return false;
        value = load_u32(input_ + position_);
        position_ += 4;
        return true;
    }

    bool bytes(std::uint8_t* value, std::size_t length) noexcept {
        if (!available(length)) return false;
        if (length != 0) std::memcpy(value, input_ + position_, length);
        position_ += length;
        return true;
    }

    bool text(std::string& value) {
        std::uint16_t length = 0;
        if (!u16(length) || length == 0 || length > kAbsoluteIdentifierBytes ||
            !available(length)) {
            return false;
        }
        value.assign(reinterpret_cast<const char*>(input_ + position_), length);
        position_ += length;
        return true;
    }

    template <typename Id>
    bool id(Id& value) {
        std::string name_space;
        std::string identifier;
        if (!text(name_space) || !text(identifier)) return false;
        const auto parsed = Id::parse(std::move(name_space), std::move(identifier),
                                      kAbsoluteIdentifierBytes);
        if (!parsed) return false;
        value = *parsed;
        return true;
    }

    [[nodiscard]] std::size_t position() const noexcept { return position_; }

private:
    bool available(std::size_t length) const noexcept {
        return position_ <= size_ && length <= size_ - position_;
    }

    const std::uint8_t* input_ = nullptr;
    std::size_t size_ = 0;
    std::size_t position_ = 0;
};

bool write_envelope(Writer& writer, const SignalEnvelope& envelope) noexcept {
    if (!envelope.id.valid() || !envelope.connection_id.valid() ||
        !envelope.binding_id.valid() || !envelope.attachment_id.valid() ||
        !envelope.source.valid() || !envelope.target.valid() ||
        !envelope.source_endpoint.valid() || !envelope.target_endpoint.valid() ||
        !envelope.source_port.valid() || !envelope.target_port.valid() ||
        envelope.domain == SignalDomain::unknown ||
        envelope.intent == SignalIntent::unknown || !envelope.subtype.valid() ||
        !envelope.schema.valid() || envelope.schema_version == 0 ||
        envelope.inline_value.size() > kReferenceWireMaximumInlineBytes ||
        envelope.provenance.size() > kReferenceWireMaximumProvenance ||
        (envelope.correlation_id && !envelope.correlation_id->valid())) {
        return false;
    }
    return writer.id(envelope.id) &&
           writer.u8(envelope.correlation_id ? 1U : 0U) &&
           (!envelope.correlation_id || writer.id(*envelope.correlation_id)) &&
           writer.id(envelope.connection_id) && writer.id(envelope.binding_id) &&
           writer.id(envelope.attachment_id) && writer.id(envelope.source) &&
           writer.id(envelope.target) && writer.id(envelope.source_endpoint) &&
           writer.id(envelope.target_endpoint) && writer.id(envelope.source_port) &&
           writer.id(envelope.target_port) &&
           writer.u8(static_cast<std::uint8_t>(envelope.domain)) &&
           writer.u8(static_cast<std::uint8_t>(envelope.intent)) &&
           writer.text(envelope.subtype.value) && writer.text(envelope.schema.value) &&
           writer.u32(envelope.schema_version) &&
           writer.u8(static_cast<std::uint8_t>(envelope.freshness)) &&
           writer.u8(static_cast<std::uint8_t>(envelope.provenance.size())) &&
           std::all_of(envelope.provenance.begin(), envelope.provenance.end(),
                       [&](const auto& id) { return writer.id(id); }) &&
           writer.u16(static_cast<std::uint16_t>(envelope.inline_value.size())) &&
           writer.bytes(envelope.inline_value.data(), envelope.inline_value.size());
}

bool read_envelope(Reader& reader, SignalEnvelope& envelope) {
    std::uint8_t has_correlation = 0;
    std::uint8_t domain = 0;
    std::uint8_t intent = 0;
    std::uint8_t freshness = 0;
    std::uint8_t provenance_count = 0;
    std::uint16_t inline_size = 0;
    CorrelationId correlation;
    if (!reader.id(envelope.id) || !reader.u8(has_correlation) ||
        has_correlation > 1 ||
        (has_correlation != 0 && !reader.id(correlation)) ||
        !reader.id(envelope.connection_id) || !reader.id(envelope.binding_id) ||
        !reader.id(envelope.attachment_id) || !reader.id(envelope.source) ||
        !reader.id(envelope.target) || !reader.id(envelope.source_endpoint) ||
        !reader.id(envelope.target_endpoint) || !reader.id(envelope.source_port) ||
        !reader.id(envelope.target_port) || !reader.u8(domain) ||
        domain == 0 || domain > static_cast<std::uint8_t>(SignalDomain::security) ||
        !reader.u8(intent) || intent == 0 ||
        intent > static_cast<std::uint8_t>(SignalIntent::response) ||
        !reader.text(envelope.subtype.value) ||
        !reader.text(envelope.schema.value) || !reader.u32(envelope.schema_version) ||
        envelope.schema_version == 0 || !reader.u8(freshness) ||
        freshness > static_cast<std::uint8_t>(FreshnessCondition::unavailable) ||
        !reader.u8(provenance_count) ||
        provenance_count > kReferenceWireMaximumProvenance) {
        return false;
    }
    envelope.correlation_id = has_correlation != 0
        ? std::optional<CorrelationId>{std::move(correlation)}
        : std::nullopt;
    envelope.domain = static_cast<SignalDomain>(domain);
    envelope.intent = static_cast<SignalIntent>(intent);
    envelope.freshness = static_cast<FreshnessCondition>(freshness);
    envelope.provenance.clear();
    envelope.provenance.reserve(provenance_count);
    for (std::uint8_t index = 0; index < provenance_count; ++index) {
        EvidenceId evidence;
        if (!reader.id(evidence)) return false;
        envelope.provenance.push_back(std::move(evidence));
    }
    if (!reader.u16(inline_size) ||
        inline_size > kReferenceWireMaximumInlineBytes) {
        return false;
    }
    envelope.inline_value.resize(inline_size);
    return reader.bytes(envelope.inline_value.data(), inline_size);
}

}  // namespace

ReferenceWireResult encode_reference_signal(
    const SignalEnvelope& envelope,
    std::uint8_t* output,
    std::size_t output_capacity) noexcept {
    if (!output) return {ReferenceWireCode::invalid_argument, 0};
    if (output_capacity < kReferenceWireHeaderBytes)
        return {ReferenceWireCode::output_too_small, 0};
    try {
        Writer writer{output, std::min(output_capacity, kReferenceWireMaximumBytes)};
        if (!write_envelope(writer, envelope)) {
            const auto code = writer.overflowed()
                ? (output_capacity >= kReferenceWireMaximumBytes
                       ? ReferenceWireCode::oversized
                       : ReferenceWireCode::output_too_small)
                : ReferenceWireCode::invalid_envelope;
            return {code,
                    0};
        }
        if (writer.position() > kReferenceWireMaximumBytes)
            return {ReferenceWireCode::oversized, 0};
        store_u32(output, kMagic);
        store_u16(output + 4, kReferenceWireVersion);
        store_u16(output + 6,
                  static_cast<std::uint16_t>(kReferenceWireHeaderBytes));
        store_u32(output + 8, static_cast<std::uint32_t>(writer.position()));
        store_u32(output + 12, static_cast<std::uint32_t>(
            writer.position() - kReferenceWireHeaderBytes));
        return {ReferenceWireCode::success, writer.position()};
    } catch (...) {
        return {ReferenceWireCode::resource_exhausted, 0};
    }
}

ReferenceWireResult decode_reference_signal(
    const std::uint8_t* input,
    std::size_t input_size,
    SignalEnvelope& envelope) noexcept {
    if (!input) return {ReferenceWireCode::invalid_argument, 0};
    if (input_size < kReferenceWireHeaderBytes)
        return {ReferenceWireCode::truncated, input_size};
    if (load_u32(input) != kMagic ||
        load_u16(input + 6) != kReferenceWireHeaderBytes) {
        return {ReferenceWireCode::invalid_header, 0};
    }
    if (load_u16(input + 4) != kReferenceWireVersion)
        return {ReferenceWireCode::unsupported_version, 0};
    const auto declared_size = static_cast<std::size_t>(load_u32(input + 8));
    const auto body_size = static_cast<std::size_t>(load_u32(input + 12));
    if (declared_size > kReferenceWireMaximumBytes ||
        body_size > kReferenceWireMaximumBytes - kReferenceWireHeaderBytes) {
        return {ReferenceWireCode::oversized, declared_size};
    }
    if (declared_size < kReferenceWireHeaderBytes ||
        body_size != declared_size - kReferenceWireHeaderBytes) {
        return {ReferenceWireCode::invalid_header, 0};
    }
    if (declared_size > input_size)
        return {ReferenceWireCode::truncated, input_size};
    if (declared_size < input_size)
        return {ReferenceWireCode::trailing_data, declared_size};
    try {
        SignalEnvelope decoded;
        Reader reader{input, input_size};
        if (!read_envelope(reader, decoded))
            return {ReferenceWireCode::invalid_field, reader.position()};
        if (reader.position() != input_size)
            return {ReferenceWireCode::trailing_data, reader.position()};
        envelope = std::move(decoded);
        return {ReferenceWireCode::success, input_size};
    } catch (...) {
        return {ReferenceWireCode::resource_exhausted, 0};
    }
}

const char* to_string(ReferenceWireCode code) noexcept {
    switch (code) {
        case ReferenceWireCode::success: return "success";
        case ReferenceWireCode::invalid_argument: return "invalid_argument";
        case ReferenceWireCode::invalid_envelope: return "invalid_envelope";
        case ReferenceWireCode::output_too_small: return "output_too_small";
        case ReferenceWireCode::invalid_header: return "invalid_header";
        case ReferenceWireCode::truncated: return "truncated";
        case ReferenceWireCode::oversized: return "oversized";
        case ReferenceWireCode::unsupported_version: return "unsupported_version";
        case ReferenceWireCode::invalid_field: return "invalid_field";
        case ReferenceWireCode::trailing_data: return "trailing_data";
        case ReferenceWireCode::resource_exhausted: return "resource_exhausted";
    }
    return "invalid_field";
}

}  // namespace prometheus::core::acs
