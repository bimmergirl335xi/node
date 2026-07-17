# MEM-0002: Memory Service Roles and Boundaries

| Field | Value |
|---|---|
| Specification | MEM-0002 |
| Title | Memory Service Roles and Boundaries |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-15 |
| Approval | Accepted as initial working draft |
| Depends on | MEM-0000, MEM-0001, ACS-0000, ACS-0001 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in role boundaries; role composition and reduced-operation behavior remain under review |

> **A memory-service role describes responsibility and authority, not a permanent process, machine, or topology position.**

## Architectural-intent notice

This specification defines the logical roles through which Node proposes, authorizes, validates, accepts, commits, retains, locates, retrieves, repairs, reconstructs, observes, and governs memory.

It defines:

- why each role exists;
- which responsibilities belong to it;
- which authority it may exercise;
- which authority it does not gain;
- how roles may be combined;
- how role assignment may migrate;
- how role availability must be represented;
- how Node degrades when required roles are unavailable.

It does not prescribe:

- one process per role;
- one physical node per role;
- one globally centralized memory service;
- one distributed deployment model;
- one consensus mechanism;
- ACS endpoints, ports, routes, or transports;
- storage-engine selection;
- database selection;
- exact replication or reconstruction policy;
- exact production capacity;
- cognitive memory-selection algorithms;
- non-public recall arbitration;
- production operator procedures.

A conforming implementation may combine several roles within one logical service or distribute one role across several participants.

The semantic and authority boundaries defined here must remain explicit regardless of physical composition.

## 1. Purpose

MEM-0002 defines the logical responsibilities required to operate a governed memory system without making any ordinary physical node irreplaceable.

Memory operation requires more than stored bytes.

A system may need to determine:

- who proposed state for storage;
- whether the proposal is admissible;
- whether the operation is authorized;
- whether the state is valid;
- whether a version entered authoritative history;
- where physical material is held;
- how memory can be located;
- how a retrieval result was assembled;
- whether a result is complete, current, or conflicting;
- who may alter retention or deletion state;
- how lost protection is repaired;
- how recovery is verified;
- whether current role coverage is sufficient for an operation.

If these responsibilities are left implicit, one convenient process or database may silently become:

- the owner of every memory;
- the sole authority for commitment;
- the only locator;
- the only deletion authority;
- the only source of truth;
- the only recovery path.

This specification separates those responsibilities so that implementations may evolve while preserving continuity, least privilege, honest availability, and recoverability.

## 2. Scope

This specification governs:

- memory-service role definitions;
- role responsibility;
- role authority;
- role assignment;
- role composition;
- role availability;
- role migration;
- role takeover;
- role coexistence;
- role conflict;
- reduced-operation behavior;
- boundaries between roles;
- boundaries between roles and physical deployment;
- boundaries between memory roles and ACS;
- boundaries between memory services and cognitive systems;
- evidence required to claim that a role is available.

This specification applies to roles supporting:

- storage proposal;
- admission;
- authorization;
- validation;
- commitment;
- physical custody;
- cataloging;
- location;
- retrieval;
- result assembly;
- provenance;
- retention;
- deletion;
- repair;
- reconstruction;
- observation;
- conformance evidence.

It applies regardless of whether an implementation is:

- local or distributed;
- single-process or multi-process;
- single-node or multi-node;
- replicated or non-replicated;
- volatile or persistent;
- centralized, federated, or decentralized;
- homogeneous or heterogeneous.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements.

The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification.

The word **may** describes permitted behavior.

A participant does not acquire a role merely because its implementation performs similar work.

Role assignment and authority must be explicit.

## 4. Role model

### 4.1 Memory-service role

A memory-service role is a defined set of responsibilities and bounded authority associated with memory operation.

A role answers:

> What memory responsibility is this participant currently permitted and expected to perform?

A role does not inherently answer:

- which physical machine performs it;
- which process performs it;
- which ACS endpoint exposes it;
- which storage engine supports it;
- whether it is centralized or distributed;
- whether one or several participants exercise it;
- whether it is currently available;
- whether it is trusted for unrelated operations.

### 4.2 Role instance

A role instance is an identified logical participant currently assigned to exercise some or all of a role within a declared scope.

A role instance may be:

- active;
- degraded;
- suspended;
- unavailable;
- recovering;
- stale;
- conflicting;
- unknown.

A physical process may host several role instances.

A logical role instance may migrate between physical processes or nodes.

### 4.3 Role assignment

A role assignment is an explicit, scoped, and revocable grant permitting an identified participant to exercise defined role responsibilities.

A role assignment must identify, directly or through governed context:

- the assigned participant;
- the role;
- the permitted operation scope;
- the memory scope;
- the authority granted;
- relevant limits;
- relevant policy version;
- activation state;
- expiration or revocation conditions where applicable.

Discovery, connectivity, authentication, physical possession, or software configuration alone does not constitute role assignment.

### 4.4 Responsibility

A responsibility is work a role is expected to perform.

A responsibility does not independently grant authority.

For example, a custodian may be responsible for retaining memory material while lacking authority to disclose or delete it.

### 4.5 Authority

Authority is the permission to make or execute a decision that changes memory state, access, interpretation, lifecycle, or authoritative history.

Authority must be:

- explicit;
- bounded;
- operation-specific where required;
- memory-scoped where required;
- revocable;
- auditable;
- separable from physical possession.

### 4.6 Role capability

A role capability is a specific operation or decision that an assigned role instance is permitted to request or exercise.

Examples include:

- evaluate a storage proposal;
- commit a memory version;
- read a particular memory class;
- validate integrity;
- report location;
- change retention state;
- authorize deletion;
- schedule repair.

Role name alone does not grant every capability commonly associated with that role.

### 4.7 Role coverage

Role coverage describes whether the responsibilities and authorities required for a declared memory operation are currently available.

An implementation need not expose every role as a separate service.

It must be able to identify how all required responsibilities are covered.

### 4.8 Role availability

Role availability describes whether an authorized role instance can currently perform its declared responsibilities under the required:

- scope;
- consistency;
- security;
- freshness;
- dependency;
- resource;
- health;
- failure-domain conditions.

Role availability shall not be inferred solely from:

- network reachability;
- process liveness;
- endpoint discovery;
- successful authentication;
- possession of memory material;
- a recent health message.

### 4.9 Role composition

Role composition is the assignment of several roles to one logical participant or service.

Composition may reduce deployment cost and latency.

It does not erase the semantic distinction between the composed roles.

### 4.10 Role distribution

Role distribution is the assignment of one role across several participants.

Distribution may support:

- scale;
- locality;
- redundancy;
- independent validation;
- failure tolerance;
- administrative separation.

The existence of several role instances does not by itself define how they coordinate or which outcome is authoritative.

## 5. Separation model

### 5.1 Semantic separation is mandatory

The roles defined in this specification remain semantically distinct even when one implementation component exercises several of them.

A combined service must still distinguish, where applicable:

- proposal from admission;
- admission from commitment;
- custody from authority;
- location from validation;
- retrieval from result interpretation;
- validation from authorization;
- repair from lifecycle authority;
- observation from mutation.

### 5.2 Logical separation is mandatory

Each exercised role must have explicit responsibility, authority, scope, and observable outcome.

A single process must not hide several memory decisions behind one ambiguous operation merely because the roles are co-located.

### 5.3 Physical separation is policy-dependent

This specification does not require every role to occupy a separate:

- process;
- node;
- storage device;
- network;
- administrative domain.

Later policies may require physical or failure-domain separation for particular memory classes or authority combinations.

### 5.4 Authority separation may be stronger than role separation

Two roles may be implemented together while still requiring independent authorization or evidence for particular actions.

For example:

- a custodian may also retrieve data but still require separate disclosure authorization;
- a validator may also participate in commitment but must preserve validation evidence;
- a lifecycle service may execute deletion only after receiving separately governed deletion authority.

### 5.5 Composition must not create implicit super-authority

Combining roles shall not create authority beyond the union of their explicit assignments.

A service exercising several roles does not become a universal memory owner.

### 5.6 No universal memory-owner role

This specification does not define one all-powerful **memory owner** role.

The following responsibilities may belong to different participants:

- creation;
- proposal;
- admission;
- commitment;
- custody;
- access authorization;
- retrieval;
- interpretation;
- retention;
- deletion;
- recovery.

Using the word *owner* in an implementation must not erase those distinctions.

### 5.7 Independent decision-making may be required by policy

Later specifications or deployment policy may require independent participants for safety-sensitive decisions.

Examples may include:

- deletion of continuity-critical memory;
- commitment under conflicting evidence;
- recovery after suspected compromise;
- key-authority restoration;
- acceptance of reconstructed memory;
- durability claims spanning independent failure domains.

MEM-0002 does not define exact quorum sizes or approval counts.

## 6. Universal role requirements

### 6.1 Roles are logical, not physical

A role shall not be permanently identified with one host, process, thread, database, storage device, or ACS connection.

### 6.2 Role assignment is explicit

A participant shall not acquire memory authority merely by:

- discovering a service;
- connecting to an endpoint;
- authenticating;
- receiving a payload;
- storing a copy;
- being configured on the same physical node.

### 6.3 Role names do not grant unrestricted authority

Classification as a custodian, validator, recovery service, or other memory role does not independently authorize every operation associated with memory.

### 6.4 Roles operate with least privilege

Each role instance shall receive only the authority required for its declared responsibilities and scope.

### 6.5 Role state is not memory state

A role may be unavailable while memory remains valid.

A role may be available while particular memory is:

- unavailable;
- stale;
- corrupted;
- conflicting;
- unauthorized;
- deleted;
- not found.

### 6.6 Connection state is not role availability

A connected role instance may lack:

- current authority;
- required metadata;
- valid keys;
- sufficient capacity;
- current indexes;
- required custodians;
- valid policy;
- integrity evidence.

A disconnected role instance may still have valid retained material that can be reconciled later.

### 6.7 Role failure is not proof of memory absence

Failure of a catalog, locator, retrieval provider, validator, or custodian shall not be reported as proof that memory does not exist.

### 6.8 Role availability is evidence-backed

A role shall be reported available only when sufficient evidence exists that it can perform its declared responsibilities.

### 6.9 Degraded and unknown role state is explicit

An implementation shall not silently represent a degraded, stale, conflicting, partially covered, or unknown role as fully available.

### 6.10 Role takeover is governed

A replacement participant shall not assume a role solely because the prior instance stopped responding.

Takeover must validate:

- identity;
- assignment;
- authority;
- current role state;
- relevant memory state;
- relevant policy;
- stale or conflicting decisions;
- unfinished operations.

### 6.11 Role migration preserves semantic continuity

Migrating a role to another physical host shall not inherently:

- change memory identity;
- reset operation identity;
- erase pending decisions;
- grant broader authority;
- invalidate committed history.

### 6.12 Rejoining instances are not automatically current

A former role instance returning after disconnection, restoration, or rollback must reconcile its role state before resuming authoritative operation.

### 6.13 Conflicting role authority is explicit

If multiple participants claim incompatible authority for the same role scope, the conflict shall remain explicit until safely resolved.

### 6.14 Role operations are bounded

Every role shall operate within explicit limits appropriate to its responsibilities.

Limits may include:

- queue depth;
- request rate;
- result volume;
- search scope;
- stored material;
- validation work;
- repair work;
- concurrent operations;
- retry behavior;
- dependency fan-out.

### 6.15 Roles do not silently expand authority during failure

Degraded operation, emergency recovery, partition, or reduced capacity shall not silently grant a role authority it did not previously possess.

### 6.16 Role observability does not grant mutation authority

A role permitted to inspect or report memory state does not thereby gain authority to modify it.

### 6.17 Role evidence remains auditable

Material decisions should preserve sufficient evidence to identify:

- which role acted;
- under which assignment;
- within which scope;
- using which relevant policy;
- with which outcome;
- under which degraded conditions.

### 6.18 Roles do not control physical-node disposition

No memory role independently acquires unrestricted authority to erase, quarantine, rebuild, disable, restart, or destroy a physical node.

## 7. Defined roles

The roles below define logical responsibilities.

They are not mandatory one-to-one services.

An implementation may combine roles, distribute roles, or omit roles for unsupported operations, provided the required boundaries remain explicit.

## 7.1 Memory proposer

### Definition

A memory proposer is a participant that submits identified state for possible admission into governed memory.

A proposer may be:

- a cognitive participant;
- a sensor-processing participant;
- an operational service;
- an immune service;
- an existing memory service;
- an authorized external participant.

### Responsibilities

A proposer is responsible for providing the information required by the storage-proposal contract.

This may include:

- proposed content or a governed reference;
- proposed logical relationship to existing memory;
- provenance;
- schema identity;
- sensitivity;
- requested retention;
- requested durability;
- operation identity;
- declared dependencies.

Exact proposal fields belong to MEM-0004.

### Authority boundary

A proposer does not inherently possess authority to:

- accept its own proposal;
- commit a version;
- declare durability;
- select physical placement;
- grant access;
- override retention policy;
- delete existing memory.

### Failure behavior

Failure of a proposer may prevent new proposals from being submitted.

It must not invalidate memory already committed by an authoritative process.

## 7.2 Memory requester and consumer

### Definition

A memory requester asks memory services to locate, retrieve, validate, or report memory.

A memory consumer receives or interprets an authorized retrieval result.

The same participant may exercise both responsibilities.

### Responsibilities

A requester is responsible for declaring the scope and requirements of its operation, including where applicable:

- recall criteria;
- logical identity;
- version constraints;
- consistency expectation;
- freshness expectation;
- completeness expectation;
- acceptable degraded outcomes;
- authorization context;
- resource limits.

A consumer is responsible for interpreting the result according to its declared state.

### Authority boundary

A requester or consumer does not inherently possess authority to:

- mutate retrieved memory;
- change retention;
- commit a new version;
- delete memory;
- reinterpret partial or stale results as complete and current;
- bypass access policy.

### Failure behavior

Failure of a requester may abandon an operation.

It must not silently convert an unfinished retrieval into not found or trigger uncontrolled retry.

## 7.3 Memory access authority

### Definition

A memory access authority evaluates whether an identified participant may perform a defined memory operation within a declared scope.

### Responsibilities

It may evaluate:

- participant identity;
- assigned capabilities;
- memory sensitivity;
- operation type;
- requested scope;
- relationship constraints;
- security state;
- revocation state;
- applicable memory policy.

### Authority boundary

Access authorization does not independently establish:

- content validity;
- memory existence;
- proposal acceptance;
- commitment;
- durability;
- retrieval completeness.

### Failure behavior

When required access authority is unavailable or indeterminate, protected memory operations must not silently proceed as authorized.

A denied or unevaluated request must not be reported as not found.

## 7.4 Admission evaluator

### Definition

An admission evaluator determines whether a storage proposal or requested memory operation is eligible to enter the next governed stage.

### Responsibilities

Admission evaluation may consider:

- proposal validity;
- schema support;
- provenance sufficiency;
- policy compatibility;
- resource budgets;
- requested durability;
- sensitivity;
- retention constraints;
- duplication;
- dependency availability;
- current system protection.

### Authority boundary

Admission does not independently mean:

- commitment;
- durability;
- completed storage;
- successful indexing;
- permanent retention.

An admission evaluator does not define non-public cognitive significance algorithms in this public specification.

### Failure behavior

If admission evaluation is unavailable, new proposals may be:

- rejected;
- deferred;
- retained as explicitly provisional;
- limited according to declared policy.

They must not be represented as committed merely because proposal data was received.

## 7.5 Commit authority

### Definition

A commit authority establishes that a memory version or lifecycle decision has entered authoritative memory history according to its declared consistency requirements.

A commit authority may be exercised by:

- one logical service;
- several coordinated services;
- a quorum;
- another explicitly governed authority model.

This specification does not select the model.

### Responsibilities

The commit authority is responsible for ensuring that commitment decisions are:

- semantically identified;
- ordered or related as required;
- bound to the correct logical memory;
- bound to the correct version;
- consistent with applicable authorization;
- supported by required validation;
- recorded durably enough for the declared commitment model.

### Authority boundary

Commit authority does not inherently grant:

- unrestricted content disclosure;
- deletion authority;
- physical-node authority;
- authority to claim durability beyond current evidence;
- authority to erase conflicting evidence.

### Failure behavior

When commit authority is unavailable:

- new proposals must not be falsely reported as committed;
- existing committed memory may remain retrievable;
- provisional or pending state must remain explicit;
- conflicting commit claims must not be silently resolved by arrival order.

## 7.6 Memory custodian

### Definition

A memory custodian retains or serves physical memory material under a governed assignment.

### Responsibilities

A custodian may be responsible for:

- storing memory representations;
- preserving assigned material;
- reporting integrity state;
- reporting availability;
- serving authorized reads;
- participating in replication;
- participating in reconstruction;
- honoring retention and deletion decisions;
- reporting resource pressure.

### Authority boundary

Custody does not inherently grant authority to:

- redefine logical identity;
- mutate committed content;
- disclose material;
- change retention;
- delete memory;
- claim authoritative completeness;
- claim that other copies do not exist.

### Failure behavior

Custodian failure may reduce:

- availability;
- locality;
- redundancy;
- durability;
- reconstruction capacity.

The resulting state must be represented honestly.

Loss of one custodian must not automatically be interpreted as loss of the logical memory.

## 7.7 Memory catalog and locator

### Definition

A memory catalog records governed metadata about logical memories.

A locator identifies known or eligible places from which memory material or further evidence may be requested.

One service may perform both roles.

### Responsibilities

A catalog or locator may maintain:

- logical identifiers;
- version relationships;
- schema references;
- provenance references;
- retention state;
- known custodians;
- representation information;
- availability observations;
- deletion markers;
- reconstruction metadata;
- index structures.

### Authority boundary

A catalog entry does not independently prove:

- content validity;
- current physical availability;
- successful authorization;
- retrieval completeness;
- current durability;
- absence when an entry is missing.

A locator does not independently authorize access to the location it reports.

### Failure behavior

If the catalog or locator is unavailable or incomplete:

- direct retrieval by known identity may still be possible;
- a complete not-found result may be impossible;
- location state must remain unknown or partial;
- stale entries must not be presented as confirmed current placement.

## 7.8 Retrieval provider

### Definition

A retrieval provider obtains and returns authorized memory material or governed references from one or more sources.

### Responsibilities

A retrieval provider may:

- fetch physical material;
- select an eligible representation;
- read from a custodian;
- stream a result;
- report source and version;
- expose validation evidence;
- report unavailable or partial material.

### Authority boundary

A retrieval provider does not inherently determine:

- whether the requester was authorized;
- whether a proposal should be committed;
- whether one representation is globally authoritative;
- whether an incomplete search proves absence;
- how the consumer should use the result cognitively.

### Failure behavior

Retrieval failure must distinguish relevant conditions, including:

- source unavailable;
- source stale;
- validation failed;
- access denied;
- operation deferred;
- result partial;
- scope incomplete;
- result indeterminate.

## 7.9 Retrieval-result coordinator

### Definition

A retrieval-result coordinator assembles, compares, and describes the semantic outcome of a retrieval operation.

A simple implementation may combine this role with a retrieval provider.

### Responsibilities

The coordinator may determine or report:

- which sources participated;
- which scope was searched;
- which versions were found;
- whether results agree;
- whether the result is complete;
- whether the result is stale;
- whether conflict remains;
- whether not found is justified;
- which evidence is missing.

### Authority boundary

A result coordinator shall not:

- hide unavailable participants;
- discard valid conflict without policy;
- strengthen the claimed consistency beyond the evidence obtained;
- report not found after an incomplete required search;
- convert an unauthorized result into absence.

### Failure behavior

If result coordination cannot complete, the operation must remain partial, failed, or indeterminate according to its contract.

It must not produce invented certainty.

## 7.10 Integrity and provenance validator

### Definition

An integrity and provenance validator evaluates whether memory material, metadata, lineage, schemas, and dependencies satisfy the validation requirements applicable to the operation.

### Responsibilities

Validation may include:

- integrity checks;
- schema checks;
- version checks;
- provenance checks;
- lineage checks;
- signature or authentication evidence;
- representation-equivalence checks;
- reconstruction validation;
- conflict evidence.

### Authority boundary

A validator does not inherently:

- authorize access;
- commit a version;
- select retention policy;
- delete memory;
- decide cognitive truth;
- control physical nodes.

Validation establishes whether defined evidence and structural requirements are satisfied.

It does not guarantee that every proposition represented by memory content is factually true.

### Failure behavior

When required validation cannot be completed, the material must remain:

- unverified;
- partial;
- unavailable for trusted use;
- otherwise explicitly degraded according to policy.

It must not be silently promoted to valid memory.

## 7.11 Lifecycle authority

### Definition

A lifecycle authority evaluates or authorizes governed changes to memory lifecycle state.

Lifecycle changes may include:

- retention modification;
- archival;
- expiration;
- deletion;
- tombstoning;
- reclamation eligibility;
- restoration eligibility;
- legal or policy holds where applicable.

### Responsibilities

A lifecycle authority is responsible for ensuring that a lifecycle decision is:

- authorized;
- bound to the correct logical memory and version scope;
- consistent with policy;
- represented as an explicit event;
- propagated sufficiently to prevent stale resurrection;
- distinguishable from physical loss.

### Authority boundary

A lifecycle authority does not inherently:

- possess every physical copy;
- perform physical sanitization;
- control storage hardware;
- commit unrelated memory versions;
- interpret cognitive significance.

### Failure behavior

If lifecycle authority is unavailable:

- existing governed lifecycle state remains in effect;
- new deletion or retention changes must not be guessed;
- custodians must not invent lifecycle decisions locally;
- resource pressure must not be relabeled as authorized forgetting.

## 7.12 Repair and reconstruction coordinator

### Definition

A repair and reconstruction coordinator identifies lost protection and coordinates restoration of required memory availability or durability.

### Responsibilities

It may:

- detect missing or damaged material;
- select valid sources;
- request replacement custody;
- coordinate reconstruction;
- validate reconstructed output;
- restore metadata protection;
- report under-protection;
- track repair progress;
- reconcile rejoining custodians.

### Authority boundary

A repair coordinator shall not independently:

- overwrite newer committed state;
- resurrect deleted memory;
- grant itself deletion authority;
- conceal conflicting sources;
- claim completed repair without evidence;
- destroy or quarantine physical nodes.

### Failure behavior

If repair coordination is unavailable:

- existing memory may remain valid;
- under-protection must remain explicit;
- new durability claims must reflect current evidence;
- uncontrolled repair storms must not be launched by individual custodians.

## 7.13 Memory observer and auditor

### Definition

A memory observer or auditor records and reports evidence about memory operation without inherently changing authoritative memory state.

### Responsibilities

It may observe:

- role assignments;
- operation outcomes;
- durability state;
- under-protection;
- access decisions;
- lifecycle events;
- repair progress;
- conflict;
- resource pressure;
- conformance evidence.

### Authority boundary

Observation does not grant authority to:

- mutate memory;
- authorize access;
- commit versions;
- alter retention;
- delete memory;
- take control of a physical node.

### Failure behavior

Loss of observation may reduce:

- auditability;
- diagnostics;
- conformance evidence;
- repair detection;
- operator awareness.

It must not silently create a false healthy state.

## 8. Role composition

### 8.1 Composition is permitted

One logical service may exercise several roles when:

- assignments are explicit;
- authorities remain scoped;
- outcomes remain distinguishable;
- failures remain observable;
- required independent evidence is preserved;
- policy does not require separation.

### 8.2 Composition does not merge semantics

A service acting as proposer, admission evaluator, custodian, and retrieval provider must still represent:

- proposal;
- admission;
- commitment;
- custody;
- retrieval

as distinct semantic events or states where required.

### 8.3 Self-approval is not implicit

A participant exercising both proposer and admission or commit roles does not automatically gain permission to approve every proposal it originates.

Any allowed self-approval must be explicit, scoped, and auditable.

### 8.4 Custody and deletion require explicit separation of authority

A custodian that also executes deletion must verify separately governed lifecycle authority.

Possession of a copy does not authorize its deletion.

### 8.5 Catalog and result coordination require scope honesty

A service acting as both locator and result coordinator must not treat the limits of its own catalog as the limits of memory existence unless the operation contract defines that catalog as authoritative for the requested scope.

### 8.6 Validation and commitment preserve evidence

A service acting as both validator and commit authority must preserve enough evidence to distinguish:

- what was validated;
- which requirements passed;
- which requirements were waived or unavailable;
- what was committed.

### 8.7 Repair and lifecycle authority must prevent resurrection

A service exercising both roles must still reconcile:

- current committed versions;
- deletion markers;
- retention state;
- stale sources;
- reconstruction evidence

before restoring material.

## 9. Role distribution and coordination

### 9.1 Several instances may exercise one role

A role may be distributed for:

- scale;
- redundancy;
- locality;
- failure tolerance;
- independent verification;
- administrative separation.

### 9.2 Distribution does not define authority resolution

The presence of several role instances does not specify:

- leader election;
- quorum size;
- conflict resolution;
- commit ordering;
- load balancing;
- shard allocation.

Those mechanisms belong to later specifications or implementation profiles.

### 9.3 Distributed roles must report scope

Each role instance must make its scope clear enough to avoid implying global authority when it holds only:

- a shard;
- a region;
- a memory class;
- a tenant;
- a version range;
- a locality;
- a partial catalog;
- a restricted capability.

### 9.4 Local success is not global success

Success by one role instance shall not be presented as global completion when additional required participants or scopes remain outstanding.

### 9.5 Conflicting instances remain explicit

When distributed instances report incompatible decisions or state, the conflict must remain visible until resolved by the applicable authority model.

## 10. Role availability and evidence

A role should be reported **available** only when sufficient evidence exists that an assigned instance can perform its declared responsibilities.

Relevant evidence may include:

1. authenticated participant identity;
2. valid role assignment;
3. unrevoked authority;
4. supported operation type;
5. declared memory scope;
6. compatible policy or schema;
7. required metadata access;
8. required key authority;
9. required custodian or dependency availability;
10. sufficient resource budget;
11. sufficiently current state;
12. acceptable integrity state;
13. acceptable consistency state;
14. known degraded limitations;
15. ability to produce the required semantic outcome.

A role may be reported **degraded** when it can perform only a reduced, explicitly described subset of its responsibilities.

A role must be reported **unavailable** when required evidence establishes that it cannot perform the requested responsibility.

A role must remain **unknown** when evidence is insufficient to establish availability or unavailability.

A role may be **conflicting** when incompatible valid-looking role assignments or states exist.

## 11. Reduced-operation behavior

Node may continue operating with incomplete role coverage when safety, continuity, and semantic honesty can be preserved.

Reduced operation must be explicit.

### 11.1 Admission unavailable

When admission evaluation is unavailable:

- existing committed memory may remain retrievable;
- new proposals may be rejected, deferred, or explicitly provisional;
- received data must not be represented as committed solely because it was buffered.

### 11.2 Commit authority unavailable

When commitment cannot be established:

- existing committed history remains authoritative;
- new versions remain pending, provisional, rejected, or indeterminate;
- no local participant may silently promote its preferred state.

### 11.3 Custody degraded

When required custodians are unavailable:

- memory may become unavailable or under-protected;
- remaining copies must not be presented as satisfying unavailable failure domains;
- repair may be scheduled when authority and resources permit.

### 11.4 Catalog or locator unavailable

When location evidence is incomplete:

- retrieval by directly known identity or location may remain possible;
- broad recall may become partial or unavailable;
- not found must not be claimed unless the required search scope completed.

### 11.5 Validation unavailable

When required validation cannot be completed:

- results remain unverified or unavailable for trusted use;
- degraded validation must be disclosed;
- content must not be promoted to fully trusted state.

### 11.6 Access authority unavailable

Protected memory operations must fail closed, defer, or remain indeterminate according to policy.

Authorization uncertainty must not become implied permission.

### 11.7 Lifecycle authority unavailable

Existing retention and deletion state remains in force.

Custodians must not invent new lifecycle decisions.

### 11.8 Repair coordination unavailable

Memory may remain usable while under-protected.

The system must not continue claiming protection that it can no longer verify or restore.

### 11.9 Observer unavailable

Loss of monitoring must be represented as reduced observability.

Absence of reports must not become evidence of health.

## 12. Migration, takeover, and rejoining

### 12.1 Migration

A role assignment may migrate between logical participants or physical hosts.

Migration must preserve:

- role scope;
- granted authority;
- pending-operation identity;
- relevant decision history;
- relevant policy state;
- explicit degraded conditions.

### 12.2 Takeover

A takeover occurs when a replacement participant assumes responsibility after loss, suspension, or planned handoff of a previous role instance.

Takeover must not be based solely on timeout.

It must reconcile authoritative state sufficiently for the role being assumed.

### 12.3 Rejoining

A rejoining role instance must present and validate:

- identity;
- assignment generation or equivalent authority state;
- local operation history;
- locally retained memory state;
- relevant lifecycle state;
- relevant policy version;
- known disconnection interval.

### 12.4 Stale authority

A role assignment that was valid before disconnection may be stale, expired, superseded, or revoked when the participant returns.

Past authority does not guarantee current authority.

### 12.5 Pending operations

Migration or takeover must account for operations that may have been:

- received but not admitted;
- admitted but not committed;
- committed but not acknowledged;
- partially retrieved;
- partially repaired;
- authorized but not executed;
- executed but not durably recorded.

Such operations may be indeterminate and require reconciliation.

### 12.6 No silent split authority

If both old and replacement instances may have exercised authority, the conflict must remain explicit until the applicable consistency and recovery rules establish a safe state.

## 13. Boundary with ACS

Memory roles may participate in ACS relationships and may expose memory operations through ACS endpoints and typed ports.

ACS remains responsible for:

- relationships;
- connections;
- endpoint identity;
- port semantics;
- signals;
- payload references;
- admission to communication;
- mediation;
- routing;
- secure sessions;
- transport behavior;
- communication backpressure.

MEM remains responsible for:

- memory role assignment;
- memory-operation authority;
- memory-specific admission;
- commitment;
- custody;
- retrieval semantics;
- validation;
- retention;
- recovery;
- role availability.

The following rules apply:

1. ACS discovery does not assign a memory role.
2. ACS authentication does not grant memory authority.
3. An ACS relationship does not become memory custody.
4. A connected endpoint is not proof that a role is available.
5. Loss of a connection does not revoke or destroy logical memory unless separately governed.
6. Communication backpressure does not define memory admission.
7. ACS retry does not create a new semantic memory operation.
8. A payload reference does not prove role assignment or memory validity.
9. MEM does not redefine routing, attachment, session, or transport mechanics.
10. ACS does not define storage, commitment, retrieval, or retention meaning.

## 14. Boundary with cognitive systems

Cognitive systems may:

- generate memory proposals;
- request recall;
- interpret retrieval results;
- assign cognitive significance;
- decide how recalled state influences reasoning;
- use non-public selection, salience, consolidation, or learning policy.

Public memory roles do not define those cognitive algorithms.

A memory service may evaluate whether a proposal satisfies public memory contracts without deciding the non-public cognitive value of the experience represented.

A retrieval provider may return memory without deciding whether the consumer should believe, prioritize, or act upon it.

A lifecycle authority may enforce explicit retention policy without publicly exposing non-public cognitive heuristics used to propose that policy.

## 15. Boundary with runtime, security, health, and immune systems

### 15.1 Runtime

Runtime systems may place, schedule, restart, and migrate memory-role implementations.

Runtime placement does not independently grant role authority.

### 15.2 Security

Security systems provide:

- participant identity;
- authentication;
- cryptographic protection;
- key custody;
- revocation primitives;
- security evidence.

Memory roles consume those facilities while preserving memory-specific authorization boundaries.

### 15.3 Health

Health systems may report:

- role-instance liveness;
- capacity;
- resource pressure;
- dependency failure;
- storage health;
- process health.

Health does not independently establish semantic role correctness.

### 15.4 Immune systems

Immune systems may observe and evaluate:

- inconsistent role claims;
- suspicious access;
- unexplained divergence;
- abnormal deletion;
- corruption;
- failed validation;
- stale rejoining behavior.

Memory roles may provide evidence and request evaluation.

They do not acquire unrestricted immune or physical-node authority.

## 16. Prohibited interpretations

This specification shall not be interpreted to mean that:

- every role requires a separate process;
- every role requires a separate physical node;
- every role must always be available;
- one service may never exercise several roles;
- one role may never have several instances;
- centralized coordination is forbidden;
- centralized coordination may become an unacknowledged single point of continuity;
- a role name grants every related capability;
- a custodian owns the memory it stores;
- a catalog is the memory;
- a locator authorizes access;
- a validator decides cognitive truth;
- a proposer may always approve its own proposal;
- a commit authority may claim unsupported durability;
- a repair service may resurrect deleted state;
- a lifecycle authority necessarily performs physical erasure;
- a requester may treat partial results as complete;
- a connected service is necessarily available;
- an unavailable service proves memory absence;
- an observer may mutate state merely because it detects a problem;
- public role contracts require disclosure of non-public cognitive policy.

## 17. Open questions

The following questions remain for later specifications:

- Which role coverage is mandatory for the smallest conforming Node deployment?
- Which roles may be absent for purely transient memory?
- Which authority combinations require independent participants?
- Which roles require failure-domain separation for continuity-critical memory?
- How should role assignments be versioned?
- How should role-assignment revocation propagate during partition?
- What evidence is sufficient for safe role takeover?
- How should pending operations be reconciled during migration?
- Which role is responsible for declaring an operation indeterminate?
- How should distributed commit authority expose its scope?
- How should catalogs describe incomplete coverage?
- How should retrieval-result coordinators prove adequate search scope?
- Which validation responsibilities must be independent of custody?
- How should under-protection affect admission decisions?
- Which lifecycle decisions require multiple authorities?
- How should observer evidence be retained without becoming authoritative memory truth?
- What minimum role coverage is required for recovery-only operation?
- How should role composition be represented in conformance reports?
- Which reduced-operation modes may continue autonomous cognitive activity?
- How should role availability be exposed without duplicating ACS health semantics?

These questions may be resolved by later MEM specifications without weakening the role and authority boundaries established here.

## 18. Closing principle

> **No participant becomes the memory of Node merely because it stores, locates, validates, retrieves, or governs part of that memory.**

Memory continuity depends on responsibilities that can move, divide, recover, and be independently verified.

Roles may share machines.

They may share processes.

They may cooperate through ACS.

Their meanings and authorities must remain distinct.

## Revision history

### Version 0.1 — 2026-07-15

- Replaced the planned MEM-0002 stub with the first normative working draft.
- Defined the logical role, role-instance, assignment, authority, responsibility, coverage, composition, distribution, and availability models.
- Established mandatory semantic and logical separation without requiring one process or physical node per role.
- Defined thirteen initial memory roles.
- Separated proposal, access authorization, admission, commitment, custody, location, retrieval, result coordination, validation, lifecycle, repair, and observation.
- Defined role composition and distributed-role boundaries.
- Established evidence-backed role availability and explicit degraded, unknown, and conflicting states.
- Defined reduced-operation behavior when individual roles are unavailable.
- Established migration, takeover, rejoining, and stale-authority requirements.
- Preserved the boundary between MEM, ACS, cognitive systems, runtime, security, health, and immune systems.
