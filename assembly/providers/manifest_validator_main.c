#include "external_component_manifest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void json_text(const char *value)
{
    const unsigned char *cursor = (const unsigned char *)value;
    putchar('"');
    while (*cursor != '\0') {
        if (*cursor == '"' || *cursor == '\\') {
            putchar('\\');
            putchar((int)*cursor);
        } else if (*cursor >= 0x20U && *cursor <= 0x7eU) {
            putchar((int)*cursor);
        } else {
            printf("\\u%04x", (unsigned int)*cursor);
        }
        ++cursor;
    }
    putchar('"');
}

int main(int argc, char **argv)
{
    const char *argument_path = NULL;
    const char *environment_path = getenv("NODE_EXTERNAL_COMPONENT_MANIFEST");
    const char *selected_path;
    struct node_component_declaration_v1 declaration;
    struct node_component_declaration_result_v1 result;
    int index;
    int status;

    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--manifest") == 0 && index + 1 < argc &&
            argument_path == NULL) {
            argument_path = argv[++index];
        } else {
            fprintf(stderr, "usage: %s [--manifest PATH]\n", argv[0]);
            return 2;
        }
    }

    if (environment_path != NULL && *environment_path == '\0') {
        environment_path = NULL;
    }
    if (argument_path != NULL && environment_path != NULL) {
        fprintf(stderr, "external manifest is ambiguous: use argument or environment, not both\n");
        return 2;
    }
    selected_path = argument_path != NULL ? argument_path : environment_path;
    if (selected_path == NULL) {
        puts("{\"record\":\"external_component_declaration\",\"declaration_state\":\"declaration_not_supplied\",\"failure\":\"none\"}");
        return 0;
    }

    status = node_external_component_manifest_load_v1(selected_path, &declaration, &result);
    printf("{\"record\":\"external_component_declaration\",\"declaration_state\":");
    json_text(node_external_component_declaration_state_name_v1(result.declaration_state));
    printf(",\"failure\":");
    json_text(node_external_component_failure_name_v1(result.failure));
    printf(",\"detail\":");
    json_text(result.detail);
    if (status == 0) {
        printf(",\"component_id\":");
        json_text(declaration.component_id.bytes);
        printf(",\"declaration_revision_ref\":");
        json_text(declaration.declaration_revision_ref.bytes);
        printf(",\"component_class\":");
        json_text(node_external_component_class_name_v1(declaration.component_class));
        printf(",\"launch_kind\":");
        json_text(node_external_component_launch_name_v1(declaration.launch_kind));
    }
    puts("}");
    return status == 0 ? 0 : 1;
}
