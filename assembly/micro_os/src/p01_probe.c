#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#ifndef NODE_P01_PROBE_KIND
#error NODE_P01_PROBE_KIND must identify the compiled public probe
#endif

#define PROBE_IDENTITY 1
#define PROBE_VOLATILE_FILESYSTEM 2
#define PROBE_DELAY_A 3
#define PROBE_DELAY_B 4
#define PROBE_REQUIRED_SUCCESS 5
#define PROBE_OPTIONAL_FAILURE 6
#define PROBE_TIMEOUT 7
#define PROBE_SIGNAL 8
#define TMPFS_MAGIC 0x01021994L

#if NODE_P01_PROBE_KIND == PROBE_DELAY_A || \
    NODE_P01_PROBE_KIND == PROBE_DELAY_B
static uint64_t monotonic_milliseconds(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return 0;
    }
    return (uint64_t)now.tv_sec * UINT64_C(1000) +
           (uint64_t)now.tv_nsec / UINT64_C(1000000);
}
#endif

#if NODE_P01_PROBE_KIND == PROBE_DELAY_A || \
    NODE_P01_PROBE_KIND == PROBE_DELAY_B || \
    NODE_P01_PROBE_KIND == PROBE_TIMEOUT
static int bounded_sleep(long milliseconds) {
    struct timespec remaining;
    remaining.tv_sec = milliseconds / 1000L;
    remaining.tv_nsec = (milliseconds % 1000L) * 1000000L;
    while (nanosleep(&remaining, &remaining) != 0) {
        if (errno != EINTR) {
            return 1;
        }
    }
    return 0;
}
#endif

#if NODE_P01_PROBE_KIND == PROBE_IDENTITY
static int identity_probe(void) {
    struct utsname identity;
    if (uname(&identity) != 0 || getpid() <= 1) {
        return 20;
    }
    (void)printf("P01_PROBE identity pid=%ld kernel=%s release=%s\n",
                 (long)getpid(), identity.sysname, identity.release);
    return 0;
}
#endif

#if NODE_P01_PROBE_KIND == PROBE_VOLATILE_FILESYSTEM
static int volatile_filesystem_probe(void) {
    const char *host_root = getenv("NODE_P01_HOST_ROOT");
    char directory[256];
    char test_path[320];
    struct statfs filesystem;
    int descriptor;
    const char marker[] = "p01-volatile\n";

    if (host_root != NULL && host_root[0] != '\0') {
        if (snprintf(directory, sizeof(directory), "%s/run/node-p01-results",
                     host_root) >= (int)sizeof(directory)) {
            return 21;
        }
    } else {
        (void)snprintf(directory, sizeof(directory), "/run/node-p01-results");
    }
    if (statfs(directory, &filesystem) != 0) {
        return 22;
    }
    if ((host_root == NULL || host_root[0] == '\0') &&
        filesystem.f_type != TMPFS_MAGIC) {
        return 23;
    }
    if (snprintf(test_path, sizeof(test_path), "%s/probe-%ld.tmp", directory,
                 (long)getpid()) >= (int)sizeof(test_path)) {
        return 24;
    }
    descriptor = open(test_path, O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC, 0600);
    if (descriptor < 0) {
        return 25;
    }
    if (write(descriptor, marker, sizeof(marker) - 1U) !=
        (ssize_t)(sizeof(marker) - 1U)) {
        (void)close(descriptor);
        (void)unlink(test_path);
        return 26;
    }
    if (close(descriptor) != 0 || unlink(test_path) != 0) {
        return 27;
    }
    (void)printf("P01_PROBE volatile_filesystems writable=true\n");
    return 0;
}
#endif

#if NODE_P01_PROBE_KIND == PROBE_DELAY_A || \
    NODE_P01_PROBE_KIND == PROBE_DELAY_B
static int delay_probe(const char *identity) {
    uint64_t started = monotonic_milliseconds();
    (void)printf("P01_PROBE %s event=begin monotonic_ms=%llu\n", identity,
                 (unsigned long long)started);
    (void)fflush(stdout);
    if (bounded_sleep(300L) != 0) {
        return 28;
    }
    (void)printf("P01_PROBE %s event=end monotonic_ms=%llu\n", identity,
                 (unsigned long long)monotonic_milliseconds());
    return 0;
}
#endif

int main(void) {
    (void)setvbuf(stdout, NULL, _IONBF, 0);
#if NODE_P01_PROBE_KIND == PROBE_IDENTITY
    return identity_probe();
#elif NODE_P01_PROBE_KIND == PROBE_VOLATILE_FILESYSTEM
    return volatile_filesystem_probe();
#elif NODE_P01_PROBE_KIND == PROBE_DELAY_A
    return delay_probe("concurrent_delay_a");
#elif NODE_P01_PROBE_KIND == PROBE_DELAY_B
    return delay_probe("concurrent_delay_b");
#elif NODE_P01_PROBE_KIND == PROBE_REQUIRED_SUCCESS
    (void)printf("P01_PROBE required_semantic_success result=declared\n");
    return 0;
#elif NODE_P01_PROBE_KIND == PROBE_OPTIONAL_FAILURE
    (void)fprintf(stderr, "P01_PROBE optional_intentional_failure result=declared\n");
    return 23;
#elif NODE_P01_PROBE_KIND == PROBE_TIMEOUT
    (void)signal(SIGTERM, SIG_IGN);
    (void)printf("P01_PROBE timeout_probe event=waiting\n");
    return bounded_sleep(3000L) == 0 ? 0 : 29;
#elif NODE_P01_PROBE_KIND == PROBE_SIGNAL
    (void)printf("P01_PROBE signal_termination_probe signal=SIGTERM\n");
    (void)raise(SIGTERM);
    return 30;
#else
#error Unsupported NODE_P01_PROBE_KIND
#endif
}
