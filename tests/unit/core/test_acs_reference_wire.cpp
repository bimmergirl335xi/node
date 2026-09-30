#include <array>
#include <cstdlib>
#include <cstring>
#include <utility>

#include "acs_test_fixture.hpp"
#include "core/acs/acs_reference_wire.hpp"

namespace acs = prometheus::core::acs;

template <typename Id>
Id id(const char* name_space, const char* value) {
    return *Id::parse(name_space, value);
}

acs::SignalEnvelope envelope(const acs_test::FixtureIds& ids) {
    acs::SignalEnvelope value{};
    value.id = id<acs::SignalId>("signal", "public-conformance");
    value.correlation_id = id<acs::CorrelationId>("correlation", "one");
    value.connection_id = ids.connection;
    value.binding_id = id<acs::BindingId>("binding", "reference-udp");
    value.attachment_id = id<acs::AttachmentId>("attachment", "conformance");
    value.source = ids.first;
    value.target = ids.second;
    value.source_endpoint = ids.first_endpoint;
    value.target_endpoint = ids.second_endpoint;
    value.source_port = ids.first_port;
    value.target_port = ids.second_port;
    value.domain = acs::SignalDomain::operational;
    value.intent = acs::SignalIntent::observation;
    value.subtype = {"public.transport.conformance"};
    value.schema = {"public.transport.conformance.v1"};
    value.schema_version = 1;
    value.freshness = acs::FreshnessCondition::current;
    value.provenance = {ids.evidence};
    value.inline_value = {0x4e, 0x4f, 0x44, 0x45};
    return value;
}

int main() {
    static_assert(noexcept(acs::encode_reference_signal(
        std::declval<const acs::SignalEnvelope&>(), nullptr, 0)));
    static_assert(noexcept(acs::decode_reference_signal(
        nullptr, 0, std::declval<acs::SignalEnvelope&>())));

    acs_test::FixtureIds ids{};
    const auto original = envelope(ids);
    acs::SignalEnvelope decoded{};
    if (acs::encode_reference_signal(original, nullptr, 0).code !=
            acs::ReferenceWireCode::invalid_argument ||
        acs::decode_reference_signal(nullptr, 0, decoded).code !=
            acs::ReferenceWireCode::invalid_argument) {
        return EXIT_FAILURE;
    }
    std::array<std::uint8_t, acs::kReferenceWireMaximumBytes> first{};
    std::array<std::uint8_t, acs::kReferenceWireMaximumBytes> second{};
    const auto encoded = acs::encode_reference_signal(
        original, first.data(), first.size());
    const auto encoded_again = acs::encode_reference_signal(
        original, second.data(), second.size());
    if (!encoded.ok() || encoded.bytes <= acs::kReferenceWireHeaderBytes ||
        encoded.bytes != encoded_again.bytes ||
        std::memcmp(first.data(), second.data(), encoded.bytes) != 0) {
        return EXIT_FAILURE;
    }

    const auto decoded_result = acs::decode_reference_signal(
        first.data(), encoded.bytes, decoded);
    if (!decoded_result.ok() || decoded.id != original.id ||
        decoded.correlation_id != original.correlation_id ||
        decoded.connection_id != original.connection_id ||
        decoded.binding_id != original.binding_id ||
        decoded.attachment_id != original.attachment_id ||
        decoded.source != original.source || decoded.target != original.target ||
        decoded.source_endpoint != original.source_endpoint ||
        decoded.target_endpoint != original.target_endpoint ||
        decoded.source_port != original.source_port ||
        decoded.target_port != original.target_port ||
        decoded.domain != original.domain || decoded.intent != original.intent ||
        decoded.subtype.value != original.subtype.value ||
        decoded.schema.value != original.schema.value ||
        decoded.schema_version != original.schema_version ||
        decoded.freshness != original.freshness ||
        decoded.provenance != original.provenance ||
        decoded.inline_value != original.inline_value) {
        return EXIT_FAILURE;
    }

    if (acs::encode_reference_signal(original, first.data(), 8).code !=
        acs::ReferenceWireCode::output_too_small) {
        return EXIT_FAILURE;
    }
    auto too_large = original;
    too_large.inline_value.resize(acs::kReferenceWireMaximumInlineBytes + 1);
    if (acs::encode_reference_signal(too_large, first.data(), first.size()).code !=
        acs::ReferenceWireCode::invalid_envelope) {
        return EXIT_FAILURE;
    }
    if (acs::decode_reference_signal(first.data(), 8, decoded).code !=
        acs::ReferenceWireCode::truncated) {
        return EXIT_FAILURE;
    }

    auto malformed = first;
    malformed[0] = 0;
    if (acs::decode_reference_signal(malformed.data(), encoded.bytes, decoded).code !=
        acs::ReferenceWireCode::invalid_header) {
        return EXIT_FAILURE;
    }
    malformed = first;
    malformed[5] = 2;
    if (acs::decode_reference_signal(malformed.data(), encoded.bytes, decoded).code !=
        acs::ReferenceWireCode::unsupported_version) {
        return EXIT_FAILURE;
    }
    malformed = first;
    malformed[8] = 0;
    malformed[9] = 1;
    malformed[10] = 0;
    malformed[11] = 0;
    if (acs::decode_reference_signal(malformed.data(), encoded.bytes, decoded).code !=
        acs::ReferenceWireCode::oversized) {
        return EXIT_FAILURE;
    }
    if (acs::decode_reference_signal(first.data(), encoded.bytes - 1, decoded).code !=
        acs::ReferenceWireCode::truncated) {
        return EXIT_FAILURE;
    }
    if (acs::decode_reference_signal(first.data(), encoded.bytes + 1, decoded).code !=
        acs::ReferenceWireCode::trailing_data) {
        return EXIT_FAILURE;
    }
    return std::strcmp(acs::to_string(acs::ReferenceWireCode::oversized),
                       "oversized") == 0
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}
