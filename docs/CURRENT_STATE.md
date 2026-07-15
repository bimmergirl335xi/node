# Current State

## Summary

Prometheus kernel Phases 1 through 5 are reported complete through candidate
validation. Phase 6 is implemented and has passed direct bench probes on a
dual-Quadro-RTX-4000 CUDA 12.4 system.

Phase 6 establishes CUDA as one optional compute backend without redirecting
robot execution away from the legacy source of truth.

Prometheus remains a distributed cognitive neural mesh. Capacity scaling means
running fewer, smaller, or narrower whole specialist instances on smaller
machines and more or larger specialists on larger machines. Cross-GPU
tensor/model sharding is a separate optional future capability.

## Preserved Safety and Architecture Boundaries

- Existing robot-facing behavior remains unchanged.
- `legacy/vision_swarm_11.cu` remains the current robot behavior source of
  truth.
- The Pico remains the authoritative cycle source.
- CUDA is not the cognitive master.
- No GPU is selected as a permanent master.
- Runtime ordinals are not durable identities.
- Discovery does not launch production kernels.
- Device-pool planning does not allocate production memory.
- Peer support is queried but not activated automatically.
- Health telemetry does not silently convert unknown values into healthy.
- Core backend code requires no GUI or display server.
- Placeholder files are not considered implemented.
- Kernel metadata alone is not considered executable coverage.
- Model-promotion authority remains outside kernels.

## Confirmed Before Phase 6

### Phase 1

Activation and reduction kernels.

### Phase 2

Dense FP32, FP64, and integer accumulation.

### Phase 3

LayerNorm and RMSNorm.

### Phase 4

Temporal state and prediction operations.

### Phase 5.1

Local Hebbian, Oja, and predictive-delta learning.

### Phase 5.2

Optimizer state and flat-parameter update API:

- signal accumulation
- SGD
- momentum
- RMSProp
- Adam
- ascent/descent
- signal, delta, and parameter clipping
- decoupled weight decay
- masks
- shadow and applied modes
- optimizer-state reset
- step-counter utilities
- FP32 and FP64

Shadow mode does not mutate parameters or optimizer state.

### Phase 5.3

Reported complete before Phase 6:

- candidate-update validation
- non-finite detection
- norm and magnitude summaries
- saturation counts
- changed-parameter counts
- rejection bitmasks
- candidate metadata/manifests
- no promotion policy inside kernels

The user reported all tests pass through candidate validation.

## Phase 6.1 — CUDA Profiles and Capabilities

### Added

```text
src/backends/cuda/cuda_profiles.hpp
src/backends/cuda/cuda_profiles.cpp
src/backends/cuda/cuda_capabilities.hpp
src/backends/cuda/cuda_capabilities.cu
tools/cuda_capability_probe.cu
```

### Implemented

- architecture-family classification
- compute-capability representation
- CUDA 12 legacy-family classification
- CUDA 13 modern-family classification
- project support policy
- tri-state support values
- immutable hardware capability reporting
- compile-time/runtime/driver CUDA version reporting
- explicit unknown binary-image and kernel-registry coverage

### Bench Validation

```text
Visible devices: 2
Both devices: Quadro RTX 4000
Compute capability: 7.5
Architecture: Turing
Toolkit/runtime/driver-supported CUDA: 12.4
Probe exit status: 0
```

## Phase 6.2 — Stable Identity and Discovery

### Added

```text
src/backends/cuda/cuda_device.hpp
src/backends/cuda/cuda_device.cu
tools/cuda_device_discovery_probe.cu
```

### Implemented

- canonical CUDA UUID formatting
- persistent Prometheus keys derived from UUID
- temporary ordinal snapshots
- PCI location reporting
- per-device discovery status
- partial-failure retention
- fatal identity/capability rejection without silently dropping diagnostics
- backend-registration readiness

### Bench Validation

```text
Discovery status: success
Reported visible devices: 2
Stable identities: 2
Capability records: 2
Registration-ready records: 2
Probe exit status: 0
```

Observed identities:

```text
cuda:GPU-888ada1c-55ed-66cd-69ce-f0719ceedc4b
  PCI 0000:2d:00.0

cuda:GPU-4ced1003-8696-6957-d524-1539252d8c8d
  PCI 0000:2e:00.0
```

## Phase 6.3 — Health and Observability

### Added

```text
src/backends/cuda/cuda_health.hpp
src/backends/cuda/cuda_health.cu
tools/cuda_health_probe.cu
```

### Implemented

- UUID-resolved NVML health provider
- runtime ordinal/UUID revalidation
- timestamped health snapshots
- total/free/used memory
- temperature
- power
- GPU and memory utilization
- ECC and retired-page reporting when supported
- raw clock-event mask
- explicit unsupported/provider-unavailable/query-failure states
- safe CUDA memory fallback
- preservation of the caller's selected CUDA device

### Important Build Correction

On the current glibc system, the direct `nvcc` probe command must not include:

```text
-ldl
```

Including it caused:

```text
nvlink warning: Skipping incompatible
/usr/lib/x86_64-linux-gnu/libdl.a when searching for -ldl
```

Removing `-ldl` produced compile exit status 0. Dynamic loading continued to
work.

### Bench Validation

Both devices reported:

```text
runtime binding: matched
NVML provider: ready
query: success
issues: none
```

Memory, temperature, power, GPU utilization, and memory utilization were
reported independently. Unsupported ECC and retired-page queries remained
explicitly unsupported.

Probe exit status: 0.

## Phase 6.4 — Multi-Device Pool and Capacity Model

### Added

```text
src/backends/cuda/cuda_device_pool.hpp
src/backends/cuda/cuda_device_pool.cu
tools/cuda_device_pool_probe.cu
```

### Implemented

- complete CUDA inventory
- UUID-keyed lookup
- capability and health snapshots
- static and observed memory summaries
- heterogeneous-architecture detection
- directed peer-capability matrix
- whole-specialist placement planning
- `spread`, `pack`, and `stable_order` policies
- per-request device allowlists
- existing advisory memory commitments
- observed-memory requirement/fallback policy
- no automatic peer activation
- no production allocation

### Bench Validation

```text
Pool status: success
Visible devices: 2
Stable identities: 2
Registration-ready devices: 2
Usable runtime bindings: 2
Observed-memory devices: 2
Heterogeneous architectures: no
Total static GPU memory: 15.56 GiB
Observed free GPU memory: 14.76 GiB
```

Directed peer access was reported supported in both directions.

Planning results:

```text
Four independent 256 MiB specialists:
  2 instances on each GPU
  unplaced: 0

Two independent 128 MiB specialists restricted to one UUID:
  2 instances on selected GPU
  unplaced: 0
```

Probe exit status: 0.

## Phase 6.5 — Backend and Kernel Registry Integration

### Added or Replaced

```text
src/backends/compute_backend.hpp
src/backends/backend_registry.hpp
src/backends/backend_registry.cpp

src/backends/cuda/cuda_backend.hpp
src/backends/cuda/cuda_backend.cu
src/backends/cuda/cuda_kernel_registry.hpp
src/backends/cuda/cuda_kernel_registry.cu

src/backends/CMakeLists.txt
src/backends/cuda/CMakeLists.txt

tools/cuda_backend_registry_probe.cu

tests/unit/backends/test_cuda_backend_registry.cu
tests/unit/backends/CMakeLists.txt

requirements.txt
docs/development/dependencies.md
```

### Generic Backend Registry

Implemented:

- generic backend kind/state/snapshot vocabulary
- explicit ownership
- duplicate-ID rejection
- no global singleton
- no hardware initialization during registration
- room for CPU, ARM, AMD GPU, accelerator, Xeon Phi, and future backends

### CUDA Backend

Implemented:

- discovery-backed initialization
- ready/degraded/unavailable/stopped state reporting
- pool refresh
- health/capacity snapshot exposure
- whole-specialist placement planning
- per-device kernel-coverage evaluation
- clean inventory shutdown

Not implemented:

- production kernel launches
- production task dispatch
- robot-path migration
- automatic peer enablement
- GPU reset
- permanent GPU selection

### CUDA Kernel Registry

Implemented metadata-first descriptors containing:

- kernel ID
- kernel family
- scalar types
- compute-capability range
- CUDA release-family compatibility
- binary-image state
- typed-adapter state
- provenance

Execution readiness requires:

```text
registered ID
compatible architecture
compatible release family
proven binary image
proven typed adapter
```

Metadata-only entries remain non-executable.

### Bench Validation

```text
Backend registration: success
Backend state: ready
Backend usable: yes
Registered backends: 1
CUDA devices in snapshot: 2

Synthetic ready descriptor:
  coverage: complete
  execution ready: yes

Metadata-only descriptor:
  coverage: unknown
  execution ready: no
  reason: compatible binary image not proven

Placement assignments: 2
Unplaced: 0

Final backend state: stopped
Phase 6.5 probe passed
Probe exit status: 0
```

## Dependency State

`requirements.txt` now exists.

Phase 6 adds no mandatory Python package, so it currently contains documentation
comments rather than packages.

Dependency ownership:

```text
requirements.txt
  direct Python packages

requirements-lock.txt
  reproducible Python package versions

future signed provisioning/release manifests
  CUDA/ROCm toolkits
  NVIDIA/AMD drivers
  NVML/management libraries
  kernel modules
  firmware
  compilers
  CMake
  OS packages
  filesystems
  boot configuration
  services
  storage roles/layouts
```

## Minimal Headless Runtime Requirement

Core backend files:

- print nothing by default;
- require no GUI;
- require no display server;
- require no desktop services;
- perform no interactive prompting;
- retain state in memory;
- expose structured snapshots to future diagnostics and messaging layers.

Human-readable output exists only in standalone probes.

Future Pocket Decoder/Lens integration will provide the intended operator-facing
diagnostic/provisioning interface.

## Final Phase 6 CMake and CTest Integration

Root integration is complete.

Applied:

```text
find_package(Threads REQUIRED)
add_subdirectory(src/backends)
add_subdirectory(tests/unit/backends) inside the test-enabled branch
```

Backend linkage now includes:

```text
prometheus_backend_core -> Threads::Threads
prometheus_cuda_backend -> prometheus_backend_core + CUDA::cudart
```

CUDA separable compilation is enabled for the CUDA backend target. No
unconditional `-ldl` linkage was added.

Validated build directory:

```text
build/phase-6-integrated
```

Validation configuration:

```text
PROMETHEUS_CUDA_ARCHITECTURES=75
PROMETHEUS_BUILD_TESTS=ON
PROMETHEUS_BUILD_BENCHMARKS=ON
PROMETHEUS_BUILD_LEGACY_VISION=OFF
```

Final CTest result:

```text
1/7 kernel_foundation passed
2/7 dense_kernels passed
3/7 normalization_kernels passed
4/7 temporal_kernels passed
5/7 learning_kernels passed
6/7 optimizer_kernels passed
7/7 prometheus_cuda_backend_registry passed

100% tests passed
0 tests failed
Total real test time: 2.27 seconds
```

Phase 6 is now integrated into the normal build and test graph. The remaining
closeout item is a focused Git diff for archival/review.

## Current Source of Truth

```text
legacy/vision_swarm_11.cu
```

## Next Phase

Phase 7: CPU backend equivalents and reference operations.

### Phase 7.1 Initial Scope

- CPU identity
- CPU topology
- stable processor/package/core/thread representation
- ISA and SIMD capabilities
- NUMA awareness where available
- memory/capacity reporting
- minimal-headless operation
- CPU backend state vocabulary aligned with Phase 6
- no production robot-path change
- no assumption that CPU is merely a CUDA fallback

## Files Required for Phase 7.1

```text
AI_CONTEXT.md
docs/CURRENT_STATE.md
CMakeLists.txt
src/backends/CMakeLists.txt
src/backends/compute_backend.hpp
src/backends/backend_registry.hpp
src/backends/backend_registry.cpp
src/backends/cuda/cuda_backend.hpp
src/backends/cuda/cuda_backend.cu
src/backends/cuda/cuda_device_pool.hpp
src/backends/cuda/cuda_kernel_registry.hpp

src/backends/cpu/cpu_backend.hpp
src/backends/cpu/cpu_backend.cpp
src/backends/cpu/cpu_capabilities.hpp
src/backends/cpu/cpu_capabilities.cpp
src/backends/cpu/cpu_thread_pool.hpp
src/backends/cpu/cpu_thread_pool.cpp
src/backends/cpu/simd_dispatch.hpp
src/backends/cpu/simd_dispatch.cpp
```

Also provide the kernel headers selected for the first CPU-reference operations,
the Phase 6 root-CMake/CTest output after integration, and a focused Git diff.

## Suggested Opening Message for Phase 7.1

> We completed Prometheus Phase 6: CUDA profiles, stable UUID identity,
> health, multi-device capacity planning, generic backend integration, and a
> metadata-first kernel registry. Direct probes pass on two Quadro RTX 4000s
> under CUDA 12.4. Please read `AI_CONTEXT.md` and
> `docs/CURRENT_STATE.md` first. We are beginning Phase 7.1: CPU identity,
> topology, capabilities, and reference-operation foundations. Preserve robot
> behavior, keep CPU first-class, support minimal headless nodes, keep CUDA as
> one backend among many, and do not treat placeholders as implemented.

## Phase 7.1 — CPU Identity, Topology, Health, Capacity, and Backend Foundation

Phase 7.1 establishes CPU as a first-class Prometheus compute backend without
making it a fallback after CUDA and without redirecting robot execution.

### Implemented Files

```text
src/backends/cpu/CMakeLists.txt
src/backends/cpu/cpu_identity.hpp
src/backends/cpu/cpu_identity.cpp
src/backends/cpu/cpu_topology.hpp
src/backends/cpu/cpu_topology.cpp
src/backends/cpu/cpu_capabilities.hpp
src/backends/cpu/cpu_capabilities.cpp
src/backends/cpu/cpu_health.hpp
src/backends/cpu/cpu_health.cpp
src/backends/cpu/cpu_capacity.hpp
src/backends/cpu/cpu_capacity.cpp
src/backends/cpu/cpu_backend.hpp
src/backends/cpu/cpu_backend.cpp

tools/cpu_backend_probe.cpp

tests/unit/backends/test_cpu_foundation.cpp
tests/unit/backends/test_cpu_backend_registry.cpp
tests/unit/backends/test_cpu_health_capacity.cpp
```

The backend-test CMake integration was extended to register all three CPU tests
and to prove that CPU and CUDA backends may coexist in the same explicit
`BackendRegistry`.

### CPU Identity and Topology Decisions

- CPU identity is node-relative in Phase 7.1.
- Package, physical-core, and hardware-thread keys are derived from observed
  topology rather than temporary scheduler ordering alone.
- A future globally durable CPU identity must combine the stable Prometheus node
  identity with these node-relative topology keys.
- Linux logical CPU numbers remain runtime observations and are not globally
  durable identities.
- Discovery distinguishes configured, online, and process-allowed logical
  processors.
- Process affinity and cpuset restrictions are honored.
- Physical packages, cores, hardware threads, SMT sibling sets, NUMA nodes,
  cache-sharing records, Linux core-type hints, and capacity hints are reported
  when available.
- Missing topology remains explicit through issue and partial-success records.

### CPU Capability Decisions

- Capability reporting targets process-allowed processors when affinity is
  known; otherwise it conservatively targets all online processors.
- Feature coverage records both common support across all targets and support on
  any target.
- Safe common SIMD selection includes scalar, SSE2, SSE4.1, AVX, AVX2,
  AVX-512F, NEON, SVE, and SVE2 vocabulary.
- x86-64, AArch64, ARMv7, RISC-V 64, and PPC64LE architecture vocabulary is
  present.
- Phase 7.1 reports SIMD capability but does not execute SIMD-specialized
  operations.

### CPU Health and Observability Decisions

CPU health is refreshable and separate from immutable identity and capability
records. It reports, where supported:

- wall-clock and monotonic sample timestamps;
- aggregate and per-logical-processor utilization counters;
- baseline-only state before a utilization delta is available;
- current and maximum frequency;
- load averages;
- total, available, and swap memory;
- CPU and memory pressure information;
- CPU-associated thermal-zone or hwmon sensors;
- explicit issue records.

Telemetry states include unknown, available, baseline-only, unsupported,
provider-unavailable, and query-failed. Missing temperature, frequency, or
pressure providers are never translated into healthy zero values.

### CPU Capacity Decisions

CPU capacity reporting distinguishes:

- configured and online logical processors;
- process-allowed logical processors;
- physical and process-allowed physical cores;
- packages and NUMA nodes;
- common safe SIMD level;
- total and currently available memory;
- heterogeneous execution groups.

Execution groups are formed from NUMA locality, Linux core type, and capacity
hints where those observations exist.

The Phase 7.1 planner is advisory only. It may report reserved cores, reserved
logical processors, worker logical processors, and worker execution groups, but
it does not create workers, change affinity, reserve memory, or bind cognitive
workloads.

The default capacity policy makes no hidden reservations, selects one advisory
worker lane per physical core, and does not count SMT siblings as full worker
lanes unless explicitly requested.

### Generic CPU Backend Integration

`CpuBackend` implements the generic `ComputeBackend` lifecycle:

- constructed;
- initializing;
- ready;
- degraded;
- unavailable;
- failed;
- stopped.

Its generic snapshot reports online logical processors as visible resources and
advisory worker lanes as usable resources. Registration remains explicit and
local; no global singleton or cognitive master is introduced.

### Standalone Probe

`tools/cpu_backend_probe.cpp` is a human-readable maintenance probe. Core CPU
backend files remain silent and headless.

The probe reports:

- backend registration and lifecycle;
- package and node-relative identity summaries;
- configured, online, and process-allowed topology;
- NUMA and cache summaries;
- architecture and common SIMD capability;
- health and telemetry provider states;
- execution groups and advisory worker-lane planning;
- explicit topology, capability, health, and capacity issues;
- clean shutdown.

The CMake probe target is `prometheus_cpu_backend_probe` and is excluded from
ordinary builds unless explicitly requested.

### Validation

The user reported successful validation on host `curiosity` for:

- CPU foundation discovery;
- CPU backend registry lifecycle;
- CPU health and capacity;
- CPU/CUDA backend coexistence;
- process-affinity-restricted behavior;
- the requested CPU and full CTest sequences.

The standalone CPU probe must exit with status 0 before this addendum is treated
as the final Phase 7.1 closeout record.

The safe build command on the 16 GiB validation system is:

```text
cmake --build <named-build-directory> --parallel 1
```

An unrestricted `--parallel` build caused global memory exhaustion and Linux
killed the VS Code process. Kernel logs identified an OOM kill of `code`; the
serial retry used substantially less memory. This was a build-concurrency
resource event, not evidence of a runtime leak in the CPU backend.

No unconditional `-ldl` linkage was added. The existing CUDA 12.4/glibc rule
remains in force.

### Explicitly Not Implemented in Phase 7.1

- CPU worker threads or worker-pool lifecycle;
- bounded work queues;
- NUMA thread pinning;
- work stealing;
- SIMD operation dispatch;
- CPU kernel registry or typed operation adapters;
- CPU reference implementations of CUDA operation semantics;
- neural-current scheduling or neuron storage;
- production task dispatch;
- robot-path migration.

Placeholder files remain placeholders until their implementation phase.

### Current Robot Behavior Source of Truth

```text
legacy/vision_swarm_11.cu
```

### Phase 7.2 Direction

Phase 7.2 begins the CPU execution substrate:

- bounded worker-pool lifecycle;
- one worker group per advisory execution group or NUMA domain;
- local queues and explicit backpressure;
- clean start, drain, stop, and shutdown semantics;
- optional affinity binding controlled by policy;
- local-first scheduling with bounded cross-group work stealing;
- no CPU reference-operation claims until typed adapters exist.

### Required Files for the Phase 7.2 Chat

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

### Suggested Phase 7.2 Opening Message

> We completed Prometheus Phase 7.1: node-relative CPU identity, Linux topology,
> affinity-aware ISA capabilities, refreshable health, NUMA-aware capacity
> groups, advisory worker-lane planning, generic CPU backend integration, and a
> standalone diagnostic probe. CPU and CUDA register together, robot behavior
> remains unchanged, and the legacy vision source remains authoritative. Please
> read `AI_CONTEXT.md` and `docs/CURRENT_STATE.md` first. We are beginning Phase
> 7.2: bounded CPU worker pools, execution-group queues, lifecycle, backpressure,
> and NUMA-local scheduling foundations. Do not implement reference-operation or
> SIMD execution claims before typed adapters exist.

## 2026-07-15 — Integration Checkpoint I-001: CPU Execution and ARM Discovery

Checkpoint I-001 reconciled the valid implementation slices from
`agent/phase-7-2-1-cpu-thread-pool` and `agent/arm-a1-linux-auxv` onto the
current `main` architecture. Shared CMake and documentation files were updated
manually; neither feature branch was merged wholesale.

### CPU Execution Foundation

The single execution-group CPU pool now provides a fixed worker count, bounded
queue capacity, low/normal/high/critical priorities with bounded weighted
service, explicit queue-full backpressure, task handles and state inspection,
completion waits, queued cancellation, exception containment, structured
counters, drain shutdown, and cancel-pending shutdown. Execution-group and
logical-processor metadata remain advisory and do not change affinity.

Narrow hardening in this checkpoint:

- worker threads retain implementation state through exit, preventing
  worker-origin pool destruction from invalidating live worker state or leaving
  a joinable self-thread that terminates the process;
- cancelled queued entries are purged before capacity is evaluated for a new
  submission;
- weighted priority service bounds starvation without adding work stealing or
  a new scheduler layer;
- allocation failures while creating task controls or growing the queue return
  an explicit `resource_exhausted` submission result;
- restart after a completed stop is explicitly supported, with cumulative
  counters.

The SIMD selector uses explicit scalar, x86, and ARM classification rather than
enum ordering. It clamps common hardware capability to the caller's highest
compiled adapter, rejects cross-family and inconsistent data, returns scalar
for unknown levels, reports fixed widths exactly, and accepts SVE/SVE2 widths
only from 16 through 256 bytes in 16-byte increments. ARMv7 accepts NEON but
rejects SVE and SVE2. SIMD selection remains capability metadata and does not
prove an executable typed operation adapter.

### ARM Discovery Foundation

The existing ARM capability and Linux auxiliary-vector providers were retained
and reconciled with a separate processor-identity provider. ARM discovery
preserves AArch32/AArch64 state, Advanced SIMD, FP16, BF16, dot-product, I8MM,
crypto, atomics, SVE/SVE2 vocabulary, explicit unknown auxiliary-vector words,
and runtime SVE vector-length observation when available.

The injectable `/proc/cpuinfo` parser records implementer, architecture,
variant, part, and revision observations; groups observed processor signatures;
marks heterogeneity only when multiple complete signatures exist; and reports
missing, malformed, duplicate, and incomplete records explicitly. It remains
observational metadata, not durable node identity, backend registration
authority, or instruction-dispatch authority.

Capability and identity providers remain separate. The diagnostic probe is the
current higher discovery layer and composes their results using the same target
logical processors. This is the smallest API change and avoids turning the
existing capability query into a semantically ambiguous full-discovery query.
ARM continues to enrich the generic CPU backend; no second CPU backend is
registered.

### Validation

Validation used `build/integration-i-001-cpu-arm`, CUDA architecture 75, tests
enabled, benchmarks disabled, legacy vision disabled, and a serial build.

- configure: passed;
- serial complete build: passed;
- focused CPU CTest: 5/5 passed;
- focused ARM CTest: 3/3 passed;
- CPU thread-pool repeat: 100/100 passed;
- complete CTest with host GPU access: 17/17 passed;
- x86 ARM probe: reported `unsupported_architecture` / `not_arm` and exited 0;
- `git diff --check`: passed before review.

The sandboxed full-suite attempt could not access the NVIDIA driver and failed
only the eight CUDA-dependent tests. The same unmodified suite passed 17/17
when rerun with host GPU access.

### Remaining Limits

- Real ARM Linux validation has not yet been performed. AArch32, AArch64,
  heterogeneous processors, and SVE/SVE2 hardware remain pending.
- Running tasks are cooperative and cannot be forcefully interrupted. Drain or
  stop can wait indefinitely for a task that never returns; timeout and
  escalation policy remains Phase 7.2.1a work.
- There is no NUMA pool collection, affinity binding, or work stealing.
- There are no typed CPU or ARM operations, kernels, or production dispatch
  adapters.
- The CPU pool is not attached to `CpuBackend` or a production scheduler.
- Robot behavior remains on the legacy path and was not migrated.
