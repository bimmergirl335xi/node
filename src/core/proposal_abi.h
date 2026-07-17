#ifndef SRC_CORE_PROPOSAL_ABI_H
#define SRC_CORE_PROPOSAL_ABI_H

#include <stdint.h>

#define PROMETHEUS_PROPOSAL_ABI_VERSION 1U
#define PROMETHEUS_PROPOSAL_ABI_MIN_VERSION 1U

#define PROMETHEUS_PROPOSAL_KIND_ADAPTIVE_MUTATION 1U
#define PROMETHEUS_PROPOSAL_KIND_ARCHITECTURE 2U

#define PROMETHEUS_PROPOSAL_ABI_STATUS_SUCCESS 0
#define PROMETHEUS_PROPOSAL_ABI_STATUS_INVALID_ARGUMENT 1
#define PROMETHEUS_PROPOSAL_ABI_STATUS_UNSUPPORTED_VERSION 2
#define PROMETHEUS_PROPOSAL_ABI_STATUS_MALFORMED_INPUT 3
#define PROMETHEUS_PROPOSAL_ABI_STATUS_LIMIT_EXCEEDED 4
#define PROMETHEUS_PROPOSAL_ABI_STATUS_BUFFER_TOO_SMALL 5
#define PROMETHEUS_PROPOSAL_ABI_STATUS_INTERNAL_ERROR 6

#define PROMETHEUS_PROPOSAL_VALIDATION_VALID 0U
#define PROMETHEUS_PROPOSAL_VALIDATION_INVALID 1U
#define PROMETHEUS_PROPOSAL_VALIDATION_UNSUPPORTED 2U
#define PROMETHEUS_PROPOSAL_VALIDATION_LIMIT_EXCEEDED 3U
#define PROMETHEUS_PROPOSAL_VALIDATION_INTERNAL_ERROR 4U

#define PROMETHEUS_PROPOSAL_FIELD_IDENTIFIER 1U
#define PROMETHEUS_PROPOSAL_FIELD_SOURCE_IDENTITY 2U
#define PROMETHEUS_PROPOSAL_FIELD_SOURCE_CATEGORY 3U
#define PROMETHEUS_PROPOSAL_FIELD_AUTHORITY_REFERENCE 4U
#define PROMETHEUS_PROPOSAL_FIELD_EXPECTED_REVISION 5U
#define PROMETHEUS_PROPOSAL_FIELD_TARGET_IDENTITY 6U
#define PROMETHEUS_PROPOSAL_FIELD_OPERATION 7U
#define PROMETHEUS_PROPOSAL_FIELD_CORRELATION_ID 8U
#define PROMETHEUS_PROPOSAL_FIELD_EVIDENCE_REFERENCE 9U
#define PROMETHEUS_PROPOSAL_FIELD_EXPLANATION 10U

#define PROMETHEUS_PROPOSAL_FIELD_ARCH_COMPONENT_TYPE 20U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_DISPLAY_NAME 21U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_ENABLED 22U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_DEPENDENCY_IDS 23U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_EDGE_TYPE 24U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_EDGE_SOURCE 25U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_EDGE_TARGET 26U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_WEIGHT 27U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_ADAPTER_IDS 28U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_BACKEND_KINDS 29U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_TENANCY 30U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_MEMORY_BYTES 31U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_EXECUTION_LANES 32U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_MEM_CONTRACT_IDS 33U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_ACS_CONTRACT_IDS 34U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_OLD_DEPENDENCY 35U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_NEW_DEPENDENCY 36U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_REQUIRED_TYPE 37U
#define PROMETHEUS_PROPOSAL_FIELD_ARCH_DEPENDENCY_TYPES 38U

#define PROMETHEUS_PROPOSAL_FIELD_MUTATION_VALUE_TYPE 40U
#define PROMETHEUS_PROPOSAL_FIELD_MUTATION_OPERAND 41U

#define PROMETHEUS_PROPOSAL_FIELD_TYPE_IDENTIFIER 1U
#define PROMETHEUS_PROPOSAL_FIELD_TYPE_TEXT 2U
#define PROMETHEUS_PROPOSAL_FIELD_TYPE_U32 3U
#define PROMETHEUS_PROPOSAL_FIELD_TYPE_U64 4U
#define PROMETHEUS_PROPOSAL_FIELD_TYPE_F64 5U
#define PROMETHEUS_PROPOSAL_FIELD_TYPE_BYTES 6U
#define PROMETHEUS_PROPOSAL_FIELD_TYPE_IDENTIFIER_LIST 7U
#define PROMETHEUS_PROPOSAL_FIELD_TYPE_U32_LIST 8U

#define PROMETHEUS_PROPOSAL_ABI_MAX_INPUT_BYTES 65536U
#define PROMETHEUS_PROPOSAL_ABI_MAX_FIELDS 64U
#define PROMETHEUS_PROPOSAL_ABI_MAX_MESSAGE_BYTES 128U

#ifdef __cplusplus
#define PROMETHEUS_PROPOSAL_ABI_NOEXCEPT noexcept
extern "C" {
#else
#define PROMETHEUS_PROPOSAL_ABI_NOEXCEPT
#endif

typedef int32_t prometheus_proposal_abi_status;

typedef struct prometheus_proposal_abi_capabilities {
    uint32_t structure_size;
    uint32_t abi_version;
    uint32_t minimum_abi_version;
    uint32_t supported_proposal_kinds;
    uint64_t maximum_input_bytes;
    uint64_t maximum_output_bytes;
    uint32_t maximum_fields;
    uint32_t flags;
} prometheus_proposal_abi_capabilities;

typedef struct prometheus_proposal_abi_validation_result {
    uint32_t structure_size;
    uint32_t abi_version;
    uint32_t proposal_kind;
    uint32_t validation_code;
    uint64_t consumed_input_bytes;
    uint32_t message_length;
    uint32_t reserved;
    char message[PROMETHEUS_PROPOSAL_ABI_MAX_MESSAGE_BYTES];
} prometheus_proposal_abi_validation_result;

/*
 * Canonical input is little-endian TLV:
 *   magic[4] = "NPA1", u16 ABI version, u16 proposal kind, u32 field count;
 *   repeated fields: u16 field id, u16 field type, u32 byte length, bytes.
 *
 * Identifier-list bytes contain u32 count followed by repeated u16 length and
 * ASCII identifier bytes. U32-list bytes contain u32 count and u32 values.
 * Unknown fields, duplicate fields, trailing bytes, executable callbacks, and
 * opaque extension payloads are rejected.
 *
 * All input and output memory is caller owned. The implementation retains no
 * pointer. For output sizing, pass output_buffer=NULL and output_capacity=0;
 * required_output_size receives the exact required size.
 */
prometheus_proposal_abi_status prometheus_proposal_abi_query_capabilities(
    uint32_t requested_abi_version,
    prometheus_proposal_abi_capabilities* capabilities,
    uint64_t capabilities_size) PROMETHEUS_PROPOSAL_ABI_NOEXCEPT;

prometheus_proposal_abi_status prometheus_proposal_abi_validate(
    uint32_t requested_abi_version,
    uint32_t proposal_kind,
    const uint8_t* input_buffer,
    uint64_t input_length,
    uint8_t* output_buffer,
    uint64_t output_capacity,
    uint64_t* required_output_size) PROMETHEUS_PROPOSAL_ABI_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#undef PROMETHEUS_PROPOSAL_ABI_NOEXCEPT

#endif  // SRC_CORE_PROPOSAL_ABI_H
