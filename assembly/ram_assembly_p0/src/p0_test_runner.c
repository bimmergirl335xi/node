#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define P0_MAX_TESTS 32U
#define P0_MAX_LINE 384U
#define P0_MAX_ID 48U
#define P0_MAX_PATH 160U
#define P0_MAX_ROOT 256U
#define P0_MIN_TIMEOUT_MS 10U
#define P0_MAX_TIMEOUT_MS 60000U
#define P0_POLL_NS 10000000L
#define P0_EXEC_FAILURE 125
#define P0_SEMANTIC_FAILURE 2

enum p0_expected {
    P0_EXPECT_PASS = 0,
    P0_EXPECT_SEMANTIC_FAIL,
    P0_EXPECT_TIMEOUT,
    P0_EXPECT_LAUNCH_FAILURE,
    P0_EXPECT_SIGNAL,
    P0_EXPECT_PROCESS_EXIT
};

enum p0_termination {
    P0_TERM_LAUNCH_FAILURE = 0,
    P0_TERM_EXIT,
    P0_TERM_SIGNAL,
    P0_TERM_TIMEOUT,
    P0_TERM_WAIT_FAILURE
};

struct p0_test {
    char id[P0_MAX_ID + 1U];
    char path[P0_MAX_PATH + 1U];
    uint32_t timeout_ms;
    enum p0_expected expected;
};

struct p0_result {
    enum p0_termination termination;
    int exit_code;
    int signal_number;
    bool launched;
    bool semantic_pass;
    bool semantic_fail;
    bool contract_pass;
};

struct p0_counts {
    unsigned int discovered;
    unsigned int executed;
    unsigned int passed;
    unsigned int failed;
    unsigned int timed_out;
    unsigned int launch_failed;
    unsigned int signaled;
    unsigned int contract_mismatch;
};

static const char *expected_name(enum p0_expected expected)
{
    switch (expected) {
    case P0_EXPECT_PASS:
        return "pass";
    case P0_EXPECT_SEMANTIC_FAIL:
        return "semantic_fail";
    case P0_EXPECT_TIMEOUT:
        return "timeout";
    case P0_EXPECT_LAUNCH_FAILURE:
        return "launch_failure";
    case P0_EXPECT_SIGNAL:
        return "signal";
    case P0_EXPECT_PROCESS_EXIT:
        return "process_exit";
    }
    return "unknown";
}

static const char *termination_name(enum p0_termination termination)
{
    switch (termination) {
    case P0_TERM_LAUNCH_FAILURE:
        return "launch_failure";
    case P0_TERM_EXIT:
        return "exit";
    case P0_TERM_SIGNAL:
        return "signal";
    case P0_TERM_TIMEOUT:
        return "timeout";
    case P0_TERM_WAIT_FAILURE:
        return "wait_failure";
    }
    return "unknown";
}

static const char *semantic_name(const struct p0_result *result)
{
    if (result->semantic_pass) {
        return "pass";
    }
    if (result->semantic_fail) {
        return "fail";
    }
    return "not_reported";
}

static bool valid_id(const char *value)
{
    size_t index;
    const size_t length = strlen(value);

    if (length == 0U || length > P0_MAX_ID) {
        return false;
    }
    for (index = 0U; index < length; ++index) {
        const unsigned char ch = (unsigned char)value[index];
        if (!(isalnum(ch) || ch == '_' || ch == '-' || ch == '.')) {
            return false;
        }
    }
    return true;
}

static bool valid_path(const char *value)
{
    const size_t length = strlen(value);

    return length > 7U && length <= P0_MAX_PATH &&
           strncmp(value, "/tests/", 7U) == 0 && strstr(value, "..") == NULL;
}

static bool parse_timeout(const char *value, uint32_t *result)
{
    char *end = NULL;
    unsigned long parsed;

    errno = 0;
    parsed = strtoul(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' ||
        parsed < P0_MIN_TIMEOUT_MS || parsed > P0_MAX_TIMEOUT_MS) {
        return false;
    }
    *result = (uint32_t)parsed;
    return true;
}

static bool parse_expected(const char *value, enum p0_expected *result)
{
    if (strcmp(value, "pass") == 0) {
        *result = P0_EXPECT_PASS;
        return true;
    }
    if (strcmp(value, "semantic_fail") == 0) {
        *result = P0_EXPECT_SEMANTIC_FAIL;
        return true;
    }
    if (strcmp(value, "timeout") == 0) {
        *result = P0_EXPECT_TIMEOUT;
        return true;
    }
    if (strcmp(value, "launch_failure") == 0) {
        *result = P0_EXPECT_LAUNCH_FAILURE;
        return true;
    }
    if (strcmp(value, "signal") == 0) {
        *result = P0_EXPECT_SIGNAL;
        return true;
    }
    if (strcmp(value, "process_exit") == 0) {
        *result = P0_EXPECT_PROCESS_EXIT;
        return true;
    }
    return false;
}

static bool parse_line(char *line, struct p0_test *test)
{
    char *fields[4];
    char *cursor = line;
    size_t field_index;

    for (field_index = 0U; field_index < 4U; ++field_index) {
        fields[field_index] = cursor;
        if (field_index < 3U) {
            cursor = strchr(cursor, '|');
            if (cursor == NULL) {
                return false;
            }
            *cursor = '\0';
            ++cursor;
        } else if (strchr(cursor, '|') != NULL) {
            return false;
        }
    }

    if (!valid_id(fields[0]) || !valid_path(fields[1]) ||
        !parse_timeout(fields[2], &test->timeout_ms) ||
        !parse_expected(fields[3], &test->expected)) {
        return false;
    }

    (void)snprintf(test->id, sizeof(test->id), "%s", fields[0]);
    (void)snprintf(test->path, sizeof(test->path), "%s", fields[1]);
    return true;
}

static int read_manifest(const char *manifest_path, struct p0_test *tests,
                         unsigned int *test_count)
{
    FILE *manifest;
    char line[P0_MAX_LINE + 2U];
    unsigned int line_number = 0U;

    manifest = fopen(manifest_path, "re");
    if (manifest == NULL) {
        fprintf(stderr, "NODE_P0 MANIFEST open_failed path=%s errno=%d\n",
                manifest_path, errno);
        return -1;
    }

    while (fgets(line, sizeof(line), manifest) != NULL) {
        size_t length;
        char *content = line;

        ++line_number;
        length = strlen(line);
        if (length == 0U) {
            continue;
        }
        if (line[length - 1U] != '\n' && !feof(manifest)) {
            fprintf(stderr, "NODE_P0 MANIFEST line_too_long line=%u\n",
                    line_number);
            (void)fclose(manifest);
            return -1;
        }
        while (length > 0U && (line[length - 1U] == '\n' ||
                               line[length - 1U] == '\r')) {
            line[--length] = '\0';
        }
        while (*content == ' ' || *content == '\t') {
            ++content;
        }
        if (*content == '\0' || *content == '#') {
            continue;
        }
        if (*test_count >= P0_MAX_TESTS) {
            fprintf(stderr, "NODE_P0 MANIFEST test_limit_exceeded max=%u\n",
                    P0_MAX_TESTS);
            (void)fclose(manifest);
            return -1;
        }
        if (!parse_line(content, &tests[*test_count])) {
            fprintf(stderr, "NODE_P0 MANIFEST invalid line=%u\n", line_number);
            (void)fclose(manifest);
            return -1;
        }
        ++(*test_count);
    }

    {
        const bool read_failed = ferror(manifest) != 0;
        const bool close_failed = fclose(manifest) != 0;
        if (!read_failed && !close_failed && *test_count > 0U) {
            return 0;
        }
        fprintf(stderr, "NODE_P0 MANIFEST read_failed_or_empty\n");
        return -1;
    }
}

static uint64_t monotonic_milliseconds(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return UINT64_MAX;
    }
    return (uint64_t)now.tv_sec * UINT64_C(1000) +
           (uint64_t)now.tv_nsec / UINT64_C(1000000);
}

static bool expected_result_observed(const struct p0_test *test,
                                     const struct p0_result *result)
{
    switch (test->expected) {
    case P0_EXPECT_PASS:
        return result->termination == P0_TERM_EXIT && result->semantic_pass;
    case P0_EXPECT_SEMANTIC_FAIL:
        return result->termination == P0_TERM_EXIT && result->semantic_fail;
    case P0_EXPECT_TIMEOUT:
        return result->termination == P0_TERM_TIMEOUT;
    case P0_EXPECT_LAUNCH_FAILURE:
        return result->termination == P0_TERM_LAUNCH_FAILURE;
    case P0_EXPECT_SIGNAL:
        return result->termination == P0_TERM_SIGNAL;
    case P0_EXPECT_PROCESS_EXIT:
        return result->termination == P0_TERM_EXIT && !result->semantic_pass &&
               !result->semantic_fail;
    }
    return false;
}

static struct p0_result execute_test(const struct p0_test *test,
                                     const char *root)
{
    struct p0_result result = {
        .termination = P0_TERM_LAUNCH_FAILURE,
        .exit_code = -1,
        .signal_number = 0,
        .launched = false,
        .semantic_pass = false,
        .semantic_fail = false,
        .contract_pass = false,
    };
    char executable[P0_MAX_ROOT + P0_MAX_PATH + 2U];
    uint64_t start_ms;
    pid_t child;
    int status = 0;

    {
        const int path_length =
            snprintf(executable, sizeof(executable), "%s%s", root, test->path);
        if (path_length < 0 || (size_t)path_length >= sizeof(executable)) {
            return result;
        }
    }

    start_ms = monotonic_milliseconds();
    if (start_ms == UINT64_MAX) {
        result.termination = P0_TERM_WAIT_FAILURE;
        return result;
    }

    child = fork();
    if (child < 0) {
        return result;
    }
    if (child == 0) {
        (void)setpgid(0, 0);
        execl(executable, executable, (char *)NULL);
        _exit(P0_EXEC_FAILURE);
    }

    (void)setpgid(child, child);
    result.launched = true;
    for (;;) {
        const pid_t wait_result = waitpid(child, &status, WNOHANG);
        if (wait_result == child) {
            break;
        }
        if (wait_result < 0 && errno != EINTR) {
            result.termination = P0_TERM_WAIT_FAILURE;
            (void)kill(-child, SIGKILL);
            (void)kill(child, SIGKILL);
            (void)waitpid(child, NULL, 0);
            result.contract_pass = false;
            return result;
        }
        if (wait_result == 0) {
            const uint64_t now_ms = monotonic_milliseconds();
            if (now_ms == UINT64_MAX || now_ms - start_ms >= test->timeout_ms) {
                (void)kill(-child, SIGKILL);
                (void)kill(child, SIGKILL);
                while (waitpid(child, &status, 0) < 0 && errno == EINTR) {
                }
                result.termination = P0_TERM_TIMEOUT;
                result.signal_number = SIGKILL;
                result.contract_pass = expected_result_observed(test, &result);
                return result;
            }
            {
                const struct timespec delay = {.tv_sec = 0, .tv_nsec = P0_POLL_NS};
                (void)nanosleep(&delay, NULL);
            }
        }
    }

    if (WIFSIGNALED(status)) {
        result.termination = P0_TERM_SIGNAL;
        result.signal_number = WTERMSIG(status);
    } else if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
        if (result.exit_code == P0_EXEC_FAILURE) {
            result.termination = P0_TERM_LAUNCH_FAILURE;
            result.launched = false;
        } else {
            result.termination = P0_TERM_EXIT;
            result.semantic_pass = result.exit_code == 0;
            result.semantic_fail = result.exit_code == P0_SEMANTIC_FAILURE;
        }
    } else {
        result.termination = P0_TERM_WAIT_FAILURE;
    }

    result.contract_pass = expected_result_observed(test, &result);
    return result;
}

static void emit_result(const struct p0_test *test,
                        const struct p0_result *result)
{
    printf("NODE_P0 TEST id=%s termination=%s semantic=%s expected=%s contract=%s\n",
           test->id, termination_name(result->termination), semantic_name(result),
           expected_name(test->expected), result->contract_pass ? "pass" : "fail");
    printf("{\"record\":\"test_result\",\"id\":\"%s\","
           "\"launch\":\"%s\",\"termination\":\"%s\","
           "\"exit_code\":%d,\"signal\":%d,\"timed_out\":%s,"
           "\"semantic\":\"%s\",\"expected\":\"%s\","
           "\"contract_pass\":%s}\n",
           test->id, result->launched ? "ok" : "failed",
           termination_name(result->termination), result->exit_code,
           result->signal_number,
           result->termination == P0_TERM_TIMEOUT ? "true" : "false",
           semantic_name(result), expected_name(test->expected),
           result->contract_pass ? "true" : "false");
    (void)fflush(stdout);
}

static void update_counts(struct p0_counts *counts,
                          const struct p0_result *result)
{
    if (result->launched) {
        ++counts->executed;
    }
    if (result->termination == P0_TERM_TIMEOUT) {
        ++counts->timed_out;
    } else if (result->termination == P0_TERM_LAUNCH_FAILURE) {
        ++counts->launch_failed;
        ++counts->failed;
    } else if (result->termination == P0_TERM_SIGNAL) {
        ++counts->signaled;
        ++counts->failed;
    } else if (result->semantic_pass) {
        ++counts->passed;
    } else {
        ++counts->failed;
    }
    if (!result->contract_pass) {
        ++counts->contract_mismatch;
    }
}

int main(int argc, char **argv)
{
    struct p0_test tests[P0_MAX_TESTS];
    struct p0_counts counts = {0};
    const char *manifest_path = "/etc/node-p0/tests.manifest";
    const char *root = "";
    unsigned int test_count = 0U;
    unsigned int index;
    int argument_index;

    for (argument_index = 1; argument_index < argc; ++argument_index) {
        if (strcmp(argv[argument_index], "--manifest") == 0 &&
            argument_index + 1 < argc) {
            manifest_path = argv[++argument_index];
        } else if (strcmp(argv[argument_index], "--root") == 0 &&
                   argument_index + 1 < argc) {
            root = argv[++argument_index];
        } else {
            fprintf(stderr, "usage: p0_test_runner [--manifest PATH] [--root PATH]\n");
            return 2;
        }
    }

    if (strlen(root) > P0_MAX_ROOT || strchr(root, '\n') != NULL ||
        strchr(root, '\r') != NULL) {
        fprintf(stderr, "NODE_P0 RUNNER invalid_root\n");
        return 2;
    }

    if (read_manifest(manifest_path, tests, &test_count) != 0) {
        printf("{\"record\":\"runner_summary\",\"outcome\":\"manifest_failed\"}\n");
        return 2;
    }

    counts.discovered = test_count;
    printf("NODE_P0 tests_discovered=%u\n", counts.discovered);
    printf("{\"record\":\"test_discovery\",\"discovered\":%u}\n",
           counts.discovered);

    for (index = 0U; index < test_count; ++index) {
        const struct p0_result result = execute_test(&tests[index], root);
        emit_result(&tests[index], &result);
        update_counts(&counts, &result);
    }

    printf("NODE_P0 SUMMARY discovered=%u executed=%u passed=%u failed=%u "
           "timed_out=%u launch_failed=%u signaled=%u contract_mismatch=%u\n",
           counts.discovered, counts.executed, counts.passed, counts.failed,
           counts.timed_out, counts.launch_failed, counts.signaled,
           counts.contract_mismatch);
    printf("{\"record\":\"runner_summary\",\"discovered\":%u,"
           "\"executed\":%u,\"passed\":%u,\"failed\":%u,"
           "\"timed_out\":%u,\"launch_failed\":%u,\"signaled\":%u,"
           "\"contract_mismatch\":%u,\"outcome\":\"%s\"}\n",
           counts.discovered, counts.executed, counts.passed, counts.failed,
           counts.timed_out, counts.launch_failed, counts.signaled,
           counts.contract_mismatch,
           counts.contract_mismatch == 0U ? "mechanisms_operated" :
                                           "mechanism_failure");
    (void)fflush(stdout);
    return counts.contract_mismatch == 0U ? 0 : 1;
}
