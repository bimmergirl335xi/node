#include "external_component_manifest.h"

#include <stdio.h>
#include <string.h>

#define FIXTURE(name) NODE_COMPONENT_FIXTURE_DIR "/" name

static int expect_failure(const char *path, uint32_t expected_failure)
{
    struct node_component_declaration_v1 declaration;
    struct node_component_lifecycle_result_v1 result;

    if (node_external_component_manifest_load_v1(path, &declaration, &result) == 0) {
        fprintf(stderr, "manifest unexpectedly accepted: %s\n", path);
        return 1;
    }
    if (result.failure != expected_failure) {
        fprintf(
            stderr,
            "manifest failure mismatch: %s expected=%u actual=%u\n",
            path,
            (unsigned int)expected_failure,
            (unsigned int)result.failure);
        return 1;
    }
    return 0;
}

int main(void)
{
    struct node_component_declaration_v1 declaration;
    struct node_component_lifecycle_result_v1 result;

    if (node_external_component_manifest_load_v1(
            NODE_COMPONENT_FIXTURE_DIR "/valid.manifest",
            &declaration,
            &result) != 0) {
        fprintf(stderr, "valid manifest rejected: %s\n", result.detail);
        return 1;
    }
    if (declaration.struct_size != sizeof(declaration) ||
        result.struct_size != sizeof(result) ||
        declaration.abi_major != NODE_EXTERNAL_COMPONENT_ABI_MAJOR_V1 ||
        declaration.abi_minor != NODE_EXTERNAL_COMPONENT_ABI_MINOR_V1 ||
        declaration.component_class != NODE_COMPONENT_CLASS_PROVIDER_V1 ||
        declaration.launch_kind != NODE_COMPONENT_LAUNCH_EXECUTABLE_V1 ||
        declaration.required_capability_count != 2U ||
        declaration.provided_capability_count != 1U ||
        declaration.dependency_count != 1U ||
        result.lifecycle != NODE_COMPONENT_LIFECYCLE_VALIDATED_V1 ||
        result.outcome != NODE_COMPONENT_OUTCOME_SUCCEEDED_V1 ||
        result.failure != NODE_COMPONENT_FAILURE_NONE_V1 ||
        strcmp(declaration.component_id.bytes, "example.metrics") != 0) {
        fputs("valid manifest did not populate the bounded ABI contract\n", stderr);
        return 1;
    }
    if (expect_failure(
            FIXTURE("malformed.manifest"),
            NODE_COMPONENT_FAILURE_MALFORMED_V1) != 0 ||
        expect_failure(
            FIXTURE("incompatible.manifest"),
            NODE_COMPONENT_FAILURE_INCOMPATIBLE_ABI_V1) != 0 ||
        expect_failure(
            FIXTURE("missing-required.manifest"),
            NODE_COMPONENT_FAILURE_MISSING_REQUIRED_V1) != 0 ||
        expect_failure(
            FIXTURE("unsupported-class.manifest"),
            NODE_COMPONENT_FAILURE_UNSUPPORTED_CLASS_V1) != 0 ||
        expect_failure(
            FIXTURE("too-many-capabilities.manifest"),
            NODE_COMPONENT_FAILURE_LIMIT_EXCEEDED_V1) != 0) {
        return 1;
    }

    puts("external component manifest ABI validation passed");
    return 0;
}
