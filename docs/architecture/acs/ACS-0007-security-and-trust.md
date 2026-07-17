# ACS-0007: Security and Trust

| Field | Value |
|---|---|
| Specification | ACS-0007 |
| Title | Security and Trust |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ACS-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Pending review |
| Depends on | ACS-0000 through ACS-0006 |
| Related specifications | ACS-0008, MEM-0000 through MEM-0007 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in identity, capability, delegation, and revocation separation; distributed trust reconciliation, recovery credentials, and authority concurrence remain under review |

> **Trust may influence what Node is willing to consider, but only explicit authority may permit an operation.**

## Architectural-intent notice

This specification defines the public architecture for identity, authentication, security state, trust evidence, capabilities, delegation, secure sessions, key custody, revocation, and authority conflict within the Adaptive Connection Substrate.

It is independently authored public architecture.

It is not produced by deleting, sanitizing, paraphrasing, or selectively redacting restricted architecture.

This specification defines public concepts and guarantees required for independently implementable Node-compatible security without disclosing:

- production credentials;
- production trust roots;
- private key material;
- private trust thresholds;
- proprietary trust-scoring algorithms;
- restricted operator procedures;
- production security topology;
- private immune heuristics;
- non-public identity catalogs;
- production recovery secrets;
- production capability catalogs;
- internal cognitive policy.

This specification does not prescribe:

- one cryptographic algorithm;
- one public-key infrastructure;
- one certificate format;
- one credential-storage technology;
- one secure-session protocol;
- one trust-scoring formula;
- one consensus algorithm;
- one identity-provider implementation;
- fixed credential lifetimes;
- fixed trust thresholds;
- production authority assignments;
- production key-rotation schedules;
- private enrollment procedures.

Implementations may use different mechanisms.

They must preserve the distinctions, authority boundaries, revocation behavior, and observable security states established here.

## 1. Purpose

Node must communicate across:

- processes;
- hosts;
- devices;
- regions;
- trust domains;
- external systems;
- changing runtime placements;
- changing connection bindings;
- partial failures;
- partitions;
- recovery events.

In such an environment, the system must determine:

- which logical participant is acting;
- whether the presented identity evidence is valid;
- whether the acting instance represents the intended participant;
- which authority the participant possesses;
- whether that authority applies to the requested operation;
- whether the evidence is current;
- whether a secure communication context exists;
- whether credentials or capabilities have been revoked;
- whether conflicting identity or authority claims exist;
- whether trust evidence supports ordinary interaction;
- whether degraded or recovery behavior is required.

A system that uses one vague value such as `trusted` will eventually confuse:

- identity with credentials;
- authentication with authorization;
- encryption with trust;
- trust with authority;
- authority with admission;
- ownership with key custody;
- operator identity with universal control;
- immune suspicion with security fact;
- previous validity with current validity;
- successful history with permanent permission.

ACS-0007 defines the security and trust model needed to keep those concepts distinct.

## 2. Scope

This specification governs:

- participant identity;
- identity scope;
- identity continuity;
- identity claims;
- identity evidence;
- identity conflict;
- credentials;
- credential status;
- authentication;
- authentication assurance;
- secure sessions;
- channel binding;
- confidentiality;
- integrity;
- authenticity;
- freshness;
- replay protection;
- downgrade resistance;
- trust evidence;
- trust dimensions;
- trust scope;
- trust state;
- trust decay and expiry;
- capabilities;
- authority;
- delegation;
- concurrence;
- revocation;
- authorization caching;
- key custody;
- trust domains;
- external participants;
- operator identity;
- recovery identity;
- security-state transitions;
- security audit;
- privacy;
- interaction with admission, lifecycle, immune, runtime, health, and MEM;
- public conformance requirements.

This specification does not define:

- relationship-formation policy;
- resource budgets;
- complete admission algorithms;
- immune evidence interpretation;
- quarantine policy;
- memory-operation authorization;
- physical-node ownership;
- software-installation policy;
- production propagation authority;
- private operator workflows;
- cognitive trust formation;
- emotional or social trust;
- one mandatory cryptographic suite.

Those subjects belong to other ACS specifications, MEM, future IMM specifications, runtime architecture, operator-control specifications, or implementation profiles.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements.

The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification.

The word **may** describes permitted behavior.

An implementation does not conform merely because it encrypts traffic or authenticates a process.

Its observable behavior must preserve the identity, trust, capability, and authority distinctions established here.

## 4. Foundational distinctions

### 4.1 Identity is not a credential

An identity describes which logical participant is being represented.

A credential provides evidence supporting an identity or authority claim.

Credentials may:

- expire;
- rotate;
- be revoked;
- be replaced;
- be compromised.

The participant identity may remain continuous across those changes.

### 4.2 Identity is not an address

A hostname, IP address, process identifier, device path, or network location does not independently define logical participant identity.

### 4.3 Authentication is not authorization

Authentication establishes a bounded level of confidence that presented identity evidence is valid.

It does not determine whether the participant may perform a requested operation.

### 4.4 Authorization is not admission

Authorization establishes that an identity may attempt an operation within a declared scope.

ACS-0006 admission still determines whether the operation may currently consume communication or resource capacity.

### 4.5 Trust is not authority

Trust evidence may influence:

- admission;
- required mediation;
- grant duration;
- validation requirements;
- permitted degradation.

Trust does not independently create permission.

### 4.6 Trust is not truth

A highly trusted participant may be mistaken.

A correctly authenticated source may submit incorrect information.

### 4.7 Encryption is not trust

Encrypted communication may protect confidentiality and integrity.

It does not prove that:

- the remote participant is authorized;
- the remote participant is healthy;
- the message is truthful;
- the requested operation is safe.

### 4.8 Secure session is not relationship

A secure session provides a protected communication context.

It does not create an ACS relationship.

### 4.9 Secure session is not connection authority

A valid secure session does not activate an ACS connection, attachment, or port grant by itself.

### 4.10 Ownership is not key custody

Owning an endpoint or service does not automatically grant possession of its private keys, root credentials, or protected security records.

### 4.11 Physical possession is not identity authority

Possession of a host, disk, accelerator, credential file, or network interface does not automatically establish authority to represent every logical participant associated with it.

### 4.12 Capability is not identity

A capability permits a bounded operation.

It does not become the holder’s identity.

### 4.13 Capability is not trust

A capability may be granted to a minimally trusted participant for one tightly constrained operation.

### 4.14 Previous validity is not current validity

A credential, session, capability, or trust observation that was valid previously may now be:

- stale;
- expired;
- revoked;
- superseded;
- conflicting;
- unknown.

### 4.15 Security denial is not proof of compromise

An operation may be denied because of:

- missing evidence;
- expired authority;
- policy incompatibility;
- resource restrictions;
- unsupported security profile;
- unknown state.

Denial alone does not prove malicious intent.

### 4.16 Immune suspicion is not security authority

Immune evidence may cause security review or bounded restrictions.

It does not automatically create credentials, revoke identities, or grant unrestricted containment authority.

## 5. Core terminology

### 5.1 Logical participant identity

A logical participant identity identifies one continuing ACS participant independent of its current:

- process;
- host;
- device;
- address;
- connection;
- secure session;
- runtime instance.

### 5.2 Instance identity

An instance identity identifies one runtime realization of a logical participant.

A participant may have:

- one active instance;
- several authorized instances;
- a primary and standby instance;
- no currently active instance.

### 5.3 Identity claim

An identity claim states that an actor, instance, endpoint, or message represents a declared identity.

A claim is not accepted merely because it is syntactically valid.

### 5.4 Identity evidence

Identity evidence supports or contradicts an identity claim.

Evidence may include:

- credential proof;
- cryptographic proof;
- enrollment records;
- continuity evidence;
- challenge response;
- secure hardware evidence;
- operator confirmation;
- prior authenticated state;
- recovery records.

### 5.5 Credential

A credential is a protected object or claim used to support:

- authentication;
- authorization;
- enrollment;
- delegation;
- secure-session establishment.

### 5.6 Authentication

Authentication evaluates whether identity evidence satisfies the required assurance for a declared scope.

### 5.7 Assurance level

An assurance level describes the strength and conditions of an authentication result.

It may consider:

- proof method;
- credential protection;
- freshness;
- challenge completion;
- hardware backing;
- number of independent factors;
- continuity evidence;
- issuer trust;
- revocation status.

### 5.8 Trust evidence

Trust evidence is an observation relevant to willingness to rely upon a participant, credential, service, mediator, or claim within a declared scope.

### 5.9 Trust state

Trust state is a bounded interpretation of available trust evidence.

It is not necessarily one global number.

### 5.10 Capability

A capability is an explicit, bounded authorization permitting a subject to attempt one operation or operation family against a declared target and scope.

### 5.11 Authority

Authority is the recognized right to request, approve, deny, perform, or govern a declared operation.

### 5.12 Delegation

Delegation grants a narrower authority derived from an existing delegable authority.

### 5.13 Revocation

Revocation invalidates or restricts previously valid:

- credentials;
- capabilities;
- sessions;
- delegations;
- trust assertions;
- admission grants.

### 5.14 Secure session

A secure session is a bounded communication-security context providing declared properties between authenticated or otherwise authorized participants.

### 5.15 Trust domain

A trust domain is a declared scope within which particular:

- identities;
- credential issuers;
- trust anchors;
- security policies;
- revocation sources;

are recognized.

### 5.16 Trust anchor

A trust anchor is a protected authority or evidence root used to validate security claims within a declared scope.

### 5.17 Key custodian

A key custodian is a logical responsibility authorized to protect or use specific cryptographic key material.

### 5.18 Security state

Security state describes current evidence concerning whether an identity, credential, session, capability, or trust relationship is:

- valid;
- restricted;
- stale;
- expired;
- revoked;
- conflicting;
- compromised;
- recovering;
- unknown.

## 6. Identity layers

Node security shall distinguish at least the following identity layers where applicable.

### 6.1 Participant identity

Identifies the continuing logical ACS participant.

### 6.2 Participant-instance identity

Identifies one runtime realization of that participant.

### 6.3 Endpoint identity

Identifies one logical communication boundary owned by a participant.

### 6.4 Connection-instance identity

Identifies one ACS connection instance and generation.

### 6.5 Operator identity

Identifies a human or externally governed operator actor.

### 6.6 Service-role identity

Identifies a logical responsibility such as:

- admission evaluator;
- security authority;
- key custodian;
- lifecycle authority;
- memory custodian;
- immune observer.

A service role is not automatically identical to the participant currently assigned to perform it.

### 6.7 Device identity

Identifies a physical or virtual device where device-level identity is required.

A device identity does not automatically become a participant identity.

### 6.8 Credential-issuer identity

Identifies the authority responsible for issuing or attesting to a credential.

### 6.9 Recovery identity

Identifies a restricted actor, record, or authority used during recovery.

Recovery identity must not silently become ordinary production authority.

## 7. Participant identity requirements

### 7.1 Stable logical scope

Participant identity shall remain stable across ordinary:

- restart;
- migration;
- address change;
- secure-session renewal;
- credential rotation;
- host replacement;

when continuity is established.

### 7.2 Explicit namespace

Every identity shall be unambiguous within its declared namespace or trust domain.

### 7.3 Non-reuse

A retired or invalidated participant identity shall not be reassigned to an unrelated participant.

### 7.4 No location-derived continuity

Identity continuity shall not be accepted solely because:

- the same address returned;
- the same host name was used;
- the same process name appeared;
- the same filesystem state was restored.

### 7.5 Conflicting claims

Conflicting identity claims shall remain explicit.

The system shall not select a claimant solely because it:

- responded first;
- occupies the previous address;
- is physically local;
- has more resources;
- presents an older cached credential.

## 8. Identity continuity

Identity continuity may be established through evidence such as:

- current credentials;
- signed continuity records;
- authorized migration records;
- secure hardware identity;
- valid recovery procedures;
- operator-approved re-enrollment;
- known ownership continuity;
- bounded challenge response.

No single evidence type is mandatory for every implementation.

The evidence must satisfy the applicable assurance profile.

## 9. Credentials

### 9.1 Credential scope

A credential shall identify or imply:

- subject;
- issuer;
- credential type;
- valid scope;
- issuance boundary;
- expiry or renewal boundary;
- revocation mechanism;
- applicable trust domain;
- permitted use.

### 9.2 Credential minimization

Credentials should disclose only information required for their purpose.

### 9.3 Credential separation

Separate credentials should be used when materially different authority or exposure requires independent:

- revocation;
- rotation;
- custody;
- assurance;
- audit.

### 9.4 Credential storage

Credential material shall be protected according to its sensitivity.

### 9.5 Credential export

Credentials and key material shall be non-exportable by default where practical.

### 9.6 Credential copying

Copying credential material does not create a new legitimate identity or authority.

### 9.7 Credential state

A credential may be:

- proposed;
- issued;
- active;
- restricted;
- rotating;
- expired;
- revoked;
- compromised;
- superseded;
- conflicting;
- unknown.

## 10. Authentication

### 10.1 Authentication is scoped

An authentication result shall be valid only for a declared:

- identity claim;
- operation;
- session;
- trust domain;
- time or epoch;
- assurance profile.

### 10.2 Mutual authentication

Connections requiring strong identity assurance should authenticate all relevant participants.

Mutual authentication does not require identical credentials or assurance methods.

### 10.3 Authentication freshness

Authentication evidence shall be sufficiently fresh for the requested operation.

### 10.4 Reauthentication

Reauthentication may be required after:

- credential rotation;
- connection re-establishment;
- security-policy change;
- trust-domain crossing;
- long suspension;
- recovery;
- privilege escalation;
- unusual security evidence.

### 10.5 Failed authentication

Failed authentication may result in:

- denial;
- challenge;
- restricted anonymous or public behavior;
- rate limiting;
- security or immune evidence.

Failure shall not consume unbounded resources.

### 10.6 Authentication uncertainty

When authentication evidence is incomplete or conflicting, the state shall remain:

- unknown;
- restricted;
- challenged;
- denied;

according to policy.

## 11. Authentication assurance

Assurance is multidimensional.

A result may need to describe:

- identity proof strength;
- credential-protection strength;
- challenge freshness;
- issuer confidence;
- device assurance;
- operator-presence assurance;
- continuity assurance;
- revocation-check status.

One numeric score is not required.

A high result in one dimension shall not conceal weakness in another.

## 12. Trust model

### 12.1 Trust is scoped

Trust shall be interpreted relative to:

- subject;
- target;
- relationship;
- operation;
- signal domain;
- signal intent;
- trust domain;
- time;
- evidence boundary.

### 12.2 Trust is multidimensional

Relevant trust dimensions may include:

- identity confidence;
- credential integrity;
- protocol conformance;
- reliability;
- provenance quality;
- operational stability;
- mediation behavior;
- security posture;
- historical compliance;
- evidence independence.

### 12.3 No universal trust scalar

Implementations may calculate scores for defined purposes.

A single global trust score shall not be treated as complete authority or universal reliability.

### 12.4 Trust does not automatically transfer

Trust in one operation does not imply trust in another.

Trust in one participant does not automatically transfer to:

- its host;
- its operator;
- its delegates;
- its mediators;
- its other endpoints;
- its other relationships.

### 12.5 Trust is not necessarily transitive

If participant A trusts B and B trusts C, A does not automatically trust C.

### 12.6 Trust evidence ages

Trust evidence becomes stale.

### 12.7 Trust may decrease

Trust may be restricted because of:

- stale evidence;
- repeated contract violations;
- credential weakness;
- unexplained identity changes;
- security incidents;
- ignored backpressure;
- conflicting provenance;
- unreliable mediation;
- repeated invalid claims.

### 12.8 Trust restoration

Trust restriction may be reduced after:

- reauthentication;
- successful challenge;
- credential replacement;
- recovery validation;
- operator review;
- evidence reconciliation;
- sustained compliant operation.

Restoration is a new decision.

## 13. Trust evidence

Trust evidence shall identify or permit recovery of:

- subject;
- evidence producer;
- observed behavior or condition;
- observation scope;
- observation time;
- evidence freshness;
- confidence;
- provenance;
- independence or correlation;
- applicable trust domain.

Repeated forwarding of one observation shall not become several independent trust observations.

## 14. Trust state

A public implementation should support states equivalent to:

- sufficient for declared scope;
- sufficient with restrictions;
- insufficient;
- stale;
- conflicting;
- unavailable;
- unknown.

The system need not disclose detailed trust state to every participant.

Internal enforcement must preserve the actual state when known.

## 15. Trust policy

Trust policy may influence:

- whether admission is allowed;
- required authentication assurance;
- grant duration;
- port visibility;
- required mediation;
- validation effort;
- resource limits;
- payload access;
- permitted signal intents;
- whether directives are allowed.

Trust policy shall not override:

- hard safety ceilings;
- explicit revocation;
- capability scope;
- constitutional restrictions;
- required operator authority.

## 16. Capabilities

### 16.1 Capability principle

Privileged operations shall require explicit authority appropriate to their scope.

Capabilities are the preferred public abstraction for representing operation-specific authority.

### 16.2 Capability contents

A capability shall identify or bind:

- capability identity;
- subject identity;
- issuer identity;
- target identity or scope;
- permitted operation;
- permitted parameters;
- permitted signal domain or intent where relevant;
- resource bounds;
- validity boundary;
- connection or policy generation where relevant;
- delegation policy;
- mediation restrictions;
- concurrence requirements;
- revocation mechanism;
- audit requirements.

### 16.3 Narrow scope

Capabilities should grant the minimum authority required.

### 16.4 Non-transferable by default

A capability shall not be transferable unless its transfer or delegation policy explicitly permits it.

### 16.5 No ambient privilege

A participant shall not gain privileged authority merely because it:

- runs locally;
- owns the endpoint;
- hosts the process;
- has administrator access to the operating system;
- established the secure session;
- has high trust;
- belongs to an infrastructure relationship.

### 16.6 Capability exercise

Exercising a capability remains subject to:

- authentication;
- admission;
- current lifecycle state;
- resource availability;
- safety conditions;
- revocation;
- operation-specific validation.

### 16.7 Capability does not guarantee success

A valid capability permits a bounded attempt.

It does not guarantee execution or completion.

## 17. Capability forms

Capabilities may be represented through:

- signed claims;
- protected references;
- session-bound grants;
- capability tokens;
- local protected handles;
- distributed authorization records;
- another verifiable mechanism.

The representation is implementation-defined.

The semantics established by this specification are not.

## 18. Bearer capabilities

A capability shall not behave as an unrestricted bearer secret unless explicitly designed for that purpose.

Bearer capabilities require especially strong:

- secrecy;
- scope limitation;
- expiry;
- replay protection;
- revocation;
- transport protection.

Subject-bound capabilities should be preferred for privileged operations.

## 19. Capability generations

Capabilities may carry a generation, epoch, or policy version.

A stale capability shall not regain authority after:

- credential rotation;
- connection recovery;
- ownership change;
- policy change;
- revocation;
- target replacement.

## 20. Delegation

### 20.1 Delegable authority

Authority may be delegated only when the original authority explicitly permits delegation.

### 20.2 Delegation record

A delegation shall identify:

- delegator;
- delegate;
- original authority source;
- granted operation;
- target scope;
- parameter limits;
- resource limits;
- validity;
- further-delegation permission;
- revocation path;
- audit requirements.

### 20.3 No amplification

Delegation shall not expand:

- operation scope;
- target scope;
- resource scope;
- validity;
- protected status;
- priority ceiling;
- further-delegation rights.

### 20.4 Delegation depth

Delegation chains shall remain bounded.

### 20.5 Delegation provenance

A verifier shall be able to recover sufficient delegation provenance to determine whether the current grant is valid.

### 20.6 Revoking a parent

Revocation or expiry of a parent authority shall affect derived delegations according to explicit policy.

A child delegation shall not silently outlive the authority from which it was derived unless independently reissued.

## 21. Authority roles

The public architecture distinguishes at least the following authority categories.

### 21.1 Participant-local authority

Authority over the participant’s own bounded participation.

### 21.2 Endpoint-owner authority

Authority over ordinary endpoint and port contracts within higher policy.

### 21.3 Admission authority

Authority to grant or deny ACS admission within a declared scope.

### 21.4 Security authority

Authority to evaluate credentials, issue or revoke capabilities, and govern secure-session requirements within scope.

### 21.5 Lifecycle authority

Authority to approve or perform defined ACS lifecycle transitions.

### 21.6 Operator authority

Authority explicitly assigned to an authenticated human or external operator actor.

### 21.7 Physical-safety authority

Authority governing narrow safety-critical actions and hard interlocks.

### 21.8 Immune authority

Authority assigned to IMM or immune-facing participants for specifically defined observation, recommendation, challenge, or containment operations.

No authority category is automatically universal.

## 22. Authority intersection

When several grants or restrictions apply, effective authority shall be no broader than the intersection of all applicable hard constraints.

A general grant shall not override a more specific restriction.

Examples include:

- a broad connection capability restricted by one port attachment;
- operator authority restricted by a physical safety ceiling;
- endpoint-owner authority restricted by security policy;
- lifecycle authority restricted by missing concurrence;
- immune containment authority restricted to one participant or port.

## 23. Authority conflict

A conflict exists when valid-looking authorities produce incompatible requirements.

Examples include:

- one authority grants while another applicable authority denies;
- two issuers claim incompatible ownership;
- an operator requests an action forbidden by a hard safety rule;
- one security domain recognizes a credential another domain has revoked;
- two connection generations present valid-looking capabilities.

Authority conflict shall remain explicit.

It shall not be resolved solely through:

- arrival order;
- numerical priority;
- physical proximity;
- computational capacity;
- relationship strength;
- trust score;
- local preference.

## 24. Authority precedence

Public ACS does not define one universal ranking of every authority.

It establishes the following rules:

1. Hard constitutional and physical safety ceilings cannot be overridden by ordinary grants.
2. Explicit applicable denial or restriction must not be ignored merely because a broader grant exists.
3. More specific authority does not become valid unless its issuer is authorized for that scope.
4. Conflicting authority remains unresolved until a declared precedence or concurrence policy applies.
5. Absence of a precedence rule does not permit the most permissive outcome.
6. Privileged operations shall fail closed, defer, or remain conflicting when required authority cannot be established.

## 25. Concurrence

Sensitive operations may require approval from several independently scoped authorities.

Potential examples include:

- ownership transfer;
- permanent retirement;
- trust-anchor replacement;
- root credential rotation;
- controlled propagation;
- destructive recovery;
- organism-wide restriction;
- release of highly sensitive data.

Concurrence shall define:

- participating authority categories;
- required scope;
- validity window;
- independence requirements;
- cancellation;
- conflict behavior;
- audit requirements.

Concurrence is not automatically simple majority voting.

## 26. Operator identity and authority

### 26.1 Explicit operator identity

Operator actions shall use an identity distinguishable from ordinary participant identities.

### 26.2 No universal operator privilege

An operator identity shall not automatically possess unrestricted authority.

### 26.3 Intent resolution

Natural-language, graphical, or physical operator input shall be resolved into a specific operation and scope before privileged execution.

### 26.4 Operator presence

Sensitive operations may require current or recent intentional operator presence.

### 26.5 Operator delegation

Operator authority may be delegated only under explicit policy.

### 26.6 Emergency actions

Emergency operator authority should favor narrow actions such as:

- safe stop;
- isolation;
- hibernation;
- power reduction;
- recovery-mode entry.

Emergency authority shall not silently become general administration.

## 27. Secure sessions

### 27.1 Session purpose

A secure session may provide:

- peer authentication;
- confidentiality;
- integrity;
- message authenticity;
- replay protection;
- freshness;
- channel binding;
- security-profile negotiation.

### 27.2 Session scope

A session shall identify or imply:

- participants;
- security profile;
- established assurance;
- cryptographic context;
- validity boundary;
- rekey behavior;
- closure behavior;
- supported channel bindings.

### 27.3 Session identity

A secure-session identity shall be distinguishable from:

- participant identity;
- connection identity;
- relationship identity;
- credential identity.

### 27.4 Session establishment is not ACS activation

A secure session may exist before the ACS connection is admitted.

### 27.5 Several connections per session

One secure session may carry several connections or port interactions when explicitly permitted.

Security context must not erase their separate admission and authority scopes.

### 27.6 Several sessions per relationship

A relationship may use several secure sessions for:

- redundancy;
- different security profiles;
- different directions;
- different ports;
- migration;
- key rotation.

## 28. Channel binding

Security-sensitive authorization may be bound to:

- one secure session;
- one connection generation;
- one endpoint pair;
- one transport context;
- one device state.

Channel binding prevents authority from being copied into an unrelated context.

Changing the bound context may require:

- reauthentication;
- reissuance;
- renewed admission;
- capability replacement.

## 29. Confidentiality

Confidentiality protects information from unauthorized disclosure.

Confidentiality requirements may differ by:

- signal domain;
- payload sensitivity;
- port;
- relationship;
- trust domain;
- mediator;
- lifecycle state.

Confidentiality does not remove the need for:

- authorization;
- integrity;
- freshness;
- provenance;
- resource limits.

## 30. Integrity and authenticity

Integrity protects information from undetected modification.

Authenticity provides evidence concerning who produced or authorized the information.

A message may possess integrity without its semantic claim being true.

A message may be authentic but unauthorized for the current operation.

## 31. Freshness and replay protection

Security-sensitive interactions shall define:

- freshness requirements;
- replay identifiers or windows;
- duplicate handling;
- expiry;
- acceptable clock or sequence uncertainty.

A previously valid directive shall not become valid again merely because it is retransmitted.

Replay protection shall remain compatible with ACS-0003 signal identity and ACS-0005 connection generations.

## 32. Downgrade resistance

Negotiation, fallback, recovery, or reconnect shall not silently select a weaker security profile than required.

A downgrade may proceed only when:

- the weaker profile is explicitly permitted;
- affected participants can identify it;
- admission is reevaluated;
- authority scope is narrowed appropriately.

## 33. Key custody

### 33.1 Separation from ownership

Endpoint, service, or memory ownership does not automatically grant key custody.

### 33.2 Explicit assignment

Key custody shall be assigned explicitly.

### 33.3 Minimum exposure

Private key material should be exposed to the smallest practical number of participants and processes.

### 33.4 Key use versus key export

A custodian may be authorized to perform cryptographic operations without being authorized to export key material.

### 33.5 Key rotation

Key rotation shall define:

- new-key activation;
- old-key overlap;
- credential reissuance;
- session treatment;
- stale-key rejection;
- recovery behavior.

### 33.6 Key loss

Loss of key material shall not automatically cause logical identity loss.

Recovery or re-enrollment may establish new credentials while preserving identity when authorized.

### 33.7 Key compromise

Suspected or confirmed compromise may require:

- credential restriction;
- revocation;
- session termination;
- capability invalidation;
- reauthentication;
- identity recovery;
- operator review.

## 34. Trust anchors

Trust anchors shall be:

- explicitly scoped;
- protected;
- replaceable through governed procedures;
- auditable where appropriate;
- distinguishable from ordinary credentials.

Possession of one trust anchor shall not automatically grant authority across unrelated trust domains.

## 35. Trust domains

### 35.1 Domain scope

A trust domain shall define:

- recognized identity namespaces;
- accepted credential issuers;
- accepted trust anchors;
- security profiles;
- revocation sources;
- boundary-crossing requirements.

### 35.2 Domain crossing

Crossing a trust-domain boundary may require:

- additional authentication;
- credential translation;
- mediation;
- narrower capabilities;
- explicit disclosure;
- renewed admission.

### 35.3 No implicit federation

Two trust domains do not automatically trust each other merely because they share transport connectivity or one participant.

### 35.4 Federation

Federation shall define:

- which identity claims are accepted;
- how issuer authority is constrained;
- how revocation propagates;
- which operations remain prohibited;
- how conflicts are handled.

## 36. External participants

External participants may interact through:

- public facades;
- bounded external trust domains;
- capability-referenced ports;
- operator-approved integration;
- mediated relationships.

External participants shall not gain internal authority merely because they:

- implement public ACS;
- authenticate successfully;
- run on trusted hardware;
- use a secure session;
- possess a public capability.

## 37. Revocation

### 37.1 Revocable security objects

The following shall support revocation or equivalent invalidation:

- credentials;
- capabilities;
- delegations;
- secure sessions;
- trust assertions;
- admission grants;
- recovery grants.

### 37.2 Revocation causes

Revocation may result from:

- expiry;
- credential compromise;
- key compromise;
- relationship retirement;
- identity conflict;
- policy change;
- operator action;
- security incident;
- immune evidence processed through authorized policy;
- ownership change;
- connection-generation change;
- misuse.

### 37.3 Revocation scope

Revocation may target:

- one credential;
- one capability;
- one operation;
- one participant instance;
- one logical participant;
- one trust domain;
- one policy generation;
- one connection generation.

### 37.4 Smallest effective scope

The smallest safe revocation scope should be preferred.

### 37.5 No voluntary dependency

Revocation shall not depend on the revoked participant cooperating.

## 38. Revocation distribution

Revocation evidence shall reach relevant enforcement points within bounded expectations appropriate to the operation’s risk.

Implementations shall define behavior during:

- delayed propagation;
- partition;
- evaluator unavailability;
- stale caches;
- conflicting revocation claims.

Privileged operations shall not continue indefinitely when revocation status cannot be established.

## 39. Authorization caching

Authorization decisions may be cached within bounded scope.

Caches shall account for:

- credential expiry;
- capability expiry;
- revocation;
- connection-generation changes;
- policy changes;
- trust-domain changes;
- ownership changes;
- immune restrictions.

Sensitive operations should require fresher validation than low-risk observations.

## 40. Session closure and rekey

Secure sessions may close or rekey because of:

- expiry;
- key rotation;
- credential change;
- inactivity;
- security incident;
- policy change;
- migration;
- recovery.

Session closure does not automatically:

- retire the relationship;
- retire the endpoint;
- complete domain operations;
- revoke every capability.

Those effects require explicit policy.

## 41. Security states

Security-sensitive objects should support states equivalent to:

- proposed;
- active;
- active with restrictions;
- stale;
- rotating;
- suspended;
- expired;
- revoked;
- compromised;
- recovering;
- conflicting;
- unknown.

Equivalent labels are permitted.

Materially different states must not be silently collapsed where the distinction affects authority.

## 42. Suspected compromise

Suspected compromise is an evidence state.

It is not identical to confirmed compromise.

Policy may respond through:

- challenge;
- reauthentication;
- narrowed grants;
- mandatory mediation;
- attachment restriction;
- session replacement;
- suspension;
- operator review.

The response must use authority and lifecycle mechanisms defined by ACS.

## 43. Confirmed compromise

Confirmed compromise may justify stronger actions within explicit authority, including:

- credential revocation;
- capability revocation;
- session termination;
- connection restriction;
- attachment suspension;
- trust-anchor replacement;
- recovery-mode entry.

Confirmed compromise does not automatically authorize destruction of:

- physical nodes;
- memory;
- unrelated participants;
- audit evidence.

## 44. Recovery and re-enrollment

### 44.1 Recovery principle

Recovery establishes new valid security state after ordinary continuity becomes unsafe or unavailable.

### 44.2 Recovery is restricted

Recovery credentials and ports should provide only the authority required to:

- establish identity continuity;
- replace credentials;
- validate substrate state;
- reconcile revocation;
- restore ordinary security services.

### 44.3 Re-enrollment

Re-enrollment may issue new credentials to an existing identity when continuity is established.

### 44.4 New identity

When continuity cannot be established, a new identity may be required.

Functional similarity does not prove identity continuity.

### 44.5 Recovery audit

Sensitive recovery shall preserve evidence sufficient to determine:

- requesting actor;
- authority;
- previous state;
- new state;
- credentials replaced;
- continuity evidence;
- unresolved conflicts.

## 45. Partition behavior

### 45.1 Existing sessions and grants

Existing security state may continue during partition only within its declared validity and partition profile.

### 45.2 New privileged authority

New privileged capabilities should fail closed, defer, or remain unknown when required security authorities are unavailable.

### 45.3 Local trust

Local cached trust evidence may support bounded reduced operation when explicitly permitted.

### 45.4 Revocation uncertainty

When revocation status is unknown, privileged operations shall be restricted according to risk.

### 45.5 Reconciliation

After partition, participants shall reconcile:

- credential generations;
- capability generations;
- revocations;
- session state;
- trust evidence;
- delegated authority;
- conflicting identity claims.

## 46. Security and resource exhaustion

Security mechanisms shall remain bounded.

The system must defend against:

- authentication flooding;
- challenge flooding;
- expensive credential validation;
- capability-probing;
- revocation-check exhaustion;
- secure-session exhaustion;
- audit amplification;
- replay floods.

Protected capacity shall remain available for:

- revocation;
- session closure;
- security-state reporting;
- bounded recovery;
- critical authentication.

## 47. Security observability

Authorized observers should be able to determine:

- identity state;
- credential state;
- session state;
- capability state;
- revocation status;
- assurance profile;
- trust evidence freshness;
- known conflict;
- recovery state.

Detailed security state need not be publicly disclosed.

## 48. Audit

Sensitive security operations should preserve bounded audit evidence concerning:

- identity claim;
- authentication outcome;
- capability issuance;
- delegation;
- revocation;
- trust-anchor change;
- recovery;
- operator action;
- authority conflict;
- privileged operation attempt.

Audit records must not expose:

- private keys;
- reusable secrets;
- unnecessary cognitive content;
- unnecessary payload content.

## 49. Privacy

Security evidence may reveal:

- relationships;
- topology;
- identity associations;
- operator activity;
- trust state;
- incidents;
- capabilities;
- recovery actions.

Access shall therefore remain separately controlled.

Security architecture shall not become an unrestricted surveillance mechanism.

## 50. Boundary with ACS-0002

ACS-0002 defines relationship classes and their purposes.

ACS-0007 defines security evidence and authority requirements that may apply to those relationships.

Relationship class does not automatically determine:

- authentication assurance;
- trust level;
- capability scope;
- secure-session profile.

## 51. Boundary with ACS-0003

ACS-0003 defines signal identity, provenance, confidence, freshness, authority representation, and replay semantics.

ACS-0007 defines how security establishes:

- source authenticity;
- capability validity;
- secure transport;
- authority evidence;
- replay protection.

The following remain distinct:

1. authentic source does not mean true claim;
2. confidence does not create authority;
3. provenance does not replace authentication;
4. encrypted delivery does not prove signal admission;
5. repeated authentic signals do not create new authority;
6. a stale capability does not become current through a fresh signal wrapper.

## 52. Boundary with ACS-0004

ACS-0004 defines endpoints, ports, visibility, attachments, bindings, and mediation.

ACS-0007 defines:

- how endpoint and participant identity are authenticated;
- which security profiles protect bindings;
- which capabilities authorize privileged port operations;
- how trust affects disclosure and mediation requirements;
- how key custody remains separate from endpoint ownership.

Visibility does not create security authority.

## 53. Boundary with ACS-0005

ACS-0005 defines connection and attachment lifecycle.

ACS-0007 defines security conditions affecting:

- connection activation;
- reauthentication;
- session replacement;
- resumption;
- rebinding;
- migration;
- suspension;
- closure;
- recovery.

A connection generation and secure-session generation may be related but are not identical.

## 54. Boundary with ACS-0006

ACS-0006 defines admission grants and resource budgets.

ACS-0007 supplies security and trust evidence used by admission.

The following remain distinct:

1. authentication does not grant admission;
2. trust may influence but does not replace admission;
3. a capability authorizes a bounded operation but does not guarantee resource availability;
4. admission does not issue broader capabilities than security policy permits;
5. resource pressure does not automatically reduce trust;
6. security authority does not override hard resource or physical safety ceilings.

## 55. Boundary with ACS-0008

ACS-0008 will define the ACS–immune integration boundary.

ACS-0007 establishes the security vocabulary available to that integration.

Immune systems may provide evidence concerning:

- identity anomalies;
- credential misuse;
- replay;
- conflicting provenance;
- suspicious mediation;
- ignored restrictions;
- unusual session behavior;
- repeated unauthorized operations.

ACS-0008 may define how such evidence leads to bounded requests or recommendations.

It shall not permit immune systems to invent:

- credentials;
- trust anchors;
- universal revocation authority;
- unrestricted key access;
- unlimited payload inspection;
- undefined containment operations.

Any immune-triggered security action must use authority, capability, admission, revocation, and lifecycle mechanisms defined by ACS.

## 56. Boundary with MEM

ACS-0007 governs communication identity, authentication, secure sessions, capabilities, and key custody boundaries.

MEM governs memory identity, memory authority, custody, retention, recovery, and memory-operation semantics.

The following remain distinct:

1. ACS participant identity is not logical memory identity.
2. An ACS credential is not memory provenance.
3. An ACS capability to invoke a port is not automatically authority to read, mutate, retain, or delete memory.
4. Memory custody does not grant key custody.
5. Physical possession of encrypted memory does not grant decryption authority.
6. A secure ACS connection does not prove memory availability or validity.
7. MEM may require additional memory-specific authority after ACS authorization.
8. Memory recovery and security recovery must coordinate without absorbing each other’s semantics.

## 57. Boundary with runtime

Runtime systems may implement:

- credential storage;
- secure-session libraries;
- key-use interfaces;
- capability caches;
- authentication services;
- audit transport.

Runtime control of those mechanisms does not create semantic authority beyond the ACS security policy.

## 58. Boundary with health

Health systems may report:

- credential-service availability;
- secure-session failure;
- key-custodian availability;
- authentication latency;
- revocation propagation state;
- hardware security status.

Health state contributes evidence.

It does not independently establish trust or authority.

## 59. Boundary with physical safety

No credential, capability, trust state, operator role, or secure session may silently override hard physical safety requirements.

Security mechanisms must preserve the ability to:

- reject unsafe directives;
- enter safe stop;
- isolate compromised control paths;
- maintain critical safety communication.

## 60. Public implementation requirements

A public implementation claiming support for ACS-0007 shall document:

- participant identity model;
- instance identity model;
- identity namespaces;
- continuity evidence;
- credential types;
- credential states;
- authentication profiles;
- authentication freshness;
- trust dimensions;
- trust-state representation;
- capability representation;
- delegation behavior;
- authority-conflict behavior;
- concurrence support;
- secure-session properties;
- channel-binding behavior;
- replay protection;
- downgrade handling;
- key-custody model;
- trust-domain behavior;
- revocation propagation;
- authorization caching;
- recovery and re-enrollment;
- audit and privacy behavior;
- unsupported features;
- known security limitations.

Public documentation need not reveal production credentials, trust anchors, keys, thresholds, or private procedures.

## 61. Conformance expectations

Conformance evidence should demonstrate that:

1. participant identity is independent of address and process;
2. credential rotation does not inherently change logical identity;
3. authentication does not grant unrestricted port use;
4. secure-session establishment does not activate an ACS connection by itself;
5. encryption does not bypass authorization;
6. trust does not create authority;
7. capabilities are scoped and bounded;
8. capability exercise remains subject to admission and lifecycle state;
9. capabilities do not self-expand after successful use;
10. delegation cannot amplify authority;
11. stale or revoked capabilities are rejected;
12. operator identity is not universal authority;
13. endpoint ownership is separate from key custody;
14. secure-session fallback does not silently weaken required security;
15. replayed directives do not regain authority;
16. conflicting identity claims remain explicit;
17. missing revocation state does not create privileged permission;
18. authorization caches expire or invalidate correctly;
19. partition does not silently duplicate exclusive authority;
20. recovery credentials do not become ordinary unrestricted credentials;
21. security mechanisms remain resource-bounded;
22. immune evidence does not automatically create universal revocation or containment authority;
23. ACS security remains distinct from MEM authority and memory identity;
24. audit remains useful without disclosing protected secrets.

## 62. Prohibited interpretations

This specification shall not be interpreted to mean that:

- a network address defines participant identity;
- a credential is the participant;
- authentication equals authorization;
- authorization equals admission;
- trust equals authority;
- encryption equals trust;
- a secure session equals a relationship;
- a secure session grants every attachment;
- endpoint ownership grants private-key access;
- physical possession grants logical identity;
- high trust bypasses capability requirements;
- a capability guarantees execution;
- capabilities are transferable by default;
- delegation may exceed the delegator’s authority;
- the most permissive conflicting authority wins;
- operator identity grants universal control;
- immune suspicion proves compromise;
- confirmed compromise authorizes destruction of unrelated state;
- session closure retires the relationship;
- credential loss necessarily destroys logical identity;
- public conformance requires disclosure of production security material.

## 63. Initial architectural commitments

ACS-0007 establishes that:

1. participant identity is logical and placement-independent;
2. identity, credentials, authentication, trust, authority, admission, and execution remain distinct;
3. identity claims require scoped evidence;
4. identity conflicts remain explicit;
5. credentials possess governed lifecycle and revocation;
6. authentication assurance is multidimensional;
7. trust is scoped, evidence-backed, time-sensitive, and non-universal;
8. trust does not automatically transfer or become transitive;
9. capabilities represent bounded operation-specific authority;
10. capabilities do not self-expand;
11. capabilities are non-transferable by default;
12. delegation is explicit, bounded, attributable, and non-amplifying;
13. authority conflicts fail closed, defer, or remain explicit;
14. hard constitutional and physical safety limits remain above ordinary grants;
15. sensitive operations may require concurrence;
16. operator authority is explicit and scoped;
17. secure sessions remain separate from relationships, connections, and admission;
18. channel binding prevents authority reuse in unrelated contexts;
19. confidentiality, integrity, authenticity, freshness, and authorization remain distinct;
20. replay and downgrade resistance are mandatory where required by profile;
21. endpoint ownership remains separate from key custody;
22. trust domains do not federate implicitly;
23. revocation does not depend on holder cooperation;
24. authorization caches remain bounded by expiry, generation, and revocation;
25. recovery and re-enrollment do not silently create unrestricted authority;
26. security mechanisms remain resource-bounded and privacy-controlled;
27. ACS-0008 must use ACS-defined security and authority mechanisms;
28. MEM retains independent memory identity and operation authority;
29. public implementations remain useful without exposing production credentials or private policy.

## 64. Open questions

The following questions remain for later specifications or implementation profiles:

- Which identity layers are mandatory for baseline public conformance?
- Which authentication-assurance dimensions should be standardized?
- Should participant identity always be globally unique within a deployment?
- Which continuity evidence is sufficient after total host loss?
- Which credentials may be hardware-bound?
- Which operations require mutual authentication?
- Which trust dimensions should be common across implementations?
- How should trust evidence from several domains be reconciled?
- Which trust evidence may be shared without violating privacy?
- How long may cached trust evidence remain usable?
- Which capabilities must be subject-bound rather than bearer-like?
- Which capability fields require standardized schemas?
- How should delegation chains be bounded?
- Which operations require independent concurrence?
- How should authority conflict be represented on public interfaces?
- Which security state should apply when revocation authorities disagree?
- How long may privileged operation continue when revocation state is unavailable?
- Which secure-session properties are required for baseline conformance?
- When must secure-session replacement change the ACS connection generation?
- How should session resumption preserve channel binding safely?
- Which downgrade paths, if any, should be standardized?
- How should root or trust-anchor replacement be authorized?
- Which recovery procedures may preserve identity after key loss?
- When must compromise require a new participant identity?
- Which security evidence may immune systems inspect without payload access?
- Which immune-triggered restrictions may be automatic?
- Which immune-triggered actions require security or operator concurrence?
- How should operator-presence evidence be represented?
- Which security audit records must survive hibernation and recovery?
- How should external trust domains interact with public ACS facades?
- Which key-custody roles may coexist on one minimal node?
- How should security continue on extremely minimal headless systems?
- Which security failures should produce health, immune, or operator notifications?
- How should security policy remain enforceable during extended partition?
- How should public conformance test that local fast paths preserve remote-equivalent security?

These questions do not permit implementations to weaken the authority, revocation, identity, or safety distinctions already established.

## 65. Closing principle

> **Node may rely upon an identity only to the degree supported by current evidence, and it may permit an operation only through explicit authority whose scope, origin, validity, and revocation state can be established.**

Identity says who is represented.

Authentication says how strongly that claim is supported.

Trust says how cautiously Node may rely upon available evidence.

A capability says which operation may be attempted.

Admission says whether the attempt may proceed now.

A secure session protects the communication context.

None of them independently establishes truth, successful execution, permanent entitlement, or unrestricted control.

## Revision history

### Version 0.1 — 2026-07-16

- Established the public identity, security, trust, capability, and authority architecture.
- Separated identity, credentials, authentication, trust, authorization, admission, and execution.
- Defined participant, instance, endpoint, connection, operator, role, device, issuer, and recovery identity layers.
- Established credential lifecycle, authentication assurance, identity continuity, and conflict requirements.
- Defined scoped, multidimensional, evidence-backed trust.
- Defined capabilities, delegation, non-amplification, authority roles, conflict, precedence, and concurrence.
- Defined secure sessions, channel binding, confidentiality, integrity, authenticity, freshness, replay protection, and downgrade resistance.
- Established key custody and trust-anchor boundaries.
- Defined trust domains, external-participant integration, revocation, authorization caching, partition behavior, and recovery.
- Preserved boundaries with admission, lifecycle, immune integration, runtime, health, physical safety, and MEM.
- Established the security and authority vocabulary required by ACS-0008.
- Defined public implementation requirements, conformance expectations, prohibited interpretations, commitments, and open questions.
