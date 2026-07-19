#ifndef NODE_P01_MANIFEST_H
#define NODE_P01_MANIFEST_H

#include <stddef.h>
#include <stdint.h>

#define NODE_P01_MANIFEST_VERSION UINT32_C(1)
#define NODE_P01_MAX_MANIFEST_BYTES ((size_t)32768)
#define NODE_P01_MAX_SERVICES ((size_t)16)
#define NODE_P01_MAX_DEPENDENCIES ((size_t)8)
#define NODE_P01_MAX_ARGUMENTS ((size_t)8)
#define NODE_P01_MAX_ENVIRONMENT ((size_t)4)
#define NODE_P01_MAX_IDENTITY_BYTES ((size_t)48)
#define NODE_P01_MAX_REVISION_BYTES ((size_t)96)
#define NODE_P01_MAX_PATH_BYTES ((size_t)128)
#define NODE_P01_MAX_VALUE_BYTES ((size_t)128)
#define NODE_P01_MAX_DETAIL_BYTES ((size_t)192)
#define NODE_P01_MIN_TIMEOUT_MS UINT32_C(10)
#define NODE_P01_MAX_TIMEOUT_MS UINT32_C(30000)
#define NODE_P01_MAX_RESTARTS UINT32_C(2)

enum node_p01_stage {
    NODE_P01_STAGE_EARLY_ENVIRONMENT = 0,
    NODE_P01_STAGE_PLATFORM_OBSERVATION = 1,
    NODE_P01_STAGE_PUBLIC_PROVIDERS = 2,
    NODE_P01_STAGE_RUNTIME_HANDOFF = 3,
    NODE_P01_STAGE_TERMINAL = 4,
    NODE_P01_STAGE_COUNT = 5
};

enum node_p01_expected_result {
    NODE_P01_EXPECT_EXIT_ZERO = 0,
    NODE_P01_EXPECT_EXIT_NONZERO = 1,
    NODE_P01_EXPECT_TIMEOUT = 2,
    NODE_P01_EXPECT_SIGNAL = 3
};

enum node_p01_restart_policy {
    NODE_P01_RESTART_NONE = 0,
    NODE_P01_RESTART_ON_FAILURE = 1
};

enum node_p01_manifest_status {
    NODE_P01_MANIFEST_ACCEPTED = 0,
    NODE_P01_MANIFEST_INVALID_ARGUMENT = 1,
    NODE_P01_MANIFEST_TOO_LARGE = 2,
    NODE_P01_MANIFEST_MALFORMED = 3,
    NODE_P01_MANIFEST_UNKNOWN_FIELD = 4,
    NODE_P01_MANIFEST_DUPLICATE_FIELD = 5,
    NODE_P01_MANIFEST_LIMIT_EXCEEDED = 6,
    NODE_P01_MANIFEST_MISSING_FIELD = 7,
    NODE_P01_MANIFEST_INVALID_VALUE = 8,
    NODE_P01_MANIFEST_DUPLICATE_SERVICE = 9,
    NODE_P01_MANIFEST_MISSING_DEPENDENCY = 10,
    NODE_P01_MANIFEST_DEPENDENCY_CYCLE = 11,
    NODE_P01_MANIFEST_IO_ERROR = 12
};

struct node_p01_string {
    uint16_t length;
    char bytes[NODE_P01_MAX_VALUE_BYTES];
};

struct node_p01_service {
    char identity[NODE_P01_MAX_IDENTITY_BYTES];
    char revision[NODE_P01_MAX_REVISION_BYTES];
    char executable[NODE_P01_MAX_PATH_BYTES];
    enum node_p01_stage stage;
    uint8_t required;
    uint32_t timeout_ms;
    enum node_p01_expected_result expected_result;
    enum node_p01_restart_policy restart_policy;
    uint32_t maximum_restart_count;
    uint8_t dependency_count;
    char dependencies[NODE_P01_MAX_DEPENDENCIES][NODE_P01_MAX_IDENTITY_BYTES];
    uint8_t argument_count;
    struct node_p01_string arguments[NODE_P01_MAX_ARGUMENTS];
    uint8_t environment_count;
    struct node_p01_string environment[NODE_P01_MAX_ENVIRONMENT];
};

struct node_p01_manifest {
    uint32_t version;
    char identity[NODE_P01_MAX_IDENTITY_BYTES];
    uint8_t service_count;
    struct node_p01_service services[NODE_P01_MAX_SERVICES];
};

struct node_p01_manifest_result {
    enum node_p01_manifest_status status;
    uint32_t line;
    char detail[NODE_P01_MAX_DETAIL_BYTES];
};

enum node_p01_manifest_status node_p01_manifest_parse(
    const char *bytes,
    size_t length,
    struct node_p01_manifest *manifest,
    struct node_p01_manifest_result *result);

enum node_p01_manifest_status node_p01_manifest_load(
    const char *path,
    struct node_p01_manifest *manifest,
    struct node_p01_manifest_result *result);

const char *node_p01_manifest_status_name(enum node_p01_manifest_status status);
const char *node_p01_stage_name(enum node_p01_stage stage);
size_t node_p01_json_escape(char *output,
                            size_t output_capacity,
                            const char *input,
                            size_t input_length);

#endif
