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
probes. These remain public conformance probes, not production runtime
providers.

`src/cpu_assembly_service.cpp` is a narrow first-boot adapter over the existing
generic CPU topology and capability providers. On every guest boot it records
fresh, bounded observation and normalized-capability evidence, then passes that
evidence to the pure BOOT-owned evaluator in
`src/node_cpu_assembly_decision.cpp`. The current bounded component set marks
`cpu.runtime` required and `gpu.runtime` not required, with compatibility and
unknown/error states reported separately. This decision does not compile,
install, activate, register, or execute a CPU runtime and does not claim
runtime readiness.

The subsequent `cpu_runtime_assembly` service consumes the exact bounded
decision exported into the current boot's result tmpfs. The canonical host
image build compiles one static x86_64/AVX2 candidate serially and records its
source, toolchain, configuration, SHA-256 digest, size, and build outcomes.
Each guest independently copies that immutable candidate into the executable
`/run` tmpfs, validates its provenance, digest, size, ELF architecture, static
linkage, entry point, and ABI symbol. BOOT then accepts it only for the bounded
test generation and launches it for one bounded probe.
The candidate submits a deterministic AVX2 vector multiply/sum through the
existing one-worker CPU thread pool and must return 120. Candidate production,
BOOT validation, current-boot activation, probe success, and global readiness
remain separate states. No runtime is installed or retained across reboot.

`src/acs_reference_service.cpp` builds a separate static development service.
Explicit mode retains the five-node configured-peer conformance exchange over
UDP port 39001 and exercises the public registry, lifecycle, admission,
binding, attachment, envelope, wire-validation, and structured-evidence paths.
Discovery mode requires no remote peer configuration and uses guest-visible
IPv4 multicast `239.77.0.1:39002` for bounded presence, response, and hint
messages. Its volatile observation store distinguishes direct and hinted
participants, stale and conflict states, and never creates canonical
relationships, connections, attachments, capabilities, or authority. The
current-boot epoch is derived from Linux's boot ID and is not authentication.
Both modes remain isolated, unencrypted reference mechanisms rather than a
production-secure session, private ACS, routing layer, or production provider.

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
