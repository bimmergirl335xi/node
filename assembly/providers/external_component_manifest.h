#ifndef NODE_EXTERNAL_COMPONENT_MANIFEST_H
#define NODE_EXTERNAL_COMPONENT_MANIFEST_H

#include "node_external_component_v1.h"

#ifdef __cplusplus
extern "C" {
#endif

int node_external_component_manifest_load_v1(
    const char *path,
    struct node_component_declaration_v1 *declaration,
    struct node_component_lifecycle_result_v1 *result);

const char *node_external_component_failure_name_v1(uint32_t failure);
const char *node_external_component_class_name_v1(uint32_t component_class);
const char *node_external_component_launch_name_v1(uint32_t launch_kind);

#ifdef __cplusplus
}
#endif

#endif
