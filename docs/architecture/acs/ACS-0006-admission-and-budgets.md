# ACS-0006: Admission and Budgets

| Field | Value |
|---|---|
| Specification | ACS-0006 |
| Title | Admission and Budgets |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ACS-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Pending review |
| Depends on | ACS-0000 through ACS-0005 |
| Related specifications | ACS-0007, ACS-0008, MEM-0000 through MEM-0007 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in admission separation, bounded grants, and protected-capacity requirements; distributed budget reconciliation and cross-region allocation remain under review |

> **Admission grants a bounded opportunity to communicate; it does not prove trust, truth, successful execution, or entitlement to future resources.**

## Architectural-intent notice

This specification defines the public architecture for admission decisions, communication grants, resource budgets, protected capacity, overload behavior, and revocation within the Adaptive Connection Substrate.

It is independently authored public architecture.

It is not produced by deleting, sanitizing, paraphrasing, or selectively redacting restricted architecture.

This document defines the public concepts and guarantees required for independently implementable Node-compatible admission and resource control without disclosing:

- production admission policy;
- private trust thresholds;
- proprietary topology-selection algorithms;
- restricted cognitive prioritization;
- private immune heuristics;
- production resource allocations;
- production node capacity;
- internal operator procedures;
- protected endpoint catalogs;
- non-public scheduling policy.

This specification does not prescribe:

- one admission algorithm;
- one policy language;
- one resource scheduler;
- one rate-limiting implementation;
- one distributed quota protocol;
- one consensus mechanism;
- one trust-scoring formula;
- one authentication system;
- fixed queue sizes;
- fixed bandwidth limits;
- fixed CPU or accelerator allocations;
- production admission thresholds;
- production emergency reserves.

Implementations may use different mechanisms.

They must preserve the distinctions, safety properties, and observable outcomes established here.

## 1. Purpose

Node must decide whether communication may begin or continue while resources, trust evidence, health, compatibility, and authority are finite or uncertain.

A participant may be:

- authenticated but not eligible for a relationship;
- related but not eligible for a connection;
- connected but not attached to a port;
- attached but not authorized for one signal;
- authorized but temporarily out of budget;
- within budget but unable to reserve required capacity;
- trusted for observation but not for directives;
- eligible under normal conditions but restricted during degradation;
- able to submit a request while the domain-specific operation is still refused.

A system that represents all of those questions through one boolean value such as `allowed` will eventually:

- grant excessive authority;
- confuse temporary overload with distrust;
- allow one participant to starve others;
- permit stale grants to remain active;
- mistake authentication for authorization;
- create inconsistent enforcement across transports;
- force security or immune components to invent communication semantics;
- allow domain-specific services to bypass ACS resource controls;
- lose the ability to explain why an interaction was restricted.

ACS-0006 establishes the public admission and budget model required to keep those conditions distinct.

## 2. Scope

This specification governs:

- admission requests;
- admission subjects and targets;
- admission scope;
- admission profiles;
- admission evidence;
- admission decisions;
- admission grants;
- denial;
- restriction;
- deferral;
- challenge requirements;
- expiry;
- cancellation;
- revocation;
- admission layering;
- relationship admission;
- connection admission;
- attachment admission;
- signal admission;
- payload-transfer admission;
- mediation admission;
- lifecycle-transition admission;
- resource budgets;
- quotas;
- reservations;
- credits;
- rate and burst limits;
- outstanding-work limits;
- fan-in and fan-out limits;
- validation budgets;
- protected capacity;
- resource borrowing;
- reclamation;
- overload behavior;
- backpressure;
- load shedding;
- fairness;
- starvation prevention;
- accounting;
- degraded and partitioned operation;
- interaction with security, trust, immune, runtime, health, and MEM;
- public implementation and conformance requirements.

This specification does not define:

- participant authentication;
- credential formats;
- cryptographic algorithms;
- trust computation;
- capability cryptography;
- immune evidence interpretation;
- immune containment policy;
- memory-operation admission;
- physical-node admission;
- software-installation authority;
- cognitive salience;
- proprietary scheduling;
- production capacity values;
- exact transport flow-control algorithms.

Those subjects belong to ACS-0007, ACS-0008, MEM, runtime specifications, implementation profiles, or other adjacent architecture.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements.

The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification.

The word **may** describes permitted behavior.

An implementation does not conform merely because it exposes one authorization callback or one resource counter.

Its observable behavior must preserve the independent admission and budget dimensions defined here.

## 4. Foundational distinctions

### 4.1 Discovery is not admission

Learning that an endpoint or port exists does not grant permission to:

- form a relationship;
- establish a connection;
- attach to a port;
- send a signal;
- access a payload;
- consume protected capacity.

### 4.2 Authentication is not admission

Authentication contributes evidence about identity.

It does not independently establish that the identity may perform the requested interaction.

### 4.3 Trust is not admission

Trust may influence admission policy.

A trust observation does not itself create an admission grant.

### 4.4 Relationship is not admission to every port

An established relationship permits only the broad association defined by its class and scope.

It does not grant access to every endpoint or port owned by either participant.

### 4.5 Connection is not attachment

An active connection may exist while no attachment authorizes a particular port.

### 4.6 Attachment is not signal acceptance

An attachment grants a bounded opportunity to submit or receive eligible communication.

Every signal remains individually admissible.

### 4.7 Signal admission is not semantic acceptance

ACS may admit a signal for delivery to a domain service.

The receiving domain may still reject the signal’s claim, request, payload, or operation.

### 4.8 Admission is not successful execution

An admitted directive or request may fail because:

- the target becomes unavailable;
- preconditions are not satisfied;
- domain-specific validation fails;
- resources disappear;
- authority expires;
- execution is cancelled;
- the result becomes indeterminate.

### 4.9 Budget is not reservation

A budget defines a maximum permitted resource scope.

It does not guarantee that the resources are immediately available.

### 4.10 Reservation is not execution

A reservation holds or protects resource capacity.

It does not prove that work began or completed.

### 4.11 Priority is not authority

Priority influences handling of otherwise admissible work.

It does not grant permission to perform an otherwise unauthorized interaction.

### 4.12 Capacity is not authority

Available resources do not create permission to use them.

### 4.13 Authority is not capacity

A participant may possess authority while insufficient capacity exists to exercise it safely.

### 4.14 Backpressure is not distrust

Backpressure communicates resource pressure.

It does not independently mean that the source is untrusted, malicious, or semantically invalid.

### 4.15 Repeated success is not permanent admission

Previous successful use does not create indefinite future eligibility.

### 4.16 Missing evidence is not permission

Unknown, stale, conflicting, or unavailable admission evidence shall not be interpreted as approval.

## 5. Core terminology

### 5.1 Admission request

An admission request asks an authorized evaluator to permit a bounded interaction or resource scope.

An admission request may concern:

- relationship establishment;
- connection establishment;
- attachment;
- signal submission;
- payload transfer;
- mediation;
- lifecycle transition;
- resource reservation;
- grant renewal;
- grant expansion.

### 5.2 Admission subject

The admission subject is the identified participant, relationship, connection, attachment, signal, mediator, or lifecycle actor seeking permission.

### 5.3 Admission target

The admission target is the endpoint, port, relationship, connection, resource pool, mediator, lifecycle object, or other governed ACS object against which admission is evaluated.

### 5.4 Admission scope

Admission scope defines the precise interaction being considered.

It may include:

- subject identity;
- target identity;
- relationship class;
- port identity;
- direction;
- signal domains;
- signal intents;
- subtype families;
- payload category;
- operation class;
- resource dimensions;
- recipient scope;
- mediation path;
- validity period;
- lifecycle state.

### 5.5 Admission profile

An admission profile is a declared policy and compatibility context governing a category of admission decisions.

A profile may define:

- required evidence;
- eligible relationship classes;
- supported decisions;
- mandatory restrictions;
- budget dimensions;
- renewal requirements;
- degraded behavior;
- audit requirements.

### 5.6 Admission evidence

Admission evidence is information used to evaluate a request.

Evidence may include:

- authenticated identity;
- relationship state;
- connection generation;
- endpoint continuity;
- port contract;
- compatibility;
- capability evidence;
- trust state;
- health state;
- budget availability;
- lifecycle state;
- security restrictions;
- immune restrictions;
- previous behavior;
- mediation requirements.

Evidence may be:

- current;
- stale;
- incomplete;
- conflicting;
- unverifiable;
- unavailable.

### 5.7 Admission evaluator

An admission evaluator is a logical responsibility authorized to produce admission decisions for a declared scope.

An evaluator may be implemented:

- locally;
- remotely;
- through several cooperating participants;
- through a policy service;
- within an endpoint implementation;
- through a mediator.

The implementation form does not change the semantic responsibility.

### 5.8 Admission decision

An admission decision is an explicit outcome concerning one admission request.

### 5.9 Admission grant

An admission grant permits a bounded interaction within declared conditions.

A grant is not unrestricted authority.

### 5.10 Restriction

A restriction narrows an existing or requested scope.

### 5.11 Denial

A denial establishes that the requested interaction is not admitted under the evaluated evidence and policy.

### 5.12 Deferral

Deferral establishes that a final decision has not been made and that the interaction is not currently admitted.

### 5.13 Challenge

A challenge requests additional evidence before admission can continue.

Challenge completion does not guarantee admission.

### 5.14 Budget

A budget defines a bounded maximum resource entitlement or allowance within a declared scope.

### 5.15 Quota

A quota is a budget measured over a declared interval, population, or accounting boundary.

### 5.16 Reservation

A reservation protects or holds resource capacity for a declared interaction or class of interactions.

### 5.17 Credit

A credit is a bounded unit of permission used by a flow-control or admission mechanism to represent available submission or processing capacity.

### 5.18 Protected capacity

Protected capacity is resource capacity reserved for one or more declared classes so ordinary demand cannot consume all ability to perform critical work.

### 5.19 Hard ceiling

A hard ceiling is a limit that ordinary policy, trust, priority, adaptation, or owner preference shall not exceed.

### 5.20 Soft limit

A soft limit is adjustable within a higher hard ceiling.

### 5.21 Target

A target is a preferred operating level used for efficiency, fairness, or health.

## 6. Admission layering

Admission may occur at several layers.

The layers may be evaluated together for efficiency, but their meanings must remain distinguishable.

### 6.1 Disclosure admission

Disclosure admission determines whether an observer may learn:

- that an endpoint exists;
- that a port exists;
- a bounded portion of a contract;
- current compatibility information.

Disclosure admission does not grant operational use.

### 6.2 Relationship admission

Relationship admission determines whether participants may establish or continue a relationship of a declared class and scope.

### 6.3 Connection admission

Connection admission determines whether a runtime connection instance may become active for an eligible relationship.

### 6.4 Attachment admission

Attachment admission determines whether a relationship or connection may attempt scoped use of a particular port.

### 6.5 Signal admission

Signal admission determines whether one signal may enter or leave a port under current:

- attachment;
- contract;
- authority;
- freshness;
- resource;
- lifecycle conditions.

### 6.6 Payload-transfer admission

Payload-transfer admission determines whether a referenced payload may be:

- requested;
- transmitted;
- received;
- retained temporarily;
- accessed through mediation.

Payload-transfer admission remains separate from signal admission.

### 6.7 Mediation admission

Mediation admission determines whether a participant may perform a declared mediation function.

Authorization to forward does not automatically authorize:

- inspection;
- transformation;
- aggregation;
- redaction;
- retention;
- re-origination.

### 6.8 Lifecycle-transition admission

Lifecycle-transition admission determines whether a requested ACS lifecycle change may proceed.

Examples include:

- activation;
- restriction;
- suspension;
- drain;
- rebinding;
- migration;
- closure;
- retirement.

### 6.9 Domain-operation admission

After ACS communication admission, a domain service may apply its own semantic admission.

For example:

- MEM decides whether a storage proposal may become admitted memory;
- a runtime service decides whether a workload request may be scheduled;
- an immune service decides whether evidence satisfies an immune operation contract.

ACS does not replace domain-specific semantic admission.

## 7. Admission-request requirements

An admission request shall identify or reference enough information to determine:

- subject;
- target;
- requested scope;
- requested duration;
- requested resources;
- relationship context;
- connection context where applicable;
- port contract version;
- requested direction;
- requested signal or payload classes;
- applicable authority evidence;
- applicable mediation;
- current lifecycle context.

A malformed or materially incomplete request may be:

- rejected;
- challenged;
- deferred;
- treated as unknown.

It shall not receive broader permission because its scope is ambiguous.

## 8. Admission decision model

### 8.1 Admitted

The requested scope is granted as requested.

### 8.2 Admitted with restrictions

A narrower scope is granted.

The granted scope must be distinguishable from the requested scope.

### 8.3 Deferred

No current grant is established because required evidence, capacity, policy, or dependencies are pending.

### 8.4 Challenged

Additional evidence is required.

No requested privileged interaction is admitted until the challenge requirements are satisfied and a grant is issued.

### 8.5 Denied

The requested scope is not permitted under current policy and evidence.

### 8.6 Unavailable

The required admission function or dependency is known to be unavailable.

### 8.7 Conflicting

Incompatible valid-looking evidence or decisions prevent a safe result.

### 8.8 Unknown

Evidence is insufficient to determine whether the request may be admitted.

### 8.9 Cancelled

The request was withdrawn or terminated before a final grant became active.

### 8.10 Expired

The request or provisional decision exceeded its validity boundary.

## 9. Admission-decision record

This section defines semantics, not a required wire format.

A decision should make it possible to determine:

```text
AdmissionDecision
    request_identity
    subject_identity
    target_identity
    requested_scope
    granted_scope
    decision_outcome
    decision_authority
    policy_or_profile_reference
    evidence_boundary
    restrictions
    budget_grants
    reservation_state
    mediation_requirements
    activation_conditions
    validity_boundary
    renewal_requirements
    revocation_reference
    audit_reference
```

Not all fields need to be transmitted to every participant.

The required meaning must remain recoverable for enforcement and reconciliation.

## 10. Admission grants

### 10.1 Grants are scoped

Every grant shall be limited by an explicit or recoverable scope.

### 10.2 Grants are attributable

A grant shall identify or permit recovery of the authority that issued it.

### 10.3 Grants are bounded

A grant shall be bounded by one or more of:

- time;
- use count;
- connection generation;
- attachment identity;
- policy epoch;
- credential epoch;
- resource budget;
- lifecycle state;
- operation completion.

### 10.4 Grants do not self-expand

A grant shall not become broader because:

- it was used successfully;
- the subject gained trust;
- the relationship strengthened;
- more resources became available;
- the endpoint migrated;
- the connection was re-established;
- the system entered degraded operation.

### 10.5 Grants may be narrowed

A grant may be narrowed by:

- explicit restriction;
- resource pressure;
- trust change;
- security policy;
- immune restriction;
- lifecycle transition;
- contract-version change.

The narrowed scope must be observable to affected enforcement points.

### 10.6 Grants may require activation

A grant may exist before all activation conditions are satisfied.

Possessing an inactive or conditional grant does not authorize use.

## 11. Admission evidence

### 11.1 Evidence is scoped

Evidence shall be interpreted only within the scope for which it is valid.

### 11.2 Evidence ages

Admission evidence becomes stale.

Implementations shall define how evidence age affects:

- new grants;
- renewal;
- continued use;
- privileged operations;
- degraded operation.

### 11.3 Conflicting evidence

Conflicting evidence shall not be silently resolved by selecting the most permissive interpretation.

### 11.4 Missing evidence

Missing required evidence produces:

- denial;
- restriction;
- deferral;
- challenge;
- unknown state;

according to the applicable profile.

### 11.5 Local evidence

Locality does not make evidence authoritative.

A local process claim, cache, address, or runtime observation may contribute evidence but does not bypass required validation.

### 11.6 Historical behavior

Past behavior may influence admission policy.

It shall not override hard security, authority, or resource restrictions.

## 12. Admission evaluators and authority

### 12.1 Explicit scope

Every evaluator shall have an explicit decision scope.

An evaluator must not imply organism-wide authority when it governs only:

- one endpoint;
- one port;
- one region;
- one relationship class;
- one resource pool;
- one lifecycle operation;
- one trust domain.

### 12.2 Distributed evaluation

Several evaluators may cooperate.

Distribution does not automatically define:

- quorum;
- precedence;
- leader;
- conflict resolution;
- global authority.

Those properties must be explicit.

### 12.3 Local evaluation

An endpoint may perform local admission when authorized.

Local evaluation shall preserve:

- higher policy;
- hard ceilings;
- revocation;
- required security evidence;
- required immune restrictions.

### 12.4 Self-admission

A participant shall not automatically approve its own request merely because it owns or implements the target service.

Any permitted self-admission must be:

- explicit;
- scoped;
- bounded;
- auditable where privileged.

### 12.5 Evaluator unavailability

If required evaluation is unavailable:

- existing unexpired grants may continue only according to their profile;
- new privileged grants must fail closed, defer, or remain unknown;
- no component may invent broader authority locally;
- reduced public or low-risk operation may continue only when explicitly permitted.

## 13. Relationship admission

Relationship admission shall consider:

- participant identities;
- requested relationship class;
- requested scope;
- compatibility;
- current participant state;
- security requirements;
- trust evidence;
- resource implications;
- required mediation;
- existing relationships;
- lifecycle restrictions;
- immune restrictions.

Relationship admission does not automatically create:

- a connection;
- an attachment;
- a port grant;
- payload access;
- directive authority.

## 14. Connection admission

Connection admission shall consider:

- relationship validity;
- connection-instance identity;
- connection generation;
- endpoint continuity;
- supported bindings;
- secure-session state;
- compatibility;
- attachment proposals;
- connection budgets;
- half-open capacity;
- lifecycle state.

Transport establishment before connection admission remains provisional runtime state.

## 15. Attachment admission

Attachment admission shall define:

- target port;
- permitted direction;
- permitted signal domains;
- permitted intents;
- permitted subtype families;
- payload policy;
- resource budget;
- priority ceiling;
- mediation requirements;
- validity;
- capability requirements.

Attachment to one port shall not grant access to another port.

## 16. Signal admission

Every signal remains independently admissible.

Signal admission may evaluate:

- signal identity;
- source identity;
- attachment state;
- connection generation;
- port contract;
- domain;
- intent;
- subtype;
- schema;
- freshness;
- expiry;
- replay state;
- provenance;
- payload references;
- authority evidence;
- recipient scope;
- priority ceiling;
- resource cost;
- current target state.

A signal rejected at this boundary shall not enter ordinary domain processing.

## 17. Payload-transfer admission

Payload transfer may consume substantially more resources and expose greater sensitivity than its referencing signal.

Payload-transfer admission shall consider:

- payload identity or reference;
- maximum size;
- expected expanded size;
- sensitivity;
- permitted recipients;
- access authority;
- integrity requirements;
- transport cost;
- retention;
- cancellation;
- current resource capacity.

A valid payload reference does not guarantee transfer admission.

## 18. Mediation admission

A mediator shall be admitted separately for each function it performs.

Possible mediation functions include:

- forwarding;
- policy enforcement;
- aggregation;
- transformation;
- redaction;
- retention;
- protocol translation;
- public facade operation.

One mediation grant shall not silently authorize another function.

## 19. Lifecycle-transition admission

Lifecycle transitions shall be admitted according to:

- affected object;
- current state;
- requested next state;
- requesting authority;
- safety conditions;
- in-flight work;
- resource requirements;
- security restrictions;
- required concurrence;
- rollback or failure behavior.

Authority to request a transition does not guarantee its execution.

## 20. Budget model

### 20.1 Every admitted scope is finite

No admission grant shall imply unlimited resource use.

### 20.2 Budgets are multidimensional

A budget may constrain:

- signal count;
- byte count;
- signal size;
- payload size;
- queue occupancy;
- submission rate;
- burst size;
- outstanding requests;
- outstanding directives;
- concurrent transfers;
- validation effort;
- compute triggered;
- response size;
- fan-out;
- mediation hops;
- retention time;
- retry count;
- connection attempts;
- lifecycle transitions.

### 20.3 Budgets may overlap

One interaction may be constrained simultaneously by:

- signal budget;
- attachment budget;
- relationship budget;
- source budget;
- port budget;
- endpoint budget;
- connection budget;
- node budget;
- regional budget;
- protected reserve.

The effective allowance shall satisfy every applicable hard restriction.

### 20.4 No single universal cost is required

Implementations may track separate physical or abstract resource dimensions.

A universal conversion between CPU time, memory, bandwidth, storage, and energy is not required.

### 20.5 Cost uncertainty

Unknown resource cost shall not be interpreted as zero cost.

An uncertain request may be:

- conservatively bounded;
- restricted;
- sampled;
- deferred;
- rejected;
- required to provide a more specific execution profile.

## 21. Budget scopes

Budgets may apply to:

1. one signal;
2. one payload;
3. one operation;
4. one attachment;
5. one connection;
6. one relationship;
7. one participant;
8. one port;
9. one endpoint;
10. one mediator;
11. one region;
12. one node;
13. one resource class;
14. one deployment-wide protected pool.

Unused budget at one scope does not automatically expand another scope.

## 22. Quotas

A quota shall define:

- measured resource;
- accounting scope;
- measurement interval;
- maximum allowance;
- burst behavior;
- reset or replenishment behavior;
- behavior after exhaustion;
- whether unused allowance carries forward.

A quota must not reset indefinitely through:

- reconnect;
- address change;
- mediator change;
- attachment recreation;
- minor request variation.

## 23. Reservations

### 23.1 Reservation purpose

Reservations protect capacity for work that requires stronger assurance than an ordinary budget.

### 23.2 Reservation scope

A reservation shall identify:

- resource;
- amount;
- holder;
- purpose;
- activation boundary;
- expiry;
- cancellation;
- release behavior;
- whether borrowing is permitted.

### 23.3 Reservation does not imply completion

Reserved work may still fail admission or execution for non-resource reasons.

### 23.4 Reservation expiry

Expired reservations shall release capacity.

### 23.5 Orphan protection

The system shall detect and bound orphaned reservations after:

- process failure;
- connection loss;
- migration;
- cancellation;
- authority revocation;
- operation completion.

## 24. Credits and flow control

Credits may represent bounded permission to submit additional:

- signals;
- bytes;
- payload segments;
- work items;
- connection attempts.

A credit mechanism shall define:

- issuer;
- scope;
- unit;
- replenishment;
- expiry;
- transferability;
- revocation;
- duplicate handling.

Credits do not create semantic authority.

## 25. Rates and bursts

### 25.1 Sustained rate

A sustained-rate limit controls average resource use over a declared window.

### 25.2 Burst allowance

A burst allowance permits temporary use above the sustained rate.

### 25.3 Burst boundaries

Burst policy shall define:

- maximum burst;
- replenishment;
- resource pool;
- protected-capacity interaction;
- behavior after exhaustion.

### 25.4 No average-only protection

A long-term average limit is insufficient where short bursts could exhaust:

- queues;
- memory;
- validation capacity;
- connection tables;
- mediators;
- safety reserves.

## 26. Outstanding-work limits

A source shall not create unbounded future obligations.

Outstanding-work limits may apply to:

- pending requests;
- active directives;
- payload transfers;
- deferred admission requests;
- challenges;
- pending acknowledgements;
- retained mediator work;
- lifecycle transitions;
- reservations.

Work that expires, is cancelled, or becomes unauthorized shall release its outstanding-work allocation.

## 27. Fan-in and fan-out

### 27.1 Fan-out

Admission shall consider maximum recipient scope and cumulative downstream cost.

A small signal shall not create unbounded broadcast.

### 27.2 Fan-in

Ports accepting many sources shall preserve per-source or per-relationship isolation.

### 27.3 Duplicate evidence

Repeated copies of one signal shall not become independent evidence merely because they arrived through several sources or paths.

### 27.4 Dynamic populations

A recipient or source population shall not silently expand a grant merely because the mesh grew.

## 28. Validation budgets

Invalid traffic still consumes resources.

Admission processing shall bound:

- framing;
- parsing;
- schema validation;
- cryptographic verification;
- capability evaluation;
- provenance inspection;
- replay detection;
- payload-reference checks;
- compatibility negotiation.

Low-cost rejection should occur before expensive validation where safe.

A source may be restricted because of excessive validation cost even when none of its submitted signals were admitted.

## 29. Response amplification

Admission shall consider the maximum permitted response cost.

A small request may trigger:

- large output;
- expensive retrieval;
- substantial compute;
- broad fan-out;
- repeated mediation;
- retained state.

Ports shall define bounded response profiles or equivalent limits.

## 30. Protected capacity

### 30.1 Purpose

Protected capacity preserves the ability to operate essential ACS functions during ordinary overload, fault, or attack.

### 30.2 Initial protected classes

Public architecture recognizes at least:

- infrastructure;
- security and trust;
- immune communication;
- lifecycle and recovery;
- physical safety;
- bounded continuity-critical communication.

### 30.3 Ordinary traffic

Ordinary communication may use most capacity during healthy operation.

It must not consume all capacity required for protected operation.

### 30.4 Protected does not mean unlimited

Every protected class remains bounded.

### 30.5 Protection is explicit

A participant or signal does not receive protected status merely because it claims importance, urgency, emergency, or high confidence.

### 30.6 Narrow use

Protected channels shall not become general-purpose high-priority paths.

## 31. Reserve borrowing

Unused protected capacity may be borrowed only when:

- borrowing is explicitly permitted;
- borrowed work can be reclaimed;
- accounting remains visible;
- emergency capability remains sufficient;
- borrowing does not become permanent ownership.

Some safety capacity may be non-borrowable.

## 32. Reclamation

When protected capacity is required, lower-protection work may be:

- throttled;
- paused;
- downgraded;
- cancelled;
- evicted;
- migrated;
- allowed to expire.

Reclamation shall preserve required handling of:

- partially executed directives;
- domain-operation identity;
- audit evidence;
- resource release.

## 33. Priority

### 33.1 Priority is bounded

Ports, attachments, and grants may impose a maximum priority ceiling.

### 33.2 Priority cannot self-escalate

A source shall not exceed its ceiling by:

- repetition;
- claimed urgency;
- changing transport;
- changing mediators;
- increasing confidence;
- strengthening a relationship.

### 33.3 Priority and reserve are distinct

High-priority work may lack a reservation.

Protected capacity may hold ordinary-priority work required for continuity.

### 33.4 Emergency priority

Emergency priority requires separately established authority or validated state.

A claimed emergency is not sufficient.

## 34. Fairness and starvation prevention

Admission and budget policy should prevent one eligible participant from indefinitely starving other eligible participants.

Fairness may consider:

- participant;
- relationship;
- attachment;
- port;
- region;
- resource class;
- previous use;
- current restrictions;
- protected minimums.

Fairness does not require equal allocation.

Ordinary traffic must not starve protected operation.

Protected operation must not indefinitely suppress all ordinary communication during healthy conditions.

## 35. Backpressure

### 35.1 Explicit pressure

When downstream capacity is insufficient, ACS shall apply explicit:

- credit reduction;
- rate reduction;
- admission restriction;
- deferral;
- queue refusal;
- payload pause;
- load shedding;
- busy state.

### 35.2 Pressure propagation

Backpressure shall propagate far enough upstream to prevent continued unsafe accumulation.

### 35.3 Semantic distinction

Backpressure does not independently mean:

- distrust;
- relationship rejection;
- signal invalidity;
- endpoint retirement;
- immune suspicion.

### 35.4 Ignored pressure

Repeated disregard of valid backpressure may become security or immune evidence under later specifications.

## 36. Load shedding

Load shedding may include:

- rejecting newest work;
- discarding oldest eligible work;
- expiring stale work;
- sampling;
- aggregation;
- fidelity reduction;
- duplicate collapse;
- refusing new attachments;
- disabling optional outputs.

The shedding policy must match signal and operation semantics.

The system shall not silently discard work that requires a definitive result or explicit indeterminate outcome.

## 37. Admission under overload

During overload, admission may:

- reject new relationships;
- reject new connections;
- restrict new attachments;
- reduce rates;
- prohibit directives;
- permit observations only;
- limit payload access;
- require mediation;
- preserve health and recovery communication;
- enter protected-only operation.

Overload shall not grant broader authority.

## 38. Degraded admission profiles

A degraded profile may narrow:

- eligible relationship classes;
- available ports;
- signal families;
- directions;
- payload categories;
- grant duration;
- budgets;
- priority ceilings;
- mediation paths.

A degraded profile must remain explicit and compatible with the declared port semantics.

## 39. Revocation

### 39.1 Revocable objects

The following shall be revocable:

- admission grants;
- attachments;
- reservations;
- credits;
- delegated budget;
- privileged lifecycle grants.

### 39.2 Revocation causes

Revocation may result from:

- security policy;
- trust change;
- immune restriction;
- relationship retirement;
- connection-generation change;
- port retirement;
- endpoint ownership change;
- resource emergency;
- policy update;
- detected misuse;
- operator action.

### 39.3 Effective scope

Revocation may affect:

- one signal;
- one attachment;
- one port;
- one connection;
- one relationship;
- one participant;
- one resource class;
- one policy epoch.

The smallest safe effective scope should be preferred.

### 39.4 Revocation does not require cooperation

A grant holder shall not be able to preserve authority merely by refusing to acknowledge revocation.

## 40. In-flight work after revocation

Policy shall define whether in-flight work is:

- cancelled;
- paused;
- allowed to complete safely;
- rolled back;
- returned as indeterminate;
- transferred to recovery handling.

A new grant shall not be inferred from completion of previously admitted work.

## 41. Renewal

Renewal is a new admission decision.

Renewal shall re-evaluate applicable:

- identity;
- relationship state;
- connection generation;
- port contract;
- security state;
- trust evidence;
- immune restrictions;
- budget;
- lifecycle state.

Successful continued traffic does not renew a grant automatically unless the profile explicitly defines bounded activity-based renewal.

## 42. Delegation and budget transfer

### 42.1 Delegated admission authority

An evaluator may delegate a narrower decision scope when authorized.

### 42.2 Delegated budget

A participant may receive a sub-budget from a broader budget when explicitly permitted.

### 42.3 No amplification

Delegation shall not increase:

- resource amount;
- validity;
- target scope;
- operation scope;
- protected status;
- priority ceiling.

### 42.4 Accounting continuity

Delegated use must remain attributable to the applicable parent and child scopes.

### 42.5 Transferability

Grants and budgets are non-transferable by default.

Transfer requires explicit policy.

## 43. Partition behavior

### 43.1 Existing grants

During partition, existing grants may continue only according to their declared partition behavior and validity boundaries.

### 43.2 New grants

New privileged grants should fail closed, defer, or remain unknown when required authorities cannot be reached.

### 43.3 Local budgets

Local operation may continue within pre-established local budgets when:

- permitted by profile;
- hard ceilings remain enforceable;
- protected capacity remains available;
- reconciliation is possible.

### 43.4 No duplicate entitlement

Partition shall not create full independent copies of one exclusive budget unless the profile explicitly permits overcommit and reconciliation.

### 43.5 Reconciliation

After partition, participants shall reconcile:

- grant generations;
- revocations;
- consumed quotas;
- delegated budgets;
- reservations;
- protected-capacity use;
- conflicting decisions.

Conflicts must remain explicit until resolved.

## 44. Migration and rebinding

Migration or rebinding shall preserve or explicitly reconcile:

- admission-grant scope;
- connection generation;
- attachment scope;
- consumed budget;
- outstanding work;
- reservations;
- revocation state;
- protected-capacity eligibility.

Migration shall not reset quotas or expand authority merely because runtime placement changed.

## 45. Recovery mode

Recovery mode may use a separate constrained admission profile.

Recovery admission may permit:

- identity verification;
- state inspection;
- grant reconciliation;
- bounded closure;
- attachment repair;
- secure-session replacement;
- restoration of ordinary admission service.

Recovery mode shall not silently permit:

- ordinary unrestricted traffic;
- new propagation;
- broad relationship creation;
- unrestricted directives;
- unlimited resource use.

## 46. Admission accounting

Admission and budget decisions shall be attributable sufficiently to support:

- enforcement;
- fairness;
- revocation;
- reconciliation;
- abuse detection;
- capacity planning;
- conformance;
- audit where required.

Accounting may track:

- requests;
- decisions;
- grants;
- denials;
- validation cost;
- consumed budget;
- reserved capacity;
- rejected-work cost;
- outstanding work;
- overrun;
- reclamation;
- revocation.

Not every signal must produce permanent detailed audit history.

## 47. Accounting uncertainty

Resource accounting may be approximate.

Approximation shall not permit claims stronger than the evidence supports.

The system shall distinguish:

- exact;
- estimated;
- stale;
- incomplete;
- conflicting;
- unknown;

accounting state where the distinction affects admission.

## 48. Budget overrun

When work exceeds its admitted budget, the runtime may:

- throttle;
- pause;
- cancel;
- reduce fidelity;
- return a partial result;
- enter degraded operation;
- request a new grant;
- report an overrun;
- produce security or immune evidence where appropriate.

An overrun shall not automatically consume unrelated protected capacity.

## 49. Admission abuse and anomaly evidence

Potential evidence includes:

- repeated denied requests;
- attachment cycling;
- connection-attempt flooding;
- challenge exhaustion;
- quota-reset attempts;
- ignored backpressure;
- expensive malformed traffic;
- response-amplification attempts;
- fan-out expansion;
- repeated overruns;
- stale-grant use;
- protected-capacity misuse.

Such behavior is evidence.

It is not automatically proof of malice or compromise.

## 50. Privacy

Admission and accounting records may reveal:

- relationships;
- topology;
- activity;
- capabilities;
- security restrictions;
- immune restrictions;
- resource pressure;
- participant behavior.

Detailed access shall therefore remain separately controlled.

Admission infrastructure shall not become an unrestricted surveillance channel into Node cognition or memory.

## 51. Boundary with ACS-0002

ACS-0002 defines why a relationship exists and which primary relationship class it possesses.

ACS-0006 defines whether that relationship may be:

- established;
- connected;
- attached;
- resourced;
- restricted;
- renewed.

Relationship class does not itself grant admission.

## 52. Boundary with ACS-0003

ACS-0003 defines signal meaning, freshness, provenance, priority, confidence, authority, and payload references.

ACS-0006 determines whether a signal may consume ACS communication capacity under current grants and budgets.

The following remain distinct:

1. high confidence is not admission;
2. high priority is not authority;
3. valid provenance is not sufficient admission;
4. signal acceptance is not domain-operation completion;
5. repeated delivery does not create additional budget;
6. a payload reference does not grant payload-transfer capacity.

## 53. Boundary with ACS-0004

ACS-0004 defines endpoints, ports, contracts, bindings, attachments, visibility, and mediation.

ACS-0006 defines:

- who may obtain attachments;
- what attachment scope may be granted;
- which budgets constrain use;
- how ports apply pressure and restriction;
- how mediation functions are separately admitted.

Port visibility does not create admission.

## 54. Boundary with ACS-0005

ACS-0005 defines lifecycle states and transitions.

ACS-0006 defines admission requirements for:

- relationship establishment;
- connection activation;
- attachment activation;
- restriction;
- renewal;
- recovery;
- lifecycle transitions.

A lifecycle state does not bypass admission.

An admission grant does not bypass the lifecycle state machine.

## 55. Boundary with ACS-0007

ACS-0007 will define:

- identity assurance;
- authentication;
- trust;
- capabilities;
- credential state;
- revocation evidence;
- secure-session requirements;
- authority validation.

ACS-0006 defines how those security and trust outputs contribute to admission decisions.

The following rules apply:

1. ACS-0006 does not invent identity assurance.
2. Authentication success does not guarantee admission.
3. Trust evidence may influence but does not replace admission.
4. A capability may authorize an operation but does not guarantee resource availability.
5. Admission evaluators shall not silently issue stronger security authority than ACS-0007 permits.
6. Unknown security state shall not become privileged admission.

## 56. Boundary with ACS-0008

ACS-0008 will define how immune systems:

- observe ACS evidence;
- assess suspicious conditions;
- request restrictions;
- participate in containment;
- support restoration.

ACS-0006 defines the admission and enforcement actions available to that integration.

Potential bounded actions include:

- deny new relationships;
- deny new connections;
- restrict attachments;
- reduce budgets;
- force mediation;
- disable directives;
- permit observations only;
- suspend selected communication;
- preserve recovery and evidence channels.

ACS-0008 must not invent new unrestricted enforcement semantics outside the ACS admission and lifecycle model.

Immune evidence alone does not create authority to apply every action.

## 57. Boundary with MEM

ACS governs communication admission and communication budgets.

MEM governs memory-operation admission and memory-specific resources.

For example:

```text
ACS
    may admit a bounded recall-request signal
    to a memory-service port

MEM
    decides whether the requested memory operation
    is authorized, meaningful, complete, and executable
```

The following remain distinct:

1. ACS admission is not memory admission.
2. ACS budget availability is not memory durability.
3. ACS backpressure is not memory rejection.
4. ACS retry is not automatically a new memory operation.
5. Memory custody does not grant ACS port admission.
6. A healthy ACS connection does not prove memory-role availability.
7. MEM may apply stricter domain-specific resource limits after ACS admission.

## 58. Boundary with runtime

Runtime systems enforce:

- queues;
- rates;
- reservations;
- scheduling;
- resource ceilings;
- cancellation;
- reclamation.

Runtime mechanisms shall preserve ACS admission semantics.

Runtime placement or spare capacity does not create admission authority.

## 59. Boundary with health

Health systems provide evidence concerning:

- capacity;
- resource pressure;
- dependency state;
- thermal state;
- transport state;
- process state.

Health evidence may narrow admission or budgets.

Health does not independently grant authority or declare a relationship trusted.

## 60. Boundary with physical safety

Physical safety restrictions are hard admission ceilings where applicable.

No admission grant, trust state, immune recommendation, operator preference, or resource surplus may silently override required:

- thermal limits;
- electrical limits;
- actuator interlocks;
- emergency stop;
- power constraints;
- physical containment.

## 61. Public implementation requirements

A public implementation claiming support for ACS-0006 shall document:

- admission layers supported;
- admission-request representation;
- decision outcomes;
- grant scope and expiry;
- evaluator scope;
- renewal behavior;
- revocation behavior;
- budget dimensions;
- quota behavior;
- reservation behavior;
- rate and burst handling;
- outstanding-work limits;
- fan-in and fan-out limits;
- validation protection;
- protected-capacity classes;
- borrowing and reclamation;
- backpressure;
- overload behavior;
- partition behavior;
- accounting uncertainty;
- interaction with security and trust;
- unsupported features;
- known enforcement limitations.

Equivalent names and representations are permitted.

The required semantic distinctions must remain observable.

## 62. Conformance expectations

Conformance evidence should demonstrate that:

1. authentication alone does not grant port use;
2. an established relationship does not grant every attachment;
3. attachment admission does not guarantee signal admission;
4. signal admission does not report domain-operation completion;
5. grants are scoped and expire;
6. stale grants do not remain privileged indefinitely;
7. denied requests do not consume unbounded validation resources;
8. one source cannot consume all shared queue or attempt capacity;
9. quotas do not reset through reconnect or address change;
10. a budget does not falsely claim reserved resources;
11. reservations release after expiry or cancellation;
12. backpressure prevents unbounded accumulation;
13. overload narrows admission without expanding authority;
14. ordinary traffic cannot consume all protected capacity;
15. protected capacity remains bounded;
16. borrowed reserve can be reclaimed;
17. priority does not exceed a declared ceiling;
18. partition does not duplicate exclusive budget silently;
19. migration does not reset consumed budget;
20. revocation affects active enforcement points;
21. unknown authority or budget state does not create privileged admission;
22. ACS admission remains distinct from MEM admission;
23. immune evidence does not automatically grant containment authority;
24. accounting and audit remain privacy-controlled.

## 63. Prohibited interpretations

This specification shall not be interpreted to mean that:

- authentication equals admission;
- trust equals admission;
- relationship class equals authority;
- connection activation grants every port;
- attachment grants every signal;
- signal admission proves semantic truth;
- admission guarantees execution;
- a budget guarantees immediate resources;
- a reservation guarantees successful work;
- available capacity grants authority;
- high priority bypasses hard ceilings;
- protected capacity is unlimited;
- unused reserve becomes permanent ordinary capacity;
- reconnect resets quotas;
- migration resets budget use;
- a local participant may bypass admission;
- an endpoint owner may self-admit unrestricted use;
- an immune component may invent new enforcement authority;
- a security service may ignore resource ceilings;
- a runtime service may reinterpret admission policy;
- ACS backpressure defines memory admission;
- public conformance requires disclosure of production budgets or thresholds.

## 64. Initial architectural commitments

ACS-0006 establishes that:

1. admission is layered;
2. admission is separate from authentication, trust, and domain semantics;
3. every grant is scoped, bounded, attributable, and revocable;
4. restricted admission is distinguishable from full admission;
5. missing evidence does not grant permission;
6. every signal remains individually admissible;
7. payload transfer requires separate resource consideration;
8. mediation functions require separate admission;
9. lifecycle transitions remain admission-governed;
10. every admitted interaction operates within finite budgets;
11. budgets are multidimensional;
12. budget is distinct from reservation;
13. quotas cannot be reset through superficial identity or connection changes;
14. validation and rejected traffic remain resource-bounded;
15. fan-in, fan-out, response amplification, retries, and outstanding work remain bounded;
16. protected capacity preserves infrastructure, security, immune, lifecycle, recovery, and safety operation;
17. protected capacity remains finite;
18. borrowing is explicit and reclaimable;
19. priority is distinct from authority and reservation;
20. backpressure is distinct from distrust;
21. overload produces explicit restriction, deferral, rejection, or degraded operation;
22. revocation does not depend on holder cooperation;
23. partitions do not silently duplicate exclusive authority or budget;
24. ACS-0007 owns security and trust semantics;
25. ACS-0008 must use ACS-defined admission and enforcement actions;
26. MEM retains independent memory-operation admission;
27. public implementations remain useful without disclosing production admission policy.

## 65. Open questions

The following questions remain for later specifications or implementation profiles:

- Which admission layers are mandatory for baseline conformance?
- Which low-risk signals may use lightweight admission?
- Should every grant have a generation independent of connection generation?
- Which grants may continue during admission-evaluator unavailability?
- Which admission decisions require several authorities?
- How should conflicting distributed evaluators reconcile?
- Which budget dimensions must every public port expose?
- Should a common abstract cost unit supplement physical resource measures?
- How should regional budgets compose with node-local hard ceilings?
- Which protected classes require guaranteed minimum capacity?
- Which physical-safety reserves must remain non-borrowable?
- How quickly must borrowed reserve be reclaimable?
- Which work may be cancelled during reclamation?
- How should active-active replicas divide endpoint and port budgets?
- Which quota state must survive total process loss?
- How should approximate resource cost affect admission?
- When should repeated denials become immune evidence?
- How should public-facing capacity remain isolated from private internal capacity?
- Which admission restrictions may immune systems request automatically?
- Which restrictions require security, lifecycle, or operator concurrence?
- How should capability revocation propagate during partition?
- When may a local participant issue provisional restricted grants?
- How should grants behave across hibernation?
- Which admission and budget evidence must remain auditable?
- How should admission privacy be preserved while allowing abuse detection?
- Which domain-specific systems may reserve ACS protected capacity?
- How should energy cost participate in future budgets?
- Which resource overruns should produce health, security, or immune evidence?
- How should emergency admission avoid becoming reusable universal authority?
- Which ACS-0006 concepts require standardized public schemas?

These questions do not permit implementations to weaken the distinctions and hard boundaries already established.

## 66. Closing principle

> **Node may admit only the interaction it can identify, authorize, bound, account for, and safely withdraw.**

Authentication may establish identity.

Trust may contribute evidence.

A relationship may establish purpose.

A connection may establish a runtime path.

An attachment may establish eligible port use.

A grant may establish bounded permission.

A budget may establish finite resource scope.

None of them independently establishes truth, successful execution, permanent entitlement, or unrestricted control.

## Revision history

### Version 0.1 — 2026-07-16

- Established the public admission and budget architecture.
- Separated disclosure, relationship, connection, attachment, signal, payload, mediation, lifecycle, and domain-operation admission.
- Defined admission requests, evidence, evaluators, decisions, grants, restrictions, denial, deferral, challenge, expiry, and revocation.
- Defined multidimensional budgets, quotas, reservations, credits, rates, bursts, outstanding-work limits, validation budgets, fan-in, fan-out, and response amplification.
- Established protected-capacity, borrowing, reclamation, priority, fairness, backpressure, and load-shedding requirements.
- Defined overload, degraded, partitioned, migration, recovery, accounting, and overrun behavior.
- Preserved the boundaries between admission, authentication, trust, immune policy, runtime enforcement, physical safety, and MEM operation semantics.
- Established the enforcement vocabulary required by future ACS-0008 immune integration.
- Defined public implementation, conformance, prohibited interpretations, commitments, and open questions.
