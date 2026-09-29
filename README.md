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
  evaluation;
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

Production scheduling, general CPU/GPU kernel dispatch, live ACS connections,
ACS transport and persistence, MEM persistence, IMM implementation, production
assembly authority, installation/recovery, and migration away from the legacy
robot loop are not complete.

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
