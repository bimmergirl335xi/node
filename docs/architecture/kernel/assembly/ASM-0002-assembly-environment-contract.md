# ASM-0002: Assembly Environment Contract

| Field | Value |
|---|---|
| Specification | ASM-0002 |
| Title | Assembly Environment Contract |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ASM-PUB |
| Parent series | KRN-PUB |
| Authors | Node |
| Last updated | 2026-07-18 |
| Approval | Pending review |
| Depends on | ASM-0000; ASM-0001; applicable KRN kernel-profile, micro-OS-profile, compatibility-declaration, and component-declaration architecture; applicable BOOT minimal-environment, authority, provider-operation, and handoff architecture; applicable ACS, installer, security-provider, resource-management, MEM, IMM, and platform architecture |
| Supersedes | ASM-0002 placeholder |
| Superseded by | None |
| Current confidence | High in environment scope, startup and reporting boundaries, profile-reference handling, capability semantics, headless operation, process supervision, storage separation, optional networking, time uncertainty, controlled termination, and return-of-control requirements; exact KRN schemas, BOOT invocation records, provider-operation schemas, resource-envelope schemas, and installer handoff records remain externally owned |

> **The assembly environment is a bounded, headless mechanism environment that can identify its capabilities, execute authorized provider work, report truthful results, and return control without claiming that Node assembly or installation has completed.**

## Architectural-intent notice

This specification defines the contract of the assembly environment accompanying a Node assembly-kernel profile.

It establishes:

- what it means for an assembly environment to be entered;
- what it means for the environment to be reporting-capable;
- what it means for the environment to satisfy its declared profile;
- the minimum control capabilities every assembly environment must provide;
- the assembly-role capabilities required for useful assembly work;
- the relationship between the active kernel profile and primary micro-OS profile;
- startup inputs and outputs;
- capability declaration and observation semantics;
- task or process execution and supervision;
- storage, filesystem, source, workspace, evidence, cache, and output boundaries;
- optional network and offline operation;
- security-provider access;
- time and ordering behavior;
- machine-readable result channels;
- environment lifetime and provider-operation lifetime;
- controlled quiescence, shutdown, reboot, and return of control;
- degraded, failed, unsupported, and indeterminate environment behavior.

This specification does not define:

- a kernel-profile schema;
- a micro-OS-profile schema;
- a kernel/micro-OS compatibility-declaration schema;
- BOOT operation identity;
- BOOT authority semantics;
- BOOT assembly-generation progression;
- exact hardware-observation records;
- exact compiler or linker interfaces;
- exact workspace quantities;
- exact failure-record encodings;
- installer mutation;
- activation;
- recovery completion;
- runtime readiness.

Those meanings remain with their owning specifications.

---

## 1. Purpose

The assembly environment is the minimal custom micro-OS environment that accompanies an assembly-kernel profile.

Its purpose is to provide a controlled setting in which Node assembly mechanisms can:

1. identify the active assembly-kernel and micro-OS profile context;
2. establish bounded machine-readable control and reporting;
3. inspect the local substrate through declared mechanisms;
4. access authorized source, package, cache, configuration, and tool inputs;
5. execute authorized local or remote provider operations;
6. supervise operation-local tasks;
7. retain bounded evidence;
8. produce ASM candidate outputs;
9. report capability, mechanism, resource, and failure outcomes;
10. quiesce and return control conservatively.

The environment exists to provide mechanisms.

It does not decide what those mechanisms mean within the governed Node lifecycle.

The mandatory boundary remains:

```text
assembly environment booted
    != Node assembled
    != installation completed
    != activation accepted
    != runtime ready
```

---

## 2. Normative language

The terms **MUST** and **MUST NOT** describe mandatory architectural requirements.

The terms **SHOULD** and **SHOULD NOT** describe strong recommendations. A departure requires documented justification and must not violate ASM-0000 or ASM-0001.

The term **MAY** describes permitted behavior.

A contract requirement applies to observable behavior regardless of:

- implementation language;
- init system;
- service manager;
- process layout;
- filesystem layout;
- kernel version;
- micro-OS packaging;
- use of containers;
- use of virtual machines;
- use of remote providers;
- technical co-location with BOOT, an installer, or another provider.

---

## 3. Foundational distinctions

The following distinctions are mandatory:

```text
assembly kernel entered
    != assembly environment reporting-capable
    != declared profile satisfied
    != provider operation ready
    != provider operation completed
```

They must also remain separate from BOOT and runtime conclusions:

```text
environment capability available
    != capability use authorized

provider operation completed
    != candidate output accepted by BOOT

candidate output returned
    != assembly generation established

return of control attempted
    != BOOT handoff accepted

reboot requested
    != recovery completed

runtime launched
    != runtime ready
```

ASM-0002 defines only the environment-side meanings.

---

## 4. Terminology

### 4.1 Assembly environment

The **assembly environment** is the micro-OS environment accompanying an assembly-kernel profile and supplying the userspace or equivalent facilities needed for the assembly role.

It includes the environment-visible combination of:

- kernel-provided mechanisms;
- micro-OS facilities;
- required local providers;
- result channels;
- bounded work areas;
- operation-supervision mechanisms;
- termination and return-of-control mechanisms.

The environment is not one particular process or executable.

### 4.2 Environment instance

An **environment instance** is one bounded entry into an assembly environment.

An environment instance begins when control enters the assembly environment and ends when control is returned, the machine is halted or powered off, or the instance becomes irrecoverably indeterminate.

An environment instance reference is an ASM-local correlation value.

It is not:

- Node identity;
- participant identity;
- an assembly-operation identity;
- an assembly-generation identity;
- a durable system identity;
- an aggregate `SystemProfileIdentity`.

### 4.3 Environment entry

**Environment entry** means execution reached the assembly kernel and accompanying micro-OS sufficiently to begin environment establishment.

Entry does not prove that:

- result channels work;
- profile declarations are available;
- required filesystems are accessible;
- provider operations can execute;
- the environment is operational.

### 4.4 Reporting-capable environment

A **reporting-capable environment** has established the minimum control plane required to emit bounded machine-readable status and to attempt controlled termination or return of control.

A reporting-capable environment may still be degraded.

### 4.5 Profile-operational environment

A **profile-operational environment** has established every capability declared as required by its applicable KRN kernel profile, primary micro-OS profile, and compatibility declaration.

A profile-operational environment may still be unsuitable for a particular provider operation.

### 4.6 Provider-operation-ready environment

A **provider-operation-ready environment** has established the specific mechanisms, resources, authority references, inputs, providers, and output paths required for one requested ASM provider operation.

Readiness is operation-specific.

### 4.7 Primary micro-OS profile

Each assembly environment uses one primary `MicroOsProfileIdentity`.

That identity describes the assembly environment as a coherent micro-OS realization.

Subordinate profile identities are used only for components that are independently:

- versioned;
- replaceable;
- reusable;
- distributed;
- maintained;
- validated;
- revoked;
- or selected.

Incidental packages and inseparable internal files do not require individual micro-OS identities.

### 4.8 Environment capability

An **environment capability** is a declared mechanism or service the environment can provide or expose.

Capability presence does not establish permission to use it.

### 4.9 Environment capability report

An **environment capability report** is an ASM result describing the declared and observed state of the environment’s capabilities.

It reports mechanism truth.

It does not report BOOT progression.

---

## 5. Environment lifetime

An environment instance may service:

- no provider operations;
- one provider operation;
- several sequential provider operations;
- several concurrent provider operations when explicitly supported.

Environment lifetime and provider-operation lifetime are distinct.

Entering the environment does not imply that a BOOT assembly operation already exists.

A diagnostic or inspection-only environment instance may be entered without an active Node-targeted provider operation.

Any later Node-targeted work remains subject to applicable BOOT authority.

### 5.1 Environment reuse

An environment instance may be reused for multiple provider operations only when the profile declares such reuse supported.

Reuse must not permit one operation to inherit another operation’s:

- authority;
- workspace;
- process state;
- candidate outputs;
- unresolved cleanup;
- secrets;
- provider connections;
- resource reservations;
- semantic result.

### 5.2 Standing assembly environments

A profile may permit a long-lived assembly environment.

A standing environment must still provide:

- bounded resource accounting;
- explicit operation isolation;
- capability refresh;
- stale-state detection;
- cache revalidation;
- controlled quiescence;
- explicit environment-instance reporting.

Long lifetime does not create expanded authority.

---

## 6. Startup contract

Assembly-environment startup must be headless, bounded, and machine-reportable.

### 6.1 Startup must not depend on interaction

Startup MUST NOT require:

- a graphical display;
- a desktop session;
- a keyboard;
- a mouse;
- an interactive shell;
- a local operator prompt;
- permanent network connectivity;
- the normal Node runtime.

An implementation may expose optional local interaction.

Failure to receive optional human input must not block required startup indefinitely.

### 6.2 Deterministic startup semantics

Deterministic startup means that the environment follows declared transition and result semantics for the same class of inputs and failures.

It does not require identical:

- timing;
- hardware enumeration order;
- processor scheduling;
- device-assignment order;
- network timing;
- log ordering across unrelated tasks.

Startup must produce explicit outcomes instead of relying on indefinite waiting or human interpretation.

### 6.3 Bounded waits

Every startup wait must be:

- bounded;
- externally cancellable;
- or explicitly classified as an environment-fatal platform dependency.

This includes waits for:

- storage;
- filesystems;
- provider services;
- network configuration;
- security hardware;
- device runtimes;
- clocks;
- result sinks;
- removable media.

A timeout must identify the capability or dependency that did not become available.

### 6.4 Startup must avoid undeclared mutation

Startup must not modify:

- production installation targets;
- unrelated local filesystems;
- existing Node generations;
- installer rollback material;
- external devices

unless another owning contract explicitly authorizes that mutation through its own provider role.

ASM-owned startup writes must remain within declared environment-owned storage.

---

## 7. Startup inputs

The environment must be able to consume or resolve the following classes of startup input where applicable.

### 7.1 Required profile context

A profile-operational environment requires references to:

- the active assembly-kernel profile;
- the active primary micro-OS profile;
- applicable subordinate micro-OS component profiles;
- the applicable KRN compatibility declaration;
- applicable required-component declarations;
- applicable optional- and unsupported-component declarations.

If those references are unavailable, the environment may enter a reporting-capable diagnostic state.

It must not claim profile satisfaction.

### 7.2 Invocation context

The environment may receive an opaque invocation context from:

- firmware;
- a bootloader;
- BOOT;
- an operator-controlled launcher;
- a test harness;
- another authorized caller.

The invocation context may identify:

- requested environment mode;
- expected result destination;
- resource-envelope reference;
- BOOT operation reference;
- authority references;
- source locations;
- cache locations;
- candidate-output destination;
- expected providers;
- termination preference.

ASM-0002 does not define the owning schemas of those references.

### 7.3 Result destination

A startup context should identify at least one authoritative machine-readable result destination.

Where none is supplied, the environment must establish an environment-owned bounded result store or report that authoritative result delivery is unavailable.

### 7.4 Resource context

The environment must consume or establish applicable resource limits before unbounded work can begin.

The exact resource model belongs to ASM-0005 and resource-management architecture.

### 7.5 Optional configuration

Optional startup configuration may include:

- locale;
- diagnostic verbosity;
- offline mode;
- network policy;
- source-media policy;
- cache policy;
- hardware-inspection limits;
- provider concurrency;
- shutdown preference.

Optional configuration must not weaken mandatory invariants.

---

## 8. Startup outcome model

ASM-0002 does not prescribe one binary startup result.

The environment must preserve at least the following distinctions.

### 8.1 Environment entered

Control reached the assembly environment.

No capability claim follows automatically.

### 8.2 Reporting-capable

The environment established:

- bounded machine-readable result output;
- an environment-instance reference;
- minimum execution supervision;
- minimum ordering semantics;
- a controlled stop or return path.

### 8.3 Profile operational

All profile-required environment capabilities were established.

### 8.4 Degraded

The environment can report and may perform a limited subset of work, but one or more required or requested capabilities are:

- unavailable;
- constrained;
- failed;
- unsupported;
- unresolved.

A degraded environment must identify prohibited operations.

### 8.5 Failed

The environment could not establish the minimum control plane required for useful governed operation.

Where possible, a bounded failure result must still be emitted through a fallback path.

### 8.6 Indeterminate

The environment cannot reliably determine whether a required transition or effect completed.

Indeterminate must not be converted into degraded success.

---

## 9. Minimum control capabilities

Every reporting-capable assembly environment must provide equivalent mechanisms for the following capabilities.

### 9.1 Environment correlation

The environment must establish a local environment-instance reference.

The reference must be unique enough within the applicable observation scope to correlate:

- startup results;
- capability reports;
- provider operations;
- task results;
- candidate outputs;
- termination results.

### 9.2 Profile-reference reporting

The environment must report the active profile and compatibility references it can establish.

Unknown or conflicting references must remain explicit.

### 9.3 Structured result output

The environment must support at least one bounded authoritative machine-readable result path.

### 9.4 Bounded execution

The environment must support controlled execution of environment tasks or equivalent execution units.

### 9.5 Execution supervision

The environment must be able to observe and control the execution units it starts within the limits of its profile.

### 9.6 Environment-owned writable area

The environment must provide or obtain a bounded writable area sufficient for:

- local state;
- result construction;
- required evidence;
- cancellation state;
- failure reporting.

This area may be RAM-backed.

Durable writable storage is not universally required.

### 9.7 Ordering basis

The environment must provide a mechanism for ordering its own authoritative events.

A reliable wall clock is not required.

### 9.8 Controlled termination path

The environment must support at least one declared path to:

- return control;
- halt;
- reboot;
- power off;
- or enter a bounded safe stopped condition.

The availability of a mechanism does not establish authority to invoke it.

---

## 10. Assembly-role capabilities

A profile-operational assembly environment must provide the capabilities required by its profile for the assembly role.

The profile must describe whether each capability is provided:

- locally;
- through another local provider;
- through an authorized remote provider;
- conditionally;
- or not at all.

### 10.1 Hardware-inspection access

The environment must provide access to the hardware-observation mechanisms required by the profile.

Detailed hardware inspection belongs to ASM-0003.

### 10.2 Input access

The environment must support bounded access to applicable:

- source material;
- packages;
- configuration;
- toolchain inputs;
- test inputs;
- cached material;
- provider references.

Input access does not establish input acceptance.

### 10.3 Provider-operation execution

The environment must support at least one declared path for executing assembly provider operations.

A provider may be:

- local;
- remote;
- device-hosted;
- virtualized;
- or supplied through another authorized subsystem.

A local compiler is not universally mandatory.

### 10.4 Workspace support

The environment must support bounded operation-local work areas appropriate to the provider mechanisms it exposes.

### 10.5 Evidence retention

The environment must retain enough bounded evidence to interpret its capability and operation results.

### 10.6 Candidate-output staging

Where the profile supports physical output production, the environment must provide bounded ASM output staging separate from production installation targets.

### 10.7 Result return

The environment must return environment, provider, resource, evidence, and candidate-output results through declared machine-readable paths.

---

## 11. Capability declaration model

KRN declarations describe what the profile requires or permits.

ASM observation describes what the current environment instance actually established.

Those dimensions must remain separate.

### 11.1 Declaration status

A capability may be declared:

- required;
- optional;
- unsupported;
- constrained;
- not applicable.

### 11.2 Observed status

A declared capability may be observed as:

- available;
- available with constraints;
- degraded;
- unavailable;
- unsupported;
- prohibited;
- failed;
- unknown;
- indeterminate;
- not applicable.

### 11.3 No inference across dimensions

Examples:

```text
declared required + observed unavailable
    = profile not satisfied

declared optional + observed unavailable
    = profile may remain satisfied

declared unsupported + mechanism physically present
    != capability supported by the profile

declared required + mechanism executable
    != operation authorized
```

### 11.4 Capability record semantics

An environment capability result must preserve, where applicable:

- capability reference;
- declaration status;
- observed status;
- provider;
- implementation location;
- constraints;
- dependencies;
- evidence reference;
- resource implications;
- operation restrictions;
- uncertainty;
- failure reference.

ASM-0002 defines these semantic fields without prescribing one serialization.

---

## 12. Environment capability report

The environment must emit an initial capability report after reaching reporting-capable status.

It must refresh or supersede that report when material capability state changes.

### 12.1 Required report content

The report must include or reference:

- environment-instance reference;
- environment startup outcome;
- active assembly-kernel profile reference;
- active primary micro-OS-profile reference;
- applicable subordinate profile references;
- KRN compatibility-declaration reference;
- profile-reference limitations or conflicts;
- authoritative result-channel description;
- ordering and time basis;
- minimum control-capability status;
- assembly-role capability status;
- resource-envelope reference or limitation;
- supported termination mechanisms;
- degraded or prohibited operations;
- report version;
- supersession relationship where applicable.

### 12.2 Capability report boundaries

A capability report must not claim:

- BOOT authority;
- component-set selection;
- pair acceptance;
- assembly-generation state;
- installation eligibility;
- activation;
- recovery completion;
- runtime readiness.

### 12.3 Material changes

Material changes requiring report refresh include:

- provider loss;
- filesystem loss;
- workspace exhaustion;
- network transition affecting a required capability;
- device-runtime loss;
- security-provider loss;
- result-channel degradation;
- resource-limit changes;
- cache invalidation affecting a requested operation;
- termination-path loss.

---

## 13. Task and process execution

The assembly environment must support controlled execution appropriate to its profile.

A profile may use:

- POSIX processes;
- threads;
- jobs;
- containers;
- virtual machines;
- device-runtime tasks;
- remote provider requests;
- another bounded execution abstraction.

ASM does not require one specific process model.

### 13.1 Execution-unit correlation

Every execution unit started for a provider operation must be attributable to:

- the environment instance;
- the local ASM provider-operation attempt;
- the requesting provider or subsystem;
- the applicable resource scope.

### 13.2 Supervision

The environment must be capable of applicable:

- start;
- status observation;
- output capture;
- timeout;
- cancellation;
- termination;
- completion collection;
- child or descendant tracking;
- resource-outcome reporting.

### 13.3 Process result separation

The following must remain distinct:

```text
execution unit started
    != execution unit completed

execution unit completed
    != execution unit succeeded

execution unit succeeded
    != provider operation completed

provider operation completed
    != BOOT accepted its output
```

### 13.4 Orphaned work

The environment must not intentionally leave operation-local execution units running after their operation is closed unless an explicit external provider contract assumes ownership.

Unterminated or unaccounted work must be reported.

### 13.5 Shell independence

A conforming environment must not require one interactive shell.

Profiles may provide command interpreters or script engines, but authoritative execution should remain possible through structured invocation.

---

## 14. Storage and filesystem contract

The assembly environment must separate environment-owned storage from observed or installer-owned targets.

### 14.1 Logical storage classes

The environment must distinguish applicable:

- immutable environment root;
- environment-local writable state;
- source area;
- package area;
- active workspace;
- cache;
- evidence area;
- ASM output staging;
- observed target storage;
- installer-owned target storage.

One physical device may host several classes.

Their ownership and mutation policies must remain distinct.

### 14.2 Environment root

The environment root may be:

- read-only;
- immutable;
- verified;
- compressed;
- RAM-resident;
- reconstructed during entry;
- writable within profile-defined bounds.

A writable root must not become an unbounded ambient workspace.

### 14.3 Source access

Source and package inputs should be mounted or exposed read-only where the operation permits.

Required mutation must occur in an operation-owned workspace.

### 14.4 Workspace

A workspace must be bounded and attributable.

Detailed workspace isolation, quotas, cleanup, and reserve behavior belong to ASM-0005.

### 14.5 Cache

Cache availability is optional unless the profile declares it required.

A cache remains non-authoritative and must be revalidated before authoritative use.

### 14.6 Evidence area

Evidence storage must remain available within declared bounds long enough to report or hand off required results.

### 14.7 ASM output staging

Candidate outputs must be staged in ASM-owned storage separate from active intermediate work where the profile permits.

ASM output staging is not BOOT generation staging.

### 14.8 Production installation targets

Production targets must default to observation-only within the ASM role.

ASM must not perform installer-owned durable mutation.

### 14.9 Filesystem outcomes

Mount, unmount, synchronization, write, and cleanup operations must return explicit mechanism results.

A mount request is not a mounted filesystem.

A sync request is not proof of durable persistence.

An unmount request is not proof that all references were released.

---

## 15. Source, package, and offline access

The environment must not assume that authorized inputs are network-accessible.

A profile may support input through:

- immutable boot media;
- removable media;
- local storage;
- preloaded package sets;
- local caches;
- direct-attached source volumes;
- remote repositories;
- ACS-mediated providers;
- another authorized transport.

### 15.1 Offline operation

Where the profile permits offline assembly, the environment must be capable of operating without:

- DNS;
- Internet access;
- external package repositories;
- remote time synchronization;
- remote identity services not declared as required.

### 15.2 Input unavailability

Missing network or external repositories must not be reported as generic environment failure when offline operation is supported.

The affected capability or provider operation must be reported precisely.

### 15.3 No ambient package acquisition

The environment must not silently fetch newer or substitute packages merely because network access is available.

Source and package selection remains governed by the applicable owning contracts.

---

## 16. Network contract

Networking is profile-declared.

An assembly environment may declare networking:

- required;
- optional;
- constrained;
- unsupported;
- unavailable.

### 16.1 No universal network dependency

Startup must not block indefinitely waiting for network service unless the profile explicitly declares networking required for environment establishment.

### 16.2 Network capability report

Where networking is present, the environment must distinguish:

- interface observed;
- interface initialized;
- link available;
- local address available;
- route available;
- transport available;
- ACS relationship available;
- requested provider reachable.

Those conditions are not equivalent.

### 16.3 ACS and transport boundary

ASM does not own:

- participant identity;
- endpoint identity;
- relationships;
- connection identity;
- attachment;
- admission;
- routing semantics;
- retransmission;
- secure-session semantics.

ASM may consume those mechanisms.

### 16.4 Network loss

Network loss must affect only capabilities and operations that actually depend on the lost path.

A local operation must not be invalidated solely because an unrelated network capability disappeared.

---

## 17. Security-provider access

An assembly environment may require access to security-provider mechanisms for:

- digest calculation;
- signature verification;
- credential use;
- authentication;
- protected key operations;
- secure sessions;
- platform evidence.

### 17.1 Provider separation

The environment may invoke or host a security provider.

ASM does not own the provider’s cryptographic semantics.

### 17.2 Secret handling

The environment must not expose reusable secrets through:

- logs;
- capability reports;
- process arguments where avoidable;
- candidate-output metadata;
- diagnostic bundles;
- unprotected temporary files.

### 17.3 Verification-result limits

A successful provider verification does not independently establish:

- source acceptance;
- release authority;
- build authority;
- pair eligibility;
- installation eligibility.

### 17.4 Security-provider unavailability

If a requested operation requires a security-provider result and the provider is unavailable, ASM must stop, narrow, or return an unresolved result.

It must not replace verification with a favorable default.

---

## 18. Time and ordering contract

The environment must provide truthful ordering without assuming a reliable wall clock.

### 18.1 Event ordering

Every authoritative result stream must support ordering through at least one of:

- a monotonic clock;
- a monotonic sequence;
- a provider-local sequence;
- an explicitly ordered append log.

### 18.2 Wall-clock time

Wall-clock time is optional unless the profile declares it required.

When wall-clock time is available, the environment must report applicable:

- source;
- synchronization status;
- estimated uncertainty;
- adjustment or reset events;
- trust limitations.

### 18.3 No fabricated time

The environment must not invent a plausible current date or time when the wall clock is unavailable or untrusted.

### 18.4 Reset boundaries

Sequence or monotonic-clock reset boundaries must be detectable through:

- environment-instance references;
- epoch references;
- restart records;
- equivalent correlation mechanisms.

### 18.5 Time does not create authority

Timestamp recency does not establish:

- freshness;
- source acceptance;
- release authority;
- cache validity;
- operation authority.

---

## 19. Machine-readable result contract

All authoritative ASM results must be representable in bounded machine-readable form.

### 19.1 Result classes

The environment must be capable of returning applicable:

- startup outcomes;
- environment capability reports;
- hardware observations;
- provider-operation status;
- execution-unit results;
- mechanism results;
- resource outcomes;
- candidate-output references;
- verification-provider results;
- failure results;
- cleanup and quarantine state;
- termination and return-of-control results.

### 19.2 Minimum result context

An authoritative result must preserve enough context to identify:

- environment instance;
- result type and version;
- provider;
- local attempt;
- external operation reference where applicable;
- subject;
- scope;
- status;
- limitations;
- ordering information;
- related evidence;
- superseded result where applicable.

### 19.3 Bounded payloads

Result channels must impose finite bounds on:

- record size;
- diagnostic size;
- attachment size;
- queue depth;
- retention;
- retry.

Oversized data should be referenced through bounded evidence storage rather than inserted without limit.

### 19.4 Authoritative result precedence

Where several output mechanisms exist, the environment must declare which mechanism or record set is authoritative.

Console text and debug logs must not silently override structured results.

### 19.5 Result-channel failure

Failure to deliver an authoritative favorable result must not be reported as successful completion.

The environment must use a declared fallback path or report the outcome as unresolved or failed.

### 19.6 Confidentiality

Result channels must preserve applicable classification and disclosure restrictions.

Machine readability does not imply public visibility.

---

## 20. Logs and diagnostics

Logs support diagnosis.

They are not the canonical semantic result.

### 20.1 Log behavior

Logs must be:

- bounded;
- attributable;
- severity-classified where applicable;
- safe for their declared audience;
- separable from authoritative results.

### 20.2 Human-readable output

Human-readable terminal output may be provided.

The environment must remain operable when no human can see it.

### 20.3 Redaction

Logs and diagnostic bundles must not expose:

- private keys;
- reusable credentials;
- protected source contents without authorization;
- unrestricted production topology;
- hidden policy;
- private module identities where restricted.

### 20.4 Diagnostic loss

Loss or truncation of optional diagnostics must not erase an otherwise preserved authoritative result.

Loss of required evidence must be reported.

---

## 21. Resource contract

The environment must operate under finite resources.

ASM-0005 defines the detailed resource and workspace model.

ASM-0002 establishes the following requirements.

### 21.1 Resource limits before work

The environment must know, inherit, or establish applicable resource bounds before beginning provider work that could grow materially.

### 21.2 Environment reserve

The environment must preserve capacity for:

- cancellation;
- failure-result construction;
- evidence finalization;
- task termination;
- candidate-output quarantine;
- cleanup reporting;
- controlled return.

### 21.3 Resource status

The capability report must identify material environment-wide resource constraints.

Operation-specific resource outcomes remain associated with the applicable provider operation.

### 21.4 No authority inference

Available resources do not grant permission to consume them.

---

## 22. Provider-operation admission

An environment may be profile-operational while not ready for a particular provider operation.

Before accepting an operation, the environment must determine whether the requested operation has the required:

- capability;
- provider;
- authority reference;
- inputs;
- workspace;
- resource envelope;
- result path;
- evidence capacity;
- termination path.

### 22.1 Admission outcomes

The environment must distinguish applicable outcomes equivalent to:

- accepted for local execution;
- accepted for remote-provider execution;
- deferred;
- rejected as unsupported;
- rejected as unavailable;
- rejected for insufficient resources;
- rejected for missing authority;
- rejected for missing input;
- rejected for missing result path;
- indeterminate.

### 22.2 Admission is not completion

Operation admission does not establish:

- task launch;
- provider completion;
- candidate-output production;
- BOOT acceptance.

### 22.3 Concurrency

If concurrent provider operations are supported, the environment must declare and enforce:

- concurrency limits;
- isolation boundaries;
- scheduling ownership;
- resource separation;
- result correlation;
- cancellation behavior.

---

## 23. Degraded operation

A degraded environment may perform only operations whose requirements remain satisfied.

### 23.1 Required degraded behavior

The environment must report:

- unavailable capabilities;
- constrained capabilities;
- affected operation classes;
- prohibited operation classes;
- available return or shutdown paths;
- whether capability restoration may be attempted.

### 23.2 No hidden fallback

A degraded environment must not silently replace:

- one compiler with another;
- one source with another;
- one provider with another;
- persistent storage with volatile storage;
- verified input with unverified input;
- local execution with remote execution

unless the applicable profile and operation contract explicitly permit that substitution.

### 23.3 Diagnostic mode

A reporting-capable environment may enter a diagnostic-only mode when profile requirements are unsatisfied.

Diagnostic mode must not be represented as profile-operational.

---

## 24. Controlled quiescence

Before controlled return, reboot, halt, or power-off, the environment must attempt to quiesce its owned work.

Quiescence includes applicable:

1. refusing new provider operations;
2. stopping or completing accepted operations according to policy;
3. cancelling or terminating operation-local execution units;
4. finalizing authoritative results;
5. recording partial and indeterminate effects;
6. quarantining incomplete candidate outputs;
7. reporting unreleased resources;
8. synchronizing ASM-owned writable filesystems;
9. unmounting or closing ASM-owned storage;
10. closing provider and result channels;
11. emitting a termination result.

### 24.1 Quiescence outcome

The environment must distinguish:

- quiescence not required;
- requested;
- active;
- complete;
- partial;
- failed;
- indeterminate.

### 24.2 External ownership

The environment must not terminate work owned by another subsystem without the applicable contract or emergency authority.

Unresolved externally owned work must be reported.

---

## 25. Shutdown, reboot, and return of control

A profile must declare which termination mechanisms it supports.

These may include:

- return to BOOT;
- return to a bootloader;
- return to firmware;
- transfer to an installer;
- controlled reboot;
- controlled power-off;
- halt;
- bounded safe stopped state.

### 25.1 Mechanism versus semantic result

The following must remain separate:

```text
return mechanism invoked
    != caller accepted control

installer invoked
    != installation completed

reboot initiated
    != reboot target accepted

power-off requested
    != durable state verified

runtime launched
    != runtime ready
```

### 25.2 Termination authority

Availability of reboot, power-off, target-transfer, or installer-launch mechanisms does not create authority to invoke them.

### 25.3 Return result

Before controlled return where technically possible, the environment must emit or persist a result describing:

- environment instance;
- return reason;
- quiescence outcome;
- outstanding provider operations;
- partial candidate outputs;
- cleanup state;
- unreleased resources;
- intended next control recipient;
- mechanism invoked;
- known limitations.

### 25.4 Failed return

If return of control fails or cannot be confirmed, the environment must report or preserve an indeterminate return state where possible.

It must not claim BOOT handoff completion.

---

## 26. Recovery-path preservation

ASM preserves mechanisms that permit BOOT or another owner to attempt recovery.

ASM does not declare recovery complete.

Recovery-path preservation may include:

- retaining the authoritative environment result;
- retaining bounded failure evidence;
- preserving candidate-output quarantine;
- preserving cleanup state;
- retaining enough writable reserve to report;
- returning to BOOT;
- exposing a reboot mechanism;
- entering a bounded diagnostic mode;
- avoiding destructive target mutation.

A preserved recovery path is not recovery.

---

## 27. Environment failure behavior

ASM-0006 defines the detailed assembly-kernel failure model.

ASM-0002 requires the environment to distinguish at least:

- entry failure;
- control-plane establishment failure;
- profile-reference failure;
- result-channel failure;
- filesystem failure;
- execution-supervision failure;
- provider-unavailable failure;
- time-basis limitation;
- resource exhaustion;
- quiescence failure;
- return-of-control failure;
- environment-indeterminate state.

### 27.1 Failure reporting

A reporting-capable environment must produce a structured failure result.

When the primary result path fails, the environment should attempt a declared bounded fallback.

### 27.2 No favorable inference

Environment failure must not be translated into:

- BOOT denial;
- target invalidity;
- source invalidity;
- compromise;
- unrecoverability;
- installer failure

without an owning evaluator’s result.

### 27.3 Restart behavior

Restarting the environment does not erase unresolved:

- provider operations;
- candidate outputs;
- cleanup;
- quarantine;
- resource ownership;
- return state.

---

## 28. Minimal and headless baseline

A conforming assembly environment must not inherently require:

- a desktop;
- a display server;
- a graphical installer;
- a browser;
- a monitor;
- a keyboard;
- a local human-readable console;
- the normal Node runtime;
- Julia;
- Python;
- one shell;
- one package manager;
- one container runtime;
- permanent networking;
- unrestricted package installation;
- unrestricted storage;
- unrestricted memory.

A profile may use any of those facilities.

They must not become universal architectural dependencies.

---

## 29. Public and private boundary

Public ASM architecture may define:

- generic capability categories;
- startup and shutdown semantics;
- machine-readable result requirements;
- generic provider interfaces;
- generic storage classes;
- generic resource requirements;
- generic degraded and failure behavior;
- public conformance requirements.

Public ASM documentation must not require disclosure of:

- production credentials;
- private keys;
- private source locations;
- protected toolchain locations;
- private module identities;
- production authority assignments;
- production installer policy;
- hidden selection rules;
- production hardware topology.

A public assembly-environment profile must remain operable without private modules unless it explicitly describes itself as a private extension rather than public conformance.

---

## 30. Prohibited interpretations

This specification must not be interpreted to mean that:

- environment entry means environment startup succeeded;
- environment startup means the declared profile is satisfied;
- profile satisfaction means a provider operation is authorized;
- provider-operation readiness means provider-operation completion;
- a local compiler is mandatory for every assembly environment;
- networking is mandatory for every assembly environment;
- wall-clock time is mandatory;
- a graphical interface is required;
- root privileges create Node authority;
- environment capability creates operation authority;
- a process exit status is a provider result;
- a provider result is a BOOT result;
- ASM output staging is assembly-generation staging;
- observed target storage may be mutated by ASM;
- security verification creates release or installation authority;
- return of control means BOOT accepted the handoff;
- reboot means recovery;
- runtime launch means runtime readiness;
- the environment instance is Node identity;
- the environment instance is `SystemProfileIdentity`;
- a private extension may weaken public ASM requirements.

---

## 31. Relationship to later ASM specifications

### ASM-0003 — Hardware Inspection Mechanisms

ASM-0003 defines:

- hardware subjects;
- observation records;
- initialization state;
- inspection providers;
- observation limits;
- unknown and conflicting results;
- observation-only behavior.

### ASM-0004 — Build Capability and Toolchain Boundary

ASM-0004 defines:

- provider-operation build categories;
- source access;
- compiler, assembler, linker, packaging, and test mechanisms;
- authority-not-required diagnostic work;
- Node-targeted build authority;
- candidate-output production;
- toolchain evidence.

### ASM-0005 — Resource and Workspace Model

ASM-0005 defines:

- resource envelopes;
- source areas;
- active workspaces;
- caches;
- evidence areas;
- output staging;
- quotas;
- reserve capacity;
- cleanup;
- quarantine.

### ASM-0006 — Kernel-Specific Failure Model

ASM-0006 defines:

- environment failure records;
- process and provider failures;
- partial output;
- interruption;
- cleanup failure;
- indeterminate state;
- return-of-control failure.

### ASM-0007 — Assembly-Kernel Conformance

ASM-0007 defines tests for:

- startup;
- profile reporting;
- capability reporting;
- offline behavior;
- process supervision;
- resource bounds;
- structured results;
- degraded operation;
- quiescence;
- termination;
- negative and failure behavior.

---

## 32. Cross-series dependencies and open contracts

### 32.1 KRN-owned contracts

KRN must define or stabilize:

- kernel-profile structure;
- micro-OS-profile structure;
- primary and subordinate profile relationships;
- compatibility-declaration structure;
- required-, optional-, constrained-, and unsupported-component declarations;
- profile lifecycle;
- role and era declarations.

ASM-0002 does not prescribe those schemas.

### 32.2 BOOT-owned contracts

BOOT must define or stabilize:

- environment invocation context;
- semantic assembly-operation identity;
- authority references;
- provider-operation requests;
- component-set instance references;
- pair-evaluation results;
- assembly-generation acceptance;
- installer handoff;
- return-of-control acknowledgement;
- continuation and reconciliation after interrupted environment instances.

ASM-0002 does not prescribe those schemas.

### 32.3 Resource-management contracts

Resource-management architecture must define or stabilize:

- resource-envelope structure;
- reservation semantics;
- ceilings;
- accounting;
- reclamation;
- pressure policy.

### 32.4 Security-provider contracts

Security-provider architecture must define or stabilize:

- provider identity;
- verification-result structure;
- protected key operations;
- credential handling;
- secure-session results.

### 32.5 Installer contracts

Installer architecture must define or stabilize:

- installer invocation;
- production-target selection;
- mutation authority;
- write verification;
- activation staging;
- rollback;
- installer completion result.

### 32.6 Deferred ownership

Ownership remains deferred for:

- aggregate `SystemProfileIdentity`;
- complete installed-system generation architecture;
- update architecture;
- source and release architecture;
- production rollout architecture.

No unresolved external schema authorizes ASM to invent favorable semantics.

---

## 33. Conformance obligations

ASM-0007 will define exact test procedures.

A conforming assembly environment must demonstrate that it can:

- enter without claiming startup success prematurely;
- establish a bounded reporting-capable control plane;
- report active profile references and unresolved profile state;
- distinguish profile satisfaction from operation readiness;
- start without a GUI or local human interaction;
- avoid indefinite undeclared waits;
- operate offline where the profile permits;
- report optional network loss accurately;
- provide structured authoritative results;
- declare result-channel precedence;
- supervise operation-local execution units;
- distinguish execution completion from provider completion;
- separate environment-owned storage from production targets;
- preserve read-only target observation;
- expose bounded workspaces and evidence areas;
- report explicit time uncertainty;
- consume security-provider results without broadening them;
- preserve resource reserve for failure handling;
- enter degraded diagnostic operation without claiming profile satisfaction;
- quiesce owned work;
- report partial cleanup and unresolved effects;
- invoke only supported and authorized termination mechanisms;
- return control without claiming BOOT acceptance;
- avoid claiming installation, activation, recovery, or runtime readiness.

---

## 34. Completion checklist

Draft review must confirm:

- [ ] Assembly environment remains an ASM term.
- [ ] Assembly generation remains a BOOT term.
- [ ] Kernel/micro-OS compatibility declaration remains a KRN term.
- [ ] One primary `MicroOsProfileIdentity` is used.
- [ ] Subordinate identities remain conditional.
- [ ] Environment-instance correlation does not become Node identity.
- [ ] Environment entry, reporting capability, profile satisfaction, and operation readiness remain separate.
- [ ] Startup is headless and bounded.
- [ ] Human input is not required.
- [ ] Networking remains profile-declared.
- [ ] Offline operation remains supported where declared.
- [ ] A local compiler is not made universally mandatory.
- [ ] Provider-operation execution may be local or remote.
- [ ] Task supervision is implementation-neutral.
- [ ] Process completion remains distinct from provider completion.
- [ ] Storage classes remain separated.
- [ ] Production targets remain observation-only to ASM.
- [ ] Machine-readable results are authoritative.
- [ ] Logs remain bounded and non-authoritative.
- [ ] Time uncertainty remains explicit.
- [ ] Resource reserve remains protected.
- [ ] Degraded operation does not hide substitution.
- [ ] Quiescence state remains explicit.
- [ ] Return of control remains distinct from handoff acceptance.
- [ ] Recovery-path preservation remains distinct from recovery completion.
- [ ] No BOOT progression state is defined by ASM.
- [ ] Aggregate `SystemProfileIdentity` ownership remains deferred.
- [ ] No implementation language, init system, filesystem layout, package manager, or network protocol is mandated.
- [ ] No cross-series conflict is silently resolved.

---

## 35. Closing principle

> **The assembly environment must always be capable of saying exactly what became available, what did not, what it attempted, what remains running, and how control was returned.**

## Revision history

### Version 0.1 — 2026-07-18

- Replaced the ASM-0002 placeholder with the initial assembly-environment contract.
- Defined environment entry, reporting-capable, profile-operational, degraded, failed, and indeterminate meanings.
- Defined environment-instance correlation without creating Node identity.
- Preserved one primary `MicroOsProfileIdentity` with conditional subordinate identities.
- Established bounded deterministic startup semantics.
- Defined minimum control capabilities and assembly-role capabilities.
- Defined declaration-versus-observation capability semantics.
- Defined the environment capability report.
- Established implementation-neutral task and process supervision.
- Defined storage and filesystem separation.
- Preserved production targets as observation-only to ASM.
- Defined offline and optional-network behavior.
- Defined security-provider access and result boundaries.
- Required explicit time and ordering semantics.
- Required bounded authoritative machine-readable results.
- Defined provider-operation admission and degraded operation.
- Defined quiescence, termination, and return-of-control behavior.
- Preserved BOOT ownership of authority, assembly generations, installer handoff, recovery, and runtime-handoff eligibility.
- Preserved deferred ownership of aggregate `SystemProfileIdentity`.
