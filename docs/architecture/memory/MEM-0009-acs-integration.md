# MEM-0009: ACS Integration

| Field | Value |
|---|---|
| Specification | MEM-0009 |
| Title | ACS Integration |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Accepted as initial working draft |
| Depends on | MEM-0000 through MEM-0008; ACS-0000 through ACS-0005 |
| Related future specifications | ACS-0006, ACS-0007, ACS-0008, ACS-0009 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in semantic separation and operation mapping; baseline port profiles and cancellation integration remain under review |

> **MEM defines memory semantics through ACS communication without becoming a second endpoint, routing, security, or transport system.**

## Architectural-intent notice

This specification defines how public MEM contracts use the Adaptive Connection Substrate.

It defines the semantic integration between:

- memory-service roles;
- ACS participants;
- relationships;
- endpoints;
- ports;
- attachments;
- connections;
- bindings;
- signals;
- payload references;
- communication admission;
- memory-operation admission;
- operation identity;
- signal identity;
- correlation;
- retry;
- cancellation;
- timeout;
- backpressure;
- result reporting;
- failure-state translation;
- degraded operation;
- mediation;
- recovery and reconnection.

It does not define:

- ACS endpoint mechanics;
- ACS port implementation;
- network addresses;
- numeric network ports;
- route selection;
- transport protocols;
- binary signal formats;
- serialization layouts;
- attachment encodings;
- secure-session mechanisms;
- cryptographic algorithms;
- exact timeout values;
- production endpoint catalogs;
- production port catalogs;
- production relationship topology;
- complete admission algorithms;
- complete budget-allocation algorithms;
- exact memory-service interfaces;
- implementation-specific memory algorithms;
- internal state formats;
- production operator procedures.

Implementations may use local function dispatch, shared memory, queues, sockets, secure remote sessions, bridge mediation, store-and-forward communication, facade services, or future transport mechanisms.

They must preserve the architectural boundaries and semantic mappings established here.

## 1. Purpose

MEM and ACS solve different problems.

ACS defines how identified participants communicate through bounded logical relationships and communication capabilities.

MEM defines what memory is and what memory operations mean.

A memory service may use ACS to receive a storage proposal, return an admission result, accept a recall request, deliver a retrieval result, expose memory-role availability, report durability state, coordinate repair, request lifecycle change, propagate deletion state, reconcile recovery, or transfer a representation or reconstruction shard.

The use of ACS does not make memory semantics part of ACS. Likewise, MEM must not create an independent communication substrate merely because memory operations require identity, request and response, retry, cancellation, payload transfer, backpressure, authentication, routing, or mediation.

Without an explicit integration model, an implementation may incorrectly assume that discovering a memory endpoint discovers every memory role; connecting to a memory service means memory is available; attaching to a storage port means a storage proposal is admitted; delivering a recall request means recall was accepted; acknowledging a signal means memory was committed; successfully transferring bytes proves retrieval completed; authentication grants authority to mutate or delete memory; an ACS retry creates a new memory operation; connection cancellation cancels an executing memory operation; connection loss proves memory absence; endpoint retirement deletes retained memory; payload-reference expiry deletes the referenced memory; communication backpressure means memory admission failed; or re-establishing a connection permits stale operations to execute again.

MEM-0009 prevents those semantic collapses.

## 2. Scope

This specification governs representation of memory services as ACS participants; relationship selection for memory communication; memory-service endpoint expectations; memory port-contract requirements; mapping of memory operations onto ACS signals; use of ACS signal domains and intents; memory payload-reference behavior; operation, signal, attempt, delivery, and correlation identity; communication admission versus memory admission; ACS authorization versus memory authorization; result layering; retry and duplicate behavior; cancellation mapping; timeout and deadline mapping; communication and memory backpressure; connection lifecycle interaction; mediation and facade behavior; local fast paths; degraded communication; failure-state translation; reconnection and reconciliation; and public conformance requirements.

It applies to communication involving all memory-service roles defined by MEM-0002, whether participants are in one process, separate processes, one physical node, several physical nodes, directly connected, mediated, temporarily partitioned, represented through facade ports, or operating under degraded conditions.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements. The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification. The word **may** describes permitted behavior.

An implementation does not conform merely because a memory service sends messages through an ACS-compatible transport. Its observable behavior must preserve the semantic distinctions established by MEM and ACS.

## 4. Foundational separation model

### 4.1 Memory is not communication

A memory is governed stored state. ACS communication may refer to memory, request memory, deliver memory material, report memory state, or request memory change. The communication object is not the memory object.

### 4.2 A memory-service role is not an ACS endpoint

A memory-service role defines responsibility and authority. An endpoint defines a communication boundary. A role may be exposed through one endpoint, several endpoints, one composite endpoint, a facade, or no currently reachable endpoint. Endpoint state does not define role state.

### 4.3 An endpoint is not memory authority

Owning or hosting an endpoint does not grant authority to commit, mutate, delete, disclose, retain, or declare durability for memory.

### 4.4 A port contract is not a memory-operation contract

An ACS port contract defines which communication is permitted, its direction, signal compatibility, payload-reference behavior, admission expectations, authority requirements, resource limits, and communication failure behavior.

A MEM operation contract defines the semantic memory operation, its subject, preconditions, requested guarantees, memory authority, memory outcomes, finality, idempotency, and reconciliation.

A port may carry a memory operation. It does not define the operation’s memory semantics.

### 4.5 Relationship class is not memory authority

A relationship class explains why communication exists. It does not independently authorize a memory operation.

A synaptic relationship does not automatically grant recall or retention authority. An infrastructure relationship does not automatically grant repair, deletion, or custody authority. An immune relationship does not automatically grant access to memory content or authority to invalidate memory. A bridge relationship does not automatically grant permission to transform or aggregate memory results.

### 4.6 Attachment is not memory admission

An active attachment permits an eligible participant to attempt scoped port use. It does not prove that a memory operation is valid, authorized, admitted, accepted, committed, or completed.

### 4.7 Signal delivery is not memory acceptance

ACS delivery establishes communication progress. It does not establish that MEM accepted the request, admitted the proposal, committed the version, retrieved the memory, completed deletion, or completed repair.

### 4.8 Signal acceptance is not memory-operation acceptance

ACS signal acceptance means the receiving communication boundary validated the signal sufficiently to consider its meaning. A separate MEM role may still reject, defer, challenge, partially admit, mark conflicting, or report unavailable.

### 4.9 Payload reference is not memory identity

A payload reference may locate or authorize access to memory material. It is not logical memory identity, version identity, representation authority, commitment evidence, durability evidence, or retention authority.

### 4.10 Payload delivery is not retrieval completion

A payload transfer may complete successfully while the memory result remains stale, incomplete, conflicting, unverified, under-protected, or semantically unavailable. A retrieval result may also complete without transferring content when it reports not found, unavailable, unauthorized, metadata only, lifecycle state, or conflict.

### 4.11 Connection identity is not operation identity

A MEM operation must retain stable identity independently of ACS relationship, endpoint, port, attachment, connection instance, connection generation, binding, secure session, and signal delivery.

### 4.12 Connection lifecycle is not memory lifecycle

Connection activation, degradation, suspension, drain, closure, re-establishment, and retirement do not directly define memory retention, archival, commitment, deletion, reclamation, or recovery.

### 4.13 ACS backpressure is not memory admission

ACS backpressure describes communication-capacity pressure. MEM admission determines whether a memory operation may enter governed memory processing. Either may succeed while the other fails.

### 4.14 Authentication is not memory authorization

ACS and security systems may establish participant identity and communication security. MEM must still determine whether that participant may perform the requested memory operation.

### 4.15 ACS availability is not memory availability

The following remain separate:

```text
Binding availability
    can the runtime communication path operate?

Connection readiness
    can the ACS connection carry a declared interaction?

Endpoint or port availability
    is the communication boundary presently usable?

Memory-role availability
    can the required logical memory responsibility be exercised?

Memory availability
    can the requested memory material or semantic evidence be obtained?
```

No layer shall silently claim the state of another.

## 5. Integration model

```text
Memory-service role
    defines responsibility and memory authority

ACS participant
    represents an identified logical communicator

ACS endpoint
    identifies a logical communication boundary owned by the participant

ACS port
    exposes a bounded typed communication capability

ACS relationship
    explains why the participants are associated

ACS connection
    carries admitted communication for the relationship

ACS attachment
    grants scoped ability to attempt use of a port

ACS signal
    carries bounded request, response, evidence, or observation semantics

ACS payload reference
    identifies separately governed substantive data

MEM operation
    defines the semantic memory action carried through the exchange
```

An implementation may represent several concepts through fewer runtime objects. Their architectural meanings must remain independently observable.

## 6. Conceptual integration records

These records define public semantics, not required wire formats.

```text
MemoryServiceExposure
    participant_identity
    endpoint_identity
    exposed_memory_roles
    port_contract_references
    role_availability_state
    endpoint_availability_state
    supported_mem_versions
    supported_acs_versions
    mediation_state
    visibility_scope
```

```text
MemoryPortBinding
    endpoint_identity
    port_identity
    port_contract_version
    supported_mem_operation_families
    supported_signal_domains
    supported_signal_intents
    supported_operation_schema_versions
    payload_reference_policy
    eligible_relationship_classes
    attachment_requirements
    communication_authority_requirements
    memory_authority_requirements
    resource_contract
    failure_behavior
```

```text
MemoryOperationExchange
    memory_operation_identity
    memory_operation_type
    acs_signal_identity
    acs_correlation_identity
    requester_identity
    target_memory_role
    endpoint_identity
    port_identity
    relationship_identity
    connection_instance_identity
    connection_generation
    attachment_identity
    payload_references
    communication_state
    memory_operation_state
    finality
```

```text
LayeredMemoryResult
    memory_operation_identity
    acs_delivery_state
    acs_signal_acceptance_state
    payload_transfer_state
    memory_admission_state
    memory_operation_outcome
    memory_finality
    achieved_guarantees
    unsatisfied_guarantees
    remaining_uncertainty
```

Equivalent information may be carried in signals, bound through authenticated context, stored in operation history, represented by references, or reconstructed during reconciliation.

## 7. Memory services as ACS participants

A memory service communicating through ACS must be represented by an identified logical participant independent of physical host, process, thread, service replica, binding, connection, or transport address.

A participant may declare one or more memory-service roles. Declaration does not prove current assignment, authority, availability, correctness, or complete role coverage. Role assignment remains governed by MEM; ACS discovery or endpoint ownership must not assign a role automatically.

A distributed memory service may use one composite participant, several cooperating participants, a stable facade participant, or another declared model. The representation must not conceal material differences in authority, availability, result scope, failure domains, or role coverage.

Several runtime replicas may implement one logical participant or endpoint. Replica selection does not change operation identity, role meaning, requested guarantees, or authorization requirements. Replica-local limitations must remain explicit.

## 8. Relationship classes for memory communication

Memory communication does not require one universal relationship class. The class must reflect why the relationship exists.

A synaptic relationship may carry recall cues, bounded cognitive retrieval requests, or cognitive use of recalled state. Synaptic classification does not grant unrestricted recall, storage, or mutation authority.

An infrastructure relationship may carry role availability, capacity, custodian status, repair coordination, durability, lifecycle propagation, recovery progress, or catalog synchronization. Infrastructure classification does not grant memory authority.

A bridge relationship may mediate memory communication across regions, trust boundaries, expensive paths, incompatible representations, or independently organized services. Bridge mediation must preserve or explicitly narrow operation identity, authority, provenance, scope, consistency expectations, and result limitations.

An immune relationship may carry memory-integrity evidence, corruption observations, inconsistent-version evidence, or recovery-verification traffic. Immune classification does not grant ordinary content access, mutation, deletion, or custody authority.

Where one association combines materially different cognitive, operational, and protective purposes, separate relationships should normally be used. Relationship class shall not redefine MEM operation meaning.

## 9. Endpoint expectations

A memory-service participant may own one or more ACS endpoints. Endpoint ownership does not create memory authority beyond assigned roles.

Endpoint identity may survive process restart, host migration, failover, transport replacement, secure-session renewal, and temporary unavailability. Endpoint continuity does not prove memory-service continuity.

A participant may expose separate endpoints for ordinary memory service, recovery, administrative lifecycle, public facade access, internal coordination, or high-volume transfer. Separate endpoints are not mandatory when one endpoint preserves all required boundaries.

Discovery may reveal supported public MEM versions, selected public port contracts, broad service purpose, and admission requirements. It must not disclose unrestricted memory inventory, role authority, memory identities, topology, custody, or private operation families.

Endpoint availability indicates only that the communication boundary may support interaction. It is not memory availability.

## 10. Memory port contracts

Every port carrying MEM operations must declare or reference supported operation families; signal domain and intent; operation schema versions; direction; payload-reference policy; eligible relationship classes; communication-admission requirements; memory-authorization requirements; resource limits; response expectations; cancellation support; retry expectations; and failure behavior.

Operation semantics remain in MEM. A port may state that it carries recall requests, but MEM determines what recall means, valid scope, outcomes, completeness, consistency, and finality.

Materially different capabilities should normally use separate ports when they differ in authority, direction, sensitivity, resource cost, payload size, lifecycle impact, failure behavior, response semantics, or cancellation behavior. Examples include ordinary recall versus deletion, metadata lookup versus bulk transfer, observation versus lifecycle directive, and ordinary service versus recovery service.

A profile may combine related operation families on one port when every operation remains explicitly typed, authority remains operation-specific, resource accounting remains separable, failure behavior remains unambiguous, and one operation cannot masquerade as another.

Conceptual public port families may include proposal and admission; recall and query; result and status; representation transfer; validation and evidence; lifecycle; repair and recovery; and role and availability observation. These are not mandatory port names or catalogs.

Material changes to operation meaning, authority, direction, payload behavior, cancellation, or result guarantees require explicit contract version change, renewed compatibility, a new port identity, or retirement as appropriate.

## 11. Signal-domain mapping

A memory operation does not automatically create a separate ACS signal domain. The domain must describe the meaning of the communication.

Memory-related signals may be cognitive when they concern recall cues or cognitive requests; operational when they concern role availability, capacity, storage condition, durability, custody, lifecycle, repair, or recovery; immune when they concern corruption or recovery verification; and security-and-trust when they concern capability, authorization, revocation, payload access, key availability, or trust restriction.

Domain does not determine authority. Implementations may define public memory-related subtypes, but each subtype must have explicit schema identity, version, validation requirements, authority requirements, freshness behavior, and duplicate behavior.

## 12. Signal-intent mapping

Observation may report role availability, memory availability, protection, lifecycle, recovery, corruption, or capacity. It does not request action.

Request asks a memory service to consider or perform an operation and does not require compliance.

Directive invokes a specifically authorized memory operation and requires explicit source authority, target authority, bounded scope, replay resistance, and observable outcome. A request-capable port is not automatically directive-capable.

Evidence may present or reference provenance, integrity, commitment, custody, deletion, recovery, or conflict evidence. Evidence is not commitment, verdict, or authorization.

Response may report communication receipt, signal rejection, memory admission, progress, result, transfer status, cancellation, or reconciliation. The response must identify which layer its status describes.

Cognitive influence may affect the likelihood or priority of a memory request, but must not disguise mutation, deletion, or lifecycle directive.

## 13. Identity and correlation model

MEM operation identity identifies one semantic memory operation and remains stable across signal retry, reconnect, rebinding, mediation, service failover, payload-transfer retry, status inquiry, and result redelivery.

ACS signal identity identifies one bounded communicated semantic unit. Several signals may participate in one MEM operation.

Correlation identity connects request and response, progress and operation, payload transfer and result, cancellation and target operation, or reconciliation and prior exchange. It does not replace operation identity.

A MEM operation retry may create a new operation attempt while preserving semantic operation identity. An ACS connection attempt remains distinct from a MEM operation attempt.

One signal may have several communication deliveries. Delivery identity does not create new memory intent.

Connection generation prevents stale ACS instances from exercising current communication authority. It does not establish memory version order or operation authority.

Payload-reference identity identifies one governed access reference and does not replace operation, memory, representation, or transfer identity.

An authorized mediator must preserve or explicitly map operation identity, signal identity, correlation identity, source identity, operation authority, payload-reference scope, and provenance.

## 14. Payload-reference integration

Large or separately governed memory material should normally be transferred through payload references rather than embedded as unbounded signal content.

A reference may identify a representation, physical copy, reconstruction shard, checkpoint, journal segment, provenance evidence, validation evidence, or result set.

A reference should make it possible to determine or resolve payload category, claimed memory and version identity, claimed representation identity, schema, integrity descriptor, access scope, validity, expected size or resource bound, and sensitivity. These claims remain subject to validation.

Possession or receipt of a reference does not authorize access. Access must evaluate participant identity, attachment scope, communication capability, memory authorization, reference validity, sensitivity, lifecycle state, and resources.

Reference expiry ends the reference’s usable access conditions. It does not delete memory, expire retention, prove payload disappearance, or invalidate memory identity.

Payload-transfer result remains separate from memory-operation result. Transfer failure may cause partial, unavailable, deferred, or indeterminate operation state; it must not automatically become not found.

An alternate reference may be issued after expiry, route change, custodian change, reconstruction, or access-policy change. It does not create a new operation or version. Transfer does not grant retention authority.

## 15. Layered admission

Memory communication may pass through relationship eligibility, connection admission, attachment admission, signal admission, payload-access admission, memory authorization, and memory-operation admission.

The following outcomes remain distinct:

```text
Connection admitted
Attachment admitted
Signal accepted
Payload access authorized
Memory operation authorized
Memory operation admitted
Memory operation committed
Memory operation completed
```

A rejection should identify the responsible layer where disclosure is permitted.

## 16. Authorization model

ACS communication authority may permit discovery, attachment, sending, receiving, mediation, or referenced-payload access. MEM authority may permit proposal, recall, validation, commitment, mutation, retention change, deletion, repair, or reconstruction.

Neither authority inherits the other. Authority remains specific to operation type, subject, scope, parameters, lifecycle state, validity, and resource limits.

Revocation of communication capability may prevent future exchange. It does not prove that an already admitted operation stopped, failed, rolled back, or was cancelled. Same-process and same-node fast paths must enforce equivalent authorization to remote paths.

## 17. Request and response progression

A typical exchange may progress through an eligible relationship, active connection, scoped attachment, request delivery, ACS signal validation, MEM authority evaluation, MEM admission or rejection, progress, semantic outcome, payload transfer, and final result.

Not every operation requires every step. No step proves completion of a later step.

## 18. Response layering and finality

A communication receipt reports delivery only. Signal acceptance reports ACS validation only. Memory progress reports non-final semantic advancement. A memory result reports the MEM outcome and must include operation identity, outcome, finality, achieved scope, unsatisfied scope, and remaining uncertainty. A payload-transfer result reports movement or accessibility and affects operation completion only when the operation contract says it does.

A final result must come from an authorized MEM role. Connection closure or silence does not establish finality. Ambiguous status terms such as accepted, completed, failed, unavailable, or cancelled must identify their layer.

## 19. Retry and duplicate behavior

ACS may retry signal delivery, payload transfer, connection establishment, binding, or mediated forwarding. MEM retry repeats one semantic memory operation and reuses operation identity when intent is unchanged. Changed intent requires a new identity.

Duplicate signal delivery must not create duplicate memory effects. A repeated operation identity must return or reconcile with existing operation state. A conforming implementation must not rely on transport-level exactly-once delivery.

Re-establishing a connection must not recreate storage proposals, versions, deletion operations, retention changes, repair operations, or restoration operations. A mediator must not defeat duplicate detection by rewriting identity.

## 20. Cancellation

Cancelling a memory operation is itself a governed MEM operation. ACS may carry a request or directive asking that cancellation be evaluated, but delivery does not prove cancellation.

Suspending or closing a connection may prevent additional communication. It does not cancel admitted or executing work. Attachment revocation prevents future port use and does not roll back prior effects.

Cancellation responses must distinguish received, accepted, pending, completed, too late, unsupported, rejected, and indeterminate. If cancellation may have completed but its result is lost, state remains indeterminate until reconciled.

## 21. Timeouts and deadlines

Signal expiry ends one signal’s useful communication lifetime, not the underlying memory operation. A requester wait timeout ends waiting through one exchange, not the operation. ACS lifecycle deadlines limit communication activities and do not define MEM operation deadlines. Payload-reference expiry affects access to that reference, not retention or operation identity.

After timeout, the strongest supportable state must be reported: not delivered, delivered but unaccepted, signal accepted, operation pending, operation may have committed, transfer incomplete, or operation state unknown.

## 22. Backpressure and resource handling

ACS backpressure regulates queue depth, bandwidth, message processing, attachment capacity, connection attempts, and payload transfer. MEM admission pressure regulates storage, validation, indexing, commitment, retrieval, repair, retention, and durability feasibility.

A signal may be delivered while the memory operation is deferred. A memory service may be ready while ACS cannot deliver the request.

ACS may throttle, defer delivery, reject communication, load shed, or reduce attachment scope. MEM may reject, defer, reduce guarantees when permitted, return resource-deferred, or limit result size.

ACS overload must not become not found, invalid memory, deletion, or failed memory authorization. MEM resource deferral must not become transport failure. Neither layer may use the other as an unbounded queue.

## 23. Failure-state translation

Failure translation must preserve the strongest known distinction.

Endpoint absence does not prove memory absence. Concealment does not prove lack of capability. Unsupported port means the communication profile cannot carry the operation. Unauthorized attachment prevents communication but does not establish not found. Unreachable communication may cause unavailable, deferred, pending, unknown, or indeterminate state, never automatic not found.

A degraded connection may produce slower progress, reduced payload access, partial result delivery, or explicit unavailable/deferred state. It must not silently weaken consistency.

When evidence proves a request was not delivered, it may be retried without claiming MEM admission. When delivery or execution is uncertain, the operation remains pending or indeterminate and reconciliation reuses the same identity.

Structural signal rejection does not necessarily create a MEM operation. Signal expiry prevents current influence but may allow a new signal with the same operation identity. Payload unavailability is not memory absence. ACS conflict is not automatically a memory-version conflict, and MEM conflict may exist over healthy communication.

## 24. Failure translation summary

| ACS observation | Permitted MEM interpretation |
|---|---|
| Endpoint reachable | Communication may be attempted; no memory guarantee |
| Port available | Compatible communication may be attempted |
| Attachment active | Scoped port use may be attempted |
| Signal delivered | Request reached an ACS boundary |
| Signal accepted | Signal meaning passed ACS validation |
| Connection unavailable | Memory operation may be unavailable, pending, or unknown |
| Connection closed | No automatic memory outcome |
| Payload reference valid | Referenced access may be attempted |
| Payload transferred | Referenced bytes moved; semantic validation remains separate |
| Port unsupported | Operation transport profile unsupported |
| ACS backpressure | Communication deferred, throttled, or rejected |
| ACS timeout | Memory outcome not established by timeout alone |
| ACS cancellation delivered | Cancellation evaluation may begin |
| Reconnection completed | Existing memory operation may be reconciled |
| Attachment revoked | Future communication scope removed; prior operation state unchanged unless separately governed |

## 25. Connection lifecycle interaction

Connection formation does not create a memory operation. Activation permits scoped communication but does not establish role availability or memory readiness. Degradation must not silently weaken MEM guarantees. Suspension policy must define treatment of requests, queued work, payloads, progress, final results, cancellation, and recovery traffic.

Drain should normally reject or redirect new ordinary memory operations while permitting cancellation, status inquiry, final results, lifecycle, and recovery coordination. Closure does not complete, fail, cancel, or roll back in-flight operations.

Re-establishment may continue communication about existing operations using existing operation identity. Resumption must not preserve stale authority or stale payload access. Rebinding must not change operation meaning. Handoff must preserve or reconcile operation identity, result delivery, payload transfer, cancellation, and attachment scope without creating split semantic authority.

## 26. Degraded integration profiles

A profile may permit status-only, observation-only, request-only, read-only, metadata-only, no-payload-transfer, no-new-proposal, no-mutation, no-lifecycle-change, recovery-only, cancellation-and-final-results-only, local-only, or mediated-only operation.

A reduced profile must identify permitted ACS interaction, permitted MEM operations, unavailable operations, result limitations, payload limitations, consistency limitations, and exit conditions. Reduced ACS mode must not redefine MEM meaning, and reduced MEM mode must not expand ACS authority.

## 27. Mediation and facade integration

An authorized mediator may filter requests, aggregate observations, translate compatible schemas, redact results, route to several roles, assemble bounded result sets, or expose a public facade.

Material mediation must preserve original requester, mediator identity, transformation, source roles, result scope, authorization boundary, and omitted or redacted scope. A mediator may preserve or narrow authority but not amplify it.

Mediation must preserve operation identity or record an explicit parent-child relationship. Aggregated recall must report participating and unavailable providers, completed and incomplete scope, conflicts, and consistency achieved.

A facade may expose fewer operations and hide internal topology, but must report unsupported or degraded behavior and semantic limitations honestly. Mediator failure is not memory absence.

## 28. Local and same-process integration

A local fast path must preserve the same operation identity, authorization, admission, validation, resource accounting, result state, cancellation, and lifecycle boundary as a remote path.

A local implementation need not serialize or use a network stack. Shared address space, process, or node does not grant authority. Local failures must map to the same public semantics as comparable remote failures.

## 29. Security and privacy

ACS and security systems protect communication identity, sessions, integrity, confidentiality, and capabilities. MEM protects memory-operation authorization, content access, lifecycle authority, and semantic integrity.

Memory endpoints and contracts may reveal capabilities, retention behavior, recovery capability, topology, or role assignment; disclosure must remain bounded. Secure session does not automatically permit plaintext memory access. Payload access may require key authority, sensitivity clearance, memory authorization, and representation validation.

Revoked communication or payload capability must not remain usable through stale attachments, resumed connections, alternate bindings, cached references, or mediator replay. Integration records and audit evidence require governed access and retention. Memory services, endpoints, connections, and payload references must not become unrestricted raw private-key custodians.

## 30. Recovery and reconciliation

ACS recovery restores or reconciles relationships, connections, bindings, attachments, and signal delivery. MEM recovery restores or reconciles memory identity, versions, provenance, custody, lifecycle, operation state, content, and durability.

Reconnection may restore communication but does not prove current memory state. A rejoining memory participant must reconcile both ACS generation and attachment state and MEM role assignment, operation history, lifecycle, and version state.

A stale connection instance must not deliver current-authority directives. A current connection must not make stale memory overwrite newer state. Status inquiry after reconnect must use the original operation identity. ACS recovery mode may carry bounded memory-recovery communication without granting unrestricted ordinary memory access.

## 31. Boundary with runtime, health, security, and immune systems

Runtime may start processes, bind endpoints, migrate implementations, select dispatch, or restart services. Runtime placement does not define role authority or endpoint ownership.

Health may report process, connection, endpoint, capacity, latency, or storage condition, but does not establish semantic correctness.

Security provides participant identity, authentication, cryptographic protection, capability validation, revocation, and key services. MEM defines memory-specific authorization and lifecycle authority.

Immune systems may observe invalid signals, unauthorized requests, conflicting identities, payload-reference abuse, ignored backpressure, suspicious retry, or recovery inconsistency. Observation does not grant mutation, deletion, content access, or physical-node authority.

## 32. Public implementation requirements

A public implementation claiming support for MEM-0009 shall document how memory roles map to ACS participants; endpoint ownership; supported memory port contracts; supported operation families; signal domains and intents; schema versioning; operation, signal, and correlation identity; payload-reference behavior; communication and memory admission; communication and memory authorization; response layering; retry and duplicate handling; cancellation; timeout; backpressure; connection-loss handling; reconnection and reconciliation; mediation and facade behavior; local fast-path equivalence; degraded profiles; unsupported features; and continuity limitations.

The implementation need not expose production endpoint names, port catalogs, topology, private operation families, or internal memory algorithms.

## 33. Conformance expectations

A conforming implementation must demonstrate that:

1. a memory-service role is distinguishable from an ACS endpoint;
2. endpoint availability is distinguishable from role availability;
3. role availability is distinguishable from memory availability;
4. relationship class does not grant memory authority;
5. attachment admission does not grant memory-operation admission;
6. signal delivery does not report memory acceptance;
7. signal acceptance does not report memory commitment;
8. payload transfer does not automatically report semantic retrieval completion;
9. payload-reference identity is separate from logical memory identity;
10. operation identity survives signal retry;
11. operation identity survives reconnect and rebinding;
12. duplicate delivery does not create duplicate memory effects;
13. transport-level exactly-once delivery is not required;
14. communication cancellation does not automatically cancel memory work;
15. connection closure does not establish operation failure;
16. timeout does not establish non-commitment;
17. ACS backpressure does not become memory not found;
18. memory resource deferral does not become ACS transport failure;
19. unsupported ports do not become memory absence;
20. degraded ACS operation does not silently weaken MEM guarantees;
21. local fast paths preserve remote authority boundaries;
22. mediation preserves operation identity and required provenance;
23. facade ports report limitations honestly;
24. reconnect does not renew stale authority;
25. current ACS state does not validate stale MEM state;
26. unknown and indeterminate states remain explicit.

Detailed fault injection belongs to MEM-0010.

## 34. Prohibited interpretations

This specification shall not be interpreted to mean that every memory role requires a dedicated endpoint; every operation requires a dedicated port; every port requires a separate socket; memory operations automatically create a signal domain; one relationship class is mandatory for all memory traffic; relationship class, endpoint ownership, reachability, attachment, delivery, signal acceptance, payload transfer, authentication, retry, reconnect, suspension, closure, timeout, backpressure, or local placement silently grant or prove MEM authority, admission, commitment, availability, cancellation, deletion, or completion.

Public integration does not require disclosure of implementation-specific memory mechanisms.

## 35. Open questions

- Which memory operation families require dedicated public port contracts?
- Which related operations may safely share one port?
- Which port families must support bidirectional communication?
- Which operations may use directive rather than request intent?
- Should a public memory-specific signal domain ever be introduced?
- Which operation-schema fields must be universal?
- Which payload-reference claims require mandatory binding to memory identity?
- Which payload-reference capabilities may survive connection migration?
- How should payload references be renewed after authorization changes?
- Which response-layer identifiers should be mandatory in shared interfaces?
- Which operations require acceptance-confirmed versus result-confirmed communication?
- Which cancellations must remain available during drain or suspension?
- Which operations may continue after attachment revocation?
- Which timeout combinations require automatic reconciliation?
- Which degraded ACS profiles permit memory mutation?
- Which memory roles require protected communication capacity?
- How should result coordinators represent mediated providers?
- Which public facade operations are required for baseline conformance?
- How should operation correlation survive multi-stage payload transfer?
- Which low-level integration structures should become shared C++ interfaces?
- How should future ACS admission and security specifications refine this document without shifting memory semantics into ACS?

These questions do not weaken the boundaries already established.

## 36. Closing principle

> **Node must never confuse permission to communicate about memory with authority to change memory.**

A relationship is not a memory role.

An endpoint is not memory authority.

A port is not an operation.

An attachment is not admission.

Delivery is not acceptance.

Acceptance is not commitment.

A payload reference is not memory identity.

A connection failure is not forgetting.

MEM defines what memory means.

ACS carries that meaning without replacing it.

## Revision history

### Version 0.1 — 2026-07-16

- Replaced the planned MEM-0009 stub with the first normative working draft.
- Defined the public integration model between MEM roles and ACS participants, relationships, endpoints, ports, attachments, connections, signals, and payload references.
- Established foundational separation between communication state and memory state.
- Defined conceptual service-exposure, port-binding, operation-exchange, and layered-result records.
- Defined relationship-class use for cognitive, operational, bridge, and immune memory communication.
- Defined memory endpoint and port-contract expectations without prescribing a production catalog.
- Mapped memory communication onto ACS signal domains and intents.
- Distinguished memory operation identity, signal identity, correlation identity, attempt identity, delivery identity, connection generation, and payload-reference identity.
- Defined payload-reference authorization and transfer-result boundaries.
- Established layered communication and memory admission.
- Defined authorization, request, response, progress, finality, retry, duplicate, cancellation, timeout, and backpressure integration.
- Established failure-state translation without semantic collapse.
- Defined connection-lifecycle, degraded-operation, mediation, facade, local-fast-path, security, recovery, runtime, health, and immune boundaries.
- Added public implementation requirements, conformance expectations, prohibited interpretations, and open questions.
