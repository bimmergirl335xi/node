#include "proposal_abi.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

namespace {

inline constexpr std::uint32_t kMaximumIdentifierBytes = 128;
inline constexpr std::uint32_t kMaximumTextBytes = 1024;
inline constexpr std::uint32_t kMaximumListEntries = 64;
inline constexpr std::uint32_t kMaximumVectorElements = 4096;
inline constexpr std::uint32_t kCapabilitySizeQuery = 1U << 0U;
inline constexpr std::uint32_t kCapabilityCallerOwnedBuffers = 1U << 1U;
inline constexpr std::uint32_t kCapabilityExceptionContainment = 1U << 2U;

static_assert(std::is_standard_layout_v<prometheus_proposal_abi_capabilities>);
static_assert(
    std::is_standard_layout_v<prometheus_proposal_abi_validation_result>);

struct FieldView {
    std::uint16_t id = 0;
    std::uint16_t type = 0;
    std::uint32_t length = 0;
    const std::uint8_t* data = nullptr;
};

struct ParseResult {
    prometheus_proposal_abi_status status =
        PROMETHEUS_PROPOSAL_ABI_STATUS_SUCCESS;
    std::uint32_t validation_code = PROMETHEUS_PROPOSAL_VALIDATION_VALID;
    const char* message = "Canonical proposal encoding is valid";
    std::uint64_t consumed = 0;
};

[[nodiscard]] std::uint16_t read_u16(const std::uint8_t* bytes) noexcept {
    return static_cast<std::uint16_t>(bytes[0]) |
           static_cast<std::uint16_t>(
               static_cast<std::uint16_t>(bytes[1]) << 8U);
}

[[nodiscard]] std::uint32_t read_u32(const std::uint8_t* bytes) noexcept {
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[3]) << 24U);
}

[[nodiscard]] std::uint64_t read_u64(const std::uint8_t* bytes) noexcept {
    std::uint64_t result = 0;
    for (std::size_t index = 0; index < 8; ++index) {
        result |= static_cast<std::uint64_t>(bytes[index]) << (index * 8U);
    }
    return result;
}

[[nodiscard]] double read_f64(const std::uint8_t* bytes) noexcept {
    const std::uint64_t bits = read_u64(bytes);
    double value = 0.0;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

[[nodiscard]] bool valid_identifier_bytes(
    const std::uint8_t* bytes,
    std::uint32_t length) noexcept {
    if (bytes == nullptr || length == 0 || length > kMaximumIdentifierBytes) {
        return false;
    }
    const auto is_alphanumeric = [](std::uint8_t byte) {
        return (byte >= static_cast<std::uint8_t>('a') &&
                byte <= static_cast<std::uint8_t>('z')) ||
               (byte >= static_cast<std::uint8_t>('A') &&
                byte <= static_cast<std::uint8_t>('Z')) ||
               (byte >= static_cast<std::uint8_t>('0') &&
                byte <= static_cast<std::uint8_t>('9'));
    };
    if (!is_alphanumeric(bytes[0])) {
        return false;
    }
    for (std::uint32_t index = 0; index < length; ++index) {
        const std::uint8_t byte = bytes[index];
        if (!is_alphanumeric(byte) && byte != static_cast<std::uint8_t>('.') &&
            byte != static_cast<std::uint8_t>('_') &&
            byte != static_cast<std::uint8_t>('-')) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool valid_text_bytes(
    const std::uint8_t* bytes,
    std::uint32_t length,
    bool required) noexcept {
    if ((required && length == 0) || length > kMaximumTextBytes ||
        (length != 0 && bytes == nullptr)) {
        return false;
    }
    for (std::uint32_t index = 0; index < length; ++index) {
        if (bytes[index] < 0x20U || bytes[index] > 0x7eU) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] const FieldView* find_field(
    const std::array<FieldView, PROMETHEUS_PROPOSAL_ABI_MAX_FIELDS>& fields,
    std::uint32_t field_count,
    std::uint16_t id) noexcept {
    for (std::uint32_t index = 0; index < field_count; ++index) {
        if (fields[index].id == id) {
            return &fields[index];
        }
    }
    return nullptr;
}

[[nodiscard]] bool field_type_and_length(
    const FieldView* field,
    std::uint16_t type,
    std::uint32_t length) noexcept {
    return field != nullptr && field->type == type && field->length == length;
}

[[nodiscard]] bool valid_identifier_field(const FieldView* field) noexcept {
    return field != nullptr &&
           field->type == PROMETHEUS_PROPOSAL_FIELD_TYPE_IDENTIFIER &&
           valid_identifier_bytes(field->data, field->length);
}

[[nodiscard]] bool valid_identifier_list(
    const FieldView* field,
    std::uint32_t* count_out = nullptr) noexcept {
    if (field == nullptr) {
        if (count_out != nullptr) {
            *count_out = 0;
        }
        return true;
    }
    if (field->type != PROMETHEUS_PROPOSAL_FIELD_TYPE_IDENTIFIER_LIST ||
        field->length < 4) {
        return false;
    }
    const std::uint32_t count = read_u32(field->data);
    if (count > kMaximumListEntries) {
        return false;
    }
    std::array<const std::uint8_t*, kMaximumListEntries> starts{};
    std::array<std::uint16_t, kMaximumListEntries> lengths{};
    std::uint32_t offset = 4;
    for (std::uint32_t index = 0; index < count; ++index) {
        if (offset > field->length || field->length - offset < 2) {
            return false;
        }
        const std::uint16_t length = read_u16(field->data + offset);
        offset += 2;
        if (offset > field->length || field->length - offset < length ||
            !valid_identifier_bytes(field->data + offset, length)) {
            return false;
        }
        starts[index] = field->data + offset;
        lengths[index] = length;
        for (std::uint32_t other = 0; other < index; ++other) {
            if (lengths[other] == length &&
                std::memcmp(starts[other], starts[index], length) == 0) {
                return false;
            }
        }
        offset += length;
    }
    if (offset != field->length) {
        return false;
    }
    if (count_out != nullptr) {
        *count_out = count;
    }
    return true;
}

[[nodiscard]] bool valid_u32_list(
    const FieldView* field,
    std::uint32_t maximum_value,
    std::uint32_t* count_out = nullptr) noexcept {
    if (field == nullptr) {
        if (count_out != nullptr) {
            *count_out = 0;
        }
        return true;
    }
    if (field->type != PROMETHEUS_PROPOSAL_FIELD_TYPE_U32_LIST ||
        field->length < 4) {
        return false;
    }
    const std::uint32_t count = read_u32(field->data);
    if (count > kMaximumListEntries ||
        field->length != 4U + count * 4U) {
        return false;
    }
    for (std::uint32_t index = 0; index < count; ++index) {
        const std::uint32_t value = read_u32(field->data + 4U + index * 4U);
        if (value > maximum_value) {
            return false;
        }
        for (std::uint32_t other = 0; other < index; ++other) {
            if (read_u32(field->data + 4U + other * 4U) == value) {
                return false;
            }
        }
    }
    if (count_out != nullptr) {
        *count_out = count;
    }
    return true;
}

[[nodiscard]] bool common_field(std::uint16_t id) noexcept {
    return id >= PROMETHEUS_PROPOSAL_FIELD_IDENTIFIER &&
           id <= PROMETHEUS_PROPOSAL_FIELD_EXPLANATION;
}

[[nodiscard]] bool architecture_field(std::uint16_t id) noexcept {
    return id >= PROMETHEUS_PROPOSAL_FIELD_ARCH_COMPONENT_TYPE &&
           id <= PROMETHEUS_PROPOSAL_FIELD_ARCH_DEPENDENCY_TYPES;
}

[[nodiscard]] ParseResult invalid(
    prometheus_proposal_abi_status status,
    std::uint32_t code,
    const char* message,
    std::uint64_t consumed) noexcept {
    return {status, code, message, consumed};
}

[[nodiscard]] bool allowed_architecture_payload_field(
    std::uint32_t operation,
    std::uint16_t id) noexcept {
    if (common_field(id)) {
        return true;
    }
    switch (operation) {
        case 0:
            return (id >= 20U && id <= 23U) ||
                   (id >= 27U && id <= 34U) || id == 38U;
        case 1:
        case 2:
        case 3:
        case 6:
            return false;
        case 4:
            return id >= 35U && id <= 37U;
        case 5:
            return id >= 24U && id <= 27U;
        case 7:
            return id == 27U;
        case 8:
            return id >= 28U && id <= 32U;
        default:
            return false;
    }
}

[[nodiscard]] ParseResult validate_mutation_fields(
    const std::array<FieldView, PROMETHEUS_PROPOSAL_ABI_MAX_FIELDS>& fields,
    std::uint32_t count,
    std::uint64_t consumed) noexcept {
    for (std::uint32_t index = 0; index < count; ++index) {
        if (!common_field(fields[index].id) &&
            fields[index].id !=
                PROMETHEUS_PROPOSAL_FIELD_MUTATION_VALUE_TYPE &&
            fields[index].id != PROMETHEUS_PROPOSAL_FIELD_MUTATION_OPERAND) {
            return invalid(
                PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT,
                PROMETHEUS_PROPOSAL_VALIDATION_INVALID,
                "Unknown mutation field is not permitted",
                consumed);
        }
    }
    const FieldView* operation = find_field(
        fields, count, PROMETHEUS_PROPOSAL_FIELD_OPERATION);
    const FieldView* value_type = find_field(
        fields, count, PROMETHEUS_PROPOSAL_FIELD_MUTATION_VALUE_TYPE);
    const FieldView* operand = find_field(
        fields, count, PROMETHEUS_PROPOSAL_FIELD_MUTATION_OPERAND);
    if (!field_type_and_length(
            operation, PROMETHEUS_PROPOSAL_FIELD_TYPE_U32, 4) ||
        read_u32(operation->data) > 3U ||
        !field_type_and_length(
            value_type, PROMETHEUS_PROPOSAL_FIELD_TYPE_U32, 4) ||
        read_u32(value_type->data) > 7U || operand == nullptr ||
        operand->type != PROMETHEUS_PROPOSAL_FIELD_TYPE_BYTES) {
        return invalid(
            PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT,
            PROMETHEUS_PROPOSAL_VALIDATION_INVALID,
            "Mutation operation, value type, or operand is malformed",
            consumed);
    }
    const std::uint32_t type = read_u32(value_type->data);
    bool valid_operand = false;
    if (type == 0U) {
        valid_operand = operand->length == 1U && operand->data[0] <= 1U;
    } else if (type == 1U || type == 2U) {
        valid_operand = operand->length == 8U;
    } else if (type == 3U) {
        valid_operand = operand->length == 8U &&
                        std::isfinite(read_f64(operand->data));
    } else if (type == 4U) {
        valid_operand = valid_text_bytes(
            operand->data, operand->length, false);
    } else if (operand->length >= 4U) {
        const std::uint32_t elements = read_u32(operand->data);
        valid_operand = elements <= kMaximumVectorElements &&
                        operand->length == 4U + elements * 8U;
        if (valid_operand && type == 7U) {
            for (std::uint32_t index = 0; index < elements; ++index) {
                if (!std::isfinite(
                        read_f64(operand->data + 4U + index * 8U))) {
                    valid_operand = false;
                    break;
                }
            }
        }
    }
    if (!valid_operand) {
        return invalid(
            PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT,
            PROMETHEUS_PROPOSAL_VALIDATION_INVALID,
            "Mutation operand encoding is invalid or non-finite",
            consumed);
    }
    return {PROMETHEUS_PROPOSAL_ABI_STATUS_SUCCESS,
            PROMETHEUS_PROPOSAL_VALIDATION_VALID,
            "Canonical mutation proposal is valid for native validation",
            consumed};
}

[[nodiscard]] ParseResult validate_architecture_fields(
    const std::array<FieldView, PROMETHEUS_PROPOSAL_ABI_MAX_FIELDS>& fields,
    std::uint32_t count,
    std::uint64_t consumed) noexcept {
    const FieldView* operation = find_field(
        fields, count, PROMETHEUS_PROPOSAL_FIELD_OPERATION);
    if (!field_type_and_length(
            operation, PROMETHEUS_PROPOSAL_FIELD_TYPE_U32, 4)) {
        return invalid(
            PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT,
            PROMETHEUS_PROPOSAL_VALIDATION_INVALID,
            "Architecture operation is missing or malformed",
            consumed);
    }
    const std::uint32_t operation_value = read_u32(operation->data);
    if (operation_value > 8U) {
        return invalid(
            PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT,
            PROMETHEUS_PROPOSAL_VALIDATION_UNSUPPORTED,
            "Architecture operation is unsupported",
            consumed);
    }
    for (std::uint32_t index = 0; index < count; ++index) {
        if ((!common_field(fields[index].id) &&
             !architecture_field(fields[index].id)) ||
            !allowed_architecture_payload_field(
                operation_value, fields[index].id)) {
            return invalid(
                PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT,
                PROMETHEUS_PROPOSAL_VALIDATION_INVALID,
                "Architecture payload contains an incompatible field",
                consumed);
        }
    }

    const auto u32_value = [&fields, count](std::uint16_t id,
                                            std::uint32_t maximum,
                                            bool required) {
        const FieldView* field = find_field(fields, count, id);
        return (!required && field == nullptr) ||
               (field_type_and_length(
                    field, PROMETHEUS_PROPOSAL_FIELD_TYPE_U32, 4) &&
                read_u32(field->data) <= maximum);
    };
    const auto u64_value = [&fields, count](std::uint16_t id, bool required) {
        const FieldView* field = find_field(fields, count, id);
        return (!required && field == nullptr) ||
               field_type_and_length(
                   field, PROMETHEUS_PROPOSAL_FIELD_TYPE_U64, 8);
    };
    const auto finite_weight = [&fields, count]() {
        const FieldView* field = find_field(
            fields, count, PROMETHEUS_PROPOSAL_FIELD_ARCH_WEIGHT);
        if (!field_type_and_length(
                field, PROMETHEUS_PROPOSAL_FIELD_TYPE_F64, 8)) {
            return false;
        }
        const double value = read_f64(field->data);
        return std::isfinite(value) && value >= 0.0 && value <= 1.0;
    };

    bool valid_payload = false;
    switch (operation_value) {
        case 0: {
            const FieldView* display = find_field(
                fields,
                count,
                PROMETHEUS_PROPOSAL_FIELD_ARCH_DISPLAY_NAME);
            std::uint32_t dependency_count = 0;
            std::uint32_t dependency_type_count = 0;
            valid_payload =
                u32_value(
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_COMPONENT_TYPE, 7U, true) &&
                display != nullptr &&
                display->type == PROMETHEUS_PROPOSAL_FIELD_TYPE_TEXT &&
                valid_text_bytes(display->data, display->length, true) &&
                u32_value(
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_ENABLED, 1U, true) &&
                finite_weight() &&
                valid_identifier_list(
                    find_field(
                        fields,
                        count,
                        PROMETHEUS_PROPOSAL_FIELD_ARCH_DEPENDENCY_IDS),
                    &dependency_count) &&
                valid_u32_list(
                    find_field(
                        fields,
                        count,
                        PROMETHEUS_PROPOSAL_FIELD_ARCH_DEPENDENCY_TYPES),
                    7U,
                    &dependency_type_count) &&
                dependency_count == dependency_type_count &&
                valid_identifier_list(find_field(
                    fields,
                    count,
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_ADAPTER_IDS)) &&
                valid_u32_list(
                    find_field(
                        fields,
                        count,
                        PROMETHEUS_PROPOSAL_FIELD_ARCH_BACKEND_KINDS),
                    5U) &&
                u32_value(
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_TENANCY, 4U, false) &&
                u64_value(
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_MEMORY_BYTES, false) &&
                u64_value(
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_EXECUTION_LANES, false) &&
                valid_identifier_list(find_field(
                    fields,
                    count,
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_MEM_CONTRACT_IDS)) &&
                valid_identifier_list(find_field(
                    fields,
                    count,
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_ACS_CONTRACT_IDS));
            break;
        }
        case 1:
        case 2:
        case 3:
        case 6:
            valid_payload = true;
            break;
        case 4:
            valid_payload =
                valid_identifier_field(find_field(
                    fields,
                    count,
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_OLD_DEPENDENCY)) &&
                valid_identifier_field(find_field(
                    fields,
                    count,
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_NEW_DEPENDENCY)) &&
                u32_value(
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_REQUIRED_TYPE, 7U, true);
            break;
        case 5:
            valid_payload =
                u32_value(
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_EDGE_TYPE, 3U, true) &&
                valid_identifier_field(find_field(
                    fields,
                    count,
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_EDGE_SOURCE)) &&
                valid_identifier_field(find_field(
                    fields,
                    count,
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_EDGE_TARGET)) &&
                finite_weight();
            break;
        case 7:
            valid_payload = finite_weight();
            break;
        case 8: {
            const FieldView* lanes = find_field(
                fields,
                count,
                PROMETHEUS_PROPOSAL_FIELD_ARCH_EXECUTION_LANES);
            valid_payload =
                valid_identifier_list(find_field(
                    fields,
                    count,
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_ADAPTER_IDS)) &&
                valid_u32_list(
                    find_field(
                        fields,
                        count,
                        PROMETHEUS_PROPOSAL_FIELD_ARCH_BACKEND_KINDS),
                    5U) &&
                u32_value(
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_TENANCY, 4U, true) &&
                u64_value(
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_MEMORY_BYTES, false) &&
                u64_value(
                    PROMETHEUS_PROPOSAL_FIELD_ARCH_EXECUTION_LANES, false) &&
                (lanes == nullptr || read_u64(lanes->data) <= 65536U);
            break;
        }
        default:
            valid_payload = false;
            break;
    }
    if (!valid_payload) {
        return invalid(
            PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT,
            PROMETHEUS_PROPOSAL_VALIDATION_INVALID,
            "Architecture operation payload is malformed",
            consumed);
    }
    return {PROMETHEUS_PROPOSAL_ABI_STATUS_SUCCESS,
            PROMETHEUS_PROPOSAL_VALIDATION_VALID,
            "Canonical architecture proposal is valid for shadow validation",
            consumed};
}

[[nodiscard]] ParseResult parse_and_validate(
    std::uint32_t requested_version,
    std::uint32_t requested_kind,
    const std::uint8_t* input,
    std::uint64_t input_length) noexcept {
    if (requested_version != PROMETHEUS_PROPOSAL_ABI_VERSION) {
        return invalid(
            PROMETHEUS_PROPOSAL_ABI_STATUS_UNSUPPORTED_VERSION,
            PROMETHEUS_PROPOSAL_VALIDATION_UNSUPPORTED,
            "Requested proposal ABI version is unsupported",
            0);
    }
    if (input_length > PROMETHEUS_PROPOSAL_ABI_MAX_INPUT_BYTES) {
        return invalid(
            PROMETHEUS_PROPOSAL_ABI_STATUS_LIMIT_EXCEEDED,
            PROMETHEUS_PROPOSAL_VALIDATION_LIMIT_EXCEEDED,
            "Proposal input exceeds the ABI byte limit",
            0);
    }
    if (input_length < 12U || input[0] != static_cast<std::uint8_t>('N') ||
        input[1] != static_cast<std::uint8_t>('P') ||
        input[2] != static_cast<std::uint8_t>('A') ||
        input[3] != static_cast<std::uint8_t>('1')) {
        return invalid(
            PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT,
            PROMETHEUS_PROPOSAL_VALIDATION_INVALID,
            "Proposal ABI header is malformed",
            0);
    }
    const std::uint16_t encoded_version = read_u16(input + 4);
    const std::uint16_t encoded_kind = read_u16(input + 6);
    const std::uint32_t field_count = read_u32(input + 8);
    if (encoded_version != requested_version || encoded_kind != requested_kind ||
        (requested_kind != PROMETHEUS_PROPOSAL_KIND_ADAPTIVE_MUTATION &&
         requested_kind != PROMETHEUS_PROPOSAL_KIND_ARCHITECTURE)) {
        return invalid(
            PROMETHEUS_PROPOSAL_ABI_STATUS_UNSUPPORTED_VERSION,
            PROMETHEUS_PROPOSAL_VALIDATION_UNSUPPORTED,
            "Encoded proposal version or kind is unsupported",
            12);
    }
    if (field_count == 0 ||
        field_count > PROMETHEUS_PROPOSAL_ABI_MAX_FIELDS) {
        return invalid(
            PROMETHEUS_PROPOSAL_ABI_STATUS_LIMIT_EXCEEDED,
            PROMETHEUS_PROPOSAL_VALIDATION_LIMIT_EXCEEDED,
            "Proposal field count exceeds ABI limits",
            12);
    }

    std::array<FieldView, PROMETHEUS_PROPOSAL_ABI_MAX_FIELDS> fields{};
    std::uint64_t offset = 12;
    std::uint16_t previous_id = 0;
    for (std::uint32_t index = 0; index < field_count; ++index) {
        if (offset > input_length || input_length - offset < 8U) {
            return invalid(
                PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT,
                PROMETHEUS_PROPOSAL_VALIDATION_INVALID,
                "Proposal field header is truncated",
                offset);
        }
        FieldView& field = fields[index];
        field.id = read_u16(input + offset);
        field.type = read_u16(input + offset + 2U);
        field.length = read_u32(input + offset + 4U);
        offset += 8U;
        if (field.id <= previous_id || field.type == 0U || field.type > 8U ||
            field.length > PROMETHEUS_PROPOSAL_ABI_MAX_INPUT_BYTES ||
            offset > input_length || input_length - offset < field.length) {
            return invalid(
                PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT,
                PROMETHEUS_PROPOSAL_VALIDATION_INVALID,
                "Proposal fields are non-canonical, duplicated, or truncated",
                offset);
        }
        field.data = input + offset;
        offset += field.length;
        previous_id = field.id;
    }
    if (offset != input_length) {
        return invalid(
            PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT,
            PROMETHEUS_PROPOSAL_VALIDATION_INVALID,
            "Proposal contains trailing bytes",
            offset);
    }

    const FieldView* proposal_id = find_field(
        fields, field_count, PROMETHEUS_PROPOSAL_FIELD_IDENTIFIER);
    const FieldView* source_id = find_field(
        fields, field_count, PROMETHEUS_PROPOSAL_FIELD_SOURCE_IDENTITY);
    const FieldView* source_category = find_field(
        fields, field_count, PROMETHEUS_PROPOSAL_FIELD_SOURCE_CATEGORY);
    const FieldView* authority = find_field(
        fields, field_count, PROMETHEUS_PROPOSAL_FIELD_AUTHORITY_REFERENCE);
    const FieldView* expected = find_field(
        fields, field_count, PROMETHEUS_PROPOSAL_FIELD_EXPECTED_REVISION);
    const FieldView* target = find_field(
        fields, field_count, PROMETHEUS_PROPOSAL_FIELD_TARGET_IDENTITY);
    if (!valid_identifier_field(proposal_id) ||
        !valid_identifier_field(source_id) ||
        !field_type_and_length(
            source_category, PROMETHEUS_PROPOSAL_FIELD_TYPE_U32, 4) ||
        read_u32(source_category->data) > 4U ||
        !valid_identifier_field(authority) ||
        !field_type_and_length(
            expected, PROMETHEUS_PROPOSAL_FIELD_TYPE_U64, 8) ||
        !valid_identifier_field(target)) {
        return invalid(
            PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT,
            PROMETHEUS_PROPOSAL_VALIDATION_INVALID,
            "Required proposal identity or revision fields are malformed",
            offset);
    }
    if (requested_kind == PROMETHEUS_PROPOSAL_KIND_ADAPTIVE_MUTATION &&
        read_u64(expected->data) == 0U) {
        return invalid(
            PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT,
            PROMETHEUS_PROPOSAL_VALIDATION_INVALID,
            "Mutation expected variable version must be non-zero",
            offset);
    }
    const FieldView* correlation = find_field(
        fields, field_count, PROMETHEUS_PROPOSAL_FIELD_CORRELATION_ID);
    const FieldView* evidence = find_field(
        fields, field_count, PROMETHEUS_PROPOSAL_FIELD_EVIDENCE_REFERENCE);
    const FieldView* explanation = find_field(
        fields, field_count, PROMETHEUS_PROPOSAL_FIELD_EXPLANATION);
    if ((correlation != nullptr && !valid_identifier_field(correlation)) ||
        (evidence != nullptr && !valid_identifier_field(evidence)) ||
        (explanation != nullptr &&
         (explanation->type != PROMETHEUS_PROPOSAL_FIELD_TYPE_TEXT ||
          !valid_text_bytes(
              explanation->data, explanation->length, false)))) {
        return invalid(
            PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT,
            PROMETHEUS_PROPOSAL_VALIDATION_INVALID,
            "Optional proposal evidence or explanation is malformed",
            offset);
    }

    return requested_kind == PROMETHEUS_PROPOSAL_KIND_ADAPTIVE_MUTATION
               ? validate_mutation_fields(fields, field_count, offset)
               : validate_architecture_fields(fields, field_count, offset);
}

void set_result_message(
    prometheus_proposal_abi_validation_result& result,
    const char* message) noexcept {
    const std::size_t available =
        PROMETHEUS_PROPOSAL_ABI_MAX_MESSAGE_BYTES - 1U;
    const std::size_t length = std::min(std::strlen(message), available);
    std::memcpy(result.message, message, length);
    result.message[length] = '\0';
    result.message_length = static_cast<std::uint32_t>(length);
}

}  // namespace

extern "C" prometheus_proposal_abi_status
prometheus_proposal_abi_query_capabilities(
    std::uint32_t requested_abi_version,
    prometheus_proposal_abi_capabilities* capabilities,
    std::uint64_t capabilities_size) noexcept {
    try {
        if (requested_abi_version != PROMETHEUS_PROPOSAL_ABI_VERSION) {
            return PROMETHEUS_PROPOSAL_ABI_STATUS_UNSUPPORTED_VERSION;
        }
        if (capabilities == nullptr ||
            capabilities_size < sizeof(*capabilities)) {
            return PROMETHEUS_PROPOSAL_ABI_STATUS_INVALID_ARGUMENT;
        }
        prometheus_proposal_abi_capabilities result{};
        result.structure_size = sizeof(result);
        result.abi_version = PROMETHEUS_PROPOSAL_ABI_VERSION;
        result.minimum_abi_version = PROMETHEUS_PROPOSAL_ABI_MIN_VERSION;
        result.supported_proposal_kinds =
            (1U << PROMETHEUS_PROPOSAL_KIND_ADAPTIVE_MUTATION) |
            (1U << PROMETHEUS_PROPOSAL_KIND_ARCHITECTURE);
        result.maximum_input_bytes = PROMETHEUS_PROPOSAL_ABI_MAX_INPUT_BYTES;
        result.maximum_output_bytes =
            sizeof(prometheus_proposal_abi_validation_result);
        result.maximum_fields = PROMETHEUS_PROPOSAL_ABI_MAX_FIELDS;
        result.flags = kCapabilitySizeQuery | kCapabilityCallerOwnedBuffers |
                       kCapabilityExceptionContainment;
        std::memcpy(capabilities, &result, sizeof(result));
        return PROMETHEUS_PROPOSAL_ABI_STATUS_SUCCESS;
    } catch (...) {
        return PROMETHEUS_PROPOSAL_ABI_STATUS_INTERNAL_ERROR;
    }
}

extern "C" prometheus_proposal_abi_status prometheus_proposal_abi_validate(
    std::uint32_t requested_abi_version,
    std::uint32_t proposal_kind,
    const std::uint8_t* input_buffer,
    std::uint64_t input_length,
    std::uint8_t* output_buffer,
    std::uint64_t output_capacity,
    std::uint64_t* required_output_size) noexcept {
    try {
        if (required_output_size == nullptr ||
            (input_buffer == nullptr && input_length != 0U) ||
            (output_buffer == nullptr && output_capacity != 0U)) {
            return PROMETHEUS_PROPOSAL_ABI_STATUS_INVALID_ARGUMENT;
        }
        *required_output_size =
            sizeof(prometheus_proposal_abi_validation_result);
        if (input_buffer == nullptr) {
            return PROMETHEUS_PROPOSAL_ABI_STATUS_INVALID_ARGUMENT;
        }
        const ParseResult parsed = parse_and_validate(
            requested_abi_version,
            proposal_kind,
            input_buffer,
            input_length);
        if (output_buffer == nullptr ||
            output_capacity < sizeof(prometheus_proposal_abi_validation_result)) {
            return PROMETHEUS_PROPOSAL_ABI_STATUS_BUFFER_TOO_SMALL;
        }

        prometheus_proposal_abi_validation_result result{};
        result.structure_size = sizeof(result);
        result.abi_version = PROMETHEUS_PROPOSAL_ABI_VERSION;
        result.proposal_kind = proposal_kind;
        result.validation_code = parsed.validation_code;
        result.consumed_input_bytes = parsed.consumed;
        set_result_message(result, parsed.message);
        std::memcpy(output_buffer, &result, sizeof(result));
        return parsed.status;
    } catch (...) {
        if (required_output_size != nullptr) {
            *required_output_size =
                sizeof(prometheus_proposal_abi_validation_result);
        }
        return PROMETHEUS_PROPOSAL_ABI_STATUS_INTERNAL_ERROR;
    }
}
