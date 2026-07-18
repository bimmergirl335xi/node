#include "external_component_manifest.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NODE_MANIFEST_LINE_CAPACITY 512U
#define NODE_MANIFEST_VERSION_V1 1U

struct manifest_presence {
    unsigned int manifest_version;
    unsigned int component_id;
    unsigned int abi_major;
    unsigned int abi_minor;
    unsigned int component_class;
    unsigned int launch_kind;
    unsigned int entry;
    unsigned int provenance_ref;
    unsigned int integrity_ref;
};

static void result_set(
    struct node_component_lifecycle_result_v1 *result,
    uint32_t lifecycle,
    uint32_t outcome,
    uint32_t failure,
    const char *detail)
{
    size_t length = 0U;

    memset(result, 0, sizeof(*result));
    result->struct_size = (uint32_t)sizeof(*result);
    result->lifecycle = lifecycle;
    result->outcome = outcome;
    result->failure = failure;
    if (detail != NULL) {
        length = strlen(detail);
        if (length >= sizeof(result->detail)) {
            length = sizeof(result->detail) - 1U;
        }
        memcpy(result->detail, detail, length);
        result->detail[length] = '\0';
        result->detail_length = (uint16_t)length;
    }
}

static int fail(
    struct node_component_lifecycle_result_v1 *result,
    uint32_t failure,
    const char *detail)
{
    uint32_t outcome = NODE_COMPONENT_OUTCOME_FAILED_V1;
    if (failure == NODE_COMPONENT_FAILURE_IO_UNAVAILABLE_V1) {
        outcome = NODE_COMPONENT_OUTCOME_UNAVAILABLE_V1;
    } else if (failure == NODE_COMPONENT_FAILURE_INCOMPATIBLE_ABI_V1 ||
               failure == NODE_COMPONENT_FAILURE_UNSUPPORTED_CLASS_V1 ||
               failure == NODE_COMPONENT_FAILURE_UNSUPPORTED_LAUNCH_V1) {
        outcome = NODE_COMPONENT_OUTCOME_UNSUPPORTED_V1;
    }
    result_set(
        result,
        NODE_COMPONENT_LIFECYCLE_FAILED_V1,
        outcome,
        failure,
        detail);
    return -1;
}

static int parse_u16(const char *value, uint16_t *output)
{
    char *end = NULL;
    unsigned long parsed;

    if (value == NULL || *value == '\0' || isspace((unsigned char)*value)) {
        return -1;
    }
    errno = 0;
    parsed = strtoul(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed > UINT16_MAX) {
        return -1;
    }
    *output = (uint16_t)parsed;
    return 0;
}

static int copy_text(
    char *destination,
    size_t capacity,
    uint16_t *length_output,
    const char *value,
    int identity)
{
    size_t index;
    size_t length;

    if (value == NULL || *value == '\0') {
        return -1;
    }
    length = strlen(value);
    if (length >= capacity || length > UINT16_MAX) {
        return -2;
    }
    for (index = 0U; index < length; ++index) {
        unsigned char character = (unsigned char)value[index];
        if (identity != 0) {
            if (!(isalnum(character) || character == '.' || character == '_' ||
                  character == '-')) {
                return -1;
            }
        } else if (!isprint(character) || isspace(character)) {
            return -1;
        }
    }
    memcpy(destination, value, length + 1U);
    *length_output = (uint16_t)length;
    return 0;
}

static int set_unique(unsigned int *presence)
{
    if (*presence != 0U) {
        return -1;
    }
    *presence = 1U;
    return 0;
}

static int add_reference(
    struct node_component_ref_v1 *references,
    uint32_t *count,
    uint32_t limit,
    const char *value)
{
    int status;

    if (*count >= limit) {
        return -2;
    }
    status = copy_text(
        references[*count].bytes,
        sizeof(references[*count].bytes),
        &references[*count].length,
        value,
        0);
    if (status != 0) {
        return status;
    }
    *count += 1U;
    return 0;
}

static int parse_class(const char *value, uint32_t *component_class)
{
    if (strcmp(value, "runtime") == 0) {
        *component_class = NODE_COMPONENT_CLASS_RUNTIME_V1;
    } else if (strcmp(value, "service") == 0) {
        *component_class = NODE_COMPONENT_CLASS_SERVICE_V1;
    } else if (strcmp(value, "provider") == 0) {
        *component_class = NODE_COMPONENT_CLASS_PROVIDER_V1;
    } else if (strcmp(value, "diagnostic") == 0) {
        *component_class = NODE_COMPONENT_CLASS_DIAGNOSTIC_V1;
    } else {
        return -1;
    }
    return 0;
}

static int parse_launch(const char *value, uint32_t *launch_kind)
{
    if (strcmp(value, "none") == 0) {
        *launch_kind = NODE_COMPONENT_LAUNCH_NONE_V1;
    } else if (strcmp(value, "executable") == 0) {
        *launch_kind = NODE_COMPONENT_LAUNCH_EXECUTABLE_V1;
    } else if (strcmp(value, "entrypoint") == 0) {
        *launch_kind = NODE_COMPONENT_LAUNCH_ENTRYPOINT_V1;
    } else {
        return -1;
    }
    return 0;
}

static int parse_line(
    char *line,
    struct node_component_declaration_v1 *declaration,
    struct manifest_presence *presence,
    struct node_component_lifecycle_result_v1 *result)
{
    char *separator;
    char *key;
    char *value;
    int status;
    uint16_t number;

    separator = strchr(line, '=');
    if (separator == NULL || separator == line || separator[1] == '\0' ||
        strchr(separator + 1, '=') != NULL) {
        return fail(result, NODE_COMPONENT_FAILURE_MALFORMED_V1, "expected one key=value pair");
    }
    *separator = '\0';
    key = line;
    value = separator + 1;

    if (strcmp(key, "manifest_version") == 0) {
        if (set_unique(&presence->manifest_version) != 0 ||
            parse_u16(value, &number) != 0) {
            return fail(result, NODE_COMPONENT_FAILURE_MALFORMED_V1, "invalid manifest_version");
        }
        if (number != NODE_MANIFEST_VERSION_V1) {
            return fail(result, NODE_COMPONENT_FAILURE_INCOMPATIBLE_ABI_V1, "unsupported manifest_version");
        }
        return 0;
    }
    if (strcmp(key, "component_id") == 0) {
        if (set_unique(&presence->component_id) != 0) {
            return fail(result, NODE_COMPONENT_FAILURE_MALFORMED_V1, "duplicate component_id");
        }
        status = copy_text(
            declaration->component_id.bytes,
            sizeof(declaration->component_id.bytes),
            &declaration->component_id.length,
            value,
            1);
    } else if (strcmp(key, "abi_major") == 0) {
        if (set_unique(&presence->abi_major) != 0 || parse_u16(value, &number) != 0) {
            return fail(result, NODE_COMPONENT_FAILURE_MALFORMED_V1, "invalid abi_major");
        }
        declaration->abi_major = number;
        return 0;
    } else if (strcmp(key, "abi_minor") == 0) {
        if (set_unique(&presence->abi_minor) != 0 || parse_u16(value, &number) != 0) {
            return fail(result, NODE_COMPONENT_FAILURE_MALFORMED_V1, "invalid abi_minor");
        }
        declaration->abi_minor = number;
        return 0;
    } else if (strcmp(key, "component_class") == 0) {
        if (set_unique(&presence->component_class) != 0) {
            return fail(result, NODE_COMPONENT_FAILURE_MALFORMED_V1, "duplicate component_class");
        }
        if (parse_class(value, &declaration->component_class) != 0) {
            return fail(result, NODE_COMPONENT_FAILURE_UNSUPPORTED_CLASS_V1, "unsupported component_class");
        }
        return 0;
    } else if (strcmp(key, "launch_kind") == 0) {
        if (set_unique(&presence->launch_kind) != 0) {
            return fail(result, NODE_COMPONENT_FAILURE_MALFORMED_V1, "duplicate launch_kind");
        }
        if (parse_launch(value, &declaration->launch_kind) != 0) {
            return fail(result, NODE_COMPONENT_FAILURE_UNSUPPORTED_LAUNCH_V1, "unsupported launch_kind");
        }
        return 0;
    } else if (strcmp(key, "entry") == 0) {
        if (set_unique(&presence->entry) != 0) {
            return fail(result, NODE_COMPONENT_FAILURE_MALFORMED_V1, "duplicate entry");
        }
        status = copy_text(
            declaration->entry.bytes,
            sizeof(declaration->entry.bytes),
            &declaration->entry.length,
            value,
            0);
    } else if (strcmp(key, "required_capability") == 0) {
        status = add_reference(
            declaration->required_capabilities,
            &declaration->required_capability_count,
            NODE_COMPONENT_CAPABILITY_LIMIT_V1,
            value);
    } else if (strcmp(key, "provided_capability") == 0) {
        status = add_reference(
            declaration->provided_capabilities,
            &declaration->provided_capability_count,
            NODE_COMPONENT_CAPABILITY_LIMIT_V1,
            value);
    } else if (strcmp(key, "dependency") == 0) {
        status = add_reference(
            declaration->dependencies,
            &declaration->dependency_count,
            NODE_COMPONENT_DEPENDENCY_LIMIT_V1,
            value);
    } else if (strcmp(key, "provenance_ref") == 0) {
        if (set_unique(&presence->provenance_ref) != 0) {
            return fail(result, NODE_COMPONENT_FAILURE_MALFORMED_V1, "duplicate provenance_ref");
        }
        status = copy_text(
            declaration->provenance_ref.bytes,
            sizeof(declaration->provenance_ref.bytes),
            &declaration->provenance_ref.length,
            value,
            0);
    } else if (strcmp(key, "integrity_ref") == 0) {
        if (set_unique(&presence->integrity_ref) != 0) {
            return fail(result, NODE_COMPONENT_FAILURE_MALFORMED_V1, "duplicate integrity_ref");
        }
        status = copy_text(
            declaration->integrity_ref.bytes,
            sizeof(declaration->integrity_ref.bytes),
            &declaration->integrity_ref.length,
            value,
            0);
    } else {
        return fail(result, NODE_COMPONENT_FAILURE_MALFORMED_V1, "unknown manifest key");
    }

    if (status == -2) {
        return fail(result, NODE_COMPONENT_FAILURE_LIMIT_EXCEEDED_V1, "manifest value or count exceeds ABI bound");
    }
    if (status != 0) {
        return fail(result, NODE_COMPONENT_FAILURE_MALFORMED_V1, "invalid manifest value");
    }
    return 0;
}

static int validate_complete(
    const struct manifest_presence *presence,
    struct node_component_declaration_v1 *declaration,
    struct node_component_lifecycle_result_v1 *result)
{
    if (presence->manifest_version == 0U || presence->component_id == 0U ||
        presence->abi_major == 0U || presence->abi_minor == 0U ||
        presence->component_class == 0U || presence->launch_kind == 0U ||
        presence->provenance_ref == 0U || presence->integrity_ref == 0U) {
        return fail(result, NODE_COMPONENT_FAILURE_MISSING_REQUIRED_V1, "required manifest key missing");
    }
    if (declaration->abi_major != NODE_EXTERNAL_COMPONENT_ABI_MAJOR_V1 ||
        declaration->abi_minor > NODE_EXTERNAL_COMPONENT_ABI_MINOR_V1) {
        return fail(result, NODE_COMPONENT_FAILURE_INCOMPATIBLE_ABI_V1, "external component ABI is incompatible");
    }
    if (declaration->launch_kind == NODE_COMPONENT_LAUNCH_NONE_V1) {
        if (presence->entry != 0U) {
            return fail(result, NODE_COMPONENT_FAILURE_MALFORMED_V1, "launch_kind none forbids entry");
        }
    } else if (presence->entry == 0U) {
        return fail(result, NODE_COMPONENT_FAILURE_MISSING_REQUIRED_V1, "launch entry is required");
    }

    result_set(
        result,
        NODE_COMPONENT_LIFECYCLE_VALIDATED_V1,
        NODE_COMPONENT_OUTCOME_SUCCEEDED_V1,
        NODE_COMPONENT_FAILURE_NONE_V1,
        "declaration structurally valid; no acceptance or launch performed");
    result->component_id = declaration->component_id;
    return 0;
}

int node_external_component_manifest_load_v1(
    const char *path,
    struct node_component_declaration_v1 *declaration,
    struct node_component_lifecycle_result_v1 *result)
{
    FILE *stream;
    char line[NODE_MANIFEST_LINE_CAPACITY];
    struct manifest_presence presence;
    unsigned long line_number = 0UL;

    if (path == NULL || declaration == NULL || result == NULL || *path == '\0') {
        if (result != NULL) {
            fail(result, NODE_COMPONENT_FAILURE_INVALID_ARGUMENT_V1, "invalid validator argument");
        }
        return -1;
    }

    memset(declaration, 0, sizeof(*declaration));
    memset(&presence, 0, sizeof(presence));
    declaration->struct_size = (uint32_t)sizeof(*declaration);
    result_set(
        result,
        NODE_COMPONENT_LIFECYCLE_DECLARED_V1,
        NODE_COMPONENT_OUTCOME_NOT_EVALUATED_V1,
        NODE_COMPONENT_FAILURE_NONE_V1,
        "manifest supplied");

    stream = fopen(path, "r");
    if (stream == NULL) {
        return fail(result, NODE_COMPONENT_FAILURE_IO_UNAVAILABLE_V1, "manifest unavailable");
    }

    while (fgets(line, sizeof(line), stream) != NULL) {
        size_t length;
        line_number += 1UL;
        length = strlen(line);
        if (length == sizeof(line) - 1U && line[length - 1U] != '\n' && !feof(stream)) {
            (void)fclose(stream);
            return fail(result, NODE_COMPONENT_FAILURE_LIMIT_EXCEEDED_V1, "manifest line exceeds bound");
        }
        while (length > 0U && (line[length - 1U] == '\n' || line[length - 1U] == '\r')) {
            line[--length] = '\0';
        }
        if (length == 0U || line[0] == '#') {
            continue;
        }
        if (parse_line(line, declaration, &presence, result) != 0) {
            (void)line_number;
            (void)fclose(stream);
            return -1;
        }
    }
    if (ferror(stream) != 0) {
        (void)fclose(stream);
        return fail(result, NODE_COMPONENT_FAILURE_IO_UNAVAILABLE_V1, "manifest read failed");
    }
    if (fclose(stream) != 0) {
        return fail(result, NODE_COMPONENT_FAILURE_IO_UNAVAILABLE_V1, "manifest close failed");
    }
    return validate_complete(&presence, declaration, result);
}

const char *node_external_component_failure_name_v1(uint32_t failure)
{
    switch (failure) {
        case NODE_COMPONENT_FAILURE_NONE_V1: return "none";
        case NODE_COMPONENT_FAILURE_INVALID_ARGUMENT_V1: return "invalid_argument";
        case NODE_COMPONENT_FAILURE_IO_UNAVAILABLE_V1: return "io_unavailable";
        case NODE_COMPONENT_FAILURE_MALFORMED_V1: return "malformed";
        case NODE_COMPONENT_FAILURE_INCOMPATIBLE_ABI_V1: return "incompatible_abi";
        case NODE_COMPONENT_FAILURE_LIMIT_EXCEEDED_V1: return "limit_exceeded";
        case NODE_COMPONENT_FAILURE_MISSING_REQUIRED_V1: return "missing_required";
        case NODE_COMPONENT_FAILURE_UNSUPPORTED_CLASS_V1: return "unsupported_class";
        case NODE_COMPONENT_FAILURE_UNSUPPORTED_LAUNCH_V1: return "unsupported_launch";
        default: return "unknown";
    }
}

const char *node_external_component_class_name_v1(uint32_t component_class)
{
    switch (component_class) {
        case NODE_COMPONENT_CLASS_RUNTIME_V1: return "runtime";
        case NODE_COMPONENT_CLASS_SERVICE_V1: return "service";
        case NODE_COMPONENT_CLASS_PROVIDER_V1: return "provider";
        case NODE_COMPONENT_CLASS_DIAGNOSTIC_V1: return "diagnostic";
        default: return "unspecified";
    }
}

const char *node_external_component_launch_name_v1(uint32_t launch_kind)
{
    switch (launch_kind) {
        case NODE_COMPONENT_LAUNCH_NONE_V1: return "none";
        case NODE_COMPONENT_LAUNCH_EXECUTABLE_V1: return "executable";
        case NODE_COMPONENT_LAUNCH_ENTRYPOINT_V1: return "entrypoint";
        default: return "unspecified";
    }
}
