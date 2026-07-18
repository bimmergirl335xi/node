# ASM-0001: Assembly-Kernel Invariants

| Field | Value |
|---|---|
| Specification | ASM-0001 |
| Title | Assembly-Kernel Invariants |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ASM-PUB |
| Parent series | KRN-PUB |
| Authors | Node |
| Last updated | 2026-07-18 |
| Approval | Pending review |
| Depends on | ASM-0000; applicable KRN kernel-family invariants; applicable BOOT invariants; applicable ACS, MEM, IMM, installer, security-provider, resource-management, and platform architecture |
| Supersedes | ASM-0001 placeholder |
| Superseded by | None |
| Current confidence | High in role separation, authority separation, hardware-observation truthfulness, candidate-output separation, installer non-substitution, boundedness, provenance, conservative failure, and headless operation; exact KRN profile schemas, BOOT provider-operation schemas, and aggregate system-profile ownership remain externally defined or intentionally deferred |

> **An assembly environment must remain truthful about what it is, what it can observe, what it is authorized to do, what it produced, and what remains unresolved.**

## Architectural-intent notice

This specification defines the mandatory invariants of the Node assembly-kernel role and its accompanying assembly environment.

The invariants constrain every conforming:

- assembly-kernel profile;
- assembly-environment profile;
- ASM provider operation;
- hardware-inspection mechanism;
- build mechanism;
- workspace;
- cache;
- evidence record;
- candidate output;
- resource outcome;
- failure result;
- ASM conformance claim.

This specification does not define:

- the complete assembly-environment contract;
- exact hardware-observation schemas;
- exact compiler or linker interfaces;
- exact workspace layouts;
- exact resource quantities;
- exact failure-record schemas;
- exact conformance-test procedures;
- BOOT operation or generation schemas;
- KRN profile or compatibility-declaration schemas.

Those details belong to ASM-0002 through ASM-0007 and the applicable owning series.

---

## 1. Purpose

ASM-0000 defines the assembly-kernel role and the boundary of the assembly environment.

ASM-0001 converts that charter into durable rules that later ASM specifications and implementations must not violate.

These invariants exist to prevent an assembly mechanism from being mistaken for:

- a completed Node;
- a BOOT lifecycle decision;
- a source or release authority;
- an accepted assembly generation;
- an installer;
- an activation mechanism;
- a recovery authority;
- the normal runtime.

The invariants also ensure that an assembly environment remains usable on constrained, partially known, offline, heterogeneous, and failure-prone hardware without inventing favorable results.

---

## 2. Normative language

The terms **MUST** and **MUST NOT** describe mandatory requirements.

The terms **SHOULD** and **SHOULD NOT** describe strong recommendations. A departure requires documented justification and must not violate a mandatory invariant.

The term **MAY** describes permitted behavior.

An invariant is violated when observable behavior contradicts its required meaning, regardless of:

- component name;
- implementation language;
- process layout;
- privilege level;
- source-tree organization;
- kernel configuration;
- deployment location;
- technical co-location with another provider.

A component cannot claim compliance merely by using the terminology in this document.

---

## 3. Invariant applicability

### 3.1 Universal invariants

Every invariant in this document applies to every conforming assembly-kernel role unless the invariant explicitly states that it applies only when a capability is present.

A profile may omit an optional mechanism.

It may not weaken the invariant governing that mechanism when the mechanism is present.

### 3.2 Profile-declared requirements

KRN declarations determine which capabilities are:

- required;
- optional;
- unsupported;
- constrained;
- or not applicable

for a particular kernel profile, micro-OS profile, hardware era, or compatibility declaration.

ASM consumes those declarations.

ASM must not reinterpret an unsupported or absent capability as implicitly optional.

### 3.3 Cross-series references

Where an invariant refers to KRN, BOOT, ACS, MEM, IMM, an installer, a security provider, a resource manager, or another architecture, ASM may use an opaque reference until the owning schema is stable.

An opaque reference must preserve ownership.

It must not be expanded into an ASM-owned substitute schema merely for local convenience.

---

# 4. Role and identity invariants

## ASM-INV-001 — The assembly environment is not the assembled Node

The running assembly kernel and assembly environment MUST NOT be represented as the assembled Node merely because they booted successfully.

The following distinction is mandatory:

```text
assembly environment booted
    != Node assembled
    != installation completed
    != activation accepted
    != runtime ready
```

A successful assembly-environment startup establishes only that the declared environment mechanisms reached their reported state.

It does not establish:

- Node identity;
- assembly-generation identity;
- component completeness;
- installation completion;
- activation;
- recovery;
- runtime readiness.

### Required consequences

A conforming environment MUST:

- identify itself as an assembly environment;
- avoid normal-runtime readiness claims;
- expose its active assembly-kernel and micro-OS profile references;
- return control through an explicitly governed handoff or termination path.

---

## ASM-INV-002 — The assembly-kernel role must be explicitly declared

A kernel MUST NOT be treated as an assembly kernel solely because it:

- boots from installation media;
- contains build tools;
- runs before the normal runtime;
- has administrator privileges;
- can inspect hardware;
- can write storage;
- carries `assembly`, `bootstrap`, or `installer` in its name.

The assembly-kernel role must be established through the applicable KRN kernel-profile declaration.

The accompanying assembly environment must be represented through the applicable KRN micro-OS-profile declaration.

### Required consequences

ASM MUST be able to report:

- the active kernel-profile reference;
- the declared assembly-kernel role;
- the active primary micro-OS-profile reference;
- applicable subordinate profile references;
- the applicable KRN compatibility-declaration reference.

Missing or unresolved declarations MUST remain explicit.

---

## ASM-INV-003 — The running assembly kernel may differ from the kernel being assembled

The kernel currently running the assembly environment MUST remain semantically distinct from any kernel represented by an ASM candidate output.

The running kernel may:

- be from a different kernel profile;
- target a different hardware era;
- use a different architecture;
- use a different configuration;
- use a different toolchain;
- be older or newer;
- be unsuitable for the eventual runtime role.

No identity, compatibility, or eligibility may be inferred from technical similarity.

### Required consequences

Records concerning the running kernel and the candidate kernel MUST use distinct references.

A candidate kernel MUST NOT inherit:

- the running kernel’s profile identity;
- the running kernel’s compatibility status;
- the running kernel’s authority;
- the running environment’s operational state.

---

## ASM-INV-004 — KRN declarations do not become BOOT selections

A KRN profile or compatibility declaration describes a declared architectural relationship.

It does not establish that BOOT has:

- selected that profile;
- selected a component-set instance;
- evaluated the selected pair;
- accepted the pair into an assembly generation;
- authorized installation.

ASM MUST preserve the distinction between:

```text
KRN declaration
    != BOOT selection
    != BOOT pair evaluation
    != BOOT generation acceptance
```

### Required consequences

ASM may report KRN declaration references.

ASM MUST NOT report a favorable BOOT pair-evaluation result unless that result was supplied by BOOT or the applicable owning evaluator.

---

## ASM-INV-005 — Aggregate system-profile ownership remains deferred

ASM MUST NOT define, synthesize, or claim ownership of an aggregate `SystemProfileIdentity`.

The unresolved aggregate identity MUST NOT be silently assigned to:

- KRN;
- ASM;
- BOOT;
- an installer;
- a generated manifest;
- the active assembly environment.

### Required consequences

Where an aggregate system-profile reference would be useful but is not canonically available, ASM MUST report it as:

- unavailable;
- unresolved;
- not applicable;
- or represented by separately owned references.

---

# 5. Authority and ownership invariants

## ASM-INV-006 — Capability is not authority

The existence of an ASM capability MUST NOT be interpreted as permission to use that capability for a particular Node-targeted operation.

Examples include:

```text
compiler available != compilation authorized
network available != transfer authorized
device visible != device use authorized
storage writable != mutation authorized
test mechanism available != execution authorized
```

### Required consequences

A conforming ASM operation MUST evaluate or consume the applicable authority result before performing Node-targeted work.

Unavailable authority MUST NOT be replaced by assumed permission.

---

## ASM-INV-007 — Local privilege does not create Node authority

Operating-system privilege, firmware privilege, device access, physical possession, or local administrator control MUST NOT create semantic Node authority.

This includes:

- root access;
- kernel mode;
- direct block-device access;
- direct memory access;
- physical console access;
- unlocked firmware;
- attached debug hardware;
- possession of local source;
- possession of local credentials not applicable to the operation.

### Required consequences

Privilege may enable a mechanism.

It MUST NOT determine:

- source acceptance;
- release authority;
- BOOT authority;
- installation eligibility;
- activation eligibility;
- recovery authority;
- runtime-handoff eligibility.

---

## ASM-INV-008 — Node-targeted build work requires applicable BOOT authority

ASM MUST NOT begin Node-targeted build work without the applicable BOOT authority result.

Node-targeted build work includes compilation, linking, packaging, testing, transformation, or output production intended for possible incorporation into a Node assembly generation.

Authority MUST apply to the actual operation scope.

Authority for one:

- source;
- target;
- component;
- profile;
- generation;
- toolchain;
- operation attempt

MUST NOT be generalized to another.

### Required consequences

ASM MUST preserve:

- the opaque BOOT operation reference;
- the authority reference;
- the authorized scope;
- the local ASM provider-operation attempt;
- any limits or exclusions attached to the authority.

---

## ASM-INV-009 — Authority-not-required compilation must be explicit and disposable

Compilation without operation-specific BOOT authority is permitted only when an owning contract explicitly declares the work to be authority-not-required.

Such work MUST remain bounded to:

- compiler diagnostics;
- feature detection;
- toolchain probing;
- disposable test compilation;
- temporary compatibility experiments;
- isolated conformance tests.

Outputs from authority-not-required compilation MUST NOT be silently:

- retained as accepted artifacts;
- installed;
- activated;
- propagated;
- executed as normal Node code;
- incorporated into an assembly generation;
- used to replace an authorized build.

### Required consequences

The owning contract MUST identify:

- why authority is not required;
- permitted inputs;
- permitted outputs;
- resource limits;
- retention rules;
- disposal or quarantine rules;
- prohibited downstream use.

Absence of such a declaration means applicable BOOT authority is required.

---

## ASM-INV-010 — Mechanism control does not transfer semantic ownership

A component controlling a physical mechanism MUST NOT be assumed to own the semantic decision enabled by that mechanism.

Examples include:

| Controlled mechanism | Semantic ownership that remains separate |
|---|---|
| Compiler execution | BOOT authority and later generation acceptance |
| Signature-verification invocation | Security-provider verification semantics |
| Network byte movement | Transport and ACS semantics |
| Storage writes inside a workspace | ASM candidate-output semantics |
| Production-target writes | Installer semantics |
| Reboot mechanism | BOOT handoff semantics |
| Runtime launch mechanism | Runtime readiness semantics |

### Required consequences

ASM records MUST preserve, where applicable:

- mechanism controller;
- requesting subsystem;
- semantic owner;
- authority provider;
- evidence provider;
- verifier;
- result consumer.

Technical co-location MUST NOT collapse those roles.

---

## ASM-INV-011 — ASM must not substitute for another owning architecture

ASM MUST NOT invent local equivalents for missing decisions owned by:

- KRN;
- BOOT;
- ACS;
- MEM;
- IMM;
- an installer;
- a security provider;
- a resource manager;
- the normal runtime;
- a physical-safety authority.

### Required consequences

When an externally owned prerequisite is unavailable, ASM MUST:

- stop;
- narrow the operation;
- return an unresolved result;
- or use an explicitly defined degraded path.

ASM MUST NOT manufacture a favorable default.

---

# 6. Hardware-observation invariants

## ASM-INV-012 — Hardware observation is not identity, eligibility, or registration

Detecting, enumerating, probing, or initializing hardware MUST NOT independently establish:

- durable hardware identity;
- Node participant identity;
- ACS membership;
- kernel-profile eligibility;
- micro-OS-profile eligibility;
- component registration;
- installation-target eligibility;
- runtime readiness.

### Required consequences

ASM hardware observations MUST remain evidence supplied to the applicable owning evaluator.

They MUST NOT be presented as that evaluator’s conclusion.

---

## ASM-INV-013 — Observation and initialization are distinct

ASM MUST preserve the distinction between:

```text
device observed
    != device initialized
    != device accessible
    != device usable
    != device authorized
```

ASM MUST NOT initialize every observed device merely to increase discovery completeness.

Observation-only hardware may include:

- production installation targets;
- existing system disks;
- unrelated accelerators;
- sensors;
- actuators;
- devices reserved for another subsystem;
- unsupported hardware;
- hardware prohibited by policy or authority.

### Required consequences

Each applicable hardware result MUST indicate whether initialization was:

- not attempted;
- unnecessary;
- requested;
- completed;
- partial;
- failed;
- prohibited;
- or unresolved.

---

## ASM-INV-014 — Hardware uncertainty must remain explicit

ASM MUST preserve uncertainty when hardware inspection is incomplete, contradictory, unsupported, interrupted, or bounded.

ASM MUST NOT translate lack of evidence into evidence of absence.

Examples include:

```text
not detected != absent
driver unavailable != device absent
probe failed != hardware defective
device inaccessible != device unauthorized
identifier conflict != identity resolved
```

### Required consequences

Hardware-observation mechanisms MUST support results equivalent to:

- observed;
- not observed;
- partially identified;
- unknown;
- conflicting;
- inaccessible;
- unsupported;
- unavailable;
- prohibited;
- failed;
- indeterminate.

---

## ASM-INV-015 — Hardware evidence must retain scope and provider

A hardware observation without its scope or provider MUST NOT be treated as universal hardware truth.

### Required consequences

Applicable observations MUST retain:

- subject reference;
- observation provider;
- mechanism used;
- observation time or ordering context where available;
- scope;
- completeness;
- limitations;
- confidence or uncertainty;
- transformation lineage;
- relevant profile or operation reference.

Evidence transformed from another report MUST retain that lineage.

---

# 7. Input, build, and verification invariants

## ASM-INV-016 — Input presence is not input acceptance

The presence of source, packages, configuration, toolchains, cached material, manifests, or credentials MUST NOT establish:

- source identity;
- source authority;
- release identity;
- release acceptance;
- freshness;
- compatibility;
- permission to build;
- permission to install.

### Required consequences

ASM MUST preserve the distinction between:

```text
input available
input identified
input verified by a provider
input authorized for the operation
input selected by BOOT
```

A later state MUST NOT be inferred solely from an earlier state.

---

## ASM-INV-017 — Build stages must remain semantically separate

The following results MUST remain distinct:

```text
source available
    != compilation authorized
    != compilation completed
    != linking completed
    != candidate output produced
    != output verified
    != output accepted by BOOT
    != installation completed
    != activation accepted
    != runtime ready
```

### Required consequences

A conforming implementation MUST NOT expose one undifferentiated `BUILD_SUCCESS` result when later stages remain unevaluated.

Each result MUST identify its actual scope.

---

## ASM-INV-018 — Compiler and linker success are mechanism results only

A zero exit status, emitted file, completed process, or successful tool invocation establishes only the declared mechanism result.

It does not establish:

- semantic correctness;
- compatibility;
- reproducibility;
- completeness;
- integrity;
- security;
- artifact acceptance;
- installation eligibility.

### Required consequences

ASM MUST report compiler, assembler, linker, package, and test results as provider-attributed mechanism outcomes.

Later interpretation remains with the applicable owning evaluator.

---

## ASM-INV-019 — Verification-provider results retain their original meaning

ASM may invoke or consume verification mechanisms.

ASM MUST NOT broaden the meaning of a verification-provider result.

Examples include:

```text
digest match != release accepted
signature valid != source authorized for this operation
test passed != component eligible
reproducible output != approved output
platform attestation valid != installation authorized
```

### Required consequences

ASM MUST preserve:

- verification provider;
- verified subject;
- verification scope;
- algorithm or method reference where appropriate;
- result;
- limitations;
- applicable authority context.

---

# 8. Candidate-output and generation invariants

## ASM-INV-020 — Physical outputs remain ASM candidate outputs until BOOT acceptance

Every physical or logical output produced by ASM remains an **ASM candidate output** until BOOT accepts it into an assembly generation.

This applies even when the output:

- compiled successfully;
- passed tests;
- has a digest;
- carries a valid signature;
- matches a previous output;
- was produced by an approved toolchain;
- appears complete;
- is stored on persistent media.

### Required consequences

ASM MUST NOT label its output as:

- an accepted assembly generation;
- a generation component;
- installation-eligible;
- activation-ready;
- runtime-ready

unless the applicable BOOT result explicitly establishes that meaning.

---

## ASM-INV-021 — ASM output staging is not BOOT generation staging

Placing candidate outputs into an ASM-controlled staging area MUST NOT establish BOOT `GENERATION_STAGED` or an equivalent generation-state conclusion.

The distinction is:

```text
ASM candidate output physically staged
    != candidate output accepted by BOOT
    != assembly generation staged
```

### Required consequences

ASM staging records MUST use ASM terminology.

BOOT generation terminology may be referenced only through a BOOT-owned result or opaque BOOT reference.

---

## ASM-INV-022 — Candidate outputs must not be silently promoted or executed

ASM MUST NOT silently promote a candidate output into:

- an accepted artifact;
- a production installation;
- an active boot target;
- a normal runtime component;
- a reusable trusted cache entry;
- a later operation’s accepted input.

Execution required for bounded testing MUST remain distinguishable from activation or ordinary runtime execution.

### Required consequences

Any downstream use of a candidate output MUST require an explicit owning contract or BOOT progression.

Absence of a rejection does not constitute acceptance.

---

## ASM-INV-023 — Partial and interrupted work remains partial or indeterminate

A stopped process, environment restart, power loss, timeout, cancellation, resource exhaustion, transport loss, or tool failure MUST NOT be converted into success.

Partial output MUST remain:

- partial;
- quarantined;
- unusable;
- cleanup-pending;
- or indeterminate

until an owning evaluator establishes otherwise.

### Required consequences

ASM MUST report, where knowable:

- completed scope;
- incomplete scope;
- candidate-output references;
- quarantine status;
- cleanup status;
- unresolved effects;
- prohibited downstream use;
- whether reconciliation may be required.

---

# 9. Storage and installer invariants

## ASM-INV-024 — ASM must not mutate production installation targets

ASM MUST NOT perform durable production installation-target mutation.

Installer-owned mutation includes:

- partition-table changes;
- formatting;
- production filesystem creation;
- production operating-system deployment;
- production package installation;
- durable production configuration;
- boot-target configuration;
- activation staging;
- rollback;
- installer cleanup.

### Required consequences

ASM may:

- enumerate a production target;
- inspect it within authority and safety limits;
- mount it read-only where permitted;
- report observations;
- produce a candidate disk-image file inside ASM staging.

Writing that image to the production target remains installer-owned.

---

## ASM-INV-025 — Technical write capability does not change the installer boundary

An ASM environment may possess technical mechanisms capable of writing a production target.

That capability MUST NOT be interpreted as ASM ownership of production mutation.

A component implementing both build and installer functions MUST expose:

- separate provider roles;
- separate authority;
- separate operation references;
- separate results;
- separate failure semantics.

### Required consequences

Installer-owned mutation MUST remain attributable to the installer even when the installer executes:

- inside the assembly environment;
- using assembly-kernel drivers;
- through the same process executable;
- on the same machine;
- immediately after ASM work.

---

# 10. Resource and workspace invariants

## ASM-INV-026 — Every ASM work domain must be bounded

ASM operations MUST remain finite in all applicable resource dimensions.

These include:

- CPU time;
- wall-clock time where available;
- process count;
- thread count;
- memory;
- temporary storage;
- persistent storage;
- file count;
- descriptor count;
- output size;
- diagnostic size;
- network transfer;
- retries;
- recursion;
- concurrency;
- device-runtime use.

### Required consequences

Each operation MUST be subject to declared or inherited bounds.

Unbounded growth MUST NOT be an accepted operating mode.

Resource-limit absence MUST remain explicit.

---

## ASM-INV-027 — ASM must preserve capacity to fail honestly

Ordinary build and inspection work MUST NOT consume every resource required to:

- report failure;
- preserve required evidence;
- process cancellation;
- quarantine partial output;
- terminate child work;
- report cleanup state;
- return control.

### Required consequences

ASM profiles MUST reserve or otherwise guarantee a bounded failure path.

Failure-path capacity may be small.

It must not depend on unlimited allocation succeeding after exhaustion has already occurred.

---

## ASM-INV-028 — Resource exhaustion is an explicit outcome

Resource exhaustion MUST NOT be reported as:

- unsupported hardware;
- invalid source;
- compiler defect;
- security failure;
- authority denial;
- successful completion.

### Required consequences

The exhausted resource class and known effects MUST be reported where possible.

ASM MUST distinguish:

- resource unavailable before operation;
- resource exhausted during operation;
- external resource withdrawal;
- resource-accounting uncertainty.

---

## ASM-INV-029 — Workspaces must be attributable and isolated

Active workspaces MUST be attributable to an ASM provider operation or another explicitly safe isolation boundary.

One operation MUST NOT silently consume, modify, or adopt another operation’s:

- intermediate objects;
- generated configuration;
- temporary files;
- partial outputs;
- locks;
- process state;
- authority context.

### Required consequences

Workspace reuse requires explicit compatibility and ownership rules.

Stale workspace material MUST NOT be assumed safe.

---

## ASM-INV-030 — Caches are non-authoritative

A cache entry MUST NOT be treated as:

- accepted source;
- release truth;
- a current component;
- a verified candidate output;
- an assembly generation;
- installation eligibility.

Cached material MUST be revalidated according to the applicable profile, operation, provider, and BOOT requirements before authoritative use.

### Required consequences

Caches MUST be:

- bounded;
- attributable where required;
- invalidatable;
- safely evictable;
- separable from accepted outputs;
- capable of reporting stale or unresolved state.

Cache loss MUST NOT destroy the only authoritative record required to interpret an operation.

---

## ASM-INV-031 — Cleanup and quarantine state must remain explicit

Cleanup initiation MUST NOT be reported as cleanup completion.

Deletion attempts, process termination, unmount requests, cache eviction, and output quarantine may fail or remain partial.

### Required consequences

ASM MUST distinguish applicable states equivalent to:

- cleanup not required;
- cleanup pending;
- cleanup active;
- cleanup complete;
- cleanup partial;
- cleanup failed;
- cleanup indeterminate;
- quarantine active;
- quarantine failed.

Restart MUST NOT silently erase incomplete cleanup or quarantine state.

---

# 11. Environment and operation invariants

## ASM-INV-032 — The assembly environment must remain minimal and headless

A conforming assembly environment MUST NOT require:

- a GUI;
- a display server;
- a desktop environment;
- a monitor;
- a keyboard;
- a local human-readable console;
- the normal Node runtime;
- one specific high-level language runtime;
- one service manager;
- one container runtime;
- unrestricted persistent storage.

### Required consequences

All authoritative ASM results MUST be available through machine-readable mechanisms.

Human-readable output MAY exist but MUST NOT be the sole authoritative result.

---

## ASM-INV-033 — Network availability is profile-declared, not universal

ASM MUST NOT assume continuous network access.

An assembly-environment profile may declare networking:

- required;
- optional;
- unsupported;
- constrained;
- unavailable.

Offline operation MUST remain possible where the applicable profile permits it.

### Required consequences

Network loss MUST NOT automatically invalidate otherwise local work unless the operation contract requires continued connectivity.

Network reachability MUST NOT establish:

- source acceptance;
- identity;
- authority;
- integrity;
- freshness;
- BOOT progression.

---

## ASM-INV-034 — Results must be structured and attributable

Every authoritative ASM result MUST be representable in a structured form.

Applicable results include:

- environment capability reports;
- hardware observations;
- mechanism results;
- resource outcomes;
- candidate outputs;
- verification-provider results;
- failure results;
- cleanup state.

### Required consequences

A result MUST preserve enough context to determine:

- what was attempted;
- by which provider;
- for which local attempt;
- under which external operation reference;
- against which subject or input;
- with which scope;
- with which limitations;
- with which outcome.

Logs alone are insufficient.

---

## ASM-INV-035 — Provenance must remain bounded but sufficient

ASM MUST preserve enough provenance to interpret its results without requiring unbounded history.

Applicable provenance may include:

- KRN profile references;
- compatibility-declaration reference;
- BOOT operation reference;
- ASM local-attempt reference;
- authority reference;
- source reference;
- toolchain reference;
- configuration reference;
- environment capabilities;
- hardware observations;
- transformations;
- cache participation;
- candidate-output lineage;
- test results;
- resource outcomes.

### Required consequences

Provenance truncation, omission, concealment, or unavailability MUST remain explicit.

ASM MUST NOT claim complete provenance when required links are missing.

---

## ASM-INV-036 — Environment restart does not imply semantic recovery

Restarting the assembly kernel or assembly environment MUST NOT independently establish:

- operation cancellation;
- cleanup completion;
- candidate-output rejection;
- resource release;
- assembly-generation rollback;
- installer rollback;
- recovery completion;
- safe continuation.

### Required consequences

Post-restart ASM behavior MUST preserve or report applicable unresolved prior state.

BOOT owns continuation, reconciliation, recovery, and generation progression.

---

## ASM-INV-037 — ASM must support conservative termination and return of control

An assembly environment MUST provide a bounded path to:

- stop further work;
- terminate or isolate operation-local processes;
- finalize or report evidence;
- quarantine partial candidate outputs;
- report unreleased resources;
- close operation-local mechanisms;
- return control to the owning caller or boot architecture.

### Required consequences

Failure to terminate cleanly MUST be reportable as an ASM failure.

A reboot or power-off MAY be used as a mechanism where authorized, but MUST NOT be represented as proof of successful handoff or recovery.

---

# 12. Conformance and publication invariants

## ASM-INV-038 — ASM conformance is profile-scoped

An implementation MUST NOT claim universal ASM conformance solely because it satisfies one assembly-kernel profile.

A conformance claim must identify the applicable:

- kernel profile;
- hardware or software era;
- primary micro-OS profile;
- compatibility declaration;
- required capabilities;
- optional capabilities;
- unsupported capabilities;
- conformance version.

### Required consequences

A profile-scoped conformance result does not establish:

- BOOT pair acceptance;
- assembly-generation eligibility;
- installation eligibility;
- activation eligibility;
- runtime readiness.

---

## ASM-INV-039 — Implementation placement does not alter invariants

ASM invariants apply regardless of whether a mechanism is implemented in:

- firmware;
- a bootloader;
- kernel space;
- userspace;
- an initramfs;
- a container;
- a virtual machine;
- a device runtime;
- a remote authorized provider;
- a combined executable.

### Required consequences

Moving a mechanism across implementation layers MUST NOT:

- transfer semantic ownership;
- broaden authority;
- erase provider attribution;
- collapse result stages;
- bypass boundedness;
- bypass failure reporting.

---

## ASM-INV-040 — Public ASM conformance must not require private capability

A public ASM profile MUST remain operable within its declared public capability set without requiring private modules, credentials, sources, or policies.

Unavailable private capability MUST be represented truthfully.

It MUST NOT be represented as successful, present, or implicitly granted.

### Required consequences

Public conformance tests MUST NOT require disclosure of:

- production credentials;
- private keys;
- private repositories;
- protected source locations;
- private module identities;
- production authority assignments;
- private installer policy;
- production topology.

A private extension may add capability.

It MUST NOT redefine the public invariants.

---

# 13. Canonical non-equivalences

The following non-equivalences summarize the mandatory invariant boundary:

```text
assembly environment booted
    != Node assembled

assembly-kernel role declared
    != kernel/micro-OS pair selected by BOOT

KRN compatibility declared
    != BOOT pair evaluation accepted

hardware observed
    != hardware identified durably

hardware identified
    != hardware authorized

device initialized
    != device eligible

capability present
    != operation authorized

local root access
    != Node authority

source present
    != source accepted

source accepted
    != compilation authorized

compilation completed
    != candidate output verified

candidate output verified
    != candidate output accepted by BOOT

ASM candidate output staged
    != BOOT assembly generation staged

assembly generation accepted
    != installation completed

installation completed
    != activation accepted

activation accepted
    != runtime ready

process exited
    != provider operation completed

reboot occurred
    != recovery completed

cache hit
    != current authoritative input

cleanup requested
    != cleanup completed

resource available
    != authority granted

network connected
    != identity or trust established

profile conformance
    != lifecycle eligibility
```

No later ASM specification may collapse these distinctions.

---

# 14. Invariant-to-specification allocation

| Invariant area | Primary refinement |
|---|---|
| Assembly-environment identity and startup | ASM-0002 |
| Required and optional environment mechanisms | ASM-0002 |
| Hardware observation and initialization | ASM-0003 |
| Hardware uncertainty and evidence | ASM-0003 |
| Build authority and toolchain mechanisms | ASM-0004 |
| Build-stage separation | ASM-0004 |
| Candidate-output production | ASM-0004 |
| Workspaces, caches, staging, and reserves | ASM-0005 |
| Resource exhaustion | ASM-0005 |
| Partial output and cleanup | ASM-0005 and ASM-0006 |
| Failure attribution and indeterminate state | ASM-0006 |
| Restart and conservative termination | ASM-0006 |
| Profile-scoped conformance | ASM-0007 |
| Negative invariant tests | ASM-0007 |
| KRN profile and compatibility structures | KRN series |
| BOOT authority, selection, pair evaluation, and generations | BOOT series |
| Production target mutation and rollback | Installer architecture |
| Identity, relationships, admission, and connections | ACS |
| Cryptographic verification and key custody | Security provider |
| Allocation, ceilings, and reclamation | Resource management |
| Runtime readiness | Normal runtime |

---

# 15. Prohibited implementation shortcuts

A conforming implementation MUST NOT:

1. use successful boot as proof that the environment is compatible with every target;
2. infer BOOT authority from local privilege;
3. infer hardware absence from a failed probe;
4. initialize unrelated production hardware without declared need;
5. treat a writable target as an authorized target;
6. compile Node-targeted material without applicable authority;
7. reuse diagnostic output as production output without a new authorized operation;
8. treat a zero process exit status as full operation success;
9. label candidate outputs as an assembly generation;
10. write candidate images directly to production targets through the ASM role;
11. treat cache contents as accepted source or artifact truth;
12. allow ordinary work to consume the failure-reporting reserve;
13. discard partial-output or cleanup uncertainty on restart;
14. depend exclusively on human-readable logs;
15. make network access an undeclared universal requirement;
16. merge physical controller and semantic owner into one implicit role;
17. treat conformance as BOOT eligibility;
18. absorb aggregate `SystemProfileIdentity` ownership;
19. replace unresolved external decisions with favorable local defaults;
20. weaken public invariants through a private extension.

---

# 16. Validation obligations

ASM-0007 will define the detailed conformance suite.

At minimum, conformance must demonstrate that the implementation can:

- boot while identifying itself only as an assembly environment;
- distinguish the running assembly kernel from a candidate kernel;
- reject Node-targeted build work lacking applicable authority;
- permit only explicitly declared bounded authority-not-required diagnostics;
- observe hardware without claiming eligibility;
- preserve unknown and conflicting hardware state;
- separate compilation, verification, BOOT acceptance, installation, and activation;
- retain candidate-output terminology before BOOT acceptance;
- prevent ASM-role mutation of a production target;
- enforce resource bounds;
- preserve failure-reporting capacity under pressure;
- invalidate or revalidate cache material;
- preserve partial and interrupted state;
- report cleanup and quarantine outcomes;
- operate without a GUI;
- produce structured machine-readable results;
- operate offline where the profile allows;
- preserve provider and provenance attribution;
- stop conservatively;
- avoid claiming BOOT or runtime conclusions;
- scope conformance to the declared KRN profile and micro-OS profile.

A conformance test that validates only the favorable path is insufficient.

Negative, interruption, exhaustion, unsupported-hardware, and indeterminate-state tests are required.

---

# 17. Cross-series dependencies and open contracts

The following dependencies remain intentionally external to ASM-0001.

## 17.1 KRN dependencies

KRN must define or stabilize:

- kernel-profile structure;
- micro-OS-profile structure;
- compatibility-declaration structure;
- required-component declarations;
- optional-component declarations;
- unsupported-component declarations;
- role and era representation;
- profile lifecycle;
- profile-scoped conformance evidence.

ASM-0001 does not prescribe those schemas.

## 17.2 BOOT dependencies

BOOT must define or stabilize:

- semantic assembly-operation identity;
- operation attempts and revisions;
- authority-envelope references;
- provider-operation requests;
- selected component-set instances;
- pair-evaluation results;
- assembly-generation identity and state;
- candidate-output acceptance;
- installer handoff;
- reconciliation of interrupted provider operations;
- the provider-attributed replacement or narrowing of `BUILD_OR_SELECTION_ACTIVE`, such as `PROVIDER_OPERATION_ACTIVE`.

ASM-0001 does not prescribe those schemas.

## 17.3 Installer dependencies

Installer architecture must define:

- production target selection;
- durable mutation;
- write verification;
- activation staging;
- rollback;
- installer-owned cleanup;
- installer result records.

## 17.4 Deferred ownership

The following remain unresolved:

- aggregate `SystemProfileIdentity`;
- complete installed-system generation architecture;
- update architecture;
- source and release architecture;
- production rollout architecture.

No unresolved contract weakens an ASM invariant.

---

# 18. Completion checklist

Draft review must confirm:

- [ ] Every invariant constrains ASM behavior rather than duplicating a BOOT lifecycle rule.
- [ ] Assembly kernel and assembly environment remain ASM terms.
- [ ] Assembly generation remains a BOOT term.
- [ ] Kernel/micro-OS compatibility declaration remains a KRN term.
- [ ] The running assembly kernel remains distinct from a candidate kernel.
- [ ] Capability remains distinct from authority.
- [ ] Local privilege remains distinct from Node authority.
- [ ] Node-targeted build work requires applicable BOOT authority.
- [ ] Authority-not-required compilation remains explicit, bounded, diagnostic, or disposable.
- [ ] Hardware observation remains distinct from identity, eligibility, and registration.
- [ ] Unknown and partial observations remain explicit.
- [ ] Input presence remains distinct from input acceptance.
- [ ] Compilation, verification, BOOT acceptance, installation, activation, and readiness remain separate.
- [ ] ASM physical outputs remain candidate outputs until BOOT acceptance.
- [ ] ASM output staging remains distinct from BOOT generation staging.
- [ ] ASM does not mutate production installation targets.
- [ ] Mechanism control remains distinct from semantic ownership.
- [ ] Workspaces and caches remain bounded and non-authoritative.
- [ ] Failure-reporting capacity remains protected.
- [ ] Partial work, cleanup, and restart uncertainty remain explicit.
- [ ] The environment remains minimal and headless.
- [ ] Machine-readable results remain mandatory.
- [ ] Networking remains profile-declared.
- [ ] Conformance remains profile-scoped.
- [ ] Public conformance does not require private capability.
- [ ] Aggregate `SystemProfileIdentity` ownership remains deferred.
- [ ] No cross-series conflict is silently resolved.

---

# 19. Closing principle

> **ASM must never turn mechanism, access, progress, or physical output into semantic acceptance by implication.**

## Revision history

### Version 0.1 — 2026-07-18

- Replaced the ASM-0001 placeholder with the initial invariant set.
- Established forty numbered assembly-kernel invariants.
- Preserved the distinction between the assembly environment and the assembled Node.
- Preserved KRN ownership of profile and compatibility declarations.
- Preserved BOOT ownership of selection, authority, pair evaluation, assembly generations, progression, installer handoff, activation, recovery, and runtime-handoff eligibility.
- Defined explicit authority requirements for Node-targeted build work.
- Restricted authority-not-required compilation to explicitly declared bounded diagnostic or disposable work.
- Preserved hardware-observation uncertainty and provider attribution.
- Established candidate-output and ASM-staging separation.
- Preserved installer ownership of production target mutation.
- Required bounded workspaces, non-authoritative caches, failure-path reserves, and explicit cleanup state.
- Required structured machine-readable results and headless operation.
- Preserved profile-scoped conformance and public/private boundaries.
- Kept aggregate `SystemProfileIdentity` ownership deferred.
