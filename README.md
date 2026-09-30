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
  evaluation, plus an isolated two-node reference transport;
- a static C11 PID 1 micro-OS and P01 candidate-image tooling;
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
transport, ACS discovery and persistence, MEM persistence, IMM implementation,
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
public proof services, supervises them to terminal states, and emits serial and
structured evidence. It does not by itself accept an assembly generation,
install a system, activate production services, or prove normal-runtime
readiness.

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

The managed development instances are declared by
`config/virtual_nodes/node-001.json` and `node-002.json`. A bounded Python
standard-library controller discovers both profiles and wraps the same
launchers without replacing their boot contract:

```sh
./python/node_lab.py list
./python/node_lab.py status node-001
./python/node_lab.py start node-001
./python/node_lab.py start node-002
./python/node_lab.py events node-001
./python/node_lab.py acs-events node-001
./python/node_lab.py serial node-001
./python/node_lab.py stop node-001
```

`restart`, `acs-events`, and `debug-info` are also available, and
`start NODE --debug` reuses
the paused DEV-001D workflow with profile-specific loopback GDB ports.
Generated PID/state records and per-node serial/event logs live only beneath
`build/virtual/NODE/`. Before stopping an instance, the controller verifies
the recorded PID, Linux process-start identity, managed QEMU marker, canonical
ISO argument, and exact private-network arguments. The two managed guests have
deterministic locally administered MAC addresses and a private point-to-point
Ethernet segment carried by QEMU Unix datagram sockets under `build/virtual/`.
It has no NAT, host bridge, TAP,
internet access, or host-LAN listener. Managed guests use only the fixed
development addresses `10.77.0.1/24` and `10.77.0.2/24` and UDP port `39001`
to exchange a bounded `public.transport.conformance` signal. Both directions
are structurally validated against public ACS registry, lifecycle, admission,
binding, attachment, and envelope evidence. This unencrypted reference path is
isolated development/conformance transport, not peer discovery, authentication,
a production-secure session, a cluster, or a private ACS control plane. The
direct build, normal QEMU, and debug launchers remain independently usable and
retain their no-network default.

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
