# BOOT-0005: Recovery Plans and Artifact Boundaries

| Field | Value |
|---|---|
| Specification | BOOT-0005 |
| Title | Recovery Plans and Artifact Boundaries |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | BOOT-PUB |
| Authors | Node |
| Last updated | 2026-07-18 |
| Approval | Pending review |
| Depends on | BOOT-0000 through BOOT-0004; applicable KRN-0000 through KRN-0002; applicable ASM-0000 through ASM-0001; applicable ACS, installer, security-provider, artifact-provider, MEM, IMM, resource-management, platform, and runtime architecture |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in recovery-plan semantics, selected component-set instances, provider-operation attribution, candidate-output evaluation, scoped artifact acceptance, generation membership, installation eligibility, installer handoff, activation, rollback, recovery, and reconciliation boundaries; exact KRN compatibility and component schemas, exact ASM provider contracts, artifact formats, installer interfaces, aggregate system-profile ownership, and production policy remain intentionally unresolved |

> **An environment may be capable of producing an output, and a KRN declaration may describe that output’s claimed profile, without BOOT having accepted the output, admitted it into an assembly generation, authorized its installation, activated it, or established recovery.**

## Architectural-intent notice

This specification defines independently authored public architecture for:

- recovery-plan proposals;
- BOOT recovery plans;
- recovery objectives;
- plan-operation graphs;
- selected component-set instances;
- opaque KRN declaration references;
- BOOT semantic operations and attempts;
- provider-operation requests and observations;
- ASM candidate-output handling;
- candidate-reference admission;
- scoped artifact acceptance;
- assembly-generation membership;
- assembly-generation identity and revision;
- provenance and licensing evaluation;
- KRN lifecycle and revalidation inputs;
- installation-eligibility evaluation;
- installer handoff;
- installer-result representation;
- activation and rollback coordination;
- interruption and unknown-mutation reconciliation;
- recovery verification;
- reboot and runtime-handoff eligibility;
- boundedness, failure, uncertainty, and conformance.

This specification does not define:

- a kernel role, era, profile, configuration, patch set, build system, or maintenance model;
- the internal schema of a KRN profile, compatibility declaration, or required-component declaration;
- the assembly kernel or assembly environment;
- compiler, linker, package, filesystem, driver, device-control, workspace, cache, staging, or test mechanisms;
- transport or ACS relationship semantics;
- cryptographic verification primitives;
- artifact or release formats;
- durable production mutation;
- runtime readiness;
- aggregate `SystemProfileIdentity`.

KRN supplies reusable declarations and identity relationships.

ASM supplies bounded assembly-environment mechanisms, hardware observations, provider-operation results, resource outcomes, and candidate outputs.

BOOT governs whether and how selected recovery and assembly work may progress.

The installer owns durable production mutation.

The normal runtime owns readiness.

## 1. Purpose

BOOT may determine that normal startup cannot proceed safely and that a governed recovery plan is required.

The required work may include:

- selecting a kernel profile and micro-OS profile;
- evaluating a KRN compatibility declaration for a local selected pair;
- selecting required and optional component instances;
- requesting authorized ASM provider operations;
- admitting candidate outputs for evaluation;
- accepting artifacts for a bounded BOOT purpose;
- accepting artifacts or components into an assembly generation;
- evaluating installation eligibility;
- handing eligible work to an installer;
- reconciling partial or unknown mutation;
- coordinating activation or rollback;
- verifying a recovery objective;
- preparing reboot or runtime handoff.

These stages are related, but none silently proves a later stage.

The canonical progression is:

```text
recovery objective
    → recovery-plan proposal
    → BOOT plan validation
    → selected component-set instance
    → selected-pair evaluation
    → operation-authority evaluation
    → provider-operation request
    → provider-operation result
    → candidate-output evaluation
    → scoped artifact acceptance
    → generation-membership acceptance
    → assembly-generation validation
    → installation-eligibility evaluation
    → installer handoff
    → installer-result evaluation
    → activation or rollback coordination
    → recovery verification
    → runtime-handoff eligibility
    → runtime-owned readiness
```

No stage silently proves the next.

## 2. Authority and relationship to earlier BOOT specifications

BOOT-0000 establishes BOOT’s responsibility to coordinate recovery plans, artifacts, installer handoff, recovery verification, and handoff without absorbing adjacent ownership.

BOOT-0001 requires:

- image boot and hardware discovery not to create authority;
- self-assembly to remain governed;
- kernel and micro-OS pairing to remain scoped;
- source, toolchain, dependency, artifact, and operation lineage to remain attributable;
- transfer, verification, installation, activation, recovery, and runtime readiness to remain distinct;
- retry and interruption not to fabricate completion;
- work and retained state to remain bounded.

BOOT-0002 defines independent dimensions for:

- authority;
- artifacts;
- installer work;
- recovery;
- handoff;
- conditional assembly activity.

BOOT-0003 defines identity, trust, authentication, authority, delegation, concurrence, revocation, and revalidation boundaries.

BOOT-0004 defines assistance discovery, plan-proposal requests, artifact referrals, provider sessions, and assistance-operation results.

BOOT-0005 refines the recovery-plan, provider-operation, artifact, assembly-generation, installer, activation, rollback, recovery, and reconciliation semantics consumed by those specifications.

If this specification conflicts with a lower-numbered BOOT specification:

1. the lower-numbered specification governs;
2. the conflict MUST be reported;
3. BOOT-0005 MUST be corrected;
4. implementations MUST NOT select the more permissive interpretation.

## 3. KRN and ASM boundary

The enduring boundary is:

> **KRN declares kernel-profile meaning and compatibility facts. ASM provides assembly-environment mechanisms. BOOT governs whether and how a selected assembly may progress.**

KRN owns reusable declaration structures and identity semantics for:

- `KernelRoleIdentity`;
- `KernelEraDeclarationIdentity`;
- `KernelProfileIdentity`;
- `KernelProfileRevisionIdentity`;
- `MicroOsProfileIdentity`;
- kernel-profile declarations;
- micro-OS-profile declarations;
- kernel/micro-OS compatibility declarations;
- required-component declarations;
- source, patch, configuration, toolchain, ABI, build, artifact, validation, conformance, and lifecycle references.

ASM owns:

- the assembly-kernel role;
- the assembly environment;
- environment-capability reporting;
- hardware observation;
- provider-operation mechanisms;
- resource outcomes;
- candidate-output production;
- ASM output staging;
- provider-scoped failure and cleanup reporting.

BOOT owns:

- the selected component-set instance;
- selected-pair evaluation;
- semantic assembly-operation identity;
- semantic operation attempts;
- assembly-plan semantics;
- assembly-generation identity and state;
- authority evaluation;
- source, release, and artifact acceptance;
- compatibility acceptance for the selected scope;
- installation eligibility;
- installer handoff;
- installer-result coordination;
- activation;
- rollback and reconciliation coordination;
- recovery;
- reboot preparation;
- runtime-handoff eligibility.

BOOT MUST NOT redefine KRN declarations or ASM mechanisms to solve a local drafting or implementation problem.

## 4. Normative language

The terms **MUST** and **MUST NOT** define mandatory architectural requirements.

The terms **SHOULD** and **SHOULD NOT** define strong recommendations. A departure requires documented justification and MUST NOT violate a mandatory requirement.

The term **MAY** defines permitted behavior.

Terms such as:

- valid;
- compatible;
- accepted;
- selected;
- produced;
- verified;
- installed;
- staged;
- active;
- recovered;
- ready

MUST identify their subject, scope, provider, revision, evidence boundary, and lifecycle boundary.

Unqualified favorable terms MUST NOT collapse independent stages.

## 5. Explicit non-goals

BOOT-0005 does not define:

- one recovery-plan encoding;
- one dependency-graph implementation;
- one kernel profile;
- one micro-OS profile;
- one compatibility-declaration schema;
- one required-component schema;
- one component catalog;
- one compiler, assembler, linker, archiver, build system, or package manager;
- one source repository or source-access protocol;
- one artifact format or manifest format;
- one signature or digest algorithm;
- one release authority;
- one installer API;
- one bootloader;
- one activation mechanism;
- one rollback mechanism;
- one runtime-readiness contract;
- one aggregate installed-system identity;
- one production authority, signing, rollout, or retention policy.

Deferral does not transfer ownership to the first implementation that requires an answer.

## 6. Foundational distinctions

### 6.1 Recovery-plan proposal is not a BOOT recovery plan

A proposal is candidate semantic input.

It is not accepted merely because it is:

- well formed;
- signed;
- produced by an authenticated source;
- produced by IMM;
- proposed by an operator;
- generated locally;
- previously successful.

### 6.2 Valid plan is not authorized plan

Structural and semantic validity do not create operation authority.

### 6.3 KRN declaration is not BOOT selection

A KRN declaration is reusable profile or compatibility information.

It does not establish a selected component set, selected pair, local availability, local integrity, or assembly generation.

### 6.4 Required-component declaration is not selected component set

A requirement does not prove that a component is present, selected, accepted, or satisfies the requirement.

### 6.5 Environment capability is not operation authority

ASM capability reporting describes possible mechanisms.

Capability does not authorize their use.

### 6.6 Hardware observation is not target eligibility

Observation may identify a substrate, device, or storage object.

It does not establish:

- durable identity;
- safe usability;
- device-use authority;
- installation eligibility;
- health;
- recovery.

### 6.7 BOOT semantic operation is not provider operation

BOOT owns why an operation exists, its semantic scope, authority, progression, and acceptance.

A provider owns its local mechanism operation and attempt.

### 6.8 Process execution is not provider-operation identity

A process, thread, compiler invocation, linker invocation, transport connection, or exit status does not replace semantic or provider-operation identity.

### 6.9 ASM candidate output is not KRN artifact identity

An ASM candidate output identifies a provider-produced physical or logical result.

A KRN artifact identity participates in KRN’s source, profile, build, artifact, and deployed-instance identity hierarchy.

One object may be referenced by both layers, but the layers are not interchangeable.

### 6.10 KRN artifact identity is not BOOT scoped artifact acceptance

KRN artifact identity does not establish that BOOT accepts the object for a local purpose.

### 6.11 BOOT scoped artifact acceptance is not generation membership

BOOT may accept an artifact for evaluation, rollback evidence, verification, or another bounded purpose without accepting it as a member of an assembly generation.

### 6.12 Generation membership is not installation eligibility

Membership means the object is accepted into an identified generation context.

Installation eligibility requires additional authority, target, verification, compatibility, installer, resource, rollback, and reconciliation conditions.

### 6.13 ASM output staging is not BOOT `GENERATION_STAGED`

ASM output staging is provider-owned workspace containing candidate outputs.

BOOT `GENERATION_STAGED` is a semantic state concerning an identified assembly generation and its accepted members.

### 6.14 Transfer is not verification

Byte transfer does not establish integrity, authenticity, completeness, provenance, compatibility, or authority.

### 6.15 Verification is property-specific

A digest, signature, structure, manifest, or provenance result establishes only its declared property and scope.

### 6.16 Verification is not compatibility

An artifact may be authentic and incompatible.

### 6.17 Compatibility is not authority

A compatible pair or component set is not automatically authorized to build, install, activate, or propagate.

### 6.18 Provider-operation completion is not candidate-output acceptance

A provider may complete its declared operation while producing no acceptable output.

### 6.19 Candidate output exists is not generation established

Physical production does not create BOOT generation identity or membership.

### 6.20 Installation eligibility is not installer acceptance

BOOT may determine that a request is eligible for installer consideration.

The installer may still reject, defer, narrow, or fail the request.

### 6.21 Installer acceptance is not mutation

Request acceptance does not prove durable state changed.

### 6.22 Mutation is not activation

Durable production mutation and activation remain separate stages.

### 6.23 Activation is not recovery

An activated generation may fail its recovery objective.

### 6.24 Recovery is not runtime readiness

BOOT may establish handoff eligibility.

The runtime owns readiness.

### 6.25 Resource sufficiency is not authority or success

Available capacity does not grant work authority, admission, acceptance, or completion.

### 6.26 Newest is not eligible

The newest kernel, micro-OS, toolchain, source, build, artifact, output, plan, or generation is not automatically preferred or eligible.

## 7. Core terminology

### 7.1 Recovery objective

A **recovery objective** identifies the bounded condition a recovery plan is intended to establish, restore, replace, validate, or prepare.

### 7.2 Recovery-plan proposal

A **recovery-plan proposal** is an unaccepted, revisioned candidate description of recovery work.

### 7.3 BOOT recovery plan

A **BOOT recovery plan** is an identified, revisioned, bounded semantic coordination object validated by BOOT for further consideration.

Plan validity does not establish plan authority.

### 7.4 Plan operation

A **plan operation** is one semantic operation within a recovery plan.

It identifies its owning provider class and result contract without embedding the provider’s mechanism.

### 7.5 Selected component-set instance

A **selected component-set instance** is BOOT’s identified selection of required, conditional, optional, substituted, and omitted component instances for one governed scope.

### 7.6 Pair evaluation

A **pair evaluation** is BOOT’s scoped evaluation of one selected kernel-profile revision and one selected micro-OS-profile revision against an applicable KRN compatibility declaration, selected component-set instance, local evidence boundary, authority boundary, and assembly generation.

### 7.7 BOOT semantic operation identity

A **BOOT semantic operation identity** identifies the continuing semantic operation requested, governed, and interpreted by BOOT.

### 7.8 BOOT operation-attempt identity

A **BOOT operation-attempt identity** identifies one BOOT-controlled attempt to satisfy the semantic operation.

### 7.9 Provider-operation identity

A **provider-operation identity** identifies one provider-scoped mechanism operation correlated with, but distinct from, the BOOT semantic operation.

### 7.10 Provider-attempt identity

A **provider-attempt identity** identifies one local attempt by the provider to perform the provider operation.

### 7.11 ASM candidate output

An **ASM candidate output** is a physical or logical output produced through an ASM provider operation but not accepted by BOOT into an assembly-generation context.

### 7.12 Candidate reference

A **candidate reference** is a bounded reference through which BOOT may identify and evaluate a provider-produced candidate output.

### 7.13 KRN artifact reference

A **KRN artifact reference** is an opaque revision-aware reference to a KRN-governed artifact identity.

### 7.14 BOOT scoped artifact acceptance

A **BOOT scoped artifact acceptance** is BOOT’s identified and revisioned decision that an artifact or candidate output is accepted for one declared BOOT purpose.

### 7.15 Generation member

A **generation member** is an accepted artifact or component reference associated with a particular assembly-generation revision.

### 7.16 Assembly generation

An **assembly generation** is BOOT’s identified, revisioned semantic grouping of:

- selected KRN profile references;
- a selected component-set instance;
- accepted generation members;
- provider-operation results;
- applicable compatibility and validation evidence;
- authority references;
- intended installation, activation, rollback, recovery, and handoff scope;
- limitations and unresolved conditions.

An assembly generation is not:

- an assembly kernel;
- an assembly environment;
- an ASM workspace;
- an ASM output-staging directory;
- a package database;
- an installer transaction;
- a deployed runtime;
- an aggregate `SystemProfileIdentity`.

### 7.17 Installation eligibility

**Installation eligibility** is BOOT’s scoped conclusion that an identified installer request may be considered under current evidence, authority, target, generation, artifact, resource, rollback, and reconciliation conditions.

### 7.18 Unknown-mutation condition

An **unknown-mutation condition** exists when BOOT cannot establish whether a provider or installer operation changed durable or authoritative state.

### 7.19 Reconciliation

**Reconciliation** evaluates current observations and provider results against prior operation, attempt, revision, target, and expected-effect identities.

## 8. Opaque KRN references

BOOT-0005 consumes KRN declarations through opaque, exact, revision-aware references.

Applicable references may include:

```text
kernel_role_revision_reference
kernel_era_declaration_revision_reference
kernel_profile_revision_reference
micro_os_profile_revision_reference
compatibility_declaration_revision_reference
required_component_declaration_revision_reference
kernel_source_reference
kernel_patch_set_reference
kernel_configuration_reference
kernel_toolchain_reference
kernel_abi_declaration_reference
kernel_validation_record_reference
kernel_build_reference
kernel_artifact_reference
kernel_lifecycle_reference
```

BOOT MUST NOT:

- replace an exact revision reference with a mutable display name;
- infer a revision from a filename;
- infer compatibility from co-location or successful startup;
- modify a KRN declaration to record a local BOOT selection;
- treat a prior pair evaluation as permanently valid;
- define a substitute KRN schema.

Where a KRN schema is not yet stable, BOOT uses an opaque reference and preserves ownership.

## 9. Micro-OS identity use

BOOT consumes one primary `MicroOsProfileIdentity` for the selected micro-OS profile.

BOOT consumes subordinate micro-OS component identities only when KRN declarations identify them as independently governed.

BOOT does not independently create subordinate profile identities merely because files, packages, services, initramfs contents, firmware, or toolchain material exist.

## 10. Recovery-plan model

### 10.1 Plan identity and revision

Every plan MUST identify or reference:

- plan identity;
- plan revision;
- proposal identity and revision;
- BOOT session;
- recovery objective;
- target;
- selected component-set requirement;
- operation graph;
- authority requirements;
- provider requirements;
- artifact requirements;
- installer requirements;
- activation and rollback requirements;
- recovery criteria;
- handoff conditions;
- cancellation and interruption behavior;
- bounded issues.

### 10.2 Plan states

A public implementation SHOULD support plan results equivalent to:

| State | Meaning |
|---|---|
| `PLAN_PROPOSED` | An unaccepted proposal exists. |
| `PLAN_VALIDATION_PENDING` | Validation awaits work or prerequisites. |
| `PLAN_VALIDATING` | BOOT is evaluating the proposal. |
| `PLAN_VALID` | The plan satisfies its declared structural and semantic contract for further consideration. |
| `PLAN_VALID_WITH_LIMITATIONS` | The plan is valid only under explicit limitations. |
| `PLAN_INVALID` | The plan fails an applicable structural or semantic requirement. |
| `PLAN_UNSUPPORTED` | The required plan feature or provider contract is unsupported. |
| `PLAN_DEFERRED` | Evaluation or progression is postponed. |
| `PLAN_CONFLICTING` | Eligible plan evidence or revisions conflict. |
| `PLAN_EXPIRED` | The plan is outside its validity boundary. |
| `PLAN_SUPERSEDED` | A governed successor replaced the plan for the declared scope. |
| `PLAN_REVOKED` | Applicable authority withdrew future use. |
| `PLAN_INDETERMINATE` | The result cannot be established more precisely. |

`PLAN_VALID` does not mean authorized, admitted, executed, installed, active, or recovered.

### 10.3 Operation graph

A plan MAY use a bounded directed graph or equivalent representation for:

- required operations;
- optional operations;
- conditional operations;
- fallback operations;
- rollback operations;
- verification gates;
- authority gates;
- provider boundaries;
- cancellation;
- interruption;
- reconciliation.

BOOT-0005 does not mandate one graph serialization.

### 10.4 Operation ownership

Every plan operation MUST identify its semantic owner and provider class.

Possible provider classes include:

- BOOT evaluation;
- KRN declaration provider;
- ASM provider;
- artifact provider;
- security provider;
- authority provider;
- transport;
- ACS;
- installer;
- MEM;
- IMM;
- resource manager;
- platform provider;
- runtime-observation provider.

Technical co-location does not collapse these roles.

### 10.5 Plan authority

Every privileged plan operation MUST identify:

- actor;
- target;
- operation class;
- input scope;
- permitted output classes;
- resource limits;
- validity;
- authority source;
- delegation or concurrence requirements;
- mutation prohibition where applicable;
- execution prohibition where applicable;
- propagation prohibition where applicable;
- revocation behavior.

## 11. Selected component-set instance

A selected component-set instance SHOULD identify:

```text
component_set_identity
component_set_revision
boot_session_identity
plan_reference
assembly_generation_reference
kernel_profile_revision_reference
micro_os_profile_revision_reference
compatibility_declaration_revision_reference
required_component_declaration_revision_reference
selected_required_component_references
selected_conditional_component_references
selected_optional_component_references
substitution_references
omitted_optional_component_references
unresolved_component_references
target_reference
selection_policy_reference
selection_authority_reference
evaluation_state
limitations
bounded_issues
```

### 11.1 Required components

A required or conditionally required component MUST NOT be reported satisfied unless the selected instance satisfies the applicable KRN declaration under current evidence.

Results SHOULD distinguish:

- satisfied;
- missing;
- unavailable;
- stale;
- invalid;
- incompatible;
- substitution unresolved;
- conflicting;
- unsupported;
- indeterminate.

### 11.2 Optional components

An optional component may be omitted only when:

- the KRN declaration permits omission for the selected scope;
- required components remain satisfied;
- the resulting limitation remains explicit;
- authority permits the reduced scope;
- installer and later runtime contracts can represent the limitation.

### 11.3 Substitution

A substitution MUST preserve:

- original requirement reference;
- substitute identity and revision;
- substitution boundary;
- validation requirement;
- pair-evaluation result;
- limitation;
- authority where required.

Discovery of a substitute does not select it.

Selection does not establish that it satisfies the declaration.

## 12. Pair evaluation

A BOOT pair evaluation SHOULD identify:

```text
evaluation_identity
evaluation_revision
boot_session_identity
plan_reference
assembly_generation_reference
kernel_profile_revision_reference
micro_os_profile_revision_reference
compatibility_declaration_revision_reference
required_component_declaration_revision_reference
selected_component_set_reference
local_availability_evidence_references
artifact_acceptance_references
hardware_observation_references
environment_capability_references
provider_result_references
declaration_lifecycle_reference
declaration_freshness_state
required_component_result
optional_component_result
compatibility_result
limitations
revalidation_requirement
bounded_issues
```

### 12.1 Pair-evaluation results

A public implementation SHOULD support values equivalent to:

| Result | Meaning |
|---|---|
| `PAIR_NOT_EVALUATED` | No local BOOT pair evaluation exists. |
| `PAIR_EVALUATION_PENDING` | Evaluation awaits work or prerequisites. |
| `PAIR_COMPATIBLE_FOR_SCOPE` | The selected pair satisfies the applicable declaration for the declared scope. |
| `PAIR_COMPATIBLE_WITH_LIMITATIONS` | The selected pair satisfies only a restricted scope. |
| `PAIR_INCOMPATIBLE` | The selected pair fails the applicable declaration. |
| `PAIR_REQUIRED_COMPONENT_MISSING` | A required selected component is absent or unavailable. |
| `PAIR_REQUIRED_COMPONENT_CONFLICTING` | Required component evidence conflicts. |
| `PAIR_DECLARATION_UNAVAILABLE` | The applicable KRN declaration is inaccessible. |
| `PAIR_DECLARATION_STALE` | The declaration or material evidence requires revalidation. |
| `PAIR_DECLARATION_RESTRICTED` | Lifecycle or policy restricts use. |
| `PAIR_DECLARATION_RETIRED` | The declaration is retired for the evaluated scope. |
| `PAIR_DECLARATION_REVOKED` | The declaration is revoked for the evaluated scope. |
| `PAIR_UNSUPPORTED` | Evaluation cannot represent the required declaration or profile. |
| `PAIR_INDETERMINATE` | Compatibility cannot be established more precisely. |

A failed or deferred BOOT pair evaluation MUST NOT rewrite or universally invalidate the reusable KRN declaration.

A valid KRN declaration MUST NOT override a failed or deferred local BOOT evaluation.

## 13. Provider-operation identity and correlation

### 13.1 Required identity separation

BOOT-0005 preserves four distinct identities:

```text
BOOT semantic operation identity
BOOT operation-attempt identity
provider-operation identity
provider-attempt identity
```

For ASM work, the latter two are ASM provider-operation and ASM provider-attempt identities.

### 13.2 BOOT semantic operation

The BOOT semantic operation identifies:

- why work exists;
- its target and scope;
- its authority;
- its expected provider result;
- its expected candidate outputs;
- its progression and acceptance conditions.

### 13.3 Provider operation

The provider operation identifies:

- the provider;
- the local mechanism contract;
- the provider attempt;
- applied resource limits;
- actual mechanism results;
- produced outputs;
- partial and indeterminate effects;
- cleanup state.

### 13.4 Process correlation

A provider MAY correlate its operation to processes, threads, compiler invocations, linker invocations, filesystem work, transport sessions, or device operations.

Those mechanism identities MUST NOT replace provider-operation or BOOT semantic identities.

### 13.5 Provider-attributed active state

BOOT MUST represent active provider work using a provider-attributed state such as:

```text
PROVIDER_OPERATION_ACTIVE
```

BOOT MUST NOT use wording implying that BOOT itself performs compilation, linking, packaging, testing, staging, or other ASM mechanisms.

## 14. Node-targeted work authority

Node-targeted provider work includes compilation, linking, packaging, testing, transformation, validation, or output production intended for possible incorporation into:

- a selected component-set instance;
- an assembly generation;
- installation input;
- activation material;
- rollback material;
- recovery material;
- propagatable Node material.

Such work requires applicable BOOT authority.

Authority for one source, target, component, profile, generation, toolchain, operation, or attempt MUST NOT be generalized to another.

### 14.1 Authority-not-required work

An operation may be `AUTHORITY_NOT_REQUIRED` only when an owning contract explicitly declares that no additional BOOT authority is required.

Permitted work must remain bounded to:

- compiler diagnostics;
- feature detection;
- toolchain probing;
- disposable test compilation;
- temporary compatibility experiments;
- isolated non-production conformance tests.

The owning contract MUST identify:

- why authority is not required;
- permitted inputs;
- permitted outputs;
- resource limits;
- retention rules;
- disposal or quarantine rules;
- prohibited downstream use.

Outputs from authority-not-required work MUST NOT be silently:

- accepted as BOOT artifacts;
- accepted as generation members;
- installed;
- activated;
- propagated;
- executed as normal Node code;
- retained as production material;
- used to replace an authorized build.

Absence of an explicit owning contract means applicable BOOT authority is required.

## 15. Provider-operation contracts

### 15.1 BOOT request

A conceptual request contains:

```text
BootProviderOperationRequest
    boot_operation_identity
    boot_attempt_identity
    plan_reference
    assembly_generation_reference
    provider_class
    provider_target
    operation_class
    input_references
    permitted_output_classes
    authority_reference
    resource_limits
    mutation_prohibition
    execution_prohibition
    propagation_prohibition
    result_contract
    idempotency_reference
```

### 15.2 Provider observation

A conceptual provider observation contains:

```text
BootProviderOperationObservation
    boot_operation_identity
    boot_attempt_identity
    provider_operation_reference
    provider_attempt_reference
    provider_identity
    provider_progress_state
    provider_outcome_state
    provider_result_reference
    candidate_output_references
    resource_outcome_reference
    cleanup_state
    unresolved_effects
    reconciliation_requirement
    bounded_issues
```

BOOT consumes the provider result.

BOOT does not replace it with a locally fabricated mechanism result.

### 15.3 Provider outcomes

Provider outcomes MAY include:

- accepted;
- admission pending;
- admitted;
- active;
- waiting;
- completed;
- completed with limitations;
- no output;
- partial output;
- canceled;
- interrupted;
- failed;
- resource exhausted;
- unsupported;
- indeterminate.

A provider outcome remains provider-scoped.

## 16. Candidate-output model

### 16.1 Candidate-output states

BOOT may consume ASM states equivalent to:

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

`Produced` means only that the provider reports the physical or logical output was emitted.

### 16.2 Candidate-output record

A candidate-output reference used by BOOT SHOULD preserve:

- provider identity;
- provider-operation reference;
- provider-attempt reference;
- BOOT semantic operation reference;
- BOOT attempt reference;
- source and input references;
- claimed output class;
- claimed KRN profile or build references where applicable;
- location or protected handle;
- output revision;
- size or bounded shape information where applicable;
- provider validation state;
- resource outcome;
- quarantine state;
- cleanup state;
- limitations;
- unresolved effects.

### 16.3 Running assembly kernel versus candidate kernel

The currently running assembly-kernel profile MUST remain distinct from any kernel profile represented by a candidate output.

A candidate output MUST NOT inherit:

- the running kernel’s profile identity;
- the running micro-OS identity;
- the running pair compatibility;
- the running environment’s authority;
- the running environment’s operational state.

## 17. Candidate-reference admission

### 17.1 Purpose

Candidate-reference admission permits BOOT to retain and evaluate a bounded candidate reference.

It does not accept the candidate as an artifact or generation member.

### 17.2 Transition

```text
CANDIDATE_OUTPUT_REPORTED
    → CANDIDATE_REFERENCE_ADMISSION_PENDING
    → CANDIDATE_REFERENCE_ADMITTED
```

Possible non-success results include:

- rejected;
- deferred;
- expired;
- malformed;
- unavailable;
- conflicting;
- resource exhausted;
- unsupported;
- indeterminate.

### 17.3 Admission requirements

Admission SHOULD evaluate:

- provider attribution;
- operation correlation;
- reference structure;
- output class;
- result completeness;
- classification;
- retention bounds;
- quarantine state;
- prohibited-use state;
- unresolved effects.

Candidate-reference admission MUST NOT fabricate identity, provenance, verification, compatibility, or authority.

## 18. Scoped artifact acceptance

### 18.1 Purpose

Scoped artifact acceptance determines whether an artifact or candidate output is acceptable for one identified BOOT purpose.

Applicable purposes MAY include:

- further evaluation;
- pair evaluation;
- generation membership;
- installation input;
- rollback input;
- verification input;
- recovery evidence;
- handoff evidence.

### 18.2 Transition

```text
CANDIDATE_REFERENCE_ADMITTED
    → ARTIFACT_ACCEPTANCE_PENDING
    → ARTIFACT_ACCEPTED_FOR_SCOPE
```

An artifact accepted for one scope is not accepted for every scope.

### 18.3 Acceptance inputs

Acceptance MAY require separate results concerning:

- operation authority;
- source identity;
- source authority;
- release identity;
- release authority;
- KRN profile and build references;
- toolchain eligibility;
- output structure;
- integrity;
- authenticity;
- completeness;
- provenance;
- licensing;
- target compatibility;
- lifecycle standing;
- required-component relevance;
- optional-component state;
- classification;
- retention and custody;
- unresolved limitations.

### 18.4 Acceptance results

A public implementation SHOULD support results equivalent to:

| Result | Meaning |
|---|---|
| `ARTIFACT_NOT_EVALUATED` | No scoped acceptance evaluation exists. |
| `ARTIFACT_ACCEPTANCE_PENDING` | Evaluation awaits work or prerequisites. |
| `ARTIFACT_ACCEPTED_FOR_SCOPE` | The artifact is accepted for the declared BOOT purpose. |
| `ARTIFACT_ACCEPTED_WITH_LIMITATIONS` | Acceptance is restricted. |
| `ARTIFACT_REJECTED` | The artifact fails the applicable acceptance contract. |
| `ARTIFACT_UNAVAILABLE` | The artifact or required provider is inaccessible. |
| `ARTIFACT_STALE` | Material evidence requires revalidation. |
| `ARTIFACT_REVOKED` | Applicable future use was revoked. |
| `ARTIFACT_CONFLICTING` | Eligible identity, provenance, lifecycle, or verification evidence conflicts. |
| `ARTIFACT_RESOURCE_EXHAUSTED` | Bounded evaluation could not complete. |
| `ARTIFACT_UNSUPPORTED` | Required artifact or verification semantics are unsupported. |
| `ARTIFACT_INDETERMINATE` | Acceptance cannot be established more precisely. |

## 19. Generation-membership acceptance

### 19.1 Separate decision

Generation membership is a separate BOOT transition after scoped artifact acceptance.

```text
ARTIFACT_ACCEPTED_FOR_SCOPE
    → GENERATION_MEMBERSHIP_PENDING
    → GENERATION_MEMBER_ACCEPTED
```

### 19.2 Membership requirements

Membership SHOULD evaluate:

- target generation identity and revision;
- selected component-set instance;
- component role;
- required or optional status;
- exact KRN profile revision where applicable;
- exact KRN build and artifact references where applicable;
- pair-evaluation effect;
- required-component satisfaction;
- substitution status;
- lifecycle standing;
- authority;
- generation revision;
- limitations;
- conflict with existing members.

### 19.3 Membership results

Results SHOULD distinguish:

- accepted;
- accepted with limitations;
- rejected;
- required member missing;
- optional member omitted;
- conflicting;
- superseded;
- stale;
- revoked;
- unsupported;
- indeterminate.

A scoped artifact acceptance remains historically valid for its declared scope even when generation membership is rejected, unless the acceptance itself is later revoked or superseded.

## 20. Artifact identity, provenance, and licensing

### 20.1 Identity layers

BOOT records MUST preserve applicable distinctions among:

- source identity;
- profile identity;
- build identity;
- provider candidate-output identity;
- KRN artifact identity;
- artifact instance identity;
- generation-member identity;
- deployed-instance identity.

### 20.2 Provenance

Applicable provenance may include:

- upstream source;
- Node source revision;
- patch provenance and order;
- configuration revision;
- compiler and linker references;
- material build flags;
- ABI expectations;
- module assumptions;
- firmware assumptions;
- package and dependency references;
- provider environment references;
- input transformations;
- cache participation;
- test evidence;
- transfer history;
- verification results;
- custody changes;
- reconstruction;
- limitations and uncertainty.

### 20.3 Licensing

Licensing evidence MUST remain separate from technical build success.

BOOT MUST NOT infer distribution or installation permission merely because:

- source is public;
- compilation succeeded;
- a digest matches;
- a signature is valid;
- an artifact is cached;
- a prior release used similar material.

Artifact acceptance SHOULD preserve:

- applicable license references;
- attribution obligations;
- source-availability obligations;
- known incompatible or restricted material;
- unresolved licensing state;
- permitted use scope;
- distribution prohibition where applicable.

BOOT does not grant rights not supplied by applicable licenses or authority.

### 20.4 Missing provenance

Missing material provenance MUST prevent stronger profile, build, artifact, generation, installation, or propagation claims where the applicable contract requires it.

## 21. Verification model

### 21.1 Provider ownership

Security and artifact providers own verification primitives and property-specific results.

BOOT consumes:

- structural results;
- digest results;
- signature results;
- manifest-consistency results;
- completeness results;
- provenance results;
- release-identity results;
- policy-eligibility results.

### 21.2 No universal verified flag

BOOT MUST NOT use one unqualified `verified` value to imply every property.

### 21.3 Verification references

A verification result SHOULD identify:

- artifact or instance;
- property verified;
- provider;
- operation and attempt;
- algorithm or profile reference where public;
- revision;
- evidence boundary;
- freshness;
- limitations;
- outcome.

### 21.4 Verification failure

Failure of one property does not fabricate failure or success for another property.

## 22. Assembly-generation model

### 22.1 Generation creation

BOOT may create a candidate assembly generation only after constructing:

- generation identity;
- generation revision;
- BOOT session reference;
- plan reference;
- target reference;
- selected component-set reference;
- exact KRN profile revision references;
- accepted generation-member references;
- candidate references retained for evaluation;
- provider-operation references;
- authority requirements;
- expected installer scope;
- activation and rollback scope;
- recovery criteria;
- limitations and bounded issues.

### 22.2 Generation record

A conceptual record contains:

```text
BootAssemblyGeneration
    generation_identity
    generation_revision
    boot_session_identity
    plan_reference
    target_reference
    kernel_profile_revision_reference
    micro_os_profile_revision_reference
    compatibility_declaration_revision_reference
    required_component_declaration_revision_reference
    selected_component_set_reference
    accepted_generation_member_references
    candidate_reference_set
    rejected_candidate_references
    kernel_build_references
    kernel_artifact_references
    other_artifact_references
    provider_operation_references
    provider_result_references
    hardware_observation_references
    environment_capability_references
    pair_evaluation_reference
    provenance_evaluation_reference
    licensing_evaluation_reference
    lifecycle_evaluation_reference
    authority_references
    installation_eligibility
    activation_eligibility
    rollback_reference
    recovery_criteria_reference
    revalidation_state
    limitations
    bounded_issues
```

### 22.3 Generation states

A public implementation SHOULD support behavior equivalent to:

- `GENERATION_NOT_CREATED`;
- `GENERATION_PROPOSED`;
- `GENERATION_COMPONENT_SET_SELECTED`;
- `GENERATION_PROVIDER_OPERATIONS_PENDING`;
- `GENERATION_CANDIDATE_OUTPUTS_AVAILABLE`;
- `GENERATION_ARTIFACT_ACCEPTANCE_PENDING`;
- `GENERATION_MEMBERSHIP_EVALUATING`;
- `GENERATION_STAGED`;
- `GENERATION_VALIDATION_PENDING`;
- `GENERATION_REVALIDATION_REQUIRED`;
- `GENERATION_INSTALLATION_ELIGIBILITY_PENDING`;
- `GENERATION_INSTALLATION_ELIGIBLE`;
- `GENERATION_INSTALLATION_INELIGIBLE`;
- `GENERATION_INSTALLED_REPORTED`;
- `GENERATION_ACTIVATION_PENDING`;
- `GENERATION_ACTIVATION_ELIGIBLE`;
- `GENERATION_ACTIVATION_ATTEMPTED`;
- `GENERATION_ACTIVE_OBSERVED`;
- `GENERATION_RECOVERY_VERIFICATION_PENDING`;
- `GENERATION_RECOVERY_SATISFIED`;
- `GENERATION_RECOVERY_UNSATISFIED`;
- `GENERATION_ROLLBACK_PENDING`;
- `GENERATION_SUPERSEDED`;
- `GENERATION_REJECTED`;
- `GENERATION_INDETERMINATE`;
- `GENERATION_CLOSED`.

Not every operation requires every state.

Skipped states MUST be explicit and MUST NOT be reported as successful.

### 22.4 `GENERATION_STAGED`

`GENERATION_STAGED` means:

- an identified generation revision exists;
- its selected component-set reference exists;
- applicable accepted generation members are associated with it;
- required unresolved conditions are retained;
- its state can be evaluated for later eligibility.

It does not mean:

- ASM output staging exists;
- every artifact is verified;
- installation is eligible;
- installer work began;
- activation is eligible;
- recovery succeeded.

### 22.5 Generation revision

A material change to any of the following requires a new generation revision or identity according to the applicable continuity contract:

- selected kernel-profile revision;
- selected micro-OS-profile revision;
- compatibility-declaration revision;
- required-component declaration;
- selected component-set instance;
- accepted generation member;
- target;
- authority;
- installer scope;
- rollback target;
- activation scope;
- recovery objective.

### 22.6 Generation comparison

BOOT MUST NOT select a generation solely because it is:

- newest;
- smallest;
- fastest to build;
- first completed;
- already cached;
- locally produced;
- signed;
- previously installed.

## 23. KRN lifecycle and revalidation

### 23.1 Lifecycle inputs

BOOT may consume KRN standing concerning:

- profile maturity;
- maintenance;
- security support;
- restrictions;
- conformance;
- supersession;
- deprecation;
- retirement;
- revocation;
- replacement;
- conflict.

### 23.2 Material change

Material changes may include:

- upstream source;
- Node source;
- patch set;
- configuration;
- compiler;
- linker;
- build flags;
- ABI assumptions;
- firmware assumptions;
- required components;
- compatibility declaration;
- maintenance or security standing.

### 23.3 Revalidation requirements

When material inputs change, BOOT MUST:

- identify affected plans, selections, pair evaluations, artifacts, memberships, and generations;
- preserve prior identities and revisions;
- invalidate or restrict stale conclusions;
- mark applicable state revalidation-required;
- obtain current provider results;
- avoid silent inheritance of prior eligibility.

### 23.4 No silent reactivation

A retired, revoked, restricted, or superseded profile, declaration, artifact, or generation MUST NOT return to ordinary eligibility merely because:

- it remains available;
- it boots;
- it compiles;
- it is cached;
- a provider restarts;
- a newer path fails.

## 24. ASM output staging boundary

ASM output staging is a bounded ASM-owned workspace area containing candidate outputs pending:

- collection;
- evaluation;
- rejection;
- quarantine;
- cleanup;
- disposal.

ASM output staging is separate from:

- source areas;
- active intermediate work;
- the running assembly environment;
- production installation targets;
- active BOOT generations;
- installer rollback material.

BOOT MUST NOT enter `GENERATION_STAGED` solely because a file or output exists in ASM output staging.

## 25. Partial output, quarantine, and cleanup

### 25.1 Partial output

When partial candidate output may exist, BOOT MUST preserve provider-attributed information concerning:

- completed scope;
- incomplete scope;
- candidate references safe to expose;
- quarantine state;
- cleanup state;
- resource state;
- unresolved effects;
- prohibited use;
- reconciliation requirement.

### 25.2 Quarantine

A quarantined output MUST NOT be:

- accepted for an incompatible scope;
- admitted as a generation member;
- installed;
- activated;
- propagated;
- executed as ordinary Node software

until an applicable transition explicitly removes or supersedes the quarantine.

### 25.3 Cleanup states

Cleanup SHOULD distinguish:

- not required;
- pending;
- active;
- complete;
- failed;
- unavailable;
- indeterminate.

Cleanup completion does not create artifact acceptance.

Cleanup failure does not automatically prove substrate compromise or generation failure.

### 25.4 Indeterminate effects

When provider effects are indeterminate, BOOT MUST preserve:

- operation and attempt identities;
- possible outputs;
- possible mutations;
- affected resources;
- last known state;
- prohibited retries;
- required reconciliation.

## 26. Installation eligibility

### 26.1 Required inputs

Installation eligibility SHOULD evaluate applicable:

- target identity and target-state evidence;
- plan validity;
- authority;
- generation identity and revision;
- selected component-set result;
- pair evaluation;
- required-component completeness;
- scoped artifact acceptance;
- generation membership;
- verification results;
- provenance and licensing;
- lifecycle and freshness;
- installer capability;
- resource state;
- rollback preparation;
- unresolved mutation;
- policy conditions.

### 26.2 Results

A public implementation SHOULD support values equivalent to:

| Result | Meaning |
|---|---|
| `INSTALLATION_NOT_EVALUATED` | No installation-eligibility evaluation exists. |
| `INSTALLATION_ELIGIBILITY_PENDING` | Evaluation awaits work or prerequisites. |
| `INSTALLATION_ELIGIBLE` | The identified installer request may be considered under the declared scope. |
| `INSTALLATION_ELIGIBLE_WITH_RESTRICTIONS` | Only a narrowed installer request is eligible. |
| `INSTALLATION_INELIGIBLE` | The request fails an applicable eligibility requirement. |
| `INSTALLATION_AUTHORITY_PENDING` | Required operation authority is unresolved. |
| `INSTALLATION_TARGET_AMBIGUOUS` | The production target cannot be established safely. |
| `INSTALLATION_ARTIFACT_INCOMPLETE` | Required accepted generation members are incomplete. |
| `INSTALLATION_INSTALLER_UNAVAILABLE` | The installer or required provider is inaccessible. |
| `INSTALLATION_ROLLBACK_UNAVAILABLE` | Required rollback capability is unavailable. |
| `INSTALLATION_REVALIDATION_REQUIRED` | Material evidence requires current evaluation. |
| `INSTALLATION_CONFLICTING` | Eligible target, authority, artifact, or generation evidence conflicts. |
| `INSTALLATION_RESOURCE_EXHAUSTED` | Bounded evaluation could not complete. |
| `INSTALLATION_UNSUPPORTED` | The required installer or target contract is unsupported. |
| `INSTALLATION_INDETERMINATE` | Eligibility cannot be established more precisely. |

Eligibility does not mean installer acceptance, mutation, activation, or recovery.

## 27. Installer handoff

### 27.1 Installer ownership

The installer owns durable production mutation, including applicable:

- partition-table changes;
- filesystem creation;
- formatting;
- operating-system deployment;
- package installation;
- durable configuration writes;
- boot-target configuration;
- activation staging;
- rollback mechanics;
- installer-owned cleanup.

ASM and BOOT MUST NOT perform or claim installer mutation merely because they have technical storage access.

### 27.2 Candidate image boundary

ASM may produce a candidate disk-image file in ASM output staging.

Writing that image to a production target remains installer-owned.

An implementation component capable of both build and installation MUST expose:

- separate provider contracts;
- separate authority;
- separate operation and attempt identities;
- separate results.

### 27.3 Handoff record

A conceptual handoff contains:

```text
BootInstallerHandoff
    handoff_identity
    handoff_revision
    boot_session_identity
    plan_reference
    assembly_generation_reference
    selected_component_set_reference
    target_reference
    accepted_generation_member_references
    artifact_verification_references
    provenance_and_licensing_references
    pair_evaluation_reference
    installation_eligibility_reference
    authority_envelope_reference
    mutation_limits
    expected_result_contract
    rollback_requirements
    cancellation_behavior
    interruption_behavior
    idempotency_reference
    audit_requirements
```

### 27.4 Installer results

BOOT SHOULD preserve installer results equivalent to:

- request rejected;
- request accepted;
- no mutation attempted;
- mutation pending;
- mutation active;
- mutation attempted with no change confirmed;
- partial mutation;
- complete mutation reported;
- mutation unknown;
- write verification pending;
- write verification complete;
- rollback available;
- rollback attempted;
- rollback complete;
- rollback failed;
- rollback unknown;
- activation staged;
- resource exhausted;
- unsupported;
- indeterminate.

An installer result remains installer-attributed.

## 28. Unknown mutation and reconciliation

### 28.1 Unknown mutation

Unknown mutation may arise from:

- interruption;
- power loss;
- installer failure;
- provider restart;
- transport loss;
- result-retention failure;
- cleanup failure;
- conflicting observations.

### 28.2 Reconciliation requirement

A conceptual record contains:

```text
BootReconciliationRequirement
    requirement_identity
    requirement_revision
    semantic_operation_identity
    boot_attempt_identity
    provider_operation_reference
    provider_attempt_reference
    affected_target_reference
    affected_generation_reference
    last_known_state
    possible_effects
    required_provider
    required_observations
    prohibited_retries
    permitted_interim_scope
    resolution_conditions
    bounded_issues
```

### 28.3 Retry prohibition

BOOT MUST NOT blindly repeat a non-idempotent provider or installer operation when prior completion or mutation is unresolved.

### 28.4 Restart does not erase uncertainty

Restarting BOOT, ASM, an installer, or the substrate MUST NOT erase:

- partial output;
- quarantine;
- failed cleanup;
- unknown prior completion;
- unknown mutation;
- stale operation references;
- pending reconciliation.

## 29. Activation coordination

### 29.1 BOOT ownership

BOOT owns:

- activation-eligibility evaluation;
- activation target selection;
- activation authority coordination;
- activation semantic operation identity;
- provider-result correlation;
- fallback and rollback coordination.

BOOT does not necessarily own the physical mechanism that updates boot configuration, transfers control, or launches code.

### 29.2 Activation results

Activation results SHOULD distinguish:

- not evaluated;
- ineligible;
- eligible;
- eligible with restrictions;
- prepared;
- attempted;
- control transfer observed;
- active state observed;
- returned unexpectedly;
- failed;
- interrupted;
- resource exhausted;
- unsupported;
- indeterminate.

### 29.3 No automatic execution

A candidate output or generation member MUST NOT be executed as ordinary Node software merely because it:

- compiled;
- linked;
- passed an ASM test;
- has a digest;
- has a valid signature;
- is locally produced;
- resembles an existing component;
- is newer;
- is cached.

Bounded ASM testing remains separate from ordinary activation.

## 30. Rollback coordination

Rollback MUST identify:

- source generation;
- target generation;
- target substrate;
- authority;
- installer or platform mechanism;
- required artifacts;
- expected durable effects;
- recovery criteria;
- fallback if rollback fails;
- unknown-mutation reconciliation.

Rollback capability does not authorize rollback.

Rollback completion does not automatically establish recovery.

## 31. Recovery verification

### 31.1 Recovery criteria

Recovery criteria MUST be:

- scoped;
- revisioned;
- bounded;
- testable;
- provider-attributed;
- linked to the plan and generation;
- explicit about required evidence and limitations.

### 31.2 Recovery results

BOOT SHOULD support results equivalent to:

- not evaluated;
- verification pending;
- satisfied;
- satisfied with limitations;
- unsatisfied;
- partial;
- unavailable;
- conflicting;
- resource exhausted;
- unsupported;
- indeterminate.

### 31.3 Provider evidence

Runtime, installer, ASM, IMM, security, platform, and other providers may contribute evidence.

No provider’s evidence automatically replaces BOOT’s recovery evaluation unless the owning contract explicitly assigns final verification.

## 32. Runtime handoff and readiness

### 32.1 Handoff eligibility

BOOT may determine that an identified generation is eligible for runtime handoff when applicable:

- recovery criteria are satisfied;
- authority permits handoff;
- target and generation state are established;
- required limitations are representable;
- continuation data is prepared;
- unresolved mutation and reconciliation requirements permit progression.

### 32.2 Runtime ownership

The runtime owns:

- initialization;
- service startup;
- runtime admission;
- production scheduling;
- runtime health;
- runtime readiness;
- ordinary runtime lifecycle.

BOOT MUST NOT report runtime ready merely because:

- control transferred;
- a kernel loaded;
- micro-OS or userspace started;
- a process is alive;
- a runtime endpoint is reachable;
- a prior generation was ready.

## 33. BOOT state-model integration

### 33.1 Assembly dimension refinement

BOOT-0005 refines the conditional assembly progression as:

```text
ASSEMBLY_NOT_REQUIRED
    → HARDWARE_EVIDENCE_PENDING
    → ASSEMBLY_PLAN_PENDING
    → ASSEMBLY_PLAN_EVALUATING
    → COMPONENT_SELECTION_ACTIVE
    → PROVIDER_OPERATION_PENDING
    → PROVIDER_OPERATION_ACTIVE
    → CANDIDATE_OUTPUT_EVALUATION_PENDING
    → GENERATION_MEMBERSHIP_EVALUATING
    → GENERATION_STAGED
    → GENERATION_VALIDATION_PENDING
    → GENERATION_ELIGIBILITY_EVALUATED
    → ASSEMBLY_DIMENSION_CLOSED
```

`PROVIDER_OPERATION_ACTIVE` replaces wording implying that BOOT performs build or selection mechanisms.

### 33.2 Other dimensions remain separate

Assembly progression MUST NOT directly overwrite:

- authority state;
- artifact state;
- installer state;
- recovery state;
- handoff state;
- runtime-observation state.

### 33.3 No implicit cascade

The following transitions are prohibited:

```text
provider operation complete
    != candidate output accepted

candidate output accepted for evaluation
    != artifact accepted for scope

artifact accepted for scope
    != generation member accepted

generation member accepted
    != generation installation eligible

installation eligible
    != installer accepted

installer complete
    != activation accepted

activation observed
    != recovery satisfied

recovery satisfied
    != runtime ready
```

## 34. Failure and uncertainty

### 34.1 Required distinctions

BOOT-0005 preserves distinct states for:

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
- partial;
- interrupted;
- resource exhausted;
- indeterminate.

### 34.2 Provider failure does not prove semantic failure

An ASM compiler, linker, filesystem, workspace, process, test, cleanup, or resource failure does not independently prove:

- source invalidity;
- release invalidity;
- target damage;
- compromise;
- assembly-generation failure;
- installer failure;
- unrecoverability.

### 34.3 No false success

BOOT MUST NOT report a favorable plan, acceptance, membership, generation, eligibility, activation, recovery, or handoff result when required result construction or bounded evidence retention failed.

## 35. Boundedness and resource exhaustion

Implementations MUST define finite limits for:

- plans per BOOT session;
- plan operations;
- operation-graph depth;
- selected components;
- substitutions;
- KRN references;
- provider operations;
- provider attempts;
- candidate outputs;
- candidate references;
- accepted artifacts;
- accepted generation members;
- verification results;
- provenance depth;
- licensing records;
- generation revisions;
- retained generations;
- fallback generations;
- retries;
- reconciliation attempts;
- issue records;
- diagnostic growth;
- memory;
- workspace;
- storage;
- network use;
- processing time.

Resource exhaustion MUST:

- remain explicit;
- avoid favorable defaults;
- leave authoritative state unchanged when required preparation failed;
- preserve prior revisions;
- avoid silent truncation reported as complete success;
- preserve bounded capacity for cancellation, failure reporting, quarantine, cleanup, reconciliation, and handoff refusal.

## 36. Conservative atomicity

Before committing authoritative state for any of the following:

- plan validation;
- selected component set;
- pair evaluation;
- candidate-reference admission;
- artifact acceptance;
- generation membership;
- generation creation or revision;
- installation eligibility;
- installer handoff;
- activation eligibility;
- recovery result;
- handoff eligibility,

BOOT MUST complete all applicable:

- identity validation;
- exact revision validation;
- authority validation;
- provider-reference construction;
- evidence-reference construction;
- freshness and lifecycle evaluation;
- provenance and licensing evaluation;
- bounded issue retention;
- result construction;
- idempotency preparation;
- conflict detection.

If required preparation fails:

- authoritative state remains unchanged;
- no revision advances;
- no partial favorable result is committed;
- no favorable result is reported;
- uncertainty remains explicit.

## 37. Revision, idempotency, and retry

### 37.1 Exact revisions

BOOT selections, pair evaluations, artifact acceptances, generation memberships, and generations MUST reference exact immutable KRN revisions where applicable.

### 37.2 Stale updates

A stale plan, provider result, candidate reference, artifact result, or generation transition MUST NOT overwrite a newer authoritative revision.

### 37.3 Duplicate delivery

Duplicate delivery MUST NOT:

- create a new semantic operation;
- create independent evidence;
- reset retry budgets;
- create additional authority;
- accept the same candidate twice;
- create duplicate generation membership;
- repeat non-idempotent mutation.

### 37.4 Retry identities

A retry MUST preserve or explicitly replace:

- BOOT semantic operation identity;
- BOOT attempt identity;
- provider-operation identity;
- provider-attempt identity;
- input revisions;
- authority;
- expected outputs;
- idempotency state.

## 38. Adjacent-system boundaries

### 38.1 KRN

KRN owns reusable kernel-family declarations and their lifecycle.

BOOT consumes opaque revisioned references and evaluates selected local instances.

### 38.2 ASM

ASM owns the assembly-kernel role, assembly environment, mechanism capabilities, observations, provider operations, resource outcomes, candidate outputs, output staging, cleanup, and ASM-scoped failure.

BOOT does not duplicate ASM mechanisms or results.

### 38.3 Installer

The installer owns durable production mutation, activation staging, rollback mechanics, and installer completion.

### 38.4 Security providers

Security providers own cryptographic identity, key custody, signature and digest verification, secure sessions, credentials, and replay-protection primitives.

### 38.5 Artifact and release providers

Artifact and release providers own the declared identity, manifest, release, custody, and verification results supplied through their contracts.

### 38.6 ACS and transport

ACS owns participant, endpoint, relationship, connection, attachment, admission, capability, and lifecycle semantics.

Transport owns byte movement.

### 38.7 MEM

MEM owns semantic persistence, custody, retention, reconstruction, deletion, and memory-recovery truth.

### 38.8 IMM

IMM may provide evidence, requests, findings, recommendations, and IMM-owned recovery verification.

IMM does not authorize itself or decide BOOT plan acceptance.

### 38.9 Resource management

Resource management owns allocation, reservation, accounting, ceilings, pressure policy, and reclamation.

### 38.10 Runtime

The runtime owns readiness and ordinary runtime lifecycle.

### 38.11 Aggregate system profile

Ownership of aggregate `SystemProfileIdentity` remains unresolved.

BOOT MUST NOT equate it with:

- a selected component-set instance;
- an assembly generation;
- an installed generation;
- a deployed kernel instance;
- an installer result;
- a runtime instance.

## 39. Public conceptual contracts

These contracts define semantic fields, not required memory layouts, wire formats, ABIs, or private policy.

### 39.1 `BootRecoveryPlanProposal`

```text
BootRecoveryPlanProposal
    proposal_identity
    proposal_revision
    proposer_identity
    boot_session_identity
    recovery_objective
    target_reference
    operation_graph_reference
    opaque_krn_references
    opaque_provider_requirements
    artifact_requirements
    authority_requirements
    verification_requirements
    rollback_requirements
    limitations
    bounded_issues
```

### 39.2 `BootRecoveryPlan`

```text
BootRecoveryPlan
    plan_identity
    plan_revision
    accepted_proposal_reference
    boot_session_identity
    recovery_objective
    target_reference
    operation_graph
    selected_component_set_reference
    authority_requirements
    artifact_requirements
    provider_operation_requirements
    installer_requirements
    activation_requirements
    rollback_requirements
    recovery_criteria
    handoff_conditions
    bounded_issues
```

### 39.3 `BootSelectedComponentSet`

```text
BootSelectedComponentSet
    component_set_identity
    component_set_revision
    kernel_profile_revision_reference
    micro_os_profile_revision_reference
    compatibility_declaration_revision_reference
    required_component_declaration_revision_reference
    selected_required_component_references
    selected_conditional_component_references
    selected_optional_component_references
    substitution_references
    omitted_optional_component_references
    unresolved_component_references
    target_reference
    generation_reference
    evaluation_state
    limitations
```

### 39.4 `BootPairEvaluation`

```text
BootPairEvaluation
    evaluation_identity
    evaluation_revision
    kernel_profile_revision_reference
    micro_os_profile_revision_reference
    compatibility_declaration_revision_reference
    required_component_declaration_revision_reference
    selected_component_set_reference
    generation_reference
    local_evidence_references
    artifact_acceptance_references
    provider_result_references
    lifecycle_reference
    compatibility_result
    required_component_result
    optional_component_result
    revalidation_requirement
    limitations
    bounded_issues
```

### 39.5 `BootProviderOperationRequest`

```text
BootProviderOperationRequest
    boot_operation_identity
    boot_attempt_identity
    plan_reference
    generation_reference
    provider_class
    provider_target
    operation_class
    input_references
    permitted_output_classes
    authority_reference
    resource_limits
    mutation_prohibition
    execution_prohibition
    propagation_prohibition
    result_contract
    idempotency_reference
```

### 39.6 `BootCandidateReferenceAdmission`

```text
BootCandidateReferenceAdmission
    admission_identity
    admission_revision
    candidate_output_reference
    provider_operation_reference
    provider_attempt_reference
    boot_operation_reference
    output_class
    classification
    quarantine_state
    cleanup_state
    retention_limit
    admission_result
    limitations
    bounded_issues
```

### 39.7 `BootArtifactAcceptance`

```text
BootArtifactAcceptance
    acceptance_identity
    acceptance_revision
    artifact_or_candidate_reference
    acceptance_scope
    source_identity_reference
    release_identity_reference
    krn_identity_references
    verification_references
    provenance_reference
    licensing_reference
    lifecycle_reference
    authority_reference
    acceptance_result
    limitations
    bounded_issues
```

### 39.8 `BootGenerationMember`

```text
BootGenerationMember
    membership_identity
    membership_revision
    assembly_generation_reference
    selected_component_set_reference
    artifact_acceptance_reference
    component_role
    required_or_optional_state
    krn_profile_build_artifact_references
    membership_result
    limitations
    bounded_issues
```

### 39.9 `BootAssemblyGeneration`

```text
BootAssemblyGeneration
    generation_identity
    generation_revision
    boot_session_identity
    plan_reference
    target_reference
    exact_krn_revision_references
    selected_component_set_reference
    accepted_generation_member_references
    candidate_reference_set
    rejected_candidate_references
    provider_operation_references
    provider_result_references
    pair_evaluation_reference
    provenance_evaluation_reference
    licensing_evaluation_reference
    lifecycle_evaluation_reference
    authority_references
    installation_eligibility
    activation_eligibility
    rollback_reference
    recovery_criteria_reference
    revalidation_state
    limitations
    bounded_issues
```

### 39.10 `BootInstallationEligibilityResult`

```text
BootInstallationEligibilityResult
    evaluation_identity
    evaluation_revision
    plan_reference
    generation_reference
    target_reference
    evaluated_revision_vector
    authority_result_reference
    artifact_acceptance_references
    generation_membership_references
    pair_evaluation_reference
    installer_capability_reference
    rollback_state
    reconciliation_state
    eligibility_result
    restrictions
    bounded_issues
```

### 39.11 `BootReconciliationRequirement`

```text
BootReconciliationRequirement
    requirement_identity
    requirement_revision
    semantic_operation_identity
    boot_attempt_identity
    provider_operation_reference
    provider_attempt_reference
    affected_target_reference
    affected_generation_reference
    last_known_state
    possible_effects
    required_provider
    required_observations
    prohibited_retries
    permitted_interim_scope
    resolution_conditions
    bounded_issues
```

No public conceptual record contains raw private keys, reusable recovery secrets, production credentials, or private authority assignments.

## 40. Conformance expectations

### 40.1 Architectural conformance

An architecture conforms when it:

- consumes KRN declarations through opaque exact revision references;
- preserves reusable declarations separately from selected BOOT instances;
- preserves BOOT semantic and attempt identities separately from provider-operation and provider-attempt identities;
- uses provider-attributed active-operation state;
- preserves ASM candidate outputs separately from KRN artifact identity, BOOT artifact acceptance, and generation membership;
- keeps ASM output staging separate from `GENERATION_STAGED`;
- preserves provenance, licensing, lifecycle, and revalidation;
- preserves partial output, quarantine, cleanup, and indeterminate effects;
- prevents BOOT and ASM from absorbing installer mutation;
- prevents BOOT from claiming runtime readiness;
- leaves aggregate `SystemProfileIdentity` unresolved.

### 40.2 Required positive tests

Conformance evidence SHOULD demonstrate:

1. consume exact KRN profile revision references;
2. consume a KRN compatibility declaration without modifying it;
3. create a BOOT selected component-set instance;
4. evaluate one selected pair;
5. authorize one Node-targeted provider operation;
6. execute one explicitly authority-not-required disposable diagnostic operation;
7. preserve all four operation and attempt identities;
8. receive an ASM candidate output;
9. admit the candidate reference for evaluation;
10. accept the artifact for one bounded scope;
11. accept generation membership separately;
12. establish `GENERATION_STAGED` without confusing it with ASM output staging;
13. evaluate installation eligibility;
14. prepare an installer handoff;
15. consume an installer result;
16. reconcile one partial or unknown result;
17. preserve activation, recovery, handoff eligibility, and runtime readiness separately.

### 40.3 Required negative tests

Conformance evidence SHOULD demonstrate that:

- KRN declaration does not become BOOT selection;
- required-component declaration does not prove selected presence;
- environment capability does not create authority;
- provider operation active does not create BOOT completion;
- compiler success does not accept candidate output;
- ASM candidate output does not become KRN artifact identity;
- KRN artifact identity does not create BOOT acceptance;
- BOOT acceptance for evaluation does not create generation membership;
- generation membership does not create installation eligibility;
- ASM output staging does not create `GENERATION_STAGED`;
- successful test does not satisfy BOOT validation automatically;
- valid digest or signature does not create release or installation authority;
- partial output cannot be silently promoted;
- quarantined output cannot be installed;
- cleanup failure cannot be hidden;
- missing provenance prevents stronger claims where required;
- successful build does not create licensing permission;
- stale KRN revisions require revalidation;
- `AUTHORITY_NOT_REQUIRED` cannot be used for production material;
- ASM and BOOT do not mutate production targets;
- installer acceptance does not prove mutation;
- mutation does not prove activation;
- activation does not prove recovery;
- recovery does not prove runtime readiness;
- aggregate `SystemProfileIdentity` is not invented.

### 40.4 Mock-provider conformance

Mocks MUST support truthful:

- available;
- unavailable;
- unsupported;
- accepted;
- active;
- complete;
- complete with limitations;
- no output;
- partial;
- quarantined;
- cleanup pending;
- cleanup failed;
- failed;
- interrupted;
- resource exhausted;
- stale;
- conflicting;
- revoked;
- indeterminate

results.

Mocks MUST NOT provide favorable results unavailable to the real provider contract.

## 41. Initial implementation implications

This specification does not authorize implementation.

When implementation begins, BOOT-facing structures should remain suitable for:

- fixed-width categories;
- bounded identifiers;
- explicit revisions;
- caller-owned buffers;
- explicit counts;
- bounded arrays;
- structured outcomes;
- checked arithmetic;
- no allocator ownership transfer;
- no exceptions crossing a C-compatible boundary;
- no unbounded diagnostic growth;
- machine-readable headless operation.

Concrete layouts belong to later implementation work and BOOT-0006.

## 42. Relationship to later BOOT specifications

### BOOT-0006 — Minimal Runtime and Language Constraints

BOOT-0006 will define measurable limits and implementation constraints for:

- plans;
- operations;
- references;
- candidates;
- artifacts;
- generations;
- identifiers;
- buffers;
- parsing;
- memory;
- stack and heap use;
- retry and time budgets.

### BOOT-0007 — ACS, IMM, and MEM Integration

BOOT-0007 will refine:

- ACS references and admission;
- IMM evidence and requests;
- MEM persistence and custody;
- cross-series result mapping.

### BOOT-0008 — Failure, Retry, and Operator Intervention

BOOT-0008 will refine:

- retry classes;
- operator review;
- cancellation;
- interruption;
- reconciliation;
- unknown mutation;
- lock-down;
- rollback intervention.

### BOOT-0009 — Public/Private Boundary and Implementation Roadmap

BOOT-0009 will refine publication gates, implementation checkpoints, and private-policy boundaries.

### Future installer, OS, source, release, update, and runtime architecture

Future architecture will define concrete:

- installer mutation contracts;
- system-generation deployment;
- source and release governance;
- update channels;
- activation mechanisms;
- aggregate system profile;
- runtime readiness.

Those specifications MUST preserve the BOOT-0005 distinctions.

## 43. Open architectural decisions

The following remain intentionally unresolved:

- exact KRN compatibility-declaration schema;
- exact KRN required-component schema;
- exact KRN lifecycle schema;
- exact ASM provider-operation and candidate-output schemas;
- exact artifact identity and manifest formats;
- exact release identity and authority contracts;
- exact plan graph encoding;
- exact generation-identity width and continuity rules;
- exact artifact-acceptance scope vocabulary;
- exact installer interface;
- exact activation and rollback providers;
- exact reconciliation provider contracts;
- exact public ABI;
- concrete bounds;
- production authority policy;
- production signing and licensing policy;
- aggregate `SystemProfileIdentity` ownership;
- complete installed-system generation architecture;
- complete source, release, and update architecture.

Unresolved architecture MUST remain unresolved until the owning series deliberately decides it.

## 44. Cross-series boundary ledger

### Entry BOOT-0005-LEDGER-001

**Decision:** ASM candidate-output status, KRN artifact identity, BOOT scoped artifact acceptance, and BOOT generation membership are separate layers.

**Owning series:** ASM, KRN, and BOOT respectively.

**Other series affected:** KRN-0003 through KRN-0005; ASM-0004 through ASM-0007.

**Question or required confirmation:** Exact cross-reference fields remain dependent on later KRN and ASM schemas.

**Blocking or non-blocking:** Non-blocking with opaque references; blocking before ABI stabilization.

### Entry BOOT-0005-LEDGER-002

**Decision:** BOOT semantic operation identity, BOOT attempt identity, provider-operation identity, and provider-attempt identity remain distinct.

**Owning series:** BOOT for semantic identity; each provider for provider identity.

**Other series affected:** ASM-0004 and ASM-0006.

**Question or required confirmation:** Exact provider-correlation schema remains provider-owned.

**Blocking or non-blocking:** Non-blocking with opaque references.

### Entry BOOT-0005-LEDGER-003

**Decision:** `PROVIDER_OPERATION_ACTIVE` replaces wording implying BOOT performs compilation, linking, packaging, testing, or selection mechanisms.

**Owning series:** BOOT.

**Other series affected:** BOOT-0002; ASM provider-operation contracts.

**Question or required confirmation:** BOOT-0002 requires a deliberate future amendment or compatibility note.

**Blocking or non-blocking:** Non-blocking for BOOT-0005; blocking before implementation relying on the old ambiguous phase.

### Entry BOOT-0005-LEDGER-004

**Decision:** ASM output staging is not BOOT `GENERATION_STAGED`.

**Owning series:** ASM for output staging; BOOT for generation staging.

**Other series affected:** ASM-0005; BOOT-0002.

**Question or required confirmation:** Exact staging-reference exchange remains undefined.

**Blocking or non-blocking:** Non-blocking with opaque references.

### Entry BOOT-0005-LEDGER-005

**Decision:** KRN declarations remain reusable facts and BOOT selections remain session- and generation-scoped instances.

**Owning series:** KRN and BOOT respectively.

**Other series affected:** KRN-0003 and KRN-0005.

**Question or required confirmation:** Exact compatibility and required-component records remain KRN-owned.

**Blocking or non-blocking:** Non-blocking.

### Entry BOOT-0005-LEDGER-006

**Decision:** Node-targeted provider work requires applicable BOOT authority; authority-not-required work must be explicitly declared and disposable.

**Owning series:** BOOT for authority; provider series for mechanism contract.

**Other series affected:** KRN-0004; ASM-0004.

**Question or required confirmation:** Exact public authority-not-required operation-class registry remains undefined.

**Blocking or non-blocking:** Non-blocking for architecture; blocking before concrete provider interfaces.

### Entry BOOT-0005-LEDGER-007

**Decision:** ASM and BOOT do not mutate production installation targets.

**Owning series:** Installer for mutation; BOOT for handoff and evaluation.

**Other series affected:** ASM-0005; future installer architecture.

**Question or required confirmation:** Exact boundary among ASM output staging, installer input staging, and installer-owned production staging remains undefined.

**Blocking or non-blocking:** Non-blocking for architecture; blocking before storage interfaces.

### Entry BOOT-0005-LEDGER-008

**Decision:** Runtime readiness remains runtime-owned and cannot be inferred from handoff, control transfer, process liveness, or recovery.

**Owning series:** Runtime.

**Other series affected:** Future runtime and handoff architecture.

**Question or required confirmation:** Exact runtime-readiness result contract remains undefined.

**Blocking or non-blocking:** Non-blocking for BOOT-0005.

### Entry BOOT-0005-LEDGER-009

**Decision:** Aggregate `SystemProfileIdentity` remains unresolved and is not equivalent to a selected component set, assembly generation, installed generation, deployed kernel instance, installer result, or runtime instance.

**Owning series:** Deferred.

**Other series affected:** KRN, ASM, installer, runtime, future OS and update architecture.

**Question or required confirmation:** A future architecture must assign ownership deliberately.

**Blocking or non-blocking:** Non-blocking for BOOT-0005.

## 45. Prohibited interpretations

This specification MUST NOT be interpreted to mean that:

- BOOT owns the kernel family;
- BOOT defines an assembly kernel or assembly environment;
- BOOT performs compilation merely because it requests compilation;
- KRN declarations become BOOT selections;
- a required-component declaration proves component presence;
- environment capability creates authority;
- hardware observation creates target eligibility;
- process identity replaces semantic or provider-operation identity;
- ASM candidate output equals KRN artifact identity;
- KRN artifact identity equals BOOT acceptance;
- BOOT acceptance for one scope equals universal acceptance;
- scoped artifact acceptance equals generation membership;
- generation membership equals installation eligibility;
- ASM output staging equals `GENERATION_STAGED`;
- provider operation completion equals output acceptance;
- signature or digest validity equals compatibility or installation authority;
- public source availability equals licensing permission;
- local root access creates build or installation authority;
- authority-not-required output may silently become production material;
- newest means eligible;
- cache presence means current;
- ASM may mutate production installation targets;
- BOOT may perform installer mutation;
- installer acceptance proves mutation;
- installation proves activation;
- activation proves recovery;
- recovery proves runtime readiness;
- reboot clears uncertainty;
- partial output may be silently promoted;
- quarantine may be silently removed;
- cleanup failure may be hidden;
- stale KRN conclusions remain current;
- one series may assign itself aggregate `SystemProfileIdentity`.

## 46. Completion checklist

- [x] BOOT-0000 through BOOT-0004 authority is preserved.
- [x] KRN-0000 through KRN-0002 boundaries are preserved.
- [x] ASM-0000 through ASM-0001 boundaries are preserved.
- [x] KRN declarations are consumed through opaque exact revision references.
- [x] Reusable declarations remain distinct from BOOT-selected instances.
- [x] BOOT semantic operation and attempt identities remain separate from provider operation and attempt identities.
- [x] `PROVIDER_OPERATION_ACTIVE` is used.
- [x] Candidate-reference admission, scoped artifact acceptance, and generation membership are separate transitions.
- [x] ASM candidate output, KRN artifact identity, BOOT acceptance, and generation membership remain separate layers.
- [x] ASM output staging remains distinct from `GENERATION_STAGED`.
- [x] Provenance and licensing remain explicit.
- [x] Material changes trigger revalidation.
- [x] Partial output, quarantine, cleanup, resource exhaustion, and indeterminate effects remain explicit.
- [x] Unknown mutation requires reconciliation before unsafe retry.
- [x] Installer ownership of production mutation is preserved.
- [x] Activation remains distinct from installation.
- [x] Recovery remains distinct from activation.
- [x] Runtime readiness remains runtime-owned.
- [x] Aggregate `SystemProfileIdentity` ownership remains unresolved.
- [x] Public contracts remain language-neutral, bounded, headless-capable, and independently implementable.
- [x] No implementation language, compiler, linker, package system, filesystem layout, transport, artifact format, installer, or runtime is mandated.
- [x] No cross-series conflict was silently resolved.

## 47. Closing principle

> **BOOT accepts progression only through explicit, revisioned transitions: a provider result remains provider evidence, a candidate remains a candidate, an artifact is accepted only for a declared scope, a generation member belongs only to an identified generation, and no installation, activation, recovery, or readiness result exists until its owning boundary reports and evaluates it.**

## Revision history

### Version 0.1 — 2026-07-18

- Defined recovery-plan proposals and BOOT recovery plans.
- Defined selected component-set instances and selected-pair evaluation.
- Consumed KRN declarations through opaque exact revision references.
- Defined BOOT semantic operation, BOOT attempt, provider operation, and provider attempt separation.
- Replaced ambiguous active-build wording with `PROVIDER_OPERATION_ACTIVE`.
- Defined candidate-reference admission, scoped artifact acceptance, and generation-membership acceptance as separate transitions.
- Preserved ASM candidate-output, KRN artifact, BOOT acceptance, and generation-membership layers.
- Defined assembly-generation identity, revision, staging, validation, eligibility, and revalidation.
- Preserved ASM output staging as distinct from BOOT `GENERATION_STAGED`.
- Defined provenance, licensing, verification, lifecycle, and revalidation behavior.
- Defined partial-output, quarantine, cleanup, indeterminate-effect, and reconciliation behavior.
- Preserved installer ownership of production mutation.
- Defined activation, rollback, recovery, and runtime-handoff boundaries.
- Preserved runtime ownership of readiness.
- Preserved unresolved aggregate `SystemProfileIdentity` ownership.
