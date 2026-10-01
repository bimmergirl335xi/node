# Node

Node is a portable, headless operating environment for heterogeneous machines
participating in a distributed embodied-cognition system. The repository
combines a Linux-based boot path, a minimal C micro-OS, a developing runtime,
hardware backends, public assembly mechanisms, and public architecture
contracts.

The project is pre-production. Its foundations are implemented in useful,
testable slices, but Node is not yet a complete production runtime and does not
replace the legacy robot loop.

## Maturity at a glance

### Implemented

- generic CPU discovery, health/capacity reporting, bounded worker execution,
  and conservative SIMD selection;
- CUDA discovery, capability/health reporting, kernel metadata, bounded job
  queues, evidence correlation, and a device-local execution worker;
- Linux PCI GPU inventory that works without CUDA;
- service lifecycle, adaptive-state, architecture-shadow, and proposal-ABI
  foundations;
- bounded public ACS descriptors, registry, lifecycle state, and pure admission
  evaluation, plus bounded volatile local-participant discovery and an isolated
  five-node explicit-peer reference transport;
- a static C11 permanent PID 1 micro-OS, P01 candidate-image tooling, a
  bounded first-boot CPU discovery/assembly/activation proof, and an explicit
  current-boot resident lab mode that supervises the selected CPU runtime and
  public ACS reference transport;
- a RAM-only assembly mechanism proof and a public external-component ABI.

### Validated in earlier development checkpoints

Focused CPU, CUDA, runtime, ACS, assembly, and P01 tests have recorded passing
results on their documented hosts. Those records are historical evidence, not
a guarantee for every checkout or machine. See [Current State](docs/CURRENT_STATE.md)
for the present baseline and validation caveats.

### Experimental

- the P01 bootable x86_64 GRUB hybrid ISO path;
- external-component declaration and assembly-provider boundaries;
- public ACS, memory, and immune architecture specifications;
- accelerator stubs and the legacy CUDA robot program.

### Planned or incomplete

Production scheduling, general CPU/GPU kernel dispatch, production-secure ACS
transport, authenticated/wide-area ACS discovery and persistence, MEM persistence, IMM implementation,
production assembly authority, installation/recovery, and migration away from
the legacy robot loop are not complete.

## Boot and runtime architecture

The intended public path is:

```text
Linux kernel
    -> Node micro-OS (PID 1)
    -> public Node runtime substrate
    -> public ACS substrate
    -> optional externally supplied components
```

Linux remains responsible for mature kernel facilities and drivers unless a
specific ownership contract says otherwise. The permanent P01 micro-OS mounts
bounded volatile filesystems, validates a fixed startup manifest, launches
public proof services, and emits serial and structured evidence. In the
default conformance mode it supervises services to a terminal state and powers
off. In explicitly selected lab mode it admits a scoped current-boot
transition only after CPU and ACS evidence passes, keeps those two services
under the same PID 1, and remains resident until stopped. This does not accept
a production assembly generation, install a system, or claim readiness for
future Node subsystems.

## CPU and GPU model

CPU-only operation is a supported state. ARM observations enrich the generic
CPU backend rather than creating a second backend for the same processor.
Affinity, NUMA pool orchestration, typed CPU kernels, and production CPU
dispatch remain incomplete.

GPU discovery separates physical hardware, kernel-driver binding, runtime
visibility, binary compatibility, backend readiness, and admission. PCI
visibility is not CUDA readiness. CUDA is optional at configuration time, GPU
absence is valid, and no public path automatically installs drivers or grants a
device production authority.

## ACS direction

The Adaptive Connection Substrate (ACS) is intended to describe governed,
bounded relationships between nodes, services, hardware, and cognitive
structures without equating a relationship with a socket or transport.
`src/core/acs/` contains the current public runtime-local foundation, while
`docs/architecture/acs/` contains the public Draft specification series.
Private deployment policy and proprietary ACS implementation do not belong in
this repository.

## Repository map

- `assembly/` — micro-OS, P01 boot tooling, RAM-only proof, profiles, and
  public providers;
- `src/` — runtime, hardware discovery, and CPU/CUDA/accelerator backends;
- `interfaces/` — public C-compatible interfaces;
- `tests/` and `benchmarks/` — test and benchmark sources;
- `docs/architecture/` — public ACS, memory, and immune specifications;
- `docs/CURRENT_STATE.md` — current integrated implementation snapshot;
- `docs/PROJECT_TREE.md` — task-oriented navigation map;
- `tools/`, `scripts/`, and `python/` — developer utilities;
- `legacy/` — production robot behavior retained during migration;
- `build/` — ignored, generated, disposable output.

## Branch model

`main` is the stable/public branch. `dev` is the active integration branch and
may temporarily be incomplete. Subsystem work normally moves from a specialized
lane (`lane/cpu`, `lane/gpu`, `lane/apu`, `lane/phi`, `lane/runtime`, or
`lane/docs`) into `dev`, is integrated and validated there, and only then moves
to `main`. `lane/tmp` is preservation/quarantine only.

## Development workflow

Read `AGENTS.md`, `docs/PROJECT_TREE.md`, and `docs/CURRENT_STATE.md` before
changing code. Work in the owning subsystem, keep CPU-only paths independent of
CUDA, add focused tests for changed behavior, and keep generated output under a
named `build/` directory.

The canonical host-native boot build reuses the validated P01 image machinery:

```sh
./scripts/build-node.sh
```

It requires the exact external Linux kernel checkout documented in
`assembly/p01_boot/README.md` (override its default location with
`--kernel-source PATH`). Generated state is disposable and remains under
`build/`; the successfully validated development ISO is always promoted to
`build/artifacts/node-current.iso`.

Boot that canonical ISO in the minimal headless development guest with:

```sh
./scripts/run-node-qemu.sh
```

BIOS is the default; `--uefi` uses an available OVMF installation and a
disposable variable-store copy. Both modes use serial output, retain the most
recent transcript at `build/logs/qemu-last-run.log`, and create no virtual
disk or persistent guest state. The launcher also extracts the micro-OS's
existing bounded JSON records to `build/logs/qemu-boot-events.jsonl`, one
event per line with `record`, `subject`, `outcome`, and normally `detail`
fields. This observation artifact is intended for development automation; it
does not grant boot acceptance or runtime readiness.

For an opt-in debugging session, start the same ISO paused in QEMU:

```sh
./scripts/debug-node-qemu.sh
```

Then attach from a second terminal and explicitly continue the guest:

```text
gdb build/artifacts/vmlinux
(gdb) target remote 127.0.0.1:1234
(gdb) continue
```

The debug endpoint is loopback-only, the default session timeout is ten
minutes, and `--port PORT`, `--bios`, and `--uefi` are supported. Normal boot
does not expose a debug endpoint or start paused. A host GDB client is required
and remains separate from the launcher. For VSCode, use the existing GDB
extension with `build/artifacts/vmlinux` as the program and
`127.0.0.1:1234` as the remote target, then continue execution from the
debugger UI; no workspace configuration is required or tracked.

For a managed multi-node debugging session, launch the selected lab profiles
paused and attach the bounded GDB/MI fleet controller:

```sh
./python/node_lab.py start-all --debug --mode lab
./python/node_gdb_fleet.py
```

The controller discovers node identities and loopback GDB ports from the
existing profiles, requires `build/artifacts/vmlinux`, and owns one GDB/MI3
subprocess per selected node. GDB commands apply to the current selection;
`select NODE`, `select all`, and `@NODE[,NODE] COMMAND` provide persistent and
one-shot targeting. `status`, `nodes`, `reconnect`, `disconnect`, `help`, and
`quit` are fleet commands. Asynchronous running, stopped, breakpoint, signal,
thread, exit, and connection events retain node attribution. Exiting the
frontend reaps only its GDB processes and never stops the managed VMs.

The twenty-five managed development instances are declared under
`config/virtual_nodes/`. A bounded Python standard-library controller discovers
the profiles, validates their explicit socket/core/thread topology, and wraps the same
launchers without replacing their boot contract:

```sh
./python/node_lab.py list
./python/node_lab.py start-all --mode lab
./python/node_lab.py start node-001 --mode lab --acs-mode discovery
./python/node_lab.py group-status
./python/node_lab.py runtime-summary
./python/node_lab.py runtime-status node-001
./python/node_lab.py status node-001
./python/node_lab.py start node-001
./python/node_lab.py events node-001
./python/node_lab.py acs-events node-001
./python/node_lab.py discovery node-001
./python/node_lab.py cpu-info node-001
./python/node_lab.py cpu-summary
./python/node_lab.py cpu-runtime node-001
./python/node_lab.py cpu-runtime-summary
./python/node_lab.py serial node-001
./python/node_lab.py stop node-001
./python/node_lab.py restart-all
./python/node_lab.py stop-all
```

`restart`, `acs-events`, `discovery`, `cpu-info`, `cpu-summary`, `cpu-runtime`,
`cpu-runtime-summary`, `runtime-status`, `runtime-summary`, and `debug-info`
are also available, and
`start NODE --debug` reuses
the paused DEV-001D workflow with profile-specific loopback GDB ports.
Generated PID/state records and per-node serial/event logs live only beneath
`build/virtual/NODE/`. Before stopping an instance, the controller verifies
the recorded PID, Linux process-start identity, managed QEMU marker, canonical
ISO argument, and exact private-network arguments. The first five managed guests
have one, two, or four profile-driven vCPUs spanning single-core, multi-core,
SMT, and multi-socket cases; the twenty scale participants each have one vCPU.
QEMU receives the explicit topology, while `cpu-info`
and `cpu-summary` report fresh guest observations plus the bounded resident CPU
worker count and deterministic per-worker probe result. The twenty-five guests also have
deterministic locally administered MAC and IPv4 addresses on one QEMU datagram
multicast Ethernet segment confined to host loopback. It has no NAT, host bridge, TAP,
internet access, or host-LAN listener. Managed guests use only the fixed
development range `10.77.0.1/24` through `10.77.0.25/24`. Explicit mode uses UDP
port `39001` to exchange bounded `public.transport.conformance` signals. All 20 directed paths
are structurally validated against public ACS registry, lifecycle, admission,
peer-specific binding, attachment, correlation, and envelope evidence. `start-all`
and `restart-all` launch concurrently; `group-status` uses structured explicit
peer-validation records in explicit mode and reconstructs distinct direct,
hint, stale, and conflict observation counts in discovery mode. Discovery mode
passes zero remote peers and uses guest-local multicast `239.77.0.1:39002` for
bounded presence, response, and participant-hint traffic. Observations are
volatile, expire to stale, and create no relationship, connection, attachment,
trust, capability, or authority. Both unencrypted paths are isolated
development transports, not authentication, a production-secure session, a
cluster, or a private ACS control plane. The direct build, normal QEMU, and
debug launchers remain independently usable and retain their no-network default.

DEV-004A host validation ran all ten discovery-driven guests concurrently. All
ten activated their profile-driven CPU runtime, reached resident readiness, and
directly observed the other nine participants, producing 90 directed volatile
observations. A node-007 stop produced stale observations without degrading the
survivors; its restart restored direct observations through bounded
rediscovery. Explicit mode remains available for deterministic five-node
conformance. This validation established no trust, canonical relationship,
mesh route, authority, or persistent membership.

DEV-004B extends the configured discovery lab to 25 profiles. Nodes 011–025
follow the lightweight one-vCPU, 256 MiB pattern with no explicit peers. This
configuration targets 24 direct observations per Node, or 600 directed volatile
observations fleet-wide; runtime validation remains separate from configuration.

ACS-001A adds no trust scoring, operator authority, source or artifact
propagation, mesh routing, or persistent discovery state.

Managed `start`, `restart`, `start-all`, and `restart-all` accept an explicit
`--mode conformance` or `--mode lab` and an optional `--acs-mode explicit` or
`--acs-mode discovery`; the ten fleet profiles default to discovery while the
original five retain their explicit peer sets for deterministic override. Lab mode
selects resident behavior through bounded QEMU platform metadata while the
same `build/artifacts/node-current.iso` is used in both modes. After the
initial CPU probe and ACS local readiness (plus complete configured exchange in
explicit mode), PID 1 evaluates a separate
transition gate, reports scoped runtime readiness, requests a second CPU work
probe, and continues supervising the live CPU worker and ACS socket.
`runtime-status` and `runtime-summary` report only observed structured records
and managed-process identity. Runtime state is volatile, and the current stop
command still performs an exact-identity bounded QEMU termination rather than
an in-guest control request.

Each managed guest also runs the existing generic CPU topology/capability
provider during first boot. Bounded JSONL records keep raw observation,
normalized capability, capability-driven profile evaluation, and the BOOT-owned
component decision distinct. The canonical serial image build compiles one
static x86_64/AVX2 conformance candidate. Each guest independently consumes
its decision, materializes that candidate into bounded volatile storage,
validates its digest, ELF structure, ABI symbol, and profile, launches it for
the current boot after bounded BOOT acceptance, and executes a deterministic
AVX2 workload through the bounded CPU worker pool. `cpu-runtime NODE` and
`cpu-runtime-summary` expose those stages without claiming global runtime
readiness. This path installs
nothing durably and `gpu.runtime` remains not required.

Component development remains CMake-based. A CPU-only configuration can be
requested explicitly:

```sh
cmake -S . -B build/local \
  -DPROMETHEUS_ENABLE_CUDA=OFF \
  -DPROMETHEUS_BUILD_TESTS=ON \
  -DPROMETHEUS_BUILD_BENCHMARKS=OFF \
  -DPROMETHEUS_BUILD_LEGACY_VISION=OFF
cmake --build build/local --parallel 1
ctest --test-dir build/local --output-on-failure
```

CUDA builds require a compatible compiler, toolkit, driver, GPU, and explicitly
selected architecture list. The current P01 image workflow is documented in
`assembly/p01_boot/README.md`; it requires an exact external Linux-kernel
checkout and may report ISO/firmware validation as unavailable when host tools
are missing. QEMU validates the actual boot path. Docker is not required for
the normal development loop.

## Documentation

- [Agent instructions](AGENTS.md)
- [Current integrated state](docs/CURRENT_STATE.md)
- [Repository navigation](docs/PROJECT_TREE.md)
- [Durable technical context](AI_CONTEXT.md)
- [Assembly environment](assembly/README.md)
- [ACS specifications](docs/architecture/acs/README.md)
- [Memory specifications](docs/architecture/memory/README.md)
- [Immune specifications](docs/architecture/immune/README.md)
