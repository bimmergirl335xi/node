# MEM-0004: Memory Operation Contracts

| Field | Value |
|---|---|
| Specification | MEM-0004 |
| Title | Memory Operation Contracts |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Accepted as initial working draft |
| Depends on | MEM-0000, MEM-0001, MEM-0002, MEM-0003 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in operation identity and outcome separation; compound-operation and cancellation details remain under review |

> **Transport delivery is only communication progress; memory acceptance, commitment, retrieval, mutation, and deletion require explicit semantic outcomes.**

## Architectural-intent notice

This specification defines the public semantic contracts governing memory operations within Node.

It defines:

- operation identity;
- request identity;
- delivery and attempt identity;
- operation scope;
- preconditions;
- requested guarantees;
- progress;
- semantic outcomes;
- duplicate handling;
- idempotency;
- retry;
- cancellation;
- timeout;
- indeterminate state;
- storage proposals;
- validation;
- commitment;
- recall;
- retrieval;
- mutation;
- retention changes;
- deletion;
- repair and reconstruction requests;
- operation-status reconciliation.

It does not define:

- ACS routing;
- ACS relationships;
- endpoint or port mechanics;
- transport retransmission;
- packet or frame formats;
- serialization;
- concrete request encodings;
- database transactions;
- consensus protocols;
- production timeout values;
- non-public recall arbitration;
- cognitive memory-selection policy;
- exact replication or repair algorithms;
- exact lifecycle policy.

An implementation may use synchronous calls, asynchronous messages, queues, journals, local function calls, remote services, or another permitted mechanism.

The semantic contract must remain the same regardless of delivery mechanism.

## 1. Purpose

Memory operations change or reveal the state of a distributed cognitive organism.

Ambiguous operation behavior can cause:

- duplicate memories;
- repeated deletion;
- unintentional mutation;
- false commitment;
- false durability claims;
- false not-found results;
- lost updates;
- stale overwrites;
- conflicting versions;
- incomplete retrieval presented as complete;
- cancellation presented as successful when work continued;
- timeout interpreted as failure even though commitment occurred;
- replayed requests acquiring new authority;
- transport acknowledgment being mistaken for memory acceptance.

MEM-0004 establishes operation semantics before low-level interfaces are implemented.

Every conforming memory operation must make it possible to determine:

1. what semantic action was requested;
2. who requested it;
3. under which authority;
4. which memory scope it affects;
5. which operation identity it carries;
6. which preconditions apply;
7. which guarantees were requested;
8. which work was admitted;
9. which semantic outcome occurred;
10. whether the outcome is final;
11. which uncertainty remains;
12. whether retry is safe;
13. whether reconciliation is required.

## 2. Scope

This specification governs contracts for:

- storage proposal;
- identity reservation;
- validation;
- commitment;
- recall;
- retrieval;
- version mutation;
- correction;
- retention change;
- archival request;
- deletion;
- restoration request;
- repair;
- reconstruction;
- migration request;
- operation cancellation;
- operation-status inquiry;
- duplicate reconciliation.

It applies whether an operation:

- is local or remote;
- completes immediately or asynchronously;
- affects one memory or a bounded set;
- uses one role instance or several;
- crosses one or several failure domains;
- returns memory content or only metadata;
- is read-only or mutating;
- completes under full or degraded role coverage.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements.

The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification.

The word **may** describes permitted behavior.

An implementation does not conform merely because it exposes methods whose names resemble the operations in this specification.

Its observable semantics must preserve the required distinctions.

## 4. Core operation model

### 4.1 Memory operation

A memory operation is an identified semantic request, decision, transition, inquiry, or result governed by MEM.

A memory operation is not identical to:

- one network message;
- one ACS signal;
- one delivery attempt;
- one process invocation;
- one thread;
- one queue entry;
- one database transaction;
- one storage write;
- one response packet.

One semantic operation may require several deliveries, participants, transactions, or process executions.

### 4.2 Operation type

An operation type identifies the semantic action being requested.

Examples include:

- propose storage;
- validate memory;
- commit version;
- recall memory;
- retrieve representation;
- propose mutation;
- change retention;
- delete memory;
- repair protection;
- reconstruct representation;
- query operation status;
- request cancellation.

Operation type must not be inferred solely from the endpoint, transport route, or payload shape.

### 4.3 Operation identity

An operation identity identifies one semantic memory operation.

It must remain stable across:

- retransmission;
- reconnect;
- route change;
- process restart where supported;
- participant failover;
- retry after timeout;
- duplicate delivery;
- status reconciliation.

A repeated request carrying the same operation identity must be treated as the same semantic operation unless the request is invalid or conflicting.

### 4.4 Attempt identity

An attempt identity distinguishes one execution or submission attempt from another.

A retry may reuse the operation identity while creating a new attempt identity.

Attempt identity may support:

- diagnostics;
- latency measurement;
- delivery tracing;
- resource accounting;
- failure analysis.

Attempt identity does not create a new semantic memory operation.

### 4.5 Delivery identity

A delivery identity identifies one communication delivery or transmission instance.

Several deliveries may belong to one attempt.

Several attempts may belong to one operation.

Delivery identity belongs to communication and observability mechanics.

It must not become semantic operation identity.

### 4.6 Requester

The requester is the identified participant asking that the operation be evaluated or performed.

The requester may differ from:

- the proposer of the underlying memory;
- the participant that physically delivered the request;
- the authority that approved the operation;
- the consumer of the result;
- the participant that ultimately performs the operation.

### 4.7 Operation authority

Operation authority describes the permission under which the operation may proceed.

Authority may be supplied through:

- role assignment;
- scoped capability;
- policy;
- separately governed approval;
- a combination of evidence.

Successful authentication does not independently establish operation authority.

### 4.8 Subject

The operation subject identifies the logical memory, version, representation, copy, query scope, lifecycle record, or operation to which the request applies.

Subject identity must use the appropriate identity layer defined by MEM-0003.

### 4.9 Operation scope

Operation scope defines the bounded extent of the request.

Scope may include:

- one logical memory;
- one version;
- one representation;
- one physical copy;
- one memory class;
- one namespace;
- a bounded query;
- a bounded set of custodians;
- one lifecycle transition;
- one repair target;
- one operation history.

Scope must not be silently expanded during processing.

### 4.10 Preconditions

A precondition is a state that must hold before an operation may produce its requested semantic effect.

Examples include:

- expected current version;
- expected lifecycle state;
- required authorization;
- required validation state;
- required schema support;
- required durability state;
- absence of an existing conflicting identity;
- presence of a deletion marker;
- availability of required dependencies.

### 4.11 Requested guarantees

A request may declare required guarantees such as:

- minimum validation;
- consistency expectation;
- freshness;
- completeness;
- durability target;
- retention policy;
- acceptable degraded state;
- maximum result size;
- maximum search scope;
- required failure-domain participation.

Requested guarantees do not become satisfied merely because they were requested.

The result must report what was actually established.

### 4.12 Operation progress

Operation progress describes non-final advancement of an operation.

Progress may include:

- received;
- authenticated;
- authorized;
- admitted;
- queued;
- validating;
- awaiting dependency;
- retrieving;
- reconstructing;
- committing;
- repairing;
- cancelling.

Progress does not independently establish the final semantic outcome.

### 4.13 Operation result

An operation result reports the semantic outcome established for an operation.

A result must identify:

- the operation identity;
- the operation type;
- the result authority;
- the outcome;
- finality;
- affected subject;
- achieved scope;
- unsatisfied scope;
- relevant evidence;
- remaining uncertainty;
- retry or reconciliation guidance where applicable.

### 4.14 Finality

Finality describes whether the reporting authority considers the semantic outcome complete under the applicable operation contract.

Finality may be:

- final;
- non-final;
- conditional;
- indeterminate;
- unknown.

A final result may still report:

- partial retrieval;
- rejection;
- conflict;
- not found;
- cancellation;
- failure.

Final does not mean successful.

### 4.15 Operation history

Operation history records enough governed state to reconcile:

- retries;
- duplicate delivery;
- timeout;
- cancellation;
- process restart;
- failover;
- indeterminate outcomes.

The required retention and durability of operation history depend on operation risk and memory policy.

## 5. Conceptual request and result records

These records define semantics, not a required wire or storage format.

### 5.1 Operation request

A memory operation request should make it possible to determine:

```text
MemoryOperationRequest
    operation_identity
    operation_type
    requester_identity
    requester_role_or_capability
    authority_context
    subject_identity_or_query_scope
    operation_parameters
    preconditions
    requested_guarantees
    resource_limits
    deadline_or_wait_preference
    cancellation_policy
    request_schema_identity
    proposal_or_source_provenance
```

### 5.2 Operation result

A memory operation result should make it possible to determine:

```text
MemoryOperationResult
    operation_identity
    operation_type
    result_authority
    outcome
    finality
    affected_subjects
    achieved_guarantees
    unsatisfied_guarantees
    completed_scope
    incomplete_scope
    resulting_identity_or_version
    validation_state
    consistency_state
    durability_state
    lifecycle_state
    evidence_references
    retry_guidance
    reconciliation_reference
```

### 5.3 Operation-status record

An operation-status record should make it possible to determine:

```text
MemoryOperationStatus
    operation_identity
    known_operation_type
    current_progress
    known_semantic_outcome
    finality
    responsible_role_instances
    pending_dependencies
    cancellation_state
    last_governed_transition
    reconciliation_state
```

Not every operation requires every field.

Equivalent information may be embedded, partitioned, journaled, or reconstructed.

## 6. Universal contract requirements

### 6.1 Operation type is explicit

Every request must identify its semantic operation type.

The operation type shall not depend solely on where the request was delivered.

### 6.2 Operation identity is mandatory for mutating operations

Every operation capable of changing:

- memory content;
- identity;
- version history;
- lifecycle state;
- retention;
- deletion state;
- custody;
- repair state;
- authoritative metadata

shall carry stable operation identity.

### 6.3 Retry reuses semantic identity

A retry of the same semantic intent shall reuse the original operation identity.

It may use a new:

- attempt identity;
- delivery identity;
- route;
- connection;
- role instance.

### 6.4 Changed intent requires new operation identity

A materially changed request shall use a new operation identity.

Material changes may include:

- different subject;
- different proposed content;
- different mutation;
- different deletion scope;
- different preconditions;
- different requested authority;
- different lifecycle effect.

### 6.5 Same identity with different intent is conflict

When the same operation identity is presented with materially different semantic content, the operation must be rejected or marked conflicting.

The implementation shall not choose one request silently.

### 6.6 Exactly-once transport is not required

MEM does not require exactly-once delivery.

It requires duplicate-safe semantic processing.

### 6.7 Operation authority is evaluated independently

Delivery through an authenticated or authorized ACS relationship does not automatically authorize the memory operation itself.

### 6.8 Preconditions are explicit and checked

A mutating operation shall identify required preconditions and report whether they were satisfied.

A failed precondition is not a transport failure.

### 6.9 Scope is bounded

Every operation must have finite, enforceable scope and resource limits appropriate to its type.

### 6.10 Results describe achieved guarantees

A result must not report requested guarantees as achieved unless evidence supports them.

### 6.11 Progress is not completion

A progress acknowledgment must not be presented as final semantic success.

### 6.12 Finality is explicit

A requester must not need to infer finality from connection closure, response timing, or absence of additional messages.

### 6.13 Partial completion is operation-specific

A contract must define whether partial completion is:

- permitted;
- prohibited;
- recoverable;
- independently committed;
- subject to compensation;
- reported as indeterminate.

### 6.14 Failure does not erase evidence

An operation failure should preserve enough evidence to support safe retry and reconciliation.

### 6.15 Operation outcome is not inferred from timeout

A timeout means the waiting participant did not receive an adequate outcome within its waiting conditions.

It does not prove:

- rejection;
- failure;
- cancellation;
- absence;
- non-commitment.

### 6.16 Indeterminate outcomes remain explicit

When the system cannot determine whether a semantic effect occurred, the operation must be marked indeterminate.

It shall not be silently retried as a new operation.

### 6.17 Results do not overstate certainty

A result must report:

- incomplete scope;
- missing participants;
- stale evidence;
- conflicting versions;
- unverified content;
- authorization-limited visibility;
- unavailable dependencies

where those conditions affect interpretation.

### 6.18 Operation evidence is auditable

Material operations should preserve enough evidence to identify:

- who requested the operation;
- which authority was used;
- which role performed it;
- which preconditions were evaluated;
- which result was established;
- which degraded conditions applied.

## 7. Outcome model

Implementations may use different enum names.

They must preserve the distinctions below where applicable.

### 7.1 Accepted

The request has been admitted into a governed stage.

Accepted does not independently mean:

- committed;
- durable;
- complete;
- retained indefinitely;
- retrievable.

### 7.2 Rejected

The operation was evaluated and not admitted or permitted.

A rejection should identify a safe reason category where disclosure is authorized.

Reasons may include:

- invalid request;
- unsupported operation;
- failed precondition;
- policy conflict;
- insufficient authority;
- resource policy;
- identity conflict;
- schema incompatibility.

### 7.3 Deferred

The operation is not currently admitted but may be reconsidered without changing its semantic intent.

Deferral may occur because of:

- temporary resource pressure;
- unavailable dependency;
- repair priority;
- missing role coverage;
- policy scheduling.

Deferred is not accepted.

### 7.4 Pending

The operation has entered governed processing but has no final semantic outcome.

### 7.5 Completed

The requested semantic operation completed according to its declared contract.

Completed must identify which guarantees and scope were achieved.

### 7.6 Committed

The requested version or lifecycle decision entered authoritative memory history.

Committed does not independently mean the durability target is satisfied.

### 7.7 Partial

A bounded portion of the operation completed and the incomplete portion is explicitly identified.

Partial is permissible only when the operation contract allows it.

### 7.8 Not found

The required search completed adequately within the declared scope and found no matching memory.

Not found shall not substitute for:

- unavailable;
- incomplete;
- unauthorized;
- timed out;
- failed;
- deferred;
- unindexed;
- unknown.

### 7.9 Unavailable

The operation could not be performed because a required role, dependency, representation, key, schema, or resource was unavailable.

### 7.10 Unauthorized

The operation lacked required authority or authorization.

Where policy requires concealment, an external response may be deliberately opaque.

Internally, the outcome must not be recorded as not found merely to conceal memory existence.

### 7.11 Invalid

The request or subject failed required structural, identity, schema, integrity, lineage, or policy validation.

### 7.12 Conflict

The operation encountered incompatible valid-looking state or authority that prevented one safe outcome.

### 7.13 Cancelled

The operation was successfully prevented from producing any further prohibited semantic effect according to its contract.

A cancellation request is not proof of cancellation.

### 7.14 Expired

The request exceeded an operation-validity condition before the semantic effect was established.

Expired does not prove that no prior effect occurred unless the contract guarantees that property.

### 7.15 Failed

The operation terminated without establishing its requested semantic result, and enough evidence exists to classify it as failed rather than indeterminate.

### 7.16 Indeterminate

The system cannot safely establish which semantic outcome occurred.

Indeterminate requires reconciliation.

### 7.17 Duplicate

The request was recognized as a repeat of an existing operation.

A duplicate outcome should reference the known operation state or result.

Duplicate does not create a second semantic effect.

### 7.18 Unknown

Available evidence is insufficient to classify the operation state.

## 8. Idempotency model

### 8.1 Semantic idempotency

An operation is semantically idempotent when repeating the same operation identity does not produce additional semantic effects beyond those of the original operation.

### 8.2 Read operations

Recall and retrieval are generally repeat-safe but not necessarily result-identical.

Results may change because of:

- newer committed versions;
- improved availability;
- changed authorization;
- completed repair;
- changed requested time;
- altered catalog coverage.

A repeated read must still preserve the original request scope and operation identity semantics.

### 8.3 Mutating operations

Mutation, commitment, retention change, deletion, and repair operations must be duplicate-safe by operation identity.

### 8.4 Identity creation

Retrying one identity-creation operation must not create several logical memories.

### 8.5 Version creation

Retrying one version-creation operation must not create several semantically duplicate committed versions.

### 8.6 Deletion

Retrying one deletion operation must not:

- broaden deletion scope;
- create repeated unrelated lifecycle events;
- erase newer state outside the original operation;
- treat missing physical copies as new deletion targets.

### 8.7 Repair

Retrying repair must not create uncontrolled copy growth or repeatedly overwrite valid current state.

### 8.8 Duplicate-result stability

Once an operation has a final semantic result, later duplicate processing should return or reference an equivalent final result unless:

- the result was invalidated;
- operation history is unavailable;
- conflict was discovered;
- policy requires a more conservative outcome.

### 8.9 Idempotency records require retention

Operation identity and outcome evidence must remain available long enough to prevent unsafe duplicate effects.

The retention period may vary by operation class and risk.

## 9. Storage-proposal contract

### 9.1 Purpose

A storage proposal requests that identified state be evaluated for admission into governed memory.

### 9.2 Required semantic inputs

A proposal should identify:

- operation identity;
- proposer identity;
- authority context;
- proposed content or governed reference;
- proposed logical identity or request for identity creation;
- proposed parent version or derivation sources;
- schema identity;
- provenance claims;
- sensitivity;
- requested retention;
- requested durability;
- resource estimates;
- required dependencies.

### 9.3 Proposal receipt

Receipt means only that the proposal became available for evaluation.

Receipt is not acceptance.

### 9.4 Proposal validation

The proposal may be evaluated for:

- structural validity;
- authority;
- identity collision;
- schema support;
- provenance sufficiency;
- resource policy;
- retention compatibility;
- durability feasibility;
- duplicate operation identity;
- conflict with existing state.

### 9.5 Proposal outcomes

A storage proposal may become:

- accepted;
- rejected;
- deferred;
- pending;
- conflicting;
- invalid;
- unauthorized;
- indeterminate.

### 9.6 Acceptance result

An acceptance result must identify:

- accepted scope;
- assigned or accepted logical identity;
- proposed or assigned version identity;
- remaining validation;
- whether commitment is pending;
- whether durability is pending;
- any reduced guarantees.

### 9.7 Acceptance is not commitment

An accepted proposal may still fail before commitment.

### 9.8 Commitment is not durability

A committed proposal may temporarily be under-protected.

### 9.9 Rejected proposals do not become memory by receipt

An implementation may retain diagnostic or security evidence about a rejected proposal.

Such evidence must not be presented as the proposed governed memory unless separately admitted.

## 10. Validation contract

### 10.1 Purpose

A validation operation evaluates defined properties of memory, metadata, lineage, provenance, representation, or operation evidence.

### 10.2 Validation scope

A validation request must declare which checks are required.

Examples include:

- integrity;
- schema compatibility;
- identity validity;
- version lineage;
- provenance evidence;
- authority;
- representation equivalence;
- reconstruction correctness;
- lifecycle consistency.

### 10.3 Validation outcomes

A validation result may be:

- passed;
- failed;
- partial;
- unavailable;
- conflicting;
- unsupported;
- indeterminate.

### 10.4 Validation result evidence

A validation result should identify:

- checks performed;
- checks not performed;
- evidence used;
- evidence missing;
- validation authority;
- validation time;
- applicable schema or policy;
- resulting trust limitation.

### 10.5 Partial validation is not full validation

A result passing integrity checks but lacking provenance validation must not be reported as fully validated when provenance was required.

### 10.6 Validation does not commit

Successful validation does not independently create or commit a memory version.

### 10.7 Failed validation does not erase material automatically

Invalid or failed material may require quarantine, repair, audit, or retention as evidence.

Validation alone does not grant deletion authority.

## 11. Commitment contract

### 11.1 Purpose

A commitment operation establishes that a memory version or lifecycle decision has entered authoritative history.

### 11.2 Commitment subject

The request must identify:

- logical memory;
- proposed version or lifecycle event;
- parent or prior state;
- required validation;
- applicable consistency expectation;
- commit authority;
- operation identity.

### 11.3 Commitment preconditions

Commitment may require:

- valid identity;
- authorized proposer;
- accepted proposal;
- valid lineage;
- satisfied schema requirements;
- required validation;
- expected parent version;
- absence or explicit handling of conflict.

### 11.4 Commitment outcome

A commitment result must identify:

- committed subject;
- resulting version or lifecycle identity;
- parent relations;
- commit evidence;
- consistency scope achieved;
- durability state;
- unresolved conflict;
- finality.

### 11.5 Commit acknowledgment

A commit acknowledgment must come from an authority permitted to establish commitment.

A storage custodian acknowledging local persistence is not automatically commit authority.

### 11.6 Indeterminate commitment

If commitment may have occurred but cannot be verified, the operation is indeterminate.

Retry must use the same operation identity and reconcile existing history.

## 12. Recall-request contract

### 12.1 Purpose

A recall request asks memory services to locate, evaluate, and return memory or retrieval evidence within a declared scope.

### 12.2 Recall criteria

A recall request may identify:

- logical memory identity;
- alias;
- version;
- lineage relation;
- schema;
- provenance condition;
- bounded semantic criteria;
- bounded temporal criteria;
- memory class;
- lifecycle state.

### 12.3 Required scope declarations

The request should declare:

- requested search scope;
- consistency expectation;
- freshness expectation;
- completeness expectation;
- acceptable versions;
- acceptable representations;
- validation requirements;
- maximum result count;
- maximum result volume;
- deadline or waiting preference;
- acceptable degraded outcomes.

### 12.4 Recall does not imply mutation

A recall request shall not silently:

- create a new semantic version;
- change retention;
- alter memory content;
- delete memory;
- mark a memory cognitively important.

Governed operational metadata may be updated only when explicitly allowed.

### 12.5 Recall outcomes

A recall may produce:

- completed result;
- partial result;
- not found;
- unavailable;
- unauthorized;
- invalid request;
- conflict;
- deferred;
- cancelled;
- indeterminate.

### 12.6 Search scope is reported

The result must identify enough information to determine:

- what scope was searched;
- what scope was not searched;
- which required participants were unavailable;
- whether indexes were complete;
- whether result limits truncated the search.

### 12.7 Not found is strict

Not found may be reported only when the required search completed adequately under the request contract.

## 13. Retrieval-result contract

### 13.1 Result identity

Each returned memory result must identify, where applicable:

- logical memory identity;
- version identity;
- representation identity;
- schema identity;
- provenance state;
- validation state;
- lifecycle state.

### 13.2 Result source

The result should identify:

- participating custodians or source authorities;
- catalog or locator scope;
- reconstruction use;
- cache use;
- direct versus derived material.

Disclosure may be limited by authorization, but limitations must remain explicit.

### 13.3 Result completeness

Completeness may be:

- complete for declared scope;
- partial;
- truncated by caller limit;
- limited by authorization;
- limited by availability;
- unknown.

### 13.4 Result freshness

Freshness may be:

- current within declared scope;
- stale;
- bounded by observed version;
- unknown;
- conflicting.

### 13.5 Result validation

The result must not imply validation that was not performed.

### 13.6 Result conflict

Conflicting versions or provenance claims must remain explicit.

### 13.7 Result redaction

A result may redact:

- content;
- provenance;
- location;
- participants;
- security metadata.

Redaction must not be represented as absence.

### 13.8 Result continuation

When bounded results require continuation, the contract must preserve:

- query identity;
- scope;
- consistency assumptions;
- authorization;
- result ordering assumptions;
- truncation state.

A continuation mechanism is not defined by this public specification.

## 14. Mutation and correction contract

### 14.1 Purpose

A mutation operation proposes a new governed state of an existing logical memory.

Committed versions are not mutated in place.

### 14.2 Required subject state

A mutation should identify:

- logical memory;
- expected base version or version set;
- requested change;
- resulting schema;
- provenance;
- authority;
- operation identity.

### 14.3 Compare-and-change behavior

Where lost updates are possible, the request must identify an expected base condition.

A mismatched base should produce:

- failed precondition;
- conflict;
- another explicit non-success outcome.

It must not silently overwrite newer state.

### 14.4 New version

Successful semantic mutation creates a new version or another explicit governed event.

### 14.5 Partial mutation

A mutation contract must declare whether it is:

- atomic;
- component-wise;
- compensatable;
- independently committed by part.

Ambiguous partial mutation is not permitted.

### 14.6 Correction

A correction must preserve:

- corrected version identity;
- correcting version or event;
- authority;
- reason or evidence where policy requires;
- prior historical existence.

## 15. Retention and archival contract

### 15.1 Purpose

A retention operation requests a governed change to how long or under which conditions memory should remain preserved.

### 15.2 Required inputs

The request should identify:

- target memory or version scope;
- current retention state where required;
- requested retention state;
- authority;
- policy basis;
- effective condition;
- operation identity.

### 15.3 Retention outcome

The result must distinguish:

- accepted policy change;
- committed policy change;
- rejected change;
- deferred change;
- blocked change;
- conflicting hold or policy;
- indeterminate state.

### 15.4 Retention does not prove physical completion

A committed archival or retention decision may require later physical movement or replication work.

The semantic decision and physical completion must remain distinct.

## 16. Deletion contract

### 16.1 Purpose

A deletion operation requests a governed transition declaring memory deleted within a defined scope.

### 16.2 Required inputs

A deletion request should identify:

- logical memory or version scope;
- expected lifecycle state;
- authority;
- deletion reason or policy basis where required;
- required approval evidence;
- stale-state suppression requirements;
- operation identity.

### 16.3 Deletion scope

Deletion scope must distinguish:

- logical memory;
- one version;
- one representation;
- one physical copy;
- one alias;
- one provenance view;
- one retention class.

Deleting a physical copy is not automatically logical deletion.

### 16.4 Deletion outcomes

A deletion may be:

- rejected;
- deferred;
- authorized;
- committed;
- partially propagated;
- blocked;
- conflicting;
- indeterminate.

### 16.5 Authorization is separate from execution

Deletion authorization does not prove every physical copy has been erased.

### 16.6 Logical deletion is separate from physical sanitization

A result must distinguish:

- lifecycle deletion committed;
- tombstone or suppression state established;
- custodians notified;
- physical reclamation pending;
- sanitization confirmed;
- sanitization unavailable.

### 16.7 Retry safety

Retrying deletion with the same operation identity must not broaden scope or erase later unrelated versions.

### 16.8 Stale resurrection prevention

Deletion completion must preserve sufficient governed evidence to prevent stale copies from restoring deleted state automatically.

## 17. Repair and reconstruction contracts

### 17.1 Purpose

Repair and reconstruction operations restore required availability, integrity, or durability without changing logical meaning unless explicitly declared.

### 17.2 Repair request

A repair request should identify:

- affected logical memory;
- version or representation;
- failed protection requirement;
- known valid sources;
- required validation;
- target protection state;
- resource limits;
- operation identity.

### 17.3 Repair outcome

A repair result must identify:

- repaired scope;
- new physical copies;
- new representations;
- validation performed;
- achieved durability;
- remaining under-protection;
- unresolved conflict.

### 17.4 Reconstruction identity

Reconstruction must apply the identity rules from MEM-0003.

### 17.5 Repair does not authorize semantic overwrite

Repair must not silently replace newer committed state with stale recovered material.

### 17.6 Repair duplication is bounded

Duplicate repair requests must not create uncontrolled replica growth.

## 18. Cancellation contract

### 18.1 Cancellation is an operation

A cancellation request is itself an identified semantic operation directed at another operation.

### 18.2 Cancellation request is not cancellation success

Receiving or acknowledging a cancellation request does not prove that the target operation stopped.

### 18.3 Cancellation states

Cancellation may be:

- requested;
- accepted;
- pending;
- completed;
- rejected;
- too late;
- unsupported;
- indeterminate.

### 18.4 Too late

Cancellation may be too late when the target operation has already:

- committed;
- completed;
- produced an irreversible governed effect;
- entered a stage that the contract declares non-cancellable.

### 18.5 Cancellation does not roll back automatically

Cancelling future work does not inherently reverse semantic effects already committed.

Compensation or reversal requires a separately governed operation.

### 18.6 Cancellation result

A successful cancellation result must identify which effects were prevented and which effects had already occurred.

## 19. Deadline and timeout behavior

### 19.1 Wait deadline

A wait deadline limits how long a requester waits for a result.

It does not inherently terminate the underlying operation.

### 19.2 Operation deadline

An operation deadline is a governed condition after which the operation must not begin or continue producing specified effects.

The contract must distinguish it from a requester wait timeout.

### 19.3 Timeout result

A timeout must report what is known:

- operation not admitted;
- operation pending;
- operation may have committed;
- operation state unknown;
- cancellation requested;
- reconciliation required.

### 19.4 Retry after timeout

Retrying after timeout must reuse the original operation identity when the semantic intent is unchanged.

### 19.5 Clock uncertainty

Deadlines based on time must account for:

- clock source;
- clock uncertainty;
- participant-local interpretation;
- unavailable time evidence.

MEM-0004 does not require synchronized global clocks.

## 20. Operation-status and reconciliation contract

### 20.1 Purpose

An operation-status request asks for the known semantic state of a prior operation.

### 20.2 Status request

The request identifies:

- original operation identity;
- requester authority;
- desired evidence scope;
- acceptable consistency.

### 20.3 Status outcomes

Status may report:

- unknown operation;
- pending;
- accepted;
- committed;
- completed;
- rejected;
- cancelled;
- failed;
- indeterminate;
- conflicting operation records.

### 20.4 Unknown operation is not proof of non-execution

A role instance lacking operation history must not report that the operation never occurred unless it possesses authority and complete evidence for that scope.

### 20.5 Reconciliation

Reconciliation may compare:

- operation journals;
- commit history;
- lifecycle history;
- custodian evidence;
- version lineage;
- role-instance records.

### 20.6 Reconciliation result

A reconciliation result must preserve uncertainty when evidence remains incomplete or conflicting.

## 21. Compound and multi-subject operations

### 21.1 Explicit compound scope

An operation affecting several memories or versions must declare:

- subject set;
- required atomicity;
- independent outcomes;
- ordering;
- rollback or compensation behavior;
- resource bounds.

### 21.2 Atomic compound operation

An operation claiming atomicity must define what observers may see and how recovery preserves that claim.

### 21.3 Non-atomic compound operation

A non-atomic operation must report per-subject results.

### 21.4 Partial compound completion

Partial completion must not be hidden behind one generic success result.

### 21.5 Unbounded bulk operation is prohibited

A bulk operation must use bounded selection and continuation or partitioning.

## 22. Security and privacy

### 22.1 Operation identity may be sensitive

Operation identifiers and status records may reveal:

- memory existence;
- access patterns;
- participant activity;
- lifecycle actions;
- repair behavior.

Access must remain authorized.

### 22.2 Results minimize disclosure

A rejected or unauthorized result should disclose only the information permitted by policy.

### 22.3 Concealment does not alter internal truth

A security policy may provide an externally opaque response.

Internally, the system must preserve the actual distinction between:

- unauthorized;
- unknown;
- unavailable;
- not found.

### 22.4 Replays do not renew authority

Replaying an old request does not automatically restore expired or revoked authority.

The operation must evaluate authority according to its contract and operation history.

### 22.5 Request content is integrity-protected

Material semantic fields must be protected against unnoticed modification.

### 22.6 Result authority is identifiable

A consumer must be able to determine which role or authority produced a material result.

## 23. Boundary with ACS

ACS governs:

- participant communication;
- relationships;
- endpoint and port exposure;
- signals;
- payload references;
- admission to communication;
- mediation;
- routing;
- secure sessions;
- transport retries;
- communication backpressure.

MEM governs:

- operation identity;
- semantic request type;
- memory authority;
- operation admission;
- preconditions;
- idempotency;
- semantic progress;
- final outcomes;
- cancellation;
- not-found meaning;
- mutation;
- commitment;
- lifecycle effects;
- operation reconciliation.

The following rules apply:

1. ACS delivery is not MEM acceptance.
2. ACS acknowledgment is not MEM completion.
3. ACS retry uses the same MEM operation identity when intent is unchanged.
4. ACS connection loss does not establish operation failure.
5. ACS timeout does not establish non-commitment.
6. ACS authentication does not establish memory-operation authority.
7. MEM does not redefine ACS retransmission.
8. MEM does not require one ACS signal per memory operation.
9. One MEM operation may use several ACS exchanges.
10. One ACS payload may contain several explicitly identified MEM operations only when their boundaries remain unambiguous.

## 24. Role boundaries

The roles defined by MEM-0002 participate as follows.

### 24.1 Requester

Creates or submits the operation request.

### 24.2 Access authority

Determines whether the requested operation is permitted.

### 24.3 Admission evaluator

Determines whether the operation may enter governed processing.

### 24.4 Validator

Evaluates required structural, integrity, provenance, schema, or equivalence conditions.

### 24.5 Commit authority

Establishes authoritative commitment.

### 24.6 Custodian

Performs assigned physical storage or retrieval work.

### 24.7 Catalog and locator

Provides known identity and location evidence.

### 24.8 Retrieval provider

Obtains memory material.

### 24.9 Result coordinator

Assembles the semantic retrieval outcome.

### 24.10 Lifecycle authority

Authorizes retention and deletion transitions.

### 24.11 Repair coordinator

Coordinates repair and reconstruction.

No role gains authority beyond its explicit assignment merely by participating in an operation.

## 25. Conformance expectations

A conforming implementation must demonstrate:

- stable operation identity;
- distinction between operation, attempt, and delivery;
- duplicate-safe mutation;
- duplicate-safe identity creation;
- duplicate-safe deletion;
- explicit acceptance and commitment;
- explicit finality;
- strict not-found semantics;
- timeout reconciliation;
- cancellation behavior;
- stale-precondition handling;
- partial-result reporting;
- authorization distinction;
- bounded operation scope;
- operation-history behavior;
- indeterminate-outcome handling.

Detailed fault testing belongs to MEM-0010.

## 26. Prohibited interpretations

This specification shall not be interpreted to mean that:

- every delivery is a new operation;
- successful delivery means acceptance;
- acceptance means commitment;
- commitment means durability;
- timeout means failure;
- timeout means non-commitment;
- cancellation request means cancellation completed;
- connection closure means operation completion;
- retry should use a new semantic identity;
- duplicate delivery may repeat mutation;
- not found may represent incomplete search;
- unauthorized may be recorded internally as not found;
- one generic success flag is sufficient;
- one generic error flag is sufficient;
- every read returns identical results;
- validation commits memory;
- local persistence commits memory;
- one custodian acknowledgment proves durability;
- physical deletion equals logical deletion;
- logical deletion proves physical sanitization;
- repair may overwrite newer state;
- bulk operations may have unbounded scope;
- public contracts disclose non-public recall arbitration.

## 27. Open questions

The following questions remain for later specifications and implementation profiles:

- Which operation types require durable operation history?
- How long must deduplication evidence be retained?
- Which read operations should require stable operation identity?
- Which operations permit partial completion?
- Which compound operations require atomicity?
- Which outcomes are universal enum values versus contract-specific details?
- How should operation identities be generated across disconnected participants?
- Which operation types permit proposer-selected identity?
- How should old operation records be compacted safely?
- Which timeout conditions should trigger automatic reconciliation?
- Which cancellation stages are mandatory?
- How should status inquiries behave after operation-history archival?
- Which operations require independent result authority?
- How should long-running recall expose progress without leaking sensitive scope?
- How should authorization concealment be represented externally?
- Which mutation preconditions are mandatory for continuity-critical memory?
- How should retention and deletion operations coordinate with active retrieval?
- Which repair operations may proceed automatically under degraded authority?
- How should implementation-specific extensions add operation types without weakening public outcomes?
- Which low-level contracts should become shared C++ interfaces before implementation begins?

These questions do not weaken the operation distinctions established here.

## 28. Closing principle

> **Node must never confuse “a message was handled” with “memory changed in the intended way.”**

A delivery is not an operation.

A retry is not new intent.

Acceptance is not commitment.

Commitment is not durability.

Timeout is not failure.

Cancellation is not rollback.

Silence is not not found.

Every semantic effect must remain identifiable, bounded, and reconcilable.

## Revision history

### Version 0.1 — 2026-07-16

- Replaced the planned MEM-0004 stub with the first normative working draft.
- Defined operation, attempt, and delivery identity.
- Defined request, result, progress, finality, and operation-history semantics.
- Established duplicate-safe retry and idempotency requirements.
- Defined universal semantic outcome classes.
- Defined storage-proposal, validation, commitment, recall, retrieval, mutation, retention, deletion, repair, and reconstruction contracts.
- Defined cancellation, deadline, timeout, status, and reconciliation behavior.
- Established compound-operation and partial-completion requirements.
- Preserved authorization, privacy, role, and ACS boundaries.
- Established conformance expectations for low-level operation implementations.
