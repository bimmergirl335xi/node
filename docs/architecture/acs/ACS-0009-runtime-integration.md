# ACS-0009: Runtime Integration

| Field | Value |
|---|---|
| Specification | ACS-0009 |
| Title | Runtime Integration |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ACS-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Pending review |
| Depends on | ACS-0000 through ACS-0008 |
| Related specifications | MEM-0000 through MEM-0010, future IMM specification series, compute-backend and system-profile specifications |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in semantic/runtime separation, enforcement placement, bounded execution, and failure-state requirements; distributed scheduling profiles, migration coordination, and baseline runtime conformance levels remain under review |

> **The runtime may implement ACS objects, enforce ACS policy, and move ACS work across hardware, but it shall not redefine the meaning, authority, identity, or lifecycle of the objects it carries.**

## Architectural-intent notice

This specification defines the public architectural boundary between the Adaptive Connection Substrate and the runtime systems that implement it.

It is independently authored public architecture.

It is not produced by deleting, sanitizing, paraphrasing, or selectively redacting restricted architecture.

This specification defines public concepts and guarantees required for independently implementable Node-compatible runtimes without disclosing:

- production runtime topology;
- private scheduling policy;
- proprietary placement algorithms;
- production node inventories;
- protected service catalogs;
- production resource allocations;
- private recovery procedures;
- internal cognitive deployment policy;
- production transport selection;
- restricted operator controls;
- production process layouts;
- private accelerator assignments.

This specification does not prescribe:

- one operating system;
- one process model;
- one programming language;
- one scheduler;
- one transport library;
- one queue implementation;
- one serialization system;
- one container runtime;
- one service manager;
- one cluster manager;
- one hardware platform;
- one device-discovery mechanism;
- one orchestration product;
- one deployment topology;
- one production placement algorithm.

Implementations may use different mechanisms.

They must preserve the identities, contracts, authority boundaries, lifecycle states, resource limits, failure semantics, and cross-substrate distinctions established by ACS.

## 1. Purpose

ACS defines logical communication architecture.

A runtime must turn that architecture into working software using concrete:

- processes;
- threads;
- tasks;
- hosts;
- devices;
- queues;
- shared memory;
- sockets;
- secure sessions;
- files;
- timers;
- schedulers;
- transport bindings;
- service managers;
- hardware backends.

Those mechanisms are necessary.

They are also dangerous if allowed to silently define architecture.

Without an explicit runtime boundary, an implementation may incorrectly assume that:

- one process equals one participant;
- one socket equals one connection;
- one queue equals one port;
- process restart creates a new logical identity;
- host ownership grants endpoint ownership;
- scheduler priority creates authority;
- local dispatch may bypass admission;
- device presence creates semantic capability;
- thread liveness proves endpoint readiness;
- transport failure proves relationship loss;
- available memory grants unlimited queue growth;
- runtime migration transfers ownership;
- fast local paths may omit validation;
- service-manager control grants lifecycle authority;
- container isolation proves security;
- runtime logs define semantic completion;
- successful message delivery proves domain-operation completion;
- runtime recovery may restore stale authority.

ACS-0009 defines how runtime mechanisms implement ACS without absorbing or redefining it.

## 2. Scope

This specification governs:

- representation of ACS objects in runtime systems;
- mapping of participants to processes and tasks;
- endpoint and port implementation;
- connection and binding implementation;
- attachment enforcement;
- signal dispatch;
- payload-reference handling;
- mediation execution;
- admission enforcement;
- capability enforcement;
- budget enforcement;
- protected capacity;
- queueing;
- scheduling;
- cancellation;
- backpressure;
- load shedding;
- runtime placement;
- local and remote fast paths;
- migration;
- restart;
- failover;
- recovery;
- hibernation;
- shutdown;
- device and backend integration;
- runtime health;
- observability;
- audit;
- clock and timer behavior;
- configuration and profiles;
- version compatibility;
- degraded operation;
- partition behavior;
- failure containment;
- minimal headless operation;
- public conformance requirements.

This specification does not define:

- proprietary placement algorithms;
- production resource values;
- private cognitive topology;
- memory-operation semantics;
- immune detection algorithms;
- credential formats;
- cryptographic algorithms;
- physical-device driver APIs;
- exact hardware backend interfaces;
- production operator workflows;
- operating-system installation;
- software propagation;
- one public deployment manifest format.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements.

The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification.

The word **may** describes permitted behavior.

An implementation does not conform merely because it can transport ACS-shaped messages.

Its observable behavior must preserve the semantic and authority distinctions established throughout the ACS series.

## 4. Foundational distinctions

### 4.1 Runtime object is not architectural object

A process, thread, queue, socket, file, task, container, or service unit may implement an ACS object.

It is not automatically identical to that object.

### 4.2 Process identity is not participant identity

One participant may be implemented by:

- one process;
- several processes;
- one task inside a shared process;
- several cooperating hosts;
- no currently running process.

One process may implement several participants when their identities and authority remain isolated.

### 4.3 Host is not owner

Hosting an endpoint, port, participant, or connection does not grant logical ownership.

### 4.4 Queue is not port

A queue stores or orders runtime work.

A port defines a semantic communication contract.

Several ports may share a queue.

One port may use several queues.

### 4.5 Socket is not connection

A socket or transport channel is one possible binding.

A connection remains a logical ACS runtime object with lifecycle, generation, attachments, and authority.

### 4.6 Scheduler priority is not signal priority

A runtime scheduler may translate ACS priority into implementation behavior.

It shall not silently create or amplify semantic priority.

### 4.7 Scheduler priority is not authority

Faster execution does not grant permission.

### 4.8 Resource availability is not admission

Free CPU, GPU, memory, storage, or bandwidth does not authorize work.

### 4.9 Admission is not placement

Admitting communication does not determine where work executes.

### 4.10 Placement is not ownership

Moving work to another host, process, accelerator, or region does not transfer semantic ownership.

### 4.11 Liveness is not readiness

A running process may not be able to satisfy its endpoint or port contracts.

### 4.12 Delivery is not semantic completion

Runtime delivery, queue removal, system-call success, or transport acknowledgement does not prove domain-operation completion.

### 4.13 Local is not trusted

Same-process or same-host communication remains subject to applicable identity, admission, authority, validation, and resource rules.

### 4.14 Container isolation is not architectural isolation

Container or process boundaries may contribute implementation protection.

They do not replace ACS identity, authority, and port boundaries.

### 4.15 Device discovery is not semantic registration

Detecting hardware does not automatically create an ACS participant, endpoint, port, relationship, or authority.

### 4.16 Restart is not recovery

Restarting software does not establish:

- identity continuity;
- connection continuity;
- attachment validity;
- operation completion;
- authority validity;
- memory consistency.

### 4.17 Recovery is not resumption

Recovery may require new runtime instances, bindings, sessions, generations, grants, and reconciliation.

### 4.18 Monitoring is not enforcement authority

A runtime observer does not gain authority to change the objects it observes merely because it has telemetry access.

## 5. Core terminology

### 5.1 Runtime

The runtime is the collection of software and infrastructure mechanisms that instantiate, schedule, connect, observe, and recover ACS participants and communication.

### 5.2 Runtime instance

A runtime instance is one executing realization of a participant, endpoint implementation, mediator, or supporting service.

### 5.3 Execution context

An execution context is a bounded environment in which runtime work executes.

It may include:

- process;
- thread;
- task;
- container;
- virtual machine;
- host;
- accelerator context;
- sandbox.

### 5.4 Runtime binding

A runtime binding maps an ACS endpoint, port, connection, or payload path to concrete communication resources.

### 5.5 Dispatch

Dispatch is the runtime act of selecting an eligible execution path for admitted work.

### 5.6 Placement

Placement assigns a runtime instance or work item to an eligible execution context or resource.

### 5.7 Runtime profile

A runtime profile declares supported mechanisms, limits, dependencies, and behavior for a repeatable implementation or deployment class.

### 5.8 System profile

A system profile describes validated hardware, operating environment, runtime capabilities, and policy-relevant limits for one Node installation or class of installations.

### 5.9 Execution lease

An execution lease is a bounded runtime permission to occupy or use an execution context or resource.

An execution lease is not semantic ownership.

### 5.10 Runtime generation

A runtime generation distinguishes current runtime state from stale process-local or host-local state after restart, recovery, or replacement.

### 5.11 Runtime health

Runtime health describes whether implementation mechanisms operate within declared limits.

### 5.12 Runtime readiness

Runtime readiness describes whether the implementation can currently satisfy a declared ACS contract or operation profile.

## 6. Runtime representation model

A conceptual runtime mapping is:

```text
ACS participant
    represented by one or more runtime instances

ACS endpoint
    implemented by one or more endpoint handlers

ACS port
    implemented by typed dispatch and validation logic

ACS relationship
    referenced by runtime connection state

ACS connection
    implemented through one or more bindings

ACS attachment
    enforced at dispatch and admission boundaries

ACS signal
    represented by a validated runtime message or local call record

ACS payload reference
    resolved through separately governed transfer or access mechanisms

Runtime resource
    hosts or carries implementation state without becoming its semantic owner
```

Implementations may combine these into fewer structures.

Their meanings must remain distinguishable.

## 7. Participant representation

### 7.1 One participant, several instances

A participant may have several authorized runtime instances for:

- replication;
- failover;
- locality;
- scaling;
- role separation;
- read/write separation.

The replication profile must define whether instances are:

- active-passive;
- active-active;
- partitioned by operation;
- partitioned by port;
- locality-selected;
- quorum-coordinated;
- independently restricted.

### 7.2 Several participants, one process

One process may implement several participants when it preserves:

- distinct identities;
- distinct authority;
- distinct endpoint ownership;
- distinct admission;
- distinct resource accounting;
- failure visibility;
- audit attribution.

### 7.3 No implicit identity sharing

Threads or services in one process shall not share participant identity merely because they share memory or executable code.

### 7.4 Instance registration

A runtime instance shall not represent a participant until required:

- identity;
- profile;
- lifecycle;
- ownership;
- security;
- compatibility;

conditions are established.

### 7.5 Instance replacement

Replacing an instance does not automatically change participant identity.

It may require new:

- instance identity;
- runtime generation;
- binding;
- connection generation;
- secure session;
- execution lease.

## 8. Endpoint implementation

An endpoint implementation shall preserve:

- endpoint identity;
- owner identity;
- ownership epoch where applicable;
- port inventory within authorized visibility;
- availability state;
- contract versions;
- binding state;
- lifecycle state;
- conflict state.

A runtime shall not derive endpoint identity solely from:

- process identifier;
- socket address;
- service name;
- host name;
- queue name.

## 9. Port implementation

Every port implementation shall enforce or invoke enforcement for:

- direction;
- signal domains;
- signal intents;
- schemas;
- subtype compatibility;
- attachment scope;
- authority;
- payload policy;
- resource limits;
- mediation;
- lifecycle state;
- failure behavior.

A generic untyped dispatch function does not by itself satisfy a port contract.

## 10. Connection implementation

A connection implementation shall preserve:

- relationship identity;
- connection-instance identity;
- connection generation;
- endpoint identities;
- active bindings;
- attachments;
- security state;
- resource state;
- lifecycle state;
- continuity confidence.

A socket becoming writable shall not automatically mark the ACS connection active.

## 11. Binding implementation

Bindings may use:

- local function dispatch;
- shared memory;
- queues;
- pipes;
- local sockets;
- remote sockets;
- secure sessions;
- bridge paths;
- store-and-forward systems;
- future transports.

Changing binding technology shall not silently alter:

- port meaning;
- relationship class;
- attachment scope;
- authority;
- delivery guarantees;
- payload semantics.

## 12. Attachment enforcement

Attachment enforcement shall occur before ordinary port processing.

The enforcement point shall verify as applicable:

- attachment identity;
- state;
- connection generation;
- source identity;
- direction;
- signal family;
- payload category;
- authority;
- rate;
- priority ceiling;
- validity;
- mediation path.

A local direct call shall not bypass attachment enforcement when an equivalent remote interaction would require it.

## 13. Signal dispatch

Runtime signal dispatch shall preserve:

- signal identity;
- source identity;
- target scope;
- domain;
- intent;
- subtype;
- schema;
- freshness;
- priority;
- confidence;
- provenance;
- authority;
- payload references;
- duplicate semantics.

The runtime may use optimized internal representations.

Optimization shall not remove semantically required information.

## 14. Local fast paths

### 14.1 Purpose

Local fast paths may reduce serialization, copying, context switching, encryption overhead, and transport latency.

### 14.2 Semantic equivalence

A local fast path shall preserve the same effective:

- identity;
- port contract;
- attachment;
- authorization;
- admission;
- validation;
- budget;
- lifecycle;
- result semantics;

required by the corresponding non-local path.

### 14.3 No pointer authority

Possession of a local pointer, object reference, file descriptor, shared-memory handle, or function reference shall not create semantic authority.

### 14.4 Validation reduction

Validation work may be optimized when equivalent guarantees are established through trusted runtime structure.

The omitted checks and replacement guarantees must be documented.

### 14.5 Fast-path fallback

Fallback from local to remote communication shall preserve operation and signal identity.

It shall not duplicate semantic effects.

## 15. Queueing

Every runtime queue shall possess hard limits for:

- item count;
- byte count;
- item size;
- age;
- source occupancy;
- attachment occupancy;
- priority occupancy;
- retention.

No queue may rely solely on eventual processing to prevent unbounded growth.

## 16. Queue semantics

Queue insertion may represent:

- accepted for communication processing;
- accepted for domain processing;
- deferred;
- reserved;
- pending mediation.

The meaning must be explicit.

Queue insertion shall not be reported as semantic operation completion.

## 17. Queue isolation

Shared queues shall preserve sufficient isolation by:

- source;
- attachment;
- port;
- relationship;
- resource class;
- priority;
- protected class.

One port or participant shall not consume all shared queue capacity without explicit policy.

## 18. Scheduling

### 18.1 Scheduling responsibility

Runtime scheduling decides when and where admitted work executes within available leases, budgets, and constraints.

### 18.2 Scheduling inputs

Scheduling may consider:

- admission grant;
- resource budget;
- priority ceiling;
- deadline;
- locality;
- health;
- hardware capability;
- energy;
- protected class;
- cancellation state;
- migration state.

### 18.3 Scheduling does not create authority

A scheduler shall not run unauthorized work merely because resources are available.

### 18.4 Scheduling does not redefine priority

Runtime ordering may approximate ACS priority.

It shall not silently increase semantic priority.

### 18.5 No universal scheduler requirement

Node-compatible runtimes may use centralized, distributed, hierarchical, local, or cooperative scheduling.

The architectural guarantees must remain equivalent.

## 19. Execution leases

An execution lease shall define:

- holder;
- resource;
- scope;
- amount;
- start boundary;
- expiry;
- renewal;
- preemption;
- cancellation;
- release.

A lease shall not:

- transfer endpoint ownership;
- create participant identity;
- grant unrelated port authority;
- outlive its governing admission indefinitely.

## 20. Resource enforcement

Runtime systems shall enforce applicable ACS-0006:

- hard ceilings;
- budgets;
- quotas;
- reservations;
- credits;
- rate limits;
- burst limits;
- outstanding-work limits;
- fan-out limits;
- protected capacity.

A runtime implementation shall not silently weaken a higher-authority limit.

## 21. Resource estimation

Runtime cost estimates may include:

- memory;
- CPU time;
- GPU or accelerator time;
- storage;
- bandwidth;
- queue occupancy;
- latency;
- energy;
- mediator work;
- validation work.

Uncertainty shall be represented.

Unknown cost does not imply zero cost.

## 22. Runtime overruns

When work exceeds its admitted estimate or lease, the runtime may:

- throttle;
- pause;
- preempt;
- cancel;
- reduce fidelity;
- migrate;
- request additional admission;
- produce a partial result;
- mark the result indeterminate;
- enter degraded state.

Overrun shall not automatically consume protected capacity.

## 23. Preemption

Preemption policy shall define:

- eligible work;
- authority;
- state preservation;
- cancellation interaction;
- partial-result behavior;
- resource release;
- restart or resume behavior.

Preempted work shall not silently appear completed.

## 24. Cancellation

Runtime cancellation shall preserve the distinction between:

- cancellation request;
- cancellation admitted;
- cancellation initiated;
- cancellation completed;
- cancellation failed;
- operation already completed;
- operation indeterminate.

Cancellation of communication does not automatically cancel a domain operation.

## 25. Backpressure

Runtime mechanisms shall propagate ACS backpressure sufficiently to prevent unsafe accumulation.

Possible mechanisms include:

- credits;
- queue refusal;
- rate reduction;
- paused reads;
- bounded subscriptions;
- demand-driven dispatch;
- payload-window reduction;
- admission restriction.

Backpressure is not a trust or semantic-failure judgment.

## 26. Load shedding

Runtime load shedding shall follow declared port and signal semantics.

Potential behavior includes:

- stale observation expiry;
- duplicate collapse;
- sampling;
- aggregation;
- fidelity reduction;
- optional-output suspension;
- rejection of new work.

The runtime shall not silently discard:

- required directive results;
- lifecycle outcomes;
- evidence requiring retention;
- ordered operations whose loss changes meaning.

## 27. Protected capacity

Runtime systems shall preserve protected capacity for applicable:

- infrastructure;
- security;
- immune communication;
- lifecycle;
- recovery;
- physical safety;
- continuity-critical communication.

Protected queues, workers, memory, or transport capacity may be used.

Protected status remains explicit and bounded.

## 28. Runtime placement

### 28.1 Eligible placement

Placement shall consider applicable:

- hardware capabilities;
- security requirements;
- trust domain;
- data locality;
- memory locality;
- device access;
- resource limits;
- energy;
- health;
- failure domains;
- latency;
- operator policy.

### 28.2 Placement does not change semantics

Placement shall not silently alter:

- participant identity;
- endpoint ownership;
- port contract;
- authority;
- relationship class;
- operation identity.

### 28.3 Unsupported placement

A runtime shall reject or report unsupported placement rather than silently executing with weaker guarantees.

### 28.4 Unknown placement capability

Unknown hardware or runtime capability shall not be treated as supported capability.

## 29. Hardware and backend integration

### 29.1 Logical representation

Physical hardware shall be represented through logical backend or service participants where ACS communication is required.

### 29.2 No duplicate physical ownership

Several runtime services shall not independently claim authoritative ownership of the same physical device without an explicit sharing or composite model.

### 29.3 Discovery versus registration

Hardware discovery establishes observed presence.

Backend registration establishes a logical service representation after validation and policy.

### 29.4 Capability reporting

Backend capability reports shall distinguish:

- supported;
- unsupported;
- unavailable;
- degraded;
- unknown;
- conflicting.

### 29.5 Health reporting

Hardware health contributes runtime evidence.

It does not determine semantic authority.

### 29.6 Device loss

Device loss may degrade or fail runtime work.

It does not automatically retire:

- participant identity;
- endpoint identity;
- relationship;
- domain state.

## 30. Heterogeneous execution

The runtime may place work across:

- CPUs;
- GPUs;
- accelerators;
- storage processors;
- embedded controllers;
- remote nodes;
- future devices.

Heterogeneity shall not change operation semantics unless an explicitly declared implementation profile defines a compatible variation.

## 31. Replicated runtime instances

Replicated instances shall define:

- active scopes;
- port coverage;
- state-sharing model;
- consistency requirements;
- authority;
- routing;
- failover;
- duplicate suppression;
- resource division;
- health reporting.

A replica shall not become authoritative merely because it responds first.

## 32. Failover

Failover shall establish:

- intended participant identity;
- replacement instance identity;
- current runtime generation;
- endpoint continuity;
- connection treatment;
- attachment treatment;
- operation reconciliation;
- stale-instance invalidation.

Failover shall not silently create two active exclusive authorities.

## 33. Migration

### 33.1 Migration scope

Migration may move:

- participant instance;
- endpoint implementation;
- selected ports;
- selected connections;
- queued work;
- execution state;
- payload-transfer state.

### 33.2 Migration record

Migration should make it possible to determine:

```text
RuntimeMigration
    migration_identity
    participant_identity
    source_instance
    target_instance
    affected_endpoints
    affected_ports
    affected_connections
    current_runtime_generation
    next_runtime_generation
    queued_work_treatment
    in_flight_work_treatment
    attachment_treatment
    security_treatment
    resource_treatment
    source_shutdown_state
    target_activation_state
    completion_state
```

### 33.3 No silent duplication

Migration shall prevent duplicated semantic execution.

### 33.4 Failure behavior

Failed migration shall not leave both source and target silently authoritative.

### 33.5 Migration and MEM

Moving a memory-service runtime does not move, transfer, commit, delete, or alter memory unless MEM operations explicitly perform those actions.

## 34. Restart

### 34.1 Process restart

Process restart shall invalidate or reconcile process-local:

- queues;
- leases;
- sessions;
- connection state;
- attachment caches;
- authorization caches;
- in-flight work.

### 34.2 Restart identity

A restarted process normally receives a new runtime-instance identity or generation.

### 34.3 No stale authority

Persisted runtime state shall not restore stale:

- capabilities;
- attachments;
- connection generations;
- secure sessions;
- leases;
- revocations.

### 34.4 Operation reconciliation

The runtime shall reconcile operations that may have:

- completed;
- partially executed;
- failed;
- remained queued;
- become indeterminate.

## 35. Recovery

Recovery shall establish sufficient evidence for:

- participant continuity;
- endpoint continuity;
- runtime generation;
- configuration validity;
- security state;
- connection state;
- attachment state;
- outstanding work;
- resource accounting;
- domain-state coordination.

A runtime shall not report recovery complete solely because processes are running.

## 36. Hibernation

### 36.1 Hibernation purpose

Hibernation preserves enough governed state to suspend Node operation safely and later recover.

### 36.2 Hibernation preparation

Preparation may include:

- restricting new work;
- draining;
- cancelling optional work;
- preserving operation identities;
- recording lifecycle state;
- releasing devices;
- closing or suspending connections;
- coordinating MEM persistence;
- preserving recovery evidence.

### 36.3 Hibernation state is not runtime liveness

A hibernated participant may retain logical identity without active runtime instances.

### 36.4 Wake

Wake shall validate:

- persisted state;
- configuration;
- security;
- credentials;
- revocations;
- hardware capability;
- runtime generation;
- connection re-establishment.

Wake does not automatically resume stale authority or in-flight work.

## 37. Shutdown

Shutdown shall distinguish:

- requested;
- admitted;
- draining;
- safe to stop;
- stopped;
- failed;
- forced;
- indeterminate.

Shutdown shall preserve capacity for:

- physical safety;
- evidence;
- memory coordination;
- lifecycle reporting;
- recovery metadata.

A process exit is not sufficient evidence of safe shutdown.

## 38. Minimal headless operation

### 38.1 No GUI dependency

Core ACS runtime behavior shall not depend on:

- graphical desktops;
- display servers;
- window systems;
- local human-readable dashboards;
- interactive terminals.

### 38.2 Headless control

Required runtime control shall be available through governed machine interfaces, configuration, or remote operator channels.

### 38.3 Optional diagnostics

Terminal or local display output may exist as optional diagnostics.

Its absence shall not impair core function.

### 38.4 Minimal environment

A conforming runtime may operate on an environment containing only what is required to:

- boot;
- initialize devices;
- initialize networking;
- load runtime services;
- enforce security;
- expose health;
- execute ACS work;
- recover safely.

## 39. Configuration

Runtime configuration shall be:

- identifiable;
- versioned where material;
- validated;
- attributable;
- bounded;
- distinguishable from discovered state.

Configuration shall not silently override higher ACS authority or safety rules.

## 40. System profiles

A system profile may declare:

- hardware capabilities;
- operating environment;
- runtime features;
- supported ACS versions;
- supported transport profiles;
- resource ceilings;
- protected-capacity expectations;
- security dependencies;
- recovery support;
- known limitations.

A profile is a claim requiring validation.

It is not proof merely because it is installed.

## 41. Configuration and discovery

Configured capability and observed capability shall remain distinct.

For example:

```text
Configured:
    GPU execution expected

Observed:
    no compatible GPU currently available

Effective:
    GPU execution unavailable
```

The runtime shall not report configured-but-unobserved capability as ready.

## 42. Dynamic discovery

Runtime discovery may observe:

- hardware;
- networks;
- transports;
- local services;
- compatible participants;
- devices;
- capacities.

Discovery does not create:

- ownership;
- relationship;
- admission;
- attachment;
- authority;
- trust.

## 43. Runtime versioning

Runtime implementations shall report or preserve:

- implementation version;
- ACS compatibility;
- enabled features;
- schema support;
- transport profiles;
- known limitations.

Version compatibility shall be evaluated before activating incompatible bindings or fast paths.

## 44. Upgrade

Upgrade may change:

- implementation;
- process layout;
- transport;
- serialization;
- scheduler;
- device support;
- performance.

Upgrade shall not silently change ACS semantics.

Material contract changes require appropriate:

- versioning;
- migration;
- attachment renewal;
- lifecycle transition;
- compatibility handling.

## 45. Rolling upgrade

Rolling upgrade shall define:

- mixed-version behavior;
- compatibility window;
- connection treatment;
- migration;
- rollback;
- stale-instance handling;
- evidence that old behavior no longer remains authoritative.

## 46. Rollback

Rollback shall not restore:

- revoked credentials;
- stale capabilities;
- retired identities;
- obsolete connection generations;
- deleted MEM state;
- superseded lifecycle state.

Software rollback and architectural state rollback are separate operations.

## 47. Runtime health model

Runtime health may include:

- process health;
- thread health;
- queue health;
- transport health;
- device health;
- scheduler health;
- memory pressure;
- resource pressure;
- clock health;
- configuration health;
- security dependency health.

Health shall be scoped.

One healthy subsystem shall not conceal another failed dependency.

## 48. Liveness, readiness, and capability

The runtime shall distinguish:

```text
Process liveness
    is an implementation process running?

Runtime health
    are implementation mechanisms functioning?

Endpoint readiness
    can the endpoint support its declared communication boundary?

Port readiness
    can the port accept or emit an eligible interaction?

Domain readiness
    can the domain service perform the requested semantic operation?
```

No layer shall claim the state of another without evidence.

## 49. Health observations expire

Health and readiness observations shall have freshness boundaries.

Old health evidence shall not remain current indefinitely.

## 50. Runtime degradation

A runtime may enter degraded operation because of:

- resource pressure;
- device loss;
- transport loss;
- partial configuration;
- security dependency loss;
- clock uncertainty;
- replica loss;
- incompatible versions;
- partition.

Degraded operation shall define which:

- participants;
- endpoints;
- ports;
- directions;
- signal families;
- payload transfers;
- resources;

remain supported.

## 51. Partition behavior

During partition, runtimes may continue local operation only within pre-established:

- identity;
- admission;
- capability;
- resource;
- lifecycle;
- security;

boundaries.

Partition shall not silently create:

- new ownership;
- duplicated exclusive leases;
- duplicated connection authority;
- unlimited local budgets;
- new trust roots.

After partition, runtimes shall reconcile:

- runtime generations;
- connection generations;
- leases;
- attachments;
- revocations;
- queued work;
- migrations;
- health state;
- resource accounting.

## 52. Clock and timer behavior

Runtime clocks may support:

- deadlines;
- expiry;
- retries;
- liveness;
- scheduling;
- retention windows.

Implementations shall account for:

- clock skew;
- drift;
- reset;
- rollback;
- missing synchronization;
- monotonicity loss;
- suspend and wake.

Wall-clock time shall not be the sole proof of identity, authority, or remote failure.

## 53. Timer persistence

Timers that survive restart or hibernation shall preserve sufficient identity and validity to prevent:

- duplicate execution;
- stale directives;
- indefinite expiry extension;
- lost cancellation.

## 54. Mediation execution

Runtime mediators shall preserve the ACS-0004 distinction between:

- transport forwarding;
- semantic mediation;
- transformation;
- aggregation;
- facade operation.

A runtime optimization shall not hide a semantic mediator from provenance when its action affects meaning.

## 55. Payload handling

Payload handling shall preserve:

- payload-reference identity;
- authorization;
- size bounds;
- sensitivity;
- integrity;
- retention;
- cancellation;
- release;
- expiry.

A local shared-memory payload remains separately governed.

## 56. Payload lifetime

Payload lifetime shall not be inferred solely from:

- connection lifetime;
- queue lifetime;
- process lifetime;
- pointer lifetime;
- file lifetime.

The applicable payload or domain contract controls its semantic lifetime.

## 57. Runtime security enforcement

Runtime enforcement may implement:

- authentication checks;
- capability validation;
- session protection;
- revocation caches;
- channel binding;
- isolation;
- access control.

The runtime shall not create security authority beyond ACS-0007.

## 58. Runtime immune enforcement

Runtime systems may enforce ACS-0008-authorized:

- admission restrictions;
- attachment restrictions;
- rate reductions;
- mandatory mediation;
- suspension;
- quarantine;
- recovery profiles.

The runtime shall not infer containment authority from an immune alert alone.

## 59. Runtime and MEM boundary

### 59.1 Runtime carries memory operations

The runtime may schedule and transport MEM operations.

It does not define their semantic outcome.

### 59.2 Process state is not memory state

Process restart, host loss, queue loss, or connection failure does not automatically mean:

- memory loss;
- deletion;
- commitment failure;
- retrieval failure;
- recovery completion.

### 59.3 Runtime placement is not custody

Executing a memory service on one node does not by itself establish MEM custody for that node.

### 59.4 Runtime copy is not memory replica

Copying bytes for caching, transport, staging, or execution does not automatically create a valid MEM replica.

### 59.5 Runtime deletion is not memory deletion

Removing a cache, queue item, temporary file, or process-local representation does not delete logical memory.

### 59.6 Memory operations retain identity

MEM operation identity shall survive runtime retry, reconnect, migration, and process replacement where the operation remains semantically the same.

## 60. Runtime and IMM boundary

The future IMM series will define immune semantics.

The runtime may:

- expose bounded evidence;
- execute authorized restrictions;
- preserve recovery channels;
- report enforcement results.

It shall not:

- classify threats independently as architectural truth;
- invent immune authority;
- broaden observation scope;
- delete evidence outside MEM policy;
- make permanent containment decisions without authority.

## 61. Runtime and physical-node boundary

Runtime control of a process or service does not automatically grant authority to:

- reboot a host;
- erase storage;
- reconfigure firmware;
- power-cycle devices;
- destroy hardware;
- alter physical safety systems.

Physical-node lifecycle and control require separate authority.

## 62. Failure containment

Runtime failure containment should prevent one faulty:

- participant;
- port;
- queue;
- mediator;
- connection;
- device backend;
- plugin;
- transport;

from consuming or corrupting unrelated runtime state.

Containment mechanisms may include:

- process boundaries;
- memory protection;
- queue isolation;
- resource groups;
- sandboxes;
- capability boundaries;
- separate workers;
- watchdogs.

Implementation isolation does not replace semantic isolation.

## 63. Runtime fault states

The runtime shall distinguish where applicable:

- failed;
- degraded;
- unavailable;
- blocked;
- stale;
- conflicting;
- recovering;
- unsupported;
- unknown.

An exception, crash, timeout, or device error shall not always be collapsed into one generic failure.

## 64. Crash behavior

After a crash, the runtime shall determine or preserve uncertainty about:

- queued work;
- executing work;
- emitted signals;
- received signals;
- payload transfers;
- resource reservations;
- connections;
- attachments;
- domain operations.

Unknown completion shall remain indeterminate.

## 65. Orphan detection

The runtime shall detect or bound orphaned:

- tasks;
- queues;
- leases;
- reservations;
- payloads;
- sessions;
- bindings;
- migration state;
- temporary credentials;
- attachment caches.

Orphans shall not persist indefinitely because ownership is uncertain.

## 66. Stale-instance suppression

Stale runtime instances shall not:

- advertise current endpoint readiness;
- accept current attachments;
- issue current directives;
- overwrite current health state;
- reclaim current leases;
- close current connections;
- emit current-generation lifecycle events.

## 67. Observability

Authorized runtime observability should make it possible to determine:

- participant instances;
- endpoint implementations;
- port readiness;
- connection generations;
- bindings;
- attachment enforcement state;
- queue pressure;
- resource use;
- leases;
- health;
- degraded state;
- migration;
- recovery;
- known conflict;
- unsupported capability.

## 68. Observability is bounded

Runtime telemetry may reveal:

- topology;
- relationships;
- cognitive activity;
- memory activity;
- security state;
- immune state;
- operator actions.

Access shall therefore remain controlled.

## 69. Metrics

Metrics shall preserve their scope and measurement boundaries.

A global average shall not conceal:

- one starved port;
- one failed queue;
- one overloaded participant;
- one unavailable protected path;
- one stale replica.

## 70. Logs

Logs are evidence.

They are not automatically authoritative state.

Logs shall not contain:

- private keys;
- reusable credentials;
- unnecessary payload content;
- unrestricted cognitive content.

## 71. Audit

Sensitive runtime actions should preserve bounded evidence concerning:

- placement;
- migration;
- execution lease;
- lifecycle transition;
- capability enforcement;
- immune restriction;
- operator action;
- rollback;
- recovery;
- protected-capacity use.

## 72. Configuration privacy

Public runtime profiles may disclose supported capabilities and limits.

They need not disclose:

- production topology;
- exact internal capacity;
- private participant catalogs;
- credentials;
- protected routes;
- private scheduling policy.

## 73. Public implementation requirements

A public implementation claiming support for ACS-0009 shall document:

- participant-to-runtime mapping;
- endpoint implementation model;
- port implementation model;
- connection and binding model;
- attachment enforcement;
- signal dispatch;
- local fast-path behavior;
- queue limits;
- scheduling model;
- execution leases;
- resource enforcement;
- protected capacity;
- placement behavior;
- hardware-backend integration;
- replication;
- failover;
- migration;
- restart;
- recovery;
- hibernation;
- shutdown;
- headless-operation support;
- configuration and profile model;
- upgrade and rollback;
- health and readiness model;
- partition behavior;
- clock behavior;
- payload handling;
- security enforcement;
- immune enforcement;
- MEM boundary;
- observability;
- unsupported features;
- known limitations.

## 74. Conformance expectations

Conformance evidence should demonstrate that:

1. participant identity is independent of process identity;
2. endpoint identity survives ordinary process or binding replacement;
3. queue identity is not treated as port identity;
4. socket state is not treated as complete connection state;
5. local fast paths preserve remote-equivalent authority and validation;
6. runtime placement does not change ownership;
7. scheduler priority does not create semantic authority;
8. available hardware does not create admission;
9. hardware discovery does not create semantic registration automatically;
10. unsupported hardware remains explicit;
11. queues remain bounded under stalled consumers;
12. one source cannot consume all shared queue capacity;
13. backpressure prevents unbounded accumulation;
14. protected capacity remains available during overload;
15. runtime overrun does not silently consume unrelated reserves;
16. cancellation preserves explicit outcome;
17. restart does not restore stale authority;
18. crash recovery preserves indeterminate operation state;
19. failover does not create two exclusive authorities;
20. migration does not duplicate semantic work;
21. rollback does not resurrect revoked or deleted state;
22. hibernation and wake preserve identity without resuming stale grants;
23. process liveness remains distinct from endpoint and domain readiness;
24. partition does not duplicate exclusive leases or authority;
25. physical-device loss does not automatically destroy logical identity;
26. runtime caching does not create MEM replicas;
27. runtime deletion does not become MEM deletion;
28. immune enforcement requires authorized ACS action;
29. runtime telemetry remains privacy-bounded;
30. the core runtime operates without GUI or desktop dependencies.

## 75. Prohibited interpretations

This specification shall not be interpreted to mean that:

- one process equals one participant;
- one socket equals one connection;
- one queue equals one port;
- process restart creates new logical ownership;
- hosting grants endpoint ownership;
- local calls bypass admission;
- scheduler priority creates authority;
- available resources create permission;
- device presence creates semantic capability;
- container isolation replaces ACS security;
- service-manager control grants unrestricted lifecycle authority;
- message delivery proves domain completion;
- process liveness proves port readiness;
- runtime migration transfers memory custody;
- byte copying creates a MEM replica;
- temporary file deletion deletes logical memory;
- an immune alert grants containment authority;
- GUI services are required for core operation;
- public conformance requires disclosure of production topology or scheduling policy.

## 76. Initial architectural commitments

ACS-0009 establishes that:

1. runtime mechanisms implement but do not define ACS objects;
2. participant identity remains separate from process and host identity;
3. endpoint ownership remains separate from hosting;
4. ports remain semantic contracts rather than queues or sockets;
5. connections remain separate from bindings;
6. attachments are enforced across local and remote paths;
7. local fast paths preserve equivalent authority and semantics;
8. scheduling does not create admission or authority;
9. execution leases do not create ownership;
10. every runtime queue remains bounded;
11. backpressure and load shedding preserve semantic requirements;
12. protected capacity remains enforceable at runtime;
13. placement does not silently alter identity, ownership, contracts, or authority;
14. hardware discovery remains separate from logical registration;
15. heterogeneous execution does not silently change operation meaning;
16. replication and failover preserve explicit authority scopes;
17. migration prevents duplicate semantic execution;
18. restart invalidates or reconciles process-local authority;
19. recovery requires more than process liveness;
20. hibernation preserves logical identity without preserving stale runtime authority;
21. core ACS operation remains headless and GUI-independent;
22. configuration and observed capability remain distinct;
23. upgrades and rollbacks preserve current authority and lifecycle state;
24. health, liveness, endpoint readiness, and domain readiness remain distinct;
25. partition does not create duplicate exclusive runtime authority;
26. runtime enforcement remains subordinate to ACS-0006, ACS-0007, and ACS-0008;
27. runtime implementation does not redefine MEM semantics;
28. runtime implementation does not redefine future IMM semantics;
29. failure and uncertainty remain explicit;
30. public runtimes may conform without exposing production topology or private policy.

## 77. Open questions

The following questions remain for implementation profiles or future specifications:

- Which runtime object mappings are mandatory for baseline conformance?
- Should runtime generations use a standardized public representation?
- Which participant replication profiles should be standardized?
- Which local fast-path equivalence tests are mandatory?
- Which queue dimensions must every runtime expose?
- Which protected-capacity mechanisms must be validated?
- How should CPU, GPU, accelerator, network, storage, and energy leases compose?
- Which scheduler behaviors require standardized semantics?
- How should execution preemption interact with directive operations?
- Which runtime state must survive total process loss?
- Which runtime state must survive host loss?
- How should active-active endpoint implementations reconcile budgets?
- Which migration scopes should baseline runtimes support?
- When may a connection generation remain unchanged across migration?
- Which rollback operations require lifecycle or operator concurrence?
- How should hibernation preserve pending operation identity?
- Which headless-management interfaces should be standardized publicly?
- How should runtime profiles report unavailable optional hardware?
- Which hardware capability records belong in public ACS profiles?
- How should device-sharing models prevent duplicate ownership?
- Which health metrics are mandatory?
- How should clock uncertainty affect deadlines and leases?
- Which runtime failures should become immune evidence?
- Which runtime evidence may IMM inspect without payload access?
- Which runtime actions require durable audit evidence?
- How should minimal nodes provide protected security and recovery capacity?
- Which public conformance levels should ACS adopt?
- Should ACS define a separate conformance specification beyond ACS-0009?
- How should runtime and MEM conformance claims reference each other?
- How should production orchestration remain private while public runtimes interoperate?
- Which implementation profiles should cover local-only, single-node distributed, regional, and large heterogeneous deployments?

These questions do not permit runtimes to redefine ACS semantics or weaken existing authority and safety boundaries.

## 78. Closing principle

> **Node may change processes, hosts, devices, transports, queues, schedulers, and execution placement without changing who its participants are, what their ports mean, which authority they possess, or what their operations have actually accomplished.**

The runtime provides motion.

ACS provides meaning and boundaries.

MEM provides memory semantics.

IMM will provide immune semantics.

Security provides identity and authority evidence.

Health provides operational evidence.

None may silently become another merely because one implementation hosts them together.

## Revision history

### Version 0.1 — 2026-07-16

- Established the public ACS runtime-integration architecture.
- Separated runtime objects from participants, endpoints, ports, connections, bindings, attachments, signals, and domain operations.
- Defined participant representation, endpoint and port implementation, connection state, binding behavior, attachment enforcement, dispatch, and local fast paths.
- Established bounded queues, scheduling, execution leases, resource enforcement, preemption, cancellation, backpressure, load shedding, and protected capacity.
- Defined placement, hardware-backend integration, heterogeneous execution, replication, failover, migration, restart, recovery, hibernation, and shutdown.
- Required minimal headless operation without GUI dependencies.
- Defined configuration, system profiles, discovery, versioning, upgrade, rolling upgrade, rollback, health, readiness, degradation, partition, and clock behavior.
- Preserved security, immune, MEM, runtime, health, and physical-node boundaries.
- Established failure containment, orphan detection, stale-instance suppression, observability, privacy, audit, public implementation requirements, and conformance expectations.
