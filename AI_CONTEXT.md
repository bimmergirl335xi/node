# Prometheus Node — AI Context

> This file is a current-state engineering handoff, not a full changelog.
> Detailed phase history belongs in `docs/CURRENT_STATE.md` and Git.

## Project

Prometheus is a ground-up distributed embodied cognitive mesh, not an LLM or a
single monolithic model. Specialized neural structures and services may run
across CPU, GPU, accelerator, and edge nodes. Small machines remain complete but
narrower nodes; larger machines may host more, larger, replicated, or
modality-specific specialists.

The runtime is being separated from the legacy CUDA vision program without
changing existing robot behavior.

## Non-Negotiable Architecture

- Production robot behavior remains defined by:

  ```text
  legacy/vision_swarm_11.cu
  ```

- The Raspberry Pi Pico pulse is the authoritative robot cycle source.
- No node, backend, GPU, or service is the permanent cognitive master.
- CUDA is one backend. CPU, ARM, AMD GPU, accelerators, and Xeon Phi are
  first-class targets.
- Hardware assignments use persistent identities, never temporary CUDA
  ordinals, Linux CPU numbers, or discovery order alone.
- Missing telemetry remains explicitly unknown, unsupported, unavailable, or
  failed—never silently healthy.
- Discovery and planning must not automatically activate peer access, allocate
  production model memory, launch kernels, or redirect robot execution.
- A kernel is executable only when registration, device compatibility, release
  family, binary image, and typed adapter are all proven.
- Learning kernels may propose and validate changes but cannot promote them.
  Promotion requires provenance, validation, versioning, and rollback.
- Core runtime code must support minimal headless Linux without GUI, display
  server, desktop services, or required human-readable output.
- Olivia is currently the only authorized operator. Destructive provisioning
  requires explicit approval.
- Placeholder files are not implemented components.
- Use named build directories; do not destructively clear shared build trees.
- The Pocket Decoder/Lens is the planned signed diagnostic, provisioning, and
  recovery interface.

## Platform Context

### Hardware

- Validation host: `curiosity`
- Validation: 2 × Quadro RTX 4000, Turing, compute capability 7.5
- Edge: P106-100, Pascal, normally compute capability 6.1
- Main server: Tesla V100, Volta, compute capability 7.0
- Compatibility case: Quadro K6000, Kepler
- Additional targets: x86 Xeon, ten unidentified-family Xeon Phi devices,
  Raspberry Pi, Hailo, IMX500, and future AMD GPUs

Current validation GPU identities:

```text
GPU-888ada1c-55ed-66cd-69ce-f0719ceedc4b  PCI 0000:2d:00.0
GPU-4ced1003-8696-6957-d524-1539252d8c8d  PCI 0000:2e:00.0
```

Persistent CUDA keys use `cuda:<GPU-UUID>`. Runtime ordinals must be rebound to
UUIDs at startup.

### CUDA Policy

- Current bench: CUDA toolkit/runtime/driver level 12.4; compiler build
  12.4.131.
- CUDA 12 family: Pascal, Volta, Turing, and newer explicitly compiled targets.
  Mixed Pascal/Volta/Turing processes should use this family.
- CUDA 13 family: Turing and newer supported architectures.
- Pascal and Volta remain on CUDA 12.
- Toolkit compatibility does not prove binary-image or typed-adapter coverage.

### Minimal Nodes and Provisioning

Nodes may run extremely small headless Linux installations containing only
boot, networking, drivers, in-RAM services, mesh communications, structured
diagnostics, and safe shutdown/recovery.

Drivers, CUDA toolkits, firmware, compilers, OS packages, boot configuration,
storage layouts, and services belong in signed provisioning manifests—not pip
requirements. Provisioning must inventory hardware, propose deliberate roles,
require approval for destructive actions, validate installation, and support
rollback.

## Completed Kernel Foundation

Reported passing before backend work:

- activation/reduction;
- FP32, FP64, and integer dense operations;
- LayerNorm and RMSNorm;
- temporal state and prediction;
- Hebbian, Oja, and predictive-delta learning;
- SGD, momentum, RMSProp, and Adam optimizer state;
- candidate validation and benchmark summaries.

Optimizer behavior includes masks, scaling/clipping, decoupled weight decay,
FP32/FP64, state reset, device step counters, and explicit shadow/apply modes.
Shadow mode must not mutate parameters or optimizer state.

Candidate validation records non-finite, changed, and saturation counts; norms;
maximum magnitudes; rejection masks; and candidate metadata. Promotion authority
remains outside kernels.

## Backend Status

### Phase 6 — CUDA Backend: Complete

Implemented under `src/backends/` and `src/backends/cuda/`:

- generic non-singleton `ComputeBackend` and `BackendRegistry`;
- CUDA profiles and capability reporting;
- stable UUID discovery and ordinal rebinding;
- refreshable health with dynamically loaded NVML;
- explicit unsupported/unknown telemetry states;
- UUID-keyed multi-device inventory and advisory placement;
- spread, pack, stable-order, allowlist, commitment, and memory policies;
- directed peer-capability reporting without automatic peer activation;
- metadata-first CUDA kernel registry;
- per-device execution-readiness evaluation;
- clean backend lifecycle and shutdown.

The backend does not take over the robot loop, launch production kernels,
permanently select a GPU, enable peer access, reset GPUs, or assume one
monolithic model.

Validation on both RTX 4000s passed discovery, health, capacity planning, kernel
coverage evaluation, registration, readiness, planning, and shutdown. Root
CMake/CTest integration passed:

```text
build/phase-6-integrated
PROMETHEUS_CUDA_ARCHITECTURES=75
PROMETHEUS_BUILD_TESTS=ON
PROMETHEUS_BUILD_BENCHMARKS=ON
PROMETHEUS_BUILD_LEGACY_VISION=OFF

7/7 tests passed
```

Build rules:

- link `Threads::Threads` and `CUDA::cudart`;
- use CUDA separable compilation;
- do not add unconditional `-ldl`;
- on the current glibc bench, do not append `-ldl` to direct `nvcc` probe
  commands because it caused `nvlink` to inspect an incompatible host archive.

The only listed Phase 6 archival loose end was a focused Git diff.

### Phase 7.1 — CPU Backend Foundation: Complete

Implemented CPU identity, topology, capabilities, health, capacity, backend
lifecycle, optional probe, and three backend tests under:

```text
src/backends/cpu/
tools/cpu_backend_probe.cpp
tests/unit/backends/test_cpu_*.cpp
```

Key decisions:

- CPU identity is node-relative for now; future durable identity combines the
  stable node identity with package/core/thread topology keys.
- Configured, online, and process-allowed CPUs are distinct; affinity and
  cpusets are honored.
- Discovery reports packages, cores, hardware threads, SMT, NUMA, cache sharing,
  core-type hints, and capacity hints where available.
- Capabilities record common and any-target ISA support across allowed CPUs.
- Health is refreshable and reports utilization baselines/deltas, frequency,
  load, memory/swap, pressure, thermal providers, timestamps, and explicit
  telemetry states.
- Capacity reports logical/physical resources, NUMA, common safe SIMD, memory,
  and heterogeneous execution groups.
- Planning is advisory only. Default policy reserves nothing, proposes one lane
  per physical core, and does not count SMT siblings as full lanes.
- CPU and CUDA coexist in the same explicit `BackendRegistry`.
- Core backend files remain silent/headless; `prometheus_cpu_backend_probe` is
  an optional maintenance target.

Reported validation covered discovery, lifecycle, health/capacity,
CPU/CUDA coexistence, affinity-restricted behavior, and requested CPU/full
CTest sequences. Final closeout requires the standalone CPU probe to exit 0.

On the 16 GiB validation host, build serially:

```text
cmake --build <named-build-directory> --parallel 1
```

Unrestricted parallel compilation caused a system OOM kill of VS Code; this was
a build-concurrency event, not evidence of a backend runtime leak.

Not yet implemented:

- worker-pool lifecycle;
- bounded queues/backpressure;
- affinity binding or work stealing;
- SIMD execution dispatch;
- CPU kernel registry or typed adapters;
- CPU reference implementations;
- neural scheduling/storage;
- production dispatch or robot-path migration.

## Current Work — Phase 7.2

Implement the CPU execution substrate:

- bounded worker pools;
- one group per advisory execution group or NUMA domain;
- local queues with explicit backpressure;
- clean start, drain, stop, and shutdown;
- policy-controlled optional affinity;
- local-first scheduling with bounded cross-group stealing;
- no reference-operation or SIMD-execution claims before typed adapters exist.

### Files Needed

```text
AI_CONTEXT.md
docs/CURRENT_STATE.md
docs/PROJECT_TREE.md
CMakeLists.txt
src/backends/CMakeLists.txt
src/backends/compute_backend.hpp
src/backends/backend_registry.hpp
src/backends/backend_registry.cpp

src/backends/cpu/CMakeLists.txt
src/backends/cpu/cpu_backend.hpp
src/backends/cpu/cpu_backend.cpp
src/backends/cpu/cpu_topology.hpp
src/backends/cpu/cpu_topology.cpp
src/backends/cpu/cpu_capacity.hpp
src/backends/cpu/cpu_capacity.cpp
src/backends/cpu/cpu_health.hpp
src/backends/cpu/cpu_health.cpp
src/backends/cpu/cpu_thread_pool.hpp
src/backends/cpu/cpu_thread_pool.cpp

tests/unit/backends/CMakeLists.txt
tests/unit/backends/test_cpu_backend_registry.cpp
tests/unit/backends/test_cpu_health_capacity.cpp
tools/cpu_backend_probe.cpp
```

### Opening Message

> We completed Prometheus Phase 7.1: node-relative CPU identity, Linux topology,
> affinity-aware ISA capabilities, refreshable health, NUMA-aware capacity
> groups, advisory worker-lane planning, generic CPU backend integration, and a
> standalone diagnostic probe. CPU and CUDA register together, robot behavior
> remains unchanged, and `legacy/vision_swarm_11.cu` remains authoritative.
> Please read `AI_CONTEXT.md` and `docs/CURRENT_STATE.md` first. We are beginning
> Phase 7.2: bounded CPU worker pools, execution-group queues, lifecycle,
> backpressure, and NUMA-local scheduling foundations. Do not claim reference
> operations or SIMD execution before typed adapters exist.

## Development Process

Each phase has five substantive implementation chats. Setup, handoff, file
collection, ordinary validation, compiler/linker correction, and focused
debugging do not consume a slot.

At phase close, update `AI_CONTEXT.md` and `docs/CURRENT_STATE.md`. Documentation
changes are append-only addenda unless Olivia explicitly requests a rewrite.
Handoffs must preserve architecture decisions, completed files, validation,
limitations, unfinished placeholders, exact commands, next-chat files, and a
clean opening message.
