#include "node_p01_manifest.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIELD_IDENTITY (UINT32_C(1) << 0)
#define FIELD_REVISION (UINT32_C(1) << 1)
#define FIELD_EXECUTABLE (UINT32_C(1) << 2)
#define FIELD_STAGE (UINT32_C(1) << 3)
#define FIELD_REQUIRED (UINT32_C(1) << 4)
#define FIELD_TIMEOUT (UINT32_C(1) << 5)
#define FIELD_EXPECTED (UINT32_C(1) << 6)
#define FIELD_DEPENDENCIES (UINT32_C(1) << 7)
#define FIELD_RESTART (UINT32_C(1) << 8)
#define FIELD_MAX_RESTARTS (UINT32_C(1) << 9)
#define FIELD_ARGUMENTS (UINT32_C(1) << 10)
#define FIELD_ENVIRONMENT (UINT32_C(1) << 11)
#define ALL_SERVICE_FIELDS ((UINT32_C(1) << 12) - UINT32_C(1))

static void set_result(struct node_p01_manifest_result *result,
                       enum node_p01_manifest_status status,
                       uint32_t line,
                       const char *format,
                       ...) {
    va_list arguments;

    result->status = status;
    result->line = line;
    result->detail[0] = '\0';
    va_start(arguments, format);
    (void)vsnprintf(result->detail, sizeof(result->detail), format, arguments);
    va_end(arguments);
}

static int valid_identity(const char *value) {
    size_t index;
    size_t length = strlen(value);

    if (length == 0 || length >= NODE_P01_MAX_IDENTITY_BYTES ||
        !isalnum((unsigned char)value[0])) {
        return 0;
    }
    for (index = 0; index < length; ++index) {
        unsigned char character = (unsigned char)value[index];
        if (!(islower(character) || isdigit(character) || character == '.' ||
              character == '_' || character == '-')) {
            return 0;
        }
    }
    return 1;
}

static int valid_reference(const char *value, size_t capacity) {
    size_t index;
    size_t length = strlen(value);

    if (length == 0 || length >= capacity) {
        return 0;
    }
    for (index = 0; index < length; ++index) {
        unsigned char character = (unsigned char)value[index];
        if (!(isalnum(character) || character == '.' || character == '_' ||
              character == '-' || character == ':' || character == '/')) {
            return 0;
        }
    }
    return 1;
}

static int valid_executable(const char *value) {
    size_t index;
    const char prefix[] = "/node/services/";
    size_t length = strlen(value);

    if (length <= sizeof(prefix) - 1 || length >= NODE_P01_MAX_PATH_BYTES ||
        strncmp(value, prefix, sizeof(prefix) - 1) != 0 ||
        strstr(value, "..") != NULL || strstr(value, "//") != NULL) {
        return 0;
    }
    for (index = sizeof(prefix) - 1; index < length; ++index) {
        unsigned char character = (unsigned char)value[index];
        if (!(islower(character) || isdigit(character) || character == '_' ||
              character == '-')) {
            return 0;
        }
    }
    return 1;
}

static int parse_u32(const char *value, uint32_t *parsed) {
    unsigned long number;
    char *end = NULL;

    if (value[0] == '\0' || value[0] == '-' || value[0] == '+') {
        return 0;
    }
    errno = 0;
    number = strtoul(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || number > UINT32_MAX) {
        return 0;
    }
    *parsed = (uint32_t)number;
    return 1;
}

static int parse_stage(const char *value, enum node_p01_stage *stage) {
    static const char *const names[NODE_P01_STAGE_COUNT] = {
        "early_environment",
        "platform_observation",
        "public_providers",
        "runtime_handoff",
        "terminal"
    };
    size_t index;

    for (index = 0; index < NODE_P01_STAGE_COUNT; ++index) {
        if (strcmp(value, names[index]) == 0) {
            *stage = (enum node_p01_stage)index;
            return 1;
        }
    }
    return 0;
}

static int copy_string(struct node_p01_string *target, const char *value) {
    size_t length = strlen(value);

    if (length == 0 || length >= sizeof(target->bytes)) {
        return 0;
    }
    target->length = (uint16_t)length;
    (void)memcpy(target->bytes, value, length + 1);
    return 1;
}

static int valid_argument(const char *value) {
    size_t index;
    size_t length = strlen(value);

    if (length == 0 || length >= NODE_P01_MAX_VALUE_BYTES) {
        return 0;
    }
    for (index = 0; index < length; ++index) {
        unsigned char character = (unsigned char)value[index];
        if (character < 0x20U || character == 0x7fU || character == ',') {
            return 0;
        }
    }
    return 1;
}

static int valid_environment(const char *value) {
    static const char *const allowed[] = {"LANG=C", "LC_ALL=C"};
    size_t index;

    for (index = 0; index < sizeof(allowed) / sizeof(allowed[0]); ++index) {
        if (strcmp(value, allowed[index]) == 0) {
            return 1;
        }
    }
    return 0;
}

static enum node_p01_manifest_status parse_list(
    char *value,
    char output[][NODE_P01_MAX_IDENTITY_BYTES],
    uint8_t *count,
    size_t limit,
    uint32_t line,
    struct node_p01_manifest_result *result) {
    char *cursor = value;

    *count = 0;
    if (*cursor == '\0') {
        return NODE_P01_MANIFEST_ACCEPTED;
    }
    while (cursor != NULL) {
        char *comma = strchr(cursor, ',');
        size_t index;
        if (comma != NULL) {
            *comma = '\0';
        }
        if (*count >= limit) {
            set_result(result, NODE_P01_MANIFEST_LIMIT_EXCEEDED, line,
                       "dependency count exceeds %zu", limit);
            return result->status;
        }
        if (!valid_identity(cursor)) {
            set_result(result, NODE_P01_MANIFEST_INVALID_VALUE, line,
                       "invalid dependency identity");
            return result->status;
        }
        for (index = 0; index < *count; ++index) {
            if (strcmp(output[index], cursor) == 0) {
                set_result(result, NODE_P01_MANIFEST_INVALID_VALUE, line,
                           "duplicate dependency identity");
                return result->status;
            }
        }
        (void)snprintf(output[*count], NODE_P01_MAX_IDENTITY_BYTES, "%s", cursor);
        ++(*count);
        cursor = comma == NULL ? NULL : comma + 1;
    }
    return NODE_P01_MANIFEST_ACCEPTED;
}

static enum node_p01_manifest_status parse_string_list(
    char *value,
    struct node_p01_string *output,
    uint8_t *count,
    size_t limit,
    int environment,
    uint32_t line,
    struct node_p01_manifest_result *result) {
    char *cursor = value;

    *count = 0;
    if (*cursor == '\0') {
        return NODE_P01_MANIFEST_ACCEPTED;
    }
    while (cursor != NULL) {
        char *comma = strchr(cursor, ',');
        size_t index;
        if (comma != NULL) {
            *comma = '\0';
        }
        if (*count >= limit) {
            set_result(result, NODE_P01_MANIFEST_LIMIT_EXCEEDED, line,
                       "%s count exceeds %zu",
                       environment ? "environment" : "argument", limit);
            return result->status;
        }
        if ((environment && !valid_environment(cursor)) ||
            (!environment && !valid_argument(cursor)) ||
            !copy_string(&output[*count], cursor)) {
            set_result(result, NODE_P01_MANIFEST_INVALID_VALUE, line,
                       "invalid %s value",
                       environment ? "environment" : "argument");
            return result->status;
        }
        for (index = 0; index < *count; ++index) {
            if (strcmp(output[index].bytes, cursor) == 0) {
                set_result(result, NODE_P01_MANIFEST_INVALID_VALUE, line,
                           "duplicate %s value",
                           environment ? "environment" : "argument");
                return result->status;
            }
        }
        ++(*count);
        cursor = comma == NULL ? NULL : comma + 1;
    }
    return NODE_P01_MANIFEST_ACCEPTED;
}

static int service_index(const struct node_p01_manifest *manifest,
                         const char *identity) {
    size_t index;
    for (index = 0; index < manifest->service_count; ++index) {
        if (strcmp(manifest->services[index].identity, identity) == 0) {
            return (int)index;
        }
    }
    return -1;
}

static int visit_service(const struct node_p01_manifest *manifest,
                         size_t index,
                         uint8_t *marks) {
    const struct node_p01_service *service = &manifest->services[index];
    size_t dependency_index;

    if (marks[index] == 1U) {
        return 0;
    }
    if (marks[index] == 2U) {
        return 1;
    }
    marks[index] = 1U;
    for (dependency_index = 0;
         dependency_index < service->dependency_count;
         ++dependency_index) {
        int dependency = service_index(manifest,
                                       service->dependencies[dependency_index]);
        if (dependency < 0 || !visit_service(manifest, (size_t)dependency, marks)) {
            return 0;
        }
    }
    marks[index] = 2U;
    return 1;
}

static int compare_services(const void *left, const void *right) {
    const struct node_p01_service *left_service = left;
    const struct node_p01_service *right_service = right;
    if (left_service->stage < right_service->stage) {
        return -1;
    }
    if (left_service->stage > right_service->stage) {
        return 1;
    }
    return strcmp(left_service->identity, right_service->identity);
}

static enum node_p01_manifest_status validate_manifest(
    struct node_p01_manifest *manifest,
    struct node_p01_manifest_result *result) {
    size_t index;
    uint8_t marks[NODE_P01_MAX_SERVICES] = {0};

    if (manifest->version != NODE_P01_MANIFEST_VERSION ||
        !valid_identity(manifest->identity) || manifest->service_count == 0U) {
        set_result(result, NODE_P01_MANIFEST_INVALID_VALUE, 0,
                   "invalid manifest version, identity, or empty service set");
        return result->status;
    }
    for (index = 0; index < manifest->service_count; ++index) {
        const struct node_p01_service *service = &manifest->services[index];
        size_t other;
        size_t dependency_index;
        for (other = index + 1; other < manifest->service_count; ++other) {
            if (strcmp(service->identity, manifest->services[other].identity) == 0) {
                set_result(result, NODE_P01_MANIFEST_DUPLICATE_SERVICE, 0,
                           "duplicate service identity: %s", service->identity);
                return result->status;
            }
        }
        for (dependency_index = 0;
             dependency_index < service->dependency_count;
             ++dependency_index) {
            int dependency = service_index(manifest,
                                           service->dependencies[dependency_index]);
            if (dependency < 0) {
                set_result(result, NODE_P01_MANIFEST_MISSING_DEPENDENCY, 0,
                           "missing dependency %s for %s",
                           service->dependencies[dependency_index], service->identity);
                return result->status;
            }
            if (strcmp(service->dependencies[dependency_index],
                       service->identity) == 0) {
                set_result(result, NODE_P01_MANIFEST_DEPENDENCY_CYCLE, 0,
                           "self dependency for %s", service->identity);
                return result->status;
            }
            if (manifest->services[(size_t)dependency].stage > service->stage) {
                set_result(result, NODE_P01_MANIFEST_INVALID_VALUE, 0,
                           "dependency %s is in a later stage",
                           service->dependencies[dependency_index]);
                return result->status;
            }
        }
    }
    for (index = 0; index < manifest->service_count; ++index) {
        if (!visit_service(manifest, index, marks)) {
            set_result(result, NODE_P01_MANIFEST_DEPENDENCY_CYCLE, 0,
                       "dependency cycle detected");
            return result->status;
        }
    }
    qsort(manifest->services, manifest->service_count,
          sizeof(manifest->services[0]), compare_services);
    set_result(result, NODE_P01_MANIFEST_ACCEPTED, 0,
               "manifest accepted for P01 structural scope");
    return result->status;
}

static enum node_p01_manifest_status set_service_field(
    struct node_p01_service *service,
    uint32_t *fields,
    const char *key,
    char *value,
    uint32_t line,
    struct node_p01_manifest_result *result) {
    uint32_t bit = 0;
    uint32_t number = 0;

    if (strcmp(key, "identity") == 0) bit = FIELD_IDENTITY;
    else if (strcmp(key, "revision") == 0) bit = FIELD_REVISION;
    else if (strcmp(key, "executable") == 0) bit = FIELD_EXECUTABLE;
    else if (strcmp(key, "stage") == 0) bit = FIELD_STAGE;
    else if (strcmp(key, "required") == 0) bit = FIELD_REQUIRED;
    else if (strcmp(key, "timeout_ms") == 0) bit = FIELD_TIMEOUT;
    else if (strcmp(key, "expected_result") == 0) bit = FIELD_EXPECTED;
    else if (strcmp(key, "dependencies") == 0) bit = FIELD_DEPENDENCIES;
    else if (strcmp(key, "restart_policy") == 0) bit = FIELD_RESTART;
    else if (strcmp(key, "maximum_restart_count") == 0) bit = FIELD_MAX_RESTARTS;
    else if (strcmp(key, "arguments") == 0) bit = FIELD_ARGUMENTS;
    else if (strcmp(key, "environment") == 0) bit = FIELD_ENVIRONMENT;
    else {
        set_result(result, NODE_P01_MANIFEST_UNKNOWN_FIELD, line,
                   "unknown service field: %s", key);
        return result->status;
    }
    if ((*fields & bit) != 0U) {
        set_result(result, NODE_P01_MANIFEST_DUPLICATE_FIELD, line,
                   "duplicate service field: %s", key);
        return result->status;
    }
    *fields |= bit;

    if (bit == FIELD_IDENTITY && valid_identity(value)) {
        (void)snprintf(service->identity, sizeof(service->identity), "%s", value);
    } else if (bit == FIELD_REVISION &&
               valid_reference(value, sizeof(service->revision))) {
        (void)snprintf(service->revision, sizeof(service->revision), "%s", value);
    } else if (bit == FIELD_EXECUTABLE && valid_executable(value)) {
        (void)snprintf(service->executable, sizeof(service->executable), "%s", value);
    } else if (bit == FIELD_STAGE && parse_stage(value, &service->stage)) {
        return NODE_P01_MANIFEST_ACCEPTED;
    } else if (bit == FIELD_REQUIRED &&
               (strcmp(value, "true") == 0 || strcmp(value, "false") == 0)) {
        service->required = (uint8_t)(strcmp(value, "true") == 0);
    } else if (bit == FIELD_TIMEOUT && parse_u32(value, &number) &&
               number >= NODE_P01_MIN_TIMEOUT_MS &&
               number <= NODE_P01_MAX_TIMEOUT_MS) {
        service->timeout_ms = number;
    } else if (bit == FIELD_EXPECTED && strcmp(value, "exit_zero") == 0) {
        service->expected_result = NODE_P01_EXPECT_EXIT_ZERO;
    } else if (bit == FIELD_EXPECTED && strcmp(value, "exit_nonzero") == 0) {
        service->expected_result = NODE_P01_EXPECT_EXIT_NONZERO;
    } else if (bit == FIELD_EXPECTED && strcmp(value, "timeout") == 0) {
        service->expected_result = NODE_P01_EXPECT_TIMEOUT;
    } else if (bit == FIELD_EXPECTED && strcmp(value, "signal") == 0) {
        service->expected_result = NODE_P01_EXPECT_SIGNAL;
    } else if (bit == FIELD_DEPENDENCIES) {
        return parse_list(value, service->dependencies,
                          &service->dependency_count,
                          NODE_P01_MAX_DEPENDENCIES, line, result);
    } else if (bit == FIELD_RESTART && strcmp(value, "none") == 0) {
        service->restart_policy = NODE_P01_RESTART_NONE;
    } else if (bit == FIELD_RESTART && strcmp(value, "on_failure") == 0) {
        service->restart_policy = NODE_P01_RESTART_ON_FAILURE;
    } else if (bit == FIELD_MAX_RESTARTS && parse_u32(value, &number) &&
               number <= NODE_P01_MAX_RESTARTS) {
        service->maximum_restart_count = number;
    } else if (bit == FIELD_ARGUMENTS) {
        return parse_string_list(value, service->arguments,
                                 &service->argument_count,
                                 NODE_P01_MAX_ARGUMENTS, 0, line, result);
    } else if (bit == FIELD_ENVIRONMENT) {
        return parse_string_list(value, service->environment,
                                 &service->environment_count,
                                 NODE_P01_MAX_ENVIRONMENT, 1, line, result);
    } else {
        set_result(result, NODE_P01_MANIFEST_INVALID_VALUE, line,
                   "invalid value for %s", key);
        return result->status;
    }
    return NODE_P01_MANIFEST_ACCEPTED;
}

enum node_p01_manifest_status node_p01_manifest_parse(
    const char *bytes,
    size_t length,
    struct node_p01_manifest *manifest,
    struct node_p01_manifest_result *result) {
    size_t offset = 0;
    uint32_t line_number = 0;
    uint32_t manifest_fields = 0;
    uint32_t service_fields = 0;
    int in_service = 0;
    struct node_p01_service *service = NULL;

    if (bytes == NULL || manifest == NULL || result == NULL) {
        if (result != NULL) {
            set_result(result, NODE_P01_MANIFEST_INVALID_ARGUMENT, 0,
                       "null parser argument");
        }
        return NODE_P01_MANIFEST_INVALID_ARGUMENT;
    }
    (void)memset(manifest, 0, sizeof(*manifest));
    (void)memset(result, 0, sizeof(*result));
    if (length == 0 || length > NODE_P01_MAX_MANIFEST_BYTES) {
        set_result(result, length == 0 ? NODE_P01_MANIFEST_MALFORMED
                                      : NODE_P01_MANIFEST_TOO_LARGE,
                   0, "manifest length is outside P01 bounds");
        return result->status;
    }

    while (offset < length) {
        char line[512];
        size_t line_length = 0;
        char *equals;
        char *key;
        char *value;

        ++line_number;
        while (offset + line_length < length &&
               bytes[offset + line_length] != '\n') {
            ++line_length;
        }
        if (line_length > 0 && bytes[offset + line_length - 1] == '\r') {
            --line_length;
        }
        if (line_length >= sizeof(line)) {
            set_result(result, NODE_P01_MANIFEST_LIMIT_EXCEEDED, line_number,
                       "manifest line exceeds 511 bytes");
            return result->status;
        }
        (void)memcpy(line, bytes + offset, line_length);
        line[line_length] = '\0';
        offset += line_length;
        if (offset < length && bytes[offset] == '\r') ++offset;
        if (offset < length && bytes[offset] == '\n') ++offset;

        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        if (strcmp(line, "service_begin") == 0) {
            if (in_service) {
                set_result(result, NODE_P01_MANIFEST_MALFORMED, line_number,
                           "nested service_begin");
                return result->status;
            }
            if (manifest->service_count >= NODE_P01_MAX_SERVICES) {
                set_result(result, NODE_P01_MANIFEST_LIMIT_EXCEEDED, line_number,
                           "service count exceeds %zu", NODE_P01_MAX_SERVICES);
                return result->status;
            }
            service = &manifest->services[manifest->service_count];
            (void)memset(service, 0, sizeof(*service));
            service_fields = 0;
            in_service = 1;
            continue;
        }
        if (strcmp(line, "service_end") == 0) {
            if (!in_service || service == NULL) {
                set_result(result, NODE_P01_MANIFEST_MALFORMED, line_number,
                           "service_end without service_begin");
                return result->status;
            }
            if (service_fields != ALL_SERVICE_FIELDS) {
                set_result(result, NODE_P01_MANIFEST_MISSING_FIELD, line_number,
                           "service entry is missing a required field");
                return result->status;
            }
            if ((service->restart_policy == NODE_P01_RESTART_NONE &&
                 service->maximum_restart_count != 0U) ||
                (service->restart_policy == NODE_P01_RESTART_ON_FAILURE &&
                 service->maximum_restart_count == 0U)) {
                set_result(result, NODE_P01_MANIFEST_INVALID_VALUE, line_number,
                           "restart policy and maximum count disagree");
                return result->status;
            }
            ++manifest->service_count;
            service = NULL;
            in_service = 0;
            continue;
        }
        equals = strchr(line, '=');
        if (equals == NULL || equals == line) {
            set_result(result, NODE_P01_MANIFEST_MALFORMED, line_number,
                       "expected key=value");
            return result->status;
        }
        *equals = '\0';
        key = line;
        value = equals + 1;
        if (in_service) {
            enum node_p01_manifest_status status =
                set_service_field(service, &service_fields, key, value,
                                  line_number, result);
            if (status != NODE_P01_MANIFEST_ACCEPTED) {
                return status;
            }
        } else if (strcmp(key, "manifest_version") == 0) {
            if ((manifest_fields & 1U) != 0U) {
                set_result(result, NODE_P01_MANIFEST_DUPLICATE_FIELD, line_number,
                           "duplicate manifest_version");
                return result->status;
            }
            manifest_fields |= 1U;
            if (!parse_u32(value, &manifest->version)) {
                set_result(result, NODE_P01_MANIFEST_INVALID_VALUE, line_number,
                           "invalid manifest_version");
                return result->status;
            }
        } else if (strcmp(key, "manifest_identity") == 0) {
            if ((manifest_fields & 2U) != 0U) {
                set_result(result, NODE_P01_MANIFEST_DUPLICATE_FIELD, line_number,
                           "duplicate manifest_identity");
                return result->status;
            }
            manifest_fields |= 2U;
            if (!valid_identity(value)) {
                set_result(result, NODE_P01_MANIFEST_INVALID_VALUE, line_number,
                           "invalid manifest_identity");
                return result->status;
            }
            (void)snprintf(manifest->identity, sizeof(manifest->identity), "%s", value);
        } else {
            set_result(result, NODE_P01_MANIFEST_UNKNOWN_FIELD, line_number,
                       "unknown manifest field: %s", key);
            return result->status;
        }
    }
    if (in_service) {
        set_result(result, NODE_P01_MANIFEST_MALFORMED, line_number,
                   "unterminated service entry");
        return result->status;
    }
    if (manifest_fields != 3U) {
        set_result(result, NODE_P01_MANIFEST_MISSING_FIELD, 0,
                   "manifest header is incomplete");
        return result->status;
    }
    return validate_manifest(manifest, result);
}

enum node_p01_manifest_status node_p01_manifest_load(
    const char *path,
    struct node_p01_manifest *manifest,
    struct node_p01_manifest_result *result) {
    FILE *input;
    char bytes[NODE_P01_MAX_MANIFEST_BYTES + 1];
    size_t length;

    if (path == NULL || manifest == NULL || result == NULL) {
        if (result != NULL) {
            set_result(result, NODE_P01_MANIFEST_INVALID_ARGUMENT, 0,
                       "null loader argument");
        }
        return NODE_P01_MANIFEST_INVALID_ARGUMENT;
    }
    input = fopen(path, "rb");
    if (input == NULL) {
        set_result(result, NODE_P01_MANIFEST_IO_ERROR, 0,
                   "manifest open failed: %s", strerror(errno));
        return result->status;
    }
    length = fread(bytes, 1, sizeof(bytes), input);
    if (ferror(input)) {
        (void)fclose(input);
        set_result(result, NODE_P01_MANIFEST_IO_ERROR, 0,
                   "manifest read failed");
        return result->status;
    }
    if (fclose(input) != 0) {
        set_result(result, NODE_P01_MANIFEST_IO_ERROR, 0,
                   "manifest close failed");
        return result->status;
    }
    if (length > NODE_P01_MAX_MANIFEST_BYTES) {
        set_result(result, NODE_P01_MANIFEST_TOO_LARGE, 0,
                   "manifest exceeds %zu bytes", NODE_P01_MAX_MANIFEST_BYTES);
        return result->status;
    }
    return node_p01_manifest_parse(bytes, length, manifest, result);
}

const char *node_p01_manifest_status_name(enum node_p01_manifest_status status) {
    static const char *const names[] = {
        "accepted", "invalid_argument", "too_large", "malformed",
        "unknown_field", "duplicate_field", "limit_exceeded",
        "missing_field", "invalid_value", "duplicate_service",
        "missing_dependency", "dependency_cycle", "io_error"
    };
    size_t index = (size_t)status;
    return index < sizeof(names) / sizeof(names[0]) ? names[index] : "unknown";
}

const char *node_p01_stage_name(enum node_p01_stage stage) {
    static const char *const names[NODE_P01_STAGE_COUNT] = {
        "early_environment", "platform_observation", "public_providers",
        "runtime_handoff", "terminal"
    };
    size_t index = (size_t)stage;
    return index < NODE_P01_STAGE_COUNT ? names[index] : "unknown";
}

size_t node_p01_json_escape(char *output,
                            size_t output_capacity,
                            const char *input,
                            size_t input_length) {
    static const char hexadecimal[] = "0123456789abcdef";
    size_t input_index;
    size_t output_index = 0;

    if (output == NULL || input == NULL || output_capacity == 0U) {
        return SIZE_MAX;
    }
    for (input_index = 0; input_index < input_length; ++input_index) {
        unsigned char character = (unsigned char)input[input_index];
        if (character == '"' || character == '\\') {
            if (output_index + 2U >= output_capacity) return SIZE_MAX;
            output[output_index++] = '\\';
            output[output_index++] = (char)character;
        } else if (character < 0x20U) {
            if (output_index + 6U >= output_capacity) return SIZE_MAX;
            output[output_index++] = '\\';
            output[output_index++] = 'u';
            output[output_index++] = '0';
            output[output_index++] = '0';
            output[output_index++] = hexadecimal[character >> 4U];
            output[output_index++] = hexadecimal[character & 0x0fU];
        } else {
            if (output_index + 1U >= output_capacity) return SIZE_MAX;
            output[output_index++] = (char)character;
        }
    }
    output[output_index] = '\0';
    return output_index;
}
