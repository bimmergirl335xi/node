# BOOT-0004: Node BOOT Discovery and Assistance Negotiation

| Field | Value |
|---|---|
| Specification | BOOT-0004 |
| Title | Node BOOT Discovery and Assistance Negotiation |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | BOOT-PUB |
| Authors | Node |
| Last updated | 2026-07-18 |
| Approval | Pending review |
| Depends on | BOOT-0000; BOOT-0001; BOOT-0002; BOOT-0003; applicable approved ACS-0003 through ACS-0009 public architecture |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in discovery separation, progressive disclosure, candidate eligibility, transport neutrality, assistance-session lifecycle, negotiation semantics, bounded multi-source behavior, and failure truthfulness; concrete discovery mechanisms, transport profiles, compatibility identifiers, retry values, responder policy, and production assistance topology remain intentionally deferred |

> **Discovery may reveal a possible path to assistance; it never decides who the responder is, whether the responder may help, or what work may proceed.**

## Architectural-intent notice

This specification defines the public architecture governing Node BOOT:

- assistance needs;
- discovery requests;
- discovery observations;
- assistance advertisements;
- assistance offers;
- candidate normalization;
- candidate identity;
- candidate eligibility;
- progressive disclosure;
- responder comparison and selection;
- transport-neutral discovery adapters;
- security-provider interaction;
- ACS admission;
- assistance-session establishment;
- assistance-session identity and generation;
- capability negotiation;
- request, offer, counteroffer, acceptance, and refusal;
- evidence requests;
- plan-proposal requests;
- artifact and installer referrals;
- operator-access requests;
- multi-source assistance;
- mediation and referral;
- session interruption, resumption, cancellation, and closure;
- timeout and retry boundaries;
- boundedness and resource exhaustion;
- public conceptual records;
- implementation and conformance expectations.

This specification does not define one network protocol, transport, packet format, service-discovery product, responder-ranking algorithm, authentication protocol, secure-session protocol, production topology, private endpoint catalog, recovery plan, artifact manifest, installer API, or operator workflow.

Implementations may use different local, removable-media, network, directory, peer, or operator-assisted discovery mechanisms.

Their observable behavior MUST preserve the semantic distinctions, ownership boundaries, boundedness, authority constraints, revision behavior, and failure meanings established here.

## 1. Purpose

BOOT may determine that the local node cannot safely proceed through an ordinary runtime handoff.

The node may require assistance to obtain:

- identity evidence;
- authentication services;
- authority references;
- operator review;
- hardware or compatibility evidence;
- a recovery-plan proposal;
- artifact references;
- artifact-transfer access;
- release information;
- installer access;
- rollback information;
- recovery-verification support;
- another bounded recovery capability.

At the moment assistance is required, the node may know very little.

It may not know:

- which assistance sources exist;
- whether a discovered source is who it claims to be;
- whether the source supports the required operation;
- whether the source is currently available;
- whether the source is authorized;
- whether BOOT is admitted to use the source;
- whether a secure session can be established;
- whether the source can disclose its full capabilities;
- whether the source possesses current information;
- whether the source can satisfy the request;
- whether another source is preferable;
- whether several sources must cooperate;
- whether any response is safe to act upon.

A discovery system that treats reachability or responsiveness as trust would create an immediate authority vulnerability.

A negotiation system that treats a compatible offer as permission would create an immediate recovery vulnerability.

BOOT-0004 defines the path from:

```text
bounded assistance need
    → bounded discovery
    → candidate observation
    → identity and compatibility evaluation
    → authentication
    → authority evaluation
    → ACS admission
    → eligible assistance session
    → bounded negotiation
    → scoped assistance result
```

No earlier stage silently proves a later stage.

## 2. Authority and relationship to earlier BOOT specifications

BOOT-0000 establishes that BOOT may:

- advertise a bounded need for assistance;
- receive bounded assistance offers;
- compare declared capabilities and requirements;
- establish an eligible assistance session;
- request evidence, plans, artifacts, authority references, installer access, or operator review.

BOOT-0000 also establishes that BOOT MUST NOT:

- define transport packet formats;
- treat the first, closest, fastest, or most responsive source as authoritative;
- confuse transport completion with semantic completion;
- confuse authentication with authority;
- confuse authority with admission.

BOOT-0001 formalizes these requirements through:

- BOOT-INV-001 — Coordination does not absorb ownership.
- BOOT-INV-002 — Execution context grants no authority.
- BOOT-INV-003 — Adjacent-system ownership remains intact.
- BOOT-INV-004 — Trust and completion stages remain distinct.
- BOOT-INV-005 — BOOT session identity is not durable participant identity.
- BOOT-INV-006 — Identity is not location, order, proximity, or responsiveness.
- BOOT-INV-007 — Identity evidence requires revalidation.
- BOOT-INV-008 — BOOT cannot self-grant authority.
- BOOT-INV-009 — BOOT state remains multidimensional.
- BOOT-INV-010 — Unknown and unavailable conditions remain truthful.
- BOOT-INV-011 — Silence, connection events, and restart do not prove completion.
- BOOT-INV-012 — BOOT data and work remain bounded.
- BOOT-INV-013 — Resource exhaustion is first-class.
- BOOT-INV-014 — Authoritative state changes are conservatively atomic.
- BOOT-INV-015 — Revision and idempotency survive retry and interruption.
- BOOT-INV-016 — Responsiveness does not create preference or authority.
- BOOT-INV-017 — Transport and secure-session success do not establish semantic acceptance.
- BOOT-INV-018 — Assistance sessions remain bounded and attributable.
- BOOT-INV-019 — Requests and plans do not create authority.
- BOOT-INV-020 — Transfer and verification remain distinct.
- BOOT-INV-027 — Update availability does not create activation authority.
- BOOT-INV-028 — Peer propagation remains locally governed.
- BOOT-INV-029 — Public and private material remain separated.

BOOT-0002 defines:

- `BOOT_DIM_ASSISTANCE`;
- assistance-session phases;
- independent authority and artifact dimensions;
- explicit unknown, unavailable, invalid, deferred, failed, and indeterminate conditions;
- revisioned transitions;
- timeout and interruption behavior;
- derived dispositions.

BOOT-0003 defines:

- BOOT identity profiles;
- identity claims and evidence;
- authentication results;
- trust-state boundaries;
- secure-session boundaries;
- bootstrap authority;
- authority envelopes;
- ACS admission separation;
- revocation and conflict behavior.

BOOT-0004 refines discovery and assistance-session semantics while preserving those earlier specifications.

If a conflict exists:

1. the lower-numbered BOOT specification governs;
2. the conflict MUST be reported;
3. BOOT-0004 MUST be corrected;
4. implementations MUST NOT choose the more permissive interpretation.

No blocking contradiction was identified during drafting.

## 3. Normative language

The terms **MUST** and **MUST NOT** define mandatory architectural requirements.

The terms **SHOULD** and **SHOULD NOT** define strong recommendations. A departure requires documented justification and MUST NOT violate a mandatory requirement.

The term **MAY** defines permitted behavior.

Terms such as:

- discovered;
- visible;
- reachable;
- available;
- compatible;
- authenticated;
- admitted;
- eligible;
- selected;
- accepted;
- authorized;
- active;
- complete

MUST be interpreted only within their declared subject, scope, provider, revision, and evidence boundary.

No such term establishes all later stages.

## 4. Scope

This specification governs BOOT-facing discovery and negotiation involving:

- local assistance services;
- remote assistance services;
- removable media;
- installation media;
- preconfigured opaque references;
- public discovery facades;
- authenticated discovery facades;
- directories or registries;
- peer referrals;
- mediated assistance;
- operator-supplied candidate references;
- persisted and revalidated candidate references;
- installer referral;
- artifact-service referral;
- security-provider referral;
- authority-provider referral;
- release or update referral;
- recovery-verification services;
- bounded multi-source assistance.

It applies during:

- initial installation;
- first boot;
- ordinary rescue;
- credential recovery;
- identity re-enrollment;
- artifact recovery;
- installation recovery;
- rollback;
- update preparation;
- offline recovery;
- partitioned operation;
- dynamic assembly;
- runtime-handoff preparation.

## 5. Explicit non-goals

BOOT-0004 does not define:

- one discovery protocol;
- DNS, mDNS, DNS-SD, DHCP, Bluetooth, USB, PXE, HTTP, HTTPS, QUIC, TCP, UDP, Unix-socket, shared-memory, serial, or removable-media formats;
- numeric network-port assignments;
- packet framing;
- routing;
- retransmission;
- congestion control;
- link encryption;
- one service registry;
- one directory technology;
- one secure-session mechanism;
- one authentication protocol;
- one responder-ranking formula;
- production preference weights;
- production trust thresholds;
- one load-balancing algorithm;
- one recovery-plan schema;
- one artifact manifest;
- one installer request;
- one operator user interface;
- one production assistance topology;
- private responder-selection policy;
- private endpoint names or addresses.

These subjects belong to transport, ACS, security-provider architecture, BOOT-0005, later implementation profiles, future OS and update architecture, or private deployment policy.

## 6. Foundational distinctions

### 6.1 Assistance need is not a discovery request

An assistance need describes what BOOT requires.

A discovery request asks one or more mechanisms to identify candidate sources.

One need MAY produce several discovery requests.

### 6.2 Discovery request is not broadcast authority

A discovery request does not authorize every observer to:

- respond;
- obtain private state;
- access protected diagnostics;
- form a relationship;
- direct recovery;
- mutate the node.

### 6.3 Discovery observation is not identity

A discovery observation establishes only that a possible source, endpoint, medium, service, or reference may exist.

It does not establish who controls it.

### 6.4 Advertisement is not availability

An advertisement indicates that a source claims to offer a capability or discovery path.

It does not prove the source remains reachable or operational.

### 6.5 Availability is not readiness

A source may be reachable but unable to perform the requested operation.

### 6.6 Reachability is not authentication

The ability to exchange bytes with a candidate does not authenticate its claimed identity.

### 6.7 Authentication is not authority

An authenticated source may lack authority for the requested assistance operation.

### 6.8 Authority is not ACS admission

A source or requester may possess operation authority while current ACS admission remains unavailable, deferred, restricted, or denied.

### 6.9 Admission is not capability compatibility

ACS admission does not establish that the candidate supports the required BOOT semantic contract.

### 6.10 Capability compatibility is not operation acceptance

A candidate may support an operation class while rejecting a particular request.

### 6.11 Request acceptance is not work admission

A source may accept a request for consideration while deferring resource or operation admission.

### 6.12 Work admission is not work start

Admitted work may remain pending.

### 6.13 Work start is not completion

A source beginning work does not establish a completed assistance result.

### 6.14 Offer is not commitment

An assistance offer identifies what a candidate claims it may be willing or able to provide under declared conditions.

It is not a guarantee.

### 6.15 Offer is not authorization

An offer from a source does not authorize BOOT to use the offered capability.

### 6.16 Selection is not authorization

BOOT selecting a candidate does not grant permission to establish a session or use an operation.

### 6.17 Selection is not exclusivity

Selecting one candidate does not automatically invalidate or prohibit every other eligible candidate.

### 6.18 First response is not preference

Response order, latency, proximity, bandwidth, or computational capacity do not independently create eligibility or preference.

### 6.19 Secure session is not assistance session

A security-provider session protects communication.

A BOOT assistance session defines semantic recovery interaction.

### 6.20 Assistance session is not ACS relationship

An assistance session may use an eligible ACS relationship, connection, attachment, or facade.

It does not silently create or replace them.

### 6.21 Connection is not assistance-session continuity

A transport or ACS connection may close and be replaced while the semantic assistance session continues under explicit continuation rules.

### 6.22 Referral is not endorsement

A source referring BOOT to another source does not independently authenticate, authorize, or validate the referred source.

### 6.23 Mediation is not authority amplification

A mediator may preserve, narrow, reject, or equivalently translate authority within its assigned scope.

It MUST NOT silently enlarge authority.

### 6.24 Negotiation is not recovery-plan validation

Assistance negotiation may produce a plan proposal.

BOOT-0005 still validates the plan as a recovery object.

### 6.25 Negotiation is not recovery authorization

Agreement about proposed work does not authorize that work.

### 6.26 Transfer availability is not artifact verification

A source offering or initiating transfer does not establish artifact integrity, identity, authenticity, compatibility, or installation eligibility.

### 6.27 Session closure is not operation completion

A session may close while one or more semantic operations remain:

- incomplete;
- interrupted;
- failed;
- indeterminate;
- continuation-required.

## 7. Core terminology

### 7.1 Assistance need

An **assistance need** is a bounded BOOT statement identifying a required capability, evidence class, provider role, or recovery-related result.

### 7.2 Discovery scope

A **discovery scope** defines where, how broadly, and under what disclosure and resource limits candidate assistance may be sought.

### 7.3 Discovery mechanism

A **discovery mechanism** is an implementation method used to produce candidate observations.

It is not itself a BOOT assistance source.

### 7.4 Discovery adapter

A **discovery adapter** is a BOOT-facing provider boundary that translates a concrete discovery mechanism into transport-neutral candidate observations and results.

### 7.5 Candidate locator

A **candidate locator** is an opaque or structured transport-facing reference through which a connection attempt may be requested.

A locator is not identity.

### 7.6 Candidate endpoint

A **candidate endpoint** is a possible concrete communication target reported by a discovery or transport adapter.

It does not automatically establish ACS endpoint identity.

### 7.7 Assistance advertisement

An **assistance advertisement** is a bounded statement that a candidate claims one or more assistance-related capabilities may be available.

### 7.8 Assistance offer

An **assistance offer** is a candidate’s bounded response to an assistance need or discovery request.

### 7.9 Assistance candidate

An **assistance candidate** is one normalized BOOT record representing a possible source of assistance.

### 7.10 Assistance source

An **assistance source** is a local or remote participant, service, device, medium, or governed facade that may provide assistance.

A candidate becomes an accepted assistance source only within the relevant evaluation scope.

### 7.11 Candidate identity claim

A **candidate identity claim** states which participant, endpoint, role, device, medium, or service the candidate claims to represent.

### 7.12 Assistance capability

An **assistance capability** is a declared class of assistance the candidate may support.

### 7.13 Candidate compatibility

**Candidate compatibility** describes whether a candidate’s declared contract can represent the requested operation.

### 7.14 Candidate eligibility

**Candidate eligibility** is BOOT’s scoped conclusion that a candidate satisfies the prerequisites necessary to be considered for an identified assistance operation.

Eligibility is not authorization or operation acceptance.

### 7.15 Responder selection

**Responder selection** identifies the candidate or bounded candidate set BOOT proposes to use for an assistance operation.

### 7.16 Assistance-session identity

An **assistance-session identity** identifies one bounded semantic BOOT assistance session.

### 7.17 Assistance-session generation

An **assistance-session generation** identifies one revision of the session’s active bindings, security context, admission state, or negotiated contract.

### 7.18 Negotiation

**Negotiation** is bounded exchange used to determine whether BOOT and a candidate can agree on a scoped assistance operation and its prerequisites.

### 7.19 Negotiation round

A **negotiation round** is one bounded request, response, offer, counteroffer, or clarification step.

### 7.20 Disclosure tier

A **disclosure tier** identifies the maximum information that may be revealed to an observer under current identity, authentication, admission, and authority conditions.

### 7.21 Assistance operation

An **assistance operation** is one identified semantic request performed within or through an assistance session.

### 7.22 Referral

A **referral** identifies another candidate, directory, mediator, provider, artifact source, or authority path for later evaluation.

### 7.23 Semantic mediator

A **semantic mediator** is an authorized participant that materially filters, translates, redacts, aggregates, or governs BOOT assistance meaning.

### 7.24 Session continuation

**Session continuation** preserves semantic assistance-session identity across an explicitly governed reconnect, rebinding, restart, or reboot.

### 7.25 Session closure

**Session closure** ends the current assistance session or generation under a declared result.

## 8. Ownership boundaries

### 8.1 BOOT ownership

BOOT owns semantic coordination of:

- assistance needs;
- discovery requests;
- candidate observations;
- candidate normalization;
- candidate eligibility;
- responder selection;
- assistance-session identity;
- negotiation requests and results;
- operation correlation;
- session closure;
- BOOT-scoped state and failure reporting.

### 8.2 Transport ownership

Transport owns:

- byte movement;
- framing;
- addressing;
- connection attempts;
- retransmission;
- link behavior;
- transport timeout;
- transport closure;
- transport protocol mismatch.

BOOT MUST NOT redefine transport state as semantic assistance state.

### 8.3 ACS ownership

ACS owns:

- participant and endpoint identity;
- endpoint and port contracts;
- visibility;
- relationships;
- connections;
- bindings;
- attachments;
- admission;
- connection lifecycle;
- revisions and idempotency;
- applicable capability references.

### 8.4 Security-provider ownership

The security provider owns:

- authentication primitives;
- cryptographic verification;
- secure-session establishment;
- challenge generation;
- protected key custody;
- credential validation;
- replay-protection primitives.

### 8.5 Authority-provider ownership

Applicable authority providers own operation-scoped grants, denials, restrictions, delegation, concurrence, expiry, and revocation.

### 8.6 Installer ownership

The installer owns durable installation mutation, write verification, activation staging, and rollback.

### 8.7 Artifact-provider ownership

Artifact and security providers own artifact identity, custody, transfer results, and verification according to their contracts.

### 8.8 MEM ownership

MEM owns semantic persistence, custody, retention, reconstruction, restoration, and deletion authority.

### 8.9 IMM ownership

IMM owns immune observation, evidence assessment, recommendation, and IMM-scoped restoration responsibilities.

IMM evidence does not select or authorize an assistance source.

### 8.10 Resource-management ownership

Resource management owns allocation, reservations, ceilings, pressure policy, and reclamation.

BOOT cannot exceed those limits through discovery or negotiation.

## 9. Assistance-need model

### 9.1 Need identity

Every assistance need MUST have an identity suitable for:

- correlation;
- revision;
- duplicate detection;
- cancellation;
- continuation;
- bounded audit.

### 9.2 Need contents

An assistance need SHOULD identify:

- need identity;
- need revision;
- BOOT session;
- requesting subject;
- target;
- required operation class;
- required evidence or result;
- required authority class;
- compatibility requirements;
- resource limits;
- disclosure ceiling;
- urgency class where public;
- validity boundary;
- acceptable degraded behavior;
- prohibited actions;
- result contract.

### 9.3 Need minimization

A need SHOULD reveal only information required to discover compatible candidates.

Sensitive local conditions SHOULD NOT be disclosed before an applicable disclosure tier permits them.

### 9.4 Need revision

Material changes require a new need revision.

Material changes include:

- target change;
- operation change;
- authority requirement change;
- compatibility change;
- disclosure change;
- resource-limit change;
- validity change.

### 9.5 Need cancellation

Cancellation of a need does not automatically cancel:

- established transport;
- an assistance session;
- admitted provider work;
- installer mutation;
- artifact transfer.

Each owning operation requires its own cancellation result.

## 10. Discovery modes

A conforming architecture MAY support any bounded combination of the following.

### 10.1 Local provider discovery

BOOT may discover assistance services within the local rescue environment or installed system.

Local execution does not create authority.

### 10.2 Removable-media discovery

BOOT may inspect bounded assistance metadata on:

- installation media;
- recovery media;
- removable tokens;
- attached storage.

Media presence does not establish authenticity, authority, or target eligibility.

### 10.3 Preconfigured opaque references

BOOT may receive protected or public references provisioned by:

- an installer;
- operator policy;
- manufacturer provisioning;
- a prior validated generation;
- an authority provider.

Persisted references require applicable revalidation.

### 10.4 Directory or registry discovery

BOOT may query an eligible directory, registry, facade, or service catalog.

The directory’s response is candidate evidence, not final identity or authority.

### 10.5 Peer discovery

BOOT may receive candidate information from other nodes or peers.

Peer identity does not create release or recovery authority.

### 10.6 Referral discovery

An assistance source may refer BOOT to another candidate.

The referral requires independent evaluation unless a separate authority contract establishes otherwise.

### 10.7 Operator-provided discovery

An authenticated operator may supply a candidate reference.

Operator identity and candidate selection do not replace candidate authentication or operation authority.

### 10.8 Hardware- or platform-provided discovery

Firmware, secure hardware, platform services, or manufacturer mechanisms may provide candidate references.

Hardware origin does not automatically establish BOOT operation authority.

### 10.9 Offline discovery

BOOT may discover assistance through local media or preprovisioned references without networking.

Offline operation remains subject to identity, freshness, revocation, and authority requirements.

### 10.10 No mandatory network

A conforming BOOT discovery architecture MUST NOT require continuous or universal network availability.

## 11. Discovery-scope model

### 11.1 Explicit scope

Every discovery request MUST declare or reference a bounded scope.

### 11.2 Scope dimensions

A discovery scope MAY constrain:

- discovery mechanisms;
- local versus remote search;
- trust domains;
- candidate classes;
- operation classes;
- architecture or platform class;
- compatibility version;
- disclosure tier;
- result count;
- response size;
- time budget;
- retry budget;
- referral depth;
- mediation depth;
- concurrent work;
- persisted-result behavior.

### 11.3 Scope expansion

Discovery scope MUST NOT silently expand.

A broader search requires:

- a new request revision;
- an applicable policy or authority basis;
- updated privacy and resource evaluation.

### 11.4 Empty result

An empty discovery result means no candidate was produced within the declared scope and observation boundary.

It does not prove that no assistance source exists anywhere.

## 12. Progressive disclosure

### 12.1 Disclosure principle

Discovery SHOULD expose the minimum information necessary for the observer’s current eligibility state.

### 12.2 Disclosure tiers

A public implementation SHOULD support behavior equivalent to:

| Tier | Permitted disclosure |
|---|---|
| `DISCLOSURE_NONE` | No assistance information is revealed. |
| `DISCLOSURE_MINIMAL_PUBLIC` | A minimal public capability or compatibility hint may be disclosed. |
| `DISCLOSURE_IDENTITY_PENDING` | Information needed to begin identity or authentication evaluation may be disclosed. |
| `DISCLOSURE_AUTHENTICATED` | Bounded purpose, public versions, and further requirements may be disclosed after authentication. |
| `DISCLOSURE_ADMITTED` | Admission-specific compatibility, budget, and operation information may be disclosed. |
| `DISCLOSURE_AUTHORIZED` | Complete contract information for the authorized scope may be disclosed. |

Equivalent labels are permitted.

Materially different disclosure states MUST remain distinguishable.

### 12.3 No false disclosure

Reduced disclosure may omit capabilities.

It MUST NOT provide materially false information concerning:

- identity;
- compatibility;
- direction;
- authority;
- availability;
- completion expectations.

### 12.4 No complete enumeration requirement

No source is required to reveal every assistance capability, endpoint, port, artifact, provider, or internal dependency.

### 12.5 Concealment is not security completion

Concealed candidates still require authentication, authorization, validation, and resource enforcement when accessed through another path.

## 13. Discovery-request contract

A conceptual discovery request contains:

```text
BootDiscoveryRequest
    request_identity
    request_revision
    boot_session_identity
    assistance_need_reference
    requester_identity_condition
    discovery_scope
    requested_operation_classes
    compatibility_requirements
    disclosure_ceiling
    result_count_limit
    response_size_limit
    time_budget
    retry_budget
    referral_depth_limit
    result_contract
```

This is a language-neutral semantic contract.

It is not a required wire format.

### 13.1 Request validation

Before issue, BOOT MUST validate:

- identity and revision;
- bounded lengths and counts;
- supported operation classes;
- disclosure ceiling;
- resource limits;
- applicable authority for non-public discovery;
- result contract.

### 13.2 Duplicate requests

Duplicate delivery of the same request MUST NOT:

- create a new BOOT need;
- create independent evidence;
- expand disclosure;
- reset retry budgets;
- create authority.

## 14. Discovery observation

A discovery adapter produces bounded observations rather than accepted candidates.

A conceptual observation contains:

```text
BootDiscoveryObservation
    observation_identity
    request_identity
    adapter_identity
    mechanism_class
    candidate_locator
    claimed_identity_reference
    claimed_endpoint_reference
    advertised_operation_classes
    compatibility_hints
    disclosure_tier
    observed_availability
    observation_revision
    freshness_state
    transport_metadata_reference
    bounded_issues
```

### 14.1 Observation attribution

Every observation MUST identify its producing adapter or provider.

### 14.2 Observation uncertainty

Unknown identity, unavailable validation, conflicting locators, stale metadata, or unsupported compatibility MUST remain explicit.

### 14.3 Observation does not mutate candidate authority

Creating a discovery observation MUST NOT alter authority, admission, or installer state.

## 15. Assistance advertisements and offers

### 15.1 Advertisement contents

An advertisement MAY identify:

- claimed source identity;
- claimed endpoint or facade;
- supported operation classes;
- public compatibility versions;
- authentication requirements;
- admission requirements;
- authority requirements;
- resource expectations;
- availability window;
- referral capabilities;
- disclosure tier.

### 15.2 Offer contents

An assistance offer SHOULD identify:

- offer identity;
- offer revision;
- responding candidate;
- need and request references;
- offered operation;
- supported parameters;
- required identity state;
- required authentication;
- required authority;
- required admission;
- resource limits;
- validity boundary;
- expected next step;
- limitations;
- refusal conditions.

### 15.3 Offer freshness

An offer may become:

- stale;
- expired;
- withdrawn;
- superseded;
- conflicting;
- unavailable.

### 15.4 Offer withdrawal

Withdrawal of an offer does not automatically:

- revoke unrelated authority;
- invalidate completed evidence;
- terminate every transport;
- cancel already admitted work.

Each applicable state must change explicitly.

## 16. Candidate normalization

### 16.1 Normalized candidate record

BOOT SHOULD normalize discovery observations into bounded assistance-candidate records.

### 16.2 Candidate correlation

Observations MAY be correlated using:

- authenticated participant identity;
- endpoint identity;
- provider references;
- signed continuity evidence;
- eligible stable opaque identifiers;
- security-provider results.

Address equality alone is insufficient.

### 16.3 Duplicate observations

Repeated observations of one candidate MUST NOT become several independent candidates solely because they arrived through:

- several transports;
- several addresses;
- repeated advertisements;
- several referrals;
- several encodings.

### 16.4 Candidate separation

Candidates MUST remain separate when identity continuity cannot be established.

### 16.5 Conflicting candidate identity

Conflicting observations MUST NOT be silently merged.

The candidate state remains conflicting or unresolved until an applicable provider resolves continuity.

## 17. Candidate-state model

Candidate state is subordinate to, and does not replace, `BOOT_DIM_ASSISTANCE`.

A public implementation SHOULD support values equivalent to:

| State | Meaning |
|---|---|
| `CANDIDATE_OBSERVED` | At least one bounded observation exists. |
| `CANDIDATE_NORMALIZED` | A candidate record was constructed. |
| `CANDIDATE_IDENTITY_PENDING` | Candidate identity remains under evaluation. |
| `CANDIDATE_AUTHENTICATION_PENDING` | Authentication remains pending. |
| `CANDIDATE_AUTHENTICATED` | Candidate authentication succeeded for the declared scope. |
| `CANDIDATE_ADMISSION_PENDING` | Applicable ACS admission remains pending. |
| `CANDIDATE_ADMITTED` | Required ACS admission exists for the declared scope. |
| `CANDIDATE_COMPATIBILITY_PENDING` | Semantic compatibility remains unresolved. |
| `CANDIDATE_ELIGIBILITY_PENDING` | One or more eligibility conditions remain unresolved. |
| `CANDIDATE_ELIGIBLE` | The candidate satisfies declared eligibility prerequisites. |
| `CANDIDATE_SELECTED` | BOOT selected the candidate for a scoped next step. |
| `CANDIDATE_REJECTED` | The candidate failed an applicable eligibility requirement. |
| `CANDIDATE_DEFERRED` | Evaluation is postponed. |
| `CANDIDATE_UNAVAILABLE` | The candidate is known to be inaccessible for the declared operation. |
| `CANDIDATE_WITHDRAWN` | The candidate or provider withdrew the relevant offer. |
| `CANDIDATE_EXPIRED` | The candidate observation or offer is outside its validity boundary. |
| `CANDIDATE_CONFLICTING` | Material candidate evidence conflicts. |
| `CANDIDATE_UNSUPPORTED` | Required candidate or contract evaluation is unsupported. |
| `CANDIDATE_INDETERMINATE` | Eligibility or identity cannot be established more precisely. |

Authentication and admission values remain provider-attributed.

## 18. Candidate eligibility

### 18.1 Eligibility is scoped

Eligibility MUST identify:

- candidate;
- assistance need;
- operation class;
- target;
- session;
- applicable revisions;
- evidence boundary;
- limitations.

### 18.2 Eligibility inputs

Candidate eligibility MAY require:

- candidate identity;
- authentication;
- trust-domain compatibility;
- secure-session properties;
- endpoint and port compatibility;
- operation compatibility;
- authority evidence;
- ACS admission;
- resource availability;
- disclosure compatibility;
- freshness;
- revocation status;
- mediator eligibility;
- target compatibility;
- local policy.

### 18.3 Eligibility order

Implementations MAY evaluate prerequisites in different orders for efficiency.

Observable results MUST preserve each independent decision.

### 18.4 Missing condition

A missing required condition MUST result in:

- pending;
- unknown;
- unavailable;
- deferred;
- rejected;
- review-required;
- another conservative scoped result.

It MUST NOT produce eligibility.

### 18.5 Eligibility expiration

Candidate eligibility may expire after:

- credential expiry;
- admission expiry;
- connection-generation change;
- authority revision;
- offer expiry;
- trust-domain change;
- policy revision;
- target change;
- operation change.

## 19. Candidate comparison

### 19.1 Comparison inputs

BOOT MAY compare eligible candidates using declared criteria such as:

- operation compatibility;
- authority compatibility;
- supported security profile;
- resource requirements;
- declared availability;
- expected limitations;
- mediation requirements;
- locality where policy permits;
- transfer cost;
- known provider state;
- rollback support;
- disclosure requirements.

### 19.2 Prohibited sole criteria

BOOT MUST NOT select a candidate solely because it:

- responded first;
- responded fastest;
- is closest;
- is local;
- has the shortest route;
- has the most compute;
- has the most storage;
- advertises the newest version;
- has the strongest relationship;
- has the highest private trust score;
- was used previously.

### 19.3 Private selection policy

Private deployment policy MAY define weighting, preference, topology, or operational ranking.

Private policy MUST NOT bypass:

- identity;
- authentication;
- authority;
- admission;
- compatibility;
- revocation;
- hard safety ceilings.

### 19.4 Deterministic public behavior

Where several candidates are otherwise equivalent under public behavior, an implementation SHOULD use a deterministic, non-authorizing tie-break suitable for repeatable tests.

A tie-break does not claim one candidate is more trusted or authoritative.

## 20. Responder selection

### 20.1 Selection record

A responder-selection record SHOULD identify:

- selection identity;
- assistance need;
- selected candidate or candidate set;
- candidate revisions;
- eligibility results;
- selection-policy reference;
- unresolved limitations;
- fallback candidates;
- validity boundary;
- prohibited assumptions.

### 20.2 Selection does not grant authority

Selection MUST NOT change `BOOT_DIM_AUTHORITY` to a favorable value.

### 20.3 Selection does not establish exclusivity

BOOT MAY retain bounded fallback candidates.

### 20.4 Selection invalidation

Selection MUST be reevaluated after material:

- identity change;
- offer revision;
- admission change;
- authority change;
- revocation;
- compatibility change;
- target change;
- policy change;
- candidate unavailability.

## 21. Transport and discovery-adapter boundary

### 21.1 Adapter responsibilities

A BOOT-facing transport/discovery adapter MAY provide:

- candidate locators;
- connection attempts;
- bounded send;
- bounded receive;
- timeout;
- closure;
- unavailable state;
- protocol mismatch;
- transport metadata references.

### 21.2 Adapter non-authority

An adapter MUST NOT report:

- authenticated;
- authorized;
- admitted;
- semantically accepted;
- assistance complete

unless it is explicitly acting through the owning provider contract for that result.

### 21.3 Transport attempt identity

Every transport attempt SHOULD have an identity distinct from:

- discovery request;
- candidate;
- BOOT session;
- assistance session;
- semantic operation.

### 21.4 Connection replacement

A failed transport connection does not automatically invalidate:

- candidate identity;
- candidate eligibility;
- an ACS relationship;
- assistance-session identity;
- completed assistance evidence.

### 21.5 Protocol mismatch

A transport or protocol mismatch means the attempted binding or profile is incompatible.

It does not prove the candidate is invalid or malicious.

### 21.6 Transport timeout

A transport timeout means required transport evidence did not arrive within the declared boundary.

It does not prove:

- no remote response occurred;
- the candidate denied the request;
- semantic work did not complete;
- no mutation occurred.

## 22. Endpoint, port, and facade compatibility

### 22.1 Logical endpoint identity

BOOT MUST distinguish ACS endpoint identity from:

- candidate locator;
- network address;
- transport connection;
- process;
- host.

### 22.2 Assistance port contract

An assistance port or facade contract SHOULD identify:

- supported operation classes;
- direction;
- schema or compatibility versions;
- authentication requirements;
- admission requirements;
- authority requirements;
- resource bounds;
- disclosure policy;
- mediation requirements;
- failure behavior.

### 22.3 Visibility does not grant use

Learning that an assistance port exists does not grant attachment or operation authority.

### 22.4 Public facade

A public assistance facade MAY expose minimal compatibility and identity-negotiation information.

Public visibility does not mean unrestricted access.

### 22.5 Typed negotiation

An assistance port MUST NOT accept arbitrary untyped traffic merely because a source is authenticated.

### 22.6 Contract revision

Material changes to an assistance contract require:

- a new contract version;
- renewed compatibility evaluation;
- a new session generation;
- renewed admission;
- or another explicit transition.

## 23. Authentication and secure-session establishment

### 23.1 Provider attribution

Authentication results MUST identify the security provider, scope, assurance profile, freshness, and revision.

### 23.2 Candidate authentication

Candidate authentication does not authenticate every:

- endpoint;
- replica;
- mediator;
- referral;
- artifact;
- operator;
- authority source

associated with that candidate.

### 23.3 Mutual authentication

Sensitive assistance operations SHOULD authenticate all relevant participants.

### 23.4 Secure-session establishment

A secure session MAY be established before or after candidate normalization and preliminary compatibility exchange, subject to disclosure policy.

### 23.5 Secure session does not grant operation use

After secure-session establishment, BOOT must still evaluate:

- candidate eligibility;
- authority;
- ACS admission;
- port compatibility;
- operation acceptance.

### 23.6 Reauthentication

Reauthentication MAY be required after:

- reconnect;
- secure-session replacement;
- session resumption;
- candidate migration;
- privilege increase;
- trust-domain crossing;
- credential rotation;
- policy revision;
- suspicious evidence;
- long suspension.

## 24. ACS admission

### 24.1 Admission remains separate

ACS admission determines whether current communication or operation use may consume applicable capacity.

### 24.2 Admission request

An admission request SHOULD identify:

- requester;
- candidate;
- endpoint or facade;
- operation class;
- expected resource use;
- direction;
- validity;
- security result references;
- authority references where applicable.

### 24.3 Admission result

Admission may be:

- granted;
- granted with restrictions;
- pending;
- deferred;
- rejected;
- expired;
- revoked;
- unavailable;
- resource exhausted;
- indeterminate.

### 24.4 Admission restrictions

Admission MAY constrain:

- rate;
- concurrency;
- message size;
- disclosure;
- operation classes;
- session duration;
- transport profile;
- mediation;
- recipient scope.

### 24.5 Admission is not work completion

A granted assistance-session admission permits a bounded attempt.

It does not guarantee negotiation or provider work success.

## 25. Assistance-session establishment

### 25.1 Establishment prerequisites

An assistance session may become active only after all prerequisites required by its declared profile are satisfied.

These MAY include:

- candidate identity;
- authentication;
- secure-session properties;
- ACS endpoint and port compatibility;
- ACS admission;
- operation authority;
- resource reservation;
- disclosure agreement;
- session contract compatibility.

### 25.2 Explicit skipped prerequisites

A profile MAY declare a prerequisite not applicable.

A skipped prerequisite MUST be explicit.

It MUST NOT be reported as successfully completed.

### 25.3 Session descriptor

A session descriptor SHOULD identify:

- assistance-session identity;
- session generation;
- BOOT session;
- local participant or identity condition;
- remote candidate;
- endpoint and port references;
- secure-session reference;
- ACS relationship, connection, or attachment references;
- admission reference;
- authority references;
- permitted operation classes;
- resource bounds;
- disclosure tier;
- validity;
- continuation behavior;
- closure behavior.

### 25.4 Session identity separation

Assistance-session identity remains distinct from:

- BOOT session identity;
- transport identity;
- secure-session identity;
- ACS connection identity;
- candidate identity;
- operation identity.

### 25.5 Several connections

One assistance session MAY use several transport or ACS connections when the session contract permits it.

### 25.6 Several sessions

One candidate MAY participate in several assistance sessions with distinct:

- scopes;
- operation classes;
- security profiles;
- admission grants;
- authority;
- resources.

## 26. Assistance-session lifecycle

BOOT-0004 refines the BOOT-0002 `BOOT_DIM_ASSISTANCE` phases.

### 26.1 `ASSISTANCE_NOT_REQUIRED`

No assistance operation is required for the current path.

### 26.2 `ASSISTANCE_INACTIVE`

Assistance may be required, but no search or session activity is active.

### 26.3 `ASSISTANCE_SEEKING`

One or more bounded discovery requests are active.

### 26.4 `CANDIDATE_AVAILABLE`

At least one candidate observation exists.

This does not mean an eligible candidate exists.

### 26.5 `ELIGIBILITY_EVALUATING`

BOOT is evaluating identity, authentication, compatibility, authority, admission, freshness, or resource prerequisites.

### 26.6 `AUTHENTICATION_PENDING`

Authentication through the owning security provider is pending.

### 26.7 `ADMISSION_PENDING`

Applicable ACS admission is pending.

### 26.8 `NEGOTIATION_PENDING`

Session establishment or operation negotiation is pending.

### 26.9 `ASSISTANCE_ACTIVE`

An eligible bounded assistance session is active.

### 26.10 `ASSISTANCE_WAITING_EXTERNAL`

The session is waiting for an external provider, operator, authority, resource, result, or event.

### 26.11 `ASSISTANCE_COMPLETE`

The session produced its declared final scoped session result.

Individual downstream operations may still remain incomplete.

### 26.12 `ASSISTANCE_CLOSING`

Session closure, final result construction, and bounded retention are underway.

### 26.13 `ASSISTANCE_CLOSED`

The assistance session is closed.

### 26.14 No giant lifecycle state

Operation-specific state MUST remain separately represented.

For example:

```text
ASSISTANCE_ACTIVE
    with evidence request pending

ASSISTANCE_ACTIVE
    with plan proposal received

ASSISTANCE_ACTIVE
    with artifact referral rejected

ASSISTANCE_WAITING_EXTERNAL
    with operator review pending
```

These do not require separate global assistance phases.

## 27. Assistance-operation classes

BOOT-0004 defines public operation classes without defining their complete payload schemas.

### 27.1 Identity-assistance request

Requests evidence or provider access needed for identity establishment or revalidation.

### 27.2 Authentication-service request

Requests access to an eligible security provider or authentication operation.

### 27.3 Authority-reference request

Requests bounded references to applicable authority providers, grants, or review paths.

### 27.4 Operator-access request

Requests access to a governed operator-intervention path.

### 27.5 Evidence request

Requests bounded evidence concerning:

- hardware;
- prior state;
- compatibility;
- release state;
- recovery state;
- operation results.

### 27.6 Recovery-plan proposal request

Requests one or more bounded recovery-plan proposals.

A proposal remains subject to BOOT-0005 validation and separate authority.

### 27.7 Artifact-reference request

Requests references to candidate recovery artifacts.

A reference is not verification or transfer authority.

### 27.8 Artifact-transfer coordination request

Requests an eligible transfer path or provider operation.

Transfer remains transport- and artifact-provider-owned.

### 27.9 Installer-referral request

Requests information or access concerning an eligible installer.

Referral does not establish installer availability or authority.

### 27.10 Recovery-status request

Requests provider-attributed status concerning prior recovery or installer work.

### 27.11 Verification-service request

Requests access to an eligible verifier or verification operation.

### 27.12 Release or update referral

Requests candidate release, repository, or peer-transfer information.

The response does not create release authority or activation eligibility.

## 28. Negotiation model

### 28.1 Negotiation purpose

Negotiation determines whether a scoped assistance operation can be represented and considered under mutually compatible contracts.

### 28.2 Negotiation elements

Negotiation MAY address:

- operation class;
- schema version;
- required inputs;
- result contract;
- identity requirements;
- authentication profile;
- authority requirements;
- admission requirements;
- resource bounds;
- disclosure;
- validity;
- expected time bounds;
- mediation;
- cancellation;
- continuation;
- failure behavior.

### 28.3 Request

A request asks a candidate to consider one bounded operation.

### 28.4 Offer

An offer proposes a bounded compatible operation and its requirements.

### 28.5 Counteroffer

A counteroffer narrows or modifies the proposed operation.

A counteroffer MUST NOT silently broaden requester or responder authority.

### 28.6 Clarification

A clarification resolves missing or ambiguous contract information.

It does not grant authority.

### 28.7 Acceptance

Acceptance establishes agreement on the negotiated operation contract.

It does not establish:

- operation authority;
- work admission;
- work start;
- operation completion.

### 28.8 Refusal

A refusal means the candidate will not accept the negotiated request under current conditions.

Refusal does not prove candidate invalidity or compromise.

### 28.9 Deferred negotiation

Negotiation may be deferred pending:

- authority;
- operator review;
- resource availability;
- authentication;
- plan information;
- provider availability;
- refreshed evidence.

### 28.10 Bounded rounds

Negotiation rounds MUST remain finite.

When the round limit is reached, BOOT MUST produce an explicit result such as:

- deferred;
- incompatible;
- resource exhausted;
- failed;
- review-required.

## 29. Assistance-operation results

A public operation result SHOULD distinguish:

| Result | Meaning |
|---|---|
| `OPERATION_NOT_ACCEPTED` | The source did not accept the request for consideration. |
| `OPERATION_ACCEPTED` | The source accepted the request for consideration. |
| `OPERATION_ADMISSION_PENDING` | Resource or provider admission remains unresolved. |
| `OPERATION_ADMITTED` | Work was admitted under the declared scope. |
| `OPERATION_STARTED` | Provider work began. |
| `OPERATION_WAITING_EXTERNAL` | Provider work awaits an external condition. |
| `OPERATION_SATISFIED` | Declared completion conditions were met. |
| `OPERATION_SATISFIED_WITH_LIMITATIONS` | Minimum conditions were met with explicit limitations. |
| `OPERATION_NO_CHANGE` | Work completed without authoritative or durable change. |
| `OPERATION_REJECTED` | The provider rejected the request or input contract. |
| `OPERATION_DENIED` | An authority denied the operation. |
| `OPERATION_DEFERRED` | A final decision or execution was postponed. |
| `OPERATION_PARTIAL` | Some declared effects occurred without full completion. |
| `OPERATION_INTERRUPTED` | Work stopped before completion was established. |
| `OPERATION_FAILED` | Declared completion conditions were not met. |
| `OPERATION_TIMED_OUT` | Required evidence did not arrive within the timeout boundary. |
| `OPERATION_RESOURCE_EXHAUSTED` | A bounded resource prevented completion. |
| `OPERATION_UNSUPPORTED` | The operation or required profile is unsupported. |
| `OPERATION_INDETERMINATE` | Completion or effects cannot be established. |

Provider-owned results remain provider-attributed.

## 30. Multi-source assistance

### 30.1 Several eligible sources

BOOT MAY use several assistance sources when the operation permits it.

### 30.2 Independent evidence

Evidence from several sources counts as independent only when provenance and correlation support independence.

Repeated forwarding of one observation is not independent corroboration.

### 30.3 Split operations

BOOT MAY obtain:

- identity evidence from one source;
- authority from another;
- a plan proposal from another;
- an artifact from another;
- installer access from another.

No source gains authority over the other operations merely through participation.

### 30.4 Conflicting sources

Conflicting results MUST remain explicit.

BOOT MUST NOT resolve conflict solely by:

- majority count;
- response order;
- aggregate bandwidth;
- relationship strength;
- combined compute capacity.

### 30.5 Concurrence

Where authority concurrence is required, BOOT-0003 concurrence rules apply.

Several assistance offers do not automatically constitute concurrence.

### 30.6 Failover

A fallback source MAY be used after failure or withdrawal only after applicable eligibility, freshness, authority, and admission are reevaluated.

## 31. Referrals and mediation

### 31.1 Referral contents

A referral SHOULD identify:

- referring source;
- referred candidate locator or identity claim;
- referral purpose;
- applicable scope;
- validity;
- provenance;
- limitations.

### 31.2 Referral independence

The referred candidate requires independent identity, compatibility, authority, and admission evaluation unless a separately governed trust-domain contract provides an explicit equivalent result.

### 31.3 Referral depth

Referral chains MUST remain bounded.

### 31.4 Semantic mediation

A mediator MAY:

- redact;
- filter;
- translate;
- aggregate;
- enforce policy;
- expose a facade;
- apply backpressure;
- conceal internal topology.

### 31.5 Mediation provenance

Material mediation MUST preserve:

- original source;
- mediator;
- transformation;
- resulting source;
- authority changes;
- freshness changes;
- disclosure changes.

### 31.6 No amplification

A mediator MUST NOT:

- create identity;
- create authority;
- enlarge capability;
- create independent evidence by repeating a claim;
- conceal a material restriction.

## 32. Pre-authentication disclosure and privacy

### 32.1 Disclosure minimization

Before authentication, BOOT and candidates SHOULD disclose only information required to:

- identify a compatible public contract;
- initiate authentication;
- communicate a bounded assistance need;
- reject incompatible or unsupported requests.

### 32.2 Protected local conditions

Pre-authentication discovery SHOULD NOT reveal unnecessary:

- hardware inventory;
- storage layout;
- identity records;
- installed versions;
- private module presence;
- private repository references;
- prior failures;
- authority state;
- operator information;
- topology.

### 32.3 Candidate privacy

BOOT MUST also avoid unnecessary collection of candidate:

- topology;
- identity associations;
- internal capabilities;
- provider relationships;
- private operational state.

### 32.4 Progressive challenge

A source MAY require stronger identity, admission, or authority before revealing more detailed compatibility or operation information.

### 32.5 Refusal without disclosure

A source MAY refuse or conceal an operation without revealing the private policy that caused the result.

The public result must remain truthful.

## 33. Replay, duplication, and downgrade resistance

### 33.1 Replay identity

Security-sensitive negotiation messages SHOULD identify:

- session;
- generation;
- operation;
- message;
- revision or sequence;
- freshness boundary.

### 33.2 Duplicate handling

Duplicate requests or responses MUST NOT:

- create a new operation;
- create new authority;
- count as independent evidence;
- reset budgets;
- repeat non-idempotent work.

### 33.3 Stale offers

A stale offer MUST NOT become current merely because it is delivered through a fresh transport or secure-session wrapper.

### 33.4 Downgrade

Reconnect, fallback, referral, or recovery MUST NOT silently select:

- weaker authentication;
- broader disclosure;
- broader operation authority;
- unmediated access;
- lower integrity requirements

than the declared session profile permits.

## 34. Timeout and retry

### 34.1 Timeout meaning

A timeout establishes only that required evidence was not observed within the declared boundary.

It does not prove:

- candidate absence;
- refusal;
- invalidity;
- no remote work;
- no mutation.

### 34.2 Retry identity

A retry MUST preserve or explicitly replace:

- discovery request identity;
- assistance need identity;
- session identity;
- operation identity;
- attempt identity.

### 34.3 Retry does not create evidence

Repeated failure or success responses do not become independent evidence solely through repetition.

### 34.4 Retry budgets

Implementations MUST define finite retry budgets for:

- discovery;
- connection;
- authentication;
- admission;
- negotiation;
- referral;
- session resumption.

### 34.5 Backoff

Implementations MAY use bounded backoff.

Backoff is implementation behavior, not semantic authority or preference.

### 34.6 Retry exhaustion

Retry exhaustion MUST produce an explicit final or deferred result.

It MUST NOT silently restart the budget.

Detailed retry policy belongs to BOOT-0008.

## 35. Interruption, reconnect, and resumption

### 35.1 Interruption

Assistance may be interrupted by:

- process termination;
- transport failure;
- secure-session loss;
- ACS connection suspension;
- candidate migration;
- provider restart;
- power loss;
- BOOT restart;
- reboot;
- resource loss.

### 35.2 Interruption result

Interruption MUST preserve:

- last known session generation;
- operation state;
- provider state;
- unresolved completion;
- possible durable effects;
- continuation requirements.

### 35.3 New connection is not new session

A new transport or ACS connection does not automatically create a new assistance session.

### 35.4 Resumption prerequisites

Session resumption MAY require:

- continuation-record validation;
- identity revalidation;
- reauthentication;
- renewed secure session;
- admission revalidation;
- authority revalidation;
- operation reconciliation;
- session-generation increment.

### 35.5 Unknown prior completion

If an assistance operation may have completed before interruption, BOOT MUST reconcile it before unsafe repetition.

### 35.6 Resumption refusal

Either participant MAY refuse resumption according to its contract.

Refusal does not erase prior operation evidence.

## 36. Cancellation, withdrawal, and closure

### 36.1 Cancellation request

A cancellation request asks the relevant provider or session to stop future work.

It does not prove work stopped.

### 36.2 Cancellation result

Cancellation states SHOULD distinguish:

- requested;
- accepted;
- active;
- complete;
- rejected;
- unavailable;
- indeterminate.

### 36.3 Candidate withdrawal

A candidate may withdraw an offer or availability.

Withdrawal does not retroactively invalidate completed scoped results.

### 36.4 Session closure reasons

Closure reasons MAY include:

- completed;
- requester canceled;
- responder withdrew;
- authority revoked;
- admission revoked;
- expired;
- incompatible;
- transport unavailable;
- security failure;
- resource exhausted;
- timed out;
- interrupted;
- failed;
- indeterminate.

### 36.5 Closure record

A closure record MUST identify:

- session;
- generation;
- final session outcome;
- unresolved operations;
- retained evidence;
- continuation or reconciliation requirement;
- resource-release status.

### 36.6 Closure does not rewrite dimensions

Closing the assistance session MUST NOT fabricate or erase authority, artifact, installer, recovery, or runtime state.

## 37. Failure and uncertainty model

### 37.1 No candidate discovered

No candidate discovered within the declared scope does not prove universal absence.

### 37.2 Discovery adapter unavailable

Adapter unavailability does not prove candidate unavailability.

### 37.3 Candidate unavailable

Candidate unavailability does not prove invalid identity or revoked authority.

### 37.4 Authentication rejected

Authentication rejection does not prove malicious intent.

### 37.5 Admission denied

Admission denial does not revoke identity or operation authority unless the owning contract explicitly does so.

### 37.6 No compatible contract

No compatible assistance contract means the requested operation cannot be represented under the evaluated candidates and versions.

It does not prove no recovery path exists.

### 37.7 Negotiation failure

Negotiation failure does not invalidate prior successful authentication or admission results.

### 37.8 Session loss

Session loss does not establish operation failure or completion.

### 37.9 Conflicting offers

Conflicting offers remain separately attributable.

### 37.10 Partial assistance

A partial assistance result MUST identify:

- completed scope;
- incomplete scope;
- evidence produced;
- unresolved effects;
- prohibited next actions.

### 37.11 Indeterminate operation

Indeterminate operation state requires conservative reconciliation before unsafe repetition or downstream progression.

## 38. Boundedness and resource exhaustion

BOOT discovery and assistance MUST remain finite under hostile, damaged, or resource-limited conditions.

Implementations MUST define bounds for:

- simultaneous assistance needs;
- discovery requests;
- discovery adapters;
- candidate observations;
- normalized candidates;
- candidate locators;
- advertisements;
- offers per candidate;
- offer size;
- referrals;
- referral depth;
- mediation depth;
- authentication attempts;
- secure sessions;
- ACS admission attempts;
- concurrent assistance sessions;
- operations per session;
- negotiation rounds;
- message size;
- issue records;
- retained session history;
- retry count;
- time budget;
- diagnostic growth.

### 38.1 Candidate flooding

BOOT MUST resist attempts to exhaust resources through excessive candidate advertisements or identity variants.

### 38.2 Offer flooding

Offers exceeding count or size limits MUST produce explicit bounded results.

### 38.3 Expensive evaluation

BOOT and providers MAY apply staged validation so expensive authentication or compatibility work is performed only after cheaper bounded checks.

### 38.4 Protected capacity

Implementations SHOULD preserve bounded capacity for:

- cancellation;
- revocation;
- session closure;
- authority withdrawal;
- critical recovery;
- security-state reporting.

### 38.5 Resource-exhausted result

Resource exhaustion MUST:

- remain explicit;
- avoid favorable defaults;
- leave authoritative state unchanged when required result preparation failed;
- preserve prior revisions;
- avoid silent truncation of required issues.

## 39. Concurrency and atomicity

### 39.1 Concurrent discovery

Several discovery adapters MAY operate concurrently.

Each observation remains independently attributed.

### 39.2 Candidate-record concurrency

Concurrent updates to one normalized candidate require revision or equivalent serialization.

### 39.3 Session establishment

Before committing an active assistance session, BOOT MUST complete all applicable:

- session-descriptor construction;
- candidate-revision validation;
- identity-result validation;
- admission validation;
- authority-reference validation;
- resource validation;
- bounded issue retention;
- result preparation.

### 39.4 No partial session commit

If required preparation fails:

- no active session is committed;
- the previous authoritative state remains unchanged;
- no favorable result is reported.

### 39.5 Multi-provider establishment

Where session establishment requires several provider results, all required references MUST form one coherent revision set.

### 39.6 Negotiation transition

A negotiation transition MUST identify its expected prior revision.

A stale request MUST NOT overwrite newer negotiated state.

### 39.7 Summary staleness

Candidate comparison or selection based on stale candidate revisions MUST be rejected or recomputed.

## 40. Offline and removable-media assistance

### 40.1 Removable media as candidate carrier

Removable media may carry:

- discovery metadata;
- candidate references;
- identity evidence;
- public contracts;
- artifact references;
- authority references;
- installer references.

Media possession does not validate any of those contents.

### 40.2 Read-only assistance

A read-only medium MAY support:

- bounded discovery;
- public compatibility information;
- signed evidence;
- referral to local artifacts.

### 40.3 Offline authority

Offline operation requires an explicitly permitted authority and revocation-uncertainty profile.

### 40.4 Media replacement

Replacing or reinserting media MUST NOT:

- create a new identity automatically;
- reset consumed enrollment state;
- renew expired offers;
- reset retry budgets;
- revive revoked authority.

### 40.5 Media conflict

Conflicting media claims MUST remain explicit.

BOOT MUST NOT choose material solely because it is physically present or booted successfully.

## 41. Source-bearing distribution and update implications

### 41.1 Local source discovery

BOOT may discover locally retained source, packages, manifests, and artifacts.

Local discovery does not authorize:

- compilation;
- installation;
- activation;
- execution;
- propagation.

### 41.2 Release discovery

Discovery of a release or repository means only that candidate release information exists.

It does not establish:

- release authority;
- signature validity;
- compatibility;
- activation permission.

### 41.3 Peer-assisted distribution

A peer may advertise or offer transfer of candidate material.

Peer authentication does not make the peer the release authority.

### 41.4 Private sources

A private source MAY disclose only bounded public information before recipient identity and access authority are established.

### 41.5 Restricted capability refusal

A public node without authorization for private modules must remain functional and may receive a truthful unavailable, concealed, unsupported, or denied result.

### 41.6 Scheduled update checks

Future update architecture may trigger discovery periodically.

Scheduled discovery does not create acceptance, fetch, staging, or activation authority.

## 42. Public conceptual records

These records define semantics, not required wire or memory layouts.

### 42.1 Assistance need

```text
BootAssistanceNeed
    need_identity
    need_revision
    boot_session_identity
    requesting_subject
    target
    operation_classes
    required_evidence
    compatibility_requirements
    authority_requirements
    disclosure_ceiling
    resource_limits
    validity_boundary
    prohibited_actions
    result_contract
```

### 42.2 Discovery request

```text
BootDiscoveryRequest
    request_identity
    request_revision
    assistance_need_reference
    discovery_scope
    mechanism_classes
    trust_domain_constraints
    candidate_class_constraints
    result_count_limit
    response_size_limit
    time_budget
    retry_budget
    referral_depth_limit
```

### 42.3 Discovery observation

```text
BootDiscoveryObservation
    observation_identity
    request_identity
    adapter_identity
    candidate_locator
    claimed_identity_reference
    claimed_endpoint_reference
    advertised_operation_classes
    compatibility_hints
    disclosure_tier
    availability_state
    freshness_state
    observation_revision
    bounded_issues
```

### 42.4 Assistance offer

```text
BootAssistanceOffer
    offer_identity
    offer_revision
    candidate_reference
    need_reference
    offered_operation
    supported_parameters
    authentication_requirements
    authority_requirements
    admission_requirements
    resource_limits
    disclosure_conditions
    validity_boundary
    limitations
    next_step
```

### 42.5 Normalized candidate

```text
BootAssistanceCandidate
    candidate_identity
    candidate_revision
    observation_references
    identity_claim_reference
    endpoint_references
    locator_references
    operation_classes
    compatibility_state
    authentication_state
    admission_state
    authority_state
    eligibility_state
    availability_state
    freshness_state
    offer_references
    bounded_issues
```

### 42.6 Candidate evaluation

```text
BootCandidateEvaluation
    evaluation_identity
    candidate_reference
    need_reference
    evaluated_revisions
    identity_result_reference
    authentication_result_reference
    admission_result_reference
    authority_result_reference
    compatibility_result
    resource_result
    eligibility_result
    limitations
    bounded_issues
```

### 42.7 Responder selection

```text
BootResponderSelection
    selection_identity
    need_reference
    selected_candidate_references
    candidate_revision_vector
    eligibility_references
    selection_policy_reference
    fallback_candidate_references
    limitations
    validity_boundary
```

### 42.8 Assistance-session descriptor

```text
BootAssistanceSessionDescriptor
    assistance_session_identity
    session_generation
    boot_session_identity
    local_identity_reference
    candidate_identity_reference
    endpoint_and_port_references
    transport_attempt_references
    secure_session_reference
    acs_relationship_reference
    acs_connection_reference
    attachment_reference
    admission_reference
    authority_references
    permitted_operation_classes
    disclosure_tier
    resource_limits
    validity_boundary
    continuation_policy
    closure_policy
```

### 42.9 Negotiation message

```text
BootAssistanceNegotiationMessage
    message_identity
    session_identity
    session_generation
    operation_identity
    message_revision
    message_kind
    proposed_contract_reference
    authority_references
    bounded_parameters
    freshness_reference
    result_expectation
```

### 42.10 Assistance-operation result

```text
BootAssistanceOperationResult
    operation_identity
    attempt_identity
    session_identity
    provider_identity
    operation_class
    progress_state
    outcome_state
    result_revision
    evidence_references
    authority_references
    limitations
    continuation_requirement
    bounded_issues
```

### 42.11 Session-closure record

```text
BootAssistanceSessionClosure
    session_identity
    final_generation
    closure_reason
    final_session_outcome
    operation_result_references
    unresolved_operations
    retained_evidence_references
    continuation_requirement
    reconciliation_requirement
    resource_release_state
    bounded_issues
```

## 43. BOOT-0002 state integration

### 43.1 Assistance dimension

BOOT-0004 directly refines `BOOT_DIM_ASSISTANCE`.

### 43.2 Other dimensions remain separate

Discovery and assistance MUST NOT directly overwrite:

- `BOOT_DIM_AUTHORITY`;
- `BOOT_DIM_ARTIFACT`;
- `BOOT_DIM_INSTALLER`;
- `BOOT_DIM_RECOVERY`;
- `BOOT_DIM_HANDOFF`;
- runtime-observation state.

### 43.3 Knowledge facet

The knowledge facet represents:

- unknown candidates;
- partial discovery;
- established observations;
- conflicting observations.

### 43.4 Availability facet

Availability identifies whether adapters, candidates, providers, sessions, or operations are accessible for their declared scope.

### 43.5 Validity facet

Validity identifies structural, semantic, identity, compatibility, or provider evaluation results.

### 43.6 Freshness facet

Freshness identifies current, stale, expired, clock-unavailable, or conflicting observations and offers.

### 43.7 Progress facet

Progress identifies pending, active, waiting, deferred, interrupted, complete, failed, or indeterminate discovery and negotiation work.

### 43.8 Enforcement facet

Enforcement may restrict or lock down assistance disclosure or operation use.

Lock-down does not mean every candidate is compromised.

### 43.9 Resource facet

Resource state identifies whether discovery and assistance work is sufficient, constrained, exhausted, or unknown.

### 43.10 Outcome facet

Operation outcomes use the BOOT-0002 outcome meanings and remain operation-scoped.

## 44. Adjacent-system integration

### 44.1 ACS endpoints and ports

ACS owns logical assistance endpoints, ports, visibility, bindings, attachments, and compatibility contracts.

BOOT consumes those contracts.

### 44.2 ACS lifecycle

ACS owns connection and attachment establishment, suspension, replacement, migration, and retirement.

An ACS lifecycle transition does not automatically close the BOOT assistance session.

### 44.3 ACS admission and budgets

ACS admission owns current communication and resource permission.

BOOT respects admission restrictions and resource budgets.

### 44.4 ACS security and trust

ACS and security providers own participant identity, authentication, trust evidence, capabilities, secure sessions, and revocation semantics.

BOOT does not invent substitutes.

### 44.5 Transport

Transport owns concrete communication behavior.

Transport metadata may support candidate observations but is not semantic identity or authority.

### 44.6 Installer

Installer referral, availability, acceptance, and completion remain separately represented.

BOOT-0005 defines the later installer handoff contract.

### 44.7 Artifact providers

Artifact referral and transfer access remain separate from artifact verification and compatibility.

### 44.8 IMM

IMM may recommend assistance or supply evidence.

It cannot:

- select itself as responder authority;
- bypass candidate eligibility;
- authorize its own recovery request;
- expose private scores through public offers.

### 44.9 MEM

MEM may retain bounded assistance records under governed persistence.

Persistence does not create truth, identity, authority, or completion.

### 44.10 Runtime

The normal runtime may later implement discovery or registry services.

Runtime hosting does not enlarge their semantic authority.

## 45. Public/private classification boundary

### 45.1 Public material

Public BOOT architecture MAY define:

- assistance-need contracts;
- discovery scopes;
- discovery observations;
- candidate-state meanings;
- disclosure tiers;
- eligibility inputs;
- session lifecycle;
- negotiation semantics;
- operation classes;
- boundedness;
- failure behavior;
- conceptual records;
- conformance tests.

### 45.2 Private material

Public BOOT documentation MUST NOT disclose:

- production assistance addresses;
- endpoint catalogs;
- private port names;
- private responder-ranking formulas;
- topology preferences;
- private trust thresholds;
- hidden scores;
- production failover order;
- operator contact paths;
- private source locations;
- production authority assignments;
- recovery secrets;
- infrastructure credentials;
- private mediation policy;
- incident-response playbooks.

### 45.3 Independent public implementation

A public implementation must remain capable of:

- discovering mock or public candidates;
- normalizing observations;
- performing compatibility evaluation;
- using provider-attributed authentication;
- requesting ACS admission;
- establishing a bounded assistance session;
- negotiating public operation contracts;
- handling failures truthfully

without private modules.

## 46. Security and privacy requirements

Discovery and assistance implementations MUST:

- minimize unauthenticated disclosure;
- avoid raw credentials;
- avoid private-key material;
- avoid reusable recovery secrets;
- preserve provider attribution;
- preserve identity and authority separation;
- bound candidate enumeration;
- prevent replay from creating new work;
- prevent unauthorized private-material propagation;
- protect sensitive candidate and local metadata;
- represent mediation and transformation;
- enforce classification.

A source MAY conceal a capability.

It MUST NOT falsely claim successful support for an unavailable capability.

## 47. Conformance expectations

### 47.1 Architectural conformance

An architecture conforms when it:

- preserves all foundational distinctions;
- remains transport-neutral;
- supports progressive disclosure;
- treats discovery as observation;
- authenticates candidates through eligible providers;
- separates authority and admission;
- preserves candidate revisions;
- keeps sessions bounded;
- prevents responsiveness from creating preference or authority;
- preserves multi-source provenance;
- represents interruption and uncertainty;
- prevents session closure from fabricating completion.

### 47.2 Implementation conformance

An implementation conforms when observable behavior preserves those properties under:

- no candidates;
- one candidate;
- several candidates;
- duplicate advertisements;
- conflicting identity claims;
- stale offers;
- first-responder races;
- local and remote candidates;
- transport failure;
- authentication rejection;
- admission denial;
- authority unavailability;
- incompatible contracts;
- negotiation exhaustion;
- candidate withdrawal;
- reconnect;
- provider restart;
- session interruption;
- referral loops;
- resource exhaustion;
- private-capability refusal.

### 47.3 Required negative tests

Conformance evidence SHOULD include:

- first responder is not automatically selected;
- lowest latency is not automatically authoritative;
- address equality does not merge candidates;
- duplicate advertisements do not create independent evidence;
- discovery does not create an ACS relationship;
- public visibility does not grant use;
- reachability does not authenticate;
- secure-session success does not admit;
- authentication does not authorize;
- authority does not bypass admission;
- selection does not create exclusivity;
- offer acceptance does not prove work start;
- session closure does not prove operation completion;
- transport timeout does not prove no remote mutation;
- referral does not authenticate the referred source;
- mediation does not amplify authority;
- stale offer does not revive through retransmission;
- new connection does not automatically create a new session;
- retry does not reset budgets;
- private source refuses unauthorized material transfer;
- candidate flooding produces bounded exhaustion behavior.

### 47.4 Mock-provider conformance

Mock discovery and assistance providers MUST support truthful:

- candidate found;
- no candidate within scope;
- duplicate;
- stale;
- conflicting;
- unavailable;
- authentication pending;
- admission pending;
- eligible;
- rejected;
- withdrawn;
- unsupported;
- resource exhausted;
- timed out;
- interrupted;
- indeterminate

results.

A mock MUST NOT return a favorable result that the real provider contract could not justify.

## 48. BOOT-I004 and BOOT-I005 implications

### 48.1 BOOT-I004 — transport/discovery adapter

BOOT-I004 should expose a language-neutral adapter supporting:

- bounded candidate observations;
- candidate locators;
- connect attempts;
- bounded send and receive;
- timeout;
- closure;
- unavailable;
- protocol mismatch;
- explicit provider identity;
- no semantic authority inference.

### 48.2 BOOT-I005 — assistance-session state machine

BOOT-I005 should implement:

- assistance need creation;
- discovery;
- candidate normalization;
- candidate evaluation;
- authentication pending;
- admission pending;
- negotiation pending;
- active session;
- external wait;
- completion;
- deferred;
- failed;
- locked-down;
- interruption and resumption;
- bounded closure.

The implementation MUST use the independent BOOT-0002 dimensions rather than one global state enumeration.

### 48.3 C-compatible boundary

Concrete ABI work remains with BOOT-I006 and BOOT-0006.

The architecture should remain compatible with:

- caller-owned buffers;
- explicit counts;
- fixed-width categories;
- bounded arrays;
- structured results;
- no allocator ownership transfer;
- no language-specific exceptions across public boundaries.

## 49. BOOT-P2 implementation proof

BOOT-P2 should demonstrate:

1. create or recover a provisional BOOT identity condition;
2. produce a bounded assistance need;
3. run one or more mock discovery adapters;
4. discover at least one mock candidate;
5. preserve discovery separately from identity;
6. reject an address-only candidate identity;
7. authenticate through a test security provider;
8. preserve authentication separately from authority;
9. request ACS admission separately;
10. minimize pre-authentication disclosure;
11. establish one bounded assistance session;
12. negotiate one typed assistance operation;
13. demonstrate duplicate handling;
14. demonstrate timeout or candidate withdrawal;
15. close the session with a structured result.

BOOT-P2 need not implement:

- production networking;
- production credentials;
- production responder policy;
- artifact transfer;
- installation;
- full recovery.

## 50. Relationship to later BOOT specifications

### BOOT-0005 — Recovery Plans and Artifact Boundaries

BOOT-0005 will define:

- plan-proposal validation;
- artifact references;
- transfer-result interpretation;
- compatibility;
- installer handoff;
- recovery-result contracts.

BOOT-0004 negotiation does not authorize those operations.

### BOOT-0006 — Minimal Runtime and Language Constraints

BOOT-0006 will define measurable limits for:

- candidates;
- offers;
- sessions;
- negotiation rounds;
- message sizes;
- adapters;
- timeouts;
- retries;
- memory use;
- stack and heap behavior.

### BOOT-0007 — ACS, IMM, and MEM Integration

BOOT-0007 will refine:

- ACS endpoint and port mapping;
- relationship and connection use;
- admission adapters;
- IMM evidence and requests;
- MEM persistence of session state.

### BOOT-0008 — Failure, Retry, and Operator Intervention

BOOT-0008 will refine:

- retry classes;
- backoff;
- lockout;
- operator review;
- interruption;
- resumption;
- cancellation;
- reconciliation.

### BOOT-0009 — Public/Private Boundary and Implementation Roadmap

BOOT-0009 will define publication, implementation checkpoint, and private-policy boundaries for discovery and assistance.

### Future OS and update architecture

Future OS and update specifications will own:

- release discovery;
- update channels;
- peer-assisted distribution;
- repository contracts;
- local fetch and staging.

They MUST preserve BOOT-0004 discovery and authority boundaries.

## 51. Open architectural decisions

The following remain intentionally unresolved:

- concrete discovery protocols;
- concrete transport adapters;
- service-directory mechanisms;
- candidate-locator formats;
- identifier widths;
- compatibility-version formats;
- disclosure-tier encodings;
- candidate-count limits;
- offer-size limits;
- negotiation-round limits;
- timeout values;
- retry values;
- backoff algorithms;
- responder-ranking policy;
- deterministic tie-break implementation;
- session-continuation representation;
- referral-depth limits;
- mediation-depth limits;
- secure-session profiles;
- ACS relationship classes used for assistance;
- concrete endpoint and port contracts;
- offline authority profiles;
- removable-media metadata formats;
- update-discovery ownership after UPD architecture exists;
- operator-discovery workflows;
- production topology.

These open decisions do not permit weaker identity, authority, boundedness, disclosure, or failure behavior.

## 52. BOOT-0001 traceability registry

| BOOT-0001 invariant | BOOT-0004 coverage |
|---|---|
| BOOT-INV-001 — Coordination without absorption | Sections 8, 21–24, and 44 |
| BOOT-INV-002 — Execution context grants no authority | Sections 6, 10, 19, and 40 |
| BOOT-INV-003 — Adjacent ownership preserved | Sections 8 and 44 |
| BOOT-INV-004 — Stage separation | Sections 6, 17–29, and 43 |
| BOOT-INV-005 — Session identity separation | Sections 7 and 25 |
| BOOT-INV-006 — Identity is not location or response order | Sections 6, 16, 18, and 19 |
| BOOT-INV-007 — Identity evidence requires validation | Sections 16–18 and 23 |
| BOOT-INV-008 — BOOT cannot self-grant authority | Sections 18–25 |
| BOOT-INV-009 — Multidimensional state | Sections 17, 26, and 43 |
| BOOT-INV-010 — Unknown remains explicit | Sections 17, 34, and 37 |
| BOOT-INV-011 — Connection events are not completion | Sections 21, 29, 34–36 |
| BOOT-INV-012 — Bounded data and work | Sections 28, 31, 34, and 38 |
| BOOT-INV-013 — Resource exhaustion is first-class | Section 38 |
| BOOT-INV-014 — Conservative atomicity | Section 39 |
| BOOT-INV-015 — Revision and idempotency continuity | Sections 13–20, 33–36, and 39 |
| BOOT-INV-016 — Responsiveness is not authority | Sections 6, 19, and 51 |
| BOOT-INV-017 — Transport is not semantic acceptance | Sections 6 and 21 |
| BOOT-INV-018 — Bounded assistance sessions | Sections 25–39 |
| BOOT-INV-019 — Plans do not create authority | Sections 27 and 50 |
| BOOT-INV-020 — Transfer and verification remain distinct | Sections 27, 41, and 50 |
| BOOT-INV-027 — Update availability grants no authority | Section 41 |
| BOOT-INV-028 — Governed peer propagation | Sections 30 and 41 |
| BOOT-INV-029 — Public/private separation | Sections 32, 41, 45, and 46 |

## 53. BOOT-0002 integration registry

| BOOT-0002 concept | BOOT-0004 use |
|---|---|
| `BOOT_DIM_ASSISTANCE` | Governs discovery and assistance-session lifecycle |
| `ASSISTANCE_INACTIVE` | No active discovery or session |
| `ASSISTANCE_SEEKING` | Bounded discovery is active |
| `CANDIDATE_AVAILABLE` | At least one observation exists |
| `ELIGIBILITY_EVALUATING` | Candidate prerequisites are under evaluation |
| `AUTHENTICATION_PENDING` | Security-provider result remains pending |
| `ADMISSION_PENDING` | ACS admission remains pending |
| `NEGOTIATION_PENDING` | Session or operation negotiation remains pending |
| `ASSISTANCE_ACTIVE` | Eligible assistance session is active |
| `ASSISTANCE_WAITING_EXTERNAL` | External provider, authority, operator, or resource is awaited |
| `ASSISTANCE_COMPLETE` | Scoped session result exists |
| `ASSISTANCE_CLOSING` | Final result and closure preparation are active |
| `ASSISTANCE_CLOSED` | Session is closed |
| Knowledge facet | Candidate discovery and evidence completeness |
| Availability facet | Adapter, candidate, provider, and session accessibility |
| Validity facet | Contract, identity, and compatibility evaluation |
| Freshness facet | Advertisement, offer, admission, and session currency |
| Progress facet | Discovery and negotiation progress |
| Enforcement facet | Disclosure or operation restriction |
| Resource facet | Bounded discovery and session capacity |
| Outcome facet | Scoped discovery, session, and operation results |

## 54. Assistance distinction registry

| Concept | Establishes | Does not establish |
|---|---|---|
| Assistance need | Required capability or result | Existence of a provider |
| Discovery request | Bounded search instruction | Authority to every observer |
| Discovery observation | Possible candidate existence | Identity or availability |
| Advertisement | Claimed capability | Current readiness |
| Candidate locator | Possible transport target | Logical identity |
| Reachability | Byte-exchange possibility | Authentication |
| Authentication | Scoped identity result | Authority |
| Authority | Permission to attempt scoped work | ACS admission |
| ACS admission | Current communication/resource permission | Semantic compatibility or completion |
| Compatibility | Contract representability | Operation acceptance |
| Offer | Proposed assistance terms | Commitment or authority |
| Candidate eligibility | Prerequisites for consideration | Selection or operation authorization |
| Responder selection | Proposed source for next step | Exclusivity or authority |
| Secure session | Declared communication protection | Assistance-session identity |
| Assistance session | Bounded semantic interaction | Recovery authorization |
| Negotiation acceptance | Agreed operation contract | Work start or completion |
| Operation start | Provider began work | Completion |
| Session closure | Session ended | Every operation completed |
| Referral | Candidate information | Authentication or endorsement |
| Peer transfer offer | Possible byte source | Release authority |

## 55. Prohibited interpretations

This specification MUST NOT be interpreted to mean that:

- a discovery request is unrestricted broadcast authority;
- discovery creates identity;
- an advertisement proves availability;
- availability proves readiness;
- reachability proves authentication;
- authentication proves authority;
- authority proves admission;
- admission proves compatibility;
- compatibility proves operation acceptance;
- offer acceptance proves work start;
- work start proves completion;
- first response creates preference;
- lowest latency creates authority;
- physical proximity creates authority;
- local execution creates authority;
- strongest hardware creates responder eligibility;
- a candidate locator is participant identity;
- equal addresses prove candidate continuity;
- repeated advertisements create independent evidence;
- several referrals create authentication;
- a secure session creates an assistance session;
- an assistance session creates an ACS relationship;
- a new connection creates a new semantic session;
- selection creates authority;
- selection creates exclusivity;
- negotiation authorizes recovery;
- a plan proposal is a validated plan;
- artifact availability proves verification;
- referral to an installer proves installer eligibility;
- session closure proves operation completion;
- transport timeout proves no remote effect;
- candidate withdrawal revokes every prior result;
- mediation amplifies authority;
- majority of assistance sources automatically resolves conflict;
- retry creates a new operation;
- reconnect resets retry budgets;
- a removable medium is authoritative through possession;
- peer authentication creates release authority;
- update discovery creates activation permission;
- private-policy concealment permits false public success reporting.

## 56. Completion checklist

- [x] BOOT-0000 authority is preserved.
- [x] Approved BOOT-0001 invariants are preserved.
- [x] BOOT-0002 assistance-state semantics are preserved.
- [x] Approved BOOT-0003 identity and authority boundaries are preserved.
- [x] Discovery remains separate from identity and authentication.
- [x] Authentication remains separate from authority.
- [x] Authority remains separate from ACS admission.
- [x] Admission remains separate from compatibility and completion.
- [x] Progressive disclosure is defined.
- [x] Candidate observations are provider-attributed.
- [x] Candidate normalization does not rely on address equality.
- [x] Duplicate advertisements do not create independent evidence.
- [x] Candidate eligibility is scoped and revisioned.
- [x] Response order, latency, proximity, and compute capacity create no authority.
- [x] Responder selection creates neither authority nor exclusivity.
- [x] Transport remains outside BOOT semantic ownership.
- [x] Secure sessions remain separate from assistance sessions.
- [x] Assistance sessions remain separate from ACS relationships and connections.
- [x] Negotiation remains separate from plan validation and authorization.
- [x] Multi-source assistance preserves evidence lineage.
- [x] Referral and mediation remain bounded.
- [x] Mediation cannot amplify authority.
- [x] Pre-authentication disclosure is minimized.
- [x] Retry, timeout, interruption, and resumption remain explicit.
- [x] Session closure does not fabricate operation completion.
- [x] Discovery and negotiation are bounded under resource exhaustion.
- [x] Conservative atomicity is preserved.
- [x] Offline and removable-media assistance remain governed.
- [x] Peer discovery does not create release authority.
- [x] Public/private boundaries are preserved.
- [x] No production addresses, rankings, topology, credentials, or policies are exposed.
- [x] BOOT-I004, BOOT-I005, and BOOT-P2 have a governing architecture basis.
- [x] Detailed plan, artifact, and installer contracts remain with BOOT-0005.
- [x] No concrete discovery or transport protocol is mandated.
- [x] No blocking contradiction was identified.

## 57. Closing principle

> **A responder becomes usable only through explicit identity, compatibility, authority, admission, and session results—not because it was visible, reachable, nearby, fast, familiar, or willing.**

## Revision history

### Version 0.1 — 2026-07-18

- Defined assistance needs, discovery scopes, requests, observations, advertisements, offers, candidates, and responder selection.
- Defined progressive disclosure and pre-authentication minimization.
- Defined candidate normalization, eligibility, comparison, and revision behavior.
- Preserved separation among transport, secure sessions, ACS admission, authority, and assistance sessions.
- Refined the BOOT-0002 assistance lifecycle.
- Defined assistance-operation classes and negotiation semantics.
- Defined multi-source, referral, mediation, interruption, resumption, cancellation, and closure behavior.
- Defined boundedness, resource exhaustion, concurrency, and conservative atomicity requirements.
- Defined offline, removable-media, source-bearing, release-discovery, and peer-transfer implications.
- Defined language-neutral conceptual records and BOOT-I004, BOOT-I005, and BOOT-P2 implementation direction.
