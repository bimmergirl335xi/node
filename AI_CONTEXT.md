# Prometheus Node — AI Context

> Current-state engineering handoff for Project Prometheus.
> This file is intentionally concise and is not a full changelog.
> Detailed phase history belongs in `docs/CURRENT_STATE.md`, architecture
> specifications, pull requests, and Git history.
>
> Last reconciled: 2026-07-17 through RC-001/I-002.
> This file describes the integrated repository line and the deliberate work
> retained beyond it on active lanes.

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
- Destructive provisioning, production activation, trust expansion, and
  propagation require explicit operator authorization.
- Placeholder files are not implemented components.
- Use named build directories. Do not destructively clear shared build trees.
- Branch-local implementation remains branch-local until deliberately
  reconciled. Codex handles source integration; documentation must not pretend
  a branch has already been merged.

## Adaptive Connection Substrate

The Adaptive Connection Substrate (ACS) is the architectural foundation for
relationships and connections between nodes, services, cognitive structures,
hardware resources, and future mesh participants.

The public Draft specification set contains ACS-0000 through ACS-0009,
covering the charter, invariants, relationship classes, signal taxonomy,
endpoints and ports, connection lifecycle, admission and budgets, security and
trust, immune integration, and runtime integration.

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

ACS-I001 is integrated into `main` under `src/core/acs/`. It provides bounded
public identities and evidence types, immutable descriptors, a bounded registry
with deterministic snapshots, independently versioned lifecycle, operational,
and enforcement state transitions with bounded idempotency, and pure
non-reserving admission evaluation. It does not establish live connections or
implement transport, discovery, authentication providers, resource reservation,
persistence, MEM authority, or BOOT/rescue behavior.

## Memory, Immune, and Bootstrap Boundaries

- The public MEM Draft series contains MEM-0000 through MEM-0010. It defines
  memory identity, roles, operations, availability, lifecycle, custody,
  recovery, ACS integration, and conformance. No MEM persistence runtime is
  implemented.
- The public IMM Draft series contains IMM-0000 and IMM-0001. It defines the
  public charter and invariants while explicitly excluding private threat
  models, heuristics, thresholds, production response policy, credentials, and
  topology. No IMM implementation is present.
- Public BOOT documents are not committed in this repository state. Bootstrap
  implementation remains owned by `lane/bootstrap`; RC-001 did not create that
  lane or add bootstrap code.
- ACS retains communication authority, MEM retains memory semantics, IMM may
  assess and request bounded action, adaptive state retains mutation semantics,
  and runtime code only executes authority granted by the owning domain.

## Platform Context

### Hardware

- Validation profiles include Pascal 6.1, Volta 7.0, and Turing 7.5 CUDA
  targets plus a legacy Kepler compatibility case.
- CPU targets include x86, generic ARM, and future heterogeneous CPU systems.
- Optional accelerator profiles include Hailo, IMX500, Xeon Phi, and future AMD
  GPUs without assuming that any is present on a generic node.

Persistent CUDA keys use `cuda:<GPU-UUID>`. Runtime ordinals are rebound to UUIDs
at startup.

### CUDA Policy

- Current validation toolkit/runtime/driver family: CUDA 12.4
- Compiler build observed on the validation workstation: 12.4.131
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

### Runtime and Adaptive-State Foundations — B-001, S-001, and A-001

Integrated through I-002:

- service lifecycle composition with callbacks outside the service-manager
  lock and an explicitly borrowed backend registry in `ServiceContext`;
- pure backend-neutral execution-policy metadata evaluation;
- dynamically loaded, bounded NVRTC readiness without artifact execution or
  promotion;
- typed adaptive-state descriptors, validation, authority separation, atomic
  transactions, and rollback;
- typed bounded architecture graphs and proposal operations;
- isolated shadow application, structural validation, advisory impact evidence,
  and explicit unknown/resource/adapter/authority outcomes;
- a versioned bounded C ABI for proposal validation with caller-owned buffers,
  size queries, fixed-width types, and exception containment.

Live architecture application, service reconfiguration, MEM persistence, live
ACS connection establishment, Julia embedding, and generated CUDA execution
remain absent.

### Adaptive Connection Substrate Foundation — ACS-I001

Integrated into `main`:

```text
Source:
  src/core/acs/

Units:
  ACS-01.01
  ACS-01.02
  ACS-01.03
  ACS-01.04

State dimensions:
  lifecycle
  operational
  enforcement
  authority

Properties:
  bounded identities
  immutable descriptors
  deterministic snapshots
  versioned transitions
  bounded idempotency
  pure admission

Explicit exclusions:
  transport
  discovery
  authentication provider
  resource reservation
  persistence
  MEM authority
  BOOT/rescue behavior
```

The integrated validation passed 29/29 CTests with host GPU access. ACS
lifecycle and concurrency tests, service lifecycle, adaptive state,
architecture shadow, proposal ABI, and the CPU thread pool each passed 20
repeated runs. Native C++ strict warning checks passed. Real AArch64 validation,
descriptor removal, a public ACS C ABI, live connection establishment, and
durable state remain pending.

## Active Branch Map

### `main`

Current integrated line. It contains:

- the Phase 6 CUDA foundation;
- the Phase 7.1 generic CPU foundation;
- the bounded single-group CPU thread pool and conservative SIMD selector;
- generic ARM Linux auxiliary-vector and processor-identity discovery;
- the first Hailo accelerator-backend slice;
- B-001, S-001, A-001, and their I-002 reconciliation merge;
- the ACS-I001 public runtime-local contract foundation;
- ACS-0000 through ACS-0009;
- MEM-0000 through MEM-0010;
- IMM-0000 and IMM-0001.

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

### `lane/runtime`

Runtime implementation lane. ACS-R001 synchronizes it to final `main` after the
ACS-I001 implementation and documentation reconciliation.

### `lane/docs`

Public architecture lane for additional IMM and future BOOT documentation. It
contains no private immune or memory mechanisms and is synchronized to final
`main` by RC-001.

### `lane/cpu`

CPU continuation lane. It retains one deliberate shutdown-observability commit
beyond `main`. That work is not integrated into `main`; timed shutdown waiting,
execution-group orchestration, and affinity work remain deferred.

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

Checkpoint validation on the x86 validation workstation passed all five
focused CPU tests, 100 repeated thread-pool runs, and the complete 17-test
CTest suite.

RC-001 classifies this pull request as a superseded historical input. It must be
closed without merging after final validation; its branch must not be deleted.

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

RC-001 classifies this pull request as a superseded historical input. It must be
closed without merging after final validation; its branch must not be deleted.

## Current Work and Next Boundaries

Next work remains lane- and checkpoint-specific:

1. Future ACS work must remain separately authorized and must not infer live
   application, transport, persistence, or resource authority from ACS-I001.
2. `lane/docs` may continue independently authored public IMM work and may add
   public BOOT architecture when separately authorized.
3. `lane/cpu` may complete shutdown observability without merging that partial
   work into `main` prematurely.
4. Generic ARM discovery still requires real AArch32/AArch64 and SVE/SVE2
   hardware validation.

Explicitly not yet complete:

- CPU affinity binding and NUMA-group worker orchestration;
- timed waiting for CPU-pool shutdown completion;
- bounded cross-group work stealing;
- typed CPU kernel adapters and CPU reference operations;
- CPU kernel registry and production CPU dispatch;
- HailoRT loading, Hailo model execution, or production accelerator dispatch;
- IMX500 backend integration;
- live ACS connection establishment, transport, discovery, and persistence;
- MEM persistence and IMM implementation;
- public BOOT architecture and bootstrap implementation;
- live architecture proposal application or service-graph reconfiguration;
- neural scheduling, storage, and whole-mesh production dispatch;
- robot-path migration away from `legacy/vision_swarm_11.cu`.

## Build and Validation Rules

- Use named build directories.
- On the 16 GiB validation workstation, build serially:

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
changes are normally append-only addenda unless the authorized operator
explicitly requests a reconciliation or rewrite.

Handoffs must preserve architecture decisions, completed work, validation,
limitations, unfinished placeholders, exact commands, required files, and a
clean next-step boundary.

## 2026-07-18 — GPU-7.1A Driver-Independent Hardware Inventory

GPU-7.1A adds a generic Linux PCI inventory below and outside the CUDA
backend. `src/hardware/linux_pci_inventory.hpp` and
`src/hardware/linux_pci_inventory.cpp` inspect only immediate entries beneath
a configurable sysfs root, defaulting to `/sys/bus/pci/devices`. The component
uses ordinary C++17 and POSIX file access; it does not include CUDA headers,
link CUDA libraries, call a driver/runtime API, or invoke external commands.

The value model preserves checked PCI addresses and raw numeric identity,
classifies devices from PCI class codes, classifies known vendor families as a
convenience, and reports bound, unbound, or unknown kernel-driver state.
Hardware presence does not establish CUDA availability, health, compatibility,
admission, or scheduling eligibility. All scans, retained devices, paths,
identifiers, driver names, issues, and attribute reads have configurable and
absolute bounds. Unavailable roots, malformed entries, partial observations,
permission failures, I/O failures, and resource exhaustion remain explicit.

Synthetic sysfs tests cover NVIDIA hardware without a driver, `nouveau`,
`vfio-pci`, AMD and Intel devices, an NVIDIA non-GPU device, empty and missing
roots, malformed data, bounded capacity, unknown vendors, failed driver-link
resolution, optional identifiers, deterministic ordering, and invalid bounds.
The ordinary-C++ no-CUDA build passed all 22 CTests under strict native
warnings, and the inventory fixture passed 100 consecutive runs. Its standalone
probe links no CUDA library and reports `CUDA_RUNTIME status=not_evaluated`.

The validation host had CUDA 12.4 and two NVIDIA Quadro RTX 4000 devices. The
generic probe observed them independently at `0000:2d:00.0` and
`0000:2e:00.0`, both bound to the `nvidia` kernel driver. The CUDA-enabled
serial build for architectures 61, 70, and 75 passed all 30 CTests with host
GPU access. Existing CUDA capability and runtime-resource probes also passed.
These CUDA results are separate corroborating evidence and are not fields or
inferences in the generic inventory.

This checkpoint adds no package installation, driver loading, runtime bridge,
health monitoring, resource allocation, admission, scheduling, or executable
GPU work. A recommended follow-up is a narrow GPU-7.1B evidence-correlation
checkpoint that keeps physical hardware, driver binding, CUDA API presence,
runtime enumeration, compatibility, and Node admission as distinct states.

## 2026-07-18 — GPU-7.1B Bounded GPU Evidence Correlation

GPU-7.1B is based exactly on GPU-7.1A commit
`3561b12b3b2f292977253c77febe13e0c8d13167`. It adds an always-build pure
correlation model in `src/hardware/gpu_evidence_correlation.*` and a
CUDA-enabled mapping adapter in `src/backends/cuda/cuda_evidence_adapter.*`.
The pure layer accepts already-observed metadata, includes no CUDA SDK header,
links no CUDA library, performs no hardware query, and mutates no input or
runtime state.

Evidence remains independent for physical inventory, per-device kernel-driver
binding, compile/runtime/driver software versions, CUDA enumeration, stable
UUID, CUDA PCI identity, architecture and release compatibility, binary-image
coverage, kernel-registry coverage, backend-registration readiness,
runtime-binding usability, execution readiness, and Node admission. Admission
is always `not_evaluated` in this checkpoint. Registration readiness does not
imply kernel coverage, execution readiness, or admission.

Full PCI identities match only when domain, bus, device, and function agree.
When CUDA omits the domain, bus/device/function may produce an explicitly
inferred match only if exactly one hardware candidate exists. Multiple
candidates are ambiguous and are never resolved by order. Runtime devices with
no usable PCI identity remain runtime-only. One physical function may retain
multiple CUDA identities, provided each logical device remains independently
addressable. Duplicate PCI identities, CUDA UUIDs or persistent keys, runtime
ordinals, and unstable one-to-many associations are explicit conflicts.

Ordering is deterministic: hardware records use numeric canonical PCI order;
CUDA associations and runtime-only records use stable UUID, persistent key,
PCI evidence, then runtime ordinal. Configurable and absolute bounds cover all
input/retained records, associations, issues, stable identities, and retained
diagnostic identity. Overlength stable identities are rejected without copying
or truncating them into results.

Strict no-CUDA validation passed 23/23 CTests. The A–T synthetic suite, which
contains 36 input permutations per run, passed 100 consecutive runs. Dynamic
dependency inspection found no CUDA library in the pure test or driverless
probe. CUDA 12.4 validation for architectures 61, 70, and 75 passed 32/32
CTests, including the host adapter test; existing capability and runtime-
resource probes also passed.

With normal visibility, the probe correlated `0000:2d:00.0` and
`0000:2e:00.0` exactly to two stable CUDA UUIDs. With
`CUDA_VISIBLE_DEVICES=` both PCI devices remained hardware-only while CUDA
reported zero visible devices. With `CUDA_VISIBLE_DEVICES=0`, one stable UUID
matched `0000:2d:00.0` exactly and `0000:2e:00.0` remained hardware-only.
Runtime ordinals were not used as durable identity.

GPU-7.1B adds no installation action, health policy, topology scheduling,
memory allocation, kernel launch, MIG management, ACS admission, or private
deployment policy. A recommended separately authorized follow-up is GPU-7.1C:
bounded installer-facing driver/runtime library and package evidence that still
does not install software or confuse presence with usability.

## 2026-07-18 — GPU-7.2A Device-Local Execution Worker

GPU-7.2A starts exactly from `lane/gpu` commit
`fef4e2d8803cf38935e59e217fc5e7cec349e89a` and is implemented on the single
temporary branch `tmp/gpu-7-2a-execution-worker`. It adds a CUDA-conditional
`CudaDeviceWorker`, typed launch-adapter contract, bounded actual-adapter
registry, fixed synthetic FP32 adapter, focused worker test, and line-oriented
execution probe. The temporary branch must be reviewed, fast-forwarded into
`lane/gpu` by the operator, and then deleted locally and remotely.

One explicitly constructed worker binds one durable UUID-based CUDA device key
to a freshly resolved runtime ordinal snapshot. Its worker thread calls
`cudaSetDevice`, creates one `cudaStreamNonBlocking` stream and one
`cudaEventDisableTiming` event, initializes device-local adapters, and only then
publishes idle/accepting state. Runtime ordinals remain temporary evidence and
are never used as durable identity. `CudaBackend` does not create workers
automatically.

The actual adapter registry remains distinct from `CudaKernelRegistry`
metadata. Exact kernel-ID resolution requires both complete executable
metadata and an initialized actual adapter. The executable probe ID is
`probe.synthetic.execute.fp32`; its adapter owns exactly eight FP32 input and
output elements, submits host-to-device copy, existing ReLU launch, and
device-to-host copy on the worker stream, then validates every output only
after the worker records and synchronizes its completion event.

The queue path is `submit → claim_next → mark_running → launch → event record →
event completion → adapter validation → complete`, preserving the original
nonzero claim token throughout. Truthful adapter or validation errors use
`fail` exactly once. Missing adapters, non-executable metadata, controlled
launch failures, and output mismatches are job-local. Device binding, stream,
event, adapter initialization, completion-context, and terminal-transition
failures stop the worker conservatively.

Submissions use a worker wrapper that rejects wrong-device or inactive work,
delegates descriptor validation to the existing queue, and increments a
bounded notification generation plus a pending-notification predicate only
after successful submission. The worker holds its lifecycle mutex across the
queue inspection and condition-variable wait transition, preventing lost
wakeups without polling. Stop rejects new submissions, prevents new claims,
finishes an already-running job, and leaves unclaimed shared-queue jobs queued.
No thread is detached. A normally stopped object may restart after complete
adapter/event/stream cleanup; it re-resolves its ordinal from the supplied pool
snapshot and retains cumulative counters.

Strict CUDA-disabled validation remained 23/23 CTests. CUDA 12.4 serial
validation for architectures `61;70;75` passed 33/33 CTests. The focused worker
suite passed 100 consecutive host-backed runs. With both Quadro RTX 4000
devices visible, the probe completed 16/16 jobs: eight on
`cuda:GPU-888ada1c-55ed-66cd-69ce-f0719ceedc4b` and eight on
`cuda:GPU-4ced1003-8696-6957-d524-1539252d8c8d`. With
`CUDA_VISIBLE_DEVICES=0`, the retained UUID completed 8/8 at ordinal snapshot
zero. With empty visibility, the probe reported `no_devices` without creating
a worker. Every probe worker joined and reported its stream, event, and adapter
resources destroyed. Existing capability and runtime-resource probes passed;
NVIDIA Compute Sanitizer memcheck reported zero errors for the two-device
probe. Pre-existing CUDA shuffle/PTX and nvlink diagnostics remain unchanged.

GPU-7.2A adds no scheduler, work stealing, multi-GPU job, arbitrary payload,
memory-lease resolution, memory-pool integration, generated kernel execution,
retry, migration, MIG, ACS admission, or deployment policy. A cooperative CUDA
operation that never completes can still delay stop because GPU-7.2A uses a
blocking event wait on its device thread. The recommended next checkpoint is
GPU-7.2B: bounded execution payload and memory-lease resolution.
