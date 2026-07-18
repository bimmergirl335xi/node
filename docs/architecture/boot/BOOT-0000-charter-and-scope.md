# BOOT-0000: Node BOOT Architecture Charter and Scope

| Field | Value |
|---|---|
| Specification | BOOT-0000 |
| Title | Node BOOT Architecture Charter and Scope |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | BOOT-PUB |
| Authors | Node |
| Last updated | 2026-07-17 |
| Approval | Pending review |
| Depends on | ACS-0000 through ACS-0009; MEM-0000 through MEM-0010; IMM-0000 and IMM-0001 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in coordination scope, authority separation, failure honesty, and adjacent-system boundaries; detailed state transitions, assistance policy, recovery-plan contracts, and minimal executable language remain under review |

> **BOOT exists so that a damaged or incomplete node can seek recovery without mistaking reachability for identity, installation for recovery, or emergency conditions for unrestricted authority.**

## Architectural-intent notice

This specification defines independently authored public architecture for Node BOOT and rescue coordination.

It establishes BOOT’s charter, terminology, responsibilities, ownership boundaries, exclusions, authority constraints, public/private boundary, minimal-runtime direction, and relationship to neighboring systems.

It does not prescribe a concrete rescue image, initramfs layout, installer implementation, transport protocol, cryptographic suite, programming language, wire format, production trust topology, or recovery-authorization policy.

Public BOOT architecture is not produced by deleting, sanitizing, paraphrasing, or selectively redacting restricted designs.

Implementations may use different operating environments, languages, transports, security providers, installers, storage mechanisms, and deployment profiles. Their observable behavior MUST preserve the public distinctions and ownership boundaries established by this specification.

## 1. Purpose

Node may encounter conditions in which its ordinary runtime cannot safely or successfully start.

Examples include:

- incomplete installation;
- missing or incompatible runtime components;
- invalid durable configuration;
- damaged storage;
- unavailable dependencies;
- interrupted activation;
- failed upgrade or rollback;
- missing recovery artifacts;
- unsupported hardware state;
- uncertain participant identity;
- stale persisted identity;
- unavailable authority;
- conflicting recovery claims;
- security-provider failure;
- installer failure;
- resource exhaustion;
- partial hardware failure;
- inability to establish whether the normal operating substrate is acceptable.

These conditions require a minimal coordination layer that can operate before, outside, or instead of the normal Node runtime.

BOOT provides that coordination layer.

BOOT exists to:

- inspect bounded local boot evidence;
- keep uncertainty and failure explicit;
- select an eligible normal, restricted, rescue, review, or refusal path;
- discover possible sources of assistance without treating discovery as authentication;
- establish bounded assistance sessions;
- validate recovery plans before mutation;
- coordinate recovery-artifact acquisition;
- obtain applicable authority;
- hand authorized work to an installer;
- receive and preserve installer results;
- coordinate recovery verification;
- perform a bounded reboot or normal-runtime handoff.

BOOT does not exist to become a second implementation of ACS, the installer, transport, the security provider, IMM, MEM, resource management, or the normal runtime.

## 2. Foundational premise

A node that cannot establish its operating condition MUST NOT pretend that uncertainty is normal readiness.

A node may be physically capable of executing instructions while remaining unable to establish:

- its durable identity;
- the validity of persisted identity;
- the authority of a responder;
- the applicability of a recovery plan;
- the integrity or suitability of an artifact;
- installer eligibility;
- installation completion;
- activation readiness;
- recovery completion;
- normal-runtime readiness.

BOOT preserves those distinctions.

Emergency, rescue, local, offline, first-boot, and degraded conditions do not dissolve architecture.

They increase the need for explicit identity, evidence, authority, boundedness, attribution, and conservative failure behavior.

## 3. Normative language

The terms **MUST** and **MUST NOT** describe mandatory architectural requirements.

The terms **SHOULD** and **SHOULD NOT** describe strong recommendations that may be departed from only with documented justification.

The term **MAY** describes permitted behavior.

Conformance depends on observable behavior rather than component names such as `boot`, `bootstrap`, `recovery`, `rescue`, `installer`, `watchdog`, `trusted`, or `emergency`.

A component does not gain BOOT authority merely because it executes early in the boot chain or has technical access to local storage.

## 4. BOOT terminology

### 4.1 BOOT

**BOOT** is the Node architecture governing minimal boot inspection and rescue coordination.

BOOT is the architecture’s proper name. This specification does not require it to be expanded as an acronym.

BOOT defines semantic responsibilities and boundaries. It does not define one executable, process, binary, image, or repository directory.

### 4.2 Physical node

A **physical node** is a physical or virtual substrate on which Node software may be installed or executed.

A physical node is not automatically:

- a validated participant;
- an admitted ACS member;
- an authorized recovery actor;
- a memory authority;
- a successful Node runtime.

### 4.3 Local rescue environment

A **local rescue environment** is the minimal execution environment in which BOOT coordination is presently operating.

It may be implemented through:

- an initramfs;
- a BusyBox-style environment;
- a dedicated rescue image;
- a restricted userspace;
- removable recovery media;
- a platform-specific recovery environment;
- another bounded headless environment.

The local rescue environment is an implementation substrate. It is not itself identity, authority, admission, verification, or proof of recovery.

### 4.4 Normal runtime

The **normal runtime** is the ordinary Node execution environment responsible for production service execution, scheduling, placement, runtime lifecycle, and runtime-owned readiness after an accepted handoff.

BOOT may select, prepare, or initiate a normal-runtime handoff.

BOOT does not define successful normal-runtime operation merely by transferring control to it.

### 4.5 BOOT session

A **BOOT session** is one bounded instance of BOOT inspection, rescue coordination, or handoff activity.

A BOOT session MUST have an identity suitable for correlation and idempotency.

A BOOT session identity is not automatically:

- a durable participant identity;
- an ACS relationship;
- a security credential;
- recovery authority;
- installer authority.

### 4.6 Local boot inspection

**Local boot inspection** is the bounded collection and evaluation of locally available evidence relevant to selecting the next boot or recovery path.

Inspection may consider declared evidence concerning:

- platform and hardware condition;
- local operating profile;
- configuration presence and structure;
- required component availability;
- persisted identity material;
- recovery environment capability;
- installer availability;
- locally available recovery artifacts;
- prior incomplete operation records;
- security-provider availability;
- applicable authority evidence.

Inspection MUST NOT silently infer truth from missing evidence.

### 4.7 Boot disposition

A **boot disposition** is BOOT’s declared candidate path for the current session and evidence boundary.

A disposition may identify eligibility for:

- normal-runtime handoff;
- restricted operation;
- rescue operation;
- operator review;
- deferred progression;
- refusal to proceed;
- locked-down operation.

Disposition selection is not authorization.

A selected path MUST still satisfy all authority, admission, verification, resource, installer, and handoff requirements applicable to that path.

### 4.8 Rescue operation

A **rescue operation** is bounded BOOT-coordinated work intended to establish, repair, replace, validate, or prepare prerequisites required for a later installation, activation, reboot, or normal-runtime handoff.

Rescue operation does not imply unrestricted access or permission to mutate durable state.

### 4.9 Assistance source

An **assistance source** is a local or remote participant, device, medium, or service that may offer evidence, capabilities, artifacts, planning input, installer access, authority references, or operator access relevant to recovery.

Reachability or discovery of an assistance source does not establish:

- identity;
- authentication;
- authority;
- admission;
- correctness;
- suitability;
- exclusive responder status.

### 4.10 Assistance session

An **assistance session** is a bounded interaction through which BOOT and an eligible assistance source negotiate declared recovery-related requests and results.

An assistance session MUST remain distinguishable from:

- discovery;
- a transport connection;
- a secure session;
- an ACS relationship;
- a recovery authorization;
- a recovery plan;
- an installer operation.

### 4.11 Recovery request

A **recovery request** is a bounded request to evaluate or coordinate a declared recovery objective.

A recovery request may originate from:

- local BOOT inspection;
- an authorized operator;
- an installer;
- an admitted service;
- IMM;
- another eligible participant.

A request is not authorization and does not compel execution.

### 4.12 Recovery plan

A **recovery plan** is an identified, revisioned, bounded description of proposed recovery work, required evidence, dependencies, artifacts, authority, installer operations, expected outcomes, failure behavior, verification conditions, and handoff conditions.

A recovery plan is not:

- an authority grant;
- an installer result;
- proof that referenced artifacts are available;
- proof that referenced artifacts are valid;
- proof that installation will succeed;
- proof that recovery has completed.

### 4.13 Recovery artifact

A **recovery artifact** is a bounded object proposed for use in inspection, repair, installation, activation staging, verification, or handoff.

Examples may include:

- system manifests;
- package collections;
- images;
- configuration inputs;
- enrollment bundles;
- installer inputs;
- firmware packages;
- recovery metadata.

A file, payload, removable medium, or transferred byte sequence is not accepted as a recovery artifact until the applicable identity, structure, integrity, compatibility, provenance, authorization, and policy requirements have been evaluated.

### 4.14 Artifact reference

An **artifact reference** identifies or locates separately governed recovery material.

An artifact reference is not:

- the artifact itself;
- proof of availability;
- proof of integrity;
- authority to retrieve;
- authority to install;
- proof of compatibility.

### 4.15 Installer handoff

An **installer handoff** is BOOT’s bounded transfer of an authorized, validated installation request and its associated references to an eligible installer.

The handoff MUST preserve:

- request identity;
- plan identity and revision;
- target identity or explicit unresolved target state;
- granted authority and scope;
- artifact references;
- required validation;
- mutation limits;
- rollback expectations;
- expected result contract;
- idempotency information;
- cancellation and interruption conditions;
- audit references.

The handoff does not make BOOT the installer.

### 4.16 Recovery verification

**Recovery verification** is the bounded evaluation of declared evidence against the completion conditions of a recovery plan or recovery stage.

Recovery verification may establish that a particular recovery scope satisfies its declared requirements.

It does not automatically establish:

- universal node health;
- unrestricted ACS admission;
- broad trust;
- memory consistency;
- IMM restoration;
- successful long-term normal-runtime operation.

### 4.17 Reboot handoff

A **reboot handoff** is BOOT’s bounded preparation and transfer of control to a platform reboot, a selected operating environment, or a normal-runtime startup path.

A reboot handoff is not proof that the next environment:

- loaded successfully;
- retained the expected state;
- activated correctly;
- entered normal operation;
- remained operational.

### 4.18 Bootstrap authority

**Bootstrap authority** is the applicable authority permitting a declared boot, enrollment, recovery, installation, activation, or handoff operation before ordinary runtime authority is fully available.

Bootstrap authority MUST be externally grounded through an eligible authority path. BOOT MUST NOT create or enlarge it through self-report.

The term describes an architectural authority role. It does not refer to the repository path `bootstrap/`.

### 4.19 Evidence

**Evidence** is identified information presented in support of a scoped BOOT conclusion or operation.

Evidence MUST preserve applicable:

- identity;
- source;
- provenance;
- freshness;
- integrity status;
- scope;
- revision;
- transformation history;
- uncertainty;
- conflict state.

Evidence does not independently create authority or prove the conclusion for which it is offered.

## 5. BOOT responsibilities

### 5.1 Local inspection

BOOT owns coordination of bounded local boot inspection.

BOOT MUST:

- state which evidence was examined;
- identify required evidence that was absent or unavailable;
- preserve invalid, stale, conflicting, partial, and unknown conditions;
- avoid treating successful code execution as proof of operating-substrate suitability;
- remain within explicit resource limits.

BOOT does not own the underlying hardware, firmware, filesystem, security, memory, or runtime semantics that produce the evidence.

### 5.2 Candidate disposition selection

BOOT owns selection of a candidate boot disposition from the currently eligible paths.

The selected disposition MUST identify:

- the applicable scope;
- the evidence boundary;
- unresolved uncertainty;
- required authority;
- required dependencies;
- prohibited next actions;
- the conditions required for progression.

Selection MUST NOT be reported as authorization.

### 5.3 Failure and uncertainty reporting

BOOT owns public reporting of BOOT-scoped failure and uncertainty.

BOOT MUST report conservative outcomes when it cannot establish a required condition.

It MUST NOT silently convert:

- missing into absent;
- unknown into healthy;
- reachable into authenticated;
- authenticated into authorized;
- installed into active;
- restarted into recovered;
- quiet into safe;
- first into preferred;
- local into trusted;
- newest into valid.

### 5.4 Assistance negotiation

BOOT owns the semantic coordination of assistance requests and results.

BOOT MAY:

- advertise a bounded need for assistance;
- receive bounded assistance offers;
- compare declared capabilities and requirements;
- establish an eligible assistance session;
- request evidence, plan proposals, artifacts, authority references, installer access, or operator review.

BOOT MUST NOT define transport packet formats or treat the first, closest, fastest, or most responsive source as authoritative merely because it responded.

### 5.5 Recovery-plan validation

BOOT owns validation of a recovery plan as a BOOT coordination object.

Validation MUST determine, within declared scope, whether:

- the plan structure is supported;
- its identity and revision are present;
- its target and preconditions are adequately represented;
- required authority is identified;
- referenced artifacts and dependencies are bounded;
- installer responsibilities are explicit;
- mutation and rollback boundaries are declared;
- expected outcomes are testable;
- verification requirements are declared;
- partial, interrupted, failed, and indeterminate outcomes are represented;
- resource requirements remain within applicable limits;
- unsupported or unknown requirements remain explicit.

Plan validation does not authorize the plan or perform its steps.

### 5.6 Artifact-recovery coordination

BOOT owns semantic coordination of recovery-artifact requests, transfer results, validation requests, and availability state.

BOOT does not own:

- network transfer;
- cryptographic verification primitives;
- artifact repository policy;
- package installation;
- durable artifact custody;
- memory retention policy.

Transfer completion MUST NOT be reported as artifact verification.

Artifact verification MUST NOT be reported as installation.

### 5.7 Authority coordination

BOOT owns the coordination and preservation of authority evidence required for BOOT-scoped progression.

BOOT MAY request or consume:

- capability references;
- authority references;
- authentication results;
- security-provider verification results;
- operator approvals;
- installer-provisioned authority;
- hardware-rooted or manufacturer-provisioned evidence;
- previously persisted and revalidated authority material.

BOOT MUST NOT:

- self-authorize;
- invent an authority anchor;
- silently widen scope;
- treat urgency as authority;
- treat physical access as authority;
- treat an enrollment token as participant identity;
- treat a secure session as permission;
- restore stale or revoked authority merely because normal operation failed.

### 5.8 Installer handoff

BOOT owns authorization checks and coordination required before an installer handoff.

BOOT MUST NOT directly perform installer-owned durable mutations.

BOOT MUST preserve an explicit result when the installer:

- rejects the request;
- lacks capability;
- is unavailable;
- reports invalid inputs;
- performs no mutation;
- performs partial mutation;
- rolls back;
- cannot roll back;
- is interrupted;
- fails;
- returns an indeterminate result.

### 5.9 Recovery-result coordination

BOOT owns collection and correlation of BOOT-scoped results following plan execution or installer activity.

BOOT MUST preserve distinctions among:

- request accepted;
- work admitted;
- work started;
- mutation attempted;
- mutation completed;
- rollback completed;
- output structurally validated;
- output security-verified;
- activation staged;
- recovery criteria verified;
- reboot prepared;
- runtime handoff attempted;
- runtime readiness reported.

No earlier result proves a later result.

### 5.10 Reboot and runtime handoff

BOOT owns preparation of a bounded reboot or normal-runtime handoff after applicable prerequisites are satisfied.

BOOT MUST preserve enough bounded information to identify:

- the selected target environment;
- the applicable plan and revision;
- the verified preconditions;
- unresolved limitations;
- expected next-stage evidence;
- failure or fallback expectations;
- the handoff attempt identity.

The normal runtime remains authoritative for its own readiness and operation after handoff.

## 6. Foundational trust and completion distinctions

BOOT MUST preserve these distinctions:

```text
discovery != authentication
authentication != authority
authority != admission
selection != authorization
transfer != verification
verification != installation
installation != activation
activation != recovery
recovery != successful normal runtime operation
```

No stage silently proves a later stage.

### 6.1 Discovery is not authentication

Discovery establishes only that a possible source, participant, service, medium, or capability may exist.

Discovery does not establish who controls it.

### 6.2 Authentication is not authority

Authentication establishes a bounded security-provider result concerning presented identity evidence.

It does not determine whether the authenticated participant may perform a recovery operation.

### 6.3 Authority is not admission

Authority establishes that an identified actor may attempt an operation within a declared scope.

ACS, BOOT, installer, runtime, resource, or other applicable admission may still defer or reject the attempt.

### 6.4 Selection is not authorization

BOOT may select a candidate path, plan, artifact, installer, or assistance source.

Selection does not grant permission to use it.

### 6.5 Transfer is not verification

Moving bytes or receiving a transport acknowledgment does not establish artifact identity, integrity, authenticity, compatibility, completeness, or suitability.

### 6.6 Verification is not installation

Verification establishes only the declared verified properties.

It does not mean an installer has applied the verified material.

### 6.7 Installation is not activation

Durable mutation or package installation does not establish that the installed environment has been selected, staged, or permitted to execute.

### 6.8 Activation is not recovery

Activation establishes that an environment was made eligible to start or was started.

It does not prove that the recovery objective was met.

### 6.9 Recovery is not successful normal-runtime operation

A scoped recovery may satisfy its declared completion conditions while the normal runtime remains unavailable, degraded, incompatible, unadmitted, or unable to satisfy its own readiness requirements.

## 7. Independent state dimensions

BOOT MUST NOT represent the entire boot and recovery process through one oversized state enumeration.

At minimum, implementations must preserve independent dimensions for:

1. **Local boot state** — what BOOT can establish about the local boot substrate and eligible next path.
2. **Assistance-session state** — the progress and result of assistance discovery and negotiation.
3. **Authority state** — the availability, validity, scope, freshness, conflict, and lifecycle of required authority.
4. **Artifact state** — the discovery, transfer, verification, compatibility, availability, and rejection state of recovery artifacts.
5. **Installer state** — installer availability, admission, execution, mutation, rollback, result, and uncertainty.

These dimensions may advance independently.

For example:

- an assistance session may be authenticated while authority remains unavailable;
- an artifact may be verified while the installer remains unavailable;
- an installer may complete mutation while activation remains deferred;
- local inspection may remain partial while operator review is active;
- recovery verification may fail even after successful installation.

An implementation MAY compute a summary for display or policy evaluation.

A summary MUST NOT erase or replace the underlying dimensional states.

## 8. Required conservative conditions

Later BOOT specifications define complete state vocabularies and transition rules.

This charter requires explicit representation of at least the following conditions wherever applicable:

### 8.1 Unknown

**Unknown** means available evidence is insufficient to establish the condition.

Unknown is not healthy, safe, absent, compatible, authenticated, authorized, current, or failed.

### 8.2 Unavailable

**Unavailable** means a required participant, dependency, evidence source, authority source, artifact, installer, security provider, resource, or operation is known to be inaccessible.

Unavailable does not prove invalidity.

### 8.3 Invalid

**Invalid** means supplied material or state failed an applicable declared structural, semantic, revision, compatibility, security-provider, authority, or policy requirement.

Invalidity MUST identify the evaluated scope and applicable requirement where disclosure is permitted.

### 8.4 Deferred

**Deferred** means progression has not been accepted or rejected permanently but cannot proceed under current conditions.

Deferred MUST identify the condition required for reconsideration where that information is public and available.

### 8.5 Failed

**Failed** means a declared BOOT operation did not satisfy its required completion conditions.

Failure of an operation does not independently prove that its target is damaged, malicious, compromised, or unrecoverable.

### 8.6 Locked down

**Locked down** means current policy or an authorized protective condition prohibits progression beyond a declared boundary.

Locked-down state:

- MUST identify its scope;
- MUST preserve applicable safety, operator, evidence, audit, and recovery paths;
- MUST NOT be treated as proof of compromise;
- MUST NOT silently become permanent authority transfer.

## 9. Identity direction

BOOT consumes ACS and security-provider identity concepts.

It does not redefine participant identity.

Public BOOT profiles may include the following rescue-specific identity conditions.

### 9.1 `BOOT_EPHEMERAL`

`BOOT_EPHEMERAL` provides correlation within a bounded rescue session.

It provides:

- no durable participant identity;
- no durable authority;
- no automatic admission;
- no authority to install, enroll, activate, or recover.

### 9.2 `BOOT_PROVISIONAL`

`BOOT_PROVISIONAL` represents identity issued through an eligible installer, operator enrollment process, or authorized provisioning authority.

It requires:

- authentication;
- declared scope;
- lifecycle state;
- revalidation before reliance;
- applicable admission and authority evaluation.

Provisional identity does not grant automatic recovery authority or ordinary mesh membership.

### 9.3 `BOOT_ROOTED`

`BOOT_ROOTED` represents identity bound through a manufacturer-provisioned or hardware-rooted security provider.

Rooted identity may provide stronger identity evidence.

It does not grant:

- automatic recovery authority;
- unrestricted installer authority;
- automatic ACS admission;
- immunity from revocation or review;
- proof that the current software environment is valid.

### 9.4 Enrollment tokens

An enrollment token is evidence used to obtain, bind, or validate identity.

It is not itself the participant identity.

Possession of an enrollment token does not independently prove:

- rightful possession;
- freshness;
- scope;
- authority;
- admission;
- successful enrollment.

### 9.5 Persisted identity

Previously persisted identity MUST be revalidated before it is relied upon in a new BOOT session.

Successful historical validation is not current validation.

### 9.6 Prohibited identity derivation

Durable BOOT or participant identity MUST NOT be derived solely from:

- an IP address;
- a MAC address;
- a hostname;
- a route;
- a disk path;
- a device path;
- a process identifier;
- a boot order;
- physical proximity;
- removable-media insertion;
- the first responder.

## 10. Authority-anchor direction

An **authority-anchor candidate** is material proposed as a basis for evaluating bootstrap authority.

A candidate does not become an accepted authority anchor merely because it is:

- present locally;
- found first;
- discovered over a network;
- stored on removable media;
- embedded in an artifact;
- supplied by an authenticated participant;
- previously accepted;
- hardware-associated.

Public authority-anchor categories may include:

- operator-approved anchor;
- installer-provisioned anchor;
- hardware-rooted anchor;
- manufacturer-provisioned anchor;
- previously persisted and revalidated anchor.

Acceptance of an authority anchor MUST use the applicable security-provider, ACS, policy, lifecycle, freshness, revocation, and scope requirements.

Discovery mechanisms and removable media MAY carry anchor candidates.

They do not create authority.

Public BOOT documentation MUST NOT publish:

- production anchors;
- real credentials;
- private keys;
- recovery secrets;
- infrastructure addresses;
- private authority-assignment policy;
- private concurrence requirements;
- production enrollment procedures.

## 11. Ownership boundaries

BOOT coordinates across adjacent systems without absorbing them.

| Domain | Retained responsibility | BOOT relationship |
|---|---|---|
| **ACS** | Participant and endpoint identities; relationships; connection descriptors; authority and capability references; evidence references; admission; lifecycle; operational condition; enforcement condition; revisions; idempotency semantics | BOOT consumes applicable ACS contracts and MUST NOT redefine, bypass, or create hidden alternatives to them |
| **Installer** | Disk preparation; operating-system deployment; package and artifact installation; durable configuration writes; activation staging; rollback mechanics | BOOT may validate prerequisites, obtain authority, and coordinate a handoff; BOOT does not perform installer mutations |
| **Transport** | Byte movement; framing; addressing; retransmission; route- and link-specific behavior | BOOT defines semantic requests and results rather than packet formats |
| **Security provider** | Cryptographic identity; key custody; authentication; signature and digest verification; secure-session establishment; credential storage; replay-protection primitives | BOOT requests and consumes scoped results; it does not manage raw private keys or treat reachability as authentication |
| **IMM** | Immune evidence; assessment; recommendation; protective coordination; IMM-scoped recovery and restoration verification | IMM may provide bounded evidence or request recovery; it cannot self-enroll, self-authorize recovery, bypass BOOT, or replace bootstrap authority |
| **MEM** | Memory identity; semantic truth; operation acceptance; persistence; provenance; custody; retention; reconstruction; deletion; memory recovery and restoration authority | BOOT may request governed persistence of bounded recovery state; it does not create memory authority or directly rewrite governed memory |
| **Resource management** | Allocation; accounting; reservations; ceilings; pressure handling; reclamation; resource policy | BOOT declares requirements and operates within granted resources; it cannot override hard ceilings |
| **Normal runtime** | Production execution; scheduling; placement; service lifecycle; runtime readiness; ordinary operation after handoff | BOOT prepares and coordinates handoff; the runtime remains authoritative for its own readiness and operation |
| **Platform boot mechanisms** | Firmware execution; platform initialization; bootloader behavior; loading and transferring control to selected executable environments | BOOT may consume their results or provide selected targets through adapter boundaries; it does not redefine platform mechanisms |

No boundary may be bypassed by relabeling an operation as:

- bootstrap;
- rescue;
- recovery;
- emergency;
- local;
- offline;
- trusted;
- automatic;
- restorative;
- operator initiated.

## 12. ACS boundary

ACS remains authoritative for its public concepts.

BOOT MUST NOT independently redefine:

- participant identity;
- endpoint identity;
- relationships;
- connection identity;
- connection descriptors;
- authority references;
- capability references;
- evidence references;
- admission;
- lifecycle;
- operational condition;
- enforcement condition;
- revision semantics;
- idempotency semantics.

BOOT-specific contracts MAY compose those concepts into boot and recovery operations.

Composition does not transfer ownership.

A BOOT operation carried through ACS remains distinct from:

- the ACS relationship;
- the ACS connection;
- the transport binding;
- the secure session;
- the individual signal delivery.

Communication success does not prove BOOT-operation success.

## 13. Installer boundary

The installer owns all mutation of the installation target.

Installer-owned work includes:

- partition and filesystem preparation;
- disk and volume mutation;
- operating-system deployment;
- package installation;
- artifact expansion;
- durable configuration changes;
- boot-target configuration;
- activation staging;
- rollback;
- cleanup of installer-owned temporary state.

BOOT MAY:

- validate a plan before handoff;
- verify that required authority is available;
- verify referenced artifacts through the owning providers;
- identify the intended target and limits;
- initiate a bounded installer request;
- receive progress and final results;
- coordinate post-installation verification.

BOOT MUST NOT claim installer success from:

- request delivery;
- process launch;
- progress output;
- partial file creation;
- transport acknowledgment;
- installer liveness.

## 14. Transport boundary

BOOT defines semantic requests, results, identities, scopes, revisions, evidence references, and failure outcomes.

Transport defines how bytes move.

BOOT MUST remain transport-neutral.

A BOOT architecture contract MUST NOT require one:

- network protocol;
- packet layout;
- address family;
- routing mechanism;
- retransmission algorithm;
- removable-media format;
- local IPC mechanism.

Transport state and BOOT state remain separate.

A transport failure may make an assistance operation unavailable or failed.

It does not independently prove that the assistance source is invalid, malicious, absent, or unauthorized.

## 15. Security-provider boundary

The security provider owns cryptographic operations and protected key material.

BOOT MUST NOT:

- store raw long-term private keys in ordinary BOOT records;
- implement identity by address;
- declare a signature valid without the security provider or equivalent eligible verifier;
- treat encryption as authority;
- treat a secure session as admission;
- restore revoked credentials because rescue is urgent;
- expose reusable secrets in diagnostics or public result records.

Terms such as **authenticated**, **signature verified**, **digest verified**, **secure session established**, and **credential valid** MUST identify the responsible provider, applicable scope, and relevant freshness or lifecycle boundary.

BOOT MUST NOT use the vague term **trusted** as a replacement for those results or for explicit authority.

## 16. IMM boundary

IMM may:

- provide bounded evidence;
- report degradation, inconsistency, possible compromise, or policy concerns;
- recommend recovery;
- request BOOT revalidation;
- request a bounded rescue operation;
- verify IMM-owned recovery or restoration conditions.

IMM cannot:

- establish its own identity through self-report;
- enroll itself;
- grant itself BOOT roles;
- authorize its own recovery request;
- select itself as bootstrap authority;
- replace BOOT disposition selection;
- replace installer authority;
- directly mutate installer targets;
- expose private scores, thresholds, fingerprints, or reasoning through public BOOT contracts.

An IMM recovery request is a request supported by evidence.

It is not a directive or recovery authorization.

BOOT failure to satisfy an IMM request does not prove the target is compromised.

## 17. MEM boundary

BOOT may request governed persistence of bounded recovery state, including eligible:

- session records;
- plan identities and revisions;
- operation identities;
- result records;
- artifact references;
- authority references;
- audit references;
- incomplete-operation evidence;
- handoff records.

MEM remains authoritative for whether and how such state is:

- accepted;
- persisted;
- retained;
- versioned;
- reconstructed;
- disclosed;
- deleted;
- restored.

BOOT does not create:

- semantic truth;
- memory custody;
- retention authority;
- reconstruction authority;
- deletion authority;
- MEM recovery authority.

Persisting a BOOT claim does not make the claim true.

Recovering a BOOT record does not automatically restore its authority, freshness, applicability, or completion state.

## 18. Resource-management boundary

BOOT must operate under finite and possibly severely reduced resources.

BOOT MUST support explicit outcomes for:

- insufficient memory;
- insufficient storage;
- unavailable working space;
- unavailable cryptographic provider capacity;
- unavailable installer capacity;
- unavailable transport capacity;
- exhausted retry budget;
- bounded issue-list exhaustion;
- diagnostic retention failure.

Resource availability is not admission or authority.

Resource pressure is not proof of attack or compromise.

BOOT MUST NOT use unbounded:

- queues;
- retry loops;
- identifier lengths;
- artifact lists;
- issue lists;
- plan depth;
- recursion;
- diagnostic growth;
- retained session history.

## 19. Normal-runtime boundary

The normal runtime and BOOT have separate responsibilities.

BOOT may determine that the known prerequisites for a runtime handoff are satisfied.

The normal runtime determines whether it can:

- establish its own identity and continuity;
- initialize required services;
- satisfy runtime readiness contracts;
- obtain ACS admission;
- access required MEM roles;
- enforce applicable IMM and security conditions;
- operate within resource limits;
- enter normal or explicitly degraded service.

A successful BOOT handoff is not successful runtime operation.

The runtime MUST NOT silently rewrite or weaken the independent BOOT or rescue authority boundary merely because it has started.

## 20. Failure and atomicity

BOOT follows this mandatory rule:

> Complete all fallible construction, validation, bounded retention, and result preparation before committing authoritative state.

Before committing authoritative BOOT state, an implementation MUST complete all applicable fallible work needed to represent the result, including:

- bounded record construction;
- identifier validation;
- revision validation;
- authority evaluation;
- dependency validation;
- bounded issue retention;
- result-code selection;
- idempotency preservation;
- diagnostic preparation required by the public contract.

Allocation, storage, encoding, diagnostic, or retention failure MUST:

- return an explicit conservative outcome;
- leave authoritative state unchanged;
- never report false success;
- never silently lose idempotency information;
- never silently lose revision information;
- never commit a partial disposition as complete;
- never continue mutation merely because failure reporting is inconvenient.

No partial authoritative commit is permitted after failed construction.

## 21. Retry and idempotency direction

Detailed retry semantics are defined by later specifications.

This charter requires that:

- a retry is not automatically a new BOOT operation;
- a new assistance connection is not automatically a new BOOT session;
- repeated delivery is not independent authority or evidence;
- installer retry MUST preserve or explicitly replace operation identity;
- unknown prior completion MUST be reconciled before repeating a non-idempotent mutation;
- retry exhaustion MUST produce an explicit bounded outcome;
- reboot MUST NOT erase unresolved operation identity or uncertainty where preservation is technically possible and authorized.

BOOT MUST consume ACS revision and idempotency semantics rather than invent incompatible substitutes.

## 22. Minimal-runtime constraints

BOOT architecture must remain implementable in extremely minimal headless Linux environments.

A conforming public BOOT contract MUST NOT require:

- a graphical interface;
- a display server;
- a desktop environment;
- local human-readable output;
- the normal Node runtime;
- Julia;
- a dynamic language runtime;
- unrestricted filesystem availability;
- unrestricted heap growth;
- network availability for every recovery path;
- one service manager;
- one container runtime;
- one package manager.

BOOT MAY operate with:

- no network;
- read-only local media;
- partial storage availability;
- a restricted clock;
- limited memory;
- limited CPU capability;
- unavailable accelerators;
- incomplete hardware discovery;
- only bounded local diagnostics.

Local terminal output MAY be provided for diagnostics.

It is not an architectural dependency.

## 23. Language neutrality

Public BOOT contracts MUST remain language-neutral.

The minimal rescue executable language remains undecided among:

- strict C;
- restricted C++;
- static Rust.

The final language decision must follow measured evidence involving:

- binary size;
- startup dependencies;
- allocator behavior;
- exception or panic behavior;
- initramfs suitability;
- cross-compilation;
- parser safety;
- cryptographic integration;
- x86_64 validation;
- AArch64 validation;
- behavior on partially functional hardware.

Julia MAY participate in higher-level orchestration or analysis after verified BOOT results exist.

Julia is not part of the minimal rescue executable.

Public BOOT value contracts SHOULD support:

- fixed-width categories;
- bounded identifiers;
- explicit lengths;
- caller-owned buffers;
- structured result codes;
- independent revisions;
- deterministic bounded issue lists;
- checked arithmetic;
- explicit resource-exhaustion outcomes;
- no exceptions across an ABI;
- no transferred allocator ownership;
- no STL exposure across a C-compatible boundary;
- no raw credentials or private-key material.

This direction does not define or require a C ABI.

## 24. Repository and artifact boundary

Public architecture documents belong under:

```text
docs/architecture/boot/
```

The repository path:

```text
bootstrap/
```

is reserved for future concrete boot and recovery artifacts, including:

- initramfs contents;
- rescue images;
- generated manifests;
- installer inputs;
- enrollment bundles;
- staged recovery payloads;
- operational boot outputs.

Architecture documents MUST NOT be stored under `bootstrap/`.

Placement under `bootstrap/` does not grant an artifact authority, integrity, compatibility, verification, installation, activation, or recovery status.

## 25. Public and private classification boundary

### 25.1 Public material

Public BOOT documentation MAY contain:

- architecture;
- terminology;
- invariants;
- public state vocabulary;
- bounded value contracts;
- public result and failure outcomes;
- authority and ownership boundaries;
- retry and idempotency requirements;
- minimal-runtime requirements;
- implementation checkpoints;
- public conformance requirements;
- transport-neutral semantic examples.

### 25.2 Private material

Public BOOT documentation MUST NOT contain:

- production trust topology;
- real credentials;
- private keys;
- recovery secrets;
- infrastructure addresses;
- artifact-repository locations;
- private signing policy;
- private enrollment policy;
- private recovery-authorization rules;
- responder-selection algorithms;
- anomaly thresholds;
- behavioral fingerprints;
- private hardware topology;
- protected capability catalogs;
- hidden scores or reasoning;
- operator recovery playbooks;
- production concurrence rules;
- deployment-specific fallback order.

Public contracts must be sufficient to implement and test required behavior without exposing private production policy or mechanisms.

Undecided private mechanism is not public architecture.

## 26. Explicit non-goals

BOOT does not:

- replace platform firmware or bootloaders;
- define one operating system;
- define one rescue-image layout;
- define one initramfs toolchain;
- define one implementation language;
- define network packet formats;
- implement transport;
- invent cryptographic algorithms;
- own raw private keys;
- define production trust anchors;
- define private enrollment policy;
- define private recovery authorization;
- select production responders through public algorithms;
- perform installer-owned durable mutation;
- replace ACS admission or lifecycle;
- replace security-provider authentication;
- replace IMM assessment or restoration authority;
- replace MEM persistence or recovery semantics;
- replace resource-management policy;
- define normal-runtime scheduling;
- define successful normal-runtime operation;
- guarantee recovery from every failure;
- treat physical access as universal authority;
- treat emergency status as unrestricted permission;
- automatically propagate Node to newly discovered hardware;
- create durable identity from addresses or boot order;
- expose protected production topology or policy.

## 27. Prohibited interpretations

This specification MUST NOT be interpreted to mean that:

- BOOT is a universal super-administrator;
- the earliest executing component owns the node;
- local access grants recovery authority;
- an operator identity grants universal control;
- the first responder is preferred or authoritative;
- removable media is authoritative because it is physically present;
- rooted identity grants automatic authority;
- authenticated assistance must be accepted;
- a valid recovery plan must be executed;
- a verified artifact must be installed;
- an installer result proves activation;
- activation proves recovery;
- recovery proves normal-runtime success;
- IMM evidence is a recovery directive;
- MEM persistence makes a BOOT claim true;
- a restart restores identity, authority, memory, or recovery state;
- locked-down state proves compromise;
- private implementation policy may violate public contracts.

## 28. Relationship to the BOOT specification series

This specification establishes BOOT’s charter and highest-level public boundaries.

Later specifications refine it:

- **BOOT-0001 — Core Invariants** defines mandatory cross-cutting rules.
- **BOOT-0002 — State and Failure Model** defines dimensional state vocabularies and transition requirements.
- **BOOT-0003 — Identity, Trust, and Authority Boundary** refines identity profiles, authority anchors, authentication, revalidation, authorization, and conflict.
- **BOOT-0004 — Discovery and Assistance Negotiation** defines discovery, offer evaluation, assistance sessions, and semantic negotiation.
- **BOOT-0005 — Recovery Plans and Artifact Boundaries** defines plan identity, revision, validation, artifacts, transfer, verification, and installer inputs.
- **BOOT-0006 — Minimal Runtime and Language Constraints** defines measurable rescue-runtime requirements and language-selection evidence.
- **BOOT-0007 — ACS, IMM, and MEM Integration** defines detailed cross-architecture mappings.
- **BOOT-0008 — Failure, Retry, and Operator Intervention** defines retries, interruption, reconciliation, lock-down, and operator involvement.
- **BOOT-0009 — Public/Private Boundary and Implementation Roadmap** defines publication constraints, implementation checkpoints, conformance stages, and the public roadmap.

Later documents MUST NOT silently weaken the boundaries established here.

## 29. Open architectural decisions

The following remain unresolved:

- the minimal rescue executable language;
- measurable baseline binary-size and dependency targets;
- exact initramfs and rescue-image conformance profiles;
- complete dimensional state vocabularies;
- recovery-plan value contracts;
- artifact-manifest structure;
- assistance-session negotiation contracts;
- public responder eligibility requirements;
- authority concurrence representation;
- offline revalidation profiles;
- installer handoff result structures;
- reboot reconciliation records;
- operator-intervention interface boundaries;
- baseline conformance levels for x86_64 and AArch64;
- behavior under specific classes of partially functional hardware.

These open decisions do not authorize implementation assumptions.

They must be resolved through later specifications, measured evidence, reviewed implementation experiments, or private policy where appropriate.

## 30. Initial architectural commitments

BOOT-0000 establishes the following durable commitments:

1. BOOT is a minimal coordination layer, not a replacement for adjacent systems.
2. BOOT remains operable before or without the normal runtime.
3. Unknown and unavailable conditions remain explicit.
4. Discovery, authentication, authority, admission, selection, transfer, verification, installation, activation, recovery, and successful operation remain distinct.
5. BOOT does not self-grant identity or authority.
6. Installer-owned mutation remains outside BOOT.
7. Transport-specific behavior remains outside BOOT semantic contracts.
8. Cryptographic key custody and verification primitives remain with the security provider.
9. IMM may provide evidence or request recovery but cannot authorize itself or replace bootstrap authority.
10. MEM retains persistence, custody, retention, reconstruction, deletion, and memory-recovery authority.
11. Independent BOOT state dimensions remain independently observable.
12. All authoritative BOOT state changes obey conservative atomicity.
13. Minimal headless operation is mandatory.
14. Public contracts remain language-neutral.
15. Private trust topology, credentials, authorization policy, responder logic, and production recovery procedures remain private.
16. A successful BOOT handoff does not claim successful normal-runtime operation.

## 31. Closing principle

> **A rescue path is trustworthy only when it preserves the boundaries that failure makes tempting to ignore.**

## Revision history

### Version 0.1 — 2026-07-17

- Established the Node BOOT architecture charter.
- Defined BOOT terminology and coordination responsibilities.
- Preserved ACS, installer, transport, security-provider, IMM, MEM, resource-management, platform, and runtime ownership boundaries.
- Established rescue identity and authority-anchor direction.
- Required independent state dimensions and conservative failure conditions.
- Preserved the complete trust and completion chain.
- Established minimal headless and language-neutral implementation direction.
- Defined the public/private architecture boundary.
