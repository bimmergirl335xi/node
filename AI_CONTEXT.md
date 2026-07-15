# Prometheus Node — AI Context

> Current-state engineering handoff for Project Prometheus.
> This file is intentionally concise and is not a full changelog.
> Detailed phase history belongs in `docs/CURRENT_STATE.md`, architecture
> specifications, pull requests, and Git history.
>
> Last reconciled: 2026-07-15.
> The same context file is maintained on the active development branches.
> Use the branch map below before assuming that branch-local code is merged.

## Project

Prometheus is a ground-up distributed embodied cognitive mesh, not an LLM and
not a single monolithic model. Specialized neural structures and services may
run across CPU, GPU, accelerator, and edge nodes. Small machines remain complete
but narrower participants; larger machines may host more, larger, replicated,
or modality-specific specialists.

The new runtime is being separated from the legacy CUDA vision program without
changing existing robot behavior.

## Non-Negotiable Architecture

- Production robot behavior remains defined by:

  ```text
  legacy/vision_swarm_11.cu
  ```

- The Raspberry Pi Pico pulse remains the authoritative robot-cycle source.
- No node, backend, GPU, service, scheduler, or branch is the permanent
  cognitive master.
- CUDA is one backend. Generic CPU, ARM, AMD GPU, accelerators, Xeon Phi, and
  future hardware are first-class targets.
- ARM means the architecture family, not Raspberry Pi. ARM targets may be SBCs,
  embedded controllers, workstations, or multi-socket servers with discrete
  GPUs or other accelerators.
- ARM capability discovery enriches the generic CPU backend. It must not
  register a duplicate backend for the same physical CPU.
- Generic ARM CPU discovery must not assume cameras, Raspberry Pi firmware,
  Hailo, IMX500, or any attached accelerator exists.
- Hardware assignments use persistent identities, never temporary CUDA
  ordinals, Linux logical-CPU numbers, discovery order, or transient device
  paths alone.
- Missing information remains explicitly unknown, unsupported, unavailable,
  degraded, partial, or failed. Unknown state is never silently healthy.
- Discovery and planning must not automatically activate peer access, allocate
  production model memory, launch kernels, admit peers, redirect robot
  execution, or grant trust.
- A kernel is executable only when registration, device compatibility, release
  family, binary image, and typed adapter are all proven.
- Learning kernels may propose and validate changes but cannot promote them.
  Promotion requires provenance, validation, versioning, authorization, and
  rollback.
- Core runtime code must support extremely small headless Linux nodes without a
  GUI, display server, desktop services, interactive prompts, or required
  human-readable local output.
- Local terminal output is optional diagnostic behavior. Long-term operator
  interaction is expected through the Pocket Decoder/Lens.
- Olivia is currently the only authorized operator. Destructive provisioning,
  production activation, trust expansion, and propagation require explicit
  authorization.
- Placeholder files are not implemented components.
- Use named build directories. Do not destructively clear shared build trees.
- Branch-local implementation remains branch-local until deliberately
  reconciled. Codex handles source integration; documentation must not pretend
  a branch has already been merged.

## Adaptive Connection Substrate

The Adaptive Connection Substrate (ACS) is the architectural foundation for
relationships and connections between nodes, services, cognitive structures,
hardware resources, and future mesh participants.

The specification set currently contains:

- ACS-0000: charter and specification framework;
- ACS-0001: core principles and twenty mandatory invariants;
- ACS-0002: relationship classes working draft;
- ACS-0003: signal taxonomy working draft.

The ACS rules include these boundaries:

- a relationship is not a socket, thread, process, route, transport, credential,
  or physical link;
- logical relationship identity and continuity remain separate from temporary
  transport and session state;
- one relationship may use multiple paths, and one path may carry multiple
  relationships without erasing logical isolation;
- connection state is explicit, bounded, observable, versioned, and
  recoverable;
- discovery does not imply identity, trust, admission, authority, or permission;
- sensitive relationship and connection traffic requires authenticated,
  integrity-protected, confidentiality-preserving security appropriate to its
  class and policy;
- keys, credentials, and sessions are replaceable security material, not the
  permanent identity of a relationship;
- compromise should be compartmentalized so one failed path, session, or peer
  does not automatically expose every relationship;
- resource use, fan-out, retry, buffering, replication, and adaptation are
  bounded and accountable;
- adaptive promotion, decay, rerouting, and learning cannot bypass security,
  health, immune, operator, or resource policy;
- production activation requires a verified and internally compatible
  substrate;
- installation, enrollment, replication, and propagation are explicit,
  bounded, authorized, auditable, interruptible, and reversible where
  technically possible.

ACS is currently specification work. Do not treat the placeholder mesh and
connection source files as an implemented runtime substrate.

## Platform Context

### Hardware

- Validation host: `curiosity`
- Validation GPUs: 2 × Quadro RTX 4000, Turing, compute capability 7.5
- Edge vision target: P106-100, Pascal, normally compute capability 6.1
- Main server: 10 × Tesla V100 32 GB, Volta, compute capability 7.0
- Compatibility case: Quadro K6000, Kepler
- CPU targets: x86 Xeon, generic ARM, and future heterogeneous CPU systems
- Other targets: ten unidentified-family Xeon Phi devices, Raspberry Pi, Hailo,
  IMX500, and future AMD GPUs

Current validation GPU identities:

```text
GPU-888ada1c-55ed-66cd-69ce-f0719ceedc4b  PCI 0000:2d:00.0
GPU-4ced1003-8696-6957-d524-1539252d8c8d  PCI 0000:2e:00.0
```

Persistent CUDA keys use `cuda:<GPU-UUID>`. Runtime ordinals are rebound to UUIDs
at startup.

### CUDA Policy

- Current validation toolkit/runtime/driver family: CUDA 12.4
- Compiler build observed on `curiosity`: 12.4.131
- CUDA 12 family retains Pascal, Volta, Turing, and newer explicitly compiled
  targets.
- CUDA 13 family supports Turing and newer targets.
- Pascal and Volta remain on CUDA 12.
- Toolkit compatibility alone does not prove a binary image or typed adapter.

### Minimal Nodes and Provisioning

Nodes may run extremely small headless Linux installations containing only what
is required to boot, initialize networking and device drivers, load services
into RAM, use CPU/GPU/peripherals, communicate with the mesh, expose structured
diagnostics, and shut down or recover safely.

Drivers, CUDA or ROCm toolkits, firmware, compilers, OS packages, kernels, boot
configuration, storage layouts, and services belong in signed provisioning and
system-profile manifests, not Python requirements.

Provisioning must inventory hardware, select a deliberate profile, verify the
complete operating substrate, require authorization for destructive actions,
validate installation, report unknown or conflicting state, and support
recovery or rollback.

## Completed Foundations

### Kernel Foundation

Reported passing before backend work:

- activation and reduction;
- FP32, FP64, and integer dense operations;
- LayerNorm and RMSNorm;
- temporal state and prediction;
- Hebbian, Oja, and predictive-delta learning;
- SGD, momentum, RMSProp, and Adam optimizer state;
- candidate validation and benchmark summaries.

Shadow optimizer mode must not mutate parameters or optimizer state. Candidate
promotion authority remains outside kernels.

### Phase 6 — CUDA Backend

Implemented and integrated:

- generic non-singleton `ComputeBackend` and `BackendRegistry`;
- CUDA profiles, capability reporting, stable UUID discovery, and ordinal
  rebinding;
- refreshable health through dynamically loaded NVML with explicit telemetry
  states;
- UUID-keyed multi-device inventory and advisory placement;
- spread, pack, stable-order, allowlist, commitment, and memory policies;
- directed peer-capability reporting without automatic peer activation;
- metadata-first CUDA kernel registry;
- per-device execution-readiness evaluation;
- clean backend lifecycle and shutdown.

The CUDA backend does not take over the robot loop, launch production kernels,
permanently select a GPU, enable peer access, reset GPUs, or assume one
monolithic model.

Validated root integration on both RTX 4000s passed 7/7 tests in the named
Phase-6 build.

### Phase 7.1 — Generic CPU Backend

Implemented:

- node-relative CPU identity;
- configured, online, and process-allowed topology;
- package, core, hardware-thread, SMT, cache, NUMA, core-type, and capacity
  observations;
- common and any-target ISA/SIMD capability reporting;
- refreshable utilization, frequency, load, memory, pressure, and thermal
  health observations;
- explicit telemetry and partial-discovery states;
- capacity and heterogeneous execution-group planning;
- generic `CpuBackend` lifecycle and explicit registry coexistence with CUDA;
- optional headless-safe maintenance probe and focused tests.

The Phase 7.1 planner is advisory. It does not create workers, change affinity,
reserve production memory, dispatch neural work, or redirect robot execution.

## Active Branch Map

### `main`

Current integrated line. It contains:

- the Phase 6 CUDA foundation;
- the Phase 7.1 generic CPU foundation;
- the bounded single-group CPU thread pool and conservative SIMD selector;
- generic ARM Linux auxiliary-vector and processor-identity discovery;
- the first Hailo accelerator-backend slice;
- ACS-0000 through ACS-0003;
- ACS-0001 version 0.2 with twenty invariants.

The integrated CPU thread pool provides a fixed worker count, bounded weighted
priority queue, explicit submission backpressure, task handles, queued
cancellation, structured counters, exception containment, restart after stop,
safe worker-origin destruction, and drain or cancel-pending shutdown. It stores
execution-group metadata but does not change affinity or orchestrate NUMA pools.

The integrated SIMD selector explicitly separates scalar, x86, and ARM
families, clamps common hardware capability to the highest compiled adapter,
rejects inconsistent and cross-family inputs, restricts ARMv7 to NEON-family
selection, and treats unknown SVE widths as unusable. Selection is capability
metadata only; it does not prove that a typed executable adapter exists.

ARM remains an enrichment layer for the generic CPU backend. Linux auxiliary
vectors provide conservative feature observations, while injectable
`/proc/cpuinfo` parsing groups observational processor signatures. Capability
and identity providers remain separate and are composed only by the diagnostic
probe in this slice. No second ARM backend is registered.

The integrated Hailo slice currently performs conservative host and visible
device discovery. A visible device remains degraded and unusable until HailoRT
integration proves runtime and model execution readiness.

### `agent/open-source-source-tree`

Full-source integration base used by active side work. It carries the repository
source tree plus generic ARM Linux auxiliary-vector capability discovery and
related test wiring. It does not make ARM a separate registered CPU backend.

### `agent/phase-7-2-1-cpu-thread-pool`

Integration source retained for history. Checkpoint I-001 reconciled its valid
CPU execution and SIMD slices into `main`, including:

- the bounded CPU thread-pool implementation;
- priority-aware queueing and explicit backpressure;
- task lifecycle, cancellation, counters, exception containment, and clean
  shutdown;
- conservative x86 and ARM SIMD-level selection;
- vector-width reporting with explicit unknown SVE/SVE2 width;
- tests for pool lifecycle, concurrency, selection, and vector widths.

The integrated slice does not implement affinity binding, NUMA-group
orchestration, cross-group work stealing, typed CPU kernels, neural-current
scheduling, or production dispatch.

Checkpoint validation on `curiosity` passed all five focused CPU tests, 100
repeated thread-pool runs, and the complete 17-test CTest suite.

### `agent/arm-a1-linux-auxv`

Integration source retained for history. Checkpoint I-001 reconciled its valid
ARM discovery slices into `main`, including:

- generic Linux ARM `AT_HWCAP` and `AT_HWCAP2` observation;
- AArch64 and conservative AArch32 capability decoding;
- runtime SVE vector-length observation when available;
- injectable `/proc/cpuinfo` processor-identity parsing;
- implementer, architecture, variant, part, and revision observations;
- heterogeneous processor-signature grouping;
- explicit issue records for missing, malformed, duplicate, or incomplete
  identity data.

Processor identity is observational metadata, not dispatch authority. The
integrated slice does not assume Raspberry Pi, cameras, Hailo, IMX500, GPUs, or
other accelerators.

Checkpoint validation passed all three ARM CTests and the x86 not-applicable
probe. Real AArch32, AArch64, heterogeneous ARM, and SVE/SVE2 hardware
validation remain pending.

## Current Work and Next Boundaries

Work is proceeding on several independent tracks:

1. Continue Phase 7.2.1a CPU hardening before adding typed CPU operation
   adapters, especially policy for running tasks that never return.
2. Validate generic ARM discovery on real AArch32 and AArch64 Linux hardware,
   including heterogeneous and SVE/SVE2 systems where available.
3. Continue Hailo discovery toward dynamically loaded HailoRT integration,
   explicit runtime state, stable device identity, health, capacity, and model
   readiness without coupling it to generic ARM discovery.
4. Continue ACS specification work by refining relationship classes, signal
   semantics, lifecycle, admission, security, budgeting, health, and immune
   integration before implementing the runtime substrate.

Explicitly not yet complete:

- CPU affinity binding and NUMA-group worker orchestration;
- bounded cross-group work stealing;
- typed CPU kernel adapters and CPU reference operations;
- CPU kernel registry and production CPU dispatch;
- HailoRT loading, Hailo model execution, or production accelerator dispatch;
- IMX500 backend integration;
- ACS runtime connection and relationship implementation;
- neural scheduling, storage, and whole-mesh production dispatch;
- robot-path migration away from `legacy/vision_swarm_11.cu`.

## Build and Validation Rules

- Use named build directories.
- On the 16 GiB `curiosity` workstation, build serially:

  ```text
  cmake --build <named-build-directory> --parallel 1
  ```

- Unrestricted parallel compilation previously caused a system OOM kill of
  VS Code. That was a build-concurrency event, not evidence of a backend runtime
  leak.
- Link `Threads::Threads` and `CUDA::cudart` where required.
- Use CUDA separable compilation.
- Do not add unconditional `-ldl`.
- On the current glibc/CUDA 12.4 bench, do not append `-ldl` to direct `nvcc`
  probes because `nvlink` inspected an incompatible host archive.
- Core backend and runtime files remain silent and headless by default.
- Human-readable output belongs in optional probes and future signed operator
  tooling.

## Development Process

Each phase has five substantive implementation chats. Setup, handoff, file
collection, ordinary validation, compiler or linker correction, and focused
debugging do not consume a slot.

At phase close, update `AI_CONTEXT.md` and `docs/CURRENT_STATE.md`. Documentation
changes are normally append-only addenda unless Olivia explicitly requests a
reconciliation or rewrite.

Handoffs must preserve architecture decisions, completed work, validation,
limitations, unfinished placeholders, exact commands, required files, and a
clean next-step boundary.
