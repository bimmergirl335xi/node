# Node — AI Technical Context

This document contains durable architectural context for AI agents. Read
`docs/CURRENT_STATE.md` for the exact integrated checkpoint and Git history for
chronology.

## Architectural purpose

Node is a dynamically assembled, distributed, embodied-cognition environment.
It is not a single model and no permanent cognitive master is assumed. Services
and specialized structures may execute across CPUs, GPUs, accelerators, and
small edge machines. A small node remains a complete but narrower participant;
a larger node may host more or larger specialists.

Current production robot behavior remains in `legacy/vision_swarm_11.cu` during
the migration. New runtime foundations must not silently take over that loop.
The Raspberry Pi Pico pulse remains the authoritative robot-cycle source until
an explicitly validated migration changes that contract.

## Dynamic assembly model

Node is assembled from explicit, versioned inputs rather than inferred host
state. Profiles, source identity, toolchain, configuration, artifacts,
acceptance, generation membership, installation, activation, and runtime
readiness are separate concepts. Evidence at one boundary never grants
authority at another.

The public assembly path must remain valid without external/private components.
An external component is an assembly-generation sibling described through a
bounded public interface. Discovering or parsing its manifest does not accept,
launch, trust, or activate it.

## Linux kernel relationship

Linux supplies boot, processes, memory management, filesystems, networking, and
mature device-driver support. Kernel source is maintained outside this
repository and selected by exact provenance and revision. Node should use Linux
ownership where appropriate instead of replacing stable driver stacks.

Hardware may be Linux-owned, ACS-owned, or coordinated/shared. The ownership
contract must be explicit; hardware presence alone does not determine it.

## Micro-OS and runtime model

The intended startup relationship is:

```text
Linux kernel
    -> static Node micro-OS as PID 1
    -> public runtime substrate
    -> public ACS substrate
    -> optional external components
```

The micro-OS is deliberately smaller than the normal runtime. Its job is to
establish bounded volatile facilities, validate an exact startup manifest,
supervise explicit services, report evidence, and end in a controlled state.
It does not infer deployment policy, accept an assembly generation, or prove
production readiness.

The runtime is composed from explicit service lifecycle and backend contracts.
Callbacks must not execute under manager locks. Borrowed registries and other
non-owning dependencies must remain explicit. Adaptive-state transactions and
architecture proposals are bounded and validated; proposal evaluation does not
grant live mutation authority.

## CPU architecture

There is one generic CPU backend per physical CPU domain. Identity, topology,
capability, health, capacity, and execution are related but distinct evidence.
ARM-specific auxiliary-vector and processor-identity observations enrich the
generic backend rather than registering a duplicate ARM backend.

CPU capability selection is conservative. It separates scalar, x86, and ARM
families, clamps runtime evidence to compiled adapters, and treats unknown
vector widths as unusable when width is required. Execution-group metadata does
not itself change affinity or establish NUMA orchestration.

The bounded CPU pool provides queueing, backpressure, task state, cancellation,
exception containment, and controlled shutdown. It is infrastructure, not a
production neural scheduler.

## GPU and accelerator architecture

CUDA is optional. Generic discovery must compile and operate without CUDA,
NVML, a GPU, or root access. The design preserves separate states for physical
PCI hardware, driver binding, toolkit/compiler support, runtime visibility,
stable identity, binary compatibility, typed adapter availability, backend
registration, execution readiness, and admission.

Persistent CUDA identity uses the device UUID; runtime ordinal is rebound at
startup and is never durable identity. PCI visibility does not prove CUDA
readiness. Metadata claiming an adapter exists does not prove a usable typed
adapter instance exists.

A device-local worker owns CUDA context-sensitive resources on its thread and
uses bounded terminal job states. It does not make the backend a global
scheduler. No path should automatically install drivers, allocate production
models, enable peer access, or admit devices.

Other accelerators, including Hailo, IMX500, AMD GPU, and Xeon Phi, remain
independent backends. Generic CPU or ARM discovery must not assume any is
attached.

## Hardware discovery principles

Discovery is observational and bounded. Preserve distinctions between:

- absent, unsupported, unavailable, unknown, partial, degraded, failed, and
  ready;
- stable identity and transient enumeration;
- physical existence and usable software support;
- discovery, policy, admission, activation, and ownership.

Never convert missing evidence into a healthy default. Discovery order, CUDA
ordinal, logical CPU number, and transient `/dev` names are not sufficient
durable identities. Synthetic/injectable evidence is useful for deterministic
tests but does not replace validation on representative hardware.

## Adaptive Connection Substrate

ACS represents governed relationships among nodes, services, cognitive
structures, and resources. A relationship is not a socket, process, route,
credential, or physical link. Logical continuity is separate from transport
and session state, and one logical relationship may use multiple paths.

Public ACS invariants include bounded identities and histories, explicit and
versioned lifecycle/operational/enforcement state, deterministic snapshots,
idempotent transitions, evidence-based admission, compartmentalization, and
bounded resource use. Discovery never grants trust or authority. Successful
admission evaluation is advisory and non-reserving until an owning system
performs an authorized action.

The public repository may contain specifications, interfaces, ABI definitions,
Linux-facing integration, public transports, and runtime glue. Proprietary ACS
logic, production topology, credentials, trust thresholds, routing policy, and
private deployment behavior remain external.

## Memory and immune boundaries

The MEM specifications define public memory identities, roles, operations,
availability, lifecycle, custody, recovery, and ACS interaction. They do not
currently imply a persistence implementation. Memory authority stays separate
from communication authority and runtime execution.

The IMM specifications define public charter and invariants. Private detection
logic, heuristics, thresholds, production response policy, credentials, and
topology do not belong in the public repository. IMM may assess and request
bounded action; it does not silently acquire another subsystem's authority.

## Headless and minimal-system requirements

Core behavior must work on small, headless Linux systems without GUI, display
server, desktop services, interactive prompts, or local human-readable output.
Structured diagnostics and serial output are preferred at boot. Operator
interaction is expected to evolve toward remote signed tooling.

Drivers, firmware, CUDA/ROCm toolkits, kernels, system packages, storage layout,
and services belong in explicit provisioning/system profiles, not Python
requirements. Destructive provisioning, trust expansion, propagation, media
writes, and production activation require explicit authorization and auditable
results.

## Development and VM philosophy

The intended loop is edit, compile into a named `build/` directory, assemble a
candidate, boot the candidate in QEMU, inspect serial evidence, debug from the
host, and repeat. Host tests validate components; they do not substitute for a
boot proof. QEMU tests the actual Node boot path before physical deployment.
Containers may support reproducibility or CI but are not required for ordinary
development.

Build output is disposable and non-authoritative. Git history preserves source
history; build checkpoints do not.

## Architectural invariants

- No backend, device, service, node, scheduler, or branch is the permanent
  cognitive master.
- CPU-only and GPU-absent operation remain valid.
- Unknown state remains explicit.
- Discovery and planning have no implicit activation or trust authority.
- Public/private and ownership boundaries are explicit and versioned.
- Bounded data, queues, histories, retries, timeouts, and diagnostics are the
  default.
- Learning or generated code may propose and validate changes but cannot
  promote itself.
- Kernel registration, hardware compatibility, binary coverage, and a typed
  adapter must all be proven before execution.
- Production changes require provenance, validation, authorization, rollback,
  and truthful failure reporting.
