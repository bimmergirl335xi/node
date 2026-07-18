# ASM-0000: Assembly-Kernel Role Charter

| Field | Value |
|---|---|
| Specification | ASM-0000 |
| Title | Assembly-Kernel Role Charter |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ASM-PUB |
| Parent series | KRN-PUB |
| Authors | Node |
| Last updated | 2026-07-18 |
| Approval | Pending review |
| Depends on | KRN-0000 through KRN-0005; BOOT-0000 through BOOT-0004; applicable ACS, MEM, IMM, installer, security-provider, resource-management, and platform architecture |
| Supersedes | ASM-0000 placeholder |
| Superseded by | None |
| Current confidence | High in role ownership, assembly-environment scope, hardware-inspection boundaries, candidate-output separation, authority separation, installer boundary, and cross-series ownership; detailed invariants, environment profiles, hardware mechanisms, toolchain contracts, resource limits, failure records, and conformance profiles remain assigned to ASM-0001 through ASM-0007 |

> **ASM exists to provide the bounded kernel and environment mechanisms through which authorized assembly work can inspect a substrate and produce candidate outputs without claiming that a Node has been assembled, installed, activated, recovered, or made ready.**

## Architectural-intent notice

This specification defines the public architecture charter for the Node assembly kernel and its accompanying assembly environment.

It establishes:

- the purpose of the ASM series;
- the assembly-kernel role;
- the assembly-environment boundary;
- the relationship between ASM and the parent KRN series;
- the relationship between ASM mechanism results and BOOT semantic results;
- the responsibilities of an assembly-kernel profile;
- the hardware-initialization and hardware-observation boundaries;
- the build-capability boundary;
- the ASM candidate-output boundary;
- the resource, workspace, cache, evidence, and staging direction;
- the installer and production-target mutation boundary;
- failure and uncertainty direction;
- headless and minimal-environment requirements;
- public/private publication constraints;
- the relationship among ASM-0000 through ASM-0007.

This specification does not define one kernel version, kernel configuration, micro-OS implementation, init system, service manager, filesystem layout, compiler, linker, package manager, build system, device runtime, network protocol, installer, artifact format, assembly-generation record, or production deployment policy.

Implementations may use different kernels, micro-OS profiles, build tools, filesystems, device runtimes, storage mechanisms, and process models.

Their observable behavior must preserve the ownership boundaries and distinctions established here.

---

## 1. Purpose

Node is intended to operate across hardware that may differ materially in:

- processor architecture;
- processor generation;
- memory topology;
- storage technology;
- firmware interfaces;
- bus topology;
- networking;
- accelerators;
- security hardware;
- available device drivers;
- available toolchains;
- supported kernel facilities;
- resource capacity.

A permanently universal kernel and userspace combination may not be suitable for every supported hardware era or assembly task.

The parent KRN series therefore defines a family of kernel profiles and micro-OS profiles rather than one permanently universal kernel realization.

ASM defines the first concrete role within that family.

The assembly kernel and assembly environment exist so that a bounded environment can:

1. start on a supported substrate;
2. initialize the mechanisms required for its own operation;
3. observe relevant hardware and platform state;
4. expose declared storage, filesystem, process, network, compiler, linker, test, and staging capabilities;
5. perform authorized provider operations;
6. produce candidate physical outputs and attributed evidence;
7. return those results to BOOT or another owning subsystem;
8. stop, fail, or narrow operation conservatively when requirements cannot be satisfied.

ASM does not decide that the resulting physical outputs constitute an accepted Node assembly.

That decision remains with BOOT.

---

## 2. Foundational boundary

The following distinctions are mandatory:

```text
assembly environment booted
    != Node assembled
    != installation completed
    != activation accepted
    != runtime ready
```

The complete ASM-facing progression is:

```text
assembly kernel entered
    → assembly environment mechanisms established
    → hardware observations produced
    → authorized provider operation attempted
    → ASM candidate outputs possibly produced
    → ASM mechanism result returned
```

The complete BOOT-facing progression remains separate:

```text
component-set instance selected
    → kernel/micro-OS pair evaluated
    → assembly generation established
    → installation eligibility evaluated
    → installer handoff authorized
    → installer result received
    → activation evaluated
    → recovery evaluated
    → runtime-handoff eligibility evaluated
    → runtime-owned readiness evaluated
```

No ASM result silently establishes any later BOOT or runtime result.

In particular:

```text
environment capability available
    != operation authorized

hardware observed
    != hardware usable

hardware usable
    != hardware authorized

source present
    != source accepted

toolchain available
    != toolchain eligible for a selected generation

compiler completed
    != candidate output accepted

candidate output exists
    != assembly generation established

test completed
    != BOOT validation satisfied

digest or signature result available
    != release accepted

installation target visible
    != installation target eligible

installer process launched
    != installation completed

control transferred
    != runtime ready
```

---

## 3. Authority and relationship to KRN

KRN is the parent architecture for the Node kernel family.

KRN owns the structures and semantics for:

- `KernelProfileIdentity`;
- `MicroOsProfileIdentity`;
- kernel-profile declarations;
- micro-OS-profile declarations;
- kernel/micro-OS compatibility declarations;
- required-component declarations;
- optional-component declarations;
- unsupported-component declarations;
- kernel-role declarations;
- hardware- and software-era declarations;
- profile revision;
- profile lineage;
- profile maintenance, supersession, restriction, deprecation, retirement, and replacement.

ASM consumes those structures.

ASM does not redefine them.

An ASM profile is a KRN kernel profile whose declared role is the assembly-kernel role.

The accompanying assembly environment is represented through one primary `MicroOsProfileIdentity`.

Subordinate micro-OS component identities are required only when the subordinate component is independently:

- versioned;
- replaceable;
- reusable;
- distributed;
- maintained;
- validated;
- revoked;
- or selected.

Incidental files, ordinary packages, and inseparable internal implementation details do not each require a separate profile identity merely because they exist.

A kernel/micro-OS compatibility declaration is a KRN object.

ASM may:

- report the active kernel-profile identity;
- report the active primary micro-OS-profile identity;
- report declared subordinate identities where applicable;
- expose the referenced compatibility declaration;
- report the environment capabilities actually observed.

ASM does not own the BOOT evaluation of a selected kernel and micro-OS pair.

The ownership of any aggregate `SystemProfileIdentity` remains intentionally deferred.

KRN, ASM, and BOOT must not independently absorb that unresolved ownership.

---

## 4. Authority and relationship to BOOT

BOOT owns the semantic lifecycle in which ASM participates.

BOOT owns:

- semantic assembly-operation identity;
- operation revision and progression;
- the selected component-set instance;
- required and optional component selection;
- kernel/micro-OS pair evaluation for the selected instance;
- assembly-plan semantics;
- assembly-generation identity;
- assembly-generation state;
- authority evaluation;
- source and release acceptance;
- artifact acceptance;
- compatibility acceptance;
- installation eligibility;
- installer handoff;
- activation;
- recovery;
- reconciliation;
- reboot preparation;
- runtime-handoff eligibility.

ASM consumes opaque BOOT references where the underlying BOOT schemas are not yet stable.

ASM must not duplicate or reinterpret those references as ASM-owned semantic records.

For each Node-targeted provider operation, ASM must receive or resolve an applicable BOOT authority result.

Node-targeted build work includes work intended to produce, modify, test, package, or validate material proposed for incorporation into a Node assembly generation.

Such work must not proceed merely because:

- the source is local;
- the source was present on the boot medium;
- the toolchain is available;
- the process has operating-system administrator access;
- the assembly environment booted successfully;
- the operation appears non-destructive;
- the target hardware is locally attached;
- a previous similar operation succeeded.

An owning contract may explicitly declare that a narrowly scoped compilation operation requires no additional BOOT authority.

Such an authority-not-required operation must be limited to bounded:

- diagnostics;
- compiler probing;
- feature detection;
- disposable test compilation;
- temporary compatibility experiments;
- conformance tests that cannot enter a Node generation without a later authorized operation.

Authority-not-required compilation must not produce material that is silently:

- promoted;
- installed;
- activated;
- executed as normal Node code;
- retained as an accepted artifact;
- propagated;
- or incorporated into an assembly generation.

A local ASM provider operation remains distinct from the BOOT semantic operation that requested or authorized it.

ASM must preserve correlation without taking ownership of BOOT operation identity.

---

## 5. Normative language

The terms **MUST** and **MUST NOT** describe mandatory architectural requirements.

The terms **SHOULD** and **SHOULD NOT** describe strong recommendations. A departure requires documented justification and must not violate a mandatory requirement.

The term **MAY** describes permitted behavior.

Conformance depends on observable behavior rather than component names.

A component does not become ASM merely because it:

- runs from installation media;
- executes before the normal runtime;
- has root access;
- can access a compiler;
- can inspect hardware;
- writes files;
- produces a kernel image;
- carries `assembly`, `build`, `bootstrap`, or `installer` in its name.

Likewise, a result does not become a BOOT assembly result merely because ASM produced it.

---

## 6. Core terminology

### 6.1 ASM

**ASM** is the Node architecture series governing the assembly-kernel role and the capabilities required from its accompanying assembly environment.

ASM defines role and mechanism boundaries.

ASM is not one executable, process, kernel tree, micro-OS image, script, build tool, or repository directory.

### 6.2 Assembly kernel

The **assembly kernel** is a KRN kernel profile whose declared role is to support the assembly environment.

The assembly kernel supplies or mediates the kernel-level mechanisms required for:

- processor execution;
- memory management;
- interrupts and exceptions;
- timing;
- device access;
- block access;
- filesystems;
- networking;
- process or task execution;
- isolation;
- resource enforcement;
- machine-readable reporting;
- shutdown or return of control.

The exact location of each mechanism may vary by profile.

### 6.3 Assembly environment

The **assembly environment** is the minimal custom micro-OS environment accompanying an assembly-kernel profile.

It supplies the userspace and supporting facilities required to:

- inspect the substrate;
- access authorized inputs;
- execute build and test tools;
- manage bounded workspaces;
- produce candidate outputs;
- retain required evidence;
- report mechanism results;
- stop or return control conservatively.

The assembly environment is not:

- an assembled Node;
- an assembly generation;
- the installer;
- BOOT;
- the normal runtime;
- proof of recovery.

### 6.4 Assembly-kernel profile

An **assembly-kernel profile** is a KRN kernel-profile declaration assigning the assembly-kernel role and describing its supported era, platform scope, required capabilities, optional capabilities, unsupported capabilities, and associated compatibility declarations.

ASM does not define the common profile structure.

### 6.5 Assembly-environment profile

An **assembly-environment profile** is the ASM use of a KRN micro-OS-profile declaration for the assembly environment.

The profile describes the environment capabilities required or offered by that micro-OS realization.

### 6.6 Assembly station

An **assembly station** is informal shorthand for the physical or virtual substrate currently running an assembly kernel and assembly environment.

The term does not create:

- participant identity;
- ACS admission;
- authority;
- ownership of the substrate;
- installation eligibility;
- assembly-generation identity.

### 6.7 ASM provider operation

An **ASM provider operation** is one bounded execution attempt through which ASM mechanisms perform requested inspection, build, packaging, testing, or staging work.

An ASM provider operation must remain correlated with, but distinct from:

- the BOOT semantic operation;
- the BOOT operation attempt;
- a process;
- a thread;
- a compiler process;
- a linker process;
- a transport connection;
- an ACS connection;
- an installer operation.

ASM provider-operation state is a provider-attributed mechanism result.

BOOT should represent active local build or selection work using a provider-attributed state such as `PROVIDER_OPERATION_ACTIVE`, rather than a state implying that BOOT itself performs the build.

### 6.8 Environment capability report

An **environment capability report** is an ASM-produced description of which declared assembly-environment mechanisms are:

- available;
- constrained;
- degraded;
- unavailable;
- unsupported;
- unknown;
- or not applicable.

A capability report does not authorize use of the capability.

### 6.9 Hardware observation

A **hardware observation** is an attributed result describing what ASM could establish about one physical or virtual hardware subject.

An observation is not:

- durable identity;
- compatibility acceptance;
- authorization;
- installation-target eligibility;
- health certification;
- proof of absence where inspection was incomplete.

### 6.10 Mechanism result

A **mechanism result** is an ASM-scoped outcome from invoking or evaluating one declared mechanism.

Examples include:

- filesystem mounted;
- device enumeration completed;
- compiler process completed;
- linker failed;
- test timed out;
- workspace exhausted;
- candidate output emitted;
- cleanup incomplete.

A mechanism result does not independently establish a BOOT semantic result.

### 6.11 ASM candidate output

An **ASM candidate output** is a physical or logical output produced through an ASM provider operation but not yet accepted by BOOT into an assembly generation.

Candidate outputs may include:

- object files;
- libraries;
- executables;
- kernel images;
- kernel modules;
- initramfs images;
- device trees;
- package files;
- archives;
- filesystem trees;
- candidate disk-image files;
- generated configuration;
- manifests;
- reports;
- provenance records;
- test results.

An ASM candidate output is not automatically:

- an accepted artifact;
- an accepted release;
- a complete component;
- an assembly generation;
- installation-eligible;
- activation-eligible;
- executable as normal Node software.

### 6.12 ASM output staging

**ASM output staging** is the bounded workspace area in which ASM places candidate outputs pending collection, evaluation, rejection, or disposal.

ASM output staging is not BOOT `GENERATION_STAGED`.

The BOOT state begins only when BOOT accepts applicable candidate outputs into an identified assembly-generation context.

### 6.13 Resource outcome

A **resource outcome** reports whether the resources required by an ASM provider operation were:

- sufficient;
- constrained;
- exhausted;
- unavailable;
- unsupported;
- or unresolved.

Resource sufficiency does not create authority or semantic acceptance.

---

## 7. ASM responsibilities

ASM owns the architecture of the assembly-kernel role and assembly-environment mechanisms.

### 7.1 Establish the assembly environment

ASM must define the capabilities required for an assembly environment to become operational within its declared profile.

This includes applicable mechanisms for:

- executable startup;
- processor execution;
- memory access;
- interrupt and exception handling;
- timing;
- storage access;
- filesystem access;
- process or task execution;
- machine-readable results;
- bounded diagnostics;
- controlled shutdown or return of control.

The environment must state which required mechanisms were established and which remain unavailable or unresolved.

### 7.2 Initialize required local mechanisms

ASM must initialize, or obtain initialized access to, the hardware and platform mechanisms required for the selected assembly-environment profile.

ASM does not need to initialize every device it observes.

Initialization may be performed by:

- firmware;
- a bootloader;
- the kernel;
- a kernel driver;
- assembly userspace;
- a device runtime;
- a security provider;
- ACS;
- another authorized subsystem.

ASM must preserve the responsible provider.

### 7.3 Inspect the local substrate

ASM owns the mechanisms for bounded hardware and platform inspection needed by assembly work.

Inspection may address:

- processor architecture and topology;
- memory capacity and topology;
- storage devices and controllers;
- filesystem capabilities;
- buses and attached devices;
- network interfaces;
- accelerators;
- security devices;
- firmware interfaces;
- platform timers;
- power and thermal facilities;
- virtualization;
- driver and runtime availability.

ASM must preserve unknown, partial, conflicting, unsupported, and unavailable results.

### 7.4 Provide build mechanisms

ASM must define the categories of build mechanisms that an assembly environment may be required to provide.

These may include:

- source access;
- archive extraction;
- dependency preparation;
- compilers;
- assemblers;
- linkers;
- archivers;
- object inspection;
- build-system execution;
- cross-compilation;
- kernel construction;
- module construction;
- device-tree compilation;
- micro-OS packaging;
- package generation;
- candidate image creation;
- static analysis;
- testing;
- emulation;
- isolated hardware validation;
- provenance capture.

Exact mandatory tools remain profile-defined and are refined by ASM-0004.

### 7.5 Provide bounded workspaces and caches

ASM owns the mechanism requirements for:

- source areas;
- active workspaces;
- intermediate output;
- caches;
- evidence retention;
- candidate-output staging;
- cleanup;
- quarantine;
- failure-report reserve.

Every facility must remain finite and attributable.

### 7.6 Execute authorized provider operations

ASM may perform a Node-targeted provider operation only when applicable BOOT authority and required external prerequisites are available.

ASM must:

- preserve the opaque BOOT operation reference;
- create a distinct local attempt reference;
- apply the declared resource limits;
- invoke only the requested mechanisms;
- preserve input and output lineage;
- produce an explicit final mechanism result;
- preserve partial and indeterminate effects;
- avoid broader work than the authorized scope.

### 7.7 Produce evidence and candidate outputs

ASM owns physical production and local reporting of:

- environment capability reports;
- hardware observations;
- mechanism results;
- resource outcomes;
- candidate outputs;
- provider-attributed verification results;
- failure records;
- cleanup state;
- unresolved limitations.

ASM does not accept those results into an assembly generation.

### 7.8 Support conservative termination

ASM must retain enough bounded capability to:

- stop further build work;
- terminate or isolate processes;
- report failures;
- identify partial output;
- quarantine incomplete output;
- release or report unreleased resources;
- sync or unmount ASM-owned writable facilities where applicable;
- preserve required evidence;
- return control to BOOT or another owning caller.

---

## 8. Hardware initialization boundary

ASM distinguishes hardware needed for the assembly environment from hardware that is relevant only as observation.

### 8.1 Hardware required for ASM operation

An assembly-kernel profile must declare the hardware classes required to operate its environment.

These commonly include:

- one or more processors;
- usable main memory;
- interrupt and exception facilities;
- a bounded timing facility;
- the boot or root medium;
- required workspace storage or RAM-backed workspace;
- required result-output storage or transport;
- buses required to reach those facilities.

A profile may additionally require:

- networking;
- persistent cache storage;
- security hardware;
- additional processors;
- NUMA support;
- accelerators;
- device-vendor runtimes;
- platform-management facilities.

Those additional requirements must be explicit.

### 8.2 Hardware observed but not initialized

A device may be observed without being initialized for use.

Observation-only hardware may include:

- production installation targets;
- existing Node system disks;
- accelerators not required by the current provider operation;
- runtime-specific peripherals;
- sensors or actuators;
- unsupported devices;
- devices reserved for another subsystem;
- devices prohibited by authority or safety policy.

ASM must not initialize or activate observation-only hardware merely to improve the completeness of its report.

### 8.3 Required hardware-state distinctions

ASM hardware reporting must preserve distinctions equivalent to:

```text
not observed
observed
partially identified
identified
initialization not attempted
initialization requested
initialized
accessible
usable for declared mechanism
constrained
unavailable
unsupported
failed
authorization unresolved
authorized for declared operation
prohibited
```

These meanings must not be collapsed into a single `DEVICE_OK` result.

### 8.4 Hardware observation does not grant authority

Hardware presence, enumeration, initialization, accessibility, or successful testing does not establish:

- participant identity;
- ACS membership;
- device-use authority;
- installation authority;
- target eligibility;
- assembly-generation eligibility;
- runtime readiness.

---

## 9. Mechanism-control boundary

ASM architecture defines required mechanisms and observable results.

It does not require every mechanism to reside in the kernel.

A mechanism may be controlled by:

- firmware;
- a bootloader;
- the assembly kernel;
- a kernel driver;
- assembly userspace;
- a device runtime;
- a security provider;
- ACS;
- a resource manager;
- an installer;
- another authorized subsystem.

The physical controller and semantic owner must remain separately identifiable.

Examples:

| Mechanism | Possible physical controller | Semantic owner |
|---|---|---|
| CPU scheduling | Kernel | Resource-management and requesting-operation contracts |
| Virtual memory | Kernel | Kernel mechanism under resource limits |
| Network byte movement | Kernel and transport provider | Transport |
| ACS connection use | ACS implementation | ACS |
| Signature verification | Security provider | Security provider |
| Compiler invocation | Assembly userspace | ASM mechanism under BOOT authority |
| Candidate-output production | Assembly userspace | ASM |
| Assembly-generation acceptance | None within ASM | BOOT |
| Production disk mutation | Installer | Installer |
| Activation staging | Installer | Installer and BOOT lifecycle |
| Runtime readiness | Normal runtime | Normal runtime |

Technical co-location does not transfer ownership.

An installer, security provider, or ACS adapter may execute within the same assembly environment.

That does not make its provider-owned results ASM results.

---

## 10. Build-capability boundary

ASM owns the capability to perform build mechanisms.

BOOT owns why a Node-targeted build is permitted and what its result means.

### 10.1 Input capabilities

An assembly environment may provide bounded access to:

- source trees;
- packages;
- toolchains;
- build descriptions;
- configuration;
- generated inputs;
- external artifact references;
- opaque authority and operation references.

Input presence does not establish acceptance or permission.

### 10.2 Toolchain capabilities

An assembly-environment profile may declare support for:

- native compilation;
- cross-compilation;
- several target architectures;
- several toolchain generations;
- target-specific assemblers;
- linkers;
- object utilities;
- device-specific code generation;
- kernel and module build systems;
- micro-OS packaging;
- reproducibility evaluation.

The newest available toolchain is not automatically the selected or eligible toolchain.

### 10.3 Controlled build execution

Build work must remain bounded in:

- process count;
- thread count;
- CPU use;
- memory use;
- temporary storage;
- file count;
- descriptor count;
- path depth;
- input size;
- output size;
- diagnostic size;
- network use;
- execution time;
- retry count;
- recursion;
- concurrency.

A build operation exceeding its limits must produce an explicit resource or bounds outcome.

### 10.4 Testing

ASM may mechanically run tests and report their results.

Testing may include:

- compiler feature probes;
- link tests;
- static analysis;
- unit tests;
- integration tests;
- emulated boot tests;
- virtual-machine tests;
- module-load tests;
- authorized device tests;
- reproducibility comparisons;
- size and dependency checks.

A successful ASM test is evidence.

BOOT or another owning evaluator decides whether that evidence satisfies a required validation contract.

### 10.5 Provenance

ASM must be capable of preserving applicable lineage for:

- the BOOT operation reference;
- the ASM provider-operation attempt;
- the active kernel profile;
- the active primary micro-OS profile;
- subordinate profile identities where applicable;
- the compatibility-declaration reference;
- source references;
- toolchain references;
- configuration;
- environment capabilities;
- hardware observations;
- cache participation;
- candidate outputs;
- tests;
- resource outcomes;
- provider results;
- warnings and failures.

ASM does not convert a source revision into a security digest or release identity.

---

## 11. ASM candidate-output boundary

ASM candidate outputs remain pre-semantic physical results.

### 11.1 Candidate-output production

ASM may produce candidate outputs only within:

- the requested provider-operation scope;
- applicable BOOT authority;
- the selected resource limits;
- the declared output contract;
- applicable classification and disclosure restrictions.

### 11.2 Candidate-output state

An ASM candidate output may be reported as:

- not produced;
- production pending;
- production active;
- produced;
- produced with limitations;
- partial;
- malformed;
- unsupported;
- failed;
- interrupted;
- resource exhausted;
- quarantined;
- cleanup pending;
- cleanup complete;
- cleanup failed;
- indeterminate.

`Produced` means only that the declared physical output was emitted.

### 11.3 BOOT acceptance boundary

A candidate output remains an ASM candidate output until BOOT accepts it into the context of an assembly generation.

BOOT acceptance may require separate evaluation of:

- operation authority;
- source identity;
- source authority;
- release identity;
- release authority;
- toolchain eligibility;
- output structure;
- integrity;
- provenance;
- target compatibility;
- kernel/micro-OS compatibility;
- required-component completeness;
- optional-component state;
- generation revision;
- installer eligibility.

ASM must not fabricate favorable values for any unresolved evaluation.

### 11.4 No automatic execution

ASM must not execute a candidate output as ordinary Node software merely because:

- it compiled;
- it linked;
- it passed an ASM test;
- it has a digest;
- it is locally produced;
- it resembles an existing component;
- it is newer;
- it is cached.

Execution required for bounded ASM testing must remain isolated from ordinary activation and runtime operation.

---

## 12. Storage, filesystem, and installation-target boundary

ASM may provide mechanisms for:

- block-device enumeration;
- filesystem recognition;
- read-only mounting;
- authorized source access;
- workspace creation;
- cache storage;
- evidence storage;
- candidate-output staging;
- temporary test filesystems;
- disposable image construction inside ASM-controlled staging.

ASM must not perform durable production installation-target mutation.

Installer-owned production mutation includes:

- partition-table changes;
- filesystem creation on the production target;
- formatting;
- production operating-system deployment;
- production package installation;
- durable configuration writes;
- boot-target configuration;
- activation staging;
- rollback;
- cleanup of installer-owned temporary state.

ASM may observe production targets and report mechanism-level facts.

ASM may produce a candidate disk-image file inside ASM-controlled output staging.

Writing that candidate image onto the production target remains installer-owned work.

An implementation component technically capable of both build work and installation must expose those roles through separate provider contracts and separately attributed results.

---

## 13. Workspace, cache, evidence, and staging direction

ASM-0005 defines the detailed resource model.

This charter establishes the following categories.

### 13.1 Source area

The source area contains authorized input material.

Source inputs should be treated as read-only where the operation permits.

### 13.2 Active workspace

The active workspace contains:

- generated build files;
- intermediate objects;
- temporary compiler files;
- test fixtures;
- operation-local metadata;
- partial outputs.

Workspaces must be isolated by provider operation or another explicitly safe boundary.

### 13.3 Cache

Caches may retain reusable, non-authoritative build material.

A cache entry is not:

- accepted source;
- release truth;
- a current component;
- an accepted artifact;
- an assembly generation.

Caches must be bounded, invalidatable, and safely evictable.

### 13.4 Evidence area

The evidence area retains bounded machine-readable records required to interpret the provider operation.

Evidence must preserve applicable:

- identity;
- provider;
- scope;
- revision;
- provenance;
- resource limits;
- limitations;
- uncertainty;
- transformation lineage;
- cleanup state.

### 13.5 ASM output staging

ASM output staging retains candidate outputs separately from:

- source input;
- active intermediate work;
- the running assembly environment;
- production installation targets;
- active Node generations;
- installer rollback material.

### 13.6 Recovery-path reserve

ASM profiles must preserve bounded capacity for:

- failure-result construction;
- cancellation processing;
- evidence finalization;
- candidate-output quarantine;
- cleanup reporting;
- session closure;
- return of control.

Ordinary build work must not consume every resource needed to fail honestly.

---

## 14. Networking and ACS boundary

Networking is profile-dependent.

An ASM profile may:

- require networking;
- support networking as optional;
- operate entirely offline;
- support only local IPC;
- support removable-media inputs;
- use preconfigured opaque references.

Network availability does not create authority.

ASM does not own:

- participant identity;
- endpoint identity;
- relationships;
- connection identity;
- attachments;
- ACS admission;
- ACS lifecycle;
- capability references;
- transport framing;
- routing;
- retransmission;
- cryptographic sessions.

ASM may consume ACS and transport mechanisms to obtain inputs or return results.

A successful connection does not establish:

- source acceptance;
- operation authority;
- transfer verification;
- build success;
- candidate-output acceptance.

An offline operation remains subject to the same authority, boundedness, provenance, classification, and output rules as a networked operation.

---

## 15. Security-provider boundary

ASM does not own cryptographic key custody or verification primitives.

A security provider may supply results concerning:

- credentials;
- authentication;
- signatures;
- digests;
- secure sessions;
- protected key operations;
- replay protection;
- firmware or platform attestations.

ASM must preserve the provider, scope, revision, and limitations of such results.

ASM must not:

- store raw long-term private keys in ordinary ASM records;
- expose reusable secrets in diagnostics;
- interpret encryption as authority;
- interpret a digest match as release acceptance;
- interpret signature validity as installation eligibility;
- interpret hardware-rooted evidence as permission to build or install.

---

## 16. Resource-management boundary

ASM must operate under finite resources.

Resource management owns:

- allocation;
- reservation;
- accounting;
- ceilings;
- pressure policy;
- reclamation;
- priority policy.

ASM must consume and enforce applicable limits.

ASM cannot enlarge its authority or resource allocation because:

- a build is important;
- recovery is urgent;
- the toolchain requests more memory;
- the target has additional idle capacity;
- a provider operation is partially complete.

Resource pressure is not proof of attack, compromise, invalid input, or target failure.

Resource exhaustion must remain a first-class ASM outcome.

---

## 17. Failure and uncertainty direction

ASM-0006 defines the detailed ASM failure model.

This charter requires failures to remain scoped and provider-attributed.

ASM failure domains include applicable:

- environment-startup failure;
- platform-mechanism failure;
- hardware-inspection failure;
- driver failure;
- device-runtime failure;
- storage failure;
- filesystem failure;
- process failure;
- network-mechanism failure;
- toolchain-unavailable failure;
- compiler failure;
- assembler failure;
- linker failure;
- packaging failure;
- test failure;
- workspace exhaustion;
- cache failure;
- evidence-retention failure;
- candidate-output failure;
- cleanup failure;
- interruption;
- unsupported mechanism;
- bounds failure;
- resource exhaustion;
- indeterminate result.

An ASM failure does not independently establish:

- target damage;
- compromise;
- source invalidity;
- release invalidity;
- BOOT operation denial;
- assembly-generation failure;
- installer failure;
- unrecoverability.

### 17.1 Partial output

When partial candidate output may exist, ASM must report:

- completed scope;
- incomplete scope;
- candidate-output references that are safe to expose;
- whether outputs are quarantined;
- cleanup state;
- resource state;
- unresolved effects;
- whether further use is prohibited;
- whether BOOT reconciliation may be required.

### 17.2 No false success

ASM must not report a favorable mechanism result when required result or evidence construction failed.

All fallible work required to represent the ASM result must complete before the favorable result is committed.

### 17.3 Process behavior is not semantic completion

The following do not independently establish a successful ASM provider operation:

- process launch;
- process liveness;
- process exit;
- zero exit status;
- file presence;
- log output;
- connection closure;
- reboot;
- environment restart.

### 17.4 Restart does not erase uncertainty

Restarting ASM must not silently erase:

- partial output;
- unknown prior completion;
- unresolved resource ownership;
- failed cleanup;
- stale operation references;
- quarantine state.

The exact continuation and reconciliation handoff remains BOOT-owned.

---

## 18. Minimal and headless environment requirements

ASM architecture must remain implementable in an extremely minimal headless Linux environment.

A conforming ASM contract must not require:

- a GUI;
- a display server;
- a desktop environment;
- a local human-readable console;
- a monitor;
- a keyboard;
- the normal Node runtime;
- Julia;
- a dynamic-language runtime;
- one service manager;
- one container runtime;
- one package manager;
- continuous network access;
- unrestricted persistent storage;
- unrestricted heap growth.

An assembly environment may operate with:

- no network;
- read-only source media;
- RAM-backed workspace;
- partial hardware discovery;
- limited CPU capacity;
- limited memory;
- unavailable accelerators;
- unavailable wall-clock time;
- bounded local diagnostics.

Human-readable terminal output may be provided.

It is not an architectural dependency and must not be the only authoritative result.

---

## 19. Public and private boundary

Public ASM architecture may define:

- role responsibilities;
- generic environment capabilities;
- generic hardware-observation categories;
- generic build capabilities;
- generic candidate-output categories;
- ownership boundaries;
- resource categories;
- public failure categories;
- public conformance requirements;
- implementation-neutral conceptual contracts.

Public ASM documentation must not disclose:

- production credentials;
- private keys;
- recovery secrets;
- private repository locations;
- production source locations;
- protected toolchain locations;
- private module identities where restricted;
- production hardware topology;
- private authority assignments;
- private signing policy;
- private installer policy;
- operator playbooks;
- hidden scoring or selection logic.

A public assembly environment must remain capable of operating within its declared public capability set without private modules.

Unavailable private capability must be represented truthfully as unavailable, unsupported, concealed, unauthorized, or not applicable.

It must not be represented as present or successful.

---

## 20. Explicit non-goals

ASM does not define:

- one universal kernel;
- one kernel version;
- one kernel configuration;
- one micro-OS implementation;
- one aggregate system-profile identity;
- one init system;
- one service manager;
- one filesystem layout;
- one compiler;
- one linker;
- one package manager;
- one build system;
- one device runtime;
- one network protocol;
- one transport;
- one cryptographic provider;
- one installer;
- one artifact format;
- one release format;
- one production authority policy;
- one production deployment topology.

ASM does not own:

- BOOT semantic operation identity;
- the selected BOOT component-set instance;
- kernel/micro-OS pair evaluation for that instance;
- assembly-plan semantics;
- assembly-generation identity or state;
- source or release acceptance;
- artifact acceptance;
- installation eligibility;
- production target mutation;
- rollback;
- activation;
- recovery completion;
- reboot or runtime handoff;
- runtime readiness;
- ACS relationships or admission;
- MEM truth, custody, or recovery;
- IMM assessment, verdict, or restoration;
- cryptographic key custody;
- physical-safety authority.

---

## 21. Prohibited interpretations

This specification must not be interpreted to mean that:

- the assembly environment is an assembled Node;
- the assembly environment is an assembly generation;
- ASM owns every operation performed before the normal runtime;
- local root access creates BOOT authority;
- source presence authorizes compilation;
- toolchain availability authorizes Node-targeted build work;
- a successful compiler probe authorizes production compilation;
- a compiler exit status proves candidate-output correctness;
- a candidate output is an accepted artifact;
- ASM output staging is BOOT generation staging;
- a successful test establishes BOOT validation;
- a digest establishes release authority;
- a signature establishes local compatibility;
- hardware discovery authorizes device use;
- device initialization authorizes installation;
- detected storage is disposable;
- ASM may mutate a production installation target;
- an installer process becomes ASM merely because it runs in the assembly environment;
- resource sufficiency creates authority;
- network reachability creates source acceptance;
- a cache entry is release truth;
- the newest kernel, micro-OS, toolchain, source, or output is automatically eligible;
- a reboot clears partial-output uncertainty;
- ASM failure proves the substrate is invalid or compromised;
- private modules are required for public ASM conformance;
- KRN, ASM, or BOOT owns the unresolved aggregate `SystemProfileIdentity`;
- ASM may replace missing BOOT, installer, ACS, security-provider, MEM, IMM, or runtime decisions with local guesses.

---

## 22. Relationship to the ASM specification series

### ASM-0001 — Assembly-Kernel Invariants

ASM-0001 defines the mandatory cross-cutting rules every assembly-kernel profile and assembly environment must preserve.

It will refine:

- role separation;
- candidate-output separation;
- authority separation;
- hardware-observation truthfulness;
- boundedness;
- headless operation;
- installer non-substitution;
- provider attribution;
- conservative failure.

### ASM-0002 — Assembly Environment Contract

ASM-0002 defines:

- environment startup;
- required environment capabilities;
- primary micro-OS-profile use;
- process and filesystem facilities;
- optional networking;
- result channels;
- controlled shutdown;
- return-of-control mechanisms.

It will consume KRN profile and compatibility-declaration structures.

### ASM-0003 — Hardware Inspection Mechanisms

ASM-0003 defines:

- hardware subjects;
- observation records;
- initialization state;
- accessibility;
- mechanism usability;
- provider attribution;
- observation bounds;
- unsupported and unknown hardware;
- observation-only target behavior.

### ASM-0004 — Build Capability and Toolchain Boundary

ASM-0004 defines:

- source-access mechanisms;
- compiler, assembler, and linker capabilities;
- cross-compilation;
- build-system execution;
- packaging;
- testing;
- provenance;
- candidate-output production;
- diagnostic and disposable authority-not-required work;
- BOOT-authorized Node-targeted build work.

### ASM-0005 — Resource and Workspace Model

ASM-0005 defines:

- memory requirements;
- storage requirements;
- source areas;
- active workspaces;
- caches;
- evidence areas;
- ASM output staging;
- cleanup;
- quarantine;
- recovery-path reserves;
- explicit resource-exhaustion outcomes.

### ASM-0006 — Kernel-Specific Failure Model

ASM-0006 defines:

- ASM failure domains;
- provider attribution;
- partial candidate outputs;
- interruption;
- cleanup failure;
- indeterminate effects;
- failure records;
- retry limitations;
- BOOT handoff of unresolved conditions.

### ASM-0007 — Assembly-Kernel Conformance

ASM-0007 defines:

- architectural conformance;
- implementation conformance;
- profile-scoped conformance;
- kernel/micro-OS conformance evidence;
- hardware-observation tests;
- build-capability tests;
- bounds and resource tests;
- candidate-output separation tests;
- installer-boundary tests;
- negative and failure-injection tests.

Later ASM specifications must not weaken this charter.

---

## 23. Cross-series open decisions

The following remain intentionally unresolved and must be completed by their owning series.

### 23.1 KRN-owned

- exact kernel-profile structure;
- exact micro-OS-profile structure;
- exact compatibility-declaration structure;
- exact required-component declaration structure;
- exact optional and unsupported component representation;
- profile lifecycle and revalidation;
- controller-versus-semantic-owner reference structure;
- common KRN failure categories;
- profile conformance evidence.

### 23.2 BOOT-owned

- semantic assembly-operation structure;
- provider-operation request structure;
- authority-envelope references;
- selected component-set instance;
- assembly-plan structure;
- assembly-generation structure;
- candidate-output acceptance;
- pair-evaluation result;
- generation progression;
- exact replacement of `BUILD_OR_SELECTION_ACTIVE` with a provider-attributed state;
- installer-handoff inputs;
- activation and recovery progression;
- continuation and reconciliation of interrupted ASM operations.

### 23.3 Deferred ownership

- aggregate `SystemProfileIdentity`;
- complete installed-system generation architecture;
- complete update architecture;
- complete source and release architecture;
- production rollout architecture.

No unresolved decision authorizes ASM to select a convenient owner or favorable default.

---

## 24. Initial architectural commitments

ASM-0000 establishes the following durable commitments:

1. ASM defines the assembly-kernel role and assembly-environment capability boundary.
2. The assembly kernel is a KRN kernel profile.
3. The assembly environment uses one primary KRN micro-OS-profile identity.
4. Subordinate micro-OS identities exist only for independently governed components.
5. Kernel/micro-OS compatibility declarations remain KRN-owned.
6. BOOT owns selected component-set instances and pair evaluation.
7. BOOT owns assembly generations.
8. ASM produces only ASM candidate outputs before BOOT acceptance.
9. Node-targeted build work requires applicable BOOT authority.
10. Authority-not-required compilation requires an explicit owning contract and remains bounded to diagnostic or disposable test work.
11. ASM initializes only the hardware required by its declared environment or authorized provider operation.
12. Hardware observation does not create identity, compatibility, authority, or installation eligibility.
13. Mechanism control and semantic ownership remain separate.
14. ASM reports environment capabilities, hardware observations, mechanism results, resource outcomes, and candidate outputs.
15. ASM does not mutate production installation targets.
16. The installer owns durable production mutation, activation staging, and rollback.
17. ASM workspaces, caches, evidence, and staging remain bounded.
18. ASM failure and partial output remain explicit.
19. The assembly environment remains minimal and headless.
20. Public ASM operation does not require private modules.
21. ASM does not claim installation, activation, recovery, handoff eligibility, or runtime readiness.
22. Aggregate `SystemProfileIdentity` ownership remains deferred.
23. No cross-series conflict may be silently resolved within ASM.

---

## 25. Completion checklist

Draft review must confirm:

- [ ] The assembly-kernel role remains subordinate to KRN.
- [ ] Assembly-kernel and assembly-environment terminology is consistent.
- [ ] Assembly generation is used only as a BOOT term.
- [ ] Kernel/micro-OS compatibility declaration is used only as a KRN term.
- [ ] The primary `MicroOsProfileIdentity` rule is preserved.
- [ ] Subordinate identity creation remains conditional.
- [ ] ASM candidate outputs remain distinct from BOOT assembly generations.
- [ ] Node-targeted build authority remains BOOT-owned.
- [ ] Diagnostic and disposable authority-not-required compilation is narrowly bounded.
- [ ] Hardware initialization and observation remain distinct.
- [ ] Mechanism control and semantic ownership remain distinct.
- [ ] Production installation-target mutation remains installer-owned.
- [ ] Resource and failure outcomes remain explicit.
- [ ] Headless operation remains mandatory.
- [ ] Public operation without private modules remains possible.
- [ ] Aggregate `SystemProfileIdentity` ownership remains deferred.
- [ ] ASM-0001 through ASM-0007 retain their intended scopes.
- [ ] No implementation language, kernel version, toolchain, filesystem, protocol, package manager, or installer is mandated.
- [ ] No unresolved KRN or BOOT matter is silently decided.

---

## 26. Closing principle

> **ASM is successful when it truthfully provides the mechanisms and evidence required for governed assembly—not when it claims ownership of the decisions those mechanisms enable.**

## Revision history

### Version 0.1 — 2026-07-18

- Replaced the ASM-0000 scope placeholder with the initial role charter.
- Defined the assembly kernel and assembly environment.
- Established the KRN parent-series relationship.
- Preserved KRN ownership of profiles, compatibility declarations, and required-component declarations.
- Preserved BOOT ownership of component-set instances, pair evaluation, assembly generations, progression, authority, installer handoff, activation, recovery, and runtime-handoff eligibility.
- Defined ASM provider operations and provider attribution.
- Defined ASM candidate outputs and ASM output staging.
- Established the BOOT-authority requirement for Node-targeted build work.
- Bounded authority-not-required compilation to explicitly declared diagnostic and disposable test work.
- Preserved the installer’s exclusive ownership of production installation-target mutation.
- Established hardware initialization and observation boundaries.
- Established mechanism-control and semantic-ownership separation.
- Established workspace, cache, evidence, staging, failure, headless, and public/private direction.
- Preserved deferred ownership of aggregate `SystemProfileIdentity`.
- Assigned detailed architecture to ASM-0001 through ASM-0007.
