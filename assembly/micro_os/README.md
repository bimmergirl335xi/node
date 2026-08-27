# Permanent Node micro-OS

This directory contains the public P01 micro-OS foundation. Its static C11
`/init` is the permanent startup implementation for the P01 candidate image;
it is not the RAM Assembly P0 control-transfer payload and it is not the normal
Node runtime.

## P01 structural scope

The implementation proves only that a selected Linux kernel entered this
micro-OS, bounded volatile filesystems were established, the exact tracked P01
startup manifest was structurally accepted, and compiled public probes were
supervised to a bounded terminal state. It does not accept artifacts into a
BOOT assembly generation, install or activate a system, establish recovery,
or report normal-runtime readiness.

`src/node_init.c`:

- must run as PID 1 except in explicit ordinary-user host-test mode;
- mounts devtmpfs, procfs, sysfs, `/run` tmpfs, and a separate 8 MiB result
  tmpfs;
- opens `/dev/console` and reports PID and kernel identity;
- mirrors PID 1 records to an available distinct `ttyS0` diagnostic channel
  while retaining `tty0` as the physical-console target;
- parses only bounded `node.micro_os.*` options;
- loads only `/etc/node-p01/p01-public-startup-v1.manifest` during real boot;
- validates every manifest entry before process creation;
- starts dependency-eligible services in deterministic stages and permits
  same-stage overlap;
- supervises exact executable paths and explicit argument/environment vectors;
- uses process groups, `signalfd`, monotonic deadlines, graceful termination,
  forced termination, and complete `waitpid` reaping;
- reports process outcomes separately from expected semantic results;
- bounds restart policy to two retries;
- emits bounded human-readable lines and JSONL records; and
- never returns from PID 1.

The supervisor accounts for `SIGCHLD`, `SIGTERM`, `SIGINT`, `SIGHUP`,
`SIGUSR1`, and `SIGUSR2`. Signal handlers perform no asynchronous work because
the signals are blocked and consumed through `signalfd`.

## Startup manifest v1

The tracked manifest is
`manifests/p01-public-startup-v1.manifest`. It uses an intentionally small
`key=value` grammar with explicit `service_begin` and `service_end` markers.
Unknown fields, duplicate scalar fields, malformed values, duplicate service
identities, missing or later-stage dependencies, cycles, and exceeded bounds
reject the entire manifest before any service launches.

Each service declares:

```text
identity
revision
executable
stage
required
timeout_ms
expected_result
dependencies
restart_policy
maximum_restart_count
arguments
environment
```

The fixed limits are 16 services, 8 dependencies per service, 8 arguments, 4
allowlisted environment entries, 30-second service deadlines, 35-second stage
deadlines, and two restarts. Executables must be exact paths below
`/node/services/`; `PATH` search, host paths, shell expressions, callbacks,
external components, private components, and directory discovery are absent.

## Public proof services

`src/p01_probe.c` builds eight separate static executables: identity,
volatile-filesystem, two 300 ms concurrent-delay, required semantic-success,
optional intentional-failure, timeout/escalation, and signal-termination
probes. These are public conformance probes, not CPU, GPU, ACS, network, or
production runtime providers.

## Host validation

From the repository root:

```sh
cmake -S . -B build/p01a-host \
  -DPROMETHEUS_BUILD_TESTS=ON \
  -DPROMETHEUS_BUILD_BENCHMARKS=OFF \
  -DPROMETHEUS_BUILD_LEGACY_VISION=OFF \
  -DPROMETHEUS_ENABLE_CUDA=OFF
cmake --build build/p01a-host --parallel 1
ctest --test-dir build/p01a-host -R '^p01_' --output-on-failure
```

Host mode maps manifest paths into the named build root and never mounts a
filesystem or requires root. It is not PID 1 or boot evidence. P01B packages
these same executables and the same tracked manifest into the initramfs.
