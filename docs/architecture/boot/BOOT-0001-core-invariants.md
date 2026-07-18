# BOOT-0001: Node BOOT Core Invariants

| Field | Value |
|---|---|
| Specification | BOOT-0001 |
| Title | Node BOOT Core Invariants |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | BOOT-PUB |
| Authors | Node |
| Last updated | 2026-07-17 |
| Approval | Pending review |
| Depends on | BOOT-0000; applicable approved ACS, MEM, and IMM public architecture |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in ownership, authority, state-truthfulness, boundedness, self-assembly, update, public/private, and runtime-handoff invariants; detailed state vocabularies, identity contracts, assistance negotiation, artifact contracts, measurable runtime profiles, and update mechanisms remain intentionally deferred |

> **Failure may reduce the paths available to BOOT, but it does not erase ownership, authority, boundedness, evidence, or completion requirements.**

## Architectural-intent notice

This specification defines the canonical mandatory invariants that every conforming Node BOOT architecture, design, implementation, deployment profile, rescue environment, installation environment, and runtime handoff must preserve.

It refines the charter established by BOOT-0000. It does not replace, supersede, weaken, or reinterpret BOOT-0000.

The invariants apply during:

- ordinary boot;
- first installation;
- installation-media boot;
- rescue;
- recovery;
- dynamic local self-assembly;
- degraded operation;
- offline operation;
- update preparation;
- peer-assisted artifact transfer;
- rollback;
- reboot;
- normal-runtime handoff;
- emergency or authorized protective conditions.

The invariants remain binding when dependencies are absent, evidence is incomplete, storage is damaged, resources are exhausted, authority is unavailable, or an operation has partially failed.

This specification is independently authored public architecture.

It is not produced by deleting, sanitizing, paraphrasing, or selectively redacting restricted designs.

It does not disclose private authorization policy, production trust topology, credentials, repository locations, responder-selection logic, private modules, thresholds, operator procedures, or protected deployment details.

## 1. Purpose

BOOT-0001 establishes the canonical invariant set governing all BOOT behavior.

Its purpose is to define which rules remain true regardless of:

- hardware architecture;
- boot medium;
- installation state;
- network availability;
- runtime availability;
- implementation language;
- transport;
- installer implementation;
- security-provider implementation;
- storage condition;
- available source or artifacts;
- update availability;
- assistance-source behavior;
- operator presence;
- resource pressure;
- deployment topology.

These invariants are intended to serve as:

- an architecture-design checklist;
- an implementation-design checklist;
- a code-review checklist;
- a conformance checklist;
- a BOOT-P0 acceptance reference;
- a BOOT-I001 acceptance reference;
- a cross-architecture conflict detector;
- a repository re-track reference;
- a failure-injection test source;
- a boundary review source for later BOOT documents.

A BOOT implementation does not conform merely because its ordinary boot or installation path succeeds.

Conformance requires preserving the applicable invariants during failure, interruption, uncertainty, resource exhaustion, degraded operation, and unauthorized requests.

## 2. Authority and relationship to BOOT-0000

BOOT-0000 is the higher-authority BOOT charter.

BOOT-0001:

- formalizes BOOT-0000’s durable commitments as stable invariant identifiers;
- makes those commitments directly usable for design, implementation, review, and testing;
- adds clarification where required by self-assembly, source-bearing distribution, and governed update direction;
- assigns detailed mechanisms to later BOOT specifications;
- does not alter BOOT’s charter, ownership, or public/private boundary.

If this specification and BOOT-0000 appear to conflict:

1. BOOT-0000 governs.
2. The conflict MUST be reported.
3. BOOT-0001 MUST be corrected or explicitly superseded.
4. An implementation MUST NOT select the weaker interpretation for convenience.

No genuine contradiction with BOOT-0000 was identified during this draft.

The OS-distribution direction introduces additional applications of existing BOOT-0000 rules. It does not grant BOOT ownership of the operating system, installer, update system, kernel, package system, or runtime.

## 3. Normative language

The terms **MUST** and **MUST NOT** describe mandatory architectural requirements.

The terms **SHOULD** and **SHOULD NOT** describe strong recommendations. A departure requires documented justification and MUST NOT violate a mandatory invariant.

The term **MAY** describes permitted behavior.

The terms **REQUIRED**, **SHALL**, and **SHALL NOT** are not used as separate requirement levels in this specification. Where they appear in quoted or incorporated material, they have the same mandatory force as **MUST** or **MUST NOT**.

An invariant is not satisfied merely because:

- a component carries a BOOT-related name;
- an operation occurs locally;
- an operation occurs early during startup;
- a path is described as rescue or emergency;
- a process has filesystem or device access;
- a request originates from an operator-facing component;
- a source is authenticated;
- an artifact is present;
- an installer process exits successfully;
- the runtime starts.

Observable behavior determines conformance.

## 4. Scope

These invariants apply to public and private implementations of BOOT responsibilities, including:

- local boot inspection;
- boot-disposition selection;
- rescue entry;
- assistance discovery;
- assistance sessions;
- identity evidence;
- bootstrap authority;
- recovery requests;
- recovery plans;
- artifact references;
- source material;
- package material;
- toolchain material;
- transferred artifacts;
- artifact verification;
- installer handoff;
- installer results;
- activation staging;
- rollback;
- reboot continuation;
- runtime handoff;
- dynamic assembly planning;
- source-bearing installation or rescue images;
- kernel and micro-OS pair selection;
- update discovery;
- release-manifest consideration;
- peer-assisted distribution;
- local update validation;
- public and private module availability;
- bounded BOOT persistence;
- BOOT conformance testing.

The invariants apply whether BOOT executes:

- in an initramfs;
- in a BusyBox-style environment;
- from removable installation media;
- from a dedicated rescue image;
- from a restricted installed userspace;
- in a virtual machine;
- through a platform-specific recovery environment;
- through a later implementation environment conforming to BOOT-0006.

## 5. Explicit non-goals

BOOT-0001 does not define:

- a complete state machine;
- complete state vocabularies;
- state-transition tables;
- BOOT identity-profile schemas;
- credential formats;
- authority-anchor formats;
- authentication mechanisms;
- assistance discovery mechanisms;
- assistance offer structures;
- responder-selection algorithms;
- transport packet formats;
- network protocols;
- recovery-plan schemas;
- artifact-manifest schemas;
- installer APIs;
- installer implementation;
- filesystem layout;
- ISO construction;
- disk-image construction;
- bootloader selection;
- Linux kernel modification;
- kernel configuration;
- one kernel version;
- one micro-OS;
- one package manager;
- one source manager;
- one toolchain;
- one update protocol;
- release-channel names;
- update-check intervals;
- rollout algorithms;
- fleet-management policy;
- one implementation language;
- a C ABI;
- private recovery policy;
- production repository locations;
- production signing policy;
- production trust topology.

These subjects belong to later BOOT specifications, future OS and update architecture, installer architecture, transport architecture, security-provider architecture, deployment policy, or private operational policy.

## 6. Invariant interpretation rules

### 6.1 Stable identifiers

Each invariant has a stable identifier in the form:

```text
BOOT-INV-NNN
```

The identifier refers to the architectural rule, not to one implementation, test, data structure, source file, or release.

Invariant identifiers MUST NOT be renumbered merely for formatting convenience after approval.

A materially changed invariant requires explicit review of:

- its prior meaning;
- its new meaning;
- affected BOOT-0000 commitments;
- affected later specifications;
- implementation consequences;
- conformance consequences.

### 6.2 Invariants are cumulative

All applicable invariants operate together.

Satisfying one invariant does not excuse violating another.

Examples:

- authenticated assistance still requires authority and admission;
- a verified artifact still requires installation eligibility;
- a resource-constrained environment still requires bounded parsing;
- emergency operation still requires attributable authority;
- a valid update still requires local compatibility and staged activation;
- successful runtime launch still requires runtime-owned readiness evaluation.

### 6.3 No compensating violation

An implementation MUST NOT claim that one protective feature compensates for violating an unrelated invariant.

Examples of invalid compensation include:

- encryption compensating for absent authorization;
- a rollback image compensating for unbounded destructive mutation;
- operator presence compensating for missing identity;
- extensive logging compensating for partial authoritative commits;
- source availability compensating for absent execution authority;
- successful testing on one host compensating for unsupported target state;
- a secure transport compensating for absent ACS admission.

### 6.4 Later specifications refine mechanisms

Later BOOT specifications MAY define:

- additional states;
- value contracts;
- records;
- transitions;
- validation requirements;
- operation flows;
- conformance profiles.

They MUST preserve these invariants.

A later specification MUST NOT narrow an invariant’s applicability unless BOOT-0000 and BOOT-0001 are deliberately revised.

### 6.5 Private policy cannot weaken public invariants

Private policy MAY:

- select among publicly eligible actions;
- define production authority assignments;
- define responder preference;
- define private release channels;
- define protected module eligibility;
- define private escalation and operator processes.

Private policy MUST NOT:

- bypass an invariant;
- convert missing evidence into success;
- create unbounded BOOT authority;
- permit unauthorized propagation;
- redefine adjacent-system ownership;
- treat private capability possession as universal permission.

### 6.6 Conservative interpretation under ambiguity

When an invariant’s application is materially ambiguous, the implementation MUST:

- preserve authoritative state;
- avoid broader mutation or authority;
- report the ambiguity;
- use an explicitly permitted narrower action;
- defer, refuse, or request review where required.

Ambiguity MUST NOT be resolved by selecting the most permissive interpretation.

### 6.7 Prohibited interpretations are normative

The prohibited interpretations listed under each invariant are part of the invariant.

They are not merely examples or commentary.

## 7. Canonical BOOT invariants

### BOOT-INV-001 — Coordination does not absorb ownership

**Binding rule**

BOOT MUST coordinate boot inspection, rescue, recovery, installation handoff, verification, reboot, and runtime handoff without absorbing the authority or semantic ownership of adjacent systems.

**Why the invariant exists**

BOOT necessarily operates across many architectural domains. Without a strict boundary, implementation convenience could turn BOOT into an undefined super-administrator.

**Required observable behavior**

A conforming implementation MUST:

- identify the owning architecture or provider for each external decision or mutation;
- distinguish BOOT coordination results from provider-owned results;
- preserve provider-specific failure and uncertainty;
- invoke privileged operations only through the owning boundary;
- avoid representing delegated work as BOOT-owned work.

**Prohibited interpretations**

This invariant forbids treating BOOT as:

- the installer;
- the security provider;
- transport;
- ACS;
- MEM;
- IMM;
- resource management;
- the operating system;
- the normal runtime;
- universal lifecycle authority.

**Failure or uncertainty behavior**

If an owning provider is unavailable, unknown, conflicting, or failed, BOOT MUST report that condition and MUST NOT silently replace the provider.

**Primary later specification**

BOOT-0007 — ACS, IMM, and MEM Integration.

---

### BOOT-INV-002 — Early, local, rescue, offline, and emergency execution do not enlarge authority

**Binding rule**

Executing before the normal runtime, executing locally, operating offline, running from installation media, entering rescue mode, or responding to an emergency MUST NOT create ownership, identity, admission, or authority that did not otherwise exist.

**Why the invariant exists**

BOOT may execute with unusual technical access and limited oversight. Those conditions increase risk; they do not create permission.

**Required observable behavior**

A conforming implementation MUST:

- apply applicable identity and authority checks to local and remote operations;
- preserve operation scope during emergency or protective conditions;
- distinguish technical access from authorization;
- preserve review, attribution, and lifecycle requirements;
- refuse or defer privileged work when authority cannot be established.

**Prohibited interpretations**

This invariant forbids claims that:

- local storage access grants mutation authority;
- early execution grants ownership of the node;
- rescue mode bypasses installer boundaries;
- offline operation permits unverifiable activation;
- emergency status permits unrestricted installation or propagation;
- removable media becomes authoritative through physical presence.

**Failure or uncertainty behavior**

Unavailable authority MUST remain unavailable. Urgency MAY affect which authorized options are considered, but MUST NOT manufacture authority.

**Primary later specification**

BOOT-0003 — Identity, Trust, and Authority Boundary.

---

### BOOT-INV-003 — Adjacent-system ownership remains intact

**Binding rule**

BOOT MUST preserve the following ownership boundaries:

- the installer owns durable installation mutation;
- transport owns byte movement, framing, addressing, retransmission, and link-specific behavior;
- security providers own cryptographic verification, authentication primitives, protected key custody, secure-session establishment, credential storage, and replay-protection primitives;
- ACS owns participant and endpoint identities, relationships, admission, applicable authority and capability references, lifecycle, revision, idempotency, and enforcement semantics;
- MEM owns semantic memory, persistence, custody, retention, reconstruction, recovery, restoration, and deletion authority;
- IMM owns immune observation, evidence assessment, recommendation, protective coordination, and IMM-scoped restoration responsibilities;
- resource management owns allocation, accounting, reservations, ceilings, pressure policy, and reclamation;
- the normal runtime owns production execution, scheduling, service initialization, and runtime readiness after an accepted handoff.

**Why the invariant exists**

Cross-domain recovery becomes incoherent when one layer silently claims another layer’s result or permission.

**Required observable behavior**

A conforming implementation MUST:

- attribute each result to its owning provider;
- retain provider-specific state and revision references;
- use adapters or contracts rather than hidden substitutions;
- distinguish request, provider acceptance, provider execution, and provider completion;
- reject operations that require an absent owning boundary.

**Prohibited interpretations**

This invariant forbids:

- BOOT writing installation targets directly as a substitute for an installer;
- BOOT validating cryptographic signatures through an undeclared local guess;
- BOOT inventing ACS identity or admission;
- BOOT directly rewriting governed MEM state;
- BOOT treating IMM evidence as an authorization;
- BOOT exceeding resource ceilings;
- BOOT declaring runtime readiness.

**Failure or uncertainty behavior**

Provider failure MUST remain distinguishable from BOOT failure and target failure. BOOT MUST NOT infer that the target caused a provider failure.

**Primary later specification**

BOOT-0007 — ACS, IMM, and MEM Integration.

---

### BOOT-INV-004 — Trust and completion stages remain distinct

**Binding rule**

BOOT MUST preserve the following distinctions:

```text
discovery is not authentication
authentication is not authority
authority is not admission
selection is not authorization
transfer is not verification
verification is not installation
installation is not activation
activation is not recovery
recovery is not successful normal-runtime operation
```

No earlier stage may silently prove a later stage.

**Why the invariant exists**

Boot and recovery pipelines involve several independently governed decisions. Collapsing them creates false success and privilege escalation.

**Required observable behavior**

A conforming implementation MUST:

- record the outcome of each applicable stage separately;
- identify which provider or authority produced each result;
- preserve stage-specific revisions and evidence references;
- require the later stage to apply its own rules;
- avoid reporting a pipeline-level success when a required later stage remains unknown or incomplete.

**Prohibited interpretations**

This invariant forbids:

- treating reachability as authentication;
- treating authentication as permission;
- treating artifact receipt as integrity verification;
- treating installer completion as activation;
- treating reboot as recovery;
- treating heartbeat output as sustained runtime success.

**Failure or uncertainty behavior**

A stage may succeed while a later stage fails, remains unavailable, or is deferred. The earlier success MUST remain scoped and MUST NOT be rewritten as total failure or total completion.

**Primary later specification**

BOOT-0002 — State and Failure Model, with operation-specific refinement in BOOT-0005.

---

### BOOT-INV-005 — BOOT session identity is not durable participant identity

**Binding rule**

A BOOT session identity MUST remain distinct from durable participant identity.

**Why the invariant exists**

A session requires correlation and idempotency even when durable identity is absent or unresolved. Conflating the two would allow a temporary execution instance to claim lasting identity.

**Required observable behavior**

A conforming implementation MUST:

- identify each BOOT session independently;
- use session identity for bounded correlation;
- state whether durable participant identity is established, provisional, unavailable, conflicting, or unresolved;
- prevent a session identifier from being used as a durable identity credential or authority source.

**Prohibited interpretations**

This invariant forbids:

- promoting a random session identifier into durable identity;
- treating repeated use of a session identifier as proof of continuity;
- granting installation, recovery, or mesh authority because a session is locally active.

**Failure or uncertainty behavior**

If durable identity cannot be established, BOOT MAY continue only within operations explicitly permitted for that identity condition. It MUST NOT fabricate durable continuity.

**Primary later specification**

BOOT-0003 — Identity, Trust, and Authority Boundary.

---

### BOOT-INV-006 — Durable identity is not location, order, proximity, or responsiveness

**Binding rule**

Durable identity MUST NOT be derived solely from:

- an IP address;
- a MAC address;
- a hostname;
- a route;
- a disk or device path;
- a process identifier;
- physical proximity;
- boot order;
- discovery order;
- first response;
- response speed.

**Why the invariant exists**

Locations and runtime identifiers are temporary, reusable, spoofable, and insufficient to establish logical continuity.

**Required observable behavior**

A conforming implementation MUST:

- treat location and order as observations rather than identity;
- use eligible identity evidence through the applicable provider;
- preserve conflicts where multiple claimants present similar location evidence;
- allow physical placement to change without automatically changing logical identity.

**Prohibited interpretations**

This invariant forbids:

- identifying a participant by “the machine at this address”;
- assigning authority to the first responder;
- restoring identity because the same disk path reappeared;
- using hostname continuity as sufficient revalidation.

**Failure or uncertainty behavior**

When eligible identity evidence is absent or conflicting, identity MUST remain unknown, unavailable, provisional, invalid, or conflicting as later defined.

**Primary later specification**

BOOT-0003 — Identity, Trust, and Authority Boundary.

---

### BOOT-INV-007 — Persisted, enrollment, and rooted identity evidence require scoped validation

**Binding rule**

Persisted identity, enrollment tokens, manufacturer-provisioned evidence, and hardware-rooted evidence MUST be validated for the current scope and lifecycle before BOOT relies upon them.

None automatically grants recovery authority.

**Why the invariant exists**

Evidence can expire, be revoked, become stale, be copied, be presented in the wrong scope, or remain valid for identity while being insufficient for authority.

**Required observable behavior**

A conforming implementation MUST:

- distinguish identity from the evidence supporting it;
- distinguish enrollment evidence from enrolled identity;
- evaluate persisted material for current applicability;
- preserve provider, freshness, revocation, and conflict results;
- evaluate recovery authority separately from identity.

**Prohibited interpretations**

This invariant forbids:

- treating an enrollment token as participant identity;
- treating possession of persisted credentials as current validity;
- treating hardware-backed identity as universal authority;
- treating a manufacturer root as automatic installer or propagation permission.

**Failure or uncertainty behavior**

Unavailable revalidation MUST NOT be interpreted as successful revalidation. A previously valid identity MAY remain historically known while current reliance is deferred or prohibited.

**Primary later specification**

BOOT-0003 — Identity, Trust, and Authority Boundary.

---

### BOOT-INV-008 — BOOT cannot self-grant or enlarge bootstrap authority

**Binding rule**

BOOT MUST NOT create, appoint, broaden, renew, or restore bootstrap authority through self-report, local control, urgency, repeated requests, successful history, or technical ability.

**Why the invariant exists**

BOOT coordinates highly privileged operations. Self-granted authority would eliminate the boundary intended to govern those operations.

**Required observable behavior**

A conforming implementation MUST:

- identify the authority source and scope;
- preserve authority revision, lifecycle, freshness, and revocation state;
- verify that the authority applies to the actor, target, action, and current operation;
- represent authority conflict or absence explicitly;
- reject scope expansion not supported by a new eligible decision.

**Prohibited interpretations**

This invariant forbids:

- BOOT selecting itself as authority;
- a recovery requester appointing its own approver;
- emergency status renewing expired authority;
- successful prior recovery creating permanent authority;
- combining several narrow grants into an undeclared universal grant.

**Failure or uncertainty behavior**

Conflicting, unavailable, stale, revoked, or indeterminate authority MUST result in a conservative scoped outcome such as deferred, denied, review-required, or locked down as later defined.

**Primary later specification**

BOOT-0003 — Identity, Trust, and Authority Boundary.

---

### BOOT-INV-009 — BOOT state remains multidimensional

**Binding rule**

BOOT MUST represent independent state dimensions rather than one oversized global boot or recovery state.

At minimum, the architecture preserves separate dimensions for:

- local boot condition;
- assistance-session condition;
- authority condition;
- artifact condition;
- installer condition.

Additional dimensions MAY be defined by BOOT-0002.

**Why the invariant exists**

Different parts of a recovery process progress and fail independently. One state value cannot truthfully represent all of them.

**Required observable behavior**

A conforming implementation MUST:

- retain each applicable dimension independently;
- identify the subject and scope of each state;
- allow dimensions to advance, regress, fail, or remain unknown independently;
- expose the underlying dimensional states when producing summaries.

**Prohibited interpretations**

This invariant forbids:

- one `BOOT_OK` value concealing unknown authority;
- one `RECOVERY_FAILED` value erasing a verified artifact;
- one `INSTALL_COMPLETE` value claiming runtime readiness;
- one `LOCKED_DOWN` value being treated as every dimension’s state.

**Failure or uncertainty behavior**

Failure in one dimension MUST NOT fabricate, overwrite, or silently resolve state in another dimension.

**Primary later specification**

BOOT-0002 — State and Failure Model.

---

### BOOT-INV-010 — Unknown, absence, unavailability, and invalidity remain truthful

**Binding rule**

Unknown, unavailable, invalid, deferred, failed, locked-down, partial, stale, conflicting, and unsupported conditions MUST remain explicit where applicable.

Absence of evidence MUST NOT become evidence of absence.

**Why the invariant exists**

Incomplete systems frequently lack observations or providers. Converting missing evidence into a favorable or definitive state creates false confidence.

**Required observable behavior**

A conforming implementation MUST:

- distinguish no observation from a negative observation;
- distinguish unavailable from invalid;
- distinguish unsupported from failed;
- distinguish deferred from denied;
- identify the scope and evidence boundary of a state;
- avoid default values that imply successful evaluation.

**Prohibited interpretations**

Unknown MUST NOT silently become:

- healthy;
- acceptable for production;
- authenticated;
- authorized;
- absent;
- compatible;
- current;
- verified;
- installed;
- active;
- recovered;
- successful.

**Failure or uncertainty behavior**

If a required condition cannot be established, BOOT MUST select only a path permitted for that unresolved condition or must defer, request review, refuse progression, or enter an authorized locked-down disposition.

**Primary later specification**

BOOT-0002 — State and Failure Model.

---

### BOOT-INV-011 — Failure, silence, liveness, launch, and restart do not fabricate completion

**Binding rule**

Silence, process launch, process liveness, process exit, connection closure, reboot, or restart MUST NOT independently establish semantic completion.

Restart MUST NOT erase unresolved identity, authority, operation, mutation, rollback, or recovery uncertainty.

**Why the invariant exists**

Execution events are implementation observations, not domain completion evidence.

**Required observable behavior**

A conforming implementation MUST:

- track semantic operation identity separately from process identity;
- preserve unresolved operation records across restart where technically possible and authorized;
- require explicit provider results for provider-owned completion;
- distinguish “no response” from success;
- reconcile prior operations before claiming a clean new attempt.

**Prohibited interpretations**

This invariant forbids:

- treating installer process exit code alone as verified installation;
- treating a reboot as successful activation;
- treating a heartbeat as complete recovery;
- treating a quiet assistance source as approval;
- clearing an unknown mutation merely because BOOT restarted.

**Failure or uncertainty behavior**

When completion cannot be established, the result MUST remain unknown, partial, interrupted, failed, or reconciliation-required as later defined.

**Primary later specification**

BOOT-0002 — State and Failure Model, with interruption refinement in BOOT-0008.

---

### BOOT-INV-012 — BOOT data, parsing, and work remain bounded

**Binding rule**

BOOT MUST apply explicit finite limits to all externally influenced and retained work, including:

- identifiers;
- records;
- strings;
- buffers;
- queues;
- retries;
- diagnostics;
- issue lists;
- plan depth;
- dependency depth;
- artifact lists;
- assistance offers;
- retained session history;
- recursion;
- parsing work;
- transformation output.

Arithmetic affecting sizes, offsets, counts, capacities, and resource estimates MUST be checked.

**Why the invariant exists**

BOOT may parse untrusted, corrupt, partial, or adversarial material under severe resource constraints.

**Required observable behavior**

A conforming implementation MUST:

- declare or derive enforceable limits;
- reject or defer oversized input before unbounded work;
- use bounded parsing;
- avoid unbounded recursion;
- detect arithmetic overflow and underflow;
- produce deterministic resource-exhaustion outcomes;
- avoid hidden growth through diagnostics or retries.

**Prohibited interpretations**

This invariant forbids:

- unbounded manifest nesting;
- unlimited artifact references;
- unlimited retry;
- reading until memory exhaustion;
- recursive parsing without a depth bound;
- diagnostic accumulation without retention limits;
- integer wraparound being treated as a small valid size.

**Failure or uncertainty behavior**

Exceeding a limit MUST produce an explicit conservative outcome and MUST NOT partially commit authoritative state.

**Primary later specification**

BOOT-0006 — Minimal Runtime and Language Constraints.

---

### BOOT-INV-013 — Resource exhaustion is a first-class outcome

**Binding rule**

Insufficient memory, storage, working space, parser capacity, issue capacity, provider capacity, transport capacity, installer capacity, time budget, or retry budget MUST be represented explicitly.

Resource exhaustion MUST NOT be converted into success, ordinary invalid input, or uncontrolled retry.

**Why the invariant exists**

A minimal rescue environment may operate close to hard limits. Resource failure is expected architecture, not an exceptional implementation detail.

**Required observable behavior**

A conforming implementation MUST:

- distinguish resource exhaustion from semantic rejection;
- identify the exhausted resource class where publicly appropriate;
- preserve authoritative state;
- preserve applicable idempotency and revision information;
- stop or narrow work within declared limits;
- permit a later retry only through the applicable retry rules.

**Prohibited interpretations**

This invariant forbids:

- truncating a required result and reporting success;
- dropping issue records silently;
- retrying indefinitely;
- treating allocation failure as target invalidity;
- exceeding a hard resource ceiling because recovery is urgent.

**Failure or uncertainty behavior**

Resource exhaustion MUST return an explicit conservative result. If the complete required result cannot be represented, the authoritative operation MUST remain uncommitted.

**Primary later specification**

BOOT-0006 — Minimal Runtime and Language Constraints.

---

### BOOT-INV-014 — Authoritative state changes are conservatively atomic

**Binding rule**

BOOT MUST preserve the following rule:

> Complete all fallible construction, validation, bounded retention, and result preparation before committing authoritative state.

**Why the invariant exists**

A state change that commits before its required result, revision, or diagnostic record is constructible may leave BOOT unable to report what actually occurred.

**Required observable behavior**

Before an authoritative commit, a conforming implementation MUST complete all applicable:

- input validation;
- authority validation;
- revision validation;
- bounded object construction;
- checked arithmetic;
- issue retention;
- result-code preparation;
- idempotency preparation;
- diagnostic preparation required by the public contract;
- provider-result correlation.

The authoritative mutation MUST occur only after those fallible steps succeed.

**Prohibited interpretations**

This invariant forbids:

- committing a new disposition and then discovering the result cannot be encoded;
- advancing a revision before issue retention succeeds;
- reporting success after storage of required operation identity fails;
- partially updating several authoritative fields without rollback or atomic commit.

**Failure or uncertainty behavior**

Allocation, encoding, storage, diagnostic, or retention failure MUST:

- leave authoritative state unchanged;
- return an explicit conservative outcome;
- never report false success;
- preserve the prior revision;
- preserve unresolved uncertainty.

**Primary later specification**

BOOT-0002 — State and Failure Model, with interruption refinement in BOOT-0008.

---

### BOOT-INV-015 — Revision and idempotency survive retry and interruption

**Binding rule**

BOOT operation identity, revision, attempt identity, and idempotency information MUST remain sufficient to distinguish:

- a retry of the same operation;
- a new attempt;
- a superseding operation;
- duplicate delivery;
- an interrupted operation;
- an operation with unknown prior completion.

**Why the invariant exists**

Boot, installer, and update operations may be interrupted by process failure, power loss, reboot, transport loss, or provider failure.

**Required observable behavior**

A conforming implementation MUST:

- keep operation identity independent of one connection or process;
- preserve expected and resulting revisions where applicable;
- avoid silently treating duplicate delivery as a new mutation;
- reconcile unknown prior completion before repeating non-idempotent work;
- retain bounded continuation records where technically possible and authorized;
- expose revision conflict rather than silently overwriting newer state.

**Prohibited interpretations**

This invariant forbids:

- generating a new operation identity merely because transport reconnects;
- repeating disk mutation because an acknowledgment was lost;
- losing rollback lineage across reboot;
- treating duplicate requests as independent authority;
- ignoring a revision conflict during rescue.

**Failure or uncertainty behavior**

If idempotency or revision state required for a mutation is unavailable, BOOT MUST NOT perform an unsafe repetition. It MUST reconcile, defer, refuse, or require review.

**Primary later specification**

BOOT-0008 — Failure, Retry, and Operator Intervention.

---

### BOOT-INV-016 — Assistance-source responsiveness does not create preference or authority

**Binding rule**

The first, nearest, fastest, most responsive, or most available assistance source MUST NOT automatically become preferred, authoritative, authenticated, admitted, or eligible for privileged recovery work.

**Why the invariant exists**

Responsiveness is a transport and timing observation. It does not establish identity, correctness, authority, or suitability.

**Required observable behavior**

A conforming implementation MUST:

- retain candidate-source identity separately from location and timing;
- apply eligibility, authentication, authority, admission, capability, and policy requirements;
- preserve conflicting offers;
- avoid disclosing protected recovery information before applicable authorization;
- allow no eligible source to remain an explicit outcome.

**Prohibited interpretations**

This invariant forbids:

- first-responder authority;
- lowest-latency authority;
- proximity-based trust;
- preference based solely on stronger signal or local network placement;
- acceptance because only one source responded.

**Failure or uncertainty behavior**

If no source satisfies the applicable requirements, BOOT MUST report no eligible assistance, defer, use an authorized offline path, request operator review, or refuse progression.

**Primary later specification**

BOOT-0004 — Discovery and Assistance Negotiation.

---

### BOOT-INV-017 — Transport and secure-session success do not establish semantic acceptance

**Binding rule**

Transport reachability, connection establishment, byte transfer, retransmission success, transport acknowledgment, encryption, or secure-session establishment MUST NOT establish:

- ACS admission;
- BOOT-operation acceptance;
- recovery authority;
- plan acceptance;
- artifact verification;
- installer acceptance;
- recovery completion.

**Why the invariant exists**

Transport and security providers establish bounded communication properties, not domain-operation results.

**Required observable behavior**

A conforming implementation MUST:

- attribute transport and security results to their providers;
- preserve BOOT semantic request and result identity separately;
- require applicable BOOT, ACS, installer, and authority decisions;
- distinguish secure-session state from assistance-session state;
- avoid treating encrypted data as correct or authorized data.

**Prohibited interpretations**

This invariant forbids:

- treating TLS or another protected channel as recovery authorization;
- treating delivered bytes as accepted plan content;
- treating a successful download as artifact verification;
- treating transport closure as semantic rejection.

**Failure or uncertainty behavior**

Transport failure MAY make an operation unavailable or interrupted. It MUST NOT independently prove that the remote source is malicious, invalid, absent, or unauthorized.

**Primary later specification**

BOOT-0004 — Discovery and Assistance Negotiation.

---

### BOOT-INV-018 — Assistance sessions and repeated communications remain bounded and attributable

**Binding rule**

Each assistance session MUST be separately identified, bounded in scope and resources, and distinguishable from discovery, transport sessions, secure sessions, ACS relationships, recovery plans, and recovery authority.

Repeated delivery, retries, copied reports, or reconnects MUST NOT automatically create new authority or independent evidence.

**Why the invariant exists**

Assistance interaction can span several connections and attempts. Without stable separation, duplicates may be miscounted or replayed as new decisions.

**Required observable behavior**

A conforming implementation MUST:

- assign bounded assistance-session identity;
- preserve source and correlation lineage;
- enforce message, offer, attempt, and retained-history limits;
- detect duplicate or replayed semantic operations where applicable;
- distinguish a new session from a resumed or retried session;
- avoid counting correlated duplicates as independent corroboration.

**Prohibited interpretations**

This invariant forbids:

- one secure session equaling one assistance session by definition;
- reconnection creating new authority;
- repeated reports increasing authority through repetition;
- connection loss proving compromise;
- copied evidence becoming independent evidence.

**Failure or uncertainty behavior**

When session continuity cannot be established, BOOT MUST create an explicitly new session or report continuity as unresolved. It MUST NOT silently merge histories.

**Primary later specification**

BOOT-0004 — Discovery and Assistance Negotiation.

---

### BOOT-INV-019 — Recovery requests, plans, and validation do not create authority

**Binding rule**

A recovery request is not authorization.

A recovery plan is not authority.

Plan validation is not plan authorization.

**Why the invariant exists**

Plans and requests describe proposed work. They do not establish who may perform it or whether it may proceed.

**Required observable behavior**

A conforming implementation MUST:

- retain request, plan, validation, authorization, and execution results separately;
- identify plan identity and revision;
- identify required authority;
- validate plan structure and bounds before mutation;
- require operation-specific authorization after validation;
- preserve denial, deferral, narrowing, and conflict.

**Prohibited interpretations**

This invariant forbids:

- executing a plan because it is well formed;
- treating an authenticated plan author as an authority;
- treating IMM recovery recommendation as authorization;
- treating operator interest as unrestricted plan approval;
- treating local plan generation as permission to mutate.

**Failure or uncertainty behavior**

A valid plan with unavailable authority MUST remain validated but unauthorized. A malformed or unsupported plan MUST NOT advance to mutation.

**Primary later specification**

BOOT-0005 — Recovery Plans and Artifact Boundaries.

---

### BOOT-INV-020 — Source, artifact, transfer, verification, installation, activation, and recovery remain distinct

**Binding rule**

BOOT MUST preserve the following distinctions:

- an artifact reference is not the artifact;
- source presence is not permission to compile;
- compilation is not artifact verification;
- artifact presence is not installation eligibility;
- transfer completion is not verification;
- verification is not compatibility;
- compatibility is not installation authority;
- installation is not activation;
- activation is not recovery;
- recovery is not successful normal-runtime operation.

**Why the invariant exists**

Source-bearing systems contain many materials that may be present, cached, compiled, or verified without being eligible for use.

**Required observable behavior**

A conforming implementation MUST:

- track source, artifact, transfer, verification, compatibility, installation, activation, and recovery results separately;
- preserve source and artifact identity and revision;
- attribute cryptographic results to the security provider;
- require compatibility evaluation for the intended target and assembly;
- require authority for compilation, installation, activation, execution, and propagation where applicable.

**Prohibited interpretations**

This invariant forbids:

- compiling merely because source is local;
- executing a successfully compiled binary without promotion and authority;
- installing a verified artifact automatically;
- treating a cache entry as release truth;
- treating source revision identity as a security digest;
- treating activation as proof of recovery.

**Failure or uncertainty behavior**

A failure at one stage MUST preserve successful results from earlier stages within their original scope. It MUST NOT promote those results into later-stage success.

**Primary later specification**

BOOT-0005 — Recovery Plans and Artifact Boundaries.

---

### BOOT-INV-021 — Installer mutation, rollback, and reconciliation remain installer-governed and explicit

**Binding rule**

The installer owns durable mutation, write verification, activation staging, and rollback mechanics.

Installer request delivery, process launch, progress output, partial writes, or process exit MUST NOT establish installer success.

Mutation, verification, activation staging, rollback, and rollback verification MUST remain separate.

**Why the invariant exists**

Installation may partially mutate durable state before failing or losing communication.

**Required observable behavior**

A conforming implementation MUST:

- hand the installer a bounded, authorized, revisioned request;
- preserve installer operation identity;
- receive explicit installer results;
- represent no mutation, partial mutation, completed mutation, rollback attempted, rollback completed, rollback failed, and unknown completion separately as later defined;
- reconcile unknown prior mutation before unsafe repetition;
- avoid claiming recovery from installer progress alone.

**Prohibited interpretations**

This invariant forbids:

- BOOT directly applying durable writes as an installer substitute;
- treating a created file as complete installation;
- treating installer exit as verified write completion;
- repeating destructive mutation after an acknowledgment loss without reconciliation;
- treating rollback initiation as rollback completion.

**Failure or uncertainty behavior**

Partial or unknown installer mutation MUST remain explicit and MUST constrain later activation, retry, rollback, and operator decisions.

**Primary later specification**

BOOT-0005 — Recovery Plans and Artifact Boundaries, with retry and interruption refinement in BOOT-0008.

---

### BOOT-INV-022 — BOOT remains implementable in a minimal headless environment

**Binding rule**

BOOT architecture MUST remain implementable in an extremely minimal headless Linux environment.

BOOT MUST NOT architecturally depend on:

- a GUI;
- a display server;
- a desktop environment;
- a local human-readable console;
- the normal Node runtime;
- Julia;
- a dynamic-language runtime;
- unrestricted heap growth;
- unrestricted filesystem availability;
- continuous network access;
- one service manager;
- one container runtime;
- one package manager.

**Why the invariant exists**

BOOT must remain available when the installed system and ordinary runtime are absent, incomplete, or damaged.

**Required observable behavior**

A conforming design MUST:

- support bounded machine-readable results;
- support offline and provider-unavailable outcomes;
- avoid mandatory desktop or interactive setup paths;
- make optional dependencies explicit;
- preserve operation under reduced resources;
- permit architecture-specific adapters without making them architectural masters.

Diagnostic terminal output MAY exist.

Diagnostic terminal output is not required for correctness or conformance.

**Prohibited interpretations**

This invariant forbids:

- requiring systemd or another single service manager by architecture;
- requiring Python, Julia, or another dynamic runtime for minimal rescue;
- requiring a writable persistent root;
- requiring network access to report local boot state;
- requiring a monitor or keyboard.

**Failure or uncertainty behavior**

Unavailable optional facilities MUST be reported as unavailable or unsupported. Their absence MUST NOT be converted into a false successful observation.

**Primary later specification**

BOOT-0006 — Minimal Runtime and Language Constraints.

---

### BOOT-INV-023 — Public BOOT contracts remain language-neutral

**Binding rule**

Public BOOT contracts MUST remain implementable across eligible languages and MUST NOT expose one language’s allocator, exception, container, ownership, or runtime model as architecture.

**Why the invariant exists**

The minimal rescue language remains undecided, and BOOT contracts must remain usable by future C, restricted C++, static Rust, test doubles, and higher-level orchestration boundaries.

**Required observable behavior**

Public contracts SHOULD support:

- fixed-width categories;
- bounded identifiers;
- explicit lengths;
- caller-owned buffers where an ABI requires buffers;
- structured result codes;
- independent revisions;
- deterministic bounded issue lists;
- checked arithmetic;
- explicit resource-exhaustion outcomes;
- no exceptions crossing an ABI;
- no transferred allocator ownership;
- no raw private-key material.

**Prohibited interpretations**

This invariant forbids:

- exposing STL types across a C-compatible boundary;
- requiring Rust ownership types in the public architecture;
- requiring garbage collection;
- requiring language exceptions for control flow;
- declaring a C ABI before its contracts are reviewed;
- selecting the rescue language through preference alone.

**Failure or uncertainty behavior**

If a language cannot preserve the required boundedness, failure behavior, or ABI constraints, it is not eligible for the affected implementation profile until evidence establishes conformance.

**Primary later specification**

BOOT-0006 — Minimal Runtime and Language Constraints.

---

### BOOT-INV-024 — Image boot and hardware discovery do not authorize participation or installation

**Binding rule**

BOOT MAY operate from a source-bearing Node installation or rescue image.

Successful image boot does not establish:

- image authenticity;
- image integrity;
- release authority;
- target compatibility;
- installation eligibility;
- recovery authority;
- runtime readiness.

Hardware discovery informs an assembly or recovery plan. It does not authorize installation.

A machine is an observed resource substrate, not a pre-authorized Node participant.

**Why the invariant exists**

The long-term Node distribution is intended to inspect diverse machines and assemble eligible local realizations. Discovery must not become automatic ownership of discovered hardware.

**Required observable behavior**

A conforming implementation MUST:

- distinguish the boot medium from the installed system;
- validate the applicable image or release evidence;
- report hardware observations with uncertainty;
- evaluate destructive target eligibility separately;
- establish identity and authority before enrollment or privileged installation;
- support no-compatible-assembly and review-required outcomes.

**Prohibited interpretations**

This invariant forbids:

- installing because installation media booted;
- treating detected storage as disposable;
- treating detected hardware as enrolled;
- treating a bootable image as authenticated;
- treating successful hardware probing as installation authorization;
- assigning Node identity to every machine that loads BOOT.

**Failure or uncertainty behavior**

Ambiguous storage, unsupported hardware, unverifiable image state, or unavailable authority MUST prevent claims of installation eligibility for the affected target.

**Primary later specification**

BOOT-0005 — Recovery Plans and Artifact Boundaries, with future OS and installer architecture refinement.

---

### BOOT-INV-025 — Dynamic self-assembly remains governed and source presence grants no execution authority

**Binding rule**

Dynamic local self-assembly MUST remain governed by identity, authority, compatibility, resource, installer, verification, and runtime boundaries.

Unattended operation does not mean ungoverned destructive mutation.

Presence of source, packages, toolchains, kernel versions, modules, or cached artifacts does not authorize:

- compilation;
- installation;
- activation;
- execution;
- enrollment;
- propagation.

**Why the invariant exists**

A source-bearing distribution is intentionally capable of adapting to observed hardware. That flexibility must not become arbitrary code execution or uncontrolled installation.

**Required observable behavior**

A conforming implementation MUST:

- derive an assembly plan from bounded evidence and eligible catalogs;
- distinguish required and optional components;
- validate source, toolchain, target, dependency, and artifact lineage;
- obtain applicable authority for each privileged stage;
- use installer-owned mutation;
- retain unused eligible source without automatically activating it;
- preserve omitted or unavailable optional capabilities explicitly.

**Prohibited interpretations**

This invariant forbids:

- compiling every present source tree automatically;
- executing a locally produced binary solely because compilation succeeded;
- deleting unused source merely because it is not selected;
- using unattended mode as authorization to erase ambiguous disks;
- treating a machine profile as permanently authoritative before inspection.

**Failure or uncertainty behavior**

An assembly whose required components cannot be established MUST NOT be reported as eligible for the affected activation. Optional-component failure MAY yield an explicitly degraded assembly if all required conditions are satisfied.

**Primary later specification**

BOOT-0005 — Recovery Plans and Artifact Boundaries, with future OS dynamic-assembly architecture refinement.

---

### BOOT-INV-026 — Kernel/micro-OS pairs and required/optional components are validated as scoped assemblies

**Binding rule**

A selected kernel and micro-OS combination MUST be evaluated as an identified pair or generation.

The newest kernel, newest micro-OS, newest source revision, newest package, or newest artifact is not automatically the eligible production choice.

Required-component failure prevents claiming the affected assembly is ready.

Unsupported or unavailable optional components MAY remain omitted without falsifying the state of the supported assembly.

**Why the invariant exists**

Individually usable components may be incompatible when combined. Version recency does not prove pair compatibility.

**Required observable behavior**

A conforming implementation MUST:

- identify the selected pair or generation;
- preserve kernel, userspace, module, toolchain, dependency, and configuration lineage needed by the applicable contract;
- distinguish required from optional components;
- validate the complete required set;
- retain fallback relationships where applicable;
- report omitted optional capability explicitly;
- prevent pair-level promotion based solely on individual component tests.

**Prohibited interpretations**

This invariant forbids:

- automatically selecting the highest version number;
- rolling back only one component while retaining an incompatible counterpart;
- claiming full assembly readiness after a required component fails;
- treating an optional unavailable accelerator as total system absence;
- treating a cached kernel as an accepted pair.

**Failure or uncertainty behavior**

If pair compatibility is unknown, conflicting, unsupported, or invalid, the pair MUST NOT be reported as eligible for the affected activation. An older separately validated pair MAY remain eligible according to applicable authority and policy.

**Primary later specification**

BOOT-0005 for BOOT-facing artifact and plan requirements, with future OS and kernel-pair architecture refinement.

---

### BOOT-INV-027 — Update availability and repository state do not create release or activation authority

**Binding rule**

Repository availability is not release authority.

The tip of `main` or any development branch is not automatically a deployable release.

An update advertisement is not an accepted update.

Fetching is not verification.

Verification is not compatibility.

Compatibility is not activation authority.

**Why the invariant exists**

Development sources, release decisions, artifact verification, target compatibility, and activation are separately governed.

**Required observable behavior**

A conforming implementation MUST:

- identify the proposed release or update revision;
- preserve the authority source for release consideration;
- evaluate integrity and authority separately from local compatibility;
- stage updates separately from active operation where required;
- perform local validation before activation;
- preserve rollback references;
- distinguish deprecated, revoked, superseded, unsupported, and merely older material.

**Prohibited interpretations**

This invariant forbids:

- deploying whatever `main` contains;
- activating an update because a repository is reachable;
- treating a signed update as locally compatible;
- treating local compatibility as permission to activate;
- treating newer material as automatically preferred;
- treating superseded material as cryptographically invalid merely because it is older.

**Failure or uncertainty behavior**

If release authority, integrity, compatibility, staging, local validation, or activation authority is unavailable or failed, broader activation MUST NOT be claimed.

**Primary later specification**

BOOT-0009 — Public/Private Boundary and Implementation Roadmap, with future update architecture refinement.

---

### BOOT-INV-028 — Peer-assisted propagation is bounded, attributable, locally validated, and reversible

**Binding rule**

One node’s possession of source or artifacts does not authorize another node to activate them.

Peer-assisted transfer does not make the peer the source of release authority.

Propagation MUST remain bounded, attributable, revisioned, authorized, locally validated, and reversible where technically possible.

**Why the invariant exists**

Peer transfer can improve availability and reduce external dependency while also creating a path for uncontrolled replication or fleet-wide failure.

**Required observable behavior**

A conforming implementation MUST:

- identify the release or artifact independently of the transferring peer;
- verify the receiving node’s applicable authority and eligibility;
- apply the same integrity and release-authority checks regardless of transfer source;
- enforce transfer and retention bounds;
- perform local compatibility and validation;
- stage activation;
- preserve rollback or refusal behavior;
- stop broader promotion after a failed canary or validation stage;
- prevent restricted material from reaching unauthorized peers.

**Prohibited interpretations**

This invariant forbids:

- peer possession creating release authority;
- successful transfer creating activation permission;
- one node’s compatibility result being universal;
- uncontrolled cascading installation;
- fleet-wide promotion continuing after a declared failed gate;
- copying private modules to an unauthorized peer.

**Failure or uncertainty behavior**

Failure at a canary, local-validation, staging, activation, or post-activation gate MUST remain scoped and MUST prevent silent broader promotion under the same failed basis.

**Primary later specification**

BOOT-0009 — Public/Private Boundary and Implementation Roadmap, with future peer-distribution and fleet-rollout architecture refinement.

---

### BOOT-INV-029 — Public and private BOOT material remain separated without making private capability mandatory

**Binding rule**

Public BOOT architecture MUST be independently authored and sufficient for independent implementation and conformance testing.

Public documentation MUST NOT be created by sanitizing restricted designs.

Public Node operation MUST remain possible without private modules.

Absence of a private module MUST be reported as unavailable, ineligible, or unsupported rather than disguised.

Private capability possession does not grant unrestricted BOOT authority.

Protected material MUST NOT propagate to unauthorized peers.

**Why the invariant exists**

Node may support private capabilities while retaining a public architecture and functional public distribution.

**Required observable behavior**

A conforming public implementation and distribution MUST:

- expose public interfaces and capability declarations without restricted implementation;
- represent optional private capability honestly;
- avoid embedding production credentials, keys, private source, private model material, or operator procedures;
- require separate identity, authority, and source eligibility for private material;
- protect restricted artifacts during storage and transfer;
- remain operational within its declared public capability set.

**Prohibited interpretations**

This invariant forbids:

- publishing a redacted private design as public architecture;
- requiring private modules for basic public BOOT operation;
- reporting a missing private module as a generic successful capability;
- granting universal recovery authority to a node that can access private source;
- embedding reusable credentials in a public image;
- transferring restricted material to every reachable node.

**Failure or uncertainty behavior**

When private capability is unavailable or unauthorized, BOOT MUST report the capability state explicitly and continue only within the eligible public or reduced capability set.

**Primary later specification**

BOOT-0009 — Public/Private Boundary and Implementation Roadmap.

---

### BOOT-INV-030 — Runtime handoff eligibility is not runtime readiness or broader authority

**Binding rule**

BOOT MAY establish that declared prerequisites for runtime handoff are satisfied.

The normal runtime owns its own initialization, admission, service readiness, degraded-operation declaration, and sustained operation.

Attempted launch is not readiness.

Runtime readiness does not retroactively rewrite BOOT results.

Successful runtime operation does not create MEM or IMM authority.

BOOT and runtime records MUST preserve enough identity, revision, and handoff lineage to reconcile the transition.

**Why the invariant exists**

BOOT and the runtime operate at different stages and own different completion claims.

**Required observable behavior**

A conforming implementation MUST:

- identify the selected target environment and generation;
- preserve the BOOT session, plan, installation, activation, and handoff references needed for reconciliation;
- distinguish handoff attempted, runtime loaded, runtime initialized, runtime ready, runtime degraded, and runtime failed as later defined;
- permit explicitly degraded runtime operation without claiming complete recovery;
- prevent runtime startup from altering historical BOOT evidence;
- require MEM and IMM operations to use their own authority.

**Prohibited interpretations**

This invariant forbids:

- reporting runtime readiness because a process launched;
- reporting complete recovery because a heartbeat appeared;
- allowing runtime success to validate an invalid installer result retroactively;
- granting memory custody because the runtime is operational;
- granting immune restoration authority because the runtime reports itself healthy;
- allowing the runtime to rewrite the independent rescue boundary.

**Failure or uncertainty behavior**

Runtime failure after an eligible BOOT handoff MUST remain a runtime-stage failure with preserved BOOT handoff evidence. BOOT MAY coordinate a new rescue decision without rewriting the prior handoff as something it was not.

**Primary later specification**

BOOT-0007 — ACS, IMM, and MEM Integration, with state refinement in BOOT-0002.

## 8. Cross-invariant interaction rules

### 8.1 The invariant set is conjunctive

A BOOT action conforms only when it satisfies every applicable invariant.

For example, a recovery plan may be:

- structurally valid under BOOT-INV-019;
- carried through a protected session under BOOT-INV-017;
- bounded under BOOT-INV-012;
- but still prohibited because authority is unavailable under BOOT-INV-008.

### 8.2 Stage evidence may cross boundaries without transferring ownership

A result from one stage MAY become evidence for another stage.

Examples:

- authentication evidence may support an authority decision;
- artifact verification evidence may support installation eligibility;
- installer results may support recovery verification;
- BOOT handoff evidence may support runtime reconciliation;
- runtime evidence may support a later BOOT rescue decision.

The receiving stage retains ownership of its own decision.

### 8.3 Stronger evidence does not replace authority

Higher-quality, hardware-rooted, repeated, independently corroborated, or locally available evidence does not itself create operation authority.

Evidence quality and authority remain separate.

### 8.4 Boundedness applies to every other invariant

No identity, authority, plan, artifact, diagnostic, retry, public/private, assembly, update, or handoff mechanism is exempt from finite limits.

### 8.5 Atomicity applies to every authoritative transition

Any transition that changes authoritative BOOT state, accepted plan revision, operation disposition, installer-handoff state, activation eligibility, or handoff eligibility is subject to BOOT-INV-014.

### 8.6 State summaries cannot erase dimensions

A summary MAY be generated for policy, diagnostics, or operator presentation.

The summary MUST NOT replace the underlying dimensional state required by BOOT-INV-009.

### 8.7 Self-assembly does not create a parallel authority system

Dynamic assembly, local compilation, hardware detection, package selection, and source retention MUST use the same identity, authority, boundedness, installer, verification, and handoff invariants as recovery from externally supplied artifacts.

### 8.8 Update flow is a specialized artifact and activation flow

Future update architecture may add release and rollout mechanisms.

It MUST preserve BOOT-INV-004, BOOT-INV-020, BOOT-INV-027, and BOOT-INV-028.

### 8.9 Degraded operation does not weaken required-component rules

An optional capability may be unavailable while the eligible base assembly operates in a declared degraded capability set.

A failed or unresolved required component prevents claiming the affected assembly or runtime scope is ready.

### 8.10 Conflict is not resolved by convenience

Conflicting identity, authority, plan, artifact, compatibility, installer, rollback, release, or runtime results MUST remain explicit until resolved through the owning architecture.

BOOT MUST NOT resolve conflict solely by selecting:

- the first result;
- the newest timestamp;
- the highest version;
- the most responsive source;
- the most permissive result;
- the most severe result;
- the most numerous correlated reports.

## 9. Failure, unknown, and resource-exhaustion behavior

### 9.1 Failure does not dissolve architecture

A conforming implementation MUST preserve these invariants during:

- storage corruption;
- memory exhaustion;
- partial writes;
- missing dependencies;
- provider failure;
- transport loss;
- reboot;
- power interruption;
- authority conflict;
- invalid artifacts;
- incompatible assemblies;
- update revocation;
- rollback failure;
- runtime startup failure.

### 9.2 Conservative failure outcome

When a required condition cannot be established, BOOT MUST:

- preserve authoritative state;
- preserve applicable operation identity and revision;
- report the unresolved condition;
- avoid broader mutation or authority;
- select only a path permitted under that condition.

### 9.3 Failure attribution

A failure report SHOULD identify, within the public contract:

- the failed operation;
- its stage;
- its subject;
- its scope;
- the reporting component;
- the applicable provider;
- the observed result;
- remaining uncertainty;
- whether authoritative state changed;
- whether reconciliation is required.

It MUST NOT expose protected policy, credentials, or private reasoning.

### 9.4 Unknown prior mutation

When BOOT cannot establish whether a non-idempotent installer, activation, rollback, or update operation completed, it MUST NOT repeat that operation merely because the expected result record is missing.

Reconciliation, review, or an explicitly permitted recovery path is required.

### 9.5 Diagnostic failure

Failure to construct or retain a required public diagnostic or result record MUST prevent false success.

Optional human-readable logging MAY fail without changing the domain result only when:

- the public machine-readable result is already complete;
- authoritative state is already validly committed;
- the optional output is not required for audit, reconciliation, or conformance.

### 9.6 Locked-down behavior

Locked-down state MUST remain:

- scoped;
- attributable;
- distinguishable from compromise;
- distinguishable from permanent retirement;
- compatible with required operator, evidence, audit, physical-safety, and recovery paths unless a separately authorized replacement exists.

### 9.7 No unbounded recovery loops

Repeated failure MUST NOT create unlimited automatic retries.

Retry limits, backoff, continuation, reconciliation, and operator intervention are refined by BOOT-0008.

## 10. Conformance expectations

### 10.1 Conformance categories

| Category | Meaning | What it does not establish |
|---|---|---|
| Architectural conformance | The design preserves BOOT-0000 and every applicable BOOT-0001 invariant | That a particular implementation is correct |
| Implementation conformance | Observable implementation behavior satisfies the applicable public contracts and tests | That one deployment’s private policy is appropriate |
| Deployment policy | A deployment selects authority sources, eligible releases, resource limits, responder policy, and operational profiles within the public architecture | Permission to violate public invariants |
| Private operational policy | Protected production rules select among publicly eligible actions without disclosure | A separate architecture or exemption from conformance |

### 10.2 Required conformance principle

A deployment MUST NOT claim conformance merely because its happy path boots, installs, updates, or launches a runtime.

Conformance evidence MUST include applicable negative and failure cases.

### 10.3 Acceptable conformance evidence

Conformance evidence MAY include:

- deterministic unit tests;
- state-transition tests;
- invariant-specific negative tests;
- failure injection;
- bounded-resource tests;
- allocation-failure tests;
- checked-arithmetic tests;
- malformed-input tests;
- depth- and count-limit tests;
- interrupted-operation tests;
- reboot-continuation tests;
- idempotency tests;
- revision-conflict tests;
- negative-authority tests;
- stale-credential tests;
- unavailable-provider tests;
- conflicting-assistance tests;
- duplicate-delivery tests;
- QEMU headless boot tests;
- installer mock tests;
- installer partial-write tests;
- source-validation tests;
- artifact-transfer and verification-separation tests;
- compatibility tests;
- kernel/micro-OS pair tests;
- required-versus-optional component tests;
- rollback tests;
- update-refusal tests;
- peer-propagation refusal tests;
- canary-failure containment tests;
- runtime-handoff reconciliation tests;
- public/private disclosure tests.

### 10.4 Invariant-specific evidence

Each implementation claim SHOULD identify:

- the invariant identifier;
- the implementation surface;
- the test or inspection evidence;
- the applicable platform and environment;
- the resource limits used;
- known unsupported cases;
- unresolved uncertainty;
- whether evidence applies to x86_64, AArch64, or another target.

### 10.5 BOOT-P0 use

BOOT-P0 may demonstrate a small subset of the final architecture.

It MUST nevertheless preserve all invariants applicable to its scope.

For example, BOOT-P0 does not need full installation or peer propagation, but it MUST:

- remain headless;
- remain bounded;
- distinguish launch from completion;
- produce truthful structured outcomes;
- avoid claiming unsupported authority;
- preserve generated-artifact separation;
- avoid hard-coding one hardware profile as the architecture.

### 10.6 Non-conformance

An implementation is non-conforming if it:

- violates any applicable mandatory invariant;
- conceals unknown or partial state;
- claims authority from execution context;
- performs installer mutation inside BOOT;
- treats transfer as verification;
- reports runtime launch as recovery;
- allows unbounded input or retry;
- partially commits authoritative state after fallible construction;
- activates repository contents without release authority;
- propagates protected material to unauthorized peers;
- requires unavailable private modules for declared public BOOT operation.

## 11. Relationship to later BOOT specifications

BOOT-0001 defines cross-cutting invariants only.

Detailed mechanisms remain assigned as follows.

### BOOT-0002 — State and Failure Model

BOOT-0002 defines:

- complete state dimensions;
- state vocabularies;
- state subjects and scopes;
- transition rules;
- summary-state rules;
- partial, conflicting, interrupted, and indeterminate outcomes;
- authoritative transition behavior.

It refines BOOT-INV-004, BOOT-INV-009, BOOT-INV-010, BOOT-INV-011, BOOT-INV-014, and BOOT-INV-030.

### BOOT-0003 — Identity, Trust, and Authority Boundary

BOOT-0003 defines:

- identity profiles;
- identity evidence;
- enrollment evidence;
- persisted identity;
- revalidation;
- authority anchors;
- authentication results;
- authority scope;
- conflict;
- revocation;
- unavailable-authority behavior.

It refines BOOT-INV-002 and BOOT-INV-005 through BOOT-INV-008.

### BOOT-0004 — Discovery and Assistance Negotiation

BOOT-0004 defines:

- discovery;
- assistance offers;
- candidate sources;
- assistance-session lifecycle;
- negotiation;
- responder eligibility;
- duplicate and replay behavior;
- transport-neutral semantic operations.

It refines BOOT-INV-016 through BOOT-INV-018.

### BOOT-0005 — Recovery Plans and Artifact Boundaries

BOOT-0005 defines:

- recovery-request identity;
- recovery-plan identity and revision;
- plan validation;
- artifact references;
- source and artifact boundaries;
- transfer results;
- verification results;
- compatibility evidence;
- installer inputs;
- installer results;
- assembly-plan inputs;
- applicable image and pair records.

It refines BOOT-INV-019 through BOOT-INV-021 and BOOT-INV-024 through BOOT-INV-026.

### BOOT-0006 — Minimal Runtime and Language Constraints

BOOT-0006 defines:

- measurable binary and dependency requirements;
- startup assumptions;
- allocator constraints;
- exception or panic requirements;
- initramfs suitability;
- parsing requirements;
- stack and heap limits;
- cross-compilation evidence;
- x86_64 and AArch64 validation;
- language-selection evidence.

It refines BOOT-INV-012, BOOT-INV-013, BOOT-INV-022, and BOOT-INV-023.

### BOOT-0007 — ACS, IMM, and MEM Integration

BOOT-0007 defines:

- detailed ACS mapping;
- detailed IMM request and evidence mapping;
- detailed MEM persistence mapping;
- operation/result translation;
- authority-reference handling;
- runtime-handoff integration.

It refines BOOT-INV-001, BOOT-INV-003, and BOOT-INV-030.

### BOOT-0008 — Failure, Retry, and Operator Intervention

BOOT-0008 defines:

- retry;
- interruption;
- reconciliation;
- lock-down;
- reboot continuation;
- unknown prior mutation;
- retry exhaustion;
- operator review and intervention boundaries.

It refines BOOT-INV-011, BOOT-INV-014, BOOT-INV-015, and BOOT-INV-021.

### BOOT-0009 — Public/Private Boundary and Implementation Roadmap

BOOT-0009 defines:

- detailed publication rules;
- public conformance stages;
- implementation checkpoints;
- repository re-track requirements;
- public/private module boundaries;
- update and propagation handoff to future architecture;
- series roadmap.

It refines BOOT-INV-027 through BOOT-INV-029.

## 12. Public/private classification boundary

### 12.1 Public material

Public BOOT architecture MAY define:

- invariant identifiers;
- mandatory distinctions;
- ownership boundaries;
- boundedness requirements;
- public state categories;
- public failure outcomes;
- authority-reference requirements;
- public conformance evidence;
- source and artifact separation;
- runtime-handoff requirements;
- update and propagation invariants;
- required public/private capability behavior.

### 12.2 Excluded private material

Public BOOT architecture MUST NOT disclose:

- production credentials;
- private keys;
- production trust anchors;
- recovery secrets;
- infrastructure addresses;
- public or private repository addresses used in production;
- private signing policy;
- private enrollment policy;
- private recovery-authorization policy;
- responder-selection algorithms;
- private preference weights;
- anomaly thresholds;
- behavioral fingerprints;
- private hardware topology;
- protected capability catalogs;
- hidden scores;
- private reasoning traces;
- operator playbooks;
- production concurrence rules;
- private release-channel policy;
- fleet rollout algorithms;
- deployment-specific fallback ordering;
- private module source or model material.

### 12.3 Independent public conformance

Public contracts MUST remain sufficient for an independent implementation to:

- represent the required states and outcomes;
- enforce the invariant boundaries;
- build conformance tests;
- implement provider mocks;
- demonstrate refusal behavior;
- operate without private modules.

Private implementation freedom MUST NOT weaken public behavior.

### 12.4 Restricted-material containment

A BOOT implementation handling restricted material MUST preserve:

- identity;
- classification;
- origin;
- authority;
- scope;
- retention;
- storage eligibility;
- transfer eligibility;
- recipient eligibility;
- revocation and supersession state.

The detailed mechanism remains outside this public specification.

## 13. Open architectural decisions

The following remain intentionally unresolved:

- complete BOOT state vocabularies;
- state-transition tables;
- BOOT identity-profile values;
- authority-anchor records;
- authentication result contracts;
- assistance offer and negotiation records;
- public responder eligibility;
- recovery-plan structures;
- artifact-manifest structures;
- installer request and result structures;
- exact bounded identifier lengths;
- exact plan and artifact count limits;
- exact issue-list limits;
- exact retry budgets;
- exact stack and heap budgets;
- strict C, restricted C++, or static Rust selection;
- exact initramfs profile;
- exact binary-size targets;
- exact x86_64 and AArch64 conformance profiles;
- filesystem layout;
- bootloader;
- package system;
- source-store implementation;
- toolchain-store implementation;
- kernel import and maintenance strategy;
- kernel/micro-OS pair-record schema;
- release-manifest schema;
- release-channel names;
- update-check schedule;
- update transport;
- fleet rollout policy;
- rollback implementation;
- operator-intervention interface;
- production authority assignments.

No unresolved decision authorizes an implementation to choose a weaker invariant.

## 14. Initial implementation implications

These implications constrain future work but do not constitute an implementation work order.

### 14.1 BOOT-I001 values and state

The first BOOT value layer will need:

- stable bounded identities;
- independent revisions;
- independent state dimensions;
- structured result codes;
- bounded issue records;
- explicit resource-exhaustion outcomes;
- expected-revision transitions;
- no-partial-commit behavior;
- language-neutral ownership.

### 14.2 BOOT-P0 seed

The BOOT-P0 seed should demonstrate:

- headless startup;
- deterministic bounded stage records;
- explicit launch and completion distinction;
- timeout behavior;
- structured final disposition;
- generated-artifact separation;
- no network dependency;
- no private-module dependency;
- no claim of installation or full recovery.

The seed should remain compatible with the long-term source-bearing distribution direction without implementing full local compilation or installation.

### 14.3 Provider mocks

Early implementation may use mock:

- security providers;
- transport adapters;
- installers;
- persistence adapters;
- assistance sources;
- runtime handoff targets.

Mocks MUST preserve the same authority, state, failure, and result separations as later providers.

### 14.4 Dynamic assembly

A future assembly planner must treat:

- hardware observations as evidence;
- catalogs as candidate material;
- source as non-executable material until authorized;
- required and optional components separately;
- the resulting generation as a revisioned assembly;
- installer mutation and activation as separate operations.

### 14.5 Update preparation

Future update work must begin with:

- release identity;
- release authority;
- local compatibility;
- staging;
- validation;
- activation authority;
- rollback lineage;
- propagation limits.

A repository pull or peer transfer is insufficient.

### 14.6 Testing

Implementation acceptance should prioritize negative and interrupted paths before broad feature growth.

A successful ordinary boot is not sufficient evidence for BOOT-P0 or BOOT-I001 conformance.

## 15. Invariant registry

The canonical invariant registry and BOOT-0000 traceability registry appear at the end of this document after the revision history.

The registries are normative indexes.

The full invariant text in section 7 governs when a registry summary is incomplete.

## 16. Prohibited interpretations

This specification MUST NOT be interpreted to mean that:

- BOOT owns every operation that occurs before the normal runtime;
- BOOT may become a universal recovery administrator;
- local execution creates authority;
- installation media may erase any detected disk;
- unattended installation means unreviewed destructive mutation;
- rooted identity creates universal authority;
- an enrollment token is identity;
- repeated requests create authority;
- a secure session creates ACS admission;
- assistance responsiveness creates preference or authority;
- plan validation authorizes a plan;
- source presence authorizes compilation;
- compilation authorizes execution;
- artifact verification authorizes installation;
- installation proves activation;
- activation proves recovery;
- recovery proves sustained runtime operation;
- successful image boot proves image authenticity;
- hardware discovery enrolls a machine;
- the newest kernel or package is automatically eligible;
- optional capability absence makes every assembly invalid;
- required-component failure may be hidden as degraded success;
- `main` is a production release channel by definition;
- peer possession creates release authority;
- canary failure may be ignored during broader rollout;
- private modules are required for public BOOT conformance;
- private capability creates unrestricted BOOT authority;
- the runtime may rewrite BOOT history;
- runtime success creates MEM custody or IMM restoration authority;
- private policy may bypass public invariants;
- BOOT-0001 defines the complete operating system, installer, update system, or kernel strategy.

## 17. Completion checklist

Draft review confirms:

- [x] BOOT-0000 authority is preserved.
- [x] Every initial BOOT-0000 commitment is mapped.
- [x] Invariants have stable identifiers.
- [x] Unknown and failure remain explicit.
- [x] Conservative atomicity is preserved.
- [x] Adjacent authority boundaries are preserved.
- [x] Self-assembly does not create unrestricted authority.
- [x] Source presence does not create execution authority.
- [x] Update availability does not create activation authority.
- [x] Peer transfer does not create release authority.
- [x] Minimal headless operation remains mandatory.
- [x] Language neutrality remains mandatory.
- [x] No private policy or topology is exposed.
- [x] Public operation without private modules is preserved.
- [x] Later BOOT documents retain their intended scopes.
- [x] No concrete operating system, filesystem, protocol, language, package manager, bootloader, or kernel choice is mandated.
- [x] Public contracts remain independently implementable and testable.
- [x] No blocking contradiction with BOOT-0000 was identified.

## 18. Closing principle

> **BOOT conforms only when it preserves the same boundaries during damage, urgency, autonomy, and success that it preserves during an ordinary boot.**

## Revision history

### Version 0.1 — 2026-07-17

- Established thirty canonical BOOT invariants.
- Mapped every BOOT-0000 durable commitment.
- Formalized ownership, authority, state-truthfulness, boundedness, and atomicity requirements.
- Formalized assistance, recovery-plan, artifact, installer, and runtime-handoff separations.
- Added source-bearing distribution and dynamic self-assembly implications without defining the complete operating system.
- Added kernel/micro-OS pair and required/optional component invariants.
- Added governed update and peer-propagation invariants.
- Preserved public/private separation and public operation without private modules.
- Established conformance categories and evidence expectations.
- Deferred detailed mechanisms to BOOT-0002 through BOOT-0009 and future OS, kernel, installer, and update architecture.

## Canonical invariant registry

| Invariant ID | Short name | One-sentence binding rule | Primary concern | Refining specification |
|---|---|---|---|---|
| BOOT-INV-001 | Coordination without absorption | BOOT coordinates cross-domain work without taking ownership of adjacent semantics or authority. | Architectural ownership | BOOT-0007 |
| BOOT-INV-002 | Execution context grants no authority | Early, local, rescue, offline, unattended, or emergency execution does not enlarge authority. | Authority separation | BOOT-0003 |
| BOOT-INV-003 | Adjacent ownership preserved | Installer, transport, security, ACS, MEM, IMM, resource management, and runtime responsibilities remain with their owners. | Architectural ownership | BOOT-0007 |
| BOOT-INV-004 | Stage separation | No trust or completion stage silently proves a later stage. | Completion truthfulness | BOOT-0002; BOOT-0005 |
| BOOT-INV-005 | Session identity separation | A BOOT session identity is not durable participant identity. | Identity | BOOT-0003 |
| BOOT-INV-006 | Identity is not location | Durable identity is not derived solely from location, order, proximity, or responsiveness. | Identity | BOOT-0003 |
| BOOT-INV-007 | Identity evidence revalidation | Persisted, enrollment, and rooted evidence require current scoped validation and grant no automatic recovery authority. | Identity and evidence | BOOT-0003 |
| BOOT-INV-008 | No self-granted bootstrap authority | BOOT cannot create or enlarge its own authority, and authority conflict remains explicit. | Authority | BOOT-0003 |
| BOOT-INV-009 | Multidimensional state | BOOT state remains independently represented across required dimensions. | State model | BOOT-0002 |
| BOOT-INV-010 | Unknown remains explicit | Unknown, absence, unavailability, invalidity, and conflict are not silently converted into favorable or definitive states. | State truthfulness | BOOT-0002 |
| BOOT-INV-011 | Execution events are not completion | Silence, liveness, launch, exit, reboot, and restart do not establish semantic completion or erase uncertainty. | Failure truthfulness | BOOT-0002; BOOT-0008 |
| BOOT-INV-012 | Bounded data and work | All externally influenced and retained BOOT data, parsing, retries, and work remain explicitly bounded. | Boundedness | BOOT-0006 |
| BOOT-INV-013 | Explicit resource exhaustion | Resource exhaustion is a first-class conservative outcome. | Resource failure | BOOT-0006 |
| BOOT-INV-014 | Conservative atomicity | All fallible construction and result preparation completes before authoritative state commits. | Atomicity | BOOT-0002; BOOT-0008 |
| BOOT-INV-015 | Revision and idempotency continuity | Operation identity, revision, and idempotency remain sufficient across retry, duplicate delivery, interruption, and reboot. | Retry and reconciliation | BOOT-0008 |
| BOOT-INV-016 | Responsiveness is not authority | Assistance-source timing or proximity does not create preference, identity, eligibility, or authority. | Assistance | BOOT-0004 |
| BOOT-INV-017 | Transport is not semantic acceptance | Transport and secure-session success do not establish admission, authority, or BOOT-operation completion. | Transport separation | BOOT-0004 |
| BOOT-INV-018 | Bounded assistance sessions | Assistance sessions remain separately identified and bounded, and repeated communications create neither new authority nor independent evidence. | Assistance lifecycle | BOOT-0004 |
| BOOT-INV-019 | Plans do not create authority | Recovery requests, plans, and plan validation remain separate from authorization. | Recovery planning | BOOT-0005 |
| BOOT-INV-020 | Source-to-recovery separation | Source, artifact, transfer, verification, compatibility, installation, activation, recovery, and runtime success remain distinct. | Artifact and completion boundaries | BOOT-0005 |
| BOOT-INV-021 | Installer mutation separation | Installer mutation, rollback, verification, and reconciliation remain installer-governed and explicitly reported. | Installer boundary | BOOT-0005; BOOT-0008 |
| BOOT-INV-022 | Minimal headless operation | BOOT remains implementable without desktop, normal runtime, dynamic-language, continuous-network, or unrestricted-resource dependencies. | Minimal runtime | BOOT-0006 |
| BOOT-INV-023 | Language-neutral contracts | Public BOOT contracts do not expose one language’s allocator, exception, container, or ownership model as architecture. | Language neutrality | BOOT-0006 |
| BOOT-INV-024 | Image and discovery grant no authority | Booting an image and discovering hardware do not authorize installation, enrollment, or participation. | Distribution and installation | BOOT-0005; future OS architecture |
| BOOT-INV-025 | Governed dynamic self-assembly | Self-assembly remains bounded and authorized, and source or toolchain presence grants no execution authority. | Dynamic assembly | BOOT-0005; future OS architecture |
| BOOT-INV-026 | Pair and component validation | Kernel/micro-OS pairs and required component sets require scoped validation, while optional absence remains explicit. | System-generation compatibility | BOOT-0005; future OS and kernel architecture |
| BOOT-INV-027 | Repository and update availability grant no authority | Repository state, update advertisement, fetching, verification, and compatibility do not independently authorize activation. | Update governance | BOOT-0009; future update architecture |
| BOOT-INV-028 | Governed peer propagation | Peer-assisted propagation remains attributable, bounded, authorized, locally validated, staged, and reversible. | Propagation | BOOT-0009; future update architecture |
| BOOT-INV-029 | Public/private separation | Public BOOT remains independently implementable without private modules while protected material remains restricted. | Classification and publication | BOOT-0009 |
| BOOT-INV-030 | Handoff is not readiness | BOOT establishes handoff eligibility, while the runtime owns readiness, degraded operation, and sustained execution without gaining MEM or IMM authority. | Runtime handoff | BOOT-0007; BOOT-0002 |

## BOOT-0000 traceability registry

| BOOT-0000 commitment number | Commitment summary | Mapped invariant IDs | Clarification introduced | Later refining specification | Coverage status |
|---:|---|---|---|---|---|
| 1 | BOOT is a minimal coordination layer, not a replacement for adjacent systems. | BOOT-INV-001, BOOT-INV-003 | Enumerates the retained installer, transport, security, ACS, MEM, IMM, resource, and runtime ownership boundaries. | BOOT-0007 | covered with clarification |
| 2 | BOOT remains operable before or without the normal runtime. | BOOT-INV-002, BOOT-INV-022, BOOT-INV-030 | Defines minimal headless dependency exclusions and preserves the runtime’s later readiness ownership. | BOOT-0006, BOOT-0007 | covered with clarification |
| 3 | Unknown and unavailable conditions remain explicit. | BOOT-INV-009, BOOT-INV-010, BOOT-INV-011, BOOT-INV-013 | Extends truthfulness to absence, invalidity, silence, liveness, restart, resource exhaustion, and cross-dimensional failure. | BOOT-0002 | covered with clarification |
| 4 | Discovery, authentication, authority, admission, selection, transfer, verification, installation, activation, recovery, and successful operation remain distinct. | BOOT-INV-004, BOOT-INV-017, BOOT-INV-019, BOOT-INV-020, BOOT-INV-030 | Applies the chain to assistance, source-bearing assembly, update preparation, installer results, and runtime handoff. | BOOT-0002, BOOT-0004, BOOT-0005 | covered with clarification |
| 5 | BOOT does not self-grant identity or authority. | BOOT-INV-005, BOOT-INV-006, BOOT-INV-007, BOOT-INV-008 | Separates session identity, location evidence, persisted identity, enrollment evidence, rooted identity, and bootstrap authority. | BOOT-0003 | covered with clarification |
| 6 | Installer-owned mutation remains outside BOOT. | BOOT-INV-003, BOOT-INV-021, BOOT-INV-024, BOOT-INV-025 | Applies the installer boundary to unattended installation, self-assembly, partial writes, rollback, and unknown prior mutation. | BOOT-0005, BOOT-0008 | covered with clarification |
| 7 | Transport-specific behavior remains outside BOOT semantic contracts. | BOOT-INV-003, BOOT-INV-017, BOOT-INV-018 | Separates transport sessions from assistance sessions and repeated delivery from new semantic work. | BOOT-0004 | covered with clarification |
| 8 | Cryptographic key custody and verification primitives remain with the security provider. | BOOT-INV-003, BOOT-INV-007, BOOT-INV-017, BOOT-INV-029 | Extends the provider boundary to image, source, artifact, update, private-module, and peer-transfer verification. | BOOT-0003, BOOT-0007, BOOT-0009 | covered with clarification |
| 9 | IMM may provide evidence or request recovery but cannot authorize itself or replace bootstrap authority. | BOOT-INV-001, BOOT-INV-003, BOOT-INV-008, BOOT-INV-019, BOOT-INV-030 | Makes IMM recommendations and recovery requests explicitly non-authorizing and preserves IMM restoration ownership after runtime handoff. | BOOT-0007 | covered with clarification |
| 10 | MEM retains persistence, custody, retention, reconstruction, deletion, and memory-recovery authority. | BOOT-INV-001, BOOT-INV-003, BOOT-INV-030 | Clarifies that BOOT persistence and runtime success create neither MEM truth nor MEM authority. | BOOT-0007 | covered with clarification |
| 11 | Independent BOOT state dimensions remain independently observable. | BOOT-INV-009, BOOT-INV-010, BOOT-INV-011 | Requires summaries to preserve underlying dimensions and prevents one dimension’s failure from fabricating another. | BOOT-0002 | covered with clarification |
| 12 | All authoritative BOOT state changes obey conservative atomicity. | BOOT-INV-012, BOOT-INV-013, BOOT-INV-014, BOOT-INV-015 | Adds checked arithmetic, bounded result retention, explicit resource exhaustion, revision preservation, and interruption behavior. | BOOT-0002, BOOT-0006, BOOT-0008 | covered with clarification |
| 13 | Minimal headless operation is mandatory. | BOOT-INV-022 | Enumerates prohibited architectural dependencies while allowing optional terminal diagnostics. | BOOT-0006 | covered with clarification |
| 14 | Public contracts remain language-neutral. | BOOT-INV-023 | Defines the required ownership, buffer, result, exception, and allocator neutrality without selecting a C ABI or implementation language. | BOOT-0006 | covered with clarification |
| 15 | Private trust topology, credentials, authorization policy, responder logic, and production recovery procedures remain private. | BOOT-INV-016, BOOT-INV-027, BOOT-INV-028, BOOT-INV-029 | Extends the boundary to private modules, repository locations, release policy, propagation, and public operation without restricted capability. | BOOT-0009 | covered with clarification |
| 16 | A successful BOOT handoff does not claim successful normal-runtime operation. | BOOT-INV-004, BOOT-INV-011, BOOT-INV-020, BOOT-INV-030 | Separates launch, initialization, readiness, degraded operation, sustained operation, and MEM/IMM authority after handoff. | BOOT-0002, BOOT-0007 | covered with clarification |
