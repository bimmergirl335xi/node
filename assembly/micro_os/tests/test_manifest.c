#include "node_p01_manifest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char valid_manifest[] =
    "manifest_version=1\n"
    "manifest_identity=test-manifest\n"
    "service_begin\n"
    "identity=probe_a\n"
    "revision=test:probe_a:1\n"
    "executable=/node/services/probe_a\n"
    "stage=early_environment\n"
    "required=true\n"
    "timeout_ms=100\n"
    "expected_result=exit_zero\n"
    "dependencies=\n"
    "restart_policy=none\n"
    "maximum_restart_count=0\n"
    "arguments=\n"
    "environment=LANG=C\n"
    "service_end\n";

static int failures = 0;

static void expect_status(const char *name,
                          const char *manifest_text,
                          enum node_p01_manifest_status expected) {
    struct node_p01_manifest manifest;
    struct node_p01_manifest_result result;
    enum node_p01_manifest_status actual = node_p01_manifest_parse(
        manifest_text, strlen(manifest_text), &manifest, &result);
    if (actual != expected) {
        (void)fprintf(stderr, "%s: expected %s, got %s (%s)\n", name,
                      node_p01_manifest_status_name(expected),
                      node_p01_manifest_status_name(actual), result.detail);
        ++failures;
    }
}

static char *replace_once(const char *input,
                          const char *needle,
                          const char *replacement) {
    const char *position = strstr(input, needle);
    size_t prefix;
    size_t length;
    char *output;
    if (position == NULL) return NULL;
    prefix = (size_t)(position - input);
    length = strlen(input) - strlen(needle) + strlen(replacement) + 1U;
    output = malloc(length);
    if (output == NULL) return NULL;
    (void)memcpy(output, input, prefix);
    (void)memcpy(output + prefix, replacement, strlen(replacement));
    (void)strcpy(output + prefix + strlen(replacement), position + strlen(needle));
    return output;
}

static void expect_replacement(const char *name,
                               const char *needle,
                               const char *replacement,
                               enum node_p01_manifest_status expected) {
    char *changed = replace_once(valid_manifest, needle, replacement);
    if (changed == NULL) {
        (void)fprintf(stderr, "%s: fixture replacement failed\n", name);
        ++failures;
        return;
    }
    expect_status(name, changed, expected);
    free(changed);
}

static void test_dependency_cases(void) {
    const char missing[] =
        "manifest_version=1\nmanifest_identity=missing-dependency\n"
        "service_begin\nidentity=probe_a\nrevision=test:a:1\n"
        "executable=/node/services/probe_a\nstage=early_environment\n"
        "required=true\ntimeout_ms=100\nexpected_result=exit_zero\n"
        "dependencies=absent\nrestart_policy=none\nmaximum_restart_count=0\n"
        "arguments=\nenvironment=LANG=C\nservice_end\n";
    const char cycle[] =
        "manifest_version=1\nmanifest_identity=cycle\n"
        "service_begin\nidentity=probe_a\nrevision=test:a:1\n"
        "executable=/node/services/probe_a\nstage=early_environment\n"
        "required=true\ntimeout_ms=100\nexpected_result=exit_zero\n"
        "dependencies=probe_b\nrestart_policy=none\nmaximum_restart_count=0\n"
        "arguments=\nenvironment=LANG=C\nservice_end\n"
        "service_begin\nidentity=probe_b\nrevision=test:b:1\n"
        "executable=/node/services/probe_b\nstage=early_environment\n"
        "required=true\ntimeout_ms=100\nexpected_result=exit_zero\n"
        "dependencies=probe_a\nrestart_policy=none\nmaximum_restart_count=0\n"
        "arguments=\nenvironment=LANG=C\nservice_end\n";
    expect_status("missing dependency", missing,
                  NODE_P01_MANIFEST_MISSING_DEPENDENCY);
    expect_status("cycle", cycle, NODE_P01_MANIFEST_DEPENDENCY_CYCLE);
    expect_replacement("self dependency", "dependencies=\n",
                       "dependencies=probe_a\n",
                       NODE_P01_MANIFEST_DEPENDENCY_CYCLE);
}

static void test_limits(void) {
    char manifest[NODE_P01_MAX_MANIFEST_BYTES + 1];
    size_t used = 0;
    size_t index;
    int count = snprintf(manifest, sizeof(manifest),
                         "manifest_version=1\nmanifest_identity=overflow\n");
    if (count < 0) {
        ++failures;
        return;
    }
    used = (size_t)count;
    for (index = 0; index < NODE_P01_MAX_SERVICES + 1U; ++index) {
        count = snprintf(
            manifest + used, sizeof(manifest) - used,
            "service_begin\nidentity=probe_%zu\nrevision=test:probe_%zu:1\n"
            "executable=/node/services/probe_%zu\nstage=early_environment\n"
            "required=true\ntimeout_ms=100\nexpected_result=exit_zero\n"
            "dependencies=\nrestart_policy=none\nmaximum_restart_count=0\n"
            "arguments=\nenvironment=LANG=C\nservice_end\n",
            index, index, index);
        if (count < 0 || (size_t)count >= sizeof(manifest) - used) {
            ++failures;
            return;
        }
        used += (size_t)count;
    }
    expect_status("service count overflow", manifest,
                  NODE_P01_MANIFEST_LIMIT_EXCEEDED);
    expect_replacement("dependency count overflow", "dependencies=\n",
                       "dependencies=a,b,c,d,e,f,g,h,i\n",
                       NODE_P01_MANIFEST_LIMIT_EXCEEDED);
}

static void test_ordering(void) {
    const char unordered[] =
        "manifest_version=1\nmanifest_identity=ordered\n"
        "service_begin\nidentity=z_probe\nrevision=test:z:1\n"
        "executable=/node/services/z_probe\nstage=terminal\nrequired=true\n"
        "timeout_ms=100\nexpected_result=exit_zero\ndependencies=a_probe\n"
        "restart_policy=none\nmaximum_restart_count=0\narguments=\n"
        "environment=LANG=C\nservice_end\n"
        "service_begin\nidentity=a_probe\nrevision=test:a:1\n"
        "executable=/node/services/a_probe\nstage=early_environment\nrequired=true\n"
        "timeout_ms=100\nexpected_result=exit_zero\ndependencies=\n"
        "restart_policy=none\nmaximum_restart_count=0\narguments=\n"
        "environment=LANG=C\nservice_end\n";
    struct node_p01_manifest manifest;
    struct node_p01_manifest_result result;
    if (node_p01_manifest_parse(unordered, strlen(unordered), &manifest, &result) !=
            NODE_P01_MANIFEST_ACCEPTED ||
        strcmp(manifest.services[0].identity, "a_probe") != 0 ||
        strcmp(manifest.services[1].identity, "z_probe") != 0) {
        (void)fprintf(stderr, "deterministic ordering failed: %s\n", result.detail);
        ++failures;
    }
}

static void test_json(void) {
    char output[32];
    const char input[] = "a\"b\\c\n";
    if (node_p01_json_escape(output, sizeof(output), input, sizeof(input) - 1U) ==
            SIZE_MAX ||
        strcmp(output, "a\\\"b\\\\c\\u000a") != 0 ||
        node_p01_json_escape(output, 4U, input, sizeof(input) - 1U) != SIZE_MAX) {
        (void)fprintf(stderr, "bounded JSON escaping failed\n");
        ++failures;
    }
}

int main(void) {
    expect_status("valid manifest", valid_manifest, NODE_P01_MANIFEST_ACCEPTED);
    expect_replacement("missing required field", "timeout_ms=100\n", "",
                       NODE_P01_MANIFEST_MISSING_FIELD);
    expect_replacement("unknown field", "required=true\n",
                       "required=true\nunknown=true\n",
                       NODE_P01_MANIFEST_UNKNOWN_FIELD);
    expect_replacement("duplicate scalar", "required=true\n",
                       "required=true\nrequired=true\n",
                       NODE_P01_MANIFEST_DUPLICATE_FIELD);
    expect_replacement("invalid identity", "identity=probe_a\n",
                       "identity=../probe\n",
                       NODE_P01_MANIFEST_INVALID_VALUE);
    expect_replacement("invalid stage", "stage=early_environment\n",
                       "stage=unknown\n", NODE_P01_MANIFEST_INVALID_VALUE);
    expect_replacement("invalid executable", "/node/services/probe_a",
                       "/tmp/probe_a", NODE_P01_MANIFEST_INVALID_VALUE);
    expect_replacement("invalid timeout", "timeout_ms=100\n",
                       "timeout_ms=0\n", NODE_P01_MANIFEST_INVALID_VALUE);
    expect_replacement("duplicate service", "service_end\n",
                       "service_end\nservice_begin\nidentity=probe_a\n"
                       "revision=test:probe_a:2\nexecutable=/node/services/probe_a\n"
                       "stage=terminal\nrequired=true\ntimeout_ms=100\n"
                       "expected_result=exit_zero\ndependencies=\n"
                       "restart_policy=none\nmaximum_restart_count=0\n"
                       "arguments=\nenvironment=LANG=C\nservice_end\n",
                       NODE_P01_MANIFEST_DUPLICATE_SERVICE);
    test_dependency_cases();
    test_limits();
    test_ordering();
    test_json();
    if (failures != 0) {
        (void)fprintf(stderr, "%d manifest tests failed\n", failures);
        return 1;
    }
    (void)printf("P01 manifest tests passed\n");
    return 0;
}
