#ifndef NODE_RUNTIME_TRANSITION_H
#define NODE_RUNTIME_TRANSITION_H

#include <stdint.h>

enum node_runtime_transition_code {
    NODE_RUNTIME_TRANSITION_ACCEPTED = 0,
    NODE_RUNTIME_REQUIRED_STARTUP_FAILED,
    NODE_RUNTIME_CPU_DECISION_INCOMPATIBLE,
    NODE_RUNTIME_CPU_NOT_ACTIVATED,
    NODE_RUNTIME_CPU_INITIAL_PROBE_FAILED,
    NODE_RUNTIME_ACS_NOT_INITIALIZED,
    NODE_RUNTIME_EVIDENCE_INCOMPLETE
};

enum node_resident_health_code {
    NODE_RESIDENT_HEALTHY = 0,
    NODE_RESIDENT_CPU_PROBE_FAILED,
    NODE_RESIDENT_ESSENTIAL_SERVICE_EXITED,
    NODE_RESIDENT_SHUTDOWN_REQUESTED
};

struct node_runtime_transition_evidence {
    uint8_t required_startup_complete;
    uint8_t cpu_decision_compatible;
    uint8_t cpu_runtime_required;
    uint8_t cpu_runtime_validated;
    uint8_t cpu_runtime_activated;
    uint8_t cpu_initial_probe_passed;
    uint8_t acs_initialized;
    uint8_t acs_peer_exchange_required;
    uint8_t acs_exchange_complete;
    uint8_t evidence_complete;
};

enum node_runtime_transition_code node_runtime_transition_evaluate(
    const struct node_runtime_transition_evidence *evidence);

enum node_resident_health_code node_resident_health_evaluate(
    uint8_t cpu_post_transition_probe_passed,
    uint8_t essential_services_alive,
    uint8_t shutdown_requested);

const char *node_runtime_transition_code_name(
    enum node_runtime_transition_code code);
const char *node_resident_health_code_name(
    enum node_resident_health_code code);

#endif
