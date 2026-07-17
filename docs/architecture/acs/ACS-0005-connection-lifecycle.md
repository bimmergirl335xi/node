# ACS-0005: Connection Lifecycle

| Field | Value |
|---|---|
| Specification | ACS-0005 |
| Title | Connection Lifecycle |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ACS-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Pending review |
| Depends on | ACS-0000, ACS-0001, ACS-0002, ACS-0003, ACS-0004 |
| Related specifications | MEM-0000 through MEM-0006 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in lifecycle separation and stale-instance protection; resumption, migration, and distributed transition coordination remain under review |

> **A connection is safe only when its creation, activation, degradation, recovery, and termination remain explicit, bounded, and subordinate to the relationship and authority that permit it.**

## Architectural-intent notice

This specification defines the public architectural lifecycle of ACS relationships, connection instances, bindings, and port attachments.

It is independently authored public architecture.

It is not produced by deleting, sanitizing, paraphrasing, or selectively redacting restricted architecture.

This specification defines the lifecycle distinctions required for interoperable Node-compatible communication without disclosing:

- production topology policy;
- proprietary relationship-formation algorithms;
- private trust thresholds;
- production recovery procedures;
- non-public routing policy;
- internal cognitive reorganization;
- restricted security credentials;
- operator-specific emergency procedures;
- production connection catalogs.

This specification does not prescribe:

- one network protocol;
- one handshake format;
- one secure-session protocol;
- exact timeout values;
- exact retry intervals;
- one leader-election system;
- one distributed-coordination algorithm;
- one connection table representation;
- one event-log format;
- one clock implementation;
- one routing algorithm;
- one operating-system socket model.

Implementations may use different mechanisms.

They must preserve the lifecycle distinctions and safety properties established here.

## 1. Purpose

ACS relationships may persist while:

- processes restart;
- hosts migrate;
- addresses change;
- transports fail;
- secure sessions renew;
- routes disappear;
- nodes partition;
- bindings are replaced;
- ports are suspended;
- participants enter recovery;
- communication becomes degraded.

A relationship cannot safely be equated with one socket or one live transport session.

Likewise, a connection cannot safely be treated as either simply **connected** or **disconnected**.

A real connection may be:

- forming;
- authenticating;
- negotiating compatibility;
- admitted but not yet active;
- active with restrictions;
- degraded;
- suspended;
- draining;
- recovering;
- closing;
- failed;
- expired;
- conflicting;
- unknown.

ACS-0005 defines how those states and transitions remain explicit.

It exists to prevent:

- stale connections from regaining authority;
- duplicate connection instances from appearing current;
- retries from creating uncontrolled resource pressure;
- transport failure from erasing logical relationships;
- reconnection from silently widening permissions;
- migration from duplicating active work;
- abrupt shutdown from abandoning resources;
- partial failure from being reported as healthy operation;
- previous authorization from becoming permanent authorization;
- connection state from redefining domain-specific operation state.

## 2. Scope

This specification governs:

- relationship lifecycle states required for connection management;
- connection-attempt lifecycle;
- connection-instance lifecycle;
- binding lifecycle;
- attachment lifecycle;
- connection identity;
- connection generations or epochs;
- initiation;
- compatibility negotiation;
- authentication coordination;
- admission;
- activation;
- health observation;
- degradation;
- suspension;
- draining;
- closure;
- failure;
- expiry;
- re-establishment;
- resumption;
- rebinding;
- migration;
- handoff;
- partition behavior;
- stale-instance suppression;
- duplicate connection handling;
- in-flight communication during transitions;
- retry and backoff;
- lifecycle resource limits;
- lifecycle authority;
- lifecycle observability;
- interaction with endpoints, ports, signals, relationships, MEM, security, runtime, health, and immune systems;
- public conformance expectations.

This specification does not define:

- detailed relationship-admission algorithms;
- exact resource-budget allocation;
- trust-scoring algorithms;
- cryptographic algorithms;
- signal schemas;
- memory-operation lifecycles;
- physical-node provisioning;
- software installation;
- propagation authorization;
- production topology adaptation;
- cognitive relationship-strength algorithms;
- physical safety procedures.

Those subjects belong to later ACS specifications, MEM, adjacent architecture, or implementation profiles.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements.

The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification.

The word **may** describes permitted behavior.

An implementation does not conform merely because it reports a boolean value named `connected`.

Its observable behavior must preserve the lifecycle states and distinctions required by this specification.

## 4. Core terminology

### 4.1 Relationship

A relationship is a persistent logical association between identified participants.

It records:

- why association exists;
- which relationship class applies;
- which broad interaction is eligible;
- which governing constraints apply.

A relationship may exist without an active connection.

### 4.2 Connection

A connection is a bounded runtime mechanism through which an eligible relationship may exchange permitted signals, payload references, or operational information.

A connection is not:

- the relationship;
- the endpoint;
- the port;
- the binding;
- the secure session;
- the transport socket;
- the domain-specific operation carried through it.

### 4.3 Connection attempt

A connection attempt is a bounded effort to create or re-establish a connection instance.

An attempt may include:

- locating an endpoint;
- selecting a binding;
- establishing transport;
- authenticating participants;
- negotiating compatibility;
- requesting admission;
- creating attachments;
- activating communication.

A failed attempt does not necessarily mean the relationship is invalid.

### 4.4 Connection instance

A connection instance is one identified runtime realization of connection state between participants.

A new instance may be created after:

- ordinary establishment;
- reconnect;
- migration;
- secure-session replacement;
- recovery;
- failover;
- handoff.

Connection-instance identity shall not be confused with relationship identity.

### 4.5 Connection generation

A connection generation, epoch, or equivalent continuity marker distinguishes current connection authority from stale instances and stale traffic.

A generation may change when:

- a new connection instance becomes authoritative;
- a suspended connection is materially resumed;
- a binding handoff changes continuity assumptions;
- ownership or relationship authority changes;
- recovery invalidates earlier runtime state.

The exact representation is implementation-defined.

### 4.6 Lifecycle event

A lifecycle event is an identified observation, request, decision, or transition concerning:

- a relationship;
- a connection attempt;
- a connection instance;
- a binding;
- an attachment.

A lifecycle event does not automatically prove that the requested transition occurred.

### 4.7 Lifecycle authority

Lifecycle authority is the explicitly granted permission to approve or execute a defined lifecycle transition.

Examples may include authority to:

- admit a connection;
- suspend an attachment;
- drain a connection;
- revoke a connection;
- replace a binding;
- retire a relationship.

Lifecycle authority is operation-specific.

### 4.8 Re-establishment

Re-establishment creates a new connection instance for an existing eligible relationship after an earlier connection was lost, closed, expired, or made unusable.

Re-establishment does not reuse the old connection instance merely because the same participants return.

### 4.9 Resumption

Resumption continues a suspended or interrupted connection context when sufficient continuity can be established.

Resumption may preserve selected:

- attachment state;
- sequence state;
- flow-control state;
- in-flight operation references;
- secure-session state.

Resumption must not preserve stale authority.

### 4.10 Rebinding

Rebinding changes the concrete runtime binding carrying a connection or endpoint without inherently changing the relationship or semantic port contract.

Rebinding may occur during:

- address change;
- transport replacement;
- local-to-remote transition;
- route migration;
- failover;
- host migration.

### 4.11 Handoff

A handoff is a coordinated transfer of active connection responsibility from one binding, runtime instance, host, or steward to another.

A handoff may be:

- make-before-break;
- break-before-make;
- overlapping under bounded coordination;
- staged through a mediator.

### 4.12 Drain

Drain is a bounded lifecycle period during which:

- new work is restricted or rejected;
- admitted in-flight work is completed, cancelled, transferred, or reconciled;
- acknowledgements and final results may still be exchanged;
- resources are prepared for release.

Drain is not ordinary active operation.

### 4.13 Suspension

Suspension temporarily disables some or all connection use without necessarily destroying:

- relationship identity;
- connection history;
- attachments;
- endpoint identity;
- audit evidence.

### 4.14 Retirement

Retirement is a governed terminal lifecycle decision ending the continuing eligibility of an object.

A retired relationship, connection, port attachment, or endpoint must not silently reactivate.

## 5. Foundational lifecycle separation

The following concepts remain distinct:

```text
Relationship lifecycle
    governs continuing logical association

Connection-attempt lifecycle
    governs one bounded effort to create runtime communication

Connection-instance lifecycle
    governs one active runtime communication context

Binding lifecycle
    governs concrete transport and placement resources

Attachment lifecycle
    governs scoped permission to use a port

Signal lifecycle
    governs one communicated semantic unit

Domain-operation lifecycle
    governs the memory, runtime, cognitive, immune, or other operation carried
```

No implementation shall silently use one lifecycle state as proof of another.

For example:

- an active relationship does not prove an active connection;
- an active connection does not prove an active attachment;
- an active attachment does not prove signal acceptance;
- signal delivery does not prove operation completion;
- connection failure does not prove relationship retirement;
- secure-session expiry does not necessarily retire a relationship;
- process restart does not prove connection continuity.

## 6. Lifecycle dimensions

A conforming implementation shall preserve at least the following conceptual lifecycle dimensions.

### 6.1 Relationship state

Describes whether the continuing logical association is:

- proposed;
- established;
- restricted;
- degraded;
- suspended;
- retiring;
- retired;
- rejected;
- conflicting;
- unknown.

### 6.2 Connection-attempt state

Describes whether one attempt is:

- pending;
- locating;
- binding;
- authenticating;
- negotiating;
- requesting admission;
- activating;
- completed;
- rejected;
- failed;
- cancelled;
- expired;
- conflicting;
- unknown.

### 6.3 Connection-instance state

Describes whether one instance is:

- forming;
- admitted;
- active;
- active with restrictions;
- degraded;
- suspended;
- draining;
- recovering;
- closing;
- closed;
- failed;
- expired;
- conflicting;
- unknown.

### 6.4 Binding state

Describes whether a concrete binding is:

- proposed;
- establishing;
- active;
- degraded;
- blocked;
- migrating;
- draining;
- closed;
- failed;
- stale;
- conflicting;
- unknown.

### 6.5 Attachment state

Describes whether scoped port use is:

- proposed;
- admitted;
- active;
- restricted;
- suspended;
- draining;
- revoked;
- expired;
- closed;
- conflicting;
- unknown.

### 6.6 Continuity confidence

Describes whether continuity with previous logical or runtime state is:

- established;
- provisionally established;
- unverified;
- stale;
- conflicting;
- impossible;
- unknown.

These dimensions may be represented together in implementation.

Their meanings must remain independently recoverable.

## 7. Relationship lifecycle

### 7.1 Proposed

A proposed relationship has been requested or suggested but has not been established.

A proposal does not grant:

- connection admission;
- attachment;
- authority;
- port visibility;
- resource reservation.

### 7.2 Established

An established relationship is a currently recognized logical association with an assigned relationship class and declared scope.

An established relationship may have:

- no active connection;
- one active connection;
- several permitted connection instances;
- suspended communication;
- degraded reachability.

### 7.3 Restricted

A restricted relationship remains established but has reduced:

- eligible ports;
- signal families;
- direction;
- rate;
- authority;
- attachment scope;
- connection count.

Restriction must remain explicit.

### 7.4 Degraded

A degraded relationship cannot currently satisfy all of its declared interaction expectations.

Possible causes include:

- incomplete participant availability;
- unavailable endpoints;
- reduced trust;
- reduced resources;
- required mediation loss;
- partial compatibility;
- security restrictions.

Degradation does not automatically retire the relationship.

### 7.5 Suspended

A suspended relationship remains logically recorded but may not create or use ordinary connections until reactivated.

Suspension may preserve:

- relationship history;
- previous attachment records;
- lifecycle evidence;
- eligibility for recovery.

### 7.6 Retiring

A retiring relationship is undergoing governed termination.

During retirement:

- new connections should normally be rejected;
- existing connections may drain;
- attachments may be revoked or closed;
- final evidence may be retained;
- domain-specific operations must be reconciled separately.

### 7.7 Retired

A retired relationship is no longer eligible for ordinary connection establishment.

Retirement shall not be reversed merely because:

- a former participant returns;
- an old credential becomes available;
- a stale connection sends traffic;
- a previous endpoint reappears.

A new relationship may be proposed under current policy.

### 7.8 Rejected

A rejected relationship proposal did not become established.

Rejection may be temporary or final according to policy.

### 7.9 Conflicting

A relationship is conflicting when incompatible valid-looking claims exist concerning:

- its identity;
- class;
- participants;
- scope;
- authority;
- current generation;
- retirement state.

Conflict must remain explicit until safely resolved.

### 7.10 Unknown

Relationship state is unknown when evidence is insufficient to establish its current validity or status.

Unknown shall not become implied establishment or implied retirement.

## 8. Connection-attempt lifecycle

### 8.1 Attempts are identified

Every connection attempt shall possess sufficient identity to distinguish it from:

- another concurrent attempt;
- a retry of the same attempt;
- a previous failed attempt;
- a stale delayed response;
- a different relationship’s attempt.

### 8.2 Attempts are bounded

Every attempt shall have explicit limits for:

- duration;
- retries;
- discovery work;
- authentication work;
- compatibility negotiation;
- admission processing;
- resource reservation;
- concurrent duplicate attempts.

### 8.3 Pending and locating

An attempt may begin while locating:

- an eligible endpoint;
- an acceptable port;
- a permitted mediator;
- a supported binding;
- compatible versions.

Discovery does not grant admission.

### 8.4 Binding establishment

Transport or local runtime resources may be established before semantic admission completes.

A successfully established socket, session, or queue does not mean the ACS connection is active.

### 8.5 Authentication

Required participant identity and continuity evidence shall be evaluated before privileged activation.

Authentication success does not independently establish:

- relationship validity;
- port compatibility;
- attachment admission;
- directive authority;
- connection activation.

### 8.6 Compatibility negotiation

The attempt shall determine whether participants support compatible:

- endpoint expectations;
- port contracts;
- signal versions;
- payload-reference policies;
- lifecycle behavior;
- resource and delivery expectations.

Compatibility must not be inferred merely from a shared transport protocol.

### 8.7 Admission request

The attempt may request admission for:

- one relationship;
- one connection instance;
- one or more scoped attachments;
- bounded resources;
- selected port versions.

Admission may be:

- granted;
- granted with restrictions;
- deferred;
- challenged;
- rejected;
- unresolved.

### 8.8 Activation

A connection becomes active only after all mandatory:

- identity;
- relationship;
- compatibility;
- security;
- admission;
- binding;
- attachment;
- resource;

conditions required by its profile have been satisfied.

### 8.9 Attempt completion

A completed attempt shall report which connection instance, generation, bindings, and attachments became active.

Successful attempt completion shall not be represented by transport establishment alone.

### 8.10 Failed attempts

A failed attempt shall release or reconcile temporary:

- bindings;
- reservations;
- challenge state;
- partial attachments;
- secure-session state;
- pending lifecycle events.

Failure shall not leave an unbounded half-open connection.

## 9. Connection identity and generations

### 9.1 Instance identity

Every connection instance shall have identity sufficient to distinguish it from previous and concurrent instances.

The identity may be scoped to:

- one relationship;
- one endpoint pair;
- one trust domain;
- one deployment;
- another declared namespace.

### 9.2 Relationship identity remains separate

A re-established connection for the same relationship shall not automatically reuse the previous connection-instance identity.

The persistent identity belongs to the relationship.

The runtime identity belongs to the connection instance.

### 9.3 Generation marker

A connection generation or equivalent marker shall allow recipients to reject stale traffic and stale lifecycle events.

The marker must change when required to prevent ambiguity after:

- re-establishment;
- recovery;
- conflicting active instances;
- material resumption;
- authority change;
- attachment replacement;
- binding handoff.

### 9.4 Generation ordering

Generation values need not form one universal global counter.

They must provide sufficient evidence to distinguish:

- current;
- stale;
- concurrent;
- conflicting;
- unknown;

connection authority within their scope.

### 9.5 Stale-instance behavior

A stale connection instance shall not:

- regain attachment authority;
- submit ordinary traffic;
- issue directives;
- overwrite current flow-control state;
- close or retire the current instance;
- reclaim resources assigned to a newer generation.

### 9.6 Delayed traffic

Traffic delayed from a previous generation must be:

- rejected;
- identified as stale;
- reconciled through subtype-specific duplicate behavior;
- or explicitly treated as unknown.

It must not silently enter the current connection context.

## 10. Connection activation

### 10.1 Activation is explicit

Connection activation shall be an explicit lifecycle transition.

A participant shall not assume active state merely because it begins receiving traffic.

### 10.2 Activation evidence

Activation evidence should identify:

- relationship identity;
- connection-instance identity;
- generation;
- participant identities;
- endpoint identities;
- binding profile;
- active attachments;
- contract versions;
- resource scope;
- applicable security state;
- activation time or ordering evidence.

### 10.3 Restricted activation

A connection may activate with restrictions.

Restrictions may include:

- observation-only behavior;
- request-only behavior;
- reduced rates;
- reduced payload access;
- restricted ports;
- mandatory mediation;
- temporary validity;
- no directive authority.

The restricted scope must be distinguishable from the requested scope.

### 10.4 Activation does not guarantee service success

An active connection establishes permitted communication capacity.

It does not prove that:

- every endpoint service is healthy;
- every request can be fulfilled;
- memory is available;
- payload data can be obtained;
- resources remain sufficient;
- every signal will be accepted.

## 11. Active operation

### 11.1 State maintenance

While active, a connection shall maintain enough state to enforce:

- current generation;
- active attachments;
- resource limits;
- sequence or duplicate policy where applicable;
- lifecycle restrictions;
- current security state;
- current binding state.

### 11.2 Active does not mean unrestricted

An active connection remains limited by:

- relationship class;
- port contract;
- attachment scope;
- capabilities;
- signal admission;
- resources;
- security;
- current lifecycle state.

### 11.3 Activity is not health

Recent traffic does not independently prove that the connection is healthy.

A connection may exchange keepalives while:

- semantic services are unavailable;
- attachments are revoked;
- one direction is blocked;
- payload access is unavailable;
- resource limits are exhausted.

### 11.4 Silence is not failure

Absence of traffic does not independently prove failure.

Some valid connections may be:

- event-driven;
- dormant;
- low-frequency;
- receive-only;
- awaiting external conditions.

Failure detection must follow the declared connection profile.

## 12. Liveness, health, and readiness

### 12.1 Liveness

Liveness describes whether evidence indicates that a participant or binding continues to respond within applicable expectations.

### 12.2 Health

Health describes whether the connection and its dependencies operate within acceptable conditions.

### 12.3 Readiness

Readiness describes whether the connection can currently support a defined class of communication.

### 12.4 Separation

The following remain distinct:

```text
Liveness
    participant or path appears responsive

Health
    connection dependencies appear acceptable

Readiness
    a declared interaction can presently be attempted

Domain readiness
    the carried service can perform its semantic operation
```

A live binding may not be ready.

A ready ACS connection may carry a domain service that is unavailable.

### 12.5 Heartbeats and keepalives

Heartbeats or keepalives may contribute liveness evidence.

They shall not independently:

- renew expired authority;
- reactivate suspended attachments;
- prove semantic service readiness;
- prove relationship validity;
- prevent required credential revalidation.

### 12.6 Stale observations

Liveness and health observations expire.

An old successful check must not remain current indefinitely.

## 13. Degraded operation

### 13.1 Explicit degradation

A connection shall enter explicit degraded state when it remains usable but cannot satisfy all declared expectations.

### 13.2 Degradation dimensions

Degradation may affect:

- one direction;
- one attachment;
- one port;
- payload access;
- latency;
- bandwidth;
- reliability;
- security profile;
- mediator availability;
- compatibility;
- resource capacity.

### 13.3 Reduced operation

A degraded connection may continue with a narrower profile when:

- the reduced behavior is defined;
- participants can identify the reduction;
- safety and authorization remain valid;
- hard requirements are not silently waived.

### 13.4 No authority expansion

Degradation shall not grant additional authority merely because ordinary services are unavailable.

### 13.5 No false health

A degraded connection must not be reported as fully healthy merely because some traffic continues.

## 14. Suspension

### 14.1 Purpose

Suspension temporarily prevents some or all connection activity while preserving enough state for:

- review;
- recovery;
- controlled resumption;
- evidence retention;
- safe drain.

### 14.2 Suspension scope

Suspension may apply to:

- one direction;
- one attachment;
- one port;
- all ordinary traffic;
- all traffic except health and recovery;
- the entire connection instance;
- all connections belonging to a relationship.

### 14.3 Suspension authority

Suspension shall require authority appropriate to its scope.

A participant shall not suspend unrelated connections merely because it can observe them.

### 14.4 Behavior during suspension

Suspension policy shall define treatment of:

- new signals;
- queued signals;
- in-flight payloads;
- outstanding requests;
- partial directives;
- acknowledgements;
- keepalives;
- recovery traffic.

### 14.5 Suspension timeout

Suspension may be time-bounded or condition-bounded.

Expiry of a suspension period does not automatically restore authority unless reactivation requirements are satisfied.

## 15. Draining

### 15.1 Purpose

Drain permits orderly transition away from active operation.

Drain may precede:

- planned closure;
- migration;
- relationship retirement;
- port retirement;
- secure-session replacement;
- maintenance;
- resource reclamation.

### 15.2 New work

A draining connection should normally reject or redirect new ordinary work.

It may continue to accept:

- cancellation;
- acknowledgements;
- final results;
- lifecycle coordination;
- health and recovery communication.

### 15.3 In-flight work

Drain policy shall define whether in-flight work is:

- completed;
- cancelled;
- transferred;
- allowed to expire;
- returned as indeterminate;
- reconciled after re-establishment.

### 15.4 Drain is bounded

Drain shall have explicit limits.

A connection shall not remain indefinitely draining because one operation never completes.

### 15.5 Drain completion

Drain completes when required in-flight work has reached an allowed terminal or transferable state and connection resources may be released safely.

## 16. Closure

### 16.1 Graceful closure

Graceful closure follows an agreed or locally authorized transition in which:

- new work stops;
- in-flight work is handled;
- attachments close;
- final state is recorded;
- resources are released.

### 16.2 Unilateral closure

A participant may close its side of a connection when authorized or required by:

- local failure;
- security policy;
- lifecycle policy;
- resource emergency;
- operator instruction;
- endpoint retirement.

The other side may observe closure without having agreed to it.

### 16.3 Closure is not relationship retirement

Closing a connection instance does not automatically retire the relationship.

The relationship may remain eligible for later re-establishment.

### 16.4 Closure is not operation completion

Closing a connection does not prove that carried:

- requests;
- directives;
- payload transfers;
- memory operations;
- lifecycle operations;

completed or failed.

Each operation must retain its own outcome.

### 16.5 Final state

Closed instances shall retain enough bounded evidence to prevent:

- stale reactivation;
- duplicate generation;
- ambiguous in-flight outcomes;
- identifier reuse;
- resource leakage.

## 17. Failure

### 17.1 Failure definition

A connection fails when evidence establishes that it can no longer continue according to its required profile and no valid active transition has preserved continuity.

### 17.2 Possible causes

Failure may result from:

- binding loss;
- authentication failure;
- credential revocation;
- protocol incompatibility;
- participant failure;
- mediator failure;
- resource exhaustion;
- contract violation;
- security incident;
- unrecoverable state divergence;
- lifecycle conflict.

### 17.3 Failure versus unknown

Failure means evidence establishes inability to continue.

Unknown means evidence is insufficient to determine whether continuation is possible.

Timeout alone may contribute evidence.

It does not always prove permanent failure.

### 17.4 Failure scope

Failure may affect:

- one binding;
- one direction;
- one attachment;
- one port;
- one connection instance;
- every connection for a relationship.

The smallest accurate scope should be represented.

### 17.5 Failure does not grant takeover

A replacement instance shall not become authoritative solely because the previous instance stopped responding.

Takeover requires current admission and continuity evidence.

## 18. Expiry

### 18.1 Time- or epoch-bounded state

Connections, attachments, capabilities, and attempts may expire according to:

- time;
- generation;
- credential epoch;
- policy epoch;
- relationship state;
- explicit use count;
- another bounded condition.

### 18.2 Expiry is explicit

Expired state shall not remain active merely because no component noticed the expiry immediately.

### 18.3 Renewal

Renewal is a new governed lifecycle decision.

Repeated use or successful traffic does not automatically renew:

- authority;
- attachment;
- secure-session validity;
- connection generation;
- relationship eligibility.

### 18.4 Expired traffic

Traffic from an expired instance shall be rejected or treated as stale.

It shall not reactivate the instance.

## 19. Re-establishment

### 19.1 New instance

Re-establishment normally creates a new connection instance and generation for an existing eligible relationship.

### 19.2 Revalidation

Re-establishment shall revalidate applicable:

- participant identity;
- relationship state;
- endpoint continuity;
- port compatibility;
- attachment eligibility;
- capabilities;
- security state;
- resource limits.

### 19.3 Previous success is insufficient

A previous connection’s successful operation does not guarantee current admission.

### 19.4 Preserved relationship state

The established relationship may preserve:

- class;
- history;
- permitted purpose;
- bounded trust evidence;
- previous compatibility knowledge.

Preserved evidence must still be current enough for use.

### 19.5 Re-establishment limits

Repeated re-establishment shall be bounded through:

- retry limits;
- backoff;
- attempt budgets;
- source limits;
- lifecycle policy.

## 20. Resumption

### 20.1 Stronger requirement than reconnect

Resumption claims that selected state from a previous connection context remains valid.

It therefore requires stronger continuity evidence than ordinary re-establishment.

### 20.2 Potentially resumable state

A profile may permit resumption of:

- flow-control credits;
- stream positions;
- attachment identifiers;
- sequence windows;
- pending request correlations;
- partial payload transfer state.

### 20.3 Non-resumable authority

Authority that has:

- expired;
- been revoked;
- changed generation;
- changed ownership scope;
- entered conflict;
- become unverifiable;

shall not be resumed.

### 20.4 Ambiguous resumption

When the system cannot establish whether the previous connection remained active elsewhere, resumption shall not create silent split authority.

The state must remain:

- conflicting;
- suspended;
- restricted;
- or require a new instance with reconciliation.

### 20.5 Resumption outcome

A resumed connection shall disclose which state was:

- preserved;
- discarded;
- revalidated;
- reset;
- left indeterminate.

## 21. Rebinding and migration

### 21.1 Rebinding principle

A connection may replace its concrete binding without changing the relationship or port meaning.

### 21.2 Continuity evidence

Rebinding shall establish that the new binding represents the intended participants and current connection generation.

### 21.3 Old-binding invalidation

A replaced binding shall be:

- closed;
- drained;
- marked stale;
- prevented from carrying current-generation traffic.

### 21.4 Make-before-break

A connection may temporarily use old and new bindings during coordinated handoff.

Overlap must be bounded and must prevent:

- duplicate semantic delivery;
- split flow-control state;
- authority ambiguity;
- unbounded resource duplication.

### 21.5 Break-before-make

A connection may close the old binding before establishing the new one.

Resulting interruption must remain explicit.

### 21.6 Host migration

Moving a participant implementation to another host does not automatically change:

- relationship identity;
- endpoint ownership;
- port contracts;
- authority.

It may require:

- new binding;
- new secure session;
- new connection generation;
- attachment revalidation.

### 21.7 Migration failure

If migration cannot establish continuity, the new instance shall not impersonate the old one.

A new connection may be established under explicit recovery policy.

## 22. Handoff

### 22.1 Coordinated transition

A handoff shall define:

- source instance;
- destination instance;
- relationship identity;
- current and next generation;
- attachment treatment;
- in-flight work treatment;
- old-binding treatment;
- completion evidence;
- rollback behavior.

### 22.2 Handoff authority

A participant capable of carrying traffic is not automatically authorized to receive a handoff.

### 22.3 Partial handoff

A handoff may transfer only:

- selected ports;
- selected attachments;
- one direction;
- selected streams;
- selected resource reservations.

Partial scope must remain explicit.

### 22.4 Handoff completion

Completion requires evidence that the destination is active within the granted scope and the source can no longer exercise conflicting current authority.

### 22.5 Failed handoff

A failed handoff shall not leave both source and destination silently authoritative.

## 23. Partition behavior

### 23.1 Partition does not prove retirement

Loss of communication between participants does not prove that:

- the relationship ended;
- the remote participant failed permanently;
- the endpoint disappeared;
- the remote connection was closed;
- domain state was deleted.

### 23.2 Partitioned authority

When both sides may continue operating, the connection profile shall define whether:

- both may retain restricted local operation;
- one side must suspend;
- all privileged operations stop;
- only observations continue;
- reconciliation is required before resumption.

### 23.3 No silent split authority

A partition shall not silently create two independently authoritative generations for operations that require one current connection context.

### 23.4 Reconciliation

After partition, participants shall reconcile:

- connection generation;
- lifecycle events;
- attachment changes;
- capability revocations;
- in-flight signal state;
- closure claims;
- handoff claims;
- security state.

### 23.5 Partition limits

Retry, buffering, retention, and reconnect attempts during partition shall remain bounded.

## 24. Duplicate and concurrent connections

### 24.1 Multiple permitted instances

A relationship may permit several concurrent connection instances when explicitly declared.

Examples may include:

- separate local and remote paths;
- parallel transport paths;
- independent port groups;
- redundant active paths;
- read-only and directive-separated connections.

### 24.2 Duplicate ambiguity

Unexpected concurrent instances claiming the same exclusive scope shall enter conflict or require arbitration.

### 24.3 No first-wins rule

The system shall not select an authoritative instance solely because it:

- connected first;
- connected last;
- has lower latency;
- uses a preferred host;
- carries more traffic;
- has more compute capacity.

### 24.4 Scope distinction

Concurrent connections must identify distinct scope or coordinated shared behavior.

### 24.5 Duplicate delivery

When several paths may carry one semantic signal, duplicate handling shall preserve ACS-0003 signal identity and subtype semantics.

Multiple deliveries shall not become multiple independent evidence items automatically.

## 25. Attachment lifecycle

### 25.1 Proposed

A proposed attachment requests scoped use of a port.

Proposal does not grant use.

### 25.2 Admitted

An admitted attachment has passed required communication admission but may still await connection activation or additional security conditions.

### 25.3 Active

An active attachment permits attempted port interaction within its declared scope.

### 25.4 Restricted

A restricted attachment remains active with narrowed:

- directions;
- signal families;
- resources;
- authority;
- payload access;
- validity.

### 25.5 Suspended

A suspended attachment temporarily permits no ordinary use within its suspended scope.

### 25.6 Draining

A draining attachment rejects new work while reconciling admitted in-flight interaction.

### 25.7 Revoked

A revoked attachment is no longer authorized.

Revocation does not depend on voluntary cooperation by the holder.

### 25.8 Expired

An expired attachment has exceeded its validity condition.

### 25.9 Closed

A closed attachment has ended normally or through connection closure.

### 25.10 Migration and renewal

Attachments may be:

- preserved;
- revalidated;
- narrowed;
- replaced;
- rejected;

during reconnection, resumption, rebinding, or migration.

They shall not silently inherit broader authority on a new instance.

## 26. In-flight signals and operations

### 26.1 Connection state is not operation state

A signal or domain operation may remain:

- queued;
- delivered;
- accepted;
- executing;
- completed;
- failed;
- cancelled;
- indeterminate;

independently of connection state.

### 26.2 Closure handling

Connection profiles shall define treatment of in-flight:

- signals;
- requests;
- directives;
- payload transfers;
- acknowledgements;
- responses.

### 26.3 Indeterminate outcomes

When a connection fails after an operation may have executed but before an outcome is established, the operation shall remain indeterminate until reconciled.

It must not be guessed as success or failure.

### 26.4 Retry identity

Retried communication shall preserve enough semantic identity to distinguish:

- retry of the same signal or operation;
- a new signal or operation;
- replay;
- stale generation traffic.

### 26.5 Exactly-once assumptions

A conforming implementation shall not depend on transport-level exactly-once delivery for semantic correctness.

### 26.6 Directive safety

Directive-capable operations shall define behavior for:

- duplicate delivery;
- partial execution;
- lost response;
- connection failure;
- retry;
- cancellation;
- generation change.

## 27. Retry and backoff

### 27.1 Retries are bounded

Connection establishment and recovery retries shall have explicit:

- maximum count or budget;
- delay;
- backoff;
- jitter where appropriate;
- cancellation condition;
- terminal behavior.

### 27.2 Retry is not entitlement

A failed attempt does not gain increased authority by being retried repeatedly.

### 27.3 Backoff

Backoff should reduce synchronized retry storms and repeated pressure on unavailable participants.

### 27.4 Retry reset

A retry budget shall not reset indefinitely through minor representation changes, alternate addresses, or mediator changes.

### 27.5 Manual or operator retry

Operator-authorized retry may override some soft retry policy.

It shall not bypass hard:

- authentication;
- authorization;
- safety;
- resource;
- compatibility;

requirements.

## 28. Lifecycle resource boundaries

### 28.1 Attempts consume resources

Lifecycle processing consumes:

- memory;
- connection-table entries;
- authentication work;
- cryptographic work;
- discovery work;
- queue space;
- mediator capacity;
- audit capacity.

### 28.2 Half-open limits

Implementations shall bound half-open or forming connections.

### 28.3 Per-source limits

One source shall not consume all connection-attempt capacity through repeated formation, challenge, failure, or reconnect.

### 28.4 State retention

Closed and failed connection evidence shall be retained only as long and in as much detail as required for:

- stale-instance rejection;
- duplicate handling;
- audit;
- reconciliation;
- security;
- conformance.

Retention shall remain bounded.

### 28.5 Recovery reserve

Resource exhaustion shall not consume all capacity required to:

- close unsafe connections;
- revoke attachments;
- process security events;
- perform bounded recovery;
- report lifecycle state.

### 28.6 Reconnect storms

The system shall defend against reconnect storms through:

- backoff;
- admission limits;
- per-relationship budgets;
- grouping;
- randomized retry;
- degraded operation;
- operator intervention where needed.

## 29. Lifecycle authority

### 29.1 Operation-specific authority

Authority to perform one lifecycle transition does not grant authority to perform all lifecycle transitions.

For example:

- authority to suspend does not imply authority to retire;
- authority to close a binding does not imply authority to retire a relationship;
- authority to admit an attachment does not imply authority to expand it;
- authority to migrate a connection does not imply authority to change its port contracts.

### 29.2 Participant authority

A participant may ordinarily close or restrict its own local participation according to policy.

It does not automatically control the remote participant’s:

- relationship identity;
- endpoint ownership;
- physical process;
- node lifecycle.

### 29.3 Owner and steward authority

Endpoint owners and stewards may govern connection behavior only within their explicit scope.

Ownership does not create unlimited lifecycle authority.

### 29.4 Security authority

Security services may:

- deny activation;
- require reauthentication;
- revoke capabilities;
- invalidate secure sessions;
- request restriction or closure;

within their granted scope.

Security authority does not automatically retire logical relationships.

### 29.5 Immune authority

Immune systems may observe lifecycle anomalies and recommend or request bounded restriction.

Observation or suspicion does not automatically grant destructive authority.

### 29.6 Operator authority

Operator actions affecting connection lifecycle shall be:

- authenticated;
- scoped;
- attributable;
- distinguishable from ordinary participant actions.

## 30. Lifecycle events and ordering

### 30.1 Identified events

Material lifecycle events should possess identity sufficient for:

- duplicate detection;
- ordering;
- replay handling;
- audit;
- reconciliation.

### 30.2 Event ordering

A universal total order is not required for every lifecycle event.

The implementation must preserve enough ordering to prevent unsafe outcomes such as:

- activation after retirement;
- stale resumption after revocation;
- old binding becoming current after handoff;
- closed attachment processing new traffic.

### 30.3 Delayed events

Delayed lifecycle events shall be evaluated against current:

- relationship state;
- connection generation;
- attachment state;
- policy epoch;
- authority.

### 30.4 Duplicate events

Repeated delivery of one lifecycle event shall not produce repeated semantic transitions when the transition is intended to be idempotent.

### 30.5 Conflicting events

Incompatible lifecycle events shall remain explicit until precedence or reconciliation policy resolves them.

## 31. Time and deadlines

### 31.1 Time is evidence

Clock readings and timeouts contribute lifecycle evidence.

They do not independently prove:

- remote failure;
- relationship retirement;
- authority transfer;
- operation failure.

### 31.2 Deadlines

Lifecycle operations may define:

- attempt deadlines;
- acknowledgement deadlines;
- drain deadlines;
- suspension periods;
- attachment expiry;
- retry windows.

### 31.3 Clock uncertainty

Implementations using clocks shall account for relevant:

- skew;
- drift;
- missing synchronization;
- restart;
- rollback;
- monotonicity.

### 31.4 Expiry interpretation

Expiry must use a model that prevents stale authority from becoming current due to clock ambiguity.

## 32. Security lifecycle coordination

### 32.1 Separate but coordinated lifecycles

Connection lifecycle and secure-session lifecycle are distinct.

A connection may require:

- a new secure session;
- secure-session renewal;
- credential rotation;
- reauthentication;
- key replacement;

without necessarily retiring the relationship.

### 32.2 Session loss

Loss of a secure session may suspend or close the connection.

It does not automatically prove participant identity loss or relationship retirement.

### 32.3 Revocation

Credential or capability revocation shall affect current connections according to explicit policy.

Cached authorization must not remain valid indefinitely after revocation.

### 32.4 Downgrade prevention

Reconnection, fallback, or recovery shall not silently select weaker security than the connection profile permits.

### 32.5 Stale credentials

A returning participant shall not regain previous authority solely by presenting credentials that were once valid.

## 33. Health and immune interaction

### 33.1 Lifecycle evidence

Health and immune systems may observe bounded evidence concerning:

- repeated failed formation;
- half-open exhaustion;
- conflicting generations;
- stale-instance traffic;
- ignored suspension;
- failed handoff;
- reconnect storms;
- abnormal closure;
- repeated authentication failure;
- lifecycle-event replay.

### 33.2 Observation is not lifecycle authority

Observation does not independently permit:

- relationship retirement;
- endpoint destruction;
- physical-node restart;
- credential revocation;
- permanent quarantine.

### 33.3 Resource failure is not automatically malicious

Repeated failure may result from:

- network instability;
- software defects;
- incompatible versions;
- resource shortage;
- misconfiguration;
- compromise;
- unknown causes.

Evidence must preserve uncertainty.

## 34. Boundary with ACS-0002 relationships

ACS-0002 defines why a relationship exists and which primary class it possesses.

ACS-0005 defines how that relationship becomes operational through bounded connection instances and how those instances end or recover.

Relationship class may influence:

- eligible lifecycle profile;
- required security;
- permitted number of connections;
- degradation behavior;
- recovery behavior;
- protected capacity.

Relationship class does not eliminate lifecycle validation.

## 35. Boundary with ACS-0003 signals

ACS-0003 defines signal meaning, identity, freshness, duplicate behavior, acceptance, and response semantics.

ACS-0005 defines whether a connection instance may currently carry the signal.

The following remain distinct:

1. connection activation is not signal acceptance;
2. connection generation is not signal identity;
3. connection retry is not automatically a new signal;
4. connection closure is not signal expiry;
5. signal acknowledgement is not connection health;
6. duplicate paths must preserve signal duplicate semantics;
7. lifecycle events carried as signals remain governed lifecycle operations.

## 36. Boundary with ACS-0004 endpoints and ports

ACS-0004 defines:

- endpoint identity;
- port identity;
- port contracts;
- bindings;
- attachments;
- direction;
- visibility;
- mediation.

ACS-0005 defines:

- how bindings become active and are replaced;
- how attachments become active, restricted, suspended, revoked, and closed;
- how connection instances activate, degrade, recover, and terminate;
- how continuity is preserved across lifecycle transitions.

An endpoint may remain valid while every connection to it is closed.

A port may remain available while one attachment is revoked.

A binding may fail while another binding preserves the connection.

## 37. Boundary with MEM

### 37.1 Connection lifecycle does not define memory lifecycle

Connection:

- formation;
- activation;
- failure;
- retry;
- closure;
- re-establishment;

does not define whether a memory operation was:

- admitted;
- committed;
- made durable;
- retrieved;
- deleted;
- repaired;
- completed.

### 37.2 Memory operation identity

A memory operation capable of retry shall retain MEM operation identity independently of:

- connection instance;
- connection generation;
- transport message;
- binding;
- secure session.

### 37.3 Connection loss

Connection loss does not prove:

- memory absence;
- memory deletion;
- retrieval failure;
- storage failure;
- operation cancellation.

### 37.4 Reconnection

Reconnection shall not silently duplicate:

- storage proposals;
- memory mutations;
- deletion;
- repair;
- retention changes.

MEM operation contracts determine reconciliation.

### 37.5 Role availability

A healthy ACS connection to a memory service does not prove that the required memory role is semantically available.

Neither ACS nor MEM may absorb the other’s lifecycle authority.

## 38. Boundary with runtime and physical nodes

### 38.1 Runtime placement

Runtime systems may:

- start processes;
- select hosts;
- establish bindings;
- migrate services;
- restart failed implementations.

Runtime action does not independently establish ACS connection continuity or authority.

### 38.2 Process restart

Process restart normally invalidates process-local connection state unless continuity is explicitly recovered.

### 38.3 Physical-node failure

Physical-node failure may cause connection failure.

It does not automatically:

- retire relationships;
- transfer endpoint ownership;
- grant takeover;
- prove participant destruction.

### 38.4 Physical-node control

Connection lifecycle services do not independently possess authority to:

- reboot;
- erase;
- rebuild;
- quarantine;
- destroy;

physical nodes.

## 39. Reduced-operation profiles

A lifecycle profile may define reduced modes such as:

- observation-only;
- request-only;
- no payload transfer;
- no new attachments;
- existing attachments only;
- health and recovery only;
- local-only;
- mediated-only;
- read-only;
- drain-only.

Reduced mode must be explicit.

It shall not silently alter signal or domain-operation meaning.

## 40. Recovery mode

### 40.1 Purpose

Recovery mode permits narrowly scoped lifecycle work when ordinary connection establishment is unsafe or impossible.

### 40.2 Permitted behavior

Recovery mode may allow:

- identity verification;
- connection-state inspection;
- generation reconciliation;
- attachment review;
- secure-session replacement;
- bounded state transfer;
- drain;
- closure;
- re-establishment.

### 40.3 Restrictions

Recovery mode shall not silently grant:

- ordinary cognitive traffic;
- unrestricted directives;
- propagation;
- relationship creation;
- broad port discovery;
- authority expansion.

### 40.4 Exit

Exiting recovery mode requires explicit validation of the connection state being activated.

## 41. Observability and audit

### 41.1 Observable lifecycle

A conforming implementation should make it possible to determine:

- relationship identity;
- connection-instance identity;
- generation;
- current lifecycle state;
- active bindings;
- active attachments;
- restriction state;
- last material transition;
- continuity confidence;
- known conflict;
- terminal reason where applicable.

### 41.2 Bounded audit

Sensitive lifecycle transitions should preserve bounded evidence identifying:

- requesting identity;
- approving authority;
- affected scope;
- previous state;
- resulting state;
- policy version;
- relevant generation;
- outcome.

### 41.3 Privacy

Lifecycle records may reveal:

- topology;
- relationships;
- activity patterns;
- trust boundaries;
- recovery actions.

Access to detailed lifecycle evidence shall therefore remain controlled.

### 41.4 Missing observability

Loss of monitoring shall be represented as reduced observability.

Absence of lifecycle reports must not become proof of healthy state.

## 42. Public implementation requirements

A public ACS implementation claiming support for ACS-0005 shall document:

- relationship lifecycle states it supports;
- connection-attempt lifecycle;
- connection-instance lifecycle;
- connection-instance identity model;
- generation or stale-instance protection;
- activation requirements;
- degradation behavior;
- suspension behavior;
- drain behavior;
- closure behavior;
- retry and backoff;
- reconnection and resumption support;
- rebinding and migration behavior;
- attachment lifecycle;
- partition behavior;
- resource ceilings;
- lifecycle event ordering;
- security-lifecycle coordination;
- unsupported features;
- known continuity limitations.

Equivalent state names are permitted.

The required semantic distinctions must remain observable.

## 43. Conformance expectations

Conformance evidence should demonstrate that:

1. a relationship can survive ordinary connection loss;
2. a connection is not marked active before required admission completes;
3. transport establishment alone does not activate the ACS connection;
4. a new connection instance is distinguishable from an old instance;
5. stale-generation traffic is rejected;
6. reconnect does not silently widen attachment scope;
7. expired authority does not resume automatically;
8. failed attempts release bounded resources;
9. retries use bounded backoff;
10. one source cannot exhaust all half-open connection capacity;
11. degraded state is distinguishable from full health;
12. silence is not automatically interpreted as failure;
13. failed binding does not automatically retire the relationship;
14. drain stops new work and handles in-flight work explicitly;
15. connection closure does not report domain operations completed;
16. ambiguous execution remains indeterminate;
17. migration does not silently create split authority;
18. handoff invalidates or restricts the old instance;
19. partition recovery reconciles generations and attachments;
20. connection loss does not become memory absence;
21. duplicate connection paths preserve signal duplicate semantics;
22. lifecycle records remain bounded and privacy-controlled.

Detailed fault testing may be refined by later specifications.

## 44. Prohibited interpretations

This specification shall not be interpreted to mean that:

- every relationship always has an active connection;
- every connection has one physical socket;
- one socket equals one connection;
- a connection may be represented only as connected or disconnected;
- transport establishment proves ACS activation;
- connection activation proves service readiness;
- liveness proves health;
- health proves domain-operation availability;
- silence proves failure;
- timeout proves permanent participant loss;
- connection failure retires the relationship;
- reconnect may reuse stale authority;
- previous success guarantees current admission;
- keepalives renew authority automatically;
- migration transfers ownership;
- failover grants takeover automatically;
- the first responding duplicate instance is authoritative;
- closing a connection completes all in-flight operations;
- re-establishment creates new domain operations;
- public conformance requires disclosure of production recovery procedure;
- a lifecycle service controls physical-node disposition.

## 45. Initial architectural commitments

ACS-0005 establishes the following commitments:

1. relationships and connection instances possess separate lifecycles;
2. connection attempts are identified and bounded;
3. transport establishment is not ACS activation;
4. connection activation requires explicit lifecycle transition;
5. each connection instance is distinguishable from previous and concurrent instances;
6. generation or equivalent state prevents stale-instance authority;
7. active connection state remains scoped by attachments, capabilities, and resources;
8. liveness, health, readiness, and domain readiness remain distinct;
9. degradation and suspension remain explicit;
10. drain is bounded and semantically distinct from active operation;
11. closure does not retire the relationship automatically;
12. connection failure does not define domain-operation outcome;
13. re-establishment normally creates a new instance;
14. resumption requires stronger continuity evidence;
15. rebinding and migration shall not create silent split authority;
16. attachment authority does not silently expand during recovery;
17. retries and reconnects remain bounded;
18. partition recovery reconciles lifecycle state explicitly;
19. stale credentials and stale lifecycle events do not regain authority;
20. lifecycle resource exhaustion cannot prevent bounded closure and recovery;
21. connection lifecycle does not redefine MEM lifecycle;
22. public implementations may interoperate without exposing restricted lifecycle policy.

## 46. Open questions

The following questions remain for later specifications or implementation profiles:

- Which relationship states are mandatory for baseline public conformance?
- Should connection generation always change after secure-session replacement?
- When may a connection instance preserve identity across rebinding?
- Which resumable state is safe across process restart?
- How should simultaneous reconnect attempts be arbitrated?
- Which relationships may permit several active connections?
- What evidence is sufficient to establish endpoint continuity during migration?
- Which lifecycle events require durable retention?
- How should implementations represent causally ordered but not totally ordered lifecycle events?
- Which attachment changes require a new connection generation?
- When must port-version change force connection re-establishment?
- Which connection profiles may operate during partition?
- How should connection-attempt budgets be negotiated across mediators?
- Which drain behaviors are mandatory for directive-capable ports?
- How should partial payload transfer resume safely?
- When should a failed handoff roll back versus create a new instance?
- Which lifecycle transitions require independent concurrence?
- How should operator-forced closure interact with indeterminate directives?
- Which lifecycle evidence may immune systems inspect without payload access?
- How should long-dormant event-driven connections prove continuing eligibility?
- Should recovery mode use separate connection identities?
- How should connection generation be preserved across hibernation?
- Which reduced-operation profiles should be standardized publicly?
- What minimum lifecycle evidence must survive total process loss?
- How should public implementations report unsupported resumption or migration?
- Which connection failures should become relationship degradation rather than ordinary reconnect?
- How should connection state interact with future adaptive topology without allowing learned authority expansion?

These questions do not permit implementations to weaken the distinctions already established.

## 47. Closing principle

> **Node must preserve the difference between an enduring relationship and the temporary connection that carries it, and it must never allow stale runtime state to regain current authority merely because communication resumes.**

Relationships may endure.

Connections may fail.

Bindings may change.

Attachments may narrow.

Signals may remain indeterminate.

Recovery may create a new instance.

Every transition must remain explicit enough that Node can continue without mistaking reconnection for continuity, liveness for health, or delayed traffic for current authority.

## Revision history

### Version 0.1 — 2026-07-16

- Established the public connection-lifecycle architecture.
- Separated relationship, connection-attempt, connection-instance, binding, attachment, signal, and domain-operation lifecycles.
- Defined conceptual lifecycle states for relationships, attempts, connections, bindings, and attachments.
- Established connection-instance identity and generation requirements.
- Defined activation, active operation, degradation, suspension, draining, closure, failure, and expiry.
- Defined re-establishment, resumption, rebinding, migration, handoff, and partition behavior.
- Required stale-instance suppression and explicit conflict handling.
- Defined in-flight operation and indeterminate-result behavior.
- Established bounded retry, backoff, lifecycle resources, and reconnect-storm protection.
- Defined lifecycle authority, event ordering, security coordination, observability, and privacy.
- Preserved boundaries with ACS-0002, ACS-0003, ACS-0004, MEM, runtime, health, immune, and physical-node systems.
- Established public conformance expectations and prohibited interpretations.
