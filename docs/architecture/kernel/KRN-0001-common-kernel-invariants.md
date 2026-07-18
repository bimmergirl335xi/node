# KRN-0001: Common Kernel Invariants

| Field | Value |
|---|---|
| Specification | KRN-0001 |
| Title | Common Kernel Invariants |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | KRN-PUB |
| Authors | Node |
| Authoring lane | `lane/docs` |
| Last updated | 2026-07-18 |
| Approval | Pending review |
| Depends on | KRN-0000; applicable ACS, MEM, IMM, and BOOT-0000 through BOOT-0004 public architecture |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in family identity, mechanism-versus-ownership, authority, compatibility, evidence, uncertainty, provenance, boundedness, lifecycle, and adjacent-architecture invariants; detailed profile schemas, role catalogs, era vocabularies, compatibility records, lifecycle transitions, and role-specific requirements remain assigned to later KRN and ASM specifications |

> **A kernel profile conforms only when its identity, claims, mechanisms, limitations, authority boundaries, and lifecycle remain truthful under ordinary operation, failure, uncertainty, replacement, and change.**

## Architectural-intent notice

This specification defines the canonical mandatory invariants shared by every Node kernel profile.

The invariants apply regardless of:

- kernel role;
- hardware era;
- software era;
- processor architecture;
- platform;
- upstream kernel lineage;
- patch set;
- configuration;
- toolchain;
- micro-OS;
- deployment environment;
- implementation mechanism;
- device-control path;
- boot medium;
- network availability;
- operator presence;
- maintenance state.

This specification refines KRN-0000.

It does not replace, weaken, or reinterpret the charter.

Later KRN specifications may define detailed identity structures, role and era models, compatibility declarations, boundary matrices, lifecycle states, conformance profiles, and implementation requirements. They MUST preserve these invariants.

Role-specific subseries such as ASM may add stricter requirements. They MUST NOT weaken the common KRN invariants.

## 1. Purpose

KRN-0000 establishes that Node maintains a family of attributable kernel profiles rather than one permanently universal kernel.

KRN-0001 converts the charter’s enduring commitments into stable rules suitable for:

- architecture review;
- profile design;
- configuration review;
- patch review;
- build-system design;
- compatibility review;
- conformance testing;
- failure injection;
- security review;
- maintenance decisions;
- supersession and retirement;
- cross-series conflict detection.

These invariants exist to prevent implementation convenience from silently redefining:

- what a kernel profile is;
- what identifies it;
- what compatibility means;
- what a capability claim proves;
- what successful boot or build proves;
- who owns authority;
- what state must remain uncertain;
- how lifecycle changes are represented;
- how Node behaves when profiles age, fail, or are replaced.

A profile does not conform merely because it boots on one machine or successfully executes its ordinary workload.

Conformance requires preservation of the applicable invariants during:

- unsupported or partially supported hardware;
- missing firmware;
- incomplete driver coverage;
- failed or interrupted builds;
- resource exhaustion;
- degraded operation;
- unavailable authority;
- conflicting evidence;
- stale compatibility declarations;
- maintenance loss;
- replacement;
- revocation;
- retirement.

## 2. Scope

These invariants apply to:

- kernel-family definitions;
- kernel roles;
- era declarations;
- kernel profiles;
- kernel source identities;
- patch-set identities;
- configuration identities;
- toolchain identities;
- kernel builds;
- kernel artifacts;
- deployed kernel instances;
- micro-OS profiles;
- subordinate micro-OS components where independently identified;
- kernel/micro-OS compatibility declarations;
- required-component declarations;
- profile capability and limitation declarations;
- profile validation and conformance;
- profile maintenance and lifecycle;
- KRN-facing observations and evidence;
- KRN interaction with ASM, BOOT, ACS, MEM, IMM, installers, runtimes, resource managers, security providers, device backends, firmware, and package systems.

They apply to public and private implementations of KRN-governed behavior.

## 3. Explicit non-goals

KRN-0001 does not define:

- the complete role catalog;
- role-specific ASM invariants;
- exact era labels;
- exact profile-record layouts;
- exact identifier encodings;
- exact compatibility-record layouts;
- exact required-component schemas;
- exact lifecycle state machines;
- profile-selection algorithms;
- BOOT state transitions;
- assembly-generation state;
- selected component-set instances;
- artifact-acceptance policy;
- installer APIs;
- runtime-readiness contracts;
- device-backend interfaces;
- concrete security mechanisms;
- production signing policy;
- production trust topology;
- aggregate `SystemProfileIdentity`;
- private deployment policy.

Those matters belong to later KRN specifications, ASM, BOOT, adjacent architecture, future architecture, or private operational policy.

## 4. Normative language

The terms **MUST** and **MUST NOT** define mandatory architectural requirements.

The terms **SHOULD** and **SHOULD NOT** define strong recommendations. A departure requires documented justification and MUST NOT violate a mandatory requirement.

The term **MAY** defines permitted behavior.

Conformance depends on observable behavior and attributable declarations, not labels such as:

- `Node kernel`;
- `supported`;
- `compatible`;
- `trusted`;
- `validated`;
- `secure`;
- `production`;
- `legacy`;
- `current`;
- `recovery`;
- `assembly`;
- `ready`.

Such terms MUST identify their applicable subject, scope, revision, evidence boundary, and declaring authority.

## 5. Invariant interpretation rules

### 5.1 Stable identifiers

Each invariant has a stable identifier:

```text
KRN-INV-NNN
```

The identifier refers to the architectural rule, not one implementation, profile, source file, test, build, or release.

Approved invariant identifiers MUST NOT be renumbered for formatting convenience.

A material change to an invariant requires explicit review of:

- the prior meaning;
- the proposed meaning;
- KRN-0000 commitments;
- affected later KRN specifications;
- affected ASM and BOOT contracts;
- adjacent architecture;
- implementation consequences;
- conformance consequences.

### 5.2 Invariants are cumulative

All applicable invariants operate together.

Satisfying one invariant does not excuse violating another.

For example:

- complete provenance does not compensate for absent authority;
- successful boot does not compensate for stale compatibility;
- rollback capability does not compensate for unauthorized mutation;
- current maintenance does not compensate for false capability claims;
- strong isolation does not compensate for ownership collapse;
- public source availability does not compensate for incompatible licensing.

### 5.3 No compensating violation

A profile or implementation MUST NOT claim that one protective or functional feature compensates for violating an unrelated invariant.

Invalid compensations include:

- secure boot compensating for absent BOOT authority;
- signatures compensating for incompatibility;
- a current kernel version compensating for missing device support;
- successful tests on one host compensating for an unsupported hardware class;
- root privilege compensating for absent operation authority;
- complete logs compensating for false state;
- recovery media compensating for unbounded destructive behavior;
- source availability compensating for absent license compliance.

### 5.4 Later specifications refine mechanisms

Later KRN specifications MAY define:

- structures;
- state vocabularies;
- role catalogs;
- era dimensions;
- validation requirements;
- lifecycle transitions;
- conformance levels;
- evidence records;
- operation flows.

They MUST preserve these invariants.

### 5.5 Private policy cannot weaken public invariants

Private policy MAY select among publicly eligible profiles, mechanisms, releases, and actions.

Private policy MUST NOT:

- fabricate missing evidence;
- weaken identity requirements;
- bypass authority;
- erase uncertainty;
- redefine adjacent ownership;
- reactivate retired or revoked profiles without governed review;
- treat protected implementation details as exemption from public conformance.

### 5.6 Conservative interpretation under ambiguity

When application of an invariant is materially ambiguous, the implementation MUST:

- preserve existing authoritative state;
- avoid broader claims;
- avoid broader authority;
- report the ambiguity;
- select an explicitly permitted narrower action;
- defer, restrict, refuse, or request review where required.

Ambiguity MUST NOT be resolved by selecting the most permissive interpretation.

### 5.7 Prohibited interpretations are normative

The prohibited interpretations listed under each invariant are part of the invariant.

They are not merely examples.

## 6. Canonical KRN invariants

### 6.1 Family, role, era, and identity

### KRN-INV-001 — No kernel profile is permanently universal

**Binding rule**

No kernel profile MUST be treated as permanently suitable for every Node role, hardware class, software generation, firmware environment, package ecosystem, security condition, or maintenance horizon.

**Why the invariant exists**

A universal-kernel assumption hides incompatible requirements and causes one profile to accumulate unsupported hardware, stale drivers, conflicting configurations, unmaintained dependencies, and untestable compatibility claims.

**Required observable behavior**

A conforming implementation MUST:

- declare the profile’s intended scope;
- identify unsupported or unevaluated roles and eras;
- permit multiple profiles to coexist within the family;
- permit a profile’s scope to narrow as evidence or maintenance conditions change;
- avoid treating the current primary profile as the permanent family definition.

**Prohibited interpretations**

This invariant forbids claims that:

- one successful profile eliminates the need for a family;
- newest means universally suitable;
- broad configuration automatically creates broad conformance;
- historical use proves indefinite future eligibility.

**Failure or uncertainty behavior**

When suitability outside the declared scope is unknown, the profile MUST remain unknown or unevaluated for that scope.

**Primary later specification**

KRN-0002 — Role, Era, and Profile Model.

---

### KRN-INV-002 — Role, era, and profile remain explicit and independent

**Binding rule**

Every kernel profile MUST identify its role and era as distinct dimensions, and MUST remain distinguishable from both.

**Why the invariant exists**

Role describes intended contribution. Era describes compatibility envelope. Profile identifies one exact realization. Collapsing them prevents accurate comparison, reuse, supersession, and validation.

**Required observable behavior**

A conforming profile MUST:

- declare at least one role;
- declare its applicable era dimensions;
- preserve a stable profile identity;
- identify which requirements are family-wide, role-specific, era-specific, or pair-specific;
- avoid deriving role solely from hardware;
- avoid deriving era solely from a kernel version.

**Prohibited interpretations**

This invariant forbids treating:

- `x86_64` as a role;
- `assembly` as an era;
- one profile name as a complete compatibility description;
- a calendar year as a sufficient era declaration.

**Failure or uncertainty behavior**

An unresolved role or era dimension MUST remain explicit and MUST NOT be inferred from naming convention.

**Primary later specification**

KRN-0002.

---

### KRN-INV-003 — Kernel-profile identity exceeds a version string

**Binding rule**

A kernel profile MUST be attributable through all identity dimensions material to its declared behavior.

**Why the invariant exists**

The same kernel version can produce materially different behavior through patches, configuration, toolchain, modules, firmware assumptions, and role requirements.

**Required observable behavior**

Profile identity MUST include or reference the applicable:

- upstream lineage;
- Node source revision;
- patch-set revision;
- configuration revision;
- toolchain identity;
- ABI expectations;
- role;
- era;
- capability declarations;
- limitations;
- lifecycle revision.

**Prohibited interpretations**

This invariant forbids identifying a profile solely through:

- `uname`;
- a release number;
- a package filename;
- a branch name;
- a configuration nickname;
- a marketing label.

**Failure or uncertainty behavior**

If a material identity dimension cannot be established, the claimed profile identity MUST remain incomplete or unverified.

**Primary later specification**

KRN-0002.

---

### KRN-INV-004 — Source, profile, build, artifact, and deployed instance remain distinct

**Binding rule**

Kernel source identity, profile identity, build identity, artifact identity, and deployed-instance identity MUST remain distinguishable.

**Why the invariant exists**

One source state may produce several profiles and builds. One build may produce several artifacts. An artifact may be deployed several times. Their evidence and lifecycle differ.

**Required observable behavior**

A conforming implementation MUST preserve:

- which source produced a build;
- which profile the build claims;
- which build produced an artifact;
- which artifact was deployed;
- which deployed instance is being observed;
- applicable transformation and packaging history.

**Prohibited interpretations**

This invariant forbids treating:

- source checkout as a build;
- successful build as an accepted artifact;
- artifact filename as profile identity;
- executing instance as proof of claimed build;
- local installation as family-wide release.

**Failure or uncertainty behavior**

Missing linkage between identity layers MUST remain explicit and MUST prevent stronger identity claims.

**Primary later specification**

KRN-0002 and KRN-0005.

---

### KRN-INV-005 — Kernel and micro-OS identities remain distinct

**Binding rule**

A kernel profile and a micro-OS profile MUST retain separate primary identities even when distributed, validated, or deployed together.

**Why the invariant exists**

A kernel and userspace environment may evolve, be reused, replaced, or validated independently.

**Required observable behavior**

A conforming declaration MUST:

- identify the kernel profile separately;
- identify the micro-OS profile separately;
- identify the declaration that relates them;
- avoid deriving either identity solely from the other;
- permit one kernel to participate in multiple demonstrated pairings;
- permit one micro-OS to participate in multiple demonstrated pairings.

**Prohibited interpretations**

This invariant forbids:

- using one undifferentiated “OS image identity” when kernel and micro-OS revisions matter independently;
- assuming any micro-OS built beside a kernel is compatible;
- assuming kernel replacement leaves the pairing unchanged.

**Failure or uncertainty behavior**

If either identity is unresolved, pair compatibility MUST remain unresolved.

**Primary later specification**

KRN-0003 — Kernel and Micro-OS Compatibility Model.

---

### KRN-INV-006 — Subordinate micro-OS identities are created only when independently governed

**Binding rule**

A micro-OS MUST use one primary `MicroOsProfileIdentity`. A subordinate component MUST receive an independent identity when its independent lifecycle materially affects compatibility, replacement, reuse, selection, distribution, maintenance, validation, or revocation.

**Why the invariant exists**

Too few identities hide meaningful changes. Too many identities create unnecessary complexity and accidental fragmentation.

**Required observable behavior**

The profile MUST evaluate whether components such as an initramfs, root filesystem, service bundle, package snapshot, toolchain bundle, or firmware bundle are independently:

- versioned;
- replaceable;
- reusable;
- selected;
- distributed;
- maintained;
- validated;
- revoked;
- referenced by compatibility requirements.

**Prohibited interpretations**

This invariant forbids:

- assigning identities to every file merely because it exists;
- hiding an independently replaceable component inside the primary identity;
- changing a separately validated component without updating its identity.

**Failure or uncertainty behavior**

Where independent-governance status is unresolved, the profile MUST document the ambiguity and MUST NOT claim identity-equivalent replacement.

**Primary later specification**

KRN-0003.

---

### 6.2 Compatibility and required components

### KRN-INV-007 — Compatibility is a scoped declaration, not an assumption

**Binding rule**

Kernel/micro-OS compatibility MUST exist only through an identified, revisioned, scoped declaration with explicit assumptions, requirements, limitations, and evidence boundaries.

**Why the invariant exists**

Co-location, common build origin, or successful startup does not prove that a kernel and micro-OS satisfy their intended contract.

**Required observable behavior**

A compatibility declaration MUST identify or reference:

- the kernel profile;
- the micro-OS profile;
- applicable purpose or role;
- required components;
- firmware and boot assumptions;
- ABI expectations;
- required capabilities;
- known limitations;
- validation scope;
- evidence;
- declaration revision and lifecycle.

**Prohibited interpretations**

This invariant forbids assuming compatibility because:

- both components were produced by the same build system;
- they exist on the same medium;
- the kernel reaches userspace;
- basic commands execute;
- no incompatibility has yet been reported.

**Failure or uncertainty behavior**

Missing, stale, conflicting, or incomplete compatibility evidence MUST remain explicit.

**Primary later specification**

KRN-0003.

---

### KRN-INV-008 — A compatibility declaration is not BOOT pair evaluation

**Binding rule**

A KRN compatibility declaration MUST remain distinct from BOOT’s evaluation of one selected kernel/micro-OS pair.

**Why the invariant exists**

KRN declarations are reusable profile facts. BOOT evaluation is session-, evidence-, component-, authority-, and generation-specific.

**Required observable behavior**

A conforming implementation MUST distinguish:

- declared compatibility;
- selected pair;
- selected component-set instance;
- local availability;
- local integrity;
- BOOT evaluation;
- BOOT assembly-generation state;
- BOOT progression.

**Prohibited interpretations**

This invariant forbids treating:

- a valid declaration as automatic BOOT eligibility;
- a prior BOOT evaluation as permanently valid;
- KRN validation as authority;
- BOOT selection as modification of the KRN declaration.

**Failure or uncertainty behavior**

BOOT failure or deferral MUST NOT silently invalidate the reusable KRN declaration outside the evaluated scope. A KRN declaration MUST NOT override a BOOT failure.

**Primary later specification**

KRN-0003 and BOOT-0005.

---

### KRN-INV-009 — Required-component declarations are not selected component sets

**Binding rule**

A KRN required-component declaration MUST remain distinct from BOOT’s selected component-set instance.

**Why the invariant exists**

A declaration states reusable requirements. BOOT selects actual components for one governed assembly generation.

**Required observable behavior**

A conforming implementation MUST preserve:

- declaration identity and revision;
- required component classes or identities;
- substitution constraints;
- validation requirements;
- the identity of each selected component;
- the BOOT generation in which selection occurs;
- the result of evaluating the selected instance.

**Prohibited interpretations**

This invariant forbids:

- treating a requirement as proof that a component is present;
- treating a discovered component as selected;
- treating a selected component as satisfying the declaration before evaluation;
- modifying a KRN declaration to record one BOOT selection.

**Failure or uncertainty behavior**

Unresolved selection, substitution, availability, or evaluation MUST remain explicit.

**Primary later specification**

KRN-0003 and BOOT-0005.

---

### KRN-INV-010 — Capability claims are scoped and evidence-backed

**Binding rule**

A kernel-profile capability claim MUST identify its subject, scope, revision, evidence boundary, required conditions, and limitation state.

**Why the invariant exists**

Capability is not a binary property. A mechanism may exist but be unavailable, restricted, untested, degraded, unsafe for a role, or incompatible with the selected micro-OS.

**Required observable behavior**

A profile MUST distinguish, where applicable:

- required and demonstrated;
- required but not demonstrated;
- optional and demonstrated;
- optional but unavailable;
- restricted;
- degraded;
- unsupported;
- unknown;
- unevaluated;
- conflicting.

**Prohibited interpretations**

This invariant forbids:

- equating compiled-in support with demonstrated capability;
- equating module presence with usable capability;
- equating hardware presence with supported capability;
- omitting failed or unevaluated conditions from the profile.

**Failure or uncertainty behavior**

Insufficient evidence MUST produce an unknown or unevaluated claim rather than supported status.

**Primary later specification**

KRN-0002, KRN-0003, and KRN-0005.

---

### KRN-INV-011 — Known limitations are part of profile meaning

**Binding rule**

Known limitations MUST be declared as part of the kernel profile and relevant compatibility declarations.

**Why the invariant exists**

A profile described only through successful features creates misleading eligibility and hides failure boundaries.

**Required observable behavior**

Limitations MUST identify, where applicable:

- affected role;
- affected era dimension;
- hardware class;
- capability;
- severity or restriction class;
- workaround assumptions;
- validation boundary;
- maintenance implications;
- conditions requiring review or revalidation.

**Prohibited interpretations**

This invariant forbids:

- omitting a known limitation because ordinary operation usually succeeds;
- treating a workaround as removal of the limitation;
- treating a documented limitation as automatic BOOT acceptance;
- hiding security limitations as performance notes.

**Failure or uncertainty behavior**

Uncharacterized behavior MUST remain unknown or restricted until evaluated.

**Primary later specification**

KRN-0002, KRN-0003, and KRN-0005.

---

### 6.3 Mechanism control, ownership, and authority

### KRN-INV-012 — Physical control location does not determine architectural ownership

**Binding rule**

The component that physically controls a mechanism MUST NOT be assumed to own the semantic object, operation, authority, or lifecycle implemented through that mechanism.

**Why the invariant exists**

Kernel, userspace, firmware, device runtime, and hardware may each control different portions of one operation.

**Required observable behavior**

A conforming design MUST identify, where applicable:

- mechanism controller;
- semantic owner;
- authorizing domain;
- evidence producer;
- operation verifier;
- lifecycle owner.

**Prohibited interpretations**

This invariant forbids claims that:

- kernel control creates kernel ownership;
- direct userspace control creates userspace authority;
- firmware control creates semantic truth;
- device-runtime control bypasses Node architecture.

**Failure or uncertainty behavior**

If ownership is unresolved, the mechanism MUST NOT assign itself the missing authority.

**Primary later specification**

KRN-0004 — Adjacent Architecture Boundaries.

---

### KRN-INV-013 — Kernel mediation does not create architectural authority

**Binding rule**

A kernel MUST NOT gain semantic or policy authority merely because an operation passes through a system call, driver, security hook, namespace, cgroup, scheduler, filesystem, or other kernel mechanism.

**Why the invariant exists**

Most operations require kernel mediation. Treating mediation as authority would make the kernel an undefined super-owner.

**Required observable behavior**

A conforming implementation MUST:

- preserve the external authority decision;
- preserve operation scope;
- distinguish enforcement from authorization;
- preserve provider-specific failure;
- avoid relabeling delegated action as KRN-owned action.

**Prohibited interpretations**

This invariant forbids treating:

- filesystem permission as complete installer authority;
- device access as backend authority;
- scheduler control as work authority;
- security-hook approval as universal semantic acceptance.

**Failure or uncertainty behavior**

Unavailable or conflicting authority MUST remain unavailable or conflicting.

**Primary later specification**

KRN-0004.

---

### KRN-INV-014 — Privileged, early, local, rescue, and emergency execution do not enlarge authority

**Binding rule**

Kernel privilege, root privilege, early execution, local execution, offline operation, rescue context, physical proximity, or emergency conditions MUST NOT create ownership or authority that did not otherwise exist.

**Why the invariant exists**

These contexts often provide broad technical access while reducing oversight.

**Required observable behavior**

A conforming implementation MUST:

- distinguish access from permission;
- apply applicable authority boundaries;
- preserve operation scope;
- preserve attribution;
- refuse or defer privileged work when authority cannot be established.

**Prohibited interpretations**

This invariant forbids claims that:

- local storage access grants mutation authority;
- boot media grants installation authority;
- emergency status grants unrestricted device use;
- kernel mode permits bypass of ACS, MEM, IMM, BOOT, or installer ownership.

**Failure or uncertainty behavior**

Urgency MAY change which authorized paths are considered. It MUST NOT manufacture authority.

**Primary later specification**

KRN-0004.

---

### KRN-INV-015 — Hardware discovery does not create Node registration

**Binding rule**

Detection or enumeration of hardware MUST NOT automatically create a Node participant, ACS endpoint, ACS relationship, backend registration, device role, or runtime admission.

**Why the invariant exists**

Physical presence establishes only that a possible resource may exist.

**Required observable behavior**

A conforming implementation MUST distinguish:

- physical detection;
- identity evidence;
- driver binding;
- capability evaluation;
- backend registration;
- ACS admission;
- runtime placement;
- operation authority.

**Prohibited interpretations**

This invariant forbids treating:

- PCI enumeration as backend registration;
- a device node as an ACS endpoint;
- a mapped BAR as a participant;
- a successful probe as runtime readiness;
- a discovered accelerator as available work capacity.

**Failure or uncertainty behavior**

Partially identified or unsupported hardware MUST remain partial, unknown, unsupported, or unavailable.

**Primary later specification**

KRN-0004.

---

### KRN-INV-016 — Driver availability and device access do not grant device-use authority

**Binding rule**

The presence of a driver, device node, mapping, handle, queue, firmware interface, or direct userspace control path MUST NOT independently authorize use of the device.

**Why the invariant exists**

Technical accessibility and semantic permission are separate.

**Required observable behavior**

A conforming implementation MUST preserve:

- device identity;
- capability status;
- applicable authority;
- ACS or backend admission;
- resource assignment;
- isolation requirements;
- lifecycle state;
- operation scope.

**Prohibited interpretations**

This invariant forbids:

- treating successful `open` or mapping as permission;
- treating root access as device-use authority;
- treating VFIO, UIO, DMA, or firmware access as backend registration;
- treating driver initialization as workload admission.

**Failure or uncertainty behavior**

Unavailable authority or admission MUST prevent broader use even when access mechanisms remain functional.

**Primary later specification**

KRN-0004.

---

### KRN-INV-017 — Resource availability is not admission or authority

**Binding rule**

Free CPU, memory, storage, bandwidth, device capacity, queue space, or scheduling opportunity MUST NOT independently authorize work or establish admission.

**Why the invariant exists**

Capacity describes what may be physically possible, not what is permitted or semantically appropriate.

**Required observable behavior**

A conforming implementation MUST distinguish:

- observed capacity;
- allocatable capacity;
- reserved capacity;
- admitted work;
- authorized work;
- assigned resources;
- actual usage;
- exhaustion or pressure state.

**Prohibited interpretations**

This invariant forbids:

- dispatching merely because a device is idle;
- using unallocated memory because it is free;
- bypassing budgets because capacity exists;
- converting capacity observation into a profile capability without validation.

**Failure or uncertainty behavior**

Unknown accounting or ownership MUST result in conservative restriction rather than opportunistic use.

**Primary later specification**

KRN-0004.

---

### KRN-INV-018 — Enforcement does not create authority

**Binding rule**

Kernel or runtime enforcement of a restriction, allocation, isolation boundary, or privileged operation MUST remain subordinate to the authority that granted the action.

**Why the invariant exists**

A component capable of enforcement may otherwise become an unreviewed policy owner.

**Required observable behavior**

Enforcement MUST preserve:

- authorizing domain;
- actor;
- target;
- scope;
- action;
- lifecycle;
- conditions;
- result;
- failure state.

**Prohibited interpretations**

This invariant forbids:

- a cgroup controller inventing resource policy;
- a security module inventing semantic authorization;
- an isolation mechanism broadening a containment request;
- technical inability to comply being reported as authority denial.

**Failure or uncertainty behavior**

Enforcement failure MUST remain distinct from authority denial and from semantic operation failure.

**Primary later specification**

KRN-0004.

---

### 6.4 Adjacent semantic boundaries

### KRN-INV-019 — Kernel communication mechanisms are not ACS objects

**Binding rule**

Sockets, numeric ports, queues, shared-memory regions, file descriptors, DMA channels, device handles, and transport sessions MUST NOT be treated as ACS participants, endpoints, ports, relationships, or logical connections merely because they implement communication.

**Why the invariant exists**

ACS identity and lifecycle must survive replacement of physical mechanisms.

**Required observable behavior**

A conforming implementation MUST preserve the mapping between:

- ACS object;
- concrete mechanism;
- binding identity;
- generation;
- admission;
- authority;
- lifecycle.

**Prohibited interpretations**

This invariant forbids:

- socket equals connection;
- TCP or UDP port equals ACS port;
- queue equals endpoint;
- transport reconnect equals new relationship;
- device handle equals participant identity.

**Failure or uncertainty behavior**

Mechanism loss MUST NOT automatically erase ACS identity or establish relationship failure without ACS evaluation.

**Primary later specification**

KRN-0004.

---

### KRN-INV-020 — Kernel storage and memory mechanisms are not MEM truth

**Binding rule**

Kernel-managed pages, files, mappings, caches, block writes, filesystem operations, storage devices, and persistent bytes MUST NOT automatically be treated as MEM-governed logical memory or as proof of a MEM outcome.

**Why the invariant exists**

Physical material and semantic memory have different identity, acceptance, durability, retention, and recovery rules.

**Required observable behavior**

A conforming implementation MUST distinguish:

- physical write;
- physical presence;
- MEM storage proposal;
- MEM acceptance;
- commitment;
- durability;
- availability;
- retention;
- deletion;
- recovery.

**Prohibited interpretations**

This invariant forbids treating:

- successful write as durability;
- file existence as accepted memory;
- cache hit as authoritative recall;
- block deletion as valid MEM deletion;
- mount success as memory recovery.

**Failure or uncertainty behavior**

Storage success or failure MUST be reported as mechanism evidence rather than fabricated MEM truth.

**Primary later specification**

KRN-0004.

---

### KRN-INV-021 — Kernel telemetry is observation, not IMM assessment or verdict

**Binding rule**

Kernel events, counters, logs, faults, interrupts, security hooks, health measurements, and performance data MUST remain observations or evidence inputs rather than IMM findings, verdicts, authority, or restoration decisions.

**Why the invariant exists**

Observation and interpretation require different ownership and evidence contracts.

**Required observable behavior**

Telemetry MUST preserve applicable:

- source;
- subject;
- scope;
- freshness;
- integrity status;
- transformation;
- uncertainty;
- loss or sampling condition.

**Prohibited interpretations**

This invariant forbids:

- fault equals compromise;
- unusual resource use equals attack;
- absence of kernel alert equals safety;
- successful restart equals recovery;
- normal metrics equal restoration.

**Failure or uncertainty behavior**

Missing or conflicting telemetry MUST remain missing or conflicting and MUST NOT become a healthy verdict.

**Primary later specification**

KRN-0004.

---

### KRN-INV-022 — Kernel launch and mechanism success do not establish later completion

**Binding rule**

Successful kernel load, userspace launch, process start, device initialization, system-call completion, build completion, transfer completion, or reboot MUST NOT independently establish a later semantic stage.

**Why the invariant exists**

Technical stages are necessary evidence but do not prove BOOT progression, installation, activation, recovery, or runtime readiness.

**Required observable behavior**

A conforming implementation MUST keep distinct:

- kernel loaded;
- micro-OS started;
- mechanism available;
- provider operation active;
- build completed;
- artifact produced;
- artifact accepted;
- installation completed;
- activation accepted;
- recovery verified;
- runtime handoff eligible;
- runtime ready.

**Prohibited interpretations**

This invariant forbids:

- booted equals compatible;
- compiled equals accepted;
- installed equals activated;
- process running equals ready;
- reboot completed equals recovered.

**Failure or uncertainty behavior**

An indeterminate or partially completed mechanism MUST remain partial or indeterminate for the owning semantic operation.

**Primary later specification**

KRN-0003 and KRN-0004.

---

### KRN-INV-023 — Provider-produced outputs remain candidates until accepted by the owning architecture

**Binding rule**

A physical output produced by a build environment, ASM, toolchain, package system, or other provider MUST remain a provider-attributed candidate until the architecture owning acceptance evaluates and accepts it.

**Why the invariant exists**

Production does not establish identity, integrity, compatibility, authority, or eligibility.

**Required observable behavior**

A candidate output MUST preserve:

- producer;
- operation identity;
- build or generation context;
- source and input references;
- claimed profile;
- validation state;
- unresolved limitations;
- acceptance state;
- accepting authority when accepted.

**Prohibited interpretations**

This invariant forbids:

- ASM output equals BOOT assembly member;
- compiler success equals release;
- signed output equals compatible output;
- copied output equals installed output;
- cached artifact equals current artifact.

**Failure or uncertainty behavior**

Outputs with unresolved identity or validation MUST remain candidates and MUST NOT be silently promoted.

**Primary later specification**

KRN-0003, KRN-0004, ASM, and BOOT-0005.

---

### KRN-INV-024 — Durable production mutation remains installer-owned

**Binding rule**

KRN, kernel-profile declarations, and ASM MUST NOT absorb ownership of durable production installation mutation.

**Why the invariant exists**

Kernel and assembly mechanisms may possess technical write access, but installer mutation requires separate authority, scope, rollback, and result contracts.

**Required observable behavior**

A conforming design MUST distinguish:

- candidate-output production;
- installation request;
- installer authority;
- target identity;
- durable mutation;
- activation staging;
- rollback;
- installer result;
- BOOT evaluation.

**Prohibited interpretations**

This invariant forbids:

- build tool writes equal installation;
- filesystem access grants installer authority;
- ASM mutating production targets;
- kernel write success equals installer completion;
- BOOT coordination becoming installer mutation.

**Failure or uncertainty behavior**

If installer authority or target state is unavailable, production mutation MUST NOT proceed under a KRN or ASM label.

**Primary later specification**

KRN-0004 and applicable BOOT and installer architecture.

---

### 6.5 Build authority, state truthfulness, and boundedness

### KRN-INV-025 — Node-targeted build work requires applicable BOOT authority

**Binding rule**

Build work intended to produce Node-targeted artifacts, installations, activation material, recovery material, or propagatable outputs MUST require applicable BOOT authority.

**Why the invariant exists**

Compilation can create artifacts capable of altering a node or entering a governed assembly even before installation begins.

**Required observable behavior**

A conforming build operation MUST identify:

- purpose;
- authority requirement;
- authority reference;
- target scope;
- permitted outputs;
- mutation prohibition;
- propagation prohibition;
- operation identity;
- result.

Authority-not-required compilation MUST be explicitly permitted by an owning contract and limited to bounded diagnostic, conformance, or disposable test work.

**Prohibited interpretations**

This invariant forbids assuming authority is unnecessary because:

- work is local;
- source is public;
- installation is not immediate;
- outputs are unsigned;
- the build is called experimental;
- the operator has root access.

**Failure or uncertainty behavior**

Unresolved authority MUST cause deferral, refusal, or restriction to an explicitly permitted authority-not-required operation.

**Primary later specification**

KRN-0004, ASM-0004, and BOOT-0005.

---

### KRN-INV-026 — Unknown, unsupported, unavailable, degraded, stale, invalid, and conflicting remain explicit

**Binding rule**

Materially different non-success and uncertainty states MUST remain distinguishable.

**Why the invariant exists**

Collapsing states produces unsafe assumptions and false compatibility.

**Required observable behavior**

Profiles and KRN-facing results MUST distinguish, where applicable:

- unknown;
- unevaluated;
- unsupported;
- unavailable;
- degraded;
- stale;
- invalid;
- conflicting;
- restricted;
- failed;
- indeterminate.

**Prohibited interpretations**

This invariant forbids:

- unknown equals supported;
- unavailable equals unsupported;
- unsupported equals failed;
- degraded equals compromised;
- stale equals current;
- conflicting equals newest wins;
- absent evidence equals absence of a problem.

**Failure or uncertainty behavior**

Missing information MUST NOT be converted into positive or negative confirmation.

**Primary later specification**

KRN-0002, KRN-0003, and KRN-0005.

---

### KRN-INV-027 — KRN operations and evidence remain attributable and revisioned

**Binding rule**

Every authoritative KRN declaration, transition, validation result, or conformance result MUST identify its subject, producer, revision, scope, operation, evidence boundary, and prior state where applicable.

**Why the invariant exists**

Kernel-family decisions may be retried, reproduced, superseded, or evaluated by several providers.

**Required observable behavior**

A conforming record MUST preserve applicable:

- profile identity;
- declaration identity;
- operation identity;
- attempt identity;
- producer;
- revision;
- prior revision;
- evidence references;
- transformation history;
- result scope;
- authoritative-change indicator.

**Prohibited interpretations**

This invariant forbids:

- log line equals authoritative state;
- wall-clock time alone equals revision;
- repeated result equals independent evidence;
- process restart creates a new semantic operation;
- newest record automatically wins a conflict.

**Failure or uncertainty behavior**

Loss of attribution or revision information MUST prevent authoritative promotion.

**Primary later specification**

KRN-0002, KRN-0003, and KRN-0005.

---

### KRN-INV-028 — KRN work and validation remain bounded

**Binding rule**

Kernel-profile evaluation, compatibility checking, evidence retention, parsing, validation, and reporting MUST operate within explicit finite limits.

**Why the invariant exists**

KRN may operate during assembly, rescue, degraded hardware, or resource pressure where unbounded behavior can destabilize the substrate.

**Required observable behavior**

Conforming implementations MUST define applicable limits for:

- input size;
- component count;
- evidence count;
- dependency depth;
- identifier length;
- issue count;
- memory use;
- processing time;
- retry count;
- output size;
- retained history.

**Prohibited interpretations**

This invariant forbids:

- unbounded manifest parsing;
- unlimited dependency traversal;
- unlimited diagnostic accumulation;
- infinite retry;
- silent truncation reported as complete success;
- resource use growing merely because more hardware is available.

**Failure or uncertainty behavior**

Resource exhaustion MUST be a first-class explicit result and MUST NOT fabricate success.

**Primary later specification**

KRN-0003 and KRN-0005.

---

### KRN-INV-029 — Conformance must not depend on graphical or local human interaction

**Binding rule**

KRN profile declaration, validation, state reporting, and conformance MUST remain usable in minimal headless environments without requiring a graphical interface, display server, desktop session, or local human-readable console.

**Why the invariant exists**

Node targets minimal and potentially unattended substrates.

**Required observable behavior**

A conforming implementation MUST provide machine-readable outcomes suitable for:

- local restricted environments;
- remote collection;
- automated validation;
- BOOT consumption;
- headless diagnostics;
- bounded persistence or transfer.

Local human-readable output MAY exist as optional diagnostic behavior.

**Prohibited interpretations**

This invariant forbids:

- GUI-only validation;
- interactive prompts as the only failure path;
- treating absent local console as inability to report state;
- requiring the normal Node runtime for basic KRN conformance.

**Failure or uncertainty behavior**

Loss of optional human-readable output MUST NOT fabricate successful conformance or erase machine-readable failure state.

**Primary later specification**

KRN-0005 and role-specific conformance specifications.

---

### 6.6 Provenance, conformance, maintenance, and lifecycle

### KRN-INV-030 — Source, patches, configuration, toolchain, ABI, and licensing remain attributable

**Binding rule**

Every profile and build MUST preserve sufficient provenance to identify all material source, patch, configuration, toolchain, ABI, and licensing inputs.

**Why the invariant exists**

Kernel behavior and distribution obligations cannot be reconstructed from version numbers or binaries alone.

**Required observable behavior**

A conforming profile or build MUST identify or reference:

- upstream source;
- Node source revision;
- patch provenance and order;
- configuration revision;
- compiler and linker;
- material build flags;
- module assumptions;
- ABI expectations;
- applicable license information;
- known incompatible or restricted material.

**Prohibited interpretations**

This invariant forbids:

- unattributed patch sets;
- manually altered binaries presented as reproducible builds;
- configuration drift without revision;
- toolchain substitution without review;
- successful build treated as license permission.

**Failure or uncertainty behavior**

Missing material provenance MUST prevent complete profile or build conformance.

**Primary later specification**

KRN-0002 and KRN-0005.

---

### KRN-INV-031 — Material change requires revision and revalidation

**Binding rule**

A material change to a profile, source, patch set, configuration, toolchain, ABI assumption, firmware assumption, required component, compatibility declaration, or maintenance condition MUST trigger an attributable revision and applicable revalidation.

**Why the invariant exists**

Prior evidence may no longer apply after a material input changes.

**Required observable behavior**

The system MUST:

- identify the changed dimension;
- preserve the prior identity;
- determine affected declarations and evidence;
- invalidate or restrict stale conclusions;
- perform required revalidation;
- record the new revision and result.

**Prohibited interpretations**

This invariant forbids:

- changing configuration under the same profile revision;
- compiler upgrades treated as immaterial by default;
- firmware changes silently inheriting compatibility;
- patch rebasing without provenance update;
- cached validation surviving an incompatible change.

**Failure or uncertainty behavior**

Until required revalidation completes, affected standing MUST remain unevaluated, stale, restricted, or otherwise explicitly unresolved.

**Primary later specification**

KRN-0005.

---

### KRN-INV-032 — Maintenance, security, supersession, retirement, revocation, and conflict remain explicit

**Binding rule**

Profile lifecycle and standing MUST remain explicit, attributable, scoped, and continuity-aware. A profile MUST NOT silently enter or return to ordinary eligibility.

**Why the invariant exists**

A technically functional profile may become unsuitable because maintenance, security support, validation, dependencies, or replacement status changed.

**Required observable behavior**

A conforming lifecycle model MUST:

- distinguish maturity, maintenance, succession, and conformance standing;
- identify restrictions and their scope;
- identify a successor when superseded;
- preserve historical profile identity;
- preserve reasons and evidence references;
- prevent silent reactivation after retirement or revocation;
- record unresolved cross-series conflict rather than choosing a convenient interpretation.

Aggregate `SystemProfileIdentity` ownership MUST remain deferred until assigned by deliberate architecture.

**Prohibited interpretations**

This invariant forbids:

- boot success restoring a revoked profile;
- artifact availability restoring a retired profile;
- supersession deleting history;
- deprecation equaling immediate revocation;
- one series silently deciding another series’ unresolved matter;
- KRN absorbing aggregate system-profile ownership.

**Failure or uncertainty behavior**

Conflicting lifecycle claims MUST remain conflicting until governed resolution. The narrower safe standing MUST apply where action cannot be deferred.

**Primary later specification**

KRN-0005.

## 7. Adjacent-authority preservation

Every conforming KRN profile and implementation MUST preserve the following boundaries.

| Domain | KRN or kernel mechanisms may | KRN and kernel mechanisms MUST NOT |
|---|---|---|
| ASM | Define the family-level identity of the assembly-kernel role and provide declared mechanisms | Own assembly-environment content, ASM observations, ASM candidate-output semantics, or ASM role-specific conformance |
| BOOT | Provide reusable profile, compatibility, component, capability, and lifecycle declarations | Own selected component sets, pair evaluation, assembly generations, progression, authority, installer handoff, activation, recovery, reconciliation, or runtime-handoff eligibility |
| ACS | Provide communication, device, scheduling, and enforcement mechanisms | Redefine participants, relationships, endpoints, ports, signals, admission, connection identity, capability, delegation, revocation, or ACS lifecycle |
| MEM | Provide memory, storage, filesystem, mapping, cache, and persistence mechanisms | Define logical memory identity, semantic acceptance, commitment, durability, retention, custody, deletion, reconstruction, or recovery truth |
| IMM | Produce observations and execute explicitly authorized enforcement | Convert telemetry into findings, recommendations, containment authority, recovery verification, restoration, or immune verdicts |
| Installer | Provide block, filesystem, device, process, and reboot mechanisms | Own durable production mutation, activation staging, rollback mechanics, or installer completion |
| Runtime | Provide processes, scheduling, isolation, devices, and system services | Define ordinary runtime placement policy, operation admission, semantic completion, or runtime readiness |
| Resource management | Expose and enforce resource mechanisms | Create allocation authority, budgets, reservations, or policy merely from observed capacity |
| Security providers | Expose protected mechanisms and consume verification results | Own raw-key custody, cryptographic identity policy, signing authority, or trust merely through kernel access |
| Device backends | Expose devices and mechanism evidence | Create backend identity, registration, tenancy, or work authority merely through discovery or access |
| Aggregate system profile | Contribute kernel-specific declarations | Claim ownership of aggregate `SystemProfileIdentity` |

## 8. Public conformance expectations

Public conformance evidence should demonstrate that:

1. no profile is treated as permanently universal;
2. role, era, and profile remain distinct;
3. profile identity exceeds a version string;
4. source, build, artifact, and deployed-instance identities remain linked but distinct;
5. kernel and micro-OS identities remain separate;
6. subordinate identities are created only when independently governed;
7. compatibility declarations are scoped and revisioned;
8. compatibility declarations do not become BOOT pair evaluations;
9. required-component declarations do not become selected component sets;
10. capability claims preserve scope, evidence, and limitations;
11. physical control does not create ownership;
12. kernel mediation and enforcement do not create authority;
13. privileged or early execution does not enlarge authority;
14. hardware discovery does not create Node registration;
15. driver availability and device access do not grant device-use authority;
16. resource availability does not create admission;
17. communication mechanisms do not become ACS objects;
18. physical storage does not become MEM truth;
19. telemetry does not become an IMM verdict;
20. kernel, build, process, and transfer success do not fabricate later completion;
21. provider outputs remain candidates until accepted;
22. durable production mutation remains installer-owned;
23. Node-targeted build work requires applicable BOOT authority;
24. uncertainty and non-success states remain distinct;
25. authoritative KRN records remain attributable and revisioned;
26. KRN work remains bounded under resource exhaustion;
27. conformance remains operable in minimal headless environments;
28. source, patch, configuration, toolchain, ABI, and license provenance remain attributable;
29. material changes trigger revision and revalidation;
30. maintenance and security state affect standing;
31. lifecycle and succession remain explicit;
32. cross-series conflicts and aggregate-profile ownership remain unresolved until deliberately assigned.

A profile does not conform merely because its ordinary boot path succeeds.

## 9. Prohibited interpretations

These invariants MUST NOT be interpreted to mean that:

- every profile must support every role;
- role or era uncertainty may be inferred from naming convention;
- one compatibility declaration guarantees BOOT progression;
- KRN owns assembly generations;
- KRN owns selected component sets;
- ASM candidate outputs are accepted artifacts;
- ASM may mutate production installation targets;
- BOOT pair evaluation rewrites KRN declarations;
- kernel privilege creates universal authority;
- a driver creates device-use authority;
- direct userspace control bypasses KRN or adjacent architecture;
- successful communication creates ACS acceptance;
- successful write creates MEM durability;
- telemetry creates IMM judgment;
- build completion creates release authority;
- installation creates activation;
- process liveness creates runtime readiness;
- a current security patch level compensates for false identity;
- a successor erases the superseded profile;
- private policy may weaken public invariants;
- unresolved aggregate system-profile ownership belongs to the first implementing series.

## 10. Initial architectural commitments

KRN-0001 establishes that:

1. the kernel family remains plural and scoped;
2. role, era, and profile remain explicit;
3. identity is attributable across source, build, artifact, and deployment;
4. kernel and micro-OS identities remain separate;
5. compatibility is declared rather than assumed;
6. reusable declarations remain distinct from BOOT-selected instances;
7. capability and limitation claims remain scoped and evidence-backed;
8. concrete mechanism control does not determine semantic ownership;
9. privilege, locality, early execution, rescue, and emergency conditions do not manufacture authority;
10. discovery, access, capacity, and enforcement remain distinct from registration, permission, admission, and ownership;
11. ACS, MEM, IMM, BOOT, installer, runtime, resource, security, and backend authority remain intact;
12. mechanism success does not fabricate semantic completion;
13. provider outputs remain candidates until accepted;
14. Node-targeted build work remains governed by applicable BOOT authority;
15. uncertainty, failure, and conflict remain honest;
16. KRN operations remain attributable, revisioned, bounded, and headless-capable;
17. source, patches, configuration, toolchain, ABI, and licensing remain attributable;
18. material changes require revalidation;
19. maintenance and security state affect eligibility;
20. lifecycle, replacement, retirement, and revocation remain explicit;
21. aggregate `SystemProfileIdentity` ownership remains deferred;
22. cross-series conflicts are recorded and reconciled rather than silently decided.

## Revision history

### Version 0.1 — 2026-07-18

- Established stable `KRN-INV-NNN` identifiers.
- Defined 32 common kernel-family invariants.
- Formalized family, role, era, profile, build, artifact, and deployment identity rules.
- Formalized kernel and micro-OS identity separation.
- Formalized compatibility-declaration and required-component boundaries.
- Preserved KRN, ASM, and BOOT terminology and ownership.
- Formalized mechanism-control, authority, and adjacent-architecture boundaries.
- Formalized capability, limitation, evidence, uncertainty, provenance, boundedness, and headless-operation requirements.
- Formalized build-authority and installer-mutation boundaries.
- Formalized maintenance, revalidation, supersession, retirement, revocation, and conflict behavior.
- Preserved deferred ownership of aggregate `SystemProfileIdentity`.
