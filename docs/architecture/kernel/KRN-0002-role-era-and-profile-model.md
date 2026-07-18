# KRN-0002: Kernel Role, Era, and Profile Model

| Field | Value |
|---|---|
| Specification | KRN-0002 |
| Title | Kernel Role, Era, and Profile Model |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | KRN-PUB |
| Authors | Node |
| Authoring lane | `lane/docs` |
| Last updated | 2026-07-18 |
| Approval | Pending review |
| Depends on | KRN-0000, KRN-0001 |
| Related specifications | KRN-0003, KRN-0004, KRN-0005, applicable ASM and BOOT architecture |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in the separation of role, era, profile, revision, build, artifact, capability, limitation, and validation identity; high in the multidimensional era model and profile-attribution requirements; candidate roles other than the assembly role remain proposals, and concrete era labels remain intentionally unfrozen |

> **Role states what a kernel is expected to do. Era states the compatibility envelope in which it claims to do it. Profile identifies the exact attributable realization of that role within that era.**

## Architectural-intent notice

This specification defines the common role, era, and profile model for the Node kernel family.

It establishes:

- independent kernel-role identity;
- multidimensional kernel-era declarations;
- exact kernel-profile identity and revision;
- architecture-family and hardware-class eligibility;
- source, patch, configuration, toolchain, and ABI attribution;
- capability claims;
- limitation declarations;
- profile-validation records;
- profile comparison and revision rules;
- the boundary between reusable KRN declarations and BOOT-selected instances;
- the relationship between common KRN profiles and role-specific subseries such as ASM.

This specification does not:

- freeze one permanent role catalog;
- define exact calendar-year eras;
- select one kernel for all Node systems;
- define kernel/micro-OS compatibility-record schemas;
- define selected component-set instances;
- define BOOT pair evaluation;
- define assembly-generation state;
- define installer mutation;
- define exact profile lifecycle transitions;
- define aggregate `SystemProfileIdentity`;
- begin kernel implementation.

KRN-0003 defines the detailed kernel/micro-OS compatibility and required-component declaration model.

KRN-0004 defines adjacent-architecture and mechanism-control boundaries.

KRN-0005 defines profile conformance and lifecycle.

Role-specific subseries define the concrete requirements of approved roles.

## 1. Purpose

Node must operate across substrates that differ materially in:

- processor architecture;
- privilege model;
- firmware;
- boot mechanism;
- memory topology;
- IOMMU and DMA isolation;
- interrupt and timer facilities;
- storage;
- networking;
- PCIe and other interconnect generations;
- accelerator support;
- kernel ABI;
- userspace ABI;
- driver availability;
- compiler and linker support;
- package ecosystems;
- security support;
- maintenance horizon;
- intended operating purpose.

A kernel version alone cannot describe those differences.

A source tree alone cannot describe them.

A configuration alone cannot describe them.

A successful boot alone cannot prove that the kernel is suitable for a role.

KRN therefore models each kernel profile through three independent primary dimensions:

```text
Role
    What the kernel is expected to do.

Era
    The hardware, firmware, ABI, toolchain, package,
    driver, security, and maintenance envelope in
    which the kernel claims support.

Profile
    The exact attributable realization of one role
    within one declared era envelope.
```

These dimensions allow Node to:

- maintain different kernel profiles for different purposes;
- support old and new hardware without pretending they are identical;
- reuse source lineage across several profiles;
- revise configurations without rewriting historical identity;
- compare profiles without assuming newer means better;
- express unsupported and unknown conditions honestly;
- separate reusable compatibility declarations from BOOT selection;
- retire or replace profiles without erasing their history.

## 2. Scope

This specification applies to:

- `KernelRoleIdentity`;
- kernel-role declarations;
- `KernelEraDeclarationIdentity`;
- kernel-era declarations;
- `KernelProfileIdentity`;
- `KernelProfileRevisionIdentity`;
- kernel-profile declarations;
- upstream lineage references;
- Node kernel source references;
- patch-set references;
- configuration references;
- toolchain references;
- ABI declarations;
- architecture-family eligibility;
- hardware-class eligibility;
- firmware expectations;
- capability claims;
- limitation declarations;
- profile-validation records;
- profile comparison;
- profile revision;
- references to micro-OS profiles;
- references to required-component declarations;
- references to compatibility declarations;
- references to lifecycle and conformance standing.

It applies regardless of kernel role, architecture, era, upstream project, implementation language, build system, distribution method, or deployment environment.

## 3. Explicit non-goals

KRN-0002 does not define:

- concrete kernel source;
- concrete patches;
- concrete configuration files;
- concrete compiler flags;
- concrete kernel binaries;
- build scripts;
- package layouts;
- bootloader configuration;
- initramfs contents;
- micro-OS contents;
- installer behavior;
- runtime behavior;
- device-backend registration;
- ACS admission;
- MEM acceptance or durability;
- IMM findings;
- BOOT authority;
- BOOT selected component-set instances;
- BOOT assembly-generation identity or progression;
- production release approval;
- private signing policy;
- exact lifecycle state machines;
- aggregate system-profile ownership.

A field or reference defined here does not transfer ownership of the referenced subject into KRN.

## 4. Normative language

The terms **MUST** and **MUST NOT** define mandatory architectural requirements.

The terms **SHOULD** and **SHOULD NOT** define strong recommendations. A departure requires documented justification and MUST NOT violate a mandatory requirement.

The term **MAY** defines permitted behavior.

The words `role`, `era`, `profile`, `build`, `artifact`, `instance`, `compatible`, `supported`, `validated`, `current`, and `legacy` MUST NOT be used as interchangeable concepts.

## 5. The three-axis kernel-family model

### 5.1 Role axis

The role axis answers:

> What is this kernel profile expected to enable or support?

Role describes intended kernel contribution.

Role does not identify:

- processor architecture;
- machine age;
- upstream kernel version;
- profile revision;
- BOOT state;
- installation state;
- authority;
- runtime readiness.

### 5.2 Era axis

The era axis answers:

> Within which declared hardware and software compatibility envelope does this profile claim support?

Era is multidimensional.

Era does not identify:

- one calendar year;
- one kernel version;
- one product generation;
- one machine;
- one role;
- one lifecycle state.

### 5.3 Profile axis

The profile axis answers:

> Which exact attributable realization of the role and era is being declared?

A profile binds:

- one role declaration;
- one era declaration;
- source identity;
- patch identity;
- configuration identity;
- toolchain identity;
- ABI expectations;
- capability claims;
- limitation declarations;
- validation evidence;
- applicable micro-OS and component references;
- conformance and lifecycle references.

### 5.4 Independence requirement

Role, era, and profile MUST remain independently identifiable.

One role MAY have several era-specific profiles.

One era declaration MAY be referenced by several roles.

One source identity MAY support several profiles.

One profile MUST reference exactly one primary role declaration.

A combined or composite role MUST be declared as its own role rather than represented as an ambiguous list of unrelated roles.

One profile MUST reference exactly one era declaration revision.

Profiles with the same role and era are not necessarily identical.

Profiles with the same source revision are not necessarily identical.

Profiles with the same kernel version are not necessarily identical.

## 6. Identity hierarchy

### 6.1 Identity principles

Every normative KRN identity MUST be:

- stable;
- unique within its namespace;
- attributable;
- revision-aware;
- machine-readable;
- independent of a mutable filename;
- independent of a display label;
- usable without a graphical environment;
- suitable for references from other specifications.

Human-readable names MAY accompany identities.

Human-readable names MUST NOT replace normative identities.

### 6.2 `KernelRoleIdentity`

`KernelRoleIdentity` identifies one enduring role definition.

A role identity remains stable across compatible clarifications to that role.

A materially different purpose, authority boundary, or requirement model requires either:

- a new role revision where continuity remains valid; or
- a new `KernelRoleIdentity` where the role’s meaning is no longer continuous.

### 6.3 `KernelRoleRevisionIdentity`

`KernelRoleRevisionIdentity` identifies one exact immutable revision of a role declaration.

A kernel profile MUST reference an exact role revision.

### 6.4 `KernelEraDeclarationIdentity`

`KernelEraDeclarationIdentity` identifies one reusable era-declaration lineage.

An era declaration is reusable when several profiles share the same declared compatibility envelope.

### 6.5 `KernelEraDeclarationRevisionIdentity`

`KernelEraDeclarationRevisionIdentity` identifies one exact immutable revision of an era declaration.

A profile MUST reference an exact era-declaration revision.

### 6.6 `KernelProfileIdentity`

`KernelProfileIdentity` identifies one continuing kernel-profile lineage.

It allows a profile to receive attributable revisions without erasing prior revisions.

### 6.7 `KernelProfileRevisionIdentity`

`KernelProfileRevisionIdentity` identifies one exact immutable revision of a kernel profile.

Builds, artifacts, compatibility declarations, validation evidence, and BOOT selections MUST reference the exact profile revision they concern.

### 6.8 Related identities

A profile may reference independently governed identities including:

- `UpstreamKernelLineageIdentity`;
- `NodeKernelSourceIdentity`;
- `KernelPatchSetIdentity`;
- `KernelConfigurationIdentity`;
- `KernelToolchainIdentity`;
- `KernelAbiDeclarationIdentity`;
- `MicroOsProfileIdentity`;
- `RequiredComponentDeclarationIdentity`;
- `KernelMicroOsCompatibilityDeclarationIdentity`;
- `KernelValidationRecordIdentity`;
- `KernelBuildIdentity`;
- `KernelArtifactIdentity`;
- `DeployedKernelInstanceIdentity`.

The exact encoding of these identities MAY be defined by later implementation-facing specifications.

Their semantic separation is mandatory.

## 7. Kernel-role model

### 7.1 Role declaration purpose

A kernel-role declaration defines the common expectations for every profile claiming that role.

It states what the role requires without selecting:

- one architecture;
- one era;
- one source revision;
- one configuration;
- one build;
- one machine;
- one micro-OS instance;
- one BOOT assembly generation.

### 7.2 Required role-declaration fields

A role declaration MUST include or reference:

| Field | Required meaning |
|---|---|
| Role identity | Stable role lineage |
| Role revision | Exact immutable declaration revision |
| Name | Human-readable role name |
| Purpose | What the role exists to enable |
| Owning series | KRN or the approved role-specific subseries |
| Required capabilities | Capabilities every conforming profile must claim and demonstrate as required |
| Optional capabilities | Capabilities permitted but not mandatory |
| Prohibited assumptions | Conditions the role must not assume |
| Required observations | Information the role must be able to report |
| Failure behavior | Required behavior when role-critical mechanisms fail |
| Resource expectations | Relevant boundedness and resource assumptions |
| Headless requirements | Required machine-readable operation |
| Adjacent boundaries | Ownership and authority that remain external |
| Conformance reference | Applicable role-conformance specification |
| Revision rationale | Reason for the current revision |

### 7.3 Role ownership

KRN owns:

- the role identity model;
- the admission process for new kernel roles;
- family-wide role requirements;
- the relationship between roles and profiles.

A role-specific subseries owns:

- concrete role purpose;
- role-specific capabilities;
- role-specific limitations;
- role-specific failure behavior;
- role-specific conformance requirements.

For the assembly role:

- `assembly kernel` is an ASM term;
- `assembly environment` is an ASM term;
- ASM defines the concrete role requirements;
- KRN provides the common role and profile structure;
- BOOT retains assembly-generation and progression ownership.

### 7.4 One primary role per profile

Each kernel-profile revision MUST reference exactly one primary role revision.

A profile requiring several inseparable purposes MUST reference a deliberately defined composite role.

A profile MUST NOT claim several unrelated roles merely because its build contains mechanisms useful to each.

Capability overlap does not establish role conformance.

### 7.5 Role admission criteria

A new role SHOULD be introduced only when at least one of the following is true:

- it has materially different required capabilities;
- it has materially different failure behavior;
- it has materially different security or authority constraints;
- it has materially different headless or recovery requirements;
- it requires a distinct validation program;
- it requires a distinct maintenance policy;
- combining it with an existing role would create contradictory configuration;
- combining it with an existing role would create misleading compatibility claims.

A role SHOULD NOT be created solely because:

- one hardware vendor exists;
- one board exists;
- one kernel version exists;
- one package name differs;
- one implementation team prefers a separate label;
- one profile needs a minor configuration adjustment.

## 8. Initial role catalog

### 8.1 Established role

| Role | Standing | Owning series | Meaning |
|---|---|---|---|
| Assembly kernel | Established first concrete role | ASM | Kernel role used by the assembly environment to inspect a substrate, provide required mechanisms, support governed provider operations, and produce ASM candidate outputs without absorbing BOOT or installer ownership |

`Established` in this section means the role is accepted as an architectural subject.

It does not mean that a conforming implementation already exists.

### 8.2 Candidate roles

The following roles remain candidate planning categories rather than approved commitments:

| Candidate role | Candidate purpose |
|---|---|
| Normal-runtime kernel | Support ordinary Node runtime execution after accepted BOOT handoff |
| Recovery-support kernel | Provide mechanisms needed by a governed recovery environment |
| Minimal edge or embedded kernel | Support constrained headless nodes with limited memory, storage, or peripherals |
| Compute-host kernel | Support high-density CPU, GPU, accelerator, and scheduling mechanisms |
| Storage-capable kernel | Support substrates whose principal contribution includes persistent storage mechanisms |
| Hardware-enablement kernel | Support newly introduced hardware before broader profile consolidation |
| Legacy-compatibility kernel | Preserve bounded support for older hardware, firmware, or ABI generations |
| Validation or reference kernel | Provide a controlled reference environment for conformance and diagnostic work |

These candidate roles MUST NOT be treated as normative until individually reviewed.

The candidate list is not exhaustive.

Role names MAY change before approval.

A candidate role MAY be rejected, merged, split, or replaced.

### 8.3 Candidate-role prohibition

No implementation MUST claim conformance to an unapproved candidate role as though the role were normative.

Experimental work MAY reference a candidate role when the candidate standing is explicit.

## 9. Kernel-era model

### 9.1 Era as a vector

A kernel era is a revisioned vector of compatibility claims.

It is not a single scalar value.

The conceptual form is:

```text
KernelEraDeclaration
    architecture dimensions
    platform dimensions
    firmware dimensions
    memory and isolation dimensions
    interconnect dimensions
    device dimensions
    ABI dimensions
    toolchain dimensions
    package dimensions
    firmware-package dimensions
    security dimensions
    maintenance dimensions
```

A substrate may be new in one dimension and old in another.

A newer kernel version may support an older hardware era.

An older upstream lineage may remain appropriate for a narrowly maintained profile.

Era comparison is therefore generally a partial ordering rather than a total ordering.

### 9.2 Era declaration requirements

An era declaration MUST:

- identify every dimension material to the profiles that reference it;
- define the supported range, set, family, or condition for each declared dimension;
- distinguish supported, unsupported, restricted, unknown, and unevaluated dimensions;
- preserve assumptions;
- preserve known exclusions;
- preserve evidence references;
- preserve revision history;
- avoid unsupported calendar boundaries;
- avoid marketing labels as normative definitions.

### 9.3 Architecture dimensions

Applicable architecture dimensions MAY include:

- instruction-set architecture family;
- architecture revision;
- word size;
- endianness;
- privilege architecture;
- virtualization architecture;
- atomic-operation requirements;
- memory-ordering assumptions;
- page-size assumptions;
- exception model;
- interrupt model;
- timer model.

An architecture-family declaration does not identify one physical machine.

### 9.4 Platform and firmware dimensions

Applicable platform and firmware dimensions MAY include:

- platform family;
- board or server class;
- firmware interface;
- boot protocol;
- device-tree generation;
- ACPI generation;
- system-management firmware;
- secure-boot mechanism support;
- measured-boot mechanism support;
- firmware-service availability;
- firmware-update assumptions.

Firmware presence does not establish firmware suitability.

### 9.5 Memory, isolation, and topology dimensions

Applicable dimensions MAY include:

- address-space capability;
- NUMA topology;
- memory-hotplug support;
- huge-page support;
- memory-encryption support;
- IOMMU generation;
- DMA-remapping support;
- interrupt-remapping support;
- device-isolation capability;
- virtualization extensions;
- cache-coherency assumptions.

A declared mechanism does not grant authority to use it.

### 9.6 Interconnect and device dimensions

Applicable dimensions MAY include:

- PCIe generation;
- PCIe feature requirements;
- coherent interconnect generation;
- storage-bus generation;
- network-controller generation;
- USB generation;
- serial-bus support;
- accelerator class;
- GPU driver family;
- device-runtime requirements;
- device-firmware requirements;
- hotplug behavior.

Device discovery does not create backend registration.

### 9.7 Software and ABI dimensions

Applicable dimensions MAY include:

- upstream kernel API generation;
- internal kernel API expectations;
- module ABI expectations;
- userspace syscall ABI;
- executable format;
- dynamic-loader expectations;
- C library generation;
- runtime-library generation;
- service-manager expectations;
- container or namespace expectations;
- filesystem-feature expectations.

Kernel ABI and userspace ABI MUST remain distinguishable.

### 9.8 Toolchain and build dimensions

Applicable dimensions MAY include:

- compiler family;
- compiler version range;
- assembler;
- linker;
- binary utilities;
- build-system generation;
- host-tool requirements;
- language-standard requirements;
- code-generation assumptions;
- link-time optimization assumptions;
- reproducibility constraints.

A toolchain update is not immaterial by default.

### 9.9 Package and firmware-package dimensions

Applicable dimensions MAY include:

- package format;
- package-manager generation;
- repository metadata generation;
- package snapshot identity;
- module package generation;
- firmware package generation;
- initramfs tooling;
- root-filesystem tooling;
- service-bundle expectations.

KRN describes compatibility-relevant requirements.

The role-specific or micro-OS-owning architecture defines concrete contents.

### 9.10 Security dimensions

Applicable security dimensions MAY include:

- upstream security-support standing;
- required mitigations;
- cryptographic API generation;
- kernel-lockdown mechanisms;
- module-signing mechanisms;
- measured-boot support;
- secure-boot support;
- vulnerability status;
- known unmitigated conditions;
- security-maintenance horizon.

Mechanism availability does not establish security authority or trust.

### 9.11 Maintenance dimensions

Applicable maintenance dimensions MAY include:

- upstream maintenance standing;
- Node maintenance standing;
- vendor support standing;
- required toolchain availability;
- required package availability;
- required firmware availability;
- expected patch cadence;
- end-of-support conditions.

Exact lifecycle states and transitions belong to KRN-0005.

### 9.12 No unsupported year ranges

An era declaration MUST NOT use exact year boundaries unless the boundaries are supported by relevant evidence and are meaningful across the declared dimensions.

Terms such as:

- `legacy`;
- `current`;
- `modern`;
- `next-generation`;
- `old`;
- `new`;

MAY be used as non-normative display labels.

They MUST NOT replace the era vector.

## 10. Architecture-family eligibility

### 10.1 Meaning

Architecture-family eligibility states which processor architecture families may satisfy a profile’s era and role requirements.

It does not state that every processor in the family is supported.

### 10.2 Required declaration

A profile MUST identify:

- eligible architecture family or families;
- required architecture revisions;
- prohibited revisions where known;
- required privilege features;
- required memory-model features;
- required atomic features;
- required exception and interrupt behavior;
- unknown or unevaluated variants.

### 10.3 Generic architecture support

A role MUST NOT assume that one architecture family corresponds to one product class.

For example, an ARM-family profile MUST NOT assume:

- Raspberry Pi hardware;
- one firmware interface;
- one board topology;
- one accelerator;
- one camera;
- one server size;
- one power envelope.

Architecture-family identity is broader than platform identity.

## 11. Hardware-class eligibility

### 11.1 Meaning

Hardware-class eligibility narrows an architecture-family claim to a declared set of platform characteristics.

A hardware class MAY describe characteristics such as:

- server;
- workstation;
- embedded controller;
- single-board computer;
- virtual machine;
- storage node;
- accelerator host;
- high-memory host;
- low-memory edge node.

These labels are descriptive only unless accompanied by explicit constraints.

### 11.2 Required constraints

A hardware-class declaration SHOULD identify applicable:

- minimum and maximum processor topology;
- memory assumptions;
- firmware interface;
- storage assumptions;
- network assumptions;
- required isolation mechanisms;
- required device classes;
- prohibited device assumptions;
- thermal and power-management assumptions;
- headless-operation requirements.

### 11.3 Hardware class is not inventory

Hardware-class eligibility does not identify the actual devices present in one machine.

ASM may report hardware observations.

BOOT may use observations and declarations when evaluating a selected pair.

KRN does not own local inventory truth.

## 12. Kernel-profile model

### 12.1 Profile purpose

A kernel profile is the central reusable KRN declaration describing one exact realization of:

- one kernel role revision;
- one kernel-era declaration revision;
- one attributable kernel-source and build-input set.

### 12.2 Required profile fields

Every kernel-profile revision MUST include or reference:

| Field | Meaning |
|---|---|
| Profile identity | Stable profile lineage |
| Profile revision identity | Exact immutable revision |
| Human-readable name | Non-normative display name |
| Role revision | Exact `KernelRoleRevisionIdentity` |
| Era revision | Exact `KernelEraDeclarationRevisionIdentity` |
| Architecture eligibility | Eligible processor architecture families and revisions |
| Hardware-class eligibility | Applicable platform constraints |
| Upstream lineage | Attributable upstream source ancestry |
| Node source identity | Exact Node-maintained source state |
| Patch-set identity | Ordered attributable patch set |
| Configuration identity | Exact configuration state |
| Toolchain identity | Compiler, linker, tools, and material flags |
| ABI declaration | Kernel, module, and userspace ABI expectations |
| Firmware expectations | Required, optional, unsupported, and unknown firmware conditions |
| Capability claims | Structured capability declarations |
| Limitation declarations | Known restrictions, exclusions, and uncertainty |
| Micro-OS expectations | Opaque references used by KRN-0003 |
| Required-component references | Opaque declaration references used by KRN-0003 |
| Validation records | Evidence-backed validation references |
| Conformance reference | Applicable KRN-0005 standing |
| Lifecycle reference | Applicable KRN-0005 state |
| Provenance | Producer and revision history |
| Licensing | Applicable licensing and distribution information |
| Replacement references | Applicable predecessor or successor references |
| Notes | Non-normative explanatory material |

### 12.3 Profile immutability

An exact `KernelProfileRevisionIdentity` MUST be immutable.

Corrections to a published revision require:

- a new profile revision;
- explicit relation to the prior revision;
- preservation of the prior record;
- identification of changed fields;
- revalidation where required.

### 12.4 Profile lineage

A profile lineage MAY continue across revisions when:

- the role remains semantically continuous;
- the era remains substantially continuous;
- the intended compatibility scope remains recognizable;
- the revision does not misrepresent prior artifacts.

A new `KernelProfileIdentity` is required when continuity would be misleading.

Examples include:

- changing the primary role;
- replacing the architecture family with an unrelated family;
- changing the profile’s fundamental compatibility scope;
- splitting one profile into independently maintained lineages;
- merging profiles whose histories must remain distinguishable.

## 13. Source and provenance model

### 13.1 Upstream lineage

A profile MUST identify the upstream kernel lineage from which its Node source state is derived.

The lineage SHOULD identify:

- upstream project;
- release family;
- tag, branch, or commit;
- relevant base revision;
- known vendor-derived ancestry where applicable.

### 13.2 Node source identity

`NodeKernelSourceIdentity` identifies the exact source state maintained or accepted by Node.

It MUST preserve:

- upstream lineage;
- Node-maintained changes;
- source revision;
- provenance;
- licensing information;
- known generated-source obligations.

### 13.3 Patch-set identity

A `KernelPatchSetIdentity` MUST identify:

- ordered patch membership;
- patch revisions;
- patch origin;
- declared requirement or issue addressed;
- applicable role or era scope;
- dependencies;
- conflicts;
- licensing or attribution;
- superseded patches;
- validation impact.

An unattributed local modification MUST NOT be treated as part of a conforming profile.

### 13.4 Configuration identity

A `KernelConfigurationIdentity` MUST identify:

- exact configuration source;
- generated configuration state;
- role-specific requirements;
- era-specific requirements;
- required built-in features;
- module selections;
- prohibited settings;
- security settings;
- debugging or diagnostic settings;
- configuration-generation inputs.

A configuration nickname is not sufficient identity.

### 13.5 Toolchain identity

A `KernelToolchainIdentity` MUST identify material build tools including:

- compiler;
- assembler;
- linker;
- binary utilities;
- build-system revision;
- host tools;
- material build flags;
- target triple;
- language-standard assumptions;
- link-time or code-generation features.

### 13.6 Attribution rather than automatic reproducibility

A conforming profile MUST provide reconstructable attribution.

KRN-0002 does not require every build to be bit-for-bit reproducible unless a later conformance profile requires it.

Where reproducibility is claimed, its scope and evidence MUST be explicit.

## 14. ABI declaration model

### 14.1 ABI categories

A profile MUST distinguish applicable:

- syscall ABI;
- executable ABI;
- module ABI;
- kernel-to-firmware ABI;
- kernel-to-device-runtime ABI;
- kernel-to-userspace control ABI;
- architecture-specific calling conventions.

### 14.2 ABI claims

An ABI claim MUST identify:

- ABI subject;
- supported version or range;
- required consumers;
- compatibility assumptions;
- known exclusions;
- validation evidence;
- stability expectation.

### 14.3 ABI success is scoped

One successfully executed userspace program does not prove general ABI compatibility.

One successfully loaded module does not prove general module ABI compatibility.

ABI uncertainty MUST remain explicit.

## 15. Capability-claim model

### 15.1 Orthogonal capability dimensions

A capability claim MUST separate:

1. **Obligation**
2. **Observed support**
3. **Evidence standing**
4. **Operational condition**

### 15.2 Obligation values

The obligation dimension MUST support:

| Value | Meaning |
|---|---|
| `REQUIRED` | The role or profile requires the capability |
| `OPTIONAL` | The capability is permitted but not mandatory |
| `PROHIBITED` | The profile must not expose or depend on the capability |
| `NOT_APPLICABLE` | The capability does not apply within the declared scope |

### 15.3 Support values

The support dimension MUST support applicable values including:

| Value | Meaning |
|---|---|
| `SUPPORTED` | The profile claims the capability within the declared conditions |
| `UNAVAILABLE` | The capability is normally recognized but unavailable in the evaluated condition |
| `UNSUPPORTED` | The profile does not support the capability |
| `RESTRICTED` | Use is limited by declared conditions |
| `DEGRADED` | Capability exists with reduced behavior |
| `UNKNOWN` | Current truth cannot be established |
| `UNEVALUATED` | No adequate evaluation has occurred |
| `CONFLICTING` | Available evidence disagrees |

### 15.4 Evidence values

The evidence dimension MUST support:

| Value | Meaning |
|---|---|
| `NOT_EVALUATED` | No adequate evidence exists |
| `DECLARED_ONLY` | Claim exists without demonstration |
| `PARTIALLY_DEMONSTRATED` | Some required behavior was demonstrated |
| `DEMONSTRATED` | Required behavior was demonstrated within scope |
| `FAILED` | Evaluation demonstrated nonconformance |
| `STALE` | Prior evidence is no longer current |
| `CONFLICTING` | Evidence produces incompatible conclusions |

### 15.5 Operational condition

A capability claim SHOULD identify applicable conditions including:

- required hardware;
- required firmware;
- required module;
- required micro-OS component;
- required privilege;
- required authority;
- required resource assignment;
- incompatible conditions;
- degraded conditions.

### 15.6 Required capability claims

A role-required capability does not conform merely because it is configured.

A required capability generally requires:

```text
obligation = REQUIRED
support = SUPPORTED
evidence = DEMONSTRATED
```

within the applicable validation scope.

A later role-specific conformance specification MAY require stronger evidence.

### 15.7 Capability does not create authority

A supported capability does not grant authority to use it.

A profile may declare:

- DMA isolation support;
- device mapping support;
- storage access;
- networking;
- reboot mechanisms;
- module loading;
- cryptographic APIs.

Their use remains subject to applicable ownership and authority.

## 16. Limitation-declaration model

### 16.1 Limitation purpose

A limitation declaration records a condition that narrows, weakens, constrains, or qualifies a profile claim.

Limitations are part of profile meaning.

### 16.2 Required limitation fields

A limitation MUST identify or reference:

- limitation identity;
- profile or era scope;
- affected role requirement;
- affected capability;
- affected hardware or software dimension;
- triggering condition;
- observable effect;
- workaround, where one exists;
- workaround limitations;
- evidence;
- review requirement;
- revalidation trigger;
- maintenance or security consequence.

### 16.3 Limitation effect categories

A limitation MAY be categorized as:

- scope-reducing;
- capability-reducing;
- performance-reducing;
- reliability-reducing;
- observability-reducing;
- validation-reducing;
- maintenance-reducing;
- security-reducing;
- compatibility-reducing.

These categories do not determine BOOT disposition.

BOOT evaluates the effect of limitations on a selected pair within its own authority and state model.

### 16.4 Workarounds

A workaround does not erase a limitation.

A workaround MUST identify:

- required action;
- authority required;
- remaining risk;
- validation scope;
- conditions under which it is valid.

## 17. Profile-validation model

### 17.1 Validation is scoped

A validation result MUST identify:

- exact profile revision;
- exact subject;
- exact operation or test;
- provider;
- environment;
- hardware class;
- firmware state;
- micro-OS reference where applicable;
- evidence;
- time or sequence relevance;
- result;
- limitations;
- freshness.

### 17.2 Validation dimensions

A profile MAY have separate validation standing for:

- declaration completeness;
- source provenance;
- patch attribution;
- configuration conformance;
- toolchain conformance;
- build success;
- artifact identity;
- boot behavior;
- mechanism behavior;
- role-required capabilities;
- failure behavior;
- resource exhaustion;
- headless operation;
- security requirements;
- maintenance requirements.

Kernel/micro-OS pair validation is refined by KRN-0003.

BOOT pair evaluation remains BOOT-owned.

### 17.3 Validation result values

A validation dimension MUST support applicable results including:

- `NOT_EVALUATED`;
- `PARTIAL`;
- `SATISFIED`;
- `FAILED`;
- `STALE`;
- `CONFLICTING`;
- `INDETERMINATE`.

### 17.4 Validation is not lifecycle

Validation evidence contributes to profile conformance and lifecycle.

It is not itself the complete lifecycle state.

KRN-0005 defines lifecycle and conformance standing.

### 17.5 Successful boot is insufficient

A successful boot MAY contribute evidence for:

- kernel loading;
- platform support;
- userspace launch;
- selected mechanism availability.

It does not independently prove:

- complete role conformance;
- complete era support;
- micro-OS compatibility;
- BOOT eligibility;
- installation success;
- recovery;
- runtime readiness.

## 18. Profile comparison model

### 18.1 Identity equality

Two profile references are identical only when their exact `KernelProfileRevisionIdentity` values match.

Matching display names are insufficient.

Matching profile lineage without matching revision is insufficient.

### 18.2 Semantic similarity

Two profiles MAY be similar because they share:

- role;
- era;
- source;
- patch set;
- configuration;
- architecture;
- hardware class.

Similarity is not identity.

### 18.3 Compatibility

Compatibility between a kernel profile and a micro-OS profile is declared under KRN-0003.

Compatibility is not derived solely from profile similarity.

### 18.4 Substitutability

One profile MUST NOT be treated as a substitute for another merely because it is newer, broader, or built from the same source.

Substitutability requires an explicit governed claim identifying:

- source profile;
- replacement profile;
- applicable role;
- applicable era;
- preserved capabilities;
- changed capabilities;
- changed limitations;
- validation evidence;
- lifecycle standing.

BOOT retains evaluation of a substitution in a selected assembly scope.

### 18.5 No total ordering

The profile family MUST NOT assume one universal ordering from oldest to newest or worst to best.

A profile may be:

- newer in upstream source;
- older in firmware compatibility;
- broader in architecture support;
- narrower in security maintenance;
- stronger in one role;
- weaker in another.

Comparison MUST identify the dimension being compared.

## 19. Revision rules

### 19.1 Material profile changes

A new `KernelProfileRevisionIdentity` is required when any material field changes, including:

- role revision;
- era revision;
- source revision;
- patch set;
- configuration;
- toolchain;
- ABI expectations;
- firmware assumptions;
- capability claim;
- limitation;
- required-component reference;
- validation-relevant build input;
- licensing condition.

### 19.2 Material role changes

A new role revision is required when:

- required capabilities change;
- prohibited assumptions change;
- role failure behavior changes;
- authority boundaries are clarified materially;
- role conformance requirements change.

A new role identity is required when the role’s purpose changes so substantially that continuity would mislead.

### 19.3 Material era changes

A new era-declaration revision is required when:

- a supported range changes;
- an unsupported condition becomes supported;
- a supported condition becomes restricted;
- firmware assumptions change;
- ABI generations change;
- toolchain requirements change;
- maintenance scope changes;
- security support changes.

### 19.4 Evidence invalidation

A material change MUST identify which validation evidence becomes:

- stale;
- invalid;
- restricted;
- unaffected;
- pending revalidation.

### 19.5 Historical preservation

Prior revisions MUST remain historically identifiable.

A new revision MUST NOT rewrite the contents or standing of prior builds and artifacts.

## 20. KRN, ASM, and BOOT relationship

### 20.1 KRN

KRN owns:

- role identity structure;
- era declaration structure;
- profile identity structure;
- capability-claim structure;
- limitation structure;
- validation-reference structure;
- common revision rules.

### 20.2 ASM

ASM owns:

- concrete assembly-kernel role requirements;
- concrete assembly-environment requirements;
- environment capabilities;
- hardware observations;
- mechanism results;
- resource outcomes;
- ASM candidate outputs;
- assembly-role conformance.

ASM observations may provide evidence used when evaluating a profile.

ASM does not select or own a BOOT assembly generation.

### 20.3 BOOT

BOOT owns:

- selected component-set instances;
- selected kernel and micro-OS references;
- pair evaluation;
- assembly-generation identity and state;
- progression;
- evidence state;
- authority;
- installer handoff;
- activation;
- recovery;
- reconciliation;
- runtime-handoff eligibility.

BOOT consumes exact profile revisions.

BOOT does not rewrite KRN role, era, or profile declarations.

### 20.4 Provider attribution

KRN, ASM, toolchains, validation providers, firmware inspectors, and other providers may each produce profile-relevant evidence.

Provider evidence MUST remain attributable.

Provider-operation activity MUST NOT be represented as universal BOOT progress.

## 21. Aggregate system-profile boundary

KRN profile identity is one possible component of a broader system profile.

KRN-0002 does not define or own aggregate `SystemProfileIdentity`.

A future owning architecture may reference:

- kernel-profile revision;
- micro-OS-profile revision;
- hardware observations;
- runtime profile;
- backend profile;
- security profile;
- resource profile;
- other installation-level facts.

KRN MUST contribute only its own declared slice.

## 22. Public conformance expectations

Public conformance evidence for KRN-0002 should demonstrate that:

1. role, era, and profile remain independent;
2. one profile references one exact primary role revision;
3. one profile references one exact era-declaration revision;
4. combined roles require explicit composite-role declarations;
5. architecture-family eligibility does not assume one product class;
6. hardware-class eligibility does not become machine inventory;
7. era declarations remain multidimensional;
8. era labels do not replace era dimensions;
9. exact year ranges require evidence;
10. source, patches, configuration, toolchain, and ABI remain attributable;
11. profile revision identity is immutable;
12. material changes create new revisions;
13. capability obligation and evidence remain distinguishable;
14. unsupported, unavailable, restricted, degraded, unknown, and conflicting remain distinct;
15. limitations remain part of profile meaning;
16. validation remains scoped and attributable;
17. successful boot does not establish full profile conformance;
18. profile comparison does not assume one total ordering;
19. substitution requires an explicit governed claim;
20. KRN declarations remain distinct from BOOT-selected instances;
21. ASM observations remain provider-attributed evidence;
22. aggregate system-profile ownership remains deferred.

## 23. Prohibited interpretations

KRN-0002 MUST NOT be interpreted to mean that:

- every profile supports every role;
- one build may ambiguously claim unrelated roles;
- one architecture family identifies one board or vendor;
- an era is a calendar year;
- newer source automatically means a newer era in every dimension;
- a profile name is sufficient identity;
- a source revision is sufficient profile identity;
- configured support is demonstrated support;
- driver availability is device-use authority;
- a limitation disappears when a workaround exists;
- successful boot proves micro-OS compatibility;
- profile validation is BOOT pair evaluation;
- KRN owns the selected component-set instance;
- KRN owns assembly generations;
- ASM owns BOOT progression;
- BOOT may rewrite KRN declarations;
- aggregate `SystemProfileIdentity` belongs to KRN.

## 24. Deferred architecture

The following remain deliberately deferred:

- approval or rejection of candidate kernel roles;
- final names of candidate roles;
- exact role-specific capability catalogs;
- concrete era labels;
- exact era-range syntax;
- exact identifier encoding;
- exact record serialization;
- compatibility-declaration schema;
- required-component declaration schema;
- profile lifecycle transitions;
- conformance levels;
- production release channels;
- aggregate `SystemProfileIdentity` ownership.

Deferral does not permit implementation to choose a permanent answer silently.

## 25. Initial architectural commitments

KRN-0002 establishes that:

1. role, era, and profile are separate primary dimensions;
2. each profile has one exact primary role revision;
3. composite roles require explicit role definitions;
4. era is a multidimensional compatibility vector;
5. exact era labels and calendar ranges remain evidence-dependent;
6. architecture family does not imply platform identity;
7. hardware-class eligibility does not become local inventory;
8. profile identity exceeds source and version identity;
9. exact profile revisions are immutable;
10. material changes require new attributable revisions;
11. source, patches, configuration, toolchain, ABI, and licensing remain attributable;
12. capability obligation, support, and evidence remain distinguishable;
13. limitations remain part of profile meaning;
14. validation remains scoped and provider-attributed;
15. successful boot is insufficient for profile conformance;
16. profile comparison is multidimensional;
17. newer does not automatically mean substitutable;
18. KRN defines reusable declarations;
19. ASM defines the concrete assembly role and reports provider evidence;
20. BOOT selects and evaluates exact profile revisions within BOOT-owned generations;
21. aggregate system-profile ownership remains deferred;
22. implementation does not begin through this specification.

## Parallel Boundary Ledger

### Ledger entry 1 — Assembly-role detail

**Decision:** KRN defines the common role structure; ASM defines the concrete assembly-kernel role.

**Owning series:** KRN for common structure; ASM for concrete role requirements.

**Affected document:** KRN-0002, Sections 7, 8, and 20.

**Other series affected:** ASM-0000 through later ASM role and conformance specifications.

**Question or required confirmation:** Exact assembly-role capabilities and failure behavior remain ASM-owned.

**Blocking or non-blocking:** Non-blocking for KRN-0002.

### Ledger entry 2 — Candidate role catalog

**Decision:** Only the assembly-kernel role is presently established. Other listed roles remain candidates.

**Owning series:** KRN, with applicable future role-specific subseries.

**Affected document:** KRN-0002, Section 8.

**Other series affected:** Future runtime, recovery, edge, compute, storage, enablement, legacy, and validation architecture.

**Question or required confirmation:** Which candidate roles warrant independent normative subseries?

**Blocking or non-blocking:** Non-blocking.

### Ledger entry 3 — Era labels

**Decision:** Era is a multidimensional declaration rather than a calendar label.

**Owning series:** KRN.

**Affected document:** KRN-0002, Section 9.

**Other series affected:** ASM hardware inspection, BOOT pair evaluation, package and toolchain architecture.

**Question or required confirmation:** Concrete reusable era declarations require evidence from actual target classes.

**Blocking or non-blocking:** Non-blocking for the model; blocking before concrete era labels are declared normative.

### Ledger entry 4 — Compatibility schema

**Decision:** KRN-0002 provides profile-side references but does not define the complete kernel/micro-OS compatibility schema.

**Owning series:** KRN-0003.

**Affected document:** KRN-0002, Sections 12 and 20.

**Other series affected:** ASM and BOOT-0005.

**Question or required confirmation:** Exact compatibility and required-component fields remain for KRN-0003.

**Blocking or non-blocking:** Non-blocking.

### Ledger entry 5 — BOOT-selected instances

**Decision:** KRN profiles are reusable declarations; BOOT owns selected component-set instances and pair evaluation.

**Owning series:** KRN and BOOT within their respective boundaries.

**Affected document:** KRN-0002, Section 20.

**Other series affected:** ASM.

**Question or required confirmation:** BOOT-0005 must consume exact profile revisions through opaque references until schemas stabilize.

**Blocking or non-blocking:** Non-blocking.

### Ledger entry 6 — Lifecycle standing

**Decision:** KRN-0002 defines revision identity and validation inputs but not the final lifecycle state machine.

**Owning series:** KRN-0005.

**Affected document:** KRN-0002, Sections 17 and 19.

**Other series affected:** BOOT eligibility and profile-maintenance processes.

**Question or required confirmation:** Exact maturity, maintenance, succession, and conformance states remain for KRN-0005.

**Blocking or non-blocking:** Non-blocking.

### Ledger entry 7 — Aggregate system profile

**Decision:** KRN contributes kernel-profile identity but does not own aggregate `SystemProfileIdentity`.

**Owning series:** Deferred.

**Affected document:** KRN-0002, Section 21.

**Other series affected:** ACS runtime integration, BOOT, runtime, backend, security, and resource architecture.

**Question or required confirmation:** Future aggregate-profile ownership and schema.

**Blocking or non-blocking:** Non-blocking for KRN-0002.

## Revision history

### Version 0.1 — 2026-07-18

- Established the independent role, era, and profile axes.
- Defined role, era, profile, and revision identity layers.
- Established one primary role per profile.
- Established explicit composite-role requirements.
- Recorded the assembly-kernel role as the first established role.
- Recorded additional roles as non-normative candidates.
- Defined the multidimensional kernel-era model.
- Defined architecture-family and hardware-class eligibility.
- Defined profile fields and immutable revision rules.
- Defined source, patch, configuration, toolchain, ABI, and licensing attribution.
- Defined orthogonal capability obligation, support, and evidence dimensions.
- Defined limitation and validation models.
- Defined multidimensional profile comparison and substitution rules.
- Preserved ASM and BOOT ownership boundaries.
- Preserved deferred aggregate `SystemProfileIdentity` ownership.
