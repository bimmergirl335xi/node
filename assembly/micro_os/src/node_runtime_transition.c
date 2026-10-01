#include "node_runtime_transition.h"

enum node_runtime_transition_code node_runtime_transition_evaluate(
    const struct node_runtime_transition_evidence *evidence) {
    if (evidence == 0) {
        return NODE_RUNTIME_EVIDENCE_INCOMPLETE;
    }
    if (!evidence->required_startup_complete) {
        return NODE_RUNTIME_REQUIRED_STARTUP_FAILED;
    }
    if (!evidence->evidence_complete) {
        return NODE_RUNTIME_EVIDENCE_INCOMPLETE;
    }
    if (!evidence->cpu_decision_compatible ||
        !evidence->cpu_runtime_required) {
        return NODE_RUNTIME_CPU_DECISION_INCOMPATIBLE;
    }
    if (!evidence->cpu_runtime_validated ||
        !evidence->cpu_runtime_activated) {
        return NODE_RUNTIME_CPU_NOT_ACTIVATED;
    }
    if (!evidence->cpu_initial_probe_passed) {
        return NODE_RUNTIME_CPU_INITIAL_PROBE_FAILED;
    }
    if (!evidence->acs_initialized ||
        (evidence->acs_peer_exchange_required &&
         !evidence->acs_exchange_complete)) {
        return NODE_RUNTIME_ACS_NOT_INITIALIZED;
    }
    return NODE_RUNTIME_TRANSITION_ACCEPTED;
}

enum node_resident_health_code node_resident_health_evaluate(
    uint8_t cpu_post_transition_probe_passed,
    uint8_t essential_services_alive,
    uint8_t shutdown_requested) {
    if (shutdown_requested) return NODE_RESIDENT_SHUTDOWN_REQUESTED;
    if (!essential_services_alive) {
        return NODE_RESIDENT_ESSENTIAL_SERVICE_EXITED;
    }
    if (!cpu_post_transition_probe_passed) {
        return NODE_RESIDENT_CPU_PROBE_FAILED;
    }
    return NODE_RESIDENT_HEALTHY;
}

const char *node_runtime_transition_code_name(
    enum node_runtime_transition_code code) {
    static const char *const names[] = {
        "accepted",
        "required_startup_failed",
        "cpu_decision_incompatible",
        "cpu_runtime_not_activated",
        "cpu_initial_probe_failed",
        "acs_not_initialized",
        "evidence_incomplete"
    };
    unsigned index = (unsigned)code;
    return index < sizeof(names) / sizeof(names[0]) ? names[index]
                                                    : "evidence_incomplete";
}

const char *node_resident_health_code_name(
    enum node_resident_health_code code) {
    static const char *const names[] = {
        "healthy",
        "cpu_post_transition_probe_failed",
        "essential_service_exited",
        "shutdown_requested"
    };
    unsigned index = (unsigned)code;
    return index < sizeof(names) / sizeof(names[0]) ? names[index]
                                                    : "essential_service_exited";
}
