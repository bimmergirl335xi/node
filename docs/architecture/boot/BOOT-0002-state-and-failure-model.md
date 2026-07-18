# BOOT-0002: Node BOOT State and Failure Model

| Field | Value |
|---|---|
| Specification | BOOT-0002 |
| Title | Node BOOT State and Failure Model |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | BOOT-PUB |
| Authors | Node |
| Last updated | 2026-07-17 |
| Approval | Pending review |
| Depends on | BOOT-0000; BOOT-0001; applicable approved ACS, MEM, and IMM public architecture |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in multidimensional state, truthful failure, conservative transition, revision, atomicity, and reboot-continuation requirements; exact numeric encodings, bounded capacities, persistence formats, and implementation synchronization remain intentionally deferred |

> **BOOT state is conforming only when every claim identifies what is known, what remains unresolved, which dimension changed, who produced the evidence, and what did not change.**

## Architectural-intent notice

This specification defines the public state and failure model for Node BOOT.

It establishes:

- independently observable BOOT state dimensions;
- orthogonal state facets;
- dimension-specific phase vocabularies;
- authoritative transition rules;
- revision and idempotency behavior;
- failure classification;
- partial and indeterminate outcomes;
- derived boot dispositions;
- reboot and process-restart continuation;
- bounded issue and diagnostic behavior;
- resource-exhaustion behavior;
- public conceptual state records;
- conformance expectations.

This specification refines BOOT-0000 and BOOT-0001.

It does not replace or weaken them.

This specification defines state semantics, not:

- one implementation language;
- one ABI;
- one binary encoding;
- one database;
- one persistence mechanism;
- one state-machine library;
- one scheduler;
- one transport;
- one installer;
- one operating-system layout;
- one update protocol;
- one operator interface.

Implementations may use different mechanisms.

Their observable behavior MUST preserve the dimensions, facets, transition rules, failure meanings, revision semantics, and authority boundaries defined here.

## 1. Purpose

BOOT coordinates work across incomplete, damaged, partially functional, and dynamically assembling systems.

A single value such as:

```text
BOOT_OK
BOOT_FAILED
RECOVERING
READY
```

cannot truthfully represent:

- local inspection state;
- assistance progress;
- identity or authority state;
- artifact transfer and verification;
- installer mutation;
- rollback;
- recovery verification;
- reboot handoff;
- runtime-reported readiness;
- resource exhaustion;
- unresolved prior work.

BOOT-0002 prevents those meanings from collapsing into one global state.

Its purpose is to ensure that a BOOT implementation can answer:

1. Which subject does this state describe?
2. Which independent dimension does it describe?
3. Which phase has been reached?
4. What is known?
5. What remains unknown or conflicting?
6. Is the required provider available?
7. Was the subject evaluated?
8. Is the evidence current?
9. Is work pending, active, deferred, complete, failed, or indeterminate?
10. Is progression restricted or locked down?
11. Are resources sufficient?
12. What scoped outcome was produced?
13. Which revision does the state represent?
14. Which operation and attempt produced it?
15. Did authoritative state change?
16. Is durable mutation known to have occurred?
17. Is reconciliation required?
18. What may happen next?

The model is intended to govern:

- normal boot;
- first installation;
- rescue;
- recovery;
- installation-media boot;
- dynamic self-assembly;
- degraded operation;
- offline operation;
- update preparation;
- activation;
- rollback;
- reboot;
- runtime handoff;
- BOOT-P0 through BOOT-P6;
- later production BOOT implementations.

## 2. Authority and relationship to earlier BOOT specifications

BOOT-0000 establishes that BOOT:

- preserves independent state dimensions;
- keeps unknown and unavailable conditions explicit;
- separates every trust and completion stage;
- uses conservative atomicity;
- preserves revisions and idempotency;
- does not claim runtime success from a BOOT handoff.

BOOT-0001 establishes the canonical invariants refined here, especially:

- BOOT-INV-004 — Trust and completion stages remain distinct.
- BOOT-INV-009 — BOOT state remains multidimensional.
- BOOT-INV-010 — Unknown, absence, unavailability, and invalidity remain truthful.
- BOOT-INV-011 — Failure, silence, liveness, launch, and restart do not fabricate completion.
- BOOT-INV-013 — Resource exhaustion is a first-class outcome.
- BOOT-INV-014 — Authoritative state changes are conservatively atomic.
- BOOT-INV-015 — Revision and idempotency survive retry and interruption.
- BOOT-INV-021 — Installer mutation, rollback, and reconciliation remain explicit.
- BOOT-INV-024 — Image boot and hardware discovery do not authorize participation or installation.
- BOOT-INV-026 — Kernel/micro-OS pairs and required components are validated as scoped assemblies.
- BOOT-INV-027 — Update availability does not create activation authority.
- BOOT-INV-028 — Peer propagation remains bounded and locally validated.
- BOOT-INV-030 — Runtime handoff eligibility is not runtime readiness.

If this specification conflicts with BOOT-0000 or BOOT-0001:

1. the lower-numbered specification governs;
2. the conflict MUST be reported;
3. this specification MUST be corrected;
4. an implementation MUST NOT choose the more permissive interpretation.

No blocking contradiction with BOOT-0000 or the approved BOOT-0001 draft was identified.

## 3. Normative language

The terms **MUST** and **MUST NOT** define mandatory architectural requirements.

The terms **SHOULD** and **SHOULD NOT** define strong recommendations. Departure requires documented justification and MUST NOT violate a mandatory requirement.

The term **MAY** defines permitted behavior.

State labels are scoped terms.

For example:

- `VALID` means valid under an identified evaluation contract;
- `AVAILABLE` means available to an identified subject for an identified purpose;
- `COMPLETE` means the identified attempt has stopped progressing;
- `SATISFIED` means declared completion conditions for one scoped operation were met;
- `LOCKED_DOWN` means progression is prohibited within an identified boundary.

None of those labels establishes universal truth, safety, authority, health, or readiness.

## 4. Scope

This specification governs state and failure semantics for:

- BOOT sessions;
- local boot inspection;
- local boot disposition;
- assistance coordination;
- authority evaluation;
- artifact acquisition;
- artifact verification and compatibility;
- installer handoff;
- installer mutation;
- installer verification;
- rollback;
- recovery verification;
- reboot preparation;
- runtime handoff;
- runtime-reported observations;
- dynamic assembly generations;
- update preparation and local validation;
- bounded BOOT persistence;
- retry and interruption evidence;
- public diagnostics;
- conformance testing.

It applies whether BOOT runs:

- from initramfs;
- from an installation image;
- from removable rescue media;
- from a restricted installed environment;
- in QEMU;
- on x86_64;
- on AArch64;
- with or without networking;
- with read-only or damaged storage;
- with incomplete hardware support;
- with mocked or production adjacent providers.

## 5. Explicit non-goals

BOOT-0002 does not define:

- complete identity-profile vocabulary;
- authentication mechanisms;
- authority-anchor formats;
- capability formats;
- ACS state-machine internals;
- discovery protocols;
- assistance message formats;
- responder-selection policy;
- recovery-plan schemas;
- artifact manifests;
- installer APIs;
- filesystem layout;
- update manifests;
- release channels;
- rollout algorithms;
- exact retry policy;
- exact operator-intervention policy;
- exact numeric enum values;
- exact identifier lengths;
- exact issue-list capacities;
- exact persistence encoding;
- exact time source;
- exact locking primitive;
- one transactional storage system;
- one state-machine framework;
- one programming language.

Those details belong to BOOT-0003 through BOOT-0009, future OS and update architecture, provider-specific architecture, or implementation profiles.

## 6. State-model foundations

### 6.1 State has a subject

Every authoritative state record MUST identify the subject it describes.

Possible subjects include:

- one BOOT session;
- one physical substrate;
- one local inspection attempt;
- one assistance session;
- one identity or authority claim;
- one artifact;
- one installer operation;
- one rollback operation;
- one recovery-verification operation;
- one assembly generation;
- one update proposal;
- one reboot handoff;
- one runtime observation.

A state without an identifiable subject MUST NOT be treated as authoritative.

### 6.2 State has a scope

Every state claim MUST identify its applicable scope.

Examples include:

- one operation;
- one attempt;
- one artifact revision;
- one authority grant;
- one target disk;
- one selected generation;
- one recovery objective;
- one runtime handoff;
- one optional capability;
- one required component set.

A scoped success MUST NOT be interpreted as universal success.

### 6.3 State has an owner or reporting provider

Each state record MUST identify:

- the architecture or component that owns the state;
- the component that reported or observed it;
- the provider whose result is being represented where applicable.

BOOT MAY retain provider-produced state as evidence.

BOOT MUST NOT silently relabel provider-produced state as a BOOT-owned decision.

### 6.4 State has a revision

Every authoritative state record MUST carry a revision sufficient to detect:

- stale writes;
- conflicting updates;
- duplicate transitions;
- supersession;
- retry;
- reconciliation;
- reloaded state from persistence.

A revision MUST NOT be inferred solely from wall-clock time.

### 6.5 State has evidence boundaries

A state claim SHOULD identify the evidence supporting it.

Where applicable, it MUST preserve:

- evidence references;
- evidence provider;
- observation scope;
- freshness;
- integrity status;
- unresolved conflicts;
- transformation lineage.

Missing evidence is not positive or negative confirmation.

### 6.6 State has an operation and attempt context

The following identities MUST remain distinguishable:

- BOOT session identity;
- boot-attempt identity;
- semantic operation identity;
- operation-attempt identity;
- transport attempt;
- installer attempt;
- transition identity;
- snapshot identity;
- reboot or handoff attempt identity.

One implementation object MAY represent several of these identities only when their independent meanings remain observable.

### 6.7 State is not a log line

A log entry MAY report or explain state.

A log entry is not authoritative state merely because it was emitted.

Human-readable output MAY be absent.

Machine-readable state and result behavior MUST remain available within the implementation profile.

### 6.8 State is not inferred from process behavior alone

Process launch, process liveness, process exit, signal receipt, transport closure, watchdog activity, or reboot MUST NOT independently establish semantic completion.

## 7. Multidimensional state structure

BOOT state consists of:

1. a **dimension-specific phase**;
2. orthogonal **state facets**;
3. an optional scoped **operation outcome**;
4. revision and identity information;
5. evidence and issue references.

A conceptual state record is:

```text
BootDimensionState
    subject_identity
    dimension_identity
    phase
    knowledge_state
    availability_state
    validity_state
    freshness_state
    progress_state
    enforcement_state
    resource_state
    outcome_state
    state_revision
    operation_identity
    attempt_identity
    reporting_provider
    evidence_references
    issue_references
    transition_identity
    prior_revision
    monotonic_sequence
    wall_time_state
    authoritative_change
```

This is a language-neutral semantic record.

It is not a required wire format or memory layout.

### 7.1 No oversized global enumeration

Implementations MUST NOT construct one global enumeration containing every permutation of:

- phase;
- unknown;
- availability;
- validity;
- freshness;
- progress;
- restriction;
- resource condition;
- outcome.

For example, an implementation SHOULD NOT require values such as:

```text
ARTIFACT_TRANSFERRED_BUT_STALE_AUTHORITY_UNAVAILABLE_RETRY_PENDING
```

Those meanings belong in independent fields and dimensions.

### 7.2 Facets do not erase phases

A dimension may remain in phase `TRANSFER_OBSERVED` while:

- validity is `NOT_EVALUATED`;
- freshness is `CURRENT`;
- progress is `COMPLETE`;
- outcome is `SATISFIED`;
- installation eligibility remains unresolved elsewhere.

The phase and facets MUST remain independently visible.

### 7.3 Phases do not prove outcomes

Reaching a phase does not automatically establish a favorable outcome.

For example:

- `INSPECTION_EVALUATED` does not mean normal handoff is eligible;
- `TRANSFER_OBSERVED` does not mean verification succeeded;
- `FINAL_RESULT_AVAILABLE` does not mean installer success;
- `CONTROL_TRANSFER_OBSERVED` does not mean runtime readiness;
- `READINESS_REPORTED` does not prove sustained runtime operation.

## 8. Orthogonal state facets

### 8.1 Knowledge state

| Value | Meaning |
|---|---|
| `NOT_APPLICABLE` | The knowledge facet does not apply to this subject and phase. |
| `UNKNOWN` | Available evidence is insufficient to establish the condition. |
| `PARTIAL` | Some required evidence is established, but material evidence remains absent or unresolved. |
| `ESTABLISHED` | The required evidence set for the declared scope is established. |
| `CONFLICTING` | Material evidence supports incompatible conclusions. |

Rules:

- `UNKNOWN` MUST NOT be treated as `ESTABLISHED`.
- `PARTIAL` MUST identify the missing or unresolved scope where public.
- `CONFLICTING` MUST remain explicit until governed resolution occurs.
- More evidence does not automatically produce `ESTABLISHED`; the owning evaluation contract decides.

### 8.2 Availability state

| Value | Meaning |
|---|---|
| `NOT_APPLICABLE` | Availability does not apply to this subject or phase. |
| `UNKNOWN` | Availability has not been established. |
| `AVAILABLE` | The subject or provider is available for the declared operation and scope. |
| `DEGRADED` | The subject is available with declared limitations. |
| `UNAVAILABLE` | The subject is known to be inaccessible for the declared operation and scope. |

Rules:

- `UNAVAILABLE` does not mean `INVALID`.
- `AVAILABLE` does not mean authorized, admitted, compatible, or sufficient.
- `DEGRADED` MUST identify the limitation relevant to the operation.
- Connection loss may change availability without proving identity or validity failure.

### 8.3 Validity state

| Value | Meaning |
|---|---|
| `NOT_APPLICABLE` | Validity evaluation does not apply. |
| `NOT_EVALUATED` | No applicable validation result exists yet. |
| `UNKNOWN` | Evaluation was attempted or required, but validity could not be established. |
| `VALID` | The subject satisfied an identified validation contract for the declared scope. |
| `INVALID` | The subject failed an identified validation contract. |
| `UNSUPPORTED` | The evaluator does not support the subject, schema, version, operation, or required mechanism. |
| `CONFLICTING` | Eligible validators or evidence produced incompatible validity results. |

Rules:

- `VALID` MUST identify the applicable contract or provider.
- `INVALID` SHOULD identify the failed requirement where disclosure is allowed.
- `UNSUPPORTED` is not `INVALID`.
- `VALID` does not establish authority, compatibility, installation eligibility, or activation eligibility unless that specific evaluation contract owns that decision.

### 8.4 Freshness state

| Value | Meaning |
|---|---|
| `NOT_APPLICABLE` | Freshness does not apply. |
| `NOT_EVALUATED` | No freshness evaluation has occurred. |
| `UNKNOWN` | Freshness cannot be established. |
| `CURRENT` | Evidence is current within an identified freshness boundary. |
| `STALE` | Evidence is outside its accepted freshness boundary but is not necessarily expired or invalid. |
| `EXPIRED` | The applicable validity period ended. |
| `CLOCK_UNAVAILABLE` | Required time evidence or clock support is unavailable. |
| `CONFLICTING` | Eligible time or revision evidence conflicts. |

Rules:

- Wall-clock time alone MUST NOT determine authority or ordering.
- Newest timestamp does not automatically mean newest valid state.
- `CLOCK_UNAVAILABLE` MUST NOT be converted into `CURRENT`.
- A provider MAY establish freshness through revisions, counters, challenges, bounded age, secure time, or another governed mechanism.

### 8.5 Progress state

| Value | Meaning |
|---|---|
| `NOT_STARTED` | The operation or phase has not begun. |
| `PENDING` | Work is eligible to begin or awaits a prerequisite. |
| `ACTIVE` | Work is currently progressing. |
| `WAITING` | Work is active but awaits an external event or provider. |
| `DEFERRED` | Progression is intentionally postponed under current conditions. |
| `INTERRUPTED` | Work stopped before a final semantic outcome was established. |
| `CANCELED` | Work was stopped through an applicable cancellation decision. |
| `COMPLETE` | The attempt reached a final outcome, favorable or unfavorable. |
| `FAILED` | The attempt reached a declared failure outcome. |
| `INDETERMINATE` | The implementation cannot establish whether or how the attempt completed. |

Rules:

- `COMPLETE` does not mean successful.
- `FAILED` describes the operation attempt, not the target’s health or intent.
- `INTERRUPTED` does not establish whether mutation occurred.
- `INDETERMINATE` requires reconciliation before unsafe repetition.

### 8.6 Enforcement state

| Value | Meaning |
|---|---|
| `OPEN` | No BOOT-scoped restriction is presently applied to progression. |
| `RESTRICTED` | Progression is permitted only within an identified reduced boundary. |
| `LOCKED_DOWN` | Progression beyond an identified boundary is prohibited. |

Rules:

- `LOCKED_DOWN` MUST identify scope and authority.
- `LOCKED_DOWN` is not proof of compromise.
- Lock-down MUST preserve applicable safety, operator, evidence, audit, and recovery paths unless a separately authorized replacement exists.
- Enforcement state MUST NOT erase other facets or dimensions.

### 8.7 Resource state

| Value | Meaning |
|---|---|
| `NOT_APPLICABLE` | Resource evaluation does not apply. |
| `UNKNOWN` | Required resource sufficiency has not been established. |
| `SUFFICIENT` | Declared resource requirements are satisfied within applicable limits. |
| `CONSTRAINED` | Work may proceed only within identified reduced limits. |
| `EXHAUSTED` | A required bounded resource is unavailable or its limit is reached. |

Rules:

- Resource availability does not create authority or admission.
- Resource exhaustion does not prove attack or compromise.
- `CONSTRAINED` MUST identify the resulting operation limit.
- `EXHAUSTED` MUST produce an explicit outcome.

### 8.8 Outcome state

| Value | Meaning |
|---|---|
| `NONE` | No final scoped outcome exists. |
| `SATISFIED` | Declared completion conditions for the scoped operation were met. |
| `SATISFIED_WITH_LIMITATIONS` | Declared minimum completion conditions were met with explicit remaining limitations. |
| `NO_CHANGE` | The operation completed without authoritative or durable mutation. |
| `REJECTED` | The receiving contract rejected the request or input. |
| `DENIED` | An applicable authority denied the requested operation. |
| `PARTIAL` | Some declared effects occurred, but full completion conditions were not met. |
| `DEFERRED` | A final favorable or unfavorable decision was postponed. |
| `CANCELED` | The operation ended through governed cancellation. |
| `INTERRUPTED` | The operation ended before final completion was established. |
| `FAILED` | The operation did not satisfy its declared completion conditions. |
| `INDETERMINATE` | Completion or effects cannot be established. |
| `RESOURCE_EXHAUSTED` | A bounded resource prevented completion. |
| `TIMED_OUT` | An observed deadline or timeout boundary was reached without the required completion evidence. |
| `UNSUPPORTED` | The required operation, version, subject, or mechanism is unsupported. |

Rules:

- An outcome MUST identify its operation and scope.
- `SATISFIED_WITH_LIMITATIONS` MUST enumerate the limitations required for downstream decisions.
- `PARTIAL`, `INTERRUPTED`, and `INDETERMINATE` MUST identify whether durable mutation is possible, known, or unknown.
- `TIMED_OUT` is not proof that the remote or local target failed.
- `REJECTED` and `DENIED` remain distinct.

## 9. Required BOOT state dimensions

BOOT-0000 mandates at least:

- local boot;
- assistance session;
- authority;
- artifact;
- installer.

BOOT-0002 additionally defines:

- BOOT session coordination;
- recovery verification;
- reboot and handoff;
- runtime observation.

Dynamic assembly and update dimensions become required when those capabilities are present.

### 9.1 BOOT session coordination dimension

**Dimension identifier**

```text
BOOT_DIM_SESSION
```

**Purpose**

Represents the lifecycle of one BOOT coordination session.

**Phase vocabulary**

| Phase | Meaning |
|---|---|
| `SESSION_CREATED` | A bounded session identity exists. |
| `ENVIRONMENT_ESTABLISHING` | BOOT is preparing its minimal execution environment. |
| `SESSION_ACTIVE` | BOOT coordination is active. |
| `WAITING_EXTERNAL` | The session awaits an external provider, event, authority, or operator decision. |
| `CONTINUATION_REQUIRED` | The session cannot safely conclude without reboot or later reconciliation. |
| `SESSION_CLOSING` | Final state and continuation preparation are underway. |
| `SESSION_CLOSED` | The session produced a final scoped session result. |

**Nominal transition graph**

```text
SESSION_CREATED
    → ENVIRONMENT_ESTABLISHING
    → SESSION_ACTIVE
    ↔ WAITING_EXTERNAL
    → CONTINUATION_REQUIRED
    → SESSION_CLOSING
    → SESSION_CLOSED
```

`SESSION_ACTIVE` MAY transition directly to `SESSION_CLOSING` when no continuation is required.

A new process MUST NOT silently create a new semantic BOOT session when a valid continuation record identifies the prior session.

### 9.2 Local boot dimension

**Dimension identifier**

```text
BOOT_DIM_LOCAL
```

**Purpose**

Represents what BOOT can establish about the local substrate and eligible next-path candidates.

**Phase vocabulary**

| Phase | Meaning |
|---|---|
| `NOT_INSPECTED` | No authoritative local inspection result exists. |
| `ENVIRONMENT_ESTABLISHED` | The minimal BOOT environment is sufficiently established for declared inspection work. |
| `INSPECTION_PENDING` | Inspection is eligible or awaiting prerequisites. |
| `INSPECTING` | Local inspection is active. |
| `INSPECTION_EVALUATED` | The inspection attempt produced a scoped result. |
| `DISPOSITION_PENDING` | BOOT is evaluating the candidate next path. |
| `DISPOSITION_SELECTED` | A candidate disposition has been selected. |
| `HANDOFF_PREREQUISITES_EVALUATED` | Known prerequisites for the selected handoff or rescue path were evaluated. |
| `LOCAL_DIMENSION_CLOSED` | No further local-state transition is expected in the current session. |

**Nominal transition graph**

```text
NOT_INSPECTED
    → ENVIRONMENT_ESTABLISHED
    → INSPECTION_PENDING
    → INSPECTING
    → INSPECTION_EVALUATED
    → DISPOSITION_PENDING
    → DISPOSITION_SELECTED
    → HANDOFF_PREREQUISITES_EVALUATED
    → LOCAL_DIMENSION_CLOSED
```

New material evidence MAY cause an explicit transition from:

- `DISPOSITION_SELECTED` to `INSPECTION_PENDING`;
- `HANDOFF_PREREQUISITES_EVALUATED` to `INSPECTION_PENDING`.

Such a transition requires a new revision and reason.

### 9.3 Assistance-session dimension

**Dimension identifier**

```text
BOOT_DIM_ASSISTANCE
```

**Purpose**

Represents BOOT’s progress in locating and interacting with bounded assistance.

**Phase vocabulary**

| Phase | Meaning |
|---|---|
| `ASSISTANCE_NOT_REQUIRED` | No assistance operation is required for the current path. |
| `ASSISTANCE_INACTIVE` | Assistance may be required but no search or interaction is active. |
| `ASSISTANCE_SEEKING` | BOOT is seeking candidate assistance within bounded rules. |
| `CANDIDATE_AVAILABLE` | At least one candidate source is known. |
| `ELIGIBILITY_EVALUATING` | Candidate eligibility is under evaluation. |
| `AUTHENTICATION_PENDING` | Authentication through the owning provider is pending. |
| `ADMISSION_PENDING` | Applicable ACS or operation admission is pending. |
| `NEGOTIATION_PENDING` | A bounded assistance operation is being negotiated. |
| `ASSISTANCE_ACTIVE` | An eligible assistance session is active. |
| `ASSISTANCE_WAITING_EXTERNAL` | The session awaits a response or external condition. |
| `ASSISTANCE_COMPLETE` | The assistance operation produced its scoped final outcome. |
| `ASSISTANCE_CLOSING` | Bounded cleanup and final result preparation are underway. |
| `ASSISTANCE_CLOSED` | The assistance session is closed. |

**Nominal transition graph**

```text
ASSISTANCE_INACTIVE
    → ASSISTANCE_SEEKING
    → CANDIDATE_AVAILABLE
    → ELIGIBILITY_EVALUATING
    → AUTHENTICATION_PENDING
    → ADMISSION_PENDING
    → NEGOTIATION_PENDING
    → ASSISTANCE_ACTIVE
    ↔ ASSISTANCE_WAITING_EXTERNAL
    → ASSISTANCE_COMPLETE
    → ASSISTANCE_CLOSING
    → ASSISTANCE_CLOSED
```

Later BOOT-0004 may refine or skip phases where the applicable operation does not require every stage.

Skipping a phase MUST be explicit and MUST NOT imply the skipped stage succeeded.

### 9.4 Authority dimension

**Dimension identifier**

```text
BOOT_DIM_AUTHORITY
```

**Purpose**

Represents whether applicable authority exists for one identified actor, target, operation, and scope.

**Phase vocabulary**

| Phase | Meaning |
|---|---|
| `AUTHORITY_NOT_REQUIRED` | The scoped operation does not require authority beyond the already established baseline. |
| `AUTHORITY_UNRESOLVED` | Required authority has not been established. |
| `AUTHORITY_EVIDENCE_PRESENT` | Candidate authority evidence exists. |
| `AUTHORITY_EVALUATION_PENDING` | Evaluation is awaiting provider or prerequisite work. |
| `AUTHORITY_EVALUATING` | Applicable authority evaluation is active. |
| `AUTHORITY_DECISION_AVAILABLE` | A scoped authority decision exists. |
| `AUTHORITY_REVIEW_PENDING` | Conflict, expiry, revocation, or ambiguity requires governed review. |
| `AUTHORITY_DIMENSION_CLOSED` | No further authority transition is expected for this operation attempt. |

**Authority decision vocabulary**

| Decision | Meaning |
|---|---|
| `NO_DECISION` | No authority decision exists. |
| `GRANTED` | The requested operation is authorized within its declared envelope. |
| `GRANTED_NARROWED` | A narrower actor, target, action, scope, duration, or condition is authorized. |
| `DENIED` | The requested operation is not authorized. |
| `DEFERRED` | A final decision awaits additional evidence, authority, or review. |
| `EXPIRED` | Previously applicable authority is no longer within its validity period. |
| `REVOKED` | Previously applicable authority was withdrawn. |
| `CONFLICTING` | Eligible authority decisions conflict. |
| `UNAVAILABLE` | The required authority source or decision path is unavailable. |
| `INDETERMINATE` | The decision or its applicability cannot be established. |

Authority is operation-scoped.

A grant for one operation MUST NOT be reused for another operation without an applicable authority relationship.

### 9.5 Artifact dimension

**Dimension identifier**

```text
BOOT_DIM_ARTIFACT
```

**Purpose**

Represents one artifact’s progression from reference through transfer, verification, compatibility evaluation, and eligibility.

**Phase vocabulary**

| Phase | Meaning |
|---|---|
| `ARTIFACT_NOT_REQUIRED` | No artifact is required for the scoped operation. |
| `ARTIFACT_UNREFERENCED` | No accepted artifact reference exists. |
| `ARTIFACT_REFERENCED` | A bounded artifact reference exists. |
| `ARTIFACT_LOCATING` | BOOT or an owning provider is locating eligible material. |
| `TRANSFER_PENDING` | Transfer is eligible or awaiting prerequisites. |
| `TRANSFERRING` | Byte movement is active through the owning transport. |
| `TRANSFER_OBSERVED` | The transport reported its scoped transfer result. |
| `VERIFICATION_PENDING` | Applicable artifact verification has not completed. |
| `VERIFYING` | Verification through the owning provider is active. |
| `COMPATIBILITY_PENDING` | Target compatibility remains unevaluated or unresolved. |
| `ELIGIBILITY_EVALUATED` | Artifact eligibility for the declared next stage was evaluated. |
| `ARTIFACT_RETAINED` | The artifact is retained under an applicable custody or cache boundary. |
| `ARTIFACT_REJECTED` | The artifact was rejected for the declared use. |
| `ARTIFACT_DIMENSION_CLOSED` | No further artifact transition is expected for the current operation attempt. |

**Nominal transition graph**

```text
ARTIFACT_UNREFERENCED
    → ARTIFACT_REFERENCED
    → ARTIFACT_LOCATING
    → TRANSFER_PENDING
    → TRANSFERRING
    → TRANSFER_OBSERVED
    → VERIFICATION_PENDING
    → VERIFYING
    → COMPATIBILITY_PENDING
    → ELIGIBILITY_EVALUATED
    → ARTIFACT_RETAINED
    → ARTIFACT_DIMENSION_CLOSED
```

`TRANSFER_OBSERVED` MUST NOT imply `VALID`.

`VERIFYING` completion MUST NOT imply compatibility or installation eligibility unless the declared verification contract specifically owns that determination.

### 9.6 Installer dimension

**Dimension identifier**

```text
BOOT_DIM_INSTALLER
```

**Purpose**

Represents installer availability, request handling, durable mutation, write verification, activation staging, and rollback.

**Phase vocabulary**

| Phase | Meaning |
|---|---|
| `INSTALLER_NOT_REQUIRED` | No installer operation is required for the scoped path. |
| `INSTALLER_UNRESOLVED` | Installer availability or eligibility is unresolved. |
| `INSTALLER_AVAILABILITY_EVALUATING` | Installer availability and capability are being evaluated. |
| `REQUEST_PREPARING` | BOOT is preparing a bounded installer request. |
| `REQUEST_PREPARED` | The installer request is complete but not yet delivered. |
| `DELIVERY_PENDING` | Request delivery awaits transport, admission, or provider availability. |
| `ACCEPTANCE_PENDING` | Installer acceptance is unresolved. |
| `INSTALLER_ACCEPTED` | The installer accepted the scoped request. |
| `INSTALLER_EXECUTING` | Installer-owned work is active. |
| `MUTATION_OBSERVED` | An installer report concerning durable mutation exists. |
| `WRITE_VERIFICATION_PENDING` | Installer-owned or provider-owned write verification remains pending. |
| `ACTIVATION_STAGED` | An identified installation generation is staged but not proven active or recovered. |
| `ROLLBACK_PENDING` | Rollback is required, requested, or awaiting evaluation. |
| `ROLLING_BACK` | Installer-owned rollback is active. |
| `FINAL_RESULT_AVAILABLE` | A scoped installer result exists. |
| `INSTALLER_DIMENSION_CLOSED` | No further installer transition is expected for the attempt. |

**Mutation-state vocabulary**

| State | Meaning |
|---|---|
| `MUTATION_NOT_APPLICABLE` | No durable mutation applies. |
| `MUTATION_NOT_ATTEMPTED` | No durable mutation was attempted. |
| `MUTATION_ATTEMPTED_NO_CHANGE_CONFIRMED` | Mutation was attempted, and the installer established that no durable change occurred. |
| `MUTATION_PARTIAL` | Some durable mutation occurred, but declared completion conditions were not met. |
| `MUTATION_COMPLETE` | The installer established that the declared durable mutation completed. |
| `MUTATION_UNKNOWN` | The implementation cannot establish whether or how much durable mutation occurred. |

**Rollback-state vocabulary**

| State | Meaning |
|---|---|
| `ROLLBACK_NOT_APPLICABLE` | Rollback does not apply. |
| `ROLLBACK_UNAVAILABLE` | No eligible rollback exists. |
| `ROLLBACK_AVAILABLE` | An eligible rollback target or mechanism exists. |
| `ROLLBACK_PREPARED` | Rollback prerequisites are prepared. |
| `ROLLBACK_ATTEMPTED` | Rollback began. |
| `ROLLBACK_COMPLETE` | The installer established that the scoped rollback completed. |
| `ROLLBACK_FAILED` | Rollback did not satisfy its completion conditions. |
| `ROLLBACK_UNKNOWN` | Rollback completion cannot be established. |

Installer process launch, process exit, progress output, file presence, and request delivery MUST NOT establish `MUTATION_COMPLETE`.

### 9.7 Recovery-verification dimension

**Dimension identifier**

```text
BOOT_DIM_RECOVERY
```

**Purpose**

Represents evaluation of one declared recovery objective.

**Phase vocabulary**

| Phase | Meaning |
|---|---|
| `RECOVERY_NOT_REQUIRED` | No recovery verification applies to the selected path. |
| `CRITERIA_PENDING` | Scoped recovery criteria remain undefined or incomplete. |
| `CRITERIA_ESTABLISHED` | Testable scoped recovery criteria exist. |
| `RECOVERY_VERIFICATION_PENDING` | Verification awaits prerequisites or provider work. |
| `RECOVERY_VERIFYING` | Verification is active. |
| `RECOVERY_RESULT_AVAILABLE` | A scoped recovery-verification result exists. |
| `RECOVERY_DIMENSION_CLOSED` | No further verification transition is expected for the attempt. |

**Recovery result vocabulary**

| Result | Meaning |
|---|---|
| `RECOVERY_NOT_EVALUATED` | No recovery result exists. |
| `RECOVERY_SATISFIED` | All declared recovery criteria for the scope were met. |
| `RECOVERY_SATISFIED_WITH_LIMITATIONS` | Minimum criteria were met with explicit unresolved limitations. |
| `RECOVERY_UNSATISFIED` | Declared recovery criteria were not met. |
| `RECOVERY_PARTIAL` | Some recovery criteria were met, but the scoped recovery is incomplete. |
| `RECOVERY_INDETERMINATE` | The criteria cannot be conclusively evaluated. |

Recovery verification does not establish normal-runtime readiness.

### 9.8 Reboot and handoff dimension

**Dimension identifier**

```text
BOOT_DIM_HANDOFF
```

**Purpose**

Represents eligibility, preparation, and observation of reboot or runtime control transfer.

**Phase vocabulary**

| Phase | Meaning |
|---|---|
| `HANDOFF_NOT_REQUIRED` | The current path does not require reboot or runtime handoff. |
| `HANDOFF_INELIGIBLE` | Known prerequisites are not satisfied. |
| `HANDOFF_ELIGIBILITY_PENDING` | Handoff prerequisites remain under evaluation. |
| `HANDOFF_ELIGIBLE` | Declared prerequisites for the scoped handoff are satisfied. |
| `HANDOFF_PREPARING` | Bounded handoff state and continuation data are being prepared. |
| `HANDOFF_PREPARED` | Handoff preparation completed. |
| `HANDOFF_ATTEMPTING` | Reboot or control transfer is being attempted. |
| `CONTROL_TRANSFER_OBSERVED` | The platform or target reported or demonstrated control transfer. |
| `RETURNED_UNEXPECTEDLY` | Control returned when continued transfer was expected. |
| `HANDOFF_RESULT_PENDING` | Final handoff evidence is not yet established. |
| `HANDOFF_DIMENSION_CLOSED` | No further handoff transition is expected for the attempt. |

`HANDOFF_ELIGIBLE` is not authorization unless the applicable authority decision is separately established.

`CONTROL_TRANSFER_OBSERVED` is not runtime readiness.

### 9.9 Runtime-observation dimension

**Dimension identifier**

```text
BOOT_DIM_RUNTIME_OBSERVATION
```

**Purpose**

Represents provider-attributed observations received after runtime handoff.

BOOT does not own runtime readiness.

**Phase vocabulary**

| Phase | Meaning |
|---|---|
| `RUNTIME_NOT_EXPECTED` | No runtime observation applies. |
| `AWAITING_RUNTIME_START` | BOOT or a continuation verifier awaits runtime evidence. |
| `RUNTIME_START_OBSERVED` | Runtime execution was observed to begin. |
| `RUNTIME_INITIALIZATION_REPORTED` | The runtime reported initialization progress. |
| `RUNTIME_READINESS_REPORTED` | The runtime reported its own readiness under an identified contract. |
| `RUNTIME_DEGRADED_REPORTED` | The runtime reported an explicitly degraded service state. |
| `RUNTIME_UNAVAILABLE_REPORTED` | The runtime reported or was established unavailable for the declared contract. |
| `RUNTIME_FAILURE_REPORTED` | A runtime-owned failure result was reported. |
| `RUNTIME_CONTACT_LOST` | Expected runtime communication or observation became unavailable. |
| `RUNTIME_OBSERVATION_CLOSED` | Runtime observation for the handoff attempt ended. |

BOOT MUST preserve the reporting provider and contract.

A runtime self-report does not create MEM authority, IMM authority, or retrospective BOOT success.

### 9.10 Dynamic-assembly dimension

This dimension is required when BOOT coordinates a dynamically assembled local generation.

**Dimension identifier**

```text
BOOT_DIM_ASSEMBLY
```

**Phase vocabulary**

| Phase | Meaning |
|---|---|
| `ASSEMBLY_NOT_REQUIRED` | No dynamic assembly applies. |
| `HARDWARE_EVIDENCE_PENDING` | Required substrate evidence remains incomplete. |
| `ASSEMBLY_PLAN_PENDING` | No complete assembly plan exists. |
| `ASSEMBLY_PLAN_EVALUATING` | Plan structure, compatibility, resources, and authority are under evaluation. |
| `COMPONENT_SELECTION_ACTIVE` | Required and optional components are being selected. |
| `BUILD_OR_SELECTION_ACTIVE` | Eligible local build or artifact selection is active. |
| `GENERATION_STAGED` | An identified assembled generation exists in staging. |
| `GENERATION_VALIDATION_PENDING` | Pair, component, and generation validation remains pending. |
| `GENERATION_ELIGIBILITY_EVALUATED` | Eligibility for installation or activation was evaluated. |
| `ASSEMBLY_DIMENSION_CLOSED` | No further assembly transition is expected for the attempt. |

Required and optional component state MUST remain distinguishable.

A failed required component prevents a favorable generation-eligibility result.

An unavailable optional component MAY produce `SATISFIED_WITH_LIMITATIONS` when all required conditions remain satisfied.

### 9.11 Update-preparation dimension

This dimension is required when BOOT participates in update staging, local validation, activation preparation, or rollback coordination.

**Dimension identifier**

```text
BOOT_DIM_UPDATE
```

**Phase vocabulary**

| Phase | Meaning |
|---|---|
| `UPDATE_NOT_REQUIRED` | No update operation applies. |
| `ADVERTISEMENT_OBSERVED` | An update or release advertisement was observed. |
| `RELEASE_EVIDENCE_PENDING` | Release identity or authority evidence remains unresolved. |
| `UPDATE_FETCH_PENDING` | Source or artifact acquisition awaits prerequisites. |
| `UPDATE_FETCH_OBSERVED` | A scoped fetch result exists. |
| `UPDATE_VERIFICATION_PENDING` | Integrity or release verification remains pending. |
| `UPDATE_COMPATIBILITY_PENDING` | Local compatibility remains unresolved. |
| `UPDATE_STAGING_PENDING` | Local staging has not completed. |
| `LOCAL_VALIDATION_PENDING` | Required local tests remain incomplete. |
| `UPDATE_ACTIVATION_PENDING` | Activation is awaiting authority or prerequisites. |
| `POST_ACTIVATION_VERIFICATION_PENDING` | Post-activation verification remains pending. |
| `UPDATE_ROLLBACK_PENDING` | Rollback is required or under consideration. |
| `UPDATE_DIMENSION_CLOSED` | No further update transition is expected for the attempt. |

An advertisement is not acceptance.

Fetching is not verification.

Verification is not compatibility.

Compatibility is not activation authority.

Peer transfer does not create release authority.

## 10. Dimension applicability

### 10.1 Explicit non-applicability

A dimension or facet that does not apply MUST use an explicit non-applicable value.

It MUST NOT be omitted in a way that could be confused with unknown or unavailable state.

### 10.2 Conditional dimensions

The assembly and update dimensions are conditionally required.

An implementation that does not support those operations MAY report them as not applicable.

It MUST NOT claim conformance for an unsupported operation family merely because the dimension is absent.

### 10.3 Multiple instances

There MAY be multiple simultaneous instances of:

- artifact dimensions;
- authority dimensions;
- installer operations;
- assembly plans;
- update proposals;
- assistance sessions.

Each instance MUST have an independent subject identity and revision.

A collection summary MUST NOT erase per-instance failures or conflicts.

## 11. Derived boot dispositions

A boot disposition is a derived candidate next path.

It is not a replacement for the underlying state vector.

### 11.1 Disposition vocabulary

| Disposition | Meaning |
|---|---|
| `DISPOSITION_UNRESOLVED` | Available state is insufficient to select a candidate path. |
| `NORMAL_HANDOFF_CANDIDATE` | Known prerequisites support consideration of an ordinary runtime handoff. |
| `RESTRICTED_HANDOFF_CANDIDATE` | A runtime handoff may be considered only with explicit declared limitations. |
| `RESCUE_REQUIRED` | Normal or restricted handoff prerequisites are not met, and a rescue path is required. |
| `RECOVERY_IN_PROGRESS` | An authorized recovery sequence is active. |
| `REBOOT_OR_HANDOFF_PENDING` | Applicable recovery or installation work completed sufficiently to prepare the next transition, but control transfer has not completed. |
| `OPERATOR_REVIEW_REQUIRED` | Progress requires a governed operator or external authority decision. |
| `DISPOSITION_DEFERRED` | Progression is intentionally postponed under current conditions. |
| `REFUSAL_REQUIRED` | BOOT cannot permit the requested next path under the established state. |
| `DISPOSITION_LOCKED_DOWN` | An authorized protective condition prohibits progression beyond a declared scope. |
| `SESSION_COMPLETE_NO_HANDOFF` | The BOOT session completed without a reboot or runtime handoff, such as an explicit test or diagnostic mode. |

### 11.2 Dispositions are derived

A disposition MUST identify:

- the state snapshot used;
- the revision vector used;
- the derivation rule or policy reference;
- unresolved limitations;
- prohibited next actions;
- required next conditions.

A disposition computed from stale dimension revisions MUST be rejected or marked stale.

### 11.3 Dispositions do not grant authority

Selecting:

- a normal candidate;
- a rescue path;
- an artifact;
- an installer;
- a generation;
- an update;
- a handoff target

does not authorize its use.

### 11.4 Minimum normal-handoff restrictions

An implementation MUST NOT derive `NORMAL_HANDOFF_CANDIDATE` when any required condition is:

- unknown;
- partial beyond the declared normal profile;
- unavailable;
- invalid;
- stale beyond its permitted boundary;
- expired;
- conflicting;
- failed;
- indeterminate;
- resource exhausted;
- locked down;
- unsupported.

The detailed eligibility contract belongs to later specifications and deployment policy.

### 11.5 Restricted handoff

`RESTRICTED_HANDOFF_CANDIDATE` MAY be derived when:

- every required minimum condition is established;
- limitations are explicit;
- unavailable capabilities are optional for the declared profile;
- the runtime can represent its own degraded state;
- applicable authority permits the restricted profile.

It MUST NOT be used to hide required-component failure.

### 11.6 Required and optional component behavior

If a required component is:

- unavailable;
- invalid;
- incompatible;
- failed;
- indeterminate;
- unsupported for the required profile,

the affected assembly MUST NOT be represented as eligible.

If an optional component is unavailable or unsupported:

- the limitation MUST remain explicit;
- the base assembly MAY remain eligible;
- the resulting disposition MAY be restricted or degraded;
- unavailable optional capability MUST NOT be reported as present.

### 11.7 Lock-down precedence

A scoped lock-down may constrain the derived disposition.

It does not erase:

- verified artifacts;
- completed installer work;
- unresolved authority;
- recovery evidence;
- runtime observations.

The underlying states remain independently visible.

## 12. Authoritative transition model

### 12.1 Transition request

A conceptual transition request contains:

```text
BootStateTransitionRequest
    transition_identity
    session_identity
    subject_identity
    dimension_identity
    expected_prior_revision
    proposed_phase
    proposed_facets
    proposed_outcome
    operation_identity
    attempt_identity
    reason_code
    reporting_provider
    evidence_references
    authority_reference
    bounded_issues
    monotonic_sequence
    optional_wall_time
    idempotency_key
```

This is not a required binary format.

### 12.2 Transition result

A conceptual transition result contains:

```text
BootStateTransitionResult
    transition_identity
    disposition
    prior_revision
    resulting_revision
    authoritative_change
    duplicate_of_transition
    conflict_revision
    outcome_state
    bounded_issues
    reconciliation_required
```

### 12.3 Expected-revision rule

An authoritative transition MUST identify the expected prior revision.

If the authoritative revision differs:

- the transition MUST NOT commit;
- the result MUST report revision conflict;
- the newer state MUST remain unchanged;
- the caller MAY re-evaluate using a fresh snapshot.

### 12.4 Revision advancement

A successful authoritative transition MUST:

- increment or replace the applicable revision exactly once;
- identify the prior revision;
- identify the resulting revision;
- preserve the transition identity;
- preserve the operation and attempt identity.

A rejected, duplicate, or conflicting transition MUST NOT silently advance the authoritative revision.

### 12.5 No-op and duplicate behavior

A semantically identical repeated transition with the same idempotency identity SHOULD return the previously established result.

It MUST NOT:

- repeat mutation;
- create a second authority effect;
- count as independent evidence;
- advance the revision merely because it was delivered again.

### 12.6 Explicit backward movement

A dimension phase MAY move to an earlier phase only through an explicit transition.

Valid reasons may include:

- new evidence;
- revocation;
- expiry;
- provider correction;
- failed verification;
- rollback;
- supersession;
- required reinspection;
- reconciliation after restart.

The transition MUST preserve why the earlier phase became applicable again.

### 12.7 No implicit cross-dimension cascade

A transition in one dimension MUST NOT silently mutate another dimension.

Examples:

- authority revocation does not delete a verified artifact;
- artifact rejection does not prove the assistance source malicious;
- installer failure does not make identity invalid;
- runtime failure does not rewrite a successful transfer result;
- lock-down does not automatically mark every operation failed.

Dependent dimensions MAY receive separate explicit transitions.

### 12.8 Multi-dimension transition bundles

An operation MAY require an all-or-none transition across several dimensions.

A conceptual bundle contains:

```text
BootStateTransitionBundle
    bundle_identity
    expected_revision_vector
    proposed_dimension_changes
    operation_identity
    bounded_issues
    result_preparation
```

Before commit, the implementation MUST:

1. validate every expected revision;
2. validate every proposed transition;
3. construct every required result;
4. retain every required issue;
5. complete all fallible preparation.

The bundle then commits all authoritative dimension changes or none.

### 12.9 Terminal attempt behavior

A final outcome is terminal for one operation attempt.

A retry MUST either:

- retain the same semantic operation identity with a new attempt identity; or
- create an explicitly new operation identity.

It MUST NOT silently reopen a completed attempt.

## 13. Conservative atomicity

BOOT follows the binding rule:

> Complete all fallible construction, validation, bounded retention, and result preparation before committing authoritative state.

Before authoritative commit, the implementation MUST complete all applicable:

- identifier validation;
- subject validation;
- scope validation;
- phase-transition validation;
- revision validation;
- authority evaluation;
- evidence-reference validation;
- checked arithmetic;
- bounded allocation;
- bounded issue retention;
- failure-record preparation;
- result-code preparation;
- idempotency preparation;
- continuation preparation required by the transition;
- diagnostic preparation required by the public contract.

If any required preparation fails:

- authoritative state remains unchanged;
- no revision advances;
- no favorable outcome is reported;
- no partial disposition is committed;
- the failure is reported conservatively;
- unresolved mutation remains explicit.

### 13.1 Required issue retention

Issues required to justify:

- refusal;
- invalidity;
- partial mutation;
- lock-down;
- recovery failure;
- reconciliation;
- a restricted disposition

MUST be retained before the corresponding authoritative state commits.

If required issue retention fails, the transition MUST NOT commit.

### 13.2 Optional diagnostics

Optional human-readable diagnostics MAY fail after a valid authoritative commit only when:

- they are not required for public result interpretation;
- they are not required for reconciliation;
- they are not required for audit;
- the authoritative machine-readable result is already complete.

### 13.3 No false success after mutation uncertainty

If durable mutation may have occurred but the result cannot be represented or retained, BOOT MUST NOT report success or no change.

The result MUST identify:

- mutation possible or unknown;
- authoritative BOOT state unchanged where applicable;
- reconciliation required.

## 14. Concurrency and ownership

### 14.1 Single-dimension serialization

Concurrent authoritative updates to the same dimension instance MUST use revision or equivalent serialization.

A stale writer MUST NOT overwrite a newer revision.

### 14.2 Independent dimension concurrency

Independent dimensions MAY advance concurrently when:

- their subjects are distinct;
- no shared invariant requires atomic coordination;
- each transition uses its own expected revision;
- the resulting summary is recomputed from a coherent revision vector.

### 14.3 Provider ownership

A provider MAY publish its own result.

BOOT MUST translate that result into BOOT state through an explicit transition.

The provider does not gain ownership of unrelated BOOT dimensions.

### 14.4 Summary concurrency

A derived disposition MUST identify the exact dimension revisions from which it was produced.

If any required dimension changes before the disposition is used:

- the disposition MUST be recomputed or rejected as stale.

### 14.5 Locking mechanism deferred

This specification does not require:

- mutexes;
- lock-free structures;
- transactions;
- journals;
- database compare-and-swap;
- one event loop.

The mechanism must satisfy the observable revision and atomicity rules.

## 15. Failure model

### 15.1 Failure is operation-scoped

A failure describes one operation, attempt, provider interaction, transition, or verification scope.

Failure MUST NOT automatically establish:

- target damage;
- malicious intent;
- compromise;
- permanent invalidity;
- unrecoverability;
- unrelated provider failure.

### 15.2 Failure record

A conceptual failure record contains:

```text
BootFailureRecord
    failure_identity
    failure_domain
    failure_code
    subject_identity
    dimension_identity
    phase
    operation_identity
    attempt_identity
    provider_identity
    observed_condition
    known_cause_state
    authoritative_state_changed
    durable_mutation_state
    rollback_state
    retry_class
    reconciliation_required
    evidence_references
    bounded_issues
    monotonic_sequence
    optional_wall_time
```

### 15.3 Canonical failure domains

| Domain | Meaning |
|---|---|
| `CONTRACT_FAILURE` | Input or result violated a declared structural or semantic contract. |
| `BOUNDS_FAILURE` | Input, depth, count, size, or retained work exceeded an applicable limit. |
| `RESOURCE_FAILURE` | Required bounded resources were insufficient or exhausted. |
| `IDENTITY_FAILURE` | Required identity evidence could not satisfy its owning contract. |
| `SECURITY_PROVIDER_FAILURE` | A cryptographic or security-provider operation failed or was unavailable. |
| `AUTHORITY_FAILURE` | Required authority was denied, expired, revoked, conflicting, unavailable, or indeterminate. |
| `ADMISSION_FAILURE` | Applicable admission rejected or deferred the operation. |
| `PROVIDER_UNAVAILABLE_FAILURE` | A required provider was unavailable. |
| `TRANSPORT_FAILURE` | Byte movement or transport-session behavior failed. |
| `ASSISTANCE_FAILURE` | Assistance negotiation or session work failed independently of transport. |
| `ARTIFACT_FAILURE` | Artifact location, transfer interpretation, verification, or eligibility failed. |
| `COMPATIBILITY_FAILURE` | The subject did not satisfy applicable target-compatibility requirements. |
| `INSTALLER_FAILURE` | Installer request, execution, verification, activation staging, or reporting failed. |
| `PARTIAL_MUTATION_FAILURE` | Durable mutation occurred without full installer completion. |
| `ROLLBACK_FAILURE` | Rollback did not satisfy its completion contract. |
| `RECOVERY_VERIFICATION_FAILURE` | Declared recovery criteria were not met or could not be established. |
| `HANDOFF_FAILURE` | Reboot or control transfer did not satisfy its declared handoff conditions. |
| `RUNTIME_OBSERVATION_FAILURE` | Runtime-owned initialization, readiness, or service behavior failed. |
| `PERSISTENCE_FAILURE` | Required BOOT continuation or state persistence failed. |
| `REVISION_CONFLICT_FAILURE` | An expected revision did not match authoritative state. |
| `IDEMPOTENCY_FAILURE` | Duplicate, prior completion, or operation continuity could not be safely resolved. |
| `TIMEOUT_FAILURE` | A declared timeout boundary was observed without required completion evidence. |
| `CANCELLATION_RESULT` | Governed cancellation ended the attempt. |
| `INTERRUPTION_FAILURE` | Work ended unexpectedly before completion was established. |
| `UNSUPPORTED_FAILURE` | The requested operation, version, platform, or mechanism is unsupported. |
| `INVARIANT_VIOLATION_FAILURE` | The implementation detected behavior that would violate BOOT-0000, BOOT-0001, or this specification. |
| `INDETERMINATE_FAILURE` | The failure domain or effect cannot be established more precisely. |

### 15.4 Known-cause state

A failure MUST distinguish:

- cause established;
- cause partially established;
- cause conflicting;
- cause unknown.

An implementation MUST NOT invent a root cause to avoid reporting unknown.

### 15.5 Failure effect

A failure record MUST identify whether:

- no authoritative state changed;
- authoritative BOOT state changed;
- durable mutation did not occur;
- durable mutation occurred;
- durable mutation may have occurred;
- durable mutation is unknown;
- rollback is required;
- rollback is unavailable;
- reconciliation is required.

### 15.6 Failure severity policy is not public state truth

A deployment MAY classify urgency or response priority.

Private thresholds or scoring MUST NOT alter the public failure meaning.

The public contract MUST remain usable without private scoring.

## 16. Canonical conservative conditions

### 16.1 Unknown

`UNKNOWN` means evidence is insufficient.

Unknown is not:

- absent;
- available;
- unavailable;
- valid;
- invalid;
- current;
- stale;
- authenticated;
- authorized;
- admitted;
- compatible;
- complete;
- failed;
- recovered.

### 16.2 Unavailable

`UNAVAILABLE` means a required subject or provider is known to be inaccessible for the declared operation.

Unavailable does not prove invalidity or maliciousness.

### 16.3 Invalid

`INVALID` means an identified validation contract was not satisfied.

Invalid does not automatically mean:

- compromised;
- malicious;
- permanently unusable;
- unsupported;
- unavailable.

### 16.4 Deferred

`DEFERRED` means progression is postponed under current conditions.

A deferred result SHOULD identify a reconsideration condition where publicly available.

Deferral is not denial or success.

### 16.5 Failed

`FAILED` means declared completion conditions were not met.

Failure does not automatically identify root cause.

### 16.6 Locked down

`LOCKED_DOWN` means progression beyond a declared boundary is prohibited through applicable authority or protective policy.

It does not establish guilt or compromise.

### 16.7 Partial

`PARTIAL` means some declared effects occurred while full completion conditions were not met.

A partial result MUST identify:

- completed scope;
- incomplete scope;
- possible durable effects;
- rollback state;
- prohibited next actions.

### 16.8 Indeterminate

`INDETERMINATE` means completion, effect, validity, or causation cannot be established.

Indeterminate state requires conservative progression and MAY require operator review or reconciliation.

### 16.9 Unsupported

`UNSUPPORTED` means the implementation or provider does not support the required subject, operation, version, architecture, or mechanism.

Unsupported is not invalid and not absent.

### 16.10 Timed out

`TIMED_OUT` means an applicable timeout boundary elapsed without the required completion evidence.

Timeout does not independently prove:

- remote failure;
- local failure;
- target compromise;
- no mutation;
- operation cancellation.

## 17. Resource-exhaustion behavior

Resource exhaustion is a first-class state and outcome.

Potential exhausted resources include:

- heap;
- stack;
- static record pool;
- result capacity;
- issue capacity;
- identifier capacity;
- parser depth;
- artifact count;
- plan depth;
- diagnostic retention;
- storage;
- temporary working space;
- transport capacity;
- provider capacity;
- installer capacity;
- retry budget;
- execution time budget.

A conforming implementation MUST:

1. detect the applicable bound;
2. stop before unbounded growth or arithmetic overflow;
3. prepare an explicit `RESOURCE_EXHAUSTED` result;
4. leave authoritative state unchanged unless a separately valid prior transition already committed;
5. preserve prior revisions;
6. preserve mutation uncertainty;
7. avoid silent truncation of required state or issues.

Resource exhaustion in one dimension MUST NOT automatically mark every dimension failed.

## 18. Timeout behavior

### 18.1 Timeouts are observations

A timeout is an observed failure to receive required completion evidence within a declared boundary.

It is not proof that the operation did not complete.

### 18.2 Timeout record

A timeout result SHOULD identify:

- operation;
- attempt;
- stage;
- timeout basis;
- elapsed monotonic interval where available;
- expected evidence;
- last established state;
- mutation possibility;
- retry or reconciliation requirement.

### 18.3 Wall-clock independence

BOOT MUST remain able to represent timeout and sequencing without synchronized wall-clock time.

A monotonic clock, bounded loop count, watchdog event, platform timer, or another implementation mechanism MAY be used.

### 18.4 Provider timeouts

A provider timeout MUST remain attributed to that provider interaction.

It MUST NOT automatically establish target invalidity.

## 19. Cancellation and interruption

### 19.1 Cancellation

Cancellation is a governed operation result.

A cancellation request does not establish that cancellation completed.

The implementation MUST distinguish:

- cancellation requested;
- cancellation accepted;
- cancellation active;
- cancellation completed;
- cancellation rejected;
- cancellation unavailable;
- cancellation outcome unknown.

Detailed cancellation contracts remain outside this specification.

### 19.2 Interruption

Interruption may result from:

- process termination;
- power loss;
- reboot;
- kernel failure;
- transport loss;
- provider failure;
- resource loss;
- manual reset;
- unknown cause.

Interruption MUST preserve possible durable effects.

### 19.3 Interrupted installer work

Interrupted installer work MUST NOT be represented as:

- no mutation;
- complete mutation;
- successful rollback

unless the installer or eligible recovery evidence establishes that result.

## 20. Revision, idempotency, and retry

### 20.1 Semantic operation identity

A semantic operation retains identity independently of:

- process;
- thread;
- transport connection;
- secure session;
- assistance session reconnect;
- installer process instance;
- reboot.

### 20.2 Attempt identity

Each distinct execution attempt MUST have its own attempt identity.

A retry MAY retain the semantic operation identity while changing the attempt identity.

### 20.3 Duplicate delivery

Duplicate delivery MUST NOT:

- create a new operation;
- repeat durable mutation;
- create new authority;
- count as independent evidence;
- advance state revisions without a material state change.

### 20.4 Unknown prior completion

If prior completion is unknown, a non-idempotent operation MUST NOT be repeated until:

- reconciliation establishes the prior effect;
- a rollback or replacement operation makes repetition safe;
- applicable authority explicitly permits a defined recovery action.

### 20.5 Retry classification

A failure MAY be classified as:

- no retry permitted;
- retry permitted without state change;
- retry permitted after refreshed evidence;
- retry permitted after authority renewal;
- retry permitted after resource recovery;
- retry permitted after reconciliation;
- retry requires a new operation identity;
- operator review required.

Exact retry policy belongs to BOOT-0008.

### 20.6 Retry exhaustion

Retry exhaustion MUST produce an explicit final outcome.

It MUST NOT restart the retry counter silently.

## 21. Persistence and restored state

### 21.1 Persistence is not truth

Persisting a BOOT record does not make its claim true.

Recovering a BOOT record does not automatically restore:

- identity validity;
- authority;
- freshness;
- applicability;
- completion;
- admission;
- runtime readiness.

### 21.2 Restored snapshot handling

A restored snapshot MUST be evaluated for:

- structural validity;
- schema or version support;
- provenance;
- revision continuity;
- subject continuity;
- integrity;
- freshness;
- supersession;
- conflict with newer state.

### 21.3 Missing persistence

Absence of a required continuation record MUST NOT be interpreted as a clean initial state when prior mutation may have occurred.

The applicable dimension MUST become:

- unknown;
- indeterminate;
- reconciliation-required;
- or another conservative state defined by the owning contract.

### 21.4 Corrupt persistence

Corrupt persisted BOOT state MUST be reported as invalid or indeterminate.

It MUST NOT be silently discarded and replaced with favorable defaults.

### 21.5 Minimal persistence adapter

A minimal BOOT implementation MAY persist only bounded BOOT-required state.

Such persistence remains distinct from MEM semantic authority.

## 22. Reboot and process-restart continuation

### 22.1 Boot attempt is not BOOT session

A platform boot attempt and a BOOT session are separate identities.

One BOOT session MAY span:

- one process restart;
- one rescue-environment restart;
- one platform reboot;
- one post-installation verification boot

when a valid continuation contract exists.

### 22.2 Continuation record

A conceptual continuation record contains:

```text
BootContinuationRecord
    prior_session_identity
    prior_boot_attempt_identity
    next_boot_attempt_expectation
    operation_identities
    last_revision_vector
    selected_generation_identity
    installation_result_reference
    activation_reference
    rollback_reference
    unresolved_mutation_state
    expected_next_evidence
    authority_revalidation_requirement
    bounded_issues
    integrity_reference
```

### 22.3 Continuation preparation

Required continuation state MUST be fully prepared before BOOT commits a handoff state that depends on it.

If preparation fails:

- handoff MUST NOT be reported as fully prepared;
- authoritative pre-handoff state remains unchanged;
- failure remains explicit.

### 22.4 New process behavior

A new process MUST NOT:

- assume prior work succeeded;
- reset revisions;
- discard unresolved mutation;
- restore expired authority;
- create a new semantic operation for an unresolved retry;
- report recovery because the process restarted.

### 22.5 Post-reboot verification

Post-reboot evidence MAY support:

- activation observation;
- selected-generation observation;
- runtime-start observation;
- recovery verification;
- rollback verification.

Each later stage retains its own decision.

## 23. Distribution image, installed system, and runtime state

The following subjects MUST remain distinct:

1. Node distribution image.
2. Installed Node operating-system generation.
3. Dynamically assembled runtime generation.
4. Active runtime instance.

State for one MUST NOT silently become state for another.

### 23.1 Distribution-image state

Successful boot from a distribution image establishes only the scoped boot observation.

It does not establish:

- image authenticity;
- release authority;
- compatibility;
- installation eligibility;
- installed-system state;
- runtime readiness.

### 23.2 Installed-generation state

An installed generation may be:

- staged;
- write-verified;
- activation-eligible;
- active;
- fallback;
- deprecated;
- revoked;
- rollback target;
- partially installed;
- indeterminate.

Detailed generation architecture belongs to future OS specifications.

### 23.3 Runtime-generation state

A runtime generation is assembled from eligible installed material and current policy.

Its state remains separate from installed-system state.

### 23.4 Active runtime instance

A running runtime instance may report:

- initializing;
- ready under an identified contract;
- degraded;
- unavailable;
- failed.

Its result does not rewrite the installation or BOOT result.

## 24. Required and optional component state

### 24.1 Required component

A required component is necessary for the declared assembly or runtime profile.

If its required state is:

- unknown;
- unavailable;
- invalid;
- incompatible;
- failed;
- indeterminate;
- unsupported,

the affected generation MUST NOT be reported as eligible.

### 24.2 Optional component

An optional component may be omitted.

Its absence or failure MUST be represented explicitly.

It MAY produce:

- a restricted handoff candidate;
- a degraded runtime profile;
- an unavailable capability;
- an unsupported capability.

### 24.3 Capability summaries

A capability summary MUST identify:

- required capabilities established;
- required capabilities unresolved or failed;
- optional capabilities available;
- optional capabilities unavailable;
- limitations affecting handoff or operation.

A generic `ALL_CAPABILITIES_OK` value is insufficient.

## 25. Update and rollback state rules

### 25.1 Update stages remain distinct

The following remain independently represented:

- advertisement observed;
- release identity established;
- release authority established;
- fetch result;
- integrity verification;
- compatibility result;
- local staging;
- local validation;
- activation authority;
- activation result;
- post-activation verification;
- rollback eligibility;
- rollback result.

### 25.2 Repository branch state

Repository branch position MUST NOT be represented as release activation state.

The tip of a branch is not automatically:

- accepted;
- verified;
- compatible;
- eligible;
- staged;
- active.

### 25.3 Peer-transfer state

A peer-transfer result identifies byte-transfer progress or completion.

It does not establish:

- release authority;
- artifact integrity;
- local compatibility;
- activation permission.

### 25.4 Canary and staged propagation

A failed validation stage MUST remain scoped to its stage and subjects.

It MUST block broader promotion when the rollout contract requires that gate.

It MUST NOT automatically mark every existing active generation failed.

### 25.5 Revoked and superseded state

Revoked, superseded, deprecated, unsupported, and merely older material MUST remain distinguishable.

An older validated fallback MAY remain eligible when not revoked and when its complete generation remains compatible.

### 25.6 Complete-generation rollback

Rollback state MUST identify the complete generation boundary.

A kernel-only rollback MUST NOT be represented as a complete system rollback unless the applicable pair contract explicitly permits it.

## 26. Cross-architecture state translation

### 26.1 ACS

ACS remains authoritative for:

- participant and endpoint identity;
- relationships;
- admission;
- connection lifecycle;
- applicable authority and capability references;
- revisions and idempotency semantics.

BOOT MAY translate ACS results into BOOT state.

It MUST preserve the original ACS result and scope.

### 26.2 Security provider

The security provider owns:

- authentication;
- signature verification;
- digest verification;
- credential state;
- protected key custody;
- secure-session establishment.

BOOT state MUST identify the provider and scope.

### 26.3 Installer

Installer state is provider-owned evidence concerning durable mutation, verification, staging, and rollback.

BOOT coordinates and records; it does not perform or redefine installer mutation.

### 26.4 Transport

Transport state concerns byte movement and link behavior.

BOOT operation state remains separate.

### 26.5 IMM

IMM may provide evidence or request recovery.

An IMM request does not directly transition authority, installer, activation, or recovery state to a favorable value.

IMM-owned restoration results remain IMM-owned.

### 26.6 MEM

MEM governs persistence, custody, retention, reconstruction, restoration, and deletion.

BOOT persistence state does not create MEM truth.

### 26.7 Resource management

Resource-management results inform BOOT resource facets.

BOOT MUST NOT enlarge allocations or exceed ceilings through a state transition.

### 26.8 Normal runtime

Runtime reports inform the runtime-observation dimension.

The runtime owns its readiness and degraded-operation contracts.

## 27. Public conceptual records

These records define public semantics, not required encodings.

### 27.1 State snapshot

```text
BootStateSnapshot
    snapshot_identity
    session_identity
    boot_attempt_identity
    snapshot_revision
    dimension_records
    derived_disposition
    disposition_basis_revision_vector
    unresolved_required_conditions
    bounded_issues
    monotonic_sequence
    optional_wall_time
```

### 27.2 Dimension record

```text
BootDimensionState
    subject_identity
    dimension_identity
    phase
    facet_values
    dimension_specific_values
    outcome
    revision
    operation_identity
    attempt_identity
    provider_identity
    evidence_references
    issue_references
    transition_identity
```

### 27.3 Issue record

```text
BootStateIssue
    issue_code
    subject_identity
    dimension_identity
    operation_identity
    effect
    requirement_reference
    evidence_references
    public_detail
```

An issue effect may include:

- advisory;
- declared limitation;
- blocks current progression;
- requires reconciliation;
- requires review;
- supports restriction;
- supports lock-down.

Private scores and thresholds are not public issue fields.

### 27.4 Disposition snapshot

```text
BootDispositionSnapshot
    disposition
    snapshot_identity
    basis_revision_vector
    required_conditions_established
    unresolved_conditions
    limitations
    prohibited_next_actions
    required_next_actions
    derivation_reference
```

### 27.5 Mutation summary

```text
BootMutationSummary
    installer_operation_identity
    target_identity
    mutation_state
    write_verification_state
    activation_stage_state
    rollback_state
    unresolved_effects
    reconciliation_required
```

## 28. Bounds and deterministic retention

Every state and failure record MUST be bounded.

Implementations MUST define finite limits for:

- dimensions per snapshot;
- instances per dimension;
- evidence references;
- issue references;
- diagnostic fields;
- identifiers;
- textual public detail;
- revision vectors;
- continuation operations;
- update candidates;
- artifacts;
- component records.

### 28.1 Deterministic issue ordering

A bounded issue list SHOULD use deterministic ordering such as:

1. dimension identity;
2. subject identity;
3. operation identity;
4. stable issue code;
5. evidence reference.

Private severity scoring MUST NOT determine public semantic order.

### 28.2 Truncation

Required issues MUST NOT be silently truncated.

If optional advisory information is omitted under an explicitly permitted bound:

- the omission MUST be represented;
- required state interpretation MUST remain possible;
- no favorable outcome may depend on the omitted information.

### 28.3 Checked arithmetic

All calculations involving:

- counts;
- capacities;
- offsets;
- lengths;
- revisions;
- sequence values;
- elapsed bounds;
- allocation size

MUST use checked arithmetic.

## 29. State-summary rules

### 29.1 Summary is derived

A summary is a convenience for:

- policy evaluation;
- diagnostics;
- operator presentation;
- test acceptance.

It is not the source of truth.

### 29.2 Summary carries revision vector

A summary MUST identify all dimension revisions material to its result.

### 29.3 No favorable default

Missing required dimension state MUST result in:

- unresolved;
- partial;
- deferred;
- review-required;
- refusal;
- lock-down;
- another conservative outcome.

It MUST NOT produce a favorable default.

### 29.4 No cross-stage success

A summary MUST NOT report:

- artifact success from transfer success;
- installation success from installer launch;
- activation success from installation;
- recovery success from activation;
- runtime success from handoff.

### 29.5 Provider attribution

Provider-specific terms such as:

- authenticated;
- signature verified;
- installer-complete;
- runtime-ready

MUST remain provider-attributed.

## 30. Conformance expectations

### 30.1 Architectural conformance

An architecture conforms when it:

- preserves independent dimensions;
- uses orthogonal facets;
- defines scoped state subjects;
- preserves revisions;
- preserves failure truthfulness;
- enforces conservative atomicity;
- avoids implicit cross-dimension transitions;
- represents resource exhaustion;
- preserves reboot uncertainty;
- derives summaries from revisioned state vectors.

### 30.2 Implementation conformance

An implementation conforms when observable behavior demonstrates those properties under:

- ordinary operation;
- malformed input;
- provider failure;
- resource exhaustion;
- timeout;
- cancellation;
- interruption;
- duplicate delivery;
- stale revision;
- partial mutation;
- reboot;
- rollback failure;
- runtime startup failure.

### 30.3 Required test families

Conformance evidence SHOULD include:

- dimension-independence tests;
- unknown-state tests;
- availability-versus-validity tests;
- progress-versus-outcome tests;
- summary-staleness tests;
- revision-conflict tests;
- duplicate-transition tests;
- multi-dimension atomicity tests;
- allocation-failure tests;
- issue-retention-failure tests;
- parser-bound tests;
- timeout tests;
- cancellation tests;
- interruption tests;
- unknown-prior-mutation tests;
- reboot-continuation tests;
- corrupt-persistence tests;
- missing-persistence tests;
- installer partial-write tests;
- rollback-failure tests;
- artifact transfer-versus-verification tests;
- required-versus-optional component tests;
- update advertisement-versus-activation tests;
- peer-transfer refusal tests;
- runtime-handoff-versus-readiness tests;
- lock-down preservation tests.

### 30.4 BOOT-P0 conformance

BOOT-P0 need not implement every dimension.

It MUST implement enough of the model to represent:

- BOOT session;
- local boot;
- handoff or test completion;
- runtime observation;
- resource state;
- timeout;
- structured failure;
- final derived disposition.

BOOT-P0 MUST distinguish at least:

```text
image boot observed
/init execution observed
seed launch observed
heartbeat evidence observed
seed completion observed
final test disposition
```

No earlier observation proves a later observation.

### 30.5 BOOT-I001 conformance

BOOT-I001 should provide language-neutral values for:

- bounded identities;
- dimensions;
- facets;
- revisions;
- transition requests and results;
- bounded issues;
- state snapshots;
- failure records;
- no-partial-commit behavior.

### 30.6 Deployment policy

Deployment policy MAY determine:

- which dimensions are required for a profile;
- which optional limitations permit restricted handoff;
- which authority sources are eligible;
- retry limits;
- timeout values;
- operator-review conditions.

Policy MUST NOT redefine public state meanings.

## 31. Relationship to later BOOT specifications

### BOOT-0003 — Identity, Trust, and Authority Boundary

BOOT-0003 refines:

- authority subjects;
- identity states;
- authentication evidence;
- revalidation;
- authority decisions;
- expiry;
- revocation;
- conflict.

It MUST map its results into the facets and authority dimension defined here.

### BOOT-0004 — Discovery and Assistance Negotiation

BOOT-0004 refines:

- assistance phases;
- offer states;
- candidate eligibility;
- session continuation;
- transport-neutral operations.

It MUST preserve assistance and transport separation.

### BOOT-0005 — Recovery Plans and Artifact Boundaries

BOOT-0005 refines:

- plan states;
- artifact states;
- installer-input states;
- mutation results;
- verification and rollback state.

It MUST preserve this specification’s phase, facet, and outcome distinctions.

### BOOT-0006 — Minimal Runtime and Language Constraints

BOOT-0006 defines measurable:

- state-record bounds;
- issue limits;
- parser limits;
- resource budgets;
- persistence limits;
- stack and heap behavior;
- language evidence.

### BOOT-0007 — ACS, IMM, and MEM Integration

BOOT-0007 defines detailed provider-state mappings.

It MUST preserve provider ownership and attribution.

### BOOT-0008 — Failure, Retry, and Operator Intervention

BOOT-0008 refines:

- retry classes;
- cancellation;
- interruption;
- reconciliation;
- lock-down transitions;
- continuation;
- operator review.

It MUST preserve the canonical failure model defined here.

### BOOT-0009 — Public/Private Boundary and Implementation Roadmap

BOOT-0009 refines:

- public conformance levels;
- publication rules;
- implementation checkpoints;
- private policy boundaries;
- repository re-track requirements.

Future OS and update specifications own complete generation, filesystem, release, propagation, activation, and rollback mechanisms.

## 32. Public/private classification boundary

### 32.1 Public state material

Public BOOT architecture MAY define:

- dimension names;
- phase vocabularies;
- facet meanings;
- public failure domains;
- public issue codes;
- revision semantics;
- transition rules;
- public conceptual records;
- conformance evidence;
- provider attribution;
- explicit unresolved conditions.

### 32.2 Excluded private material

Public state or failure records MUST NOT disclose:

- credentials;
- private keys;
- recovery secrets;
- production trust anchors;
- infrastructure addresses;
- private repository locations;
- private authority policy;
- responder-selection algorithms;
- protected module source;
- private topology;
- private thresholds;
- hidden scores;
- internal reasoning;
- operator playbooks;
- production rollout policy.

### 32.3 Public operation without private modules

The public BOOT state model MUST remain functional without private modules.

Unavailable private capability MUST be represented as:

- not applicable;
- unavailable;
- unsupported;
- restricted;
- another truthful scoped value.

It MUST NOT be represented as present or successful.

## 33. Security and privacy requirements

State and failure records MUST:

- avoid raw credentials;
- avoid private-key material;
- avoid reusable secrets;
- minimize pre-authentication disclosure;
- preserve identity and authority references rather than secret contents;
- bound diagnostic text;
- preserve classification;
- prevent unauthorized peer propagation of protected state.

A detailed public record MAY identify that a private policy or protected condition affected a decision without disclosing the mechanism.

## 34. Open architectural decisions

The following remain unresolved:

- exact numeric values for dimensions, facets, phases, and failures;
- maximum dimension instances;
- maximum issue and evidence references;
- identifier widths;
- revision representation;
- sequence-counter width and wrap behavior;
- exact state persistence format;
- exact continuation format;
- durable journal requirements;
- cross-process synchronization mechanism;
- cross-reboot integrity mechanism;
- exact timeout source;
- wall-clock integration;
- exact summary-derivation contracts;
- exact restricted-handoff policy;
- exact required/optional capability profiles;
- exact update-dimension ownership after future UPD architecture exists;
- exact generation-state ownership after future OS architecture exists;
- exact runtime observation duration;
- exact operator-review state transitions;
- C-compatible boundary design.

These decisions do not authorize favorable defaults or weaker failure semantics.

## 35. Initial implementation implications

### 35.1 BOOT-I001 value layer

The initial value layer should support:

- stable typed dimension identifiers;
- bounded phase values;
- orthogonal facets;
- independent revisions;
- expected-revision transitions;
- operation and attempt identities;
- deterministic bounded issues;
- explicit transition outcomes;
- resource-exhaustion results;
- failure records;
- snapshots;
- derived dispositions;
- no-partial-commit behavior.

### 35.2 BOOT-P0

The first seed proof should produce ordered, bounded records for:

- BOOT session creation;
- minimal environment establishment;
- `/init` execution;
- seed runtime launch;
- heartbeat observations;
- seed completion;
- final disposition;
- timeout or failure.

A heartbeat is runtime liveness evidence.

It is not full runtime readiness or recovery.

### 35.3 Mock providers

Mock installers, security providers, transports, assistance sources, and runtime targets MUST preserve:

- provider attribution;
- independent dimensions;
- partial and failed outcomes;
- unavailable states;
- revision behavior;
- atomicity.

A mock MUST NOT return favorable results that a real provider contract could not justify.

### 35.4 Generated artifacts

Generated BOOT-P0 state logs and test artifacts may be retained under the reserved operational-artifact namespace.

Their presence does not make them architecture or authoritative source.

## 36. BOOT-0001 traceability registry

| BOOT-0001 invariant | BOOT-0002 coverage |
|---|---|
| BOOT-INV-004 — Trust and completion stages remain distinct | Facets, dimension phases, disposition rules, and cross-stage summary prohibitions |
| BOOT-INV-009 — BOOT state remains multidimensional | Mandatory and conditional dimensions; no oversized global enumeration |
| BOOT-INV-010 — Unknown and failure truthfulness | Knowledge, availability, validity, freshness, partial, unsupported, and indeterminate semantics |
| BOOT-INV-011 — Execution events are not completion | Runtime observation, installer state, timeout, interruption, and reboot rules |
| BOOT-INV-013 — Resource exhaustion is first-class | Resource facet, canonical outcome, failure domain, and no-commit behavior |
| BOOT-INV-014 — Conservative atomicity | Transition preparation, required issue retention, bundles, and no partial commit |
| BOOT-INV-015 — Revision and idempotency continuity | Expected revisions, operation/attempt identity, duplicate delivery, retry, reconciliation |
| BOOT-INV-021 — Installer mutation separation | Installer dimension, mutation states, rollback states, unknown prior mutation |
| BOOT-INV-022 — Minimal headless operation | Machine-readable state independent of console or desktop behavior |
| BOOT-INV-024 — Image and discovery grant no authority | Distribution-image state separation and conservative handoff rules |
| BOOT-INV-026 — Pair and component validation | Assembly dimension and required/optional component state |
| BOOT-INV-027 — Update availability grants no authority | Update dimension and stage-separation rules |
| BOOT-INV-028 — Governed peer propagation | Peer-transfer state and staged-validation behavior |
| BOOT-INV-030 — Handoff is not readiness | Handoff and runtime-observation dimensions |

## 37. Prohibited interpretations

This specification MUST NOT be interpreted to mean that:

- one global status may replace the state vector;
- `COMPLETE` means successful;
- `AVAILABLE` means valid or authorized;
- `VALID` means eligible for installation;
- `CURRENT` means authoritative;
- `LOCKED_DOWN` means compromised;
- `TIMED_OUT` means no mutation occurred;
- `INTERRUPTED` means failed safely;
- `PARTIAL` may be presented as complete;
- `UNSUPPORTED` means invalid;
- the newest timestamp wins;
- the highest version wins;
- a persisted snapshot is automatically current;
- a new process creates clean state;
- a reboot clears unresolved mutation;
- a runtime heartbeat proves readiness;
- runtime readiness proves recovery;
- recovery proves sustained runtime operation;
- installer progress proves durable completion;
- artifact transfer proves verification;
- optional capability absence may be hidden;
- required-component failure may be called degraded success;
- update fetch proves activation eligibility;
- a peer’s state report creates local release authority;
- private policy may redefine a public state value.

## 38. Completion checklist

- [x] BOOT-0000 authority is preserved.
- [x] Approved BOOT-0001 invariants are preserved.
- [x] Independent state dimensions are defined.
- [x] Orthogonal facets prevent a giant global state enumeration.
- [x] Unknown, unavailable, invalid, deferred, failed, locked-down, partial, unsupported, timed-out, and indeterminate conditions are explicit.
- [x] Failure in one dimension does not fabricate another dimension’s state.
- [x] Stage completion remains separate from later-stage success.
- [x] Resource exhaustion is first-class.
- [x] Conservative atomicity is preserved.
- [x] Expected revisions and idempotency are defined.
- [x] Concurrent and multi-dimension transitions are bounded.
- [x] Installer partial and unknown mutation remain explicit.
- [x] Reboot and process restart do not erase uncertainty.
- [x] Distribution image, installed system, runtime generation, and runtime instance remain distinct.
- [x] Required and optional component state remains distinct.
- [x] Update and peer-transfer stages remain separate.
- [x] Runtime handoff remains distinct from readiness.
- [x] Public/private boundaries are preserved.
- [x] No implementation language, ABI, filesystem, protocol, kernel, package manager, or persistence technology is mandated.
- [x] The model is usable by BOOT-I001 and BOOT-P0.
- [x] Detailed identity, assistance, artifact, runtime, and retry mechanisms remain with later specifications.
- [x] No blocking contradiction was identified.

## 39. Closing principle

> **A BOOT state report is trustworthy only when it makes uncertainty, scope, revision, ownership, and incomplete effects as visible as successful progress.**

## Revision history

### Version 0.1 — 2026-07-17

- Established the multidimensional BOOT state model.
- Defined orthogonal knowledge, availability, validity, freshness, progress, enforcement, resource, and outcome facets.
- Defined mandatory BOOT dimensions and conditional assembly and update dimensions.
- Defined candidate boot dispositions without converting them into authority.
- Defined expected-revision transitions and multi-dimension atomic bundles.
- Defined conservative failure domains and failure records.
- Defined partial mutation, rollback, timeout, cancellation, interruption, and indeterminate behavior.
- Defined persistence and reboot-continuation semantics.
- Separated distribution-image, installed-generation, runtime-generation, and runtime-instance state.
- Defined required and optional component behavior.
- Defined BOOT-I001 and BOOT-P0 implementation implications.
