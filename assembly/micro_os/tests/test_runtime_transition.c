#include "node_runtime_transition.h"

#include <stdio.h>
#include <stdlib.h>

static int failures;

static void expect_transition(
    const char *name,
    struct node_runtime_transition_evidence evidence,
    enum node_runtime_transition_code expected) {
    enum node_runtime_transition_code actual =
        node_runtime_transition_evaluate(&evidence);
    if (actual != expected) {
        (void)fprintf(stderr, "%s: expected %s, got %s\n", name,
                      node_runtime_transition_code_name(expected),
                      node_runtime_transition_code_name(actual));
        ++failures;
    }
}

int main(void) {
    struct node_runtime_transition_evidence evidence = {
        1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U
    };
    expect_transition("complete evidence", evidence,
                      NODE_RUNTIME_TRANSITION_ACCEPTED);
    evidence.required_startup_complete = 0U;
    expect_transition("required startup failure", evidence,
                      NODE_RUNTIME_REQUIRED_STARTUP_FAILED);
    evidence.required_startup_complete = 1U;
    evidence.cpu_decision_compatible = 0U;
    expect_transition("CPU decision incompatible", evidence,
                      NODE_RUNTIME_CPU_DECISION_INCOMPATIBLE);
    evidence.cpu_decision_compatible = 1U;
    evidence.cpu_runtime_activated = 0U;
    expect_transition("CPU runtime inactive", evidence,
                      NODE_RUNTIME_CPU_NOT_ACTIVATED);
    evidence.cpu_runtime_activated = 1U;
    evidence.cpu_initial_probe_passed = 0U;
    expect_transition("initial CPU probe failure", evidence,
                      NODE_RUNTIME_CPU_INITIAL_PROBE_FAILED);
    evidence.cpu_initial_probe_passed = 1U;
    evidence.acs_initialized = 0U;
    expect_transition("ACS initialization failure", evidence,
                      NODE_RUNTIME_ACS_NOT_INITIALIZED);
    evidence.acs_initialized = 1U;
    evidence.acs_exchange_complete = 0U;
    expect_transition("ACS exchange incomplete", evidence,
                      NODE_RUNTIME_ACS_NOT_INITIALIZED);
    evidence.acs_exchange_complete = 1U;
    evidence.acs_peer_exchange_required = 0U;
    evidence.acs_exchange_complete = 0U;
    expect_transition("discovery mode local ACS readiness", evidence,
                      NODE_RUNTIME_TRANSITION_ACCEPTED);
    evidence.acs_exchange_complete = 1U;
    evidence.evidence_complete = 0U;
    expect_transition("incomplete transition evidence", evidence,
                      NODE_RUNTIME_EVIDENCE_INCOMPLETE);
    if (node_runtime_transition_evaluate(NULL) !=
        NODE_RUNTIME_EVIDENCE_INCOMPLETE) {
        (void)fprintf(stderr, "null transition evidence did not fail closed\n");
        ++failures;
    }
    if (node_resident_health_evaluate(1U, 1U, 0U) !=
            NODE_RESIDENT_HEALTHY ||
        node_resident_health_evaluate(0U, 1U, 0U) !=
            NODE_RESIDENT_CPU_PROBE_FAILED ||
        node_resident_health_evaluate(1U, 0U, 0U) !=
            NODE_RESIDENT_ESSENTIAL_SERVICE_EXITED ||
        node_resident_health_evaluate(1U, 1U, 1U) !=
            NODE_RESIDENT_SHUTDOWN_REQUESTED) {
        (void)fprintf(stderr, "resident health classification failed\n");
        ++failures;
    }
    if (failures != 0) return EXIT_FAILURE;
    (void)printf("Runtime transition tests passed\n");
    return EXIT_SUCCESS;
}
