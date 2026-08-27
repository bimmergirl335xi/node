#define _GNU_SOURCE

#include "node_p01_manifest.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/reboot.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define NODE_P01_BOOT_IDENTITY "node-p01-micro-os-v1"
#define NODE_P01_MANIFEST_PATH "/etc/node-p01/p01-public-startup-v1.manifest"
#define NODE_P01_CMDLINE_BYTES ((size_t)4096)
#define NODE_P01_STAGE_TIMEOUT_MS UINT64_C(35000)
#define NODE_P01_TERMINATION_GRACE_MS UINT64_C(250)
#define NODE_P01_HOLD_MAX_SECONDS UINT32_C(600)
#define NODE_P01_FD_CLOSE_LIMIT 256

enum process_outcome {
    PROCESS_NOT_LAUNCHED = 0,
    PROCESS_LAUNCH_FAILURE,
    PROCESS_NORMAL_EXIT,
    PROCESS_SIGNAL_TERMINATION,
    PROCESS_TIMEOUT,
    PROCESS_CANCELLATION,
    PROCESS_INDETERMINATE
};

enum terminal_action {
    TERMINAL_POWER_OFF = 0,
    TERMINAL_HALT,
    TERMINAL_REBOOT,
    TERMINAL_HOLD
};

struct boot_options {
    uint32_t hold_seconds;
    enum terminal_action action;
    uint32_t log_verbosity;
    char manifest_path[NODE_P01_MAX_PATH_BYTES];
    char expected_boot_identity[NODE_P01_MAX_IDENTITY_BYTES];
};

struct service_runtime {
    pid_t pid;
    uint64_t started_ms;
    uint64_t termination_requested_ms;
    uint32_t restart_count;
    int launched;
    int finished;
    int semantic_success;
    int termination_requested;
    int force_requested;
    int wait_status;
    enum process_outcome outcome;
};

struct supervisor {
    struct node_p01_manifest manifest;
    struct service_runtime runtime[NODE_P01_MAX_SERVICES];
    int signal_fd;
    int host_mode;
    int shutdown_requested;
    int concurrent_overlap_observed;
    char host_root[NODE_P01_MAX_PATH_BYTES];
};

static int diagnostic_serial_fd = -1;

static uint64_t monotonic_milliseconds(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return 0;
    }
    return (uint64_t)now.tv_sec * UINT64_C(1000) +
           (uint64_t)now.tv_nsec / UINT64_C(1000000);
}

static void emit_human(const char *format, ...) {
    va_list arguments;
    char message[512];
    int length;
    va_start(arguments, format);
    length = vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    if (length < 0 || (size_t)length >= sizeof(message)) {
        (void)snprintf(message, sizeof(message), "bounded human diagnostic exhausted");
    }
    (void)dprintf(STDOUT_FILENO, "NODE_P01 %s\n", message);
    if (diagnostic_serial_fd >= 0) {
        (void)dprintf(diagnostic_serial_fd, "NODE_P01 %s\n", message);
    }
}

static void emit_json(const char *record_type,
                      const char *subject,
                      const char *outcome,
                      const char *detail) {
    char escaped_subject[2 * NODE_P01_MAX_VALUE_BYTES];
    char escaped_detail[2 * NODE_P01_MAX_DETAIL_BYTES];
    char record[1024];
    int record_length;
    size_t subject_result = node_p01_json_escape(
        escaped_subject, sizeof(escaped_subject), subject, strlen(subject));
    size_t detail_result = node_p01_json_escape(
        escaped_detail, sizeof(escaped_detail), detail, strlen(detail));
    if (subject_result == SIZE_MAX || detail_result == SIZE_MAX) {
        record_length = snprintf(
            record, sizeof(record),
            "{\"record\":\"diagnostic_failure\","
            "\"subject\":\"json_encoding\","
            "\"outcome\":\"bounded_output_exhausted\"}\n");
    } else {
        record_length = snprintf(
            record, sizeof(record),
            "{\"record\":\"%s\",\"subject\":\"%s\","
            "\"outcome\":\"%s\",\"detail\":\"%s\"}\n",
            record_type, escaped_subject, outcome, escaped_detail);
    }
    if (record_length < 0 || (size_t)record_length >= sizeof(record)) {
        return;
    }
    (void)write(STDOUT_FILENO, record, (size_t)record_length);
    if (diagnostic_serial_fd >= 0) {
        (void)write(diagnostic_serial_fd, record, (size_t)record_length);
    }
}

static int parse_u32_bounded(const char *value,
                             uint32_t minimum,
                             uint32_t maximum,
                             uint32_t *output) {
    unsigned long parsed;
    char *end = NULL;
    if (value == NULL || value[0] == '\0' || value[0] == '-' || value[0] == '+') {
        return 0;
    }
    errno = 0;
    parsed = strtoul(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed < minimum ||
        parsed > maximum) {
        return 0;
    }
    *output = (uint32_t)parsed;
    return 1;
}

static int parse_command_line(const char *input,
                              struct boot_options *options,
                              char *detail,
                              size_t detail_capacity) {
    char copy[NODE_P01_CMDLINE_BYTES + 1];
    char *save = NULL;
    char *token;
    size_t length;

    options->hold_seconds = 1U;
    options->action = TERMINAL_POWER_OFF;
    options->log_verbosity = 1U;
    (void)snprintf(options->manifest_path, sizeof(options->manifest_path), "%s",
                   NODE_P01_MANIFEST_PATH);
    (void)snprintf(options->expected_boot_identity,
                   sizeof(options->expected_boot_identity), "%s",
                   NODE_P01_BOOT_IDENTITY);
    if (input == NULL) {
        (void)snprintf(detail, detail_capacity, "missing command line");
        return 0;
    }
    length = strlen(input);
    if (length > NODE_P01_CMDLINE_BYTES) {
        (void)snprintf(detail, detail_capacity, "command line exceeds bound");
        return 0;
    }
    (void)memcpy(copy, input, length + 1U);
    token = strtok_r(copy, " \t", &save);
    while (token != NULL) {
        const char *value;
        if (strncmp(token, "node.micro_os.",
                    sizeof("node.micro_os.") - 1U) != 0) {
            token = strtok_r(NULL, " \t", &save);
            continue;
        }
        if (strncmp(token, "node.micro_os.hold_seconds=",
                    sizeof("node.micro_os.hold_seconds=") - 1U) == 0) {
            value = token + sizeof("node.micro_os.hold_seconds=") - 1U;
            if (!parse_u32_bounded(value, 0U, NODE_P01_HOLD_MAX_SECONDS,
                                   &options->hold_seconds)) {
                (void)snprintf(detail, detail_capacity, "invalid hold_seconds");
                return 0;
            }
        } else if (strncmp(token, "node.micro_os.terminal_action=",
                           sizeof("node.micro_os.terminal_action=") - 1U) == 0) {
            value = token + sizeof("node.micro_os.terminal_action=") - 1U;
            if (strcmp(value, "poweroff") == 0) options->action = TERMINAL_POWER_OFF;
            else if (strcmp(value, "halt") == 0) options->action = TERMINAL_HALT;
            else if (strcmp(value, "reboot") == 0) options->action = TERMINAL_REBOOT;
            else if (strcmp(value, "hold") == 0) options->action = TERMINAL_HOLD;
            else {
                (void)snprintf(detail, detail_capacity, "invalid terminal_action");
                return 0;
            }
        } else if (strncmp(token, "node.micro_os.log_verbosity=",
                           sizeof("node.micro_os.log_verbosity=") - 1U) == 0) {
            value = token + sizeof("node.micro_os.log_verbosity=") - 1U;
            if (!parse_u32_bounded(value, 0U, 3U, &options->log_verbosity)) {
                (void)snprintf(detail, detail_capacity, "invalid log_verbosity");
                return 0;
            }
        } else if (strncmp(token, "node.micro_os.manifest=",
                           sizeof("node.micro_os.manifest=") - 1U) == 0) {
            value = token + sizeof("node.micro_os.manifest=") - 1U;
            if (strcmp(value, NODE_P01_MANIFEST_PATH) != 0) {
                (void)snprintf(detail, detail_capacity,
                               "only the tracked P01 manifest is permitted");
                return 0;
            }
        } else if (strncmp(token, "node.micro_os.expected_boot_identity=",
                           sizeof("node.micro_os.expected_boot_identity=") - 1U) == 0) {
            value = token + sizeof("node.micro_os.expected_boot_identity=") - 1U;
            if (strcmp(value, NODE_P01_BOOT_IDENTITY) != 0) {
                (void)snprintf(detail, detail_capacity,
                               "unexpected boot identity");
                return 0;
            }
        } else {
            (void)snprintf(detail, detail_capacity,
                           "unknown node.micro_os option");
            return 0;
        }
        token = strtok_r(NULL, " \t", &save);
    }
    (void)snprintf(detail, detail_capacity, "command line accepted");
    return 1;
}

static int read_command_line(char *output, size_t capacity) {
    int descriptor = open("/proc/cmdline", O_RDONLY | O_CLOEXEC);
    ssize_t count;
    if (descriptor < 0) return 0;
    count = read(descriptor, output, capacity - 1U);
    (void)close(descriptor);
    if (count < 0 || (size_t)count >= capacity - 1U) return 0;
    while (count > 0 && (output[count - 1] == '\n' || output[count - 1] == '\r')) {
        --count;
    }
    output[count] = '\0';
    return 1;
}

static int ensure_directory(const char *path, mode_t mode) {
    if (mkdir(path, mode) == 0 || errno == EEXIST) return 1;
    return 0;
}

static int mount_or_verify(const char *source,
                           const char *target,
                           const char *type,
                           unsigned long flags,
                           const char *data) {
    if (mount(source, target, type, flags, data) == 0 || errno == EBUSY) {
        return 1;
    }
    emit_json("filesystem_mount", target, "failed", strerror(errno));
    return 0;
}

static int establish_filesystems(void) {
    if (!ensure_directory("/dev", 0755) ||
        !mount_or_verify("devtmpfs", "/dev", "devtmpfs",
                         MS_NOSUID, "mode=0755") ||
        !ensure_directory("/proc", 0555) ||
        !mount_or_verify("proc", "/proc", "proc",
                         MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL) ||
        !ensure_directory("/sys", 0555) ||
        !mount_or_verify("sysfs", "/sys", "sysfs",
                         MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL) ||
        !ensure_directory("/run", 0755) ||
        !mount_or_verify("tmpfs", "/run", "tmpfs",
                         MS_NOSUID | MS_NODEV, "mode=0755,size=16m") ||
        !ensure_directory("/run/node-p01-results", 0700) ||
        !mount_or_verify("tmpfs", "/run/node-p01-results", "tmpfs",
                         MS_NOSUID | MS_NODEV | MS_NOEXEC,
                         "mode=0700,size=8m")) {
        return 0;
    }
    return 1;
}

static int establish_console(void) {
    int descriptor;
    int serial_descriptor;
    struct stat console_status;
    struct stat serial_status;
    if (access("/dev/console", F_OK) != 0 && errno == ENOENT) {
        (void)mknod("/dev/console", S_IFCHR | 0600, makedev(5, 1));
    }
    descriptor = open("/dev/console", O_RDWR | O_CLOEXEC);
    if (descriptor < 0) return 0;
    if (dup2(descriptor, STDIN_FILENO) < 0 ||
        dup2(descriptor, STDOUT_FILENO) < 0 ||
        dup2(descriptor, STDERR_FILENO) < 0) {
        (void)close(descriptor);
        return 0;
    }
    if (descriptor > STDERR_FILENO) (void)close(descriptor);
    serial_descriptor = open("/dev/ttyS0", O_WRONLY | O_NOCTTY | O_NONBLOCK |
                                           O_CLOEXEC);
    if (serial_descriptor >= 0 &&
        fstat(STDOUT_FILENO, &console_status) == 0 &&
        fstat(serial_descriptor, &serial_status) == 0 &&
        console_status.st_rdev != serial_status.st_rdev) {
        diagnostic_serial_fd = serial_descriptor;
    } else if (serial_descriptor >= 0) {
        (void)close(serial_descriptor);
    }
    return 1;
}

static int setup_signal_fd(void) {
    sigset_t mask;
    (void)sigemptyset(&mask);
    (void)sigaddset(&mask, SIGCHLD);
    (void)sigaddset(&mask, SIGTERM);
    (void)sigaddset(&mask, SIGINT);
    (void)sigaddset(&mask, SIGHUP);
    (void)sigaddset(&mask, SIGUSR1);
    (void)sigaddset(&mask, SIGUSR2);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) != 0) return -1;
    return signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
}

static int index_for_pid(const struct supervisor *supervisor, pid_t pid) {
    size_t index;
    for (index = 0; index < supervisor->manifest.service_count; ++index) {
        if (supervisor->runtime[index].pid == pid &&
            !supervisor->runtime[index].finished) {
            return (int)index;
        }
    }
    return -1;
}

static const char *process_outcome_name(enum process_outcome outcome) {
    static const char *const names[] = {
        "not_launched", "launch_failure", "normal_exit",
        "signal_termination", "timeout", "cancellation", "indeterminate"
    };
    size_t index = (size_t)outcome;
    return index < sizeof(names) / sizeof(names[0]) ? names[index] : "indeterminate";
}

static int expected_matches(const struct node_p01_service *service,
                            const struct service_runtime *runtime) {
    if (service->expected_result == NODE_P01_EXPECT_TIMEOUT) {
        return runtime->outcome == PROCESS_TIMEOUT;
    }
    if (service->expected_result == NODE_P01_EXPECT_SIGNAL) {
        return runtime->outcome == PROCESS_SIGNAL_TERMINATION;
    }
    if (runtime->outcome != PROCESS_NORMAL_EXIT || !WIFEXITED(runtime->wait_status)) {
        return 0;
    }
    if (service->expected_result == NODE_P01_EXPECT_EXIT_ZERO) {
        return WEXITSTATUS(runtime->wait_status) == 0;
    }
    return WEXITSTATUS(runtime->wait_status) != 0;
}

static void emit_service_result(const struct supervisor *supervisor, size_t index) {
    const struct node_p01_service *service = &supervisor->manifest.services[index];
    const struct service_runtime *runtime = &supervisor->runtime[index];
    char detail[NODE_P01_MAX_DETAIL_BYTES];
    if (runtime->launched && WIFEXITED(runtime->wait_status) &&
        runtime->outcome != PROCESS_LAUNCH_FAILURE) {
        (void)snprintf(detail, sizeof(detail),
                       "process=%s exit_code=%d semantic=%s required=%s restarts=%u",
                       process_outcome_name(runtime->outcome),
                       WEXITSTATUS(runtime->wait_status),
                       runtime->semantic_success ? "semantic_success" : "semantic_failure",
                       service->required ? "true" : "false", runtime->restart_count);
    } else if (runtime->launched && WIFSIGNALED(runtime->wait_status)) {
        (void)snprintf(detail, sizeof(detail),
                       "process=%s signal=%d semantic=%s required=%s restarts=%u",
                       process_outcome_name(runtime->outcome),
                       WTERMSIG(runtime->wait_status),
                       runtime->semantic_success ? "semantic_success" : "semantic_failure",
                       service->required ? "true" : "false", runtime->restart_count);
    } else {
        (void)snprintf(detail, sizeof(detail),
                       "process=%s semantic=%s required=%s restarts=%u",
                       process_outcome_name(runtime->outcome),
                       runtime->semantic_success ? "semantic_success" : "semantic_failure",
                       service->required ? "true" : "false", runtime->restart_count);
    }
    emit_json("service_process_result", service->identity,
              process_outcome_name(runtime->outcome), detail);
    emit_json("service_semantic_result", service->identity,
              runtime->semantic_success ? "semantic_success" : "semantic_failure",
              service->revision);
}

static void finalize_waited_service(struct supervisor *supervisor,
                                    size_t index,
                                    int status) {
    struct service_runtime *runtime = &supervisor->runtime[index];
    const struct node_p01_service *service = &supervisor->manifest.services[index];
    runtime->wait_status = status;
    runtime->pid = 0;
    runtime->finished = 1;
    if (runtime->outcome != PROCESS_TIMEOUT &&
        runtime->outcome != PROCESS_CANCELLATION) {
        if (WIFEXITED(status)) runtime->outcome = PROCESS_NORMAL_EXIT;
        else if (WIFSIGNALED(status)) runtime->outcome = PROCESS_SIGNAL_TERMINATION;
        else runtime->outcome = PROCESS_INDETERMINATE;
    }
    runtime->semantic_success = expected_matches(service, runtime);
    emit_service_result(supervisor, index);
    if (!runtime->semantic_success && !supervisor->shutdown_requested &&
        service->restart_policy == NODE_P01_RESTART_ON_FAILURE &&
        runtime->restart_count < service->maximum_restart_count) {
        uint32_t next_restart = runtime->restart_count + 1U;
        emit_json("service_restart", service->identity, "bounded_retry_scheduled",
                  "restart policy permits another attempt");
        (void)memset(runtime, 0, sizeof(*runtime));
        runtime->restart_count = next_restart;
    }
}

static void reap_children(struct supervisor *supervisor) {
    for (;;) {
        int status = 0;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            int index = index_for_pid(supervisor, pid);
            if (index >= 0) {
                finalize_waited_service(supervisor, (size_t)index, status);
            } else {
                emit_json("child_reaping", "untracked_child", "reaped",
                          "PID 1 reaped a child without an active service record");
            }
        } else if (pid == 0 || (pid < 0 && errno == ECHILD)) {
            break;
        } else if (pid < 0 && errno != EINTR) {
            emit_json("child_reaping", "waitpid", "indeterminate", strerror(errno));
            break;
        }
    }
}

static void request_termination(struct supervisor *supervisor,
                                size_t index,
                                enum process_outcome outcome) {
    struct service_runtime *runtime = &supervisor->runtime[index];
    if (runtime->pid <= 0 || runtime->finished || runtime->termination_requested) return;
    runtime->outcome = outcome;
    runtime->termination_requested = 1;
    runtime->termination_requested_ms = monotonic_milliseconds();
    if (kill(-runtime->pid, SIGTERM) != 0 && errno != ESRCH) {
        runtime->outcome = PROCESS_INDETERMINATE;
    }
}

static int build_exec_path(const struct supervisor *supervisor,
                           const struct node_p01_service *service,
                           char *output,
                           size_t capacity) {
    int count;
    if (supervisor->host_mode) {
        count = snprintf(output, capacity, "%s%s", supervisor->host_root,
                         service->executable);
    } else {
        count = snprintf(output, capacity, "%s", service->executable);
    }
    return count >= 0 && (size_t)count < capacity;
}

static int launch_service(struct supervisor *supervisor, size_t index) {
    const struct node_p01_service *service = &supervisor->manifest.services[index];
    struct service_runtime *runtime = &supervisor->runtime[index];
    char executable[2 * NODE_P01_MAX_PATH_BYTES];
    int exec_status[2];
    int exec_errno = 0;
    ssize_t exec_status_count;
    uint32_t restart_count = runtime->restart_count;
    pid_t pid;

    if (!build_exec_path(supervisor, service, executable, sizeof(executable))) {
        runtime->finished = 1;
        runtime->outcome = PROCESS_LAUNCH_FAILURE;
        emit_service_result(supervisor, index);
        return 0;
    }
    emit_json("service_launch_attempt", service->identity, "launch_requested",
              executable);
    if (pipe2(exec_status, O_CLOEXEC) != 0) {
        runtime->finished = 1;
        runtime->outcome = PROCESS_LAUNCH_FAILURE;
        runtime->semantic_success = 0;
        emit_service_result(supervisor, index);
        return 0;
    }
    pid = fork();
    if (pid < 0) {
        (void)close(exec_status[0]);
        (void)close(exec_status[1]);
        runtime->finished = 1;
        runtime->outcome = PROCESS_LAUNCH_FAILURE;
        runtime->semantic_success = 0;
        emit_service_result(supervisor, index);
        return 0;
    }
    if (pid == 0) {
        char *arguments[NODE_P01_MAX_ARGUMENTS + 2];
        char *environment[NODE_P01_MAX_ENVIRONMENT + 2];
        size_t argument_index;
        size_t environment_index;
        sigset_t empty;
        int descriptor;

        (void)close(exec_status[0]);
        (void)setpgid(0, 0);
        (void)sigemptyset(&empty);
        (void)sigprocmask(SIG_SETMASK, &empty, NULL);
        arguments[0] = executable;
        for (argument_index = 0; argument_index < service->argument_count;
             ++argument_index) {
            arguments[argument_index + 1U] =
                (char *)(void *)service->arguments[argument_index].bytes;
        }
        arguments[service->argument_count + 1U] = NULL;
        for (environment_index = 0;
             environment_index < service->environment_count;
             ++environment_index) {
            environment[environment_index] =
                (char *)(void *)service->environment[environment_index].bytes;
        }
        if (supervisor->host_mode) {
            static char host_root_environment[2 * NODE_P01_MAX_PATH_BYTES];
            (void)snprintf(host_root_environment, sizeof(host_root_environment),
                           "NODE_P01_HOST_ROOT=%s", supervisor->host_root);
            environment[environment_index++] = host_root_environment;
        }
        environment[environment_index] = NULL;
        for (descriptor = 3; descriptor < NODE_P01_FD_CLOSE_LIMIT; ++descriptor) {
            if (descriptor != exec_status[1]) {
                (void)close(descriptor);
            }
        }
        execve(executable, arguments, environment);
        exec_errno = errno;
        (void)write(exec_status[1], &exec_errno, sizeof(exec_errno));
        _exit(127);
    }
    (void)close(exec_status[1]);
    (void)setpgid(pid, pid);
    (void)memset(runtime, 0, sizeof(*runtime));
    runtime->restart_count = restart_count;
    runtime->pid = pid;
    runtime->launched = 1;
    runtime->started_ms = monotonic_milliseconds();
    do {
        exec_status_count = read(exec_status[0], &exec_errno, sizeof(exec_errno));
    } while (exec_status_count < 0 && errno == EINTR);
    (void)close(exec_status[0]);
    if (exec_status_count > 0) {
        int status = 0;
        (void)waitpid(pid, &status, 0);
        runtime->pid = 0;
        runtime->finished = 1;
        runtime->outcome = PROCESS_LAUNCH_FAILURE;
        runtime->semantic_success = 0;
        emit_service_result(supervisor, index);
        return 0;
    }
    return 1;
}

static int dependencies_succeeded(const struct supervisor *supervisor,
                                  size_t service_index_value,
                                  int *blocked) {
    const struct node_p01_service *service =
        &supervisor->manifest.services[service_index_value];
    size_t dependency_index;
    *blocked = 0;
    for (dependency_index = 0; dependency_index < service->dependency_count;
         ++dependency_index) {
        size_t candidate;
        for (candidate = 0; candidate < supervisor->manifest.service_count;
             ++candidate) {
            if (strcmp(supervisor->manifest.services[candidate].identity,
                       service->dependencies[dependency_index]) == 0) {
                if (!supervisor->runtime[candidate].finished) return 0;
                if (!supervisor->runtime[candidate].semantic_success) *blocked = 1;
                break;
            }
        }
    }
    return 1;
}

static size_t active_service_count(const struct supervisor *supervisor,
                                   enum node_p01_stage stage) {
    size_t index;
    size_t count = 0;
    for (index = 0; index < supervisor->manifest.service_count; ++index) {
        if (supervisor->manifest.services[index].stage == stage &&
            supervisor->runtime[index].pid > 0 &&
            !supervisor->runtime[index].finished) {
            ++count;
        }
    }
    return count;
}

static void process_signal_events(struct supervisor *supervisor) {
    struct signalfd_siginfo information;
    for (;;) {
        ssize_t count = read(supervisor->signal_fd, &information,
                             sizeof(information));
        if (count != (ssize_t)sizeof(information)) {
            if (count < 0 && errno != EAGAIN && errno != EINTR) {
                emit_json("signal_event", "signalfd", "indeterminate",
                          strerror(errno));
            }
            break;
        }
        if (information.ssi_signo == SIGTERM || information.ssi_signo == SIGINT) {
            supervisor->shutdown_requested = 1;
            emit_json("signal_event", "shutdown", "cancellation_requested",
                      information.ssi_signo == SIGTERM ? "SIGTERM" : "SIGINT");
        } else if (information.ssi_signo == SIGHUP) {
            emit_json("signal_event", "SIGHUP", "observed",
                      "runtime reconfiguration is not implemented in P01");
        } else if (information.ssi_signo == SIGUSR1 ||
                   information.ssi_signo == SIGUSR2) {
            emit_json("signal_event", "diagnostic_signal", "observed",
                      information.ssi_signo == SIGUSR1 ? "SIGUSR1" : "SIGUSR2");
        }
    }
}

static int run_stage(struct supervisor *supervisor, enum node_p01_stage stage) {
    uint64_t stage_start = monotonic_milliseconds();
    int stage_complete = 0;
    int required_failure = 0;
    char detail[NODE_P01_MAX_DETAIL_BYTES];

    emit_json("startup_stage", node_p01_stage_name(stage), "entered",
              "stage execution began");
    while (!stage_complete) {
        size_t index;
        uint64_t now;
        size_t unfinished = 0;

        for (index = 0; index < supervisor->manifest.service_count; ++index) {
            struct service_runtime *runtime = &supervisor->runtime[index];
            int dependency_blocked = 0;
            if (supervisor->manifest.services[index].stage != stage ||
                runtime->launched || runtime->finished) {
                continue;
            }
            if (!dependencies_succeeded(supervisor, index, &dependency_blocked)) {
                continue;
            }
            if (dependency_blocked || supervisor->shutdown_requested) {
                runtime->finished = 1;
                runtime->outcome = PROCESS_CANCELLATION;
                runtime->semantic_success = 0;
                emit_service_result(supervisor, index);
            } else {
                (void)launch_service(supervisor, index);
            }
        }
        if (active_service_count(supervisor, stage) >= 2U) {
            supervisor->concurrent_overlap_observed = 1;
        }
        reap_children(supervisor);
        now = monotonic_milliseconds();
        for (index = 0; index < supervisor->manifest.service_count; ++index) {
            const struct node_p01_service *service = &supervisor->manifest.services[index];
            struct service_runtime *runtime = &supervisor->runtime[index];
            if (service->stage != stage || runtime->finished || runtime->pid <= 0) {
                continue;
            }
            if (supervisor->shutdown_requested) {
                request_termination(supervisor, index, PROCESS_CANCELLATION);
            } else if (!runtime->termination_requested &&
                       now - runtime->started_ms >= service->timeout_ms) {
                request_termination(supervisor, index, PROCESS_TIMEOUT);
            }
            if (runtime->termination_requested && !runtime->force_requested &&
                now - runtime->termination_requested_ms >=
                    NODE_P01_TERMINATION_GRACE_MS) {
                if (kill(-runtime->pid, SIGKILL) != 0 && errno != ESRCH) {
                    runtime->outcome = PROCESS_INDETERMINATE;
                }
                runtime->force_requested = 1;
            }
        }
        if (now - stage_start >= NODE_P01_STAGE_TIMEOUT_MS) {
            for (index = 0; index < supervisor->manifest.service_count; ++index) {
                if (supervisor->manifest.services[index].stage == stage) {
                    request_termination(supervisor, index, PROCESS_TIMEOUT);
                }
            }
        }
        for (index = 0; index < supervisor->manifest.service_count; ++index) {
            if (supervisor->manifest.services[index].stage == stage &&
                !supervisor->runtime[index].finished) {
                ++unfinished;
            }
        }
        stage_complete = unfinished == 0U;
        if (!stage_complete) {
            struct pollfd descriptor;
            descriptor.fd = supervisor->signal_fd;
            descriptor.events = POLLIN;
            descriptor.revents = 0;
            (void)poll(&descriptor, 1, 20);
            if ((descriptor.revents & POLLIN) != 0) {
                process_signal_events(supervisor);
            }
        }
    }
    for (size_t index = 0; index < supervisor->manifest.service_count; ++index) {
        if (supervisor->manifest.services[index].stage == stage &&
            supervisor->manifest.services[index].required &&
            !supervisor->runtime[index].semantic_success) {
            required_failure = 1;
        }
    }
    (void)snprintf(detail, sizeof(detail), "concurrent_overlap=%s",
                   supervisor->concurrent_overlap_observed ? "true" : "false");
    emit_json("stage_result", node_p01_stage_name(stage),
              required_failure ? "required_service_failure" : "stage_completed",
              detail);
    return !required_failure;
}

static void cancel_later_services(struct supervisor *supervisor,
                                  enum node_p01_stage failed_stage) {
    size_t index;
    for (index = 0; index < supervisor->manifest.service_count; ++index) {
        struct service_runtime *runtime = &supervisor->runtime[index];
        if (supervisor->manifest.services[index].stage > failed_stage &&
            !runtime->finished) {
            runtime->finished = 1;
            runtime->outcome = PROCESS_CANCELLATION;
            runtime->semantic_success = 0;
            emit_service_result(supervisor, index);
        }
    }
}

static int verify_no_children(void) {
    int status = 0;
    pid_t result = waitpid(-1, &status, WNOHANG);
    return result < 0 && errno == ECHILD;
}

static int run_supervisor(struct supervisor *supervisor) {
    enum node_p01_stage stage;
    int required_scope_complete = 1;
    for (stage = NODE_P01_STAGE_EARLY_ENVIRONMENT;
         stage < NODE_P01_STAGE_COUNT;
         stage = (enum node_p01_stage)(stage + 1)) {
        if (!run_stage(supervisor, stage)) {
            required_scope_complete = 0;
            cancel_later_services(supervisor, stage);
            break;
        }
    }
    reap_children(supervisor);
    emit_json("child_reaping", "pid1_children",
              verify_no_children() ? "zombie_free" : "indeterminate",
              "bounded child table inspection complete");
    emit_json("concurrency_evidence", "same_stage_services",
              supervisor->concurrent_overlap_observed ? "overlap_observed"
                                                       : "overlap_not_observed",
              "process lifetimes overlapped under PID 1 supervision");
    emit_json("micro_os_final_result", NODE_P01_BOOT_IDENTITY,
              required_scope_complete ? "p01_terminal_state_reached"
                                      : "required_service_failure",
              "micro-OS entered; no runtime-readiness or installation claim");
    return required_scope_complete ? 0 : 2;
}

static void bounded_delay(uint32_t seconds) {
    struct timespec remaining;
    remaining.tv_sec = (time_t)seconds;
    remaining.tv_nsec = 0;
    while (nanosleep(&remaining, &remaining) != 0 && errno == EINTR) {
    }
}

static void terminal_policy(const struct boot_options *options, int host_mode) {
    bounded_delay(options->hold_seconds);
    emit_json("shutdown_result", NODE_P01_BOOT_IDENTITY, "terminal_action_requested",
              options->action == TERMINAL_REBOOT ? "reboot" :
              options->action == TERMINAL_HALT ? "halt" :
              options->action == TERMINAL_HOLD ? "bounded_hold_then_halt" :
              "poweroff");
    if (host_mode) return;
    sync();
    if (options->action == TERMINAL_REBOOT) (void)reboot(RB_AUTOBOOT);
    else if (options->action == TERMINAL_POWER_OFF) (void)reboot(RB_POWER_OFF);
    else (void)reboot(RB_HALT_SYSTEM);
    for (;;) pause();
}

static void emergency_halt(void) {
    const char message[] =
        "NODE_P01 fatal early-environment failure; bounded halt requested\n";
    (void)write(STDERR_FILENO, message, sizeof(message) - 1U);
    bounded_delay(1U);
    sync();
    (void)reboot(RB_HALT_SYSTEM);
    for (;;) pause();
}

static int usage(void) {
    (void)fprintf(stderr,
                  "usage: node_p01_init [--host-root DIR --manifest FILE] "
                  "[--cmdline TEXT]\n");
    return 64;
}

int main(int argument_count, char **arguments) {
    struct supervisor supervisor;
    struct boot_options options;
    struct node_p01_manifest_result manifest_result;
    struct utsname kernel_identity;
    char command_line[NODE_P01_CMDLINE_BYTES + 1] = {0};
    char command_line_detail[NODE_P01_MAX_DETAIL_BYTES];
    char manifest_path[2 * NODE_P01_MAX_PATH_BYTES];
    const char *manifest_override = NULL;
    const char *command_line_override = NULL;
    int index;
    int result;

    (void)memset(&supervisor, 0, sizeof(supervisor));
    supervisor.signal_fd = -1;
    for (index = 1; index < argument_count; ++index) {
        if (strcmp(arguments[index], "--host-root") == 0 && index + 1 < argument_count) {
            supervisor.host_mode = 1;
            if (snprintf(supervisor.host_root, sizeof(supervisor.host_root), "%s",
                         arguments[++index]) >= (int)sizeof(supervisor.host_root)) {
                return usage();
            }
        } else if (strcmp(arguments[index], "--manifest") == 0 &&
                   index + 1 < argument_count) {
            manifest_override = arguments[++index];
        } else if (strcmp(arguments[index], "--cmdline") == 0 &&
                   index + 1 < argument_count) {
            command_line_override = arguments[++index];
        } else {
            return usage();
        }
    }
    if (!supervisor.host_mode && getpid() != 1) {
        (void)fprintf(stderr, "node_p01_init refuses non-PID1 execution\n");
        return 65;
    }
    if (supervisor.host_mode) {
        if (supervisor.host_root[0] != '/' || strstr(supervisor.host_root, "..") != NULL ||
            manifest_override == NULL) {
            return usage();
        }
        (void)setvbuf(stdout, NULL, _IONBF, 0);
    } else {
        if (!establish_filesystems() || !establish_console()) {
            emergency_halt();
        }
        emit_json("filesystem_setup", "volatile_filesystems",
                  "established", "devtmpfs, procfs, sysfs, and bounded tmpfs");
    }
    emit_json("micro_os_boot_attempt", NODE_P01_BOOT_IDENTITY, "entered",
              "permanent C PID 1 entered");
    if (uname(&kernel_identity) == 0) {
        emit_human("pid=%ld kernel=%s release=%s", (long)getpid(),
                   kernel_identity.sysname, kernel_identity.release);
        emit_json("kernel_identity", kernel_identity.sysname, "observed",
                  kernel_identity.release);
    }
    if (command_line_override != NULL) {
        (void)snprintf(command_line, sizeof(command_line), "%s", command_line_override);
    } else if (supervisor.host_mode) {
        (void)snprintf(command_line, sizeof(command_line),
                       "node.micro_os.hold_seconds=0 "
                       "node.micro_os.terminal_action=poweroff "
                       "node.micro_os.expected_boot_identity=%s",
                       NODE_P01_BOOT_IDENTITY);
    } else if (!read_command_line(command_line, sizeof(command_line))) {
        emit_json("kernel_command_line", "node.micro_os", "manifest_rejection",
                  "command line could not be read within bounds");
        for (;;) pause();
    }
    if (!parse_command_line(command_line, &options, command_line_detail,
                            sizeof(command_line_detail))) {
        emit_json("kernel_command_line", "node.micro_os", "manifest_rejection",
                  command_line_detail);
        if (!supervisor.host_mode) {
            terminal_policy(&options, 0);
        }
        return 66;
    }
    if (manifest_override != NULL) {
        (void)snprintf(manifest_path, sizeof(manifest_path), "%s", manifest_override);
    } else {
        (void)snprintf(manifest_path, sizeof(manifest_path), "%s",
                       options.manifest_path);
    }
    if (node_p01_manifest_load(manifest_path, &supervisor.manifest,
                               &manifest_result) != NODE_P01_MANIFEST_ACCEPTED) {
        emit_json("manifest_validation", "p01_startup_manifest", "manifest_rejection",
                  manifest_result.detail);
        terminal_policy(&options, supervisor.host_mode);
        return 67;
    }
    emit_json("manifest_validation", supervisor.manifest.identity,
              "accepted_for_p01_structural_scope", manifest_result.detail);
    supervisor.signal_fd = setup_signal_fd();
    if (supervisor.signal_fd < 0) {
        emit_json("signal_supervision", "signalfd", "failed", strerror(errno));
        terminal_policy(&options, supervisor.host_mode);
        return 68;
    }
    result = run_supervisor(&supervisor);
    (void)close(supervisor.signal_fd);
    terminal_policy(&options, supervisor.host_mode);
    if (!supervisor.host_mode) {
        for (;;) pause();
    }
    return result;
}
