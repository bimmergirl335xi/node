#include <array>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <future>
#include <string>
#include <utility>
#include <vector>

#include "core/proposal_abi.h"

namespace {

void append_u16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
}

void append_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    for (std::size_t index = 0; index < 4; ++index) {
        bytes.push_back(
            static_cast<std::uint8_t>((value >> (index * 8U)) & 0xffU));
    }
}

void append_u64(std::vector<std::uint8_t>& bytes, std::uint64_t value) {
    for (std::size_t index = 0; index < 8; ++index) {
        bytes.push_back(
            static_cast<std::uint8_t>((value >> (index * 8U)) & 0xffU));
    }
}

struct WireBuilder {
    explicit WireBuilder(std::uint16_t kind) {
        bytes = {'N', 'P', 'A', '1'};
        append_u16(bytes, PROMETHEUS_PROPOSAL_ABI_VERSION);
        append_u16(bytes, kind);
        append_u32(bytes, 0);
    }

    void field(
        std::uint16_t id,
        std::uint16_t type,
        const std::vector<std::uint8_t>& value) {
        append_u16(bytes, id);
        append_u16(bytes, type);
        append_u32(bytes, static_cast<std::uint32_t>(value.size()));
        bytes.insert(bytes.end(), value.begin(), value.end());
        ++field_count;
    }

    void identifier(std::uint16_t id, const std::string& value) {
        field(
            id,
            PROMETHEUS_PROPOSAL_FIELD_TYPE_IDENTIFIER,
            std::vector<std::uint8_t>(value.begin(), value.end()));
    }

    void text(std::uint16_t id, const std::string& value) {
        field(
            id,
            PROMETHEUS_PROPOSAL_FIELD_TYPE_TEXT,
            std::vector<std::uint8_t>(value.begin(), value.end()));
    }

    void u32(std::uint16_t id, std::uint32_t value) {
        std::vector<std::uint8_t> encoded{};
        append_u32(encoded, value);
        field(id, PROMETHEUS_PROPOSAL_FIELD_TYPE_U32, encoded);
    }

    void u64(std::uint16_t id, std::uint64_t value) {
        std::vector<std::uint8_t> encoded{};
        append_u64(encoded, value);
        field(id, PROMETHEUS_PROPOSAL_FIELD_TYPE_U64, encoded);
    }

    void f64(std::uint16_t id, double value) {
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(value));
        std::vector<std::uint8_t> encoded{};
        append_u64(encoded, bits);
        field(id, PROMETHEUS_PROPOSAL_FIELD_TYPE_F64, encoded);
    }

    void raw(std::uint16_t id, const std::vector<std::uint8_t>& value) {
        field(id, PROMETHEUS_PROPOSAL_FIELD_TYPE_BYTES, value);
    }

    [[nodiscard]] std::vector<std::uint8_t> finish() {
        bytes[8] = static_cast<std::uint8_t>(field_count & 0xffU);
        bytes[9] = static_cast<std::uint8_t>((field_count >> 8U) & 0xffU);
        bytes[10] = static_cast<std::uint8_t>((field_count >> 16U) & 0xffU);
        bytes[11] = static_cast<std::uint8_t>((field_count >> 24U) & 0xffU);
        return bytes;
    }

    std::vector<std::uint8_t> bytes{};
    std::uint32_t field_count = 0;
};

void common_fields(
    WireBuilder& builder,
    const std::string& proposal_id,
    std::uint32_t operation,
    std::uint64_t revision = 1) {
    builder.identifier(PROMETHEUS_PROPOSAL_FIELD_IDENTIFIER, proposal_id);
    builder.identifier(
        PROMETHEUS_PROPOSAL_FIELD_SOURCE_IDENTITY, "planner.native");
    builder.u32(PROMETHEUS_PROPOSAL_FIELD_SOURCE_CATEGORY, 0);
    builder.identifier(
        PROMETHEUS_PROPOSAL_FIELD_AUTHORITY_REFERENCE,
        "evidence.proposal");
    builder.u64(PROMETHEUS_PROPOSAL_FIELD_EXPECTED_REVISION, revision);
    builder.identifier(
        PROMETHEUS_PROPOSAL_FIELD_TARGET_IDENTITY,
        "component.target");
    builder.u32(PROMETHEUS_PROPOSAL_FIELD_OPERATION, operation);
}

[[nodiscard]] std::vector<std::uint8_t> valid_mutation() {
    WireBuilder builder{PROMETHEUS_PROPOSAL_KIND_ADAPTIVE_MUTATION};
    common_fields(builder, "proposal.mutation", 0);
    builder.u32(PROMETHEUS_PROPOSAL_FIELD_MUTATION_VALUE_TYPE, 1);
    std::vector<std::uint8_t> operand{};
    append_u64(operand, 42);
    builder.raw(PROMETHEUS_PROPOSAL_FIELD_MUTATION_OPERAND, operand);
    return builder.finish();
}

[[nodiscard]] std::vector<std::uint8_t> valid_architecture() {
    WireBuilder builder{PROMETHEUS_PROPOSAL_KIND_ARCHITECTURE};
    common_fields(builder, "proposal.architecture", 7, 0);
    builder.f64(PROMETHEUS_PROPOSAL_FIELD_ARCH_WEIGHT, 0.75);
    return builder.finish();
}

[[nodiscard]] bool validate_success(
    std::uint32_t kind,
    const std::vector<std::uint8_t>& input) {
    std::uint64_t required = 0;
    if (prometheus_proposal_abi_validate(
            PROMETHEUS_PROPOSAL_ABI_VERSION,
            kind,
            input.data(),
            input.size(),
            nullptr,
            0,
            &required) != PROMETHEUS_PROPOSAL_ABI_STATUS_BUFFER_TOO_SMALL ||
        required != sizeof(prometheus_proposal_abi_validation_result)) {
        return false;
    }
    std::vector<std::uint8_t> output(required);
    if (prometheus_proposal_abi_validate(
            PROMETHEUS_PROPOSAL_ABI_VERSION,
            kind,
            input.data(),
            input.size(),
            output.data(),
            output.size(),
            &required) != PROMETHEUS_PROPOSAL_ABI_STATUS_SUCCESS) {
        return false;
    }
    prometheus_proposal_abi_validation_result result{};
    std::memcpy(&result, output.data(), sizeof(result));
    return result.abi_version == PROMETHEUS_PROPOSAL_ABI_VERSION &&
           result.proposal_kind == kind &&
           result.validation_code == PROMETHEUS_PROPOSAL_VALIDATION_VALID &&
           result.message_length > 0 &&
           result.consumed_input_bytes == input.size();
}

[[nodiscard]] bool test_capability_query() {
    prometheus_proposal_abi_capabilities capabilities{};
    return prometheus_proposal_abi_query_capabilities(
               PROMETHEUS_PROPOSAL_ABI_VERSION,
               &capabilities,
               sizeof(capabilities)) ==
               PROMETHEUS_PROPOSAL_ABI_STATUS_SUCCESS &&
           capabilities.structure_size == sizeof(capabilities) &&
           capabilities.abi_version == PROMETHEUS_PROPOSAL_ABI_VERSION &&
           capabilities.maximum_input_bytes ==
               PROMETHEUS_PROPOSAL_ABI_MAX_INPUT_BYTES &&
           capabilities.maximum_output_bytes ==
               sizeof(prometheus_proposal_abi_validation_result) &&
           prometheus_proposal_abi_query_capabilities(
               99, &capabilities, sizeof(capabilities)) ==
               PROMETHEUS_PROPOSAL_ABI_STATUS_UNSUPPORTED_VERSION &&
           prometheus_proposal_abi_query_capabilities(
               PROMETHEUS_PROPOSAL_ABI_VERSION,
               nullptr,
               sizeof(capabilities)) ==
               PROMETHEUS_PROPOSAL_ABI_STATUS_INVALID_ARGUMENT;
}

[[nodiscard]] bool test_valid_proposals_and_bounded_output() {
    const auto mutation = valid_mutation();
    const auto architecture = valid_architecture();
    std::uint64_t required = 0;
    std::array<std::uint8_t, 8> too_small{};
    return validate_success(
               PROMETHEUS_PROPOSAL_KIND_ADAPTIVE_MUTATION, mutation) &&
           validate_success(
               PROMETHEUS_PROPOSAL_KIND_ARCHITECTURE, architecture) &&
           prometheus_proposal_abi_validate(
               PROMETHEUS_PROPOSAL_ABI_VERSION,
               PROMETHEUS_PROPOSAL_KIND_ARCHITECTURE,
               architecture.data(),
               architecture.size(),
               too_small.data(),
               too_small.size(),
               &required) == PROMETHEUS_PROPOSAL_ABI_STATUS_BUFFER_TOO_SMALL &&
           required == sizeof(prometheus_proposal_abi_validation_result);
}

[[nodiscard]] bool test_pointer_length_and_malformed_rejection() {
    const auto valid = valid_architecture();
    std::uint64_t required = 0;
    std::array<std::uint8_t, sizeof(prometheus_proposal_abi_validation_result)>
        output{};
    if (prometheus_proposal_abi_validate(
            PROMETHEUS_PROPOSAL_ABI_VERSION,
            PROMETHEUS_PROPOSAL_KIND_ARCHITECTURE,
            nullptr,
            1,
            output.data(),
            output.size(),
            &required) != PROMETHEUS_PROPOSAL_ABI_STATUS_INVALID_ARGUMENT ||
        prometheus_proposal_abi_validate(
            PROMETHEUS_PROPOSAL_ABI_VERSION,
            PROMETHEUS_PROPOSAL_KIND_ARCHITECTURE,
            valid.data(),
            valid.size(),
            nullptr,
            1,
            &required) != PROMETHEUS_PROPOSAL_ABI_STATUS_INVALID_ARGUMENT ||
        prometheus_proposal_abi_validate(
            PROMETHEUS_PROPOSAL_ABI_VERSION,
            PROMETHEUS_PROPOSAL_KIND_ARCHITECTURE,
            valid.data(),
            valid.size(),
            output.data(),
            output.size(),
            nullptr) != PROMETHEUS_PROPOSAL_ABI_STATUS_INVALID_ARGUMENT) {
        return false;
    }

    auto malformed = valid;
    malformed[0] = 'X';
    if (prometheus_proposal_abi_validate(
            PROMETHEUS_PROPOSAL_ABI_VERSION,
            PROMETHEUS_PROPOSAL_KIND_ARCHITECTURE,
            malformed.data(),
            malformed.size(),
            output.data(),
            output.size(),
            &required) != PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT) {
        return false;
    }
    prometheus_proposal_abi_validation_result result{};
    std::memcpy(&result, output.data(), sizeof(result));
    if (result.validation_code != PROMETHEUS_PROPOSAL_VALIDATION_INVALID) {
        return false;
    }

    const std::uint8_t one_byte = 0;
    return prometheus_proposal_abi_validate(
               PROMETHEUS_PROPOSAL_ABI_VERSION,
               PROMETHEUS_PROPOSAL_KIND_ARCHITECTURE,
               &one_byte,
               PROMETHEUS_PROPOSAL_ABI_MAX_INPUT_BYTES + 1ULL,
               output.data(),
               output.size(),
               &required) == PROMETHEUS_PROPOSAL_ABI_STATUS_LIMIT_EXCEEDED;
}

[[nodiscard]] bool test_no_exceptions_and_concurrent_calls() {
    const auto input = valid_architecture();
    std::vector<std::future<bool>> workers{};
    for (std::size_t worker = 0; worker < 4; ++worker) {
        workers.push_back(std::async(
            std::launch::async,
            [&input]() {
                for (std::size_t iteration = 0; iteration < 100; ++iteration) {
                    if (!validate_success(
                            PROMETHEUS_PROPOSAL_KIND_ARCHITECTURE, input)) {
                        return false;
                    }
                    std::array<std::uint8_t, 16> malformed{};
                    std::uint64_t required = 0;
                    std::array<
                        std::uint8_t,
                        sizeof(prometheus_proposal_abi_validation_result)>
                        output{};
                    const auto status = prometheus_proposal_abi_validate(
                        PROMETHEUS_PROPOSAL_ABI_VERSION,
                        PROMETHEUS_PROPOSAL_KIND_ARCHITECTURE,
                        malformed.data(),
                        malformed.size(),
                        output.data(),
                        output.size(),
                        &required);
                    if (status !=
                        PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT) {
                        return false;
                    }
                }
                return true;
            }));
    }
    for (auto& worker : workers) {
        if (!worker.get()) {
            return false;
        }
    }
    return true;
}

}  // namespace

int main() {
    return test_capability_query() &&
                   test_valid_proposals_and_bounded_output() &&
                   test_pointer_length_and_malformed_rejection() &&
                   test_no_exceptions_and_concurrent_calls()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
