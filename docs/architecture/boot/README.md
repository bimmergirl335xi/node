# Node BOOT Architecture Specifications

The **Node BOOT Architecture** specification series defines the public architectural contracts governing minimal boot inspection, rescue selection, assistance coordination, recovery-plan validation, artifact-recovery coordination, installer handoff, recovery verification, and reboot handoff within Node.

BOOT exists for conditions in which a node is incomplete, damaged, misconfigured, partially functional, or unable to launch its normal runtime.

BOOT specifications are written before production implementation so that rescue behavior remains explicit, bounded, attributable, conservative under uncertainty, and subordinate to the architectures that own identity, authority, transport, installation, security, memory, immune behavior, and resource management.

BOOT is an architecture and coordination layer. It is not a replacement operating system, installer, transport stack, cryptographic provider, admission system, memory authority, immune authority, or resource manager.

## Classification

| Field | Value |
|---|---|
| Work-item family | BOOT-PUB |
| Classification | PUBLIC-ARCHITECTURE |
| Authoring lane | `lane/docs` |
| Owned path | `docs/architecture/boot/` |
| Reserved operational path | `bootstrap/` |

This directory contains independently authored public architecture.

Public BOOT specifications MUST NOT be produced by deleting, sanitizing, paraphrasing, or selectively redacting restricted designs. Production trust topology, credentials, private authorization policy, responder-selection logic, recovery secrets, infrastructure addresses, artifact locations, protected hardware topology, and operator playbooks remain outside this public series.

## Normative language

The terms **MUST** and **MUST NOT** describe mandatory architectural requirements.

The terms **SHOULD** and **SHOULD NOT** describe strong recommendations that may be departed from only with documented justification.

The term **MAY** describes permitted behavior.

A numbered BOOT specification may define additional terms and states for its own scope. Such definitions MUST preserve the authority and distinctions established by lower-numbered BOOT specifications and neighboring architecture.

## Directory boundary

`docs/architecture/boot/` contains public BOOT architecture documents.

`bootstrap/` is reserved for future concrete boot and recovery artifacts, including:

- initramfs contents;
- rescue images;
- generated manifests;
- installer inputs;
- enrollment bundles;
- staged recovery payloads;
- other operational boot or rescue outputs.

Architecture documents MUST NOT be placed in `bootstrap/`.

The existence, location, generation, or successful loading of an artifact under `bootstrap/` MUST NOT by itself establish that the artifact is authenticated, authorized, verified, installed, active, or suitable for normal runtime operation.

## Specification authority

The BOOT series is hierarchical:

1. **BOOT-0000 — Charter and Scope** defines BOOT’s enduring purpose, ownership boundaries, terminology, responsibilities, exclusions, and relationships with neighboring systems.
2. **BOOT-0001 — Core Invariants** defines the mandatory architectural rules that every conforming BOOT implementation must preserve.
3. Later specifications refine state, trust, discovery, assistance, recovery plans, artifacts, runtime constraints, integration, intervention, publication boundaries, and implementation checkpoints without contradicting higher-authority specifications.

Lower-numbered specifications carry broader architectural authority.

Later specifications MAY clarify or specialize earlier requirements, but they MUST NOT silently contradict them. A discovered conflict must be reviewed and deliberately corrected, revised, or superseded before implementation normalizes it.

This README is an index and orientation document. If it conflicts with an approved numbered BOOT specification, the numbered specification governs and the README must be corrected.

## BOOT charter direction

BOOT coordinates a bounded recovery sequence that may include:

- inspecting the locally observable boot condition;
- selecting between normal startup, restricted startup, rescue operation, operator review, or refusal to proceed;
- reporting failure, uncertainty, unavailability, invalidity, deferral, and locked-down conditions explicitly;
- discovering possible sources of assistance;
- negotiating an assistance session;
- validating that a proposed recovery plan is eligible for further consideration;
- coordinating retrieval of referenced recovery artifacts;
- authorizing and preparing an installer handoff through the applicable authority boundary;
- receiving and evaluating installer results;
- verifying the declared recovery outcome;
- coordinating a reboot or normal-runtime handoff.

Coordination does not transfer ownership.

BOOT MUST preserve the distinction between requesting an operation, authorizing it, performing it, verifying it, and accepting its result.

## Foundational ownership boundaries

BOOT does not absorb the responsibilities of adjacent systems.

| System or architecture | Retained responsibility | BOOT relationship |
|---|---|---|
| **ACS** | Participant and endpoint identities, relationships, connection descriptors, authority and capability references, evidence references, admission, lifecycle, operational condition, enforcement condition, revision, and idempotency semantics | BOOT consumes applicable ACS concepts and contracts but does not redefine, bypass, or privately substitute for them |
| **Installer** | Disk preparation, operating-system deployment, package and artifact installation, durable configuration writes, activation staging, and rollback mechanics | BOOT may authorize and coordinate a handoff but does not perform installer mutations |
| **Transport** | Byte movement, framing, addressing, retransmission, and link-specific behavior | BOOT defines semantic requests and results rather than packet formats |
| **Security provider** | Cryptographic identity, key custody, authentication, signature and digest verification, secure-session establishment, credential storage, and replay-protection primitives | BOOT requests and consumes security results but does not implement private-key custody or treat reachability as authentication |
| **IMM** | Bounded immune evidence, assessment, recommendation, protective coordination, and immune recovery verification | IMM may provide evidence or request recovery but cannot self-enroll, self-authorize recovery, bypass BOOT, or replace bootstrap authority |
| **MEM** | Memory identity, semantic acceptance, persistence, provenance, custody, retention, reconstruction, deletion, and memory recovery authority | BOOT may request governed persistence of bounded recovery state but does not create semantic truth or memory authority |
| **Resource management** | Allocation, accounting, reservations, ceilings, pressure handling, reclamation, and resource policy | BOOT operates within declared limits and cannot manufacture capacity or override hard ceilings |
| **Normal runtime** | Production execution, scheduling, service operation, and ordinary runtime lifecycle after an accepted handoff | BOOT may prepare or verify a handoff but does not treat attempted startup as proof of successful normal operation |

No boundary may be bypassed by relabeling an operation as rescue, emergency, bootstrap, local, automatic, trusted, restorative, or necessary.

## Required trust and completion distinctions

The BOOT architecture preserves the following distinctions:

- discovery is not authentication;
- authentication is not authority;
- authority is not admission;
- selection is not authorization;
- transfer is not verification;
- verification is not installation;
- installation is not activation;
- activation is not recovery;
- recovery is not successful normal runtime operation.

No stage silently proves a later stage.

A successful earlier stage MAY provide evidence required by a later stage. The later stage remains responsible for applying its own requirements and reporting its own result.

## State-model direction

BOOT MUST NOT use one oversized state value to represent the entire recovery process.

At minimum, public BOOT architecture distinguishes independent dimensions for:

- local boot state;
- assistance-session state;
- authority state;
- artifact state;
- installer state.

These dimensions may advance, regress, fail, become unavailable, or remain unresolved independently.

State reporting must preserve explicit conditions including:

- **Unknown:** available evidence is insufficient to establish the condition.
- **Unavailable:** a required participant, dependency, authority, artifact, or operation is known to be inaccessible.
- **Invalid:** supplied material or state fails an applicable structural, semantic, security-provider, revision, or policy requirement.
- **Deferred:** the operation has not been rejected but cannot proceed under the present conditions.
- **Failed:** a declared operation did not satisfy its required completion conditions.
- **Locked down:** policy or an authorized protective condition prohibits progression beyond a declared boundary.

Later specifications define the complete state vocabulary and transition rules for each dimension.

Unknown MUST NOT be converted into healthy, safe, authenticated, authorized, absent, compatible, current, or successful state.

Failure in one dimension MUST NOT silently overwrite or fabricate state in another dimension.

## Identity and authority direction

Public BOOT architecture distinguishes rescue-session correlation from durable participant identity.

The series may define profiles including:

- `BOOT_EPHEMERAL`;
- `BOOT_PROVISIONAL`;
- `BOOT_ROOTED`.

No identity profile grants automatic recovery authority.

An enrollment token is evidence used to obtain or validate identity. It is not itself the participant identity.

Previously persisted identity must be revalidated before being relied upon for a new BOOT session.

Durable identity MUST NOT be derived solely from:

- an IP address;
- a MAC address;
- a hostname;
- a route;
- a disk path;
- a process identifier;
- boot order;
- physical proximity;
- the first responder.

Public authority-anchor categories may include operator-approved, installer-provisioned, hardware-rooted, manufacturer-provisioned, and previously persisted and revalidated anchors.

Discovery mechanisms and removable media MAY carry authority-anchor candidates. They do not create authority.

The public series does not publish production anchors, credentials, addresses, repositories, private enrollment policy, or private recovery-authorization policy.

## Minimal-runtime direction

BOOT must remain suitable for extremely minimal headless Linux environments.

A conforming architecture MUST NOT require:

- a graphical interface;
- a display server;
- a desktop session;
- a local human-readable console;
- the normal Node runtime;
- Julia;
- dynamically available high-level orchestration services;
- unrestricted heap growth;
- network availability as a prerequisite for all recovery paths.

Implementations MAY operate in initramfs, BusyBox-style, rescue-image, restricted userspace, or similarly constrained environments.

Local terminal output MAY exist as diagnostic behavior. It is not an architectural dependency.

The minimal rescue executable language remains undecided among:

- strict C;
- restricted C++;
- static Rust.

The choice must follow measured evidence concerning binary size, startup dependencies, allocator behavior, exception or panic behavior, initramfs suitability, cross-compilation, parser safety, cryptographic integration, x86_64 validation, AArch64 validation, and behavior on partially functional hardware.

Julia MAY participate in higher-level orchestration or analysis after verified BOOT results exist. Julia is not part of the minimal rescue executable.

## Language-neutral contract direction

Public BOOT contracts remain language-neutral.

Future public value and operation contracts SHOULD support:

- fixed-width categories;
- bounded identifiers;
- explicit lengths;
- caller-owned buffers;
- structured result codes;
- independent revisions;
- deterministic bounded issue lists;
- checked arithmetic;
- explicit resource-exhaustion outcomes;
- no partial authoritative commit after fallible construction;
- no exceptions crossing an ABI;
- no transferred allocator ownership;
- no raw credentials or private-key material;
- no STL exposure across a C-compatible boundary.

The public architecture does not define a C ABI merely by stating these requirements.

## Failure and atomicity direction

BOOT follows the corrective atomicity rule used by neighboring architecture:

> Complete all fallible construction, validation, bounded retention, and result preparation before committing authoritative state.

Allocation, storage, diagnostic, encoding, or retention failure MUST:

- return an explicit conservative outcome;
- leave authoritative state unchanged;
- never report false success;
- never silently lose revision or idempotency information;
- never commit an incomplete recovery decision merely because reporting the complete result failed.

Resource exhaustion is a first-class failure outcome. It MUST NOT be converted into malformed success, an unbounded retry, or partial authoritative mutation.

## Current specifications

| Specification | Title | Status |
|---|---|---|
| [BOOT-0000](BOOT-0000-charter-and-scope.md) | Charter and Scope | Draft |
| [BOOT-0001](BOOT-0001-core-invariants.md) | Core Invariants | Planned |
| [BOOT-0002](BOOT-0002-state-and-failure-model.md) | State and Failure Model | Planned |
| [BOOT-0003](BOOT-0003-identity-trust-and-authority-boundary.md) | Identity, Trust, and Authority Boundary | Planned |
| [BOOT-0004](BOOT-0004-discovery-and-assistance-negotiation.md) | Discovery and Assistance Negotiation | Planned |
| [BOOT-0005](BOOT-0005-recovery-plans-and-artifact-boundaries.md) | Recovery Plans and Artifact Boundaries | Planned |
| [BOOT-0006](BOOT-0006-minimal-runtime-and-language-constraints.md) | Minimal Runtime and Language Constraints | Planned |
| [BOOT-0007](BOOT-0007-acs-imm-and-mem-integration.md) | ACS, IMM, and MEM Integration | Planned |
| [BOOT-0008](BOOT-0008-failure-retry-and-operator-intervention.md) | Failure, Retry, and Operator Intervention | Planned |
| [BOOT-0009](BOOT-0009-public-private-boundary-and-implementation-roadmap.md) | Public/Private Boundary and Implementation Roadmap | Planned |

The series may be refined through review. Approved documents must not be renumbered or substantially restructured without explicit architectural discussion.

## Document status

BOOT documents use the following status values:

- **Planned:** responsibility and boundaries are reserved, but normative architecture has not yet been drafted.
- **Draft:** under active review and subject to substantial revision.
- **Stable:** accepted architectural guidance suitable for implementation.
- **Frozen:** foundational intent that should change only through explicit supersession.
- **Deprecated:** retained for history but no longer recommended.
- **Superseded:** replaced by a named later specification.

A planned entry preserves scope and sequencing. It does not make its listed subject normative.

## Public and private boundary

Public BOOT documentation MAY contain:

- architecture;
- invariants;
- public state vocabulary;
- bounded value contracts;
- public failure outcomes;
- ownership and authority boundaries;
- retry semantics;
- minimal-runtime requirements;
- implementation checkpoints;
- public conformance requirements.

Public BOOT documentation MUST NOT contain:

- production trust topology;
- real credentials or key material;
- infrastructure addresses;
- artifact-repository locations;
- private signing policy;
- private enrollment policy;
- private recovery-authorization rules;
- responder-selection algorithms;
- anomaly thresholds;
- behavioral fingerprints;
- private hardware topology;
- hidden scores or reasoning;
- operator recovery secrets or production playbooks.

Public contracts must be sufficient to implement and test the declared architectural behavior without exposing the private mechanisms or policy that select among eligible production actions.

Undecided private mechanism is not public architecture.

## Publication gate

Before a BOOT document is committed publicly, it must pass:

1. independent-authorship and public-source review;
2. BOOT hierarchy and terminology review;
3. ACS identity, admission, lifecycle, revision, idempotency, and enforcement-boundary review;
4. installer, transport, and security-provider ownership review;
5. IMM and MEM authority-boundary review;
6. runtime and resource-management boundary review;
7. minimal headless-runtime and language-neutrality review;
8. state honesty, failure, retry, and atomicity review;
9. restricted-information and production-detail contamination review;
10. substantive approval;
11. a fresh read of the target documents and trusted repository baseline;
12. final diff and path-scope review before commit.

Passing the publication gate does not authorize implementation or deployment.

## Node engineering method

Each BOOT architectural concept should answer:

1. **What is it?**
2. **Why does it exist?**
3. **What does it enable?**
4. **What does it forbid?**
5. **Who owns the authority?**
6. **What evidence establishes the condition?**
7. **How does it fail?**
8. **How is uncertainty represented?**
9. **What remains unchanged when the operation fails?**
10. **How is successful handoff or recovery verified?**

Architecture precedes interfaces. Interfaces precede implementation. Implementation access does not create architectural authority.

> Recovery remains trustworthy only when discovery, identity, authority, transfer, verification, installation, activation, and successful operation remain distinguishable.
