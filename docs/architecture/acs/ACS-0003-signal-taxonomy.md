# ACS-0003: Signal Taxonomy

| Field | Value |
|---|---|
| Specification | ACS-0003 |
| Title | Signal Taxonomy |
| Status | Draft |
| Version | 0.1 |
| Classification | Architecture Specification |
| Authors | Project Prometheus |
| Last updated | 2026-07-14 |
| Approval | Pending review |
| Depends on | ACS-0000, ACS-0001, ACS-0002 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | Medium-high; semantic model requires further review |

> **A signal may influence the organism only according to its meaning, freshness, provenance, trust, and explicitly granted authority.**

## Architectural-intent notice

This specification defines the semantic taxonomy and architectural requirements of signals within the Adaptive Connection Substrate.

It does not prescribe binary layouts, field widths, serialization formats, network protocols, transport mechanisms, clock implementations, programming-language types, or routing algorithms unless explicitly stated.

Later ACS specifications may define concrete representations and runtime behavior. They must preserve the distinctions established here between signal meaning, payload data, priority, confidence, trust, authority, delivery, and acceptance.

## 1. Purpose

ACS-0003 defines how bounded meaning is communicated through Project Prometheus.

Signals allow participants to report observations, influence cognitive operation, request work, invoke authorized operations, present evidence, and report results.

A signal is not merely a packet of bytes. It possesses defined semantics that determine:

- what the sender claims or requests;
- which part of the organism the meaning concerns;
- how long the meaning remains useful;
- what validation is required;
- what authority is necessary;
- how the receiver may interpret it;
- whether associated payload data may be accessed;
- how rejection, expiration, duplication, or transformation must be handled.

This specification establishes a two-dimensional taxonomy based on:

1. **signal domain**, describing what part of the organism the signal concerns;
2. **signal intent**, describing what the sender is attempting to communicate or accomplish.

The taxonomy is intentionally extensible. Later specifications may define domain-specific subtypes without weakening the universal signal requirements established here.

## 2. Scope

This specification governs:

- the distinction between signals and payloads;
- signal domains;
- signal intents;
- signal subtypes and schema identity;
- signal traits;
- semantic validation;
- freshness and expiration;
- provenance;
- priority;
- confidence;
- cognitive influence strength;
- authority;
- recipient scope;
- payload references;
- duplicate and replay handling;
- delivery, acceptance, acknowledgment, and rejection;
- bridge mediation;
- signal transformation;
- protected infrastructure capacity;
- prohibited interpretations of signals.

This specification does not yet define:

- binary or textual signal encodings;
- concrete network headers;
- exact timestamp formats;
- exact identifier widths;
- queue implementations;
- transport reliability protocols;
- cryptographic algorithms;
- concrete signal registries;
- signal-processing APIs;
- domain-specific cognitive vocabularies;
- complete immune evidence policy;
- lifecycle state machines for connections or relationships;
- admission and resource-budget algorithms.

Those subjects belong to later ACS specifications and implementation profiles.

## 3. Terminology

### 3.1 Signal

A signal is a bounded, typed, semantically defined unit of communication through which one participant expresses an observation, influence, request, directive, item of evidence, or response.

A signal is intended to be inexpensive enough to validate, route, reject, expire, and account for without requiring interpretation of an arbitrarily large body of data.

A signal may stand alone or may reference one or more separately governed payloads.

### 3.2 Payload

A payload is substantive data whose size, complexity, cost, sensitivity, or lifetime exceeds that of an ordinary signal.

Examples may include:

- image data;
- audio data;
- latent tensors;
- memory objects;
- model state;
- checkpoints;
- stored immune evidence;
- diagnostic logs;
- large health reports;
- bulk sensor data;
- executable work descriptions;
- serialized cognitive state.

A payload is not granted signal semantics merely because it is transmitted through the same transport.

### 3.3 Payload reference

A payload reference is a bounded description that allows an authorized recipient to identify, validate, locate, request, or access a payload.

A payload reference is part of a signal's meaning but is not the referenced payload itself.

### 3.4 Domain

A signal domain identifies the broad part of the organism that the signal concerns.

Domain answers:

> What kind of organizational meaning does this signal carry?

The initial domains are:

- cognitive;
- operational;
- immune;
- security and trust.

### 3.5 Intent

A signal intent identifies what the sender is attempting to communicate or cause.

Intent answers:

> What is the sender doing by sending this signal?

The initial intents are:

- observation;
- influence;
- request;
- directive;
- evidence;
- response.

### 3.6 Signal subtype

A signal subtype provides a more specific semantic definition within a domain and intent.

For example:

```text
Domain: Operational
Intent: Observation
Subtype: Thermal condition
```

or:

```text
Domain: Immune
Intent: Evidence
Subtype: Inconsistent challenge response
```

Subtypes must possess explicit schemas and version identity.

### 3.7 Signal trait

A signal trait describes a bounded property of a signal without changing its domain, intent, or subtype.

Traits may describe:

- urgency;
- sensitivity;
- reliability expectation;
- temporal form;
- idempotence;
- transformability;
- aggregation permission;
- delivery expectation;
- recipient scope;
- retention permission.

### 3.8 Source

The source is the authenticated or otherwise identified logical participant responsible for originating the signal.

The transport sender and logical source may differ when a signal is forwarded or mediated.

### 3.9 Recipient

A recipient is a logical participant permitted to receive and evaluate a signal.

A signal may address one recipient or an explicitly bounded recipient scope.

### 3.10 Provenance

Provenance describes where a claim, observation, evidence item, or transformed signal originated and what processing occurred before delivery.

Provenance may include:

- original source;
- observing component;
- transformation history;
- aggregation history;
- evidence source;
- relevant execution context;
- trust boundary crossings.

### 3.11 Acceptance

Acceptance means that a recipient has validated a signal sufficiently to consider its meaning according to local policy.

Acceptance does not necessarily mean that the signal's claim is true, that its request will be fulfilled, or that its directive has completed.

## 4. Signal and payload separation

### 4.1 Separation principle

Signals and payloads shall remain architecturally distinct.

A signal communicates bounded meaning.

A payload contains substantive data.

A large object shall not be treated as an ordinary signal merely to bypass payload budgeting, retention, access control, or transfer policy.

### 4.2 Purpose of the separation

The separation exists so that the organism may:

- route control meaning without moving bulk data unnecessarily;
- reject invalid or unauthorized communication before payload access;
- prioritize infrastructure signals independently of payload traffic;
- prevent large data transfers from exhausting signal-processing capacity;
- reference data already available near the recipient;
- preserve payload-specific security and retention rules;
- expire a signal independently of long-term stored evidence;
- retain historical data without leaving an active signal permanently influential.

### 4.3 Payload-bearing communication

A signal may:

- contain a small bounded value directly;
- contain a payload reference;
- identify several related payload references when explicitly permitted;
- report that a payload is available;
- request access to a payload;
- report payload validation or transfer status.

The payload's availability does not imply that every recipient is authorized to access it.

### 4.4 Payload access

Payload access must remain subject to:

- authentication;
- authorization;
- sensitivity policy;
- integrity validation;
- resource budgets;
- retention rules;
- freshness requirements;
- availability state.

A valid signal containing a payload reference does not automatically authorize retrieval.

### 4.5 Historical persistence

Historical observations, memories, logs, evidence, and records belong in appropriate storage or memory systems.

They must not remain active merely because their originating signal was once valid.

A stored record may later be referenced by a new signal whose freshness, authority, and relevance are evaluated independently.

## 5. Taxonomy model

### 5.1 Type composition

The semantic type of a signal is formed from:

1. domain;
2. intent;
3. subtype;
4. subtype schema version.

Traits supplement that semantic identity.

A receiver must not infer signal meaning from transport channel, queue name, relationship class, sender implementation, or payload format alone.

### 5.2 Domain and intent are independent

The same intent may exist in several domains.

Examples include:

```text
Cognitive observation
Operational observation
Immune observation
Security observation
```

Likewise, one domain may use several intents:

```text
Immune observation
Immune evidence
Immune request
Immune response
```

This prevents the creation of a separate top-level type for every possible semantic combination.

### 5.3 Relationship class does not determine signal type

A relationship class may constrain which signal domains, intents, and subtypes it is permitted to carry.

It does not redefine the signal's meaning.

For example:

- a bridge relationship may mediate a cognitive influence;
- an infrastructure relationship may carry an operational observation;
- an immune relationship may carry immune evidence;
- a synaptic relationship may carry a cognitive request.

A relationship must not silently reinterpret one domain as another.

### 5.4 Signal class does not grant authority

A signal's domain, intent, or subtype does not independently grant authority.

An immune directive is not authorized merely because its domain is immune.

An operational directive is not authorized merely because it concerns infrastructure.

Authorization depends on:

- source identity;
- granted capabilities;
- target policy;
- relationship permissions;
- signal freshness;
- security state;
- applicable constraints.

## 6. Universal signal requirements

Every signal shall have an explicitly defined semantic type.

Every signal shall provide, directly or through a bound communication context, sufficient information to determine:

- domain;
- intent;
- subtype;
- schema version;
- source identity;
- recipient or recipient scope;
- freshness or expiration condition;
- applicable validation rules;
- required authority;
- priority or scheduling class where relevant;
- sensitivity where relevant;
- correlation identity where relevant;
- payload references where present;
- provenance where required.

A concrete implementation may omit repeated fields from every encoded signal when an authenticated and unambiguous context supplies them.

Context compression must not make meaning ambiguous.

### 6.1 Boundedness

Every signal shall be bounded in:

- encoded size;
- validation cost;
- processing cost;
- queue occupancy;
- forwarding behavior;
- retry behavior;
- useful lifetime.

### 6.2 Semantic validation

A signal shall be rejected when:

- its domain, intent, subtype, or schema is unsupported;
- required fields or contextual bindings are missing;
- its meaning is ambiguous;
- its values violate the subtype schema;
- the source is unauthorized;
- the target is invalid;
- the signal is expired;
- required provenance is absent;
- required payload references are malformed;
- integrity validation fails;
- replay protection fails where required.

### 6.3 Explicit uncertainty

Signals may represent unknown, partial, uncertain, or stale information.

Such states must be explicit.

A signal shall not silently convert unknown information into:

- false;
- zero;
- absent;
- healthy;
- current;
- trusted;
- complete.

## 7. Signal domains

## 7.1 Cognitive domain

### 7.1.1 Definition

A cognitive signal concerns perception, memory, attention, association, reasoning, learning, prediction, planning, affective influence, homeostatic influence, coordination, or behavior.

### 7.1.2 Purpose

Cognitive signals allow logical cognitive entities to influence one another without requiring transport-specific coupling.

### 7.1.3 Cognitive signal examples

Cognitive subtypes may later include concepts such as:

- activation;
- inhibition;
- salience;
- attention;
- novelty;
- expectation;
- prediction error;
- association;
- confidence;
- recall cue;
- memory availability;
- emotional influence;
- homeostatic influence;
- coordination request;
- sensory feature availability;
- action proposal.

These examples are not a frozen cognitive vocabulary.

Later cognitive specifications may introduce domain-specific subtypes.

### 7.1.4 Cognitive influence is not command

Cognitive signals should generally influence rather than command.

A cognitive influence may affect:

- activation level;
- weighting;
- attention;
- interpretation;
- recall likelihood;
- prioritization;
- association strength.

It does not automatically force a deterministic action.

### 7.1.5 Cognitive strength

A cognitive signal may possess a bounded influence strength.

Influence strength shall remain distinct from:

- source trust;
- confidence;
- priority;
- authority;
- repetition count;
- payload magnitude.

A strong signal may still be rejected.

A trusted signal may still have weak cognitive influence.

### 7.1.6 Cognitive signal prohibitions

A cognitive signal shall not inherently:

- bypass admission or security;
- grant infrastructure authority;
- grant immune authority;
- remain permanently active without persistence rules;
- treat repetition as independent corroboration;
- convert delivery into truth;
- create unbounded recurrent activation.

## 7.2 Operational domain

### 7.2.1 Definition

An operational signal concerns the conditions required for the organism and its services to remain available, observable, schedulable, recoverable, and coherent.

### 7.2.2 Purpose

Operational signals communicate state and coordination information needed to sustain operation.

### 7.2.3 Operational signal examples

Operational subtypes may include:

- health state;
- capacity;
- utilization;
- resource pressure;
- thermal condition;
- power condition;
- timing quality;
- synchronization state;
- congestion;
- topology change;
- service availability;
- lifecycle state;
- recovery progress;
- shutdown readiness;
- transport degradation;
- storage condition;
- accelerator availability.

### 7.2.4 Protected capacity

Operational signals required for health, lifecycle, security, congestion control, and recovery shall have protected processing and delivery capacity.

Protected capacity does not mean unlimited or permanently highest priority.

It means ordinary cognitive payload traffic must not consume every resource needed to preserve observability, regulation, and recovery.

### 7.2.5 Operational state refresh

Many operational signals represent current state rather than permanent fact.

Such signals shall define:

- a maximum useful age;
- a refresh expectation;
- a lease;
- a sequence epoch;
- or another explicit freshness mechanism.

When refresh stops, the state becomes stale or unknown.

It must not remain silently healthy or current.

### 7.2.6 Operational signal prohibitions

An operational signal shall not inherently:

- grant unrestricted hardware control;
- bypass authorization;
- carry unlimited private cognitive content;
- remain current after its freshness guarantee expires;
- hide missing telemetry by reporting default healthy values;
- consume unbounded protected capacity.

## 7.3 Immune domain

### 7.3.1 Definition

An immune signal concerns suspected harm, anomaly, corruption, inconsistency, integrity, containment, immune coordination, or recovery verification.

### 7.3.2 Purpose

Immune signals allow bounded protective participants to exchange observations and evidence without collapsing suspicion, judgment, authorization, and action into one step.

### 7.3.3 Immune progression

Immune communication should preserve distinctions such as:

```text
Observation
    ↓
Evidence
    ↓
Assessment
    ↓
Recommendation
    ↓
Authorized directive
    ↓
Action result
    ↓
Recovery verification
```

No stage silently possesses the authority of the next stage.

### 7.3.4 Immune signal examples

Immune subtypes may include:

- anomalous behavior observed;
- integrity mismatch;
- inconsistent challenge response;
- evidence available;
- evidence correlation request;
- suspicion assessment;
- trust-restriction recommendation;
- containment recommendation;
- independent verification request;
- recovery verification result;
- immune-coverage degradation;
- immune-shard disagreement.

### 7.3.5 Suspicion is not proof

An immune observation or assessment shall not be interpreted as proof of compromise merely because it originates from an immune participant.

Immune signals must preserve:

- uncertainty;
- provenance;
- evidence references;
- source identity;
- freshness;
- scope;
- confidence;
- disagreement where present.

### 7.3.6 Independent observations

Repeated signals from one shard shall not be treated as equivalent to corroboration from several independent shards.

Where independent corroboration matters, provenance must distinguish:

- repeated observation by one source;
- repeated observation through one shared failure domain;
- observation by independently placed sources;
- transformed copies of the same original evidence.

### 7.3.7 Immune authority

An immune relationship may carry a directive only when the source holds the separately granted capability required for that directive.

Immune classification alone does not authorize:

- quarantine;
- destruction;
- state erasure;
- hardware shutdown;
- credential revocation;
- permanent trust removal.

### 7.3.8 Immune signal prohibitions

An immune signal shall not:

- represent suspicion as certainty without support;
- propagate indefinitely without expiration or reassessment;
- hide evidence provenance;
- gain authority through repetition;
- bypass auditing;
- become immune to rejection or immune evaluation;
- treat one shard's claim as organism-wide fact.

## 7.4 Security and trust domain

### 7.4.1 Definition

A security and trust signal concerns authentication, authorization, credential state, trust state, secure-session state, revocation, policy enforcement, or security-boundary changes.

### 7.4.2 Purpose

Security and trust signals communicate changes or requests that affect who may communicate, what may be accessed, and which operations are permitted.

### 7.4.3 Security and trust examples

Subtypes may include:

- authentication completed;
- authentication failed;
- authorization granted;
- authorization denied;
- credential expiring;
- credential revoked;
- secure session established;
- secure session degraded;
- replay detected;
- trust state changed;
- capability granted;
- capability revoked;
- policy mismatch;
- security posture degraded.

### 7.4.4 Security state is explicit

Security state shall not be inferred solely from:

- transport establishment;
- physical locality;
- relationship class;
- previous successful authentication;
- source familiarity;
- network address;
- historical trust.

### 7.4.5 Trust changes

A signal reporting a trust change must identify:

- the affected identity or scope;
- the basis for the change;
- the authority responsible;
- the effective time or epoch;
- the expiration or persistence rule;
- the relevant policy version where applicable.

### 7.4.6 Security and trust prohibitions

Security and trust signals shall not:

- expose sensitive capability descriptions before required authentication;
- grant authority without an authorized source;
- silently change relationship identity;
- treat secure delivery as proof that content is correct;
- rely on relationship importance as a substitute for authorization;
- remain effective after explicit revocation.

## 8. Signal intents

## 8.1 Observation intent

### 8.1.1 Definition

An observation reports a condition, event, measurement, or detected state.

### 8.1.2 Interpretation

An observation represents what the source reports having observed.

It does not automatically establish:

- truth;
- cause;
- authority;
- required action;
- global state.

### 8.1.3 Requirements

An observation should identify, where relevant:

- observed subject;
- observation context;
- observation time or age;
- measurement quality;
- uncertainty;
- provenance;
- scope.

## 8.2 Influence intent

### 8.2.1 Definition

An influence attempts to alter interpretation, weighting, attention, activation, association, or other cognitive behavior without directly invoking an authorized operation.

### 8.2.2 Scope

Influence is principally associated with the cognitive domain.

Other domains should use influence only when the semantics remain unambiguous.

### 8.2.3 Requirements

An influence should define:

- target of influence;
- bounded strength;
- duration or decay;
- applicable context;
- interaction with existing influences;
- whether recurrence is allowed.

### 8.2.4 Prohibitions

Influence shall not be used to disguise:

- a directive;
- a privileged control operation;
- an infrastructure override;
- an immune verdict.

## 8.3 Request intent

### 8.3.1 Definition

A request asks a participant or service to perform, consider, provide, retrieve, evaluate, or schedule an operation.

### 8.3.2 Authority

A request does not imply that the receiver must comply.

The receiver may:

- accept;
- reject;
- defer;
- partially fulfill;
- redirect;
- report inability;
- require additional authorization.

### 8.3.3 Request requirements

A request should define:

- requested operation;
- parameters;
- target;
- response expectation;
- expiration;
- correlation identity;
- required authorization;
- resource bounds where relevant.

## 8.4 Directive intent

### 8.4.1 Definition

A directive invokes a specifically authorized operation or state change.

### 8.4.2 Directive requirements

A directive shall require:

- authenticated source identity;
- explicit authorization;
- permitted target;
- defined operation;
- bounded parameters;
- expiration or valid execution window;
- replay resistance where relevant;
- observable result or status;
- auditability where required.

### 8.4.3 Directive interpretation

A directive is not a universal command.

It is valid only within the capability and scope granted to its source.

### 8.4.4 Directive prohibitions

A directive shall not gain authority from:

- urgency;
- priority;
- relationship class;
- sender confidence;
- repeated delivery;
- physical locality;
- secure transport alone.

## 8.5 Evidence intent

### 8.5.1 Definition

Evidence presents or references information intended to support evaluation of a claim, condition, assessment, or decision.

### 8.5.2 Evidence structure

Evidence may be:

- contained directly when small;
- referenced through one or more payload references;
- aggregated from several observations;
- transformed or summarized when provenance is preserved.

### 8.5.3 Evidence requirements

Evidence shall identify, where relevant:

- subject;
- source;
- collection context;
- creation time or age;
- integrity state;
- transformation history;
- confidence or uncertainty;
- relationship to the claim being evaluated.

### 8.5.4 Evidence is not verdict

Evidence may support an assessment.

It does not independently establish:

- guilt;
- compromise;
- truth;
- required containment;
- authorization.

## 8.6 Response intent

### 8.6.1 Definition

A response reports status or outcome related to another signal, request, directive, transfer, or operation.

### 8.6.2 Response examples

Responses may indicate:

- received;
- rejected;
- accepted;
- deferred;
- in progress;
- partially completed;
- completed;
- failed;
- expired;
- unauthorized;
- unsupported;
- payload unavailable.

### 8.6.3 Correlation

A response shall identify the signal or operation to which it refers.

### 8.6.4 Response interpretation

A receipt acknowledgment means only that delivery was observed.

It does not necessarily mean:

- semantic validation succeeded;
- the request was accepted;
- the directive was authorized;
- the operation completed;
- the claimed effect occurred.

## 9. Signal traits

Traits supplement semantic type.

They shall not be used to redefine domain or intent.

### 9.1 Priority

Priority describes scheduling urgency or service importance.

Priority is not authority.

### 9.2 Sensitivity

Sensitivity describes confidentiality or disclosure restrictions.

Sensitivity may influence:

- permitted transport;
- storage;
- forwarding;
- logging;
- recipient scope;
- payload access.

### 9.3 Reliability expectation

A signal may be:

- best effort;
- delivery-confirmed;
- acceptance-confirmed;
- result-confirmed.

These expectations must remain explicit.

### 9.4 Temporal form

A signal may represent:

- an event;
- a current state snapshot;
- a leased state;
- a transition;
- a periodic refresh;
- a bounded continuing influence.

Temporal form determines expiration behavior.

### 9.5 Idempotence

A signal subtype may declare whether repeated processing is safe.

Non-idempotent directives require stronger duplicate and replay protection.

### 9.6 Transformability

A subtype may declare whether bridges or intermediaries may:

- forward unchanged;
- summarize;
- aggregate;
- filter;
- redact;
- transform.

Transformation permission must remain explicit.

### 9.7 Retention permission

A signal may specify whether it may be:

- discarded after processing;
- retained temporarily;
- retained for audit;
- converted into stored evidence;
- incorporated into memory.

Retention does not preserve active influence beyond the signal's valid lifetime.

## 10. Freshness and expiration

### 10.1 Expiration principle

Signals shall not remain active indefinitely unless the subtype explicitly defines persistent semantics.

Most signals shall expire.

### 10.2 Freshness mechanisms

A signal may define freshness through one or more mechanisms such as:

- maximum relative age;
- absolute deadline;
- monotonic sequence context;
- epoch identifier;
- lease duration;
- refresh interval;
- bounded hop lifetime;
- operation state;
- connection-local timing context.

This specification does not require globally synchronized wall clocks.

### 10.3 Clock uncertainty

Where clocks are not reliably synchronized, receivers shall not treat an unverified remote wall-clock timestamp as exact truth.

Implementations may use:

- measured signal age;
- bounded clock uncertainty;
- monotonic local timing;
- authenticated time services;
- sequence epochs;
- lease renewal;
- sender-receiver negotiated timing context.

### 10.4 State expiration

A state signal becomes stale when its freshness guarantee is no longer satisfied.

Stale state shall be represented explicitly.

It must not be silently reused as current state.

### 10.5 Influence decay

A cognitive influence may define:

- immediate expiration;
- linear decay;
- nonlinear decay;
- refresh-dependent continuation;
- event-bounded continuation.

The exact mathematical behavior belongs to domain-specific specifications.

### 10.6 Evidence lifetime

The active signal presenting evidence may expire while the referenced evidence remains stored according to separate retention rules.

A later signal may reference retained evidence again.

The new signal must be evaluated according to its own freshness and authority.

## 11. Validation and admission

### 11.1 Validation stages

Signal processing may include:

1. structural validation;
2. schema validation;
3. integrity validation;
4. source authentication;
5. authorization validation;
6. freshness validation;
7. recipient-scope validation;
8. relationship-permission validation;
9. payload-reference validation;
10. semantic admission.

Not every implementation must expose these as separate runtime stages.

The architectural distinctions must remain preservable.

### 11.2 Rejection before cognition

Malformed, expired, unauthorized, unsupported, corrupt, replayed, or invalid signals shall be rejected before ordinary cognitive interpretation.

### 11.3 Local policy

A semantically valid signal may still be rejected because:

- the recipient lacks capacity;
- the relationship does not permit the subtype;
- the signal is irrelevant;
- the requested operation is unavailable;
- trust is insufficient;
- the recipient is degraded;
- the payload cannot be accessed;
- local policy forbids acceptance.

### 11.4 Rejection reporting

Rejection may produce a response when:

- the sender expects one;
- the recipient can safely provide one;
- responding will not create an amplification attack;
- security policy permits disclosure;
- resources remain available.

A system must not create unbounded rejection storms.

## 12. Priority, confidence, strength, trust, and authority

These properties shall remain separate.

### 12.1 Priority

Priority determines how urgently processing or delivery should be attempted.

Priority does not prove correctness.

### 12.2 Confidence

Confidence represents the source's or assessor's stated degree of certainty.

Confidence does not prove truth.

### 12.3 Influence strength

Influence strength describes intended cognitive effect.

Strength does not grant trust or authority.

### 12.4 Trust

Trust represents the receiver's or system's current willingness to rely on a source, relationship, claim, or evidence process.

Trust does not grant every capability.

### 12.5 Authority

Authority is permission to request or perform a defined operation within a defined scope.

Authority must be explicitly granted.

### 12.6 Separation requirement

No implementation shall silently collapse these properties into one scalar.

A highly trusted source may send a low-confidence observation.

A high-priority signal may come from a restricted source.

A strong cognitive influence may carry no directive authority.

An authorized directive may still be low priority.

## 13. Provenance, correlation, duplication, and replay

### 13.1 Provenance preservation

Signals whose meaning depends on observation, evidence, transformation, aggregation, or independent corroboration shall preserve sufficient provenance.

### 13.2 Forwarding

A forwarder shall not silently replace the original logical source with itself.

The forwarding participant may be recorded separately.

### 13.3 Transformation

A transformed signal shall identify:

- the original source where available;
- the transforming participant;
- the transformation applied;
- whether information was removed, summarized, aggregated, or redacted;
- the resulting schema version;
- any change in confidence or uncertainty.

### 13.4 Aggregation

An aggregate shall distinguish:

- number of contributing observations;
- number of independent sources;
- shared failure domains where known;
- duplicated underlying evidence;
- aggregation method;
- uncertainty introduced by aggregation.

### 13.5 Correlation

Related signals should use explicit correlation identity.

Correlation may connect:

- request and response;
- directive and result;
- observation and evidence;
- assessment and recommendation;
- payload availability and transfer status;
- recovery action and verification.

### 13.6 Duplicate handling

Duplicate detection may rely on:

- signal identity;
- source sequence;
- correlation identity;
- operation identity;
- payload identity;
- replay window;
- subtype-specific idempotence.

Duplicate delivery must not automatically cause duplicate effect.

### 13.7 Replay protection

Signals capable of causing privileged, irreversible, expensive, or security-sensitive effects shall be replay-resistant.

## 14. Payload references

### 14.1 Reference requirements

A payload reference should provide or bind enough information to determine:

- payload identity;
- payload type or schema;
- expected size or bounds;
- integrity descriptor;
- availability state;
- access conditions;
- sensitivity;
- expiration or retention state;
- ownership or custody;
- location or retrieval mechanism where relevant.

### 14.2 Reference validity

A valid reference does not guarantee:

- the payload is still available;
- the payload is authorized for the recipient;
- the payload is current;
- transfer resources are available;
- the payload has not been revoked;
- the payload is semantically useful.

### 14.3 Multiple payloads

A signal may reference several payloads when the subtype explicitly permits it and the reference count remains bounded.

### 14.4 Payload integrity

Payload integrity shall be validated separately from signal integrity when the payload is accessed or transferred.

### 14.5 Payload lifetime

A signal and its payload may have different lifetimes.

The relationship between those lifetimes must remain explicit.

## 15. Delivery, acceptance, acknowledgment, and result

### 15.1 Delivery

Delivery means that the signal reached a receiving boundary.

### 15.2 Validation

Validation means that the signal passed required structural, security, freshness, and policy checks.

### 15.3 Acceptance

Acceptance means that the recipient admitted the signal for semantic consideration or processing.

### 15.4 Fulfillment

Fulfillment means that a request or directive was performed to some defined degree.

### 15.5 Verification

Verification means that the claimed result was independently or procedurally confirmed.

These states must not be treated as equivalent.

### 15.6 Acknowledgment profiles

A subtype or request may require:

- no acknowledgment;
- delivery acknowledgment;
- acceptance response;
- progress response;
- final result response;
- independent verification.

Acknowledgment requirements must remain bounded to avoid unnecessary traffic.

## 16. Recipient scope

### 16.1 Individual recipient

A signal may address one identified participant.

### 16.2 Group scope

A signal may address an explicitly defined group, service class, region, role, or bounded membership set.

### 16.3 Broadcast restrictions

Unrestricted organism-wide broadcast shall not be the default.

A broadcast-capable signal must define:

- authorized source;
- bounded audience;
- purpose;
- priority;
- expiration;
- amplification limits;
- forwarding limits.

### 16.4 Membership changes

A group-scoped signal must define whether its recipient set is evaluated:

- when sent;
- when delivered;
- when accepted;
- according to a named membership epoch.

### 16.5 Privacy and disclosure

A group scope must not reveal sensitive information to participants that lack authorization merely because they belong to the broader group.

## 17. Bridge mediation

### 17.1 Bridge responsibility

A bridge relationship may forward, filter, summarize, aggregate, transform, redact, or reject signals according to explicit policy.

### 17.2 Meaning preservation

A bridge shall not silently change:

- domain;
- intent;
- source identity;
- authority;
- confidence;
- freshness;
- evidence independence;
- sensitivity.

Any permitted change must be represented explicitly.

### 17.3 Provenance through bridges

Bridge mediation shall preserve sufficient provenance for the receiver to understand that mediation occurred.

### 17.4 Filtering

A bridge may filter signals based on:

- subtype;
- source;
- target;
- priority;
- sensitivity;
- freshness;
- trust;
- budget;
- relevance;
- regional policy.

Filtering must not silently fabricate healthy or empty state.

### 17.5 Aggregation

Bridge aggregation may reduce traffic, but it must not falsely represent repeated or correlated observations as independent evidence.

### 17.6 Backpressure

A bridge may apply backpressure or rejection when budgets are exceeded.

Backpressure must not silently convert an undelivered signal into a successful one.

## 18. Capacity protection and backpressure

### 18.1 Protected signal capacity

The organism shall preserve sufficient capacity for signals required to maintain:

- health visibility;
- security;
- trust;
- congestion management;
- lifecycle coordination;
- recovery;
- shutdown;
- immune coverage reporting.

### 18.2 Bounded protection

Protected capacity shall itself remain bounded.

A malicious or defective source must not exhaust the organism merely by labeling every signal urgent or operational.

### 18.3 Backpressure behavior

Backpressure may cause:

- delayed delivery;
- reduced frequency;
- aggregation;
- sampling;
- rejection;
- degradation notice;
- suspension of low-priority traffic;
- refusal of new work.

### 18.4 Signal storms

Implementations shall guard against signal storms caused by:

- repeated retries;
- reciprocal rejection;
- acknowledgment loops;
- cascading state refresh;
- immune escalation loops;
- broadcast amplification;
- unstable topology updates.

## 19. Cross-domain and cross-class interaction

Signals from different domains may participate in one process without losing their identity.

For example:

1. a cognitive entity emits an unusual cognitive pattern;
2. an immune shard emits an immune observation;
3. another shard emits immune evidence;
4. an immune assessment recommends restriction;
5. a security and trust request asks whether capability reduction is authorized;
6. an authorized security directive changes a capability;
7. an operational response reports the applied lifecycle effect;
8. an immune signal later verifies recovery or continued risk.

Each signal retains its own domain and intent.

No signal silently becomes the next stage.

## 20. Prohibited interpretations

The following interpretations are prohibited:

- delivery means truth;
- delivery means acceptance;
- acceptance means fulfillment;
- fulfillment means verification;
- priority means authority;
- confidence means truth;
- cognitive strength means trust;
- repetition means independent corroboration;
- encryption means correctness;
- authentication means authorization;
- relationship class means capability;
- immune origin means unquestionable verdict;
- operational origin means unrestricted control;
- bridge transformation preserves meaning without provenance;
- stale state remains current;
- missing state means healthy;
- payload reference grants payload access;
- stored evidence remains an active signal;
- large payloads may bypass payload rules by being labeled signals;
- secure transport permits sensitive exchange before required authentication;
- one source may simulate consensus by repeating itself;
- a signal may trigger an implicit relationship-class transition.

## 21. Relationship to other ACS specifications

- **ACS-0000** defines the organism-centered architectural charter.
- **ACS-0001** requires signals to be typed, validated, expiring, bounded, and rejected before cognition when invalid.
- **ACS-0002** defines which relationship classes may carry signals and the authority boundaries between those relationships.
- **ACS-0004** will define endpoints and ports through which permitted signals are exposed and received.
- **ACS-0005** will define lifecycle behavior for relationships and connections.
- **ACS-0006** will define admission, signal budgets, queue budgets, and protected capacity.
- **ACS-0007** will define identity, trust, authorization, secure sessions, capability grants, replay resistance, and revocation.
- **ACS-0008** will define immune evidence, assessment, shard coordination, escalation, containment recommendations, and recovery verification.
- **ACS-0009** will define runtime integration and execution boundaries.

Later specifications may refine signal subtypes and processing rules.

They shall not silently collapse the distinctions established here.

## 22. Design rationale

Prometheus requires many kinds of communication.

A cognitive influence, thermal observation, immune evidence item, authorization directive, and recovery result are not equivalent merely because each can be encoded as bytes.

Without a semantic taxonomy:

- requests could be mistaken for commands;
- suspicion could become verdict;
- priority could become authority;
- stale state could remain active;
- large payloads could overwhelm control paths;
- repeated claims could imitate consensus;
- bridge transformations could erase provenance;
- delivery could be mistaken for successful action;
- secure transport could be mistaken for trustworthy meaning.

At the same time, defining a permanent top-level type for every possible signal would create an unmanageable and inflexible architecture.

ACS-0003 therefore separates:

- domain;
- intent;
- subtype;
- schema version;
- traits;
- payload references;
- trust;
- authority.

This provides semantic precision without freezing the future cognitive vocabulary of Prometheus.

## 23. Architectural consequences

Conforming implementations will require:

- explicit signal schemas;
- signal and payload separation;
- domain and intent identity;
- bounded validation cost;
- explicit freshness;
- explicit uncertainty;
- source and recipient identity;
- provenance where meaning depends on origin;
- separate priority, confidence, influence strength, trust, and authority;
- explicit directive authorization;
- duplicate and replay handling;
- bounded recipient scopes;
- bridge transformation records;
- protected operational capacity;
- bounded acknowledgment behavior;
- explicit rejection and degradation semantics;
- storage systems separate from active signal influence.

These consequences are intentional.

## 24. Open questions

The following questions remain unresolved:

- Should security and trust remain a separate domain or become a protected operational subdomain?
- Are there additional foundational domains that cannot be represented safely through the initial four?
- Should evidence remain an intent, or should some evidence forms also be represented as payload categories?
- Should directives remain signals, or should later implementation profiles place them in a more restricted control envelope?
- Which signal metadata is mandatory for extremely small local signals?
- How much metadata may be supplied by relationship or port context rather than repeated in each signal?
- Which freshness mechanisms are required when clocks cannot be synchronized?
- How should multi-recipient signal identity interact with changing group membership?
- Under what conditions may bridges transform cognitive signals?
- How should confidence be represented without implying false precision?
- How should cognitive influence strength combine across several sources?
- Which signals require acknowledgments?
- How are acknowledgment and rejection storms prevented?
- How should signal sampling and aggregation preserve uncertainty?
- How are duplicated observations distinguished from independent evidence?
- May one signal safely reference several payloads with different lifetimes?
- When should a signal become a persistent memory or stored evidence record?
- How should an unavailable payload affect acceptance of the referencing signal?
- Which signals may be retained for audit?
- How should degraded-security operation restrict signal domains and intents?
- How should unknown signal subtypes be forwarded, rejected, or isolated?
- Can a bridge preserve end-to-end authorization while transforming the signal?
- How are urgent but unauthorized signals handled safely?
- Which domain owns homeostatic influence when it affects both physical operation and cognition?

These questions should remain open until later specifications provide enough context to resolve them safely.

## 25. Future evolution

New domains, intents, traits, or universal requirements may be added when later work demonstrates that:

1. the new concept has distinct architectural meaning;
2. existing taxonomy elements cannot represent it safely;
3. the addition reduces rather than increases ambiguity;
4. authority and trust boundaries remain explicit;
5. backward interpretation can be managed;
6. the effect on later specifications and implementations is documented.

Domain-specific subtype registries may evolve independently when their schemas remain versioned and their meanings remain compatible with this specification.

While ACS-0003 remains Draft, major revisions are permitted.

Later stabilization should occur only after the taxonomy has been challenged against:

- cognitive communication;
- infrastructure health;
- distributed immune behavior;
- security and trust operations;
- payload transfer;
- bridge mediation;
- overloaded and degraded operation.

## 26. Closing principle

> **A signal may influence the organism only according to its meaning, freshness, provenance, trust, and explicitly granted authority.**

## Revision history

### Version 0.1 — 2026-07-14

- Established the initial signal taxonomy.
- Defined signal and payload separation.
- Established cognitive, operational, immune, and security-and-trust domains.
- Established observation, influence, request, directive, evidence, and response intents.
- Separated priority, confidence, cognitive strength, trust, and authority.
- Defined freshness, expiration, provenance, duplicate handling, and replay boundaries.
- Defined payload-reference requirements.
- Established bridge-mediation requirements.
- Established protected operational signal capacity.
- Recorded unresolved questions for later ACS specifications.
