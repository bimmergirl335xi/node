#include <errno.h>
#include <fcntl.h>
#include <linux/reboot.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define P0_CMDLINE_MAX 2048U
#define P0_DEFAULT_HOLD_SECONDS 20U
#define P0_MAX_HOLD_SECONDS 600U

static void print_json_string(const char *value)
{
    const unsigned char *cursor = (const unsigned char *)value;

    (void)putchar('"');
    while (*cursor != '\0') {
        switch (*cursor) {
        case '"':
            (void)fputs("\\\"", stdout);
            break;
        case '\\':
            (void)fputs("\\\\", stdout);
            break;
        case '\b':
            (void)fputs("\\b", stdout);
            break;
        case '\f':
            (void)fputs("\\f", stdout);
            break;
        case '\n':
            (void)fputs("\\n", stdout);
            break;
        case '\r':
            (void)fputs("\\r", stdout);
            break;
        case '\t':
            (void)fputs("\\t", stdout);
            break;
        default:
            if (*cursor < 0x20U) {
                (void)printf("\\u%04x", (unsigned int)*cursor);
            } else {
                (void)putchar((int)*cursor);
            }
            break;
        }
        ++cursor;
    }
    (void)putchar('"');
}

static int ensure_directory(const char *path, mode_t mode)
{
    if (mkdir(path, mode) == 0 || errno == EEXIST) {
        return 0;
    }
    return -1;
}

static int mount_if_needed(const char *source, const char *target,
                           const char *filesystem, unsigned long flags,
                           const char *data)
{
    if (ensure_directory(target, 0755) != 0) {
        return -1;
    }
    if (mount(source, target, filesystem, flags, data) == 0 || errno == EBUSY) {
        return 0;
    }
    return -1;
}

static void open_console(void)
{
    int console = open("/dev/console", O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (console < 0) {
        return;
    }
    if (console <= STDERR_FILENO) {
        const int duplicate = fcntl(console, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
        (void)close(console);
        console = duplicate;
        if (console < 0) {
            return;
        }
    }
    (void)dup2(console, STDIN_FILENO);
    (void)dup2(console, STDOUT_FILENO);
    (void)dup2(console, STDERR_FILENO);
    (void)close(console);
}

static unsigned int parse_hold_seconds(bool *halt_only)
{
    char command_line[P0_CMDLINE_MAX + 1U];
    const char hold_prefix[] = "node.p0.hold_seconds=";
    FILE *file;
    char *token;
    char *save = NULL;
    unsigned int result = P0_DEFAULT_HOLD_SECONDS;

    *halt_only = false;
    file = fopen("/proc/cmdline", "re");
    if (file == NULL) {
        return result;
    }
    if (fgets(command_line, sizeof(command_line), file) == NULL) {
        (void)fclose(file);
        return result;
    }
    (void)fclose(file);

    token = strtok_r(command_line, " \t\r\n", &save);
    while (token != NULL) {
        if (strncmp(token, hold_prefix, sizeof(hold_prefix) - 1U) == 0) {
            char *end = NULL;
            const unsigned long parsed =
                strtoul(token + sizeof(hold_prefix) - 1U, &end, 10);
            if (end != token + sizeof(hold_prefix) - 1U && *end == '\0' &&
                parsed <= P0_MAX_HOLD_SECONDS) {
                result = (unsigned int)parsed;
            }
        } else if (strcmp(token, "node.p0.halt=1") == 0) {
            *halt_only = true;
        }
        token = strtok_r(NULL, " \t\r\n", &save);
    }
    return result;
}

static int run_tests(void)
{
    pid_t child;
    int status = 0;

    child = fork();
    if (child < 0) {
        return -1;
    }
    if (child == 0) {
        execl("/bin/p0_test_runner", "/bin/p0_test_runner", (char *)NULL);
        _exit(125);
    }
    while (waitpid(child, &status, 0) < 0) {
        if (errno != EINTR) {
            return -1;
        }
    }
    if (!WIFEXITED(status)) {
        return -1;
    }
    return WEXITSTATUS(status);
}

int main(void)
{
    struct utsname identity;
    bool halt_only;
    unsigned int hold_seconds;
    int mount_failures = 0;
    int runner_result;

    if (mount_if_needed("devtmpfs", "/dev", "devtmpfs", MS_NOSUID, "mode=0755") != 0) {
        ++mount_failures;
    }
    open_console();

    printf("\n============================================================\n");
    printf(" Node RAM Assembly P0 candidate environment\n");
    printf(" Mechanism proof only: no installation or activation claim\n");
    printf("============================================================\n");
    printf("NODE_P0 PID1 entered pid=%ld\n", (long)getpid());
    printf("{\"record\":\"stage\",\"stage\":\"pid1_entered\","
           "\"observed\":true}\n");

    if (mount_if_needed("proc", "/proc", "proc", MS_NOSUID | MS_NODEV | MS_NOEXEC,
                        NULL) != 0) {
        ++mount_failures;
    }
    if (mount_if_needed("sysfs", "/sys", "sysfs", MS_NOSUID | MS_NODEV | MS_NOEXEC,
                        NULL) != 0) {
        ++mount_failures;
    }
    if (mount_if_needed("tmpfs", "/run", "tmpfs", MS_NOSUID | MS_NODEV,
                        "mode=0755,size=16M") != 0) {
        ++mount_failures;
    }

    if (uname(&identity) == 0) {
        printf("NODE_P0 kernel_entered sysname=%s release=%s machine=%s\n",
               identity.sysname, identity.release, identity.machine);
        printf("{\"record\":\"stage\",\"stage\":\"kernel_entered\","
               "\"observed\":true,\"release\":");
        print_json_string(identity.release);
        printf(",\"machine\":");
        print_json_string(identity.machine);
        printf("}\n");
    } else {
        printf("NODE_P0 kernel_identity unavailable errno=%d\n", errno);
    }
    printf("NODE_P0 filesystem_mount_failures=%d\n", mount_failures);
    (void)fflush(stdout);

    runner_result = run_tests();
    printf("NODE_P0 FINAL mechanism_outcome=%s runner_exit=%d mount_failures=%d\n",
           runner_result == 0 && mount_failures == 0 ? "mechanisms_operated" :
                                                      "mechanism_failure",
           runner_result, mount_failures);
    printf("{\"record\":\"p0_final\",\"outcome\":\"%s\","
           "\"runner_exit\":%d,\"mount_failures\":%d,"
           "\"claims\":[\"candidate_environment_entered\","
           "\"bundled_c_tests_executed\"]}\n",
           runner_result == 0 && mount_failures == 0 ? "mechanisms_operated" :
                                                      "mechanism_failure",
           runner_result, mount_failures);

    hold_seconds = parse_hold_seconds(&halt_only);
    printf("NODE_P0 observation_hold_seconds=%u shutdown=%s\n", hold_seconds,
           halt_only ? "halt" : "poweroff");
    (void)fflush(stdout);
    while (hold_seconds > 0U) {
        const struct timespec one_second = {.tv_sec = 1, .tv_nsec = 0};
        (void)nanosleep(&one_second, NULL);
        --hold_seconds;
    }

    if (!halt_only) {
        (void)reboot(LINUX_REBOOT_CMD_POWER_OFF);
    }
    (void)reboot(LINUX_REBOOT_CMD_HALT);
    for (;;) {
        (void)pause();
    }
}
