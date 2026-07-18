# KRN-0000: Node Kernel Family Charter

| Field | Value |
|---|---|
| Specification | KRN-0000 |
| Title | Node Kernel Family Charter |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | KRN-PUB |
| Authors | Node |
| Authoring lane | `lane/docs` |
| Last updated | 2026-07-18 |
| Approval | Pending review |
| Depends on | Applicable ACS, MEM, IMM, and BOOT-0000 through BOOT-0004 public architecture |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in kernel-family purpose, role/era/profile separation, identity boundaries, KRN/ASM/BOOT ownership, and mechanism-versus-semantic-ownership rules; detailed schemas, role catalogs, era vocabularies, compatibility records, and lifecycle states remain assigned to later KRN specifications |

> **A Node-compatible kernel is an attributable, scoped mechanism profile. It is not Node’s identity, authority, installation state, recovery state, or proof that a system may proceed.**

## Architectural-intent notice

This specification defines independently authored public architecture for the Node kernel family.

It establishes:

- why Node maintains a family of kernel profiles rather than one permanently universal kernel;
- what architectural purpose the kernel family serves;
- the distinction between kernel role, era, profile, build, artifact, and deployed instance;
- the relationship between kernel profiles and micro-OS profiles;
- the meaning of a kernel/micro-OS compatibility declaration;
- the boundary between KRN, ASM, BOOT, and adjacent Node architectures;
- the distinction between concrete mechanism control and architectural ownership;
- the kernel-family public/private and licensing boundaries;
- the direction for profile conformance, maintenance, supersession, retirement, and revocation.

This specification does not prescribe:

- one kernel version;
- one upstream branch;
- one configuration;
- one patch set;
- one compiler;
- one architecture family;
- one hardware generation;
- one micro-OS;
- one package ecosystem;
- one bootloader;
- one installer;
- one runtime;
- one concrete role catalog;
- one compatibility-record encoding;
- one artifact format;
- one distribution mechanism.

Implementations and profiles may differ. Their observable declarations and behavior must preserve the ownership, identity, compatibility, uncertainty, and lifecycle distinctions established by this specification and its later refinements.

## 1. Purpose

Node is intended to operate across changing physical substrates.

Those substrates may differ in:

- processor architecture;
- instruction-set capability;
- firmware;
- boot mechanism;
- chipset;
- memory topology;
- IOMMU and DMA-isolation support;
- storage and network devices;
- PCIe and accelerator generations;
- driver requirements;
- kernel ABI;
- userspace ABI;
- compiler and linker support;
- package availability;
- security-maintenance status;
- intended operating role.

One kernel cannot be assumed to support every combination indefinitely without accumulating incompatible configuration, maintenance, security, driver, ABI, and validation requirements.

Node therefore maintains the architectural concept of a **kernel family**.

The kernel family exists to provide:

1. explicit kernel roles;
2. explicit hardware and software era declarations;
3. attributable kernel-profile identities;
4. scoped capability and limitation declarations;
5. attributable source, patch, configuration, toolchain, and ABI identity;
6. compatibility declarations between kernel and micro-OS profiles;
7. role-specific and era-specific conformance;
8. explicit maintenance, restriction, supersession, retirement, and revocation;
9. a stable boundary between kernel mechanisms and the architectures that own semantic meaning and authority.

The kernel family allows Node to evolve across hardware and software generations without pretending that every machine requires the same kernel or that a kernel is the identity of Node.

## 2. Foundational premise

Node is not defined by one kernel.

A kernel provides concrete execution and hardware-management mechanisms. Those mechanisms may be essential, privileged, early-loading, or deeply coupled to the local substrate. They do not thereby become the semantic owner of every object or operation they help implement.

A kernel may provide or mediate:

- processes and threads;
- virtual memory;
- physical memory mapping;
- scheduling;
- interrupts;
- timers;
- device drivers;
- IOMMU configuration;
- DMA isolation;
- filesystems;
- storage access;
- networking;
- sockets;
- shared memory;
- namespaces;
- cgroups;
- access-control mechanisms;
- observability;
- power and thermal interfaces;
- reboot and shutdown mechanisms.

Those mechanisms may be used by ACS, MEM, IMM, BOOT, ASM, the installer, runtimes, resource managers, security providers, device backends, and other components.

Providing or controlling a mechanism does not independently establish:

- architectural ownership;
- logical identity;
- semantic acceptance;
- authority;
- admission;
- lifecycle ownership;
- successful completion;
- health;
- recovery;
- readiness.

The enduring rule is:

> **Physical control location does not determine architectural ownership.**

Kernel mediation, kernel implementation, direct userspace control, userspace-driver control, device-runtime control, firmware control, and other authorized realization paths may all provide concrete mechanisms.

None independently defines the semantic owner of the object or operation being realized.

## 3. Normative language

The terms **MUST** and **MUST NOT** describe mandatory architectural requirements.

The terms **SHOULD** and **SHOULD NOT** describe strong recommendations. A departure requires documented justification and MUST NOT violate a mandatory requirement.

The term **MAY** describes permitted behavior.

Conformance depends on observable identity, declaration, boundary, lifecycle, and failure behavior rather than component names such as:

- `kernel`;
- `trusted kernel`;
- `Node kernel`;
- `assembly kernel`;
- `recovery kernel`;
- `secure kernel`;
- `runtime kernel`;
- `reference kernel`.

A kernel does not become Node-compatible merely because it carries a Node-related name, boots successfully, detects hardware, or launches Node-shaped software.

## 4. Core terminology

Later KRN specifications define detailed structures and state models. They must preserve the distinctions established here.

### 4.1 Node kernel family

The **Node kernel family** is the governed set of kernel roles, era models, profiles, declarations, conformance requirements, and lifecycle rules recognized by KRN architecture.

The family is an architectural classification and governance model.

It is not:

- one source repository;
- one branch;
- one kernel tree;
- one configuration;
- one binary;
- one release channel;
- one currently supported profile.

### 4.2 Upstream kernel lineage

An **upstream kernel lineage** identifies the externally maintained kernel source ancestry from which a Node kernel source state is derived.

It may identify:

- an upstream project;
- a release family;
- a tag;
- a branch;
- a commit;
- an attributable sequence of upstream revisions.

Upstream lineage is not the complete identity of a Node kernel profile.

### 4.3 Node kernel source identity

A **Node kernel source identity** identifies the attributable source state used by Node.

It includes or references:

- upstream lineage;
- Node-maintained changes;
- accepted patch sets;
- source revision;
- applicable provenance;
- applicable licensing information.

Two profiles may share one source identity while differing in role, era, configuration, toolchain, capabilities, or compatibility declarations.

### 4.4 Kernel role

A **kernel role** describes what a kernel profile is intended to enable or support.

Role is a semantic declaration of intended kernel contribution.

It is not:

- a machine identity;
- an architecture family;
- a hardware age;
- a BOOT disposition;
- current installation state;
- authority;
- runtime readiness.

The assembly kernel is the first concrete kernel role. Additional roles may be introduced through deliberate KRN review.

The role catalog remains open and MUST NOT be frozen merely because the first role has entered implementation planning.

### 4.5 Kernel era

A **kernel era** is a multidimensional compatibility envelope describing the hardware, firmware, ABI, toolchain, package, driver, and maintenance generations within which a profile claims support.

Era is not equivalent to:

- a calendar year;
- a hardware release date;
- a kernel version;
- the terms `legacy`, `current`, or `modern` without declared dimensions;
- a marketing generation.

A substrate may belong to different generations across different era dimensions.

Exact year ranges MUST NOT be assigned without supporting evidence.

### 4.6 Kernel profile

A **kernel profile** is an exact, attributable realization of a declared kernel role within a declared era envelope.

A kernel profile includes or references:

- profile identity and revision;
- kernel role;
- era declaration;
- architecture-family eligibility;
- hardware-class eligibility;
- upstream lineage;
- Node kernel source identity;
- patch-set identity;
- configuration identity;
- toolchain identity;
- ABI expectations;
- required capabilities;
- optional capabilities;
- unsupported capabilities;
- unknown or unevaluated capabilities;
- known limitations;
- expected micro-OS relationships;
- conformance evidence;
- maintenance and lifecycle state.

A kernel profile is more specific than a kernel version string.

### 4.7 Kernel build

A **kernel build** is one produced realization of a kernel profile under an identified build operation and environment.

A build may include:

- kernel image;
- modules;
- symbols;
- generated metadata;
- configuration output;
- build logs;
- test evidence;
- associated manifests.

A successful build does not independently establish:

- profile conformance;
- artifact acceptance;
- BOOT selection;
- installation eligibility;
- activation;
- runtime readiness.

### 4.8 Kernel artifact

A **kernel artifact** is a bounded physical output containing or representing all or part of a kernel build.

Examples may include:

- a kernel image;
- a module package;
- an archive;
- a bootable payload;
- a manifest;
- a signed or digested distribution object.

Physical presence does not independently prove profile identity, integrity, compatibility, acceptance, or authority.

### 4.9 Deployed kernel instance

A **deployed kernel instance** is one installed or executing realization of a kernel build or artifact on a physical or virtual substrate.

A deployed instance is not the kernel profile itself.

Successful execution of a deployed instance does not independently establish:

- that the instance matches its claimed profile;
- that the local micro-OS is compatible;
- that the system is eligible to proceed;
- that Node is installed;
- that recovery completed;
- that the normal runtime is ready.

### 4.10 Micro-OS profile

A **micro-OS profile** declares the identity, scope, capabilities, dependencies, limitations, and compatibility-relevant properties of a minimal operating environment intended to accompany or operate with one or more kernel profiles.

KRN defines the common `MicroOsProfileIdentity` and declaration structure required for kernel-family compatibility reasoning.

The architecture uses one primary `MicroOsProfileIdentity` for each declared micro-OS profile.

A subordinate component requires an independent identity only when it is independently:

- versioned;
- replaceable;
- reusable;
- distributed;
- maintained;
- validated;
- selected;
- revoked;
- or referenced by compatibility requirements.

Potential subordinate identities may include an initramfs, root filesystem, package snapshot, service bundle, toolchain bundle, or firmware bundle. Their use is conditional rather than automatic.

The architecture or role series responsible for a concrete micro-OS defines its actual contents and role-specific requirements.

For the first assembly environment, that responsibility belongs to ASM.

### 4.11 Kernel/micro-OS compatibility declaration

A **kernel/micro-OS compatibility declaration** is a KRN-governed, identified, revisioned, and scoped declaration that a specified kernel profile and specified micro-OS profile are compatible for a declared purpose under identified assumptions and limitations.

A compatibility declaration may reference:

- required components;
- firmware assumptions;
- boot assumptions;
- ABI expectations;
- required capability results;
- unsupported conditions;
- known limitations;
- validation scope;
- evidence references;
- lifecycle state.

A compatibility declaration is not:

- a selected pair;
- a BOOT assembly generation;
- BOOT pair evaluation;
- authority;
- installation eligibility;
- artifact acceptance;
- activation;
- recovery;
- runtime-handoff eligibility;
- runtime readiness.

KRN declares compatibility meaning.

BOOT determines whether a selected kernel/micro-OS pair satisfies the applicable declaration within a specific BOOT session, selected component-set instance, evidence boundary, authority boundary, and assembly generation.

### 4.12 Required-component declaration

A **required-component declaration** is a KRN-governed, identified and revisioned description of components or component classes required for a kernel profile, micro-OS profile, or compatibility declaration to make its stated claim.

KRN owns the common declaration structure.

A required-component declaration may describe:

- required component identity or class;
- revision constraints;
- required capability;
- role in the pairing;
- mandatory or conditional status;
- compatibility assumptions;
- acceptable substitution boundaries;
- validation requirements;
- known exclusions.

A required-component declaration is not a selected component set.

BOOT owns the **selected component-set instance** used by a particular assembly generation and owns evaluation of whether that instance satisfies applicable declarations.

### 4.13 System profile

A broader system profile may eventually aggregate kernel, micro-OS, hardware, runtime, backend, resource, security, and other installation-level facts.

Ownership of aggregate `SystemProfileIdentity` remains deliberately deferred.

KRN MUST NOT absorb ownership of the aggregate system profile merely because kernel-profile information contributes to it.

KRN may define and expose the kernel-specific declarations needed by a future aggregate profile.

## 5. Kernel-family model

The kernel-family model has three independent primary dimensions:

```text
Role
    What the kernel profile is intended to enable.

Era
    The hardware and software compatibility envelope
    within which the profile claims support.

Profile
    The exact attributable realization of that role
    within that era.
```

These dimensions MUST remain independently observable.

One role may have several era-specific profiles.

One era may contain profiles for several roles.

One upstream lineage may support several profiles.

One profile may produce several builds.

One build may produce several physical artifacts.

One artifact may be deployed more than once.

None of those relationships permits identity collapse.

For example:

- sharing upstream source does not make two profiles identical;
- sharing a role does not make two era profiles interchangeable;
- matching a profile name does not prove artifact identity;
- loading an artifact does not establish compatibility;
- compatibility does not establish BOOT eligibility;
- BOOT handoff eligibility does not establish runtime readiness.

## 6. KRN responsibilities

KRN owns the following architecture.

### 6.1 Kernel-family purpose

KRN defines why the kernel family exists and the enduring rules shared across it.

### 6.2 Kernel roles

KRN defines:

- the meaning of a kernel role;
- common role-declaration requirements;
- how new roles are proposed;
- how roles relate to family-wide requirements;
- how role-specific subseries refine the family.

A role-specific series may define concrete requirements without weakening KRN.

### 6.3 Era model

KRN defines:

- the meaning of hardware and software era dimensions;
- how era claims remain explicit and attributable;
- how unknown or unsupported dimensions are represented;
- how maintenance and security state affect era eligibility;
- how era declarations evolve without rewriting historical identity.

### 6.4 Kernel-profile structure

KRN defines the common structure and identity requirements for kernel profiles.

Detailed structure belongs to KRN-0002 and later specifications.

### 6.5 Micro-OS-profile structure

KRN defines the common identity and declaration structure required to reference micro-OS profiles in kernel-family compatibility declarations.

KRN does not automatically own each concrete micro-OS implementation or its complete operational content.

### 6.6 Compatibility declarations

KRN defines the common structure, meaning, revision, limitations, and lifecycle of kernel/micro-OS compatibility declarations.

KRN does not select the pair used by a BOOT session.

### 6.7 Required-component declarations

KRN defines the declaration structure through which profiles and compatibility declarations identify their requirements.

KRN does not own BOOT’s selected component-set instance.

### 6.8 Kernel capabilities and limitations

KRN defines how profiles declare:

- required mechanisms;
- optional mechanisms;
- unsupported mechanisms;
- unknown or unevaluated mechanisms;
- role-specific limitations;
- era-specific limitations;
- maintenance limitations;
- security limitations;
- validation scope.

A capability declaration does not grant authority to use that capability.

### 6.9 Profile conformance and lifecycle

KRN defines how profiles are:

- proposed;
- evaluated;
- validated;
- released;
- maintained;
- restricted;
- deprecated;
- superseded;
- retired;
- revoked;
- replaced;
- revalidated.

Detailed lifecycle dimensions and transitions belong to KRN-0005.

### 6.10 Mechanism and ownership boundaries

KRN defines how kernel mechanisms relate to adjacent architecture without silently redefining ownership.

Detailed controller, semantic-owner, authorizing-domain, evidence-producer, and final-verifier relationships belong to KRN-0004.

## 7. Explicit non-goals

KRN does not own or define:

- BOOT session identity;
- boot-attempt identity;
- local boot inspection;
- candidate boot disposition;
- assembly-generation identity or state;
- bootstrap identity;
- bootstrap authority;
- selected component-set instances;
- BOOT pair-evaluation state;
- artifact acceptance into an assembly generation;
- installer mutation;
- activation;
- rollback coordination;
- recovery coordination;
- reconciliation;
- reboot handoff;
- normal-runtime-handoff eligibility;
- normal-runtime readiness;
- ACS participant, relationship, endpoint, port, signal, admission, or connection semantics;
- MEM logical identity, acceptance, commitment, durability, retention, custody, deletion, or recovery truth;
- IMM evidence interpretation, findings, recommendations, containment, recovery verification, or restoration;
- runtime placement or scheduling policy;
- resource-allocation authority;
- security-provider cryptographic mechanisms or key custody;
- aggregate `SystemProfileIdentity`;
- production release policy;
- private signing policy;
- private deployment topology.

KRN may define interfaces and required declarations consumed by those systems. Coordination and reference do not transfer ownership.

## 8. Boundary with ASM

ASM is the role-specific subseries for the assembly kernel and assembly environment.

The terms:

- **assembly kernel**;
- **assembly environment**;

belong to ASM.

ASM defines:

- the assembly-kernel role;
- the capabilities required from its kernel;
- the capabilities required from its environment;
- hardware-observation mechanisms;
- environment-capability reporting;
- mechanism-result reporting;
- resource-outcome reporting;
- candidate-output production;
- role-specific kernel failure behavior;
- assembly-role conformance.

ASM reports:

- environment capabilities;
- hardware observations;
- mechanism results;
- resource outcomes;
- candidate outputs.

ASM does not own:

- semantic assembly-generation state;
- BOOT authority;
- selected component-set instances;
- final artifact acceptance;
- installer mutation;
- activation;
- recovery;
- runtime-handoff eligibility.

### 8.1 ASM candidate outputs

Physical outputs produced by ASM are **ASM candidate outputs**.

Candidate outputs may include kernel images, modules, micro-OS material, manifests, compiled Node components, toolchain products, test results, or related physical artifacts.

An ASM candidate output remains a candidate output until BOOT accepts or references it within a BOOT-owned assembly generation under applicable identity, evidence, compatibility, authority, and progression rules.

ASM production of an output does not establish:

- acceptance;
- release standing;
- installation eligibility;
- authority to install;
- activation eligibility;
- runtime readiness.

### 8.2 Production mutation

ASM MUST NOT mutate production installation targets as part of its architectural responsibility.

The installer owns durable production mutation, including applicable:

- disk preparation;
- filesystem creation;
- operating-system deployment;
- package installation;
- durable configuration writes;
- activation staging;
- rollback mechanics.

Technical ability to write a target does not transfer installer authority to ASM or KRN.

## 9. Boundary with BOOT

The term **assembly generation** belongs to BOOT.

BOOT owns:

- selected kernel and micro-OS references;
- selected component-set instances;
- evaluation of selected pairs;
- assembly-generation identity and state;
- progression;
- evidence and uncertainty state;
- authority evaluation;
- artifact state;
- installer handoff;
- installer-result coordination;
- activation;
- recovery;
- rollback and reconciliation coordination;
- reboot handoff;
- runtime-handoff eligibility.

KRN provides declarations that BOOT may evaluate.

KRN does not define BOOT success.

### 9.1 Declaration versus selected instance

KRN defines reusable declaration structures.

BOOT owns selected, session-scoped instances.

```text
KRN KernelProfile
    reusable declaration

KRN MicroOsProfile
    reusable declaration

KRN CompatibilityDeclaration
    reusable declaration

KRN RequiredComponentDeclaration
    reusable declaration

BOOT SelectedComponentSetInstance
    selected components for one governed scope

BOOT PairEvaluation
    evaluation of a selected kernel/micro-OS pair

BOOT AssemblyGeneration
    BOOT-owned generation state and progression
```

A KRN declaration may be valid while a particular BOOT evaluation fails, remains unknown, becomes unavailable, or is rejected under local evidence and authority.

### 9.2 Build authority

KRN profile or compatibility declarations do not authorize build operations.

Node-targeted build work requires applicable BOOT authority.

A build may be declared authority-not-required only when an owning contract explicitly permits it and limits the work to bounded:

- diagnostic compilation;
- disposable test compilation;
- non-production conformance probes;
- equivalent non-installable or non-propagating work.

The absence of required authority MUST NOT be inferred merely because:

- the build runs locally;
- no installation is immediately planned;
- source is publicly available;
- the output is described as experimental;
- the output has not yet been signed;
- the environment has sufficient resources.

Authority-not-required work must remain incapable of silently becoming production installation, activation, or propagation work.

### 9.3 Provider attribution

KRN facts and ASM results are provider-produced inputs to BOOT.

BOOT state must remain attributable to the provider and operation that produced the underlying observation or result.

KRN does not define or own BOOT’s detailed state vocabulary.

A provider operation becoming active must not be represented as universal assembly progress, profile validity, successful selection, or BOOT completion.

## 10. Boundaries with adjacent architecture

### 10.1 ACS

ACS owns:

- participant identity;
- relationships;
- endpoints;
- ports;
- signals;
- admission;
- budgets;
- connection identity and lifecycle;
- ACS security and trust semantics;
- capability, delegation, and revocation within ACS;
- ACS enforcement semantics.

Kernel mechanisms may implement or support ACS.

Examples include:

- sockets;
- shared memory;
- queues;
- PCIe mappings;
- DMA paths;
- device events;
- local calls;
- network transports.

A socket is not an ACS connection.

A numeric network port is not an ACS port.

A device handle is not an ACS participant.

A mapped PCIe BAR is not an endpoint, relationship, or admitted backend.

An ACS implementation may directly control sockets, numeric ports, PCIe devices, device queues, DMA resources, or other low-level mechanisms where authorized.

Such direct control does not remove the mechanism from applicable kernel protection, resource, security, or lifecycle boundaries, and it does not make KRN the semantic owner of the ACS object.

### 10.2 MEM

MEM owns:

- logical memory identity;
- storage proposals;
- semantic acceptance;
- commitment;
- durability;
- retention;
- custody;
- retrieval;
- reconstruction;
- deletion;
- memory recovery;
- memory-operation outcomes.

Kernel-managed pages, files, block devices, caches, buffers, mappings, and writes are physical mechanisms or material.

They are not automatically governed logical memory.

A successful write does not independently establish:

- MEM acceptance;
- commitment;
- durability;
- retention;
- complete replication;
- successful reconstruction;
- valid deletion.

### 10.3 IMM

IMM owns:

- immune evidence interpretation;
- assessment;
- findings;
- recommendations;
- protective coordination;
- immune recovery verification;
- restoration verification;
- immune audit.

Kernel telemetry may provide observations or evidence.

A kernel event, interrupt, log, counter, fault, security hook, or performance measurement is not independently:

- an IMM verdict;
- proof of compromise;
- authorization;
- containment;
- recovery;
- restoration.

Kernel or runtime mechanisms may enforce an IMM-triggered action only under applicable authority from the domain that owns the affected operation.

### 10.4 Installer

The installer owns durable production mutation.

A kernel may provide the filesystem, block, storage, and device mechanisms used by the installer.

KRN does not define successful installation merely because a kernel operation or userspace process completed.

### 10.5 Runtime

The runtime owns ordinary production execution, scheduling, placement, runtime lifecycle, and runtime-owned readiness after an accepted handoff.

A kernel may provide the execution substrate.

Kernel launch, process launch, thread liveness, device discovery, or service-manager success does not independently establish runtime readiness.

### 10.6 Resource management

Resource management owns:

- allocation;
- accounting;
- reservations;
- ceilings;
- pressure handling;
- reclamation;
- resource policy.

Kernel mechanisms may enforce allocations and limits.

Available CPU, memory, storage, bandwidth, or device capacity does not create admission or authority.

### 10.7 Security providers

Security providers own applicable:

- cryptographic identity mechanisms;
- key custody;
- signature verification;
- digest verification;
- secure-session establishment;
- credential storage;
- replay-protection mechanisms.

The kernel may expose or protect security mechanisms.

Technical access to keys, secure hardware, or credential storage does not grant KRN broad authority over their use.

## 11. Common charter principles

Later KRN-0001 invariants will formalize mandatory rules. The kernel family is founded on the following charter-level principles.

### 11.1 No one kernel is permanently universal

No kernel profile is presumed to support every role, era, platform, package generation, or maintenance horizon.

### 11.2 Role and era remain explicit

A profile must state both what it is intended to do and the compatibility envelope within which it claims to do it.

### 11.3 Identity exceeds a version string

Kernel identity must preserve the applicable source, patch, configuration, toolchain, ABI, role, era, and profile distinctions.

### 11.4 Kernel and micro-OS identities remain distinct

A kernel profile and a micro-OS profile may be compatible without becoming one identity.

### 11.5 Compatibility is scoped

Compatibility exists only under an identified declaration, revision, purpose, component requirement, assumption, limitation, and evidence boundary.

### 11.6 Mechanism control does not create semantic ownership

Kernel, userspace, firmware, device-runtime, and direct-device control paths may realize mechanisms without redefining the owning architecture.

### 11.7 Discovery does not create registration

Detecting hardware does not create:

- a Node participant;
- an ACS endpoint;
- a compute backend;
- device-use authority;
- runtime readiness.

### 11.8 Capability does not create authority

A profile declaring a mechanism does not authorize every actor or operation to use it.

### 11.9 Successful execution does not prove later stages

Boot, build, transfer, verification, installation, activation, recovery, handoff, and runtime readiness remain distinct.

### 11.10 Unknown remains explicit

Unknown, unsupported, unavailable, degraded, invalid, stale, restricted, conflicting, and unevaluated conditions must not silently become success.

### 11.11 Privilege does not enlarge architecture

Early execution, local execution, root privilege, kernel privilege, direct hardware control, rescue context, or emergency context does not create ownership or authority that did not otherwise exist.

### 11.12 Profiles have lifecycles

Kernel profiles, compatibility declarations, and required-component declarations require explicit revision, maintenance, supersession, restriction, retirement, and revocation behavior.

## 12. Profile capability and limitation declarations

A profile must declare its support honestly.

At minimum, later KRN specifications must permit capabilities to be represented as:

- required and demonstrated;
- required but not demonstrated;
- optional and demonstrated;
- optional but unavailable;
- unsupported;
- restricted;
- unknown;
- unevaluated;
- conflicting;
- degraded.

A profile must not claim compatibility merely because no failure has yet been observed.

Known limitations are part of profile meaning.

Limitations may include:

- unsupported devices;
- incomplete isolation;
- missing firmware support;
- absent security mitigations;
- unavailable toolchains;
- package-generation restrictions;
- architecture-specific errata;
- degraded performance;
- incomplete conformance evidence;
- limited recovery capability;
- maintenance restrictions.

A limitation does not automatically determine BOOT disposition. BOOT evaluates its effect within the current selected scope.

## 13. Public, private, and licensing boundaries

### 13.1 Public KRN material

Public KRN architecture may define:

- kernel-family purpose;
- roles and era dimensions;
- public identity structures;
- public compatibility structures;
- common capabilities and limitation categories;
- ownership boundaries;
- lifecycle states;
- conformance requirements;
- failure and uncertainty behavior;
- public patch provenance;
- public implementation requirements.

### 13.2 Restricted material

Public KRN architecture must not disclose:

- credentials;
- private signing keys;
- production trust anchors;
- protected repository locations;
- private release-authorization policy;
- production deployment inventory;
- protected hardware topology;
- private vulnerability-response procedures;
- private operator playbooks;
- reusable recovery secrets;
- production access-control details.

Public contracts must remain sufficient to implement and test declared behavior without exposing protected operational material.

### 13.3 Licensing and attribution

Node kernel source, patches, configuration material, generated source obligations, and distribution artifacts must preserve applicable upstream and third-party license requirements.

KRN architecture does not:

- replace upstream licensing terms;
- grant rights not provided by applicable licenses;
- remove attribution obligations;
- convert incompatible material into compatible material;
- authorize distribution merely because an artifact was built successfully.

Kernel profiles must preserve enough source and patch provenance to identify the applicable lineage and obligations.

License compatibility is a prerequisite for an eligible profile or artifact declaration, not an optional publication step.

## 14. Family conformance and evolution

A kernel profile does not conform merely because it boots or passes ordinary functional tests.

Conformance must consider the declared:

- role;
- era;
- source identity;
- patch identity;
- configuration;
- toolchain;
- ABI expectations;
- required capabilities;
- optional capabilities;
- limitations;
- micro-OS relationships;
- failure behavior;
- resource-exhaustion behavior;
- maintenance state;
- security-support state;
- evidence boundary.

Material changes may require revalidation.

Examples include changes to:

- upstream source;
- Node patches;
- configuration;
- compiler;
- linker;
- build flags;
- package dependencies;
- firmware assumptions;
- module set;
- ABI expectations;
- required components;
- compatibility declaration;
- security support;
- supported hardware class.

Historical profile identity must not be rewritten merely because a successor exists.

A successor must identify what it replaces and within which scope.

A retired, revoked, or superseded profile must not silently return to ordinary eligibility.

BOOT may consume profile lifecycle evidence but retains its own pair evaluation, assembly-generation state, authority, and progression decisions.

## 15. Deferred architecture

The following matters remain deliberately deferred:

- final kernel-role catalog;
- exact era vocabulary;
- exact profile-record schema;
- exact micro-OS-profile schema;
- exact compatibility-declaration schema;
- exact required-component declaration schema;
- exact profile-lifecycle dimensions and transitions;
- concrete conformance levels;
- aggregate `SystemProfileIdentity` ownership;
- production release channels;
- production signing policy;
- private distribution and rollout policy.

Deferral does not transfer these matters to whichever implementation encounters them first.

An unresolved matter must remain unresolved until the owning series deliberately decides it.

## 16. Initial architectural commitments

KRN-0000 establishes that:

1. Node maintains a kernel family rather than one permanently universal kernel.
2. Kernel role, era, and profile are independent dimensions.
3. Kernel-profile identity exceeds a kernel version string.
4. Kernel build, artifact, and deployed-instance identities remain distinct.
5. Kernel and micro-OS profiles retain separate primary identities.
6. Subordinate micro-OS component identities are required only when independently versioned, replaceable, reusable, distributed, maintained, selected, revoked, or separately validated.
7. KRN owns kernel-profile, micro-OS-profile, compatibility-declaration, and required-component declaration structures.
8. BOOT owns selected component-set instances, pair evaluation, assembly generations, authority, progression, installer handoff, activation, recovery, reconciliation, and runtime-handoff eligibility.
9. Assembly kernel and assembly environment are ASM terms.
10. Assembly generation is a BOOT term.
11. Kernel/micro-OS compatibility declaration is a KRN term.
12. ASM outputs remain candidate outputs until BOOT accepts or references them within an assembly generation.
13. ASM does not own durable production mutation.
14. The installer owns durable production mutation.
15. Node-targeted build work requires applicable BOOT authority unless an owning contract explicitly permits bounded diagnostic or disposable test compilation without that authority.
16. Physical mechanism control does not determine architectural ownership.
17. Kernel mediation does not create semantic authority.
18. Direct userspace or device control does not erase applicable architecture, authority, security, resource, or lifecycle boundaries.
19. Hardware discovery does not create Node registration.
20. Kernel telemetry is observation or evidence rather than an IMM assessment or verdict.
21. Kernel-managed storage and memory are not automatically MEM-governed logical memory.
22. Kernel launch does not establish BOOT progression or runtime readiness.
23. Aggregate `SystemProfileIdentity` ownership remains deferred.
24. Public KRN architecture must preserve applicable licensing, provenance, attribution, and public/private boundaries.
25. Cross-series conflicts must be recorded and reconciled rather than silently resolved in KRN prose or implementation.

## Revision history

### Version 0.1 — 2026-07-18

- Established the Node kernel-family charter.
- Defined the purpose of maintaining multiple role- and era-specific kernel profiles.
- Established role, era, profile, build, artifact, and deployed-instance distinctions.
- Established kernel-profile, micro-OS-profile, compatibility-declaration, and required-component declaration ownership.
- Incorporated the accepted KRN, ASM, and BOOT terminology boundaries.
- Established ASM candidate-output and installer-mutation boundaries.
- Established Node-targeted build-authority requirements.
- Established the distinction between concrete mechanism control and architectural ownership.
- Preserved ACS, MEM, IMM, BOOT, installer, runtime, resource-management, and security-provider authority.
- Deferred aggregate `SystemProfileIdentity` ownership.
