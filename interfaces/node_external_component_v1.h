#ifndef NODE_EXTERNAL_COMPONENT_V1_H
#define NODE_EXTERNAL_COMPONENT_V1_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NODE_EXTERNAL_COMPONENT_ABI_MAJOR_V1 UINT16_C(1)
#define NODE_EXTERNAL_COMPONENT_ABI_MINOR_V1 UINT16_C(0)

#define NODE_COMPONENT_ID_CAPACITY_V1 UINT32_C(64)
#define NODE_COMPONENT_REF_CAPACITY_V1 UINT32_C(128)
#define NODE_COMPONENT_ENTRY_CAPACITY_V1 UINT32_C(192)
#define NODE_COMPONENT_DETAIL_CAPACITY_V1 UINT32_C(160)
#define NODE_COMPONENT_CAPABILITY_LIMIT_V1 UINT32_C(8)
#define NODE_COMPONENT_DEPENDENCY_LIMIT_V1 UINT32_C(8)

enum node_component_class_v1 {
    NODE_COMPONENT_CLASS_UNSPECIFIED_V1 = 0,
    NODE_COMPONENT_CLASS_RUNTIME_V1 = 1,
    NODE_COMPONENT_CLASS_SERVICE_V1 = 2,
    NODE_COMPONENT_CLASS_PROVIDER_V1 = 3,
    NODE_COMPONENT_CLASS_DIAGNOSTIC_V1 = 4
};

enum node_component_launch_kind_v1 {
    NODE_COMPONENT_LAUNCH_NONE_V1 = 0,
    NODE_COMPONENT_LAUNCH_EXECUTABLE_V1 = 1,
    NODE_COMPONENT_LAUNCH_ENTRYPOINT_V1 = 2
};

enum node_component_declaration_state_v1 {
    NODE_COMPONENT_DECLARATION_NOT_EVALUATED_V1 = 0,
    NODE_COMPONENT_DECLARATION_NOT_SUPPLIED_V1 = 1,
    NODE_COMPONENT_DECLARATION_STRUCTURALLY_VALID_V1 = 2,
    NODE_COMPONENT_DECLARATION_MALFORMED_V1 = 3,
    NODE_COMPONENT_DECLARATION_ABI_INCOMPATIBLE_V1 = 4,
    NODE_COMPONENT_DECLARATION_UNSUPPORTED_V1 = 5,
    NODE_COMPONENT_DECLARATION_INDETERMINATE_V1 = 6
};

enum node_component_failure_v1 {
    NODE_COMPONENT_FAILURE_NONE_V1 = 0,
    NODE_COMPONENT_FAILURE_INVALID_ARGUMENT_V1 = 1,
    NODE_COMPONENT_FAILURE_IO_UNAVAILABLE_V1 = 2,
    NODE_COMPONENT_FAILURE_MALFORMED_V1 = 3,
    NODE_COMPONENT_FAILURE_INCOMPATIBLE_ABI_V1 = 4,
    NODE_COMPONENT_FAILURE_LIMIT_EXCEEDED_V1 = 5,
    NODE_COMPONENT_FAILURE_MISSING_REQUIRED_V1 = 6,
    NODE_COMPONENT_FAILURE_UNSUPPORTED_CLASS_V1 = 7,
    NODE_COMPONENT_FAILURE_UNSUPPORTED_LAUNCH_V1 = 8
};

struct node_component_id_v1 {
    uint16_t length;
    uint16_t reserved;
    char bytes[NODE_COMPONENT_ID_CAPACITY_V1];
};

struct node_component_ref_v1 {
    uint16_t length;
    uint16_t reserved;
    char bytes[NODE_COMPONENT_REF_CAPACITY_V1];
};

struct node_component_entry_v1 {
    uint16_t length;
    uint16_t reserved;
    char bytes[NODE_COMPONENT_ENTRY_CAPACITY_V1];
};

struct node_component_declaration_v1 {
    uint32_t struct_size;
    uint16_t abi_major;
    uint16_t abi_minor;
    uint32_t component_class;
    uint32_t launch_kind;
    struct node_component_id_v1 component_id;
    struct node_component_ref_v1 declaration_revision_ref;
    struct node_component_entry_v1 entry;
    uint32_t required_capability_count;
    struct node_component_ref_v1
        required_capabilities[NODE_COMPONENT_CAPABILITY_LIMIT_V1];
    uint32_t provided_capability_count;
    struct node_component_ref_v1
        provided_capabilities[NODE_COMPONENT_CAPABILITY_LIMIT_V1];
    uint32_t dependency_count;
    struct node_component_ref_v1
        dependencies[NODE_COMPONENT_DEPENDENCY_LIMIT_V1];
    struct node_component_ref_v1 provenance_ref;
    struct node_component_ref_v1 integrity_ref;
};

struct node_component_declaration_result_v1 {
    uint32_t struct_size;
    uint32_t declaration_state;
    uint32_t failure;
    struct node_component_id_v1 component_id;
    struct node_component_ref_v1 declaration_revision_ref;
    uint16_t detail_length;
    uint16_t reserved;
    char detail[NODE_COMPONENT_DETAIL_CAPACITY_V1];
};

#ifdef __cplusplus
}
#endif

#endif
