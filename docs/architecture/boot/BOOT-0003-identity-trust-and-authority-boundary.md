# BOOT-0003: Node BOOT Identity, Trust, and Authority Boundary

| Field | Value |
|---|---|
| Specification | BOOT-0003 |
| Title | Node BOOT Identity, Trust, and Authority Boundary |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | BOOT-PUB |
| Authors | Node |
| Last updated | 2026-07-18 |
| Approval | Pending review |
| Depends on | BOOT-0000; BOOT-0001; BOOT-0002; applicable approved ACS, MEM, and IMM public architecture |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in identity separation, authentication boundaries, rescue-specific identity profiles, bootstrap-authority scope, revalidation, delegation, concurrence, revocation, and conflict behavior; concrete credential formats, assurance profiles, trust anchors, security providers, and production authority assignments remain intentionally deferred |

> **BOOT may operate before ordinary runtime authority is available, but it must never convert presence, possession, reachability, authentication, urgency, or technical control into identity or permission.**

## Architectural-intent notice

This specification defines the public BOOT architecture governing:

- rescue-session identity;
- durable participant-identity boundaries;
- device and substrate identity;
- identity claims;
- identity evidence;
- persisted identity;
- enrollment evidence;
- provisional identity;
- hardware-rooted identity evidence;
- authentication results;
- authentication assurance;
- scoped trust evidence;
- trust-state interpretation;
- secure-session boundaries;
- bootstrap authority;
- authority anchors;
- authority envelopes;
- capabilities;
- delegation;
- concurrence;
- revocation;
- authority conflict;
- operator identity and authority;
- offline and partitioned operation;
- recovery identity;
- re-enrollment;
- identity continuity;
- authority revalidation;
- public conceptual records;
- conformance expectations.

This specification refines BOOT-0000, BOOT-0001, and BOOT-0002.

It does not replace ACS participant identity, ACS admission, security-provider cryptographic mechanisms, installer authority, MEM authority, IMM authority, runtime authority, physical-safety authority, or private deployment policy.

The public architecture is independently authored.

It is not produced by deleting, sanitizing, paraphrasing, or selectively redacting restricted identity, security, or recovery designs.

## 1. Purpose

BOOT may execute when:

- the normal runtime is unavailable;
- participant identity has not yet been established;
- persisted credentials may be stale;
- the machine is booting from installation media;
- storage has been replaced or cloned;
- authority providers are unreachable;
- revocation status is unavailable;
- a node requires re-enrollment;
- an assistance source offers recovery;
- an operator is present locally;
- manufacturer or hardware-rooted evidence is available;
- emergency conditions exist;
- an installation or recovery operation may alter durable state.

These conditions create pressure to accept weak shortcuts.

Common unsafe shortcuts include:

- using the BOOT session identifier as durable identity;
- identifying a node by address or hostname;
- treating a disk’s contents as proof of continuity;
- treating an enrollment token as identity;
- treating hardware-rooted evidence as unrestricted authority;
- treating authentication as authorization;
- treating a secure session as admission;
- treating a local operator as universal administrator;
- treating an assistance source as authoritative because it responded first;
- treating installation media as authoritative because it is physically present;
- restoring expired authority because recovery is urgent;
- accepting cached grants indefinitely while offline;
- using vague `trusted` status in place of explicit results.

BOOT-0003 prevents those shortcuts.

It defines what BOOT may rely upon, what it must preserve, what it must request from adjacent providers, and how it behaves when identity, authentication, trust evidence, or authority is incomplete.

## 2. Authority and relationship to earlier specifications

BOOT-0000 establishes that:

- BOOT does not self-grant identity or authority;
- discovery is not authentication;
- authentication is not authority;
- authority is not admission;
- a session identity is not durable participant identity;
- enrollment evidence is not identity;
- rooted identity does not automatically create authority;
- persisted identity requires revalidation;
- local, offline, rescue, first-boot, and emergency conditions do not dissolve architecture.

BOOT-0001 formalizes those rules through:

- BOOT-INV-002 — Execution context grants no authority.
- BOOT-INV-003 — Adjacent-system ownership remains intact.
- BOOT-INV-004 — Trust and completion stages remain distinct.
- BOOT-INV-005 — BOOT session identity is not durable participant identity.
- BOOT-INV-006 — Durable identity is not location, order, proximity, or responsiveness.
- BOOT-INV-007 — Persisted, enrollment, and rooted identity evidence require scoped validation.
- BOOT-INV-008 — BOOT cannot self-grant or enlarge bootstrap authority.
- BOOT-INV-010 — Unknown, absence, unavailability, and invalidity remain truthful.
- BOOT-INV-016 — Assistance-source responsiveness does not create preference or authority.
- BOOT-INV-017 — Transport and secure-session success do not establish semantic acceptance.
- BOOT-INV-018 — Assistance sessions remain bounded and attributable.
- BOOT-INV-019 — Recovery requests and plans do not create authority.
- BOOT-INV-024 — Image boot and hardware discovery do not authorize participation or installation.
- BOOT-INV-025 — Dynamic self-assembly remains governed.
- BOOT-INV-027 — Update availability does not create release or activation authority.
- BOOT-INV-028 — Peer propagation remains locally authorized.
- BOOT-INV-029 — Public and private material remain separated.
- BOOT-INV-030 — Runtime handoff does not create broader authority.

BOOT-0002 defines:

- independent authority state;
- operation-scoped authority decisions;
- explicit unknown, unavailable, expired, revoked, conflicting, and indeterminate conditions;
- revisioned authoritative transitions;
- lock-down scope;
- restored-state revalidation;
- assistance phases for authentication and admission.

BOOT-0003 defines the identity, trust, authentication, and authority semantics consumed by those documents.

If this specification conflicts with a lower-numbered BOOT specification:

1. the lower-numbered specification governs;
2. the conflict MUST be reported;
3. BOOT-0003 MUST be corrected;
4. implementations MUST NOT choose the more permissive interpretation.

No blocking contradiction was identified during drafting.

## 3. Normative language

The terms **MUST** and **MUST NOT** define mandatory architectural requirements.

The terms **SHOULD** and **SHOULD NOT** define strong recommendations. A departure requires documented justification and MUST NOT violate a mandatory requirement.

The term **MAY** defines permitted behavior.

Terms such as:

- authenticated;
- authorized;
- admitted;
- signature verified;
- digest verified;
- current;
- rooted;
- provisional;
- secure;
- eligible;
- revoked;
- expired;
- conflicting

MUST identify their applicable:

- subject;
- provider;
- scope;
- operation;
- trust domain;
- revision;
- freshness or lifecycle boundary.

The unqualified term **trusted** MUST NOT be used as a substitute for those explicit results.

## 4. Scope

This specification governs BOOT-facing semantics for:

- BOOT sessions;
- boot attempts;
- physical and virtual substrates;
- devices;
- logical Node participants;
- participant instances;
- operators;
- service roles;
- security providers;
- credential issuers;
- authority sources;
- recovery actors;
- installation media;
- distribution releases;
- assistance sources;
- installer actors;
- transferring peers;
- artifact and release authorities;
- public and restricted module sources.

It applies during:

- normal boot;
- first installation;
- rescue;
- recovery;
- re-enrollment;
- credential rotation;
- device replacement;
- storage restoration;
- offline operation;
- partitioned operation;
- operator-assisted recovery;
- unattended installation;
- dynamic local assembly;
- artifact retrieval;
- update preparation;
- rollback;
- runtime handoff.

## 5. Explicit non-goals

BOOT-0003 does not define:

- one participant-identity scheme;
- one identity namespace;
- one public-key infrastructure;
- one certificate format;
- one credential format;
- one hardware root;
- one trusted-platform module;
- one secure-element interface;
- one authentication protocol;
- one secure-session protocol;
- one cryptographic suite;
- one signature algorithm;
- one digest algorithm;
- one key-storage mechanism;
- one trust-score formula;
- one assurance-level scale;
- one authority hierarchy;
- one operator-authentication method;
- one enrollment workflow;
- one recovery secret;
- one revocation-distribution mechanism;
- one concurrence policy;
- one trust-anchor replacement policy;
- one private recovery policy;
- one release-signing policy;
- one source-server identity;
- one production identity catalog;
- one production authority assignment.

Those mechanisms belong to ACS, security-provider architecture, future OS and update architecture, deployment profiles, or private operational policy.

## 6. Foundational distinctions

### 6.1 BOOT session identity is not participant identity

A BOOT session identity identifies one bounded instance of BOOT coordination.

It supports:

- correlation;
- revision;
- retry;
- continuation;
- diagnostics;
- idempotency.

It does not establish:

- durable participant identity;
- ACS admission;
- recovery authority;
- installer authority;
- release authority;
- ordinary mesh membership.

### 6.2 Identity is not a credential

Identity describes the logical subject being represented.

A credential provides evidence supporting an identity or authority claim.

Credentials may:

- expire;
- rotate;
- be replaced;
- be revoked;
- be compromised;
- become unavailable.

Logical identity may remain continuous through credential change when continuity is established.

### 6.3 Identity is not an address or location

The following do not independently establish identity:

- IP address;
- MAC address;
- hostname;
- route;
- interface;
- disk path;
- device path;
- process identifier;
- process name;
- filesystem mount point;
- physical location;
- discovery order;
- boot order.

### 6.4 Identity is not physical possession

Possession of:

- a machine;
- a disk;
- installation media;
- a credential file;
- a secure element;
- a removable token;
- an artifact cache;
- a network interface

does not automatically establish the right to represent every identity associated with it.

### 6.5 Identity claim is not accepted identity

A syntactically valid identity claim is not accepted merely because:

- it is well formed;
- it was signed by some key;
- it came from local storage;
- it was used previously;
- it arrived through a secure session;
- no competing claim is presently visible.

### 6.6 Authentication is not authority

Authentication establishes a provider-scoped result concerning presented identity evidence.

It does not determine whether the authenticated subject may:

- recover;
- install;
- activate;
- enroll;
- delete;
- propagate;
- access protected source;
- request ordinary runtime admission.

### 6.7 Authority is not admission

Authority permits a bounded attempt within an identified envelope.

ACS or another owning admission boundary may still:

- reject;
- defer;
- restrict;
- rate-limit;
- deny resource allocation.

### 6.8 Trust evidence is not authority

Trust evidence may influence:

- required authentication assurance;
- mediation;
- disclosure;
- grant duration;
- resource limits;
- validation effort;
- restricted operation.

Trust evidence does not independently create permission.

### 6.9 Trust is not truth

A strongly authenticated or historically reliable subject may still:

- be mistaken;
- present stale information;
- offer incompatible material;
- lack applicable authority;
- be compromised after authentication.

### 6.10 Encryption is not trust or authority

Protected communication may provide declared confidentiality and integrity.

It does not prove:

- semantic truth;
- operation authority;
- ACS admission;
- artifact eligibility;
- recovery completion.

### 6.11 Secure session is not assistance session

A secure session is a security-provider context.

An assistance session is a BOOT semantic interaction.

Either may exist without the other.

### 6.12 Secure session is not relationship or admission

Establishing a secure session does not create:

- an ACS relationship;
- an admitted connection;
- a capability;
- recovery authority;
- installer authority.

### 6.13 Capability is not identity

A capability permits a bounded operation.

It does not become the holder’s identity.

### 6.14 Capability is not success

A valid capability permits an attempt.

It does not prove:

- provider availability;
- resource availability;
- operation acceptance;
- execution;
- completion.

### 6.15 Operator identity is not universal privilege

An authenticated operator receives only authority explicitly granted for the applicable operation and scope.

### 6.16 Rooted identity is not unrestricted authority

Hardware-backed or manufacturer-provisioned evidence may strengthen an identity claim.

It does not automatically establish:

- recovery authority;
- installer authority;
- ACS admission;
- release authority;
- propagation authority;
- software validity;
- current credential status.

### 6.17 Previous validity is not current validity

Previous authentication, authority, admission, trust evidence, or credential validity may now be:

- stale;
- expired;
- revoked;
- superseded;
- conflicting;
- unavailable;
- indeterminate.

### 6.18 Denial is not proof of compromise

A request may be denied because of:

- missing evidence;
- unavailable authority;
- expired credentials;
- unsupported mechanisms;
- scope mismatch;
- resource constraints;
- policy restriction;
- ambiguity.

Denial alone does not prove malicious intent.

## 7. Core terminology

### 7.1 Identity namespace

An **identity namespace** defines the context in which an identity value is unambiguous.

An identity value without a namespace MUST NOT be assumed globally unique.

### 7.2 Logical participant identity

A **logical participant identity** identifies one continuing Node or ACS participant independently of its current process, host, address, transport, or runtime instance.

ACS owns logical participant identity.

### 7.3 Participant-instance identity

A **participant-instance identity** identifies one runtime realization of a logical participant.

### 7.4 Device identity

A **device identity** identifies one physical or virtual device where device-level identity is required.

Device identity does not automatically become participant identity.

### 7.5 Substrate identity

A **substrate identity** identifies the physical or virtual machine being inspected.

A substrate may remain unidentified, provisionally identified, or known only through bounded hardware evidence.

### 7.6 BOOT session identity

A **BOOT session identity** identifies one bounded BOOT coordination session.

### 7.7 Boot-attempt identity

A **boot-attempt identity** identifies one firmware, bootloader, kernel, or rescue-environment startup attempt.

A BOOT session may span more than one boot attempt through an authorized continuation record.

### 7.8 Operator identity

An **operator identity** identifies a human or externally governed operator actor.

It remains distinct from participant, device, and service-role identity.

### 7.9 Service-role identity

A **service-role identity** identifies a logical responsibility such as:

- security authority;
- installer;
- admission evaluator;
- key custodian;
- operator gateway;
- recovery coordinator;
- release authority.

A role is not identical to the participant currently assigned to perform it.

### 7.10 Credential-issuer identity

A **credential-issuer identity** identifies the authority responsible for issuing or attesting to a credential.

### 7.11 Authority-source identity

An **authority-source identity** identifies the actor or governed mechanism producing an authority decision.

### 7.12 Recovery identity

A **recovery identity** identifies a restricted actor, grant, credential, or role usable during recovery.

Recovery identity MUST NOT silently become ordinary production authority.

### 7.13 Release identity

A **release identity** identifies one governed Node distribution or update release.

It is not the identity of the peer transferring it.

### 7.14 Identity claim

An **identity claim** states that an actor, instance, device, endpoint, message, or record represents a declared identity.

### 7.15 Identity evidence

**Identity evidence** supports or contradicts an identity claim.

### 7.16 Credential

A **credential** is a protected object, claim, or handle used to support:

- authentication;
- enrollment;
- authorization;
- delegation;
- secure-session establishment.

### 7.17 Authentication

**Authentication** evaluates whether identity evidence satisfies an identified assurance profile for a declared scope.

### 7.18 Authentication assurance

**Authentication assurance** describes the established conditions and limitations of an authentication result.

### 7.19 Trust evidence

**Trust evidence** is an observation relevant to willingness to rely upon a subject or claim within a declared scope.

### 7.20 Trust state

**Trust state** is a bounded interpretation of available trust evidence.

It is not a universal scalar and not authority.

### 7.21 Bootstrap authority

**Bootstrap authority** is the authority permitting an identified boot, enrollment, recovery, installation, activation, or handoff operation before ordinary runtime authority is fully available.

### 7.22 Authority anchor

An **authority anchor** is an externally grounded source or root used to determine whether a bootstrap-authority claim is eligible for evaluation.

An anchor is not automatically an operation grant.

### 7.23 Authority evidence

**Authority evidence** supports or contradicts a claim that an authority source may govern a declared operation.

### 7.24 Authority envelope

An **authority envelope** is the complete bounded meaning of one authority decision.

### 7.25 Capability

A **capability** is a bounded authorization permitting a subject to attempt an operation against a declared target and scope.

### 7.26 Delegation

**Delegation** derives narrower authority from an existing delegable authority.

### 7.27 Concurrence

**Concurrence** requires several independently scoped authority decisions for one sensitive operation.

### 7.28 Revocation

**Revocation** ends or narrows future use of previously applicable security or authority material.

### 7.29 Revalidation

**Revalidation** evaluates whether previously established identity, authentication, or authority material remains applicable under current conditions.

### 7.30 Trust domain

A **trust domain** is a declared scope within which specified identity namespaces, issuers, anchors, revocation sources, and security policies are recognized.

## 8. Identity namespaces and layers

BOOT-facing records MUST distinguish applicable identity layers.

At minimum, implementations MUST avoid confusing:

| Identity layer | Primary meaning | Must remain distinct from |
|---|---|---|
| BOOT session | One coordination session | Durable participant identity |
| Boot attempt | One startup attempt | BOOT session |
| Physical substrate | Machine being inspected | Logical Node participant |
| Device | One physical or virtual device | Participant identity |
| Logical participant | Continuing Node or ACS participant | Host, process, address |
| Participant instance | One runtime realization | Logical participant |
| Operator | Human or governed external actor | Participant and service role |
| Service role | Logical responsibility | Current role holder |
| Credential issuer | Credential-producing authority | Credential subject |
| Authority source | Decision-producing authority | Requester |
| Recovery actor | Restricted recovery subject | Ordinary production actor |
| Release identity | Governed distribution or update | Transferring peer |
| Assistance source | Candidate provider of help | Authority source |

A single implementation component MAY hold several roles only when each identity and authority scope remains separately observable.

## 9. Rescue-specific BOOT identity profiles

BOOT profiles describe BOOT-facing identity conditions.

They do not replace ACS participant-identity lifecycle states.

### 9.1 `BOOT_EPHEMERAL`

`BOOT_EPHEMERAL` represents a session-scoped identity condition used only for bounded BOOT correlation.

It MUST provide:

- a bounded session identity;
- a defined namespace;
- operation and attempt correlation;
- no durable participant-identity claim;
- no durable authority;
- no automatic ACS admission;
- no ordinary mesh membership;
- no installer authority;
- no activation authority;
- no propagation authority.

A `BOOT_EPHEMERAL` subject MAY perform operations that require no broader identity or authority, such as:

- bounded local inspection;
- generation of non-secret diagnostics;
- requesting an eligible identity process;
- requesting assistance;
- presenting candidate evidence.

It MUST NOT perform privileged durable mutation solely because it is local or active.

### 9.2 `BOOT_PROVISIONAL`

`BOOT_PROVISIONAL` represents a bounded identity condition issued or recognized through an eligible provisioning, enrollment, installer, or operator-authority path.

A provisional record MUST identify or reference:

- subject;
- namespace;
- issuer;
- issuance authority;
- issuance revision;
- intended scope;
- validity boundary;
- revalidation requirement;
- credential or evidence references;
- revocation path;
- unresolved limitations.

`BOOT_PROVISIONAL` does not automatically establish:

- ordinary ACS admission;
- durable production authority;
- installer authority;
- recovery authority;
- propagation authority;
- unrestricted access to private modules.

A provisional subject MAY receive separately authorized narrow capabilities.

### 9.3 `BOOT_ROOTED`

`BOOT_ROOTED` represents an identity condition supported by manufacturer-provisioned, hardware-backed, or equivalent security-provider evidence.

A rooted record MUST identify or reference:

- security provider;
- root or attestation class;
- subject binding;
- challenge or freshness result where applicable;
- hardware or device scope;
- lifecycle state;
- revocation or invalidation mechanism;
- assurance limitations.

`BOOT_ROOTED` MAY provide stronger continuity or anti-cloning evidence.

It does not establish:

- current software validity;
- current credential validity;
- participant admission;
- recovery authority;
- installer authority;
- release authority;
- universal ownership.

### 9.4 Profile transitions

A profile transition MUST be explicit and revisioned.

Examples include:

```text
BOOT_EPHEMERAL
    → BOOT_PROVISIONAL

BOOT_EPHEMERAL
    → BOOT_ROOTED

BOOT_PROVISIONAL
    → revalidation required

BOOT_ROOTED
    → conflicting

BOOT_PROVISIONAL
    → retired
```

A transition to stronger evidence does not automatically increase authority.

### 9.5 Profile coexistence

A subject MAY have:

- an ephemeral BOOT session identity;
- provisional participant evidence;
- rooted device evidence

at the same time.

These records describe different scopes and MUST NOT be merged into one unrestricted identity claim.

## 10. Identity claims and evidence

### 10.1 Identity claim requirements

A BOOT-facing identity claim MUST identify:

- claim identity;
- claimed subject;
- claimed namespace;
- claimant;
- claim scope;
- evidence references;
- operation context;
- freshness boundary;
- claim revision.

### 10.2 Eligible identity evidence

Identity evidence MAY include:

- credential proof;
- challenge response;
- enrollment records;
- installer-provisioned records;
- operator-confirmed enrollment;
- signed continuity records;
- authorized migration records;
- hardware-backed evidence;
- manufacturer-provisioned evidence;
- recovery records;
- prior authenticated state;
- security-provider results.

No one evidence class is universally sufficient.

### 10.3 Evidence provenance

Identity evidence MUST identify or permit recovery of:

- producer;
- issuer;
- subject;
- observation or issuance scope;
- revision;
- freshness;
- integrity status;
- transformation history;
- correlation with other evidence;
- uncertainty;
- conflict.

### 10.4 Evidence independence

Repeated delivery, copying, forwarding, or re-encoding of one evidence item MUST NOT become several independent items.

### 10.5 Location observations

Address, hostname, route, disk path, process identity, and physical location MAY be retained as observations.

They MUST NOT become durable identity evidence by themselves.

### 10.6 Cloned or restored storage

Copied or restored identity material MUST NOT automatically establish continuity.

A copied credential may indicate:

- legitimate restoration;
- unauthorized cloning;
- stale backup;
- duplicate active identity;
- unknown state.

Revalidation and conflict detection are required.

### 10.7 Identity conflict

A conflict exists when eligible-looking claims identify incompatible subjects or continuity histories.

Conflict MUST remain explicit.

BOOT MUST NOT select a claimant solely because it:

- responded first;
- is physically local;
- occupies the prior address;
- contains the older filesystem;
- has more resources;
- presents the longest history;
- presents a cached credential;
- controls installation media.

## 11. BOOT identity-evaluation results

BOOT may retain a provider-attributed identity-evaluation result using values equivalent to:

| Result | Meaning |
|---|---|
| `IDENTITY_NOT_EVALUATED` | No applicable identity evaluation exists. |
| `IDENTITY_UNRESOLVED` | Required identity has not been established. |
| `IDENTITY_EVIDENCE_PRESENT` | Candidate evidence exists but has not produced an accepted result. |
| `IDENTITY_AUTHENTICATION_PENDING` | Authentication is awaiting provider work or prerequisites. |
| `IDENTITY_AUTHENTICATED` | The provider authenticated the claim for the declared scope and assurance profile. |
| `IDENTITY_AUTHENTICATED_RESTRICTED` | Authentication succeeded with material limitations. |
| `IDENTITY_REVALIDATION_REQUIRED` | Previously established identity requires current revalidation. |
| `IDENTITY_REJECTED` | The identity claim failed the applicable evaluation contract. |
| `IDENTITY_EXPIRED` | Applicable identity or credential validity ended. |
| `IDENTITY_REVOKED` | Applicable identity-supporting material was revoked. |
| `IDENTITY_CONFLICTING` | Eligible claims or evidence conflict. |
| `IDENTITY_UNAVAILABLE` | Required identity provider or evidence is unavailable. |
| `IDENTITY_UNSUPPORTED` | The implementation cannot evaluate the required identity class or profile. |
| `IDENTITY_INDETERMINATE` | The result cannot be established more precisely. |
| `IDENTITY_RETIRED` | The identity is no longer eligible for ordinary future use under the declared lifecycle. |

These values describe BOOT-facing reliance.

They do not redefine ACS participant lifecycle.

## 12. Persisted identity and revalidation

### 12.1 Persisted identity is evidence

A persisted identity record is evidence of prior state.

It is not automatic proof of current:

- identity continuity;
- credential validity;
- authority;
- admission;
- ownership;
- security posture.

### 12.2 Required revalidation triggers

Revalidation MAY be required after:

- BOOT restart;
- platform reboot;
- recovery entry;
- storage restoration;
- credential rotation;
- key rotation;
- security-provider replacement;
- trust-domain crossing;
- long suspension;
- clock uncertainty;
- revocation-source unavailability;
- policy revision;
- authority revision;
- device replacement;
- host replacement;
- identity conflict;
- privilege increase;
- installer completion;
- runtime handoff;
- restored snapshots.

Sensitive operations SHOULD require fresher validation than non-mutating inspection.

### 12.3 Revalidation result

Revalidation MUST identify:

- subject;
- prior identity reference;
- current evidence;
- provider;
- assurance profile;
- freshness;
- revocation status;
- continuity result;
- unresolved conflict;
- resulting restrictions.

### 12.4 Unavailable revalidation

Unavailable revalidation MUST NOT be interpreted as successful revalidation.

The identity MAY remain historically known while current reliance is:

- restricted;
- deferred;
- denied;
- review-required;
- indeterminate.

### 12.5 Restored snapshots

Restored BOOT or identity snapshots MUST be evaluated for:

- integrity;
- schema support;
- provenance;
- revision continuity;
- subject continuity;
- supersession;
- revocation;
- freshness;
- conflicting newer state.

## 13. Enrollment tokens and provisioning evidence

### 13.1 Enrollment token is evidence

An enrollment token is evidence used to request, bind, issue, or validate identity.

It is not itself:

- the participant identity;
- a complete credential set;
- recovery authority;
- installer authority;
- ordinary admission.

### 13.2 Token possession

Possession of an enrollment token does not independently prove:

- rightful possession;
- intended subject;
- intended target;
- freshness;
- non-revocation;
- single use;
- authority scope.

### 13.3 Token use

An enrollment-token evaluation SHOULD consider:

- token identity;
- issuer;
- intended subject or subject class;
- intended device or substrate;
- permitted operation;
- validity;
- replay state;
- revocation state;
- consumption state;
- channel binding;
- applicable trust domain.

### 13.4 Replay and reuse

A consumed, expired, revoked, or replayed enrollment token MUST NOT regain eligibility merely because:

- BOOT restarted;
- the token was copied;
- another assistance source forwarded it;
- a fresh transport wrapper was used.

### 13.5 Provisioning result

Successful token evaluation may support issuance of:

- provisional identity;
- credential references;
- a bounded enrollment capability;
- an installer-provisioned identity record.

It does not automatically establish ACS admission or broader authority.

## 14. Identity continuity and re-enrollment

### 14.1 Continuity principle

Identity continuity describes whether the current subject represents the same logical participant as a prior subject.

Functional similarity does not prove continuity.

### 14.2 Continuity evidence

Continuity MAY be established through:

- current credentials;
- authorized credential replacement;
- signed continuity records;
- hardware-backed continuity;
- operator-approved re-enrollment;
- authorized migration records;
- recovery procedures;
- challenge response;
- ownership continuity.

### 14.3 Key loss

Loss of key material does not automatically require logical identity loss.

Authorized recovery or re-enrollment MAY issue new credentials while preserving identity when continuity is established.

### 14.4 Continuity failure

When continuity cannot be established:

- the prior identity MUST NOT be silently assumed;
- a new identity MAY be required;
- duplicated claims MUST remain conflicting;
- prior authority MUST NOT automatically transfer.

### 14.5 Re-enrollment audit

Sensitive re-enrollment SHOULD preserve bounded evidence of:

- requesting actor;
- authority source;
- prior identity;
- proposed identity;
- continuity evidence;
- credentials replaced;
- unresolved conflict;
- resulting restrictions.

## 15. Authentication

### 15.1 Security-provider ownership

Authentication primitives belong to the security provider.

BOOT coordinates authentication requests and consumes results.

BOOT MUST NOT:

- implement identity through address matching;
- declare a signature valid through an undeclared local guess;
- store raw long-term private keys in ordinary BOOT records;
- treat successful parsing as authentication;
- treat encryption as authentication;
- treat authentication as authority.

### 15.2 Authentication scope

An authentication result MUST identify:

- identity claim;
- subject;
- provider;
- operation or operation family;
- session or channel context where applicable;
- trust domain;
- assurance profile;
- freshness boundary;
- result revision.

### 15.3 Authentication result vocabulary

A BOOT-facing authentication result SHOULD support values equivalent to:

| Result | Meaning |
|---|---|
| `AUTH_NOT_EVALUATED` | No authentication evaluation exists. |
| `AUTH_PENDING` | Authentication is awaiting work or prerequisites. |
| `AUTH_CHALLENGE_REQUIRED` | Additional challenge evidence is required. |
| `AUTHENTICATED` | The identity evidence satisfied the declared assurance profile. |
| `AUTHENTICATED_WITH_LIMITATIONS` | Authentication succeeded with explicit material limitations. |
| `AUTH_REJECTED` | Evidence failed the applicable authentication contract. |
| `AUTH_EXPIRED` | Applicable authentication validity ended. |
| `AUTH_REVOKED` | Supporting credential or authentication state was revoked. |
| `AUTH_STALE` | Evidence exists but is outside the accepted freshness boundary. |
| `AUTH_CONFLICTING` | Eligible authentication results conflict. |
| `AUTH_UNAVAILABLE` | Required provider or evidence is inaccessible. |
| `AUTH_UNSUPPORTED` | The required authentication profile is unsupported. |
| `AUTH_INDETERMINATE` | Authentication cannot be conclusively established. |

### 15.4 Mutual authentication

Operations requiring strong identity assurance SHOULD authenticate all relevant participants.

Mutual authentication does not require identical methods.

### 15.5 Authentication freshness

Authentication evidence MUST be sufficiently current for the requested operation.

A previously authenticated secure session does not automatically authenticate:

- a new connection generation;
- a new operation;
- a new target;
- a new privilege scope;
- a new trust domain.

### 15.6 Authentication failure

Authentication failure MAY result in:

- challenge;
- denial;
- restricted public behavior;
- rate limiting;
- review;
- security or immune evidence.

It MUST NOT consume unbounded resources.

## 16. Authentication assurance

### 16.1 Assurance is multidimensional

Authentication assurance MAY include:

- identity-proof strength;
- credential-protection strength;
- challenge freshness;
- issuer recognition;
- hardware backing;
- continuity evidence;
- operator-presence evidence;
- revocation-check status;
- channel binding;
- trust-domain scope.

A high result in one dimension MUST NOT hide weakness in another.

### 16.2 No universal assurance score

A deployment MAY calculate assurance scores for a declared purpose.

A single global score MUST NOT be treated as:

- identity;
- authority;
- admission;
- universal reliability;
- unrestricted privilege.

### 16.3 Assurance profile

An assurance profile SHOULD identify:

- required evidence classes;
- freshness;
- provider requirements;
- hardware requirements where applicable;
- challenge requirements;
- revocation requirements;
- acceptable unavailable conditions;
- resulting restrictions.

Exact profiles remain deployment-defined.

## 17. Trust evidence and trust state

### 17.1 Trust is scoped

A trust evaluation MUST be interpreted relative to:

- subject;
- target;
- operation;
- relationship;
- trust domain;
- evidence boundary;
- time or revision;
- provider.

### 17.2 Trust is multidimensional

Relevant dimensions MAY include:

- identity confidence;
- credential integrity;
- protocol conformance;
- provenance quality;
- operational stability;
- reliability;
- mediation behavior;
- historical compliance;
- evidence independence;
- current security posture.

### 17.3 BOOT trust-state vocabulary

BOOT-facing trust state SHOULD support values equivalent to:

| State | Meaning |
|---|---|
| `TRUST_NOT_EVALUATED` | No applicable trust evaluation exists. |
| `TRUST_SUFFICIENT_FOR_SCOPE` | Evidence supports the declared reliance scope. |
| `TRUST_SUFFICIENT_WITH_RESTRICTIONS` | Evidence supports only a reduced reliance scope. |
| `TRUST_INSUFFICIENT` | Evidence does not support the requested reliance. |
| `TRUST_STALE` | Material evidence is outside its accepted freshness boundary. |
| `TRUST_CONFLICTING` | Eligible evidence supports incompatible interpretations. |
| `TRUST_UNAVAILABLE` | Required evidence or evaluator is unavailable. |
| `TRUST_UNKNOWN` | Available evidence is insufficient to decide. |

### 17.4 Trust does not transfer automatically

Trust in one operation does not automatically transfer to:

- another operation;
- another endpoint;
- another participant instance;
- another device;
- another operator;
- a delegate;
- a mediator;
- a transferring peer;
- an artifact.

### 17.5 Trust is not necessarily transitive

If A relies upon B and B relies upon C, A does not automatically rely upon C.

### 17.6 Trust evidence ages

Trust evidence may become stale.

Restoration to a stronger trust state is a new evaluation.

### 17.7 Repeated evidence

Repeated forwarding of one observation MUST NOT become independent corroboration.

## 18. Secure sessions and channel binding

### 18.1 Secure-session ownership

Secure-session establishment and cryptographic context belong to the security provider.

### 18.2 Secure-session identity

Secure-session identity MUST remain distinct from:

- participant identity;
- BOOT session identity;
- assistance-session identity;
- ACS connection identity;
- credential identity;
- authority identity.

### 18.3 Secure-session properties

A secure session MAY provide declared:

- peer authentication;
- confidentiality;
- integrity;
- message authenticity;
- freshness;
- replay protection;
- channel binding;
- security-profile negotiation.

Each property MUST be identified explicitly.

### 18.4 Secure session does not create authority

A secure session MUST NOT automatically create:

- bootstrap authority;
- recovery authority;
- installer authority;
- release authority;
- ACS admission;
- relationship activation.

### 18.5 Channel binding

Authority MAY be bound to:

- one secure session;
- one connection generation;
- one endpoint pair;
- one device state;
- one installer operation;
- one BOOT session;
- one boot attempt.

Moving the authority into another context may require:

- reauthentication;
- reissuance;
- renewed admission;
- capability replacement;
- renewed authority evaluation.

### 18.6 Downgrade resistance

Recovery, reconnect, fallback, or offline operation MUST NOT silently select a weaker security profile than required.

A weaker profile may be used only when:

- explicitly permitted;
- observable to affected components;
- authority scope is narrowed appropriately;
- admission is reevaluated where required.

## 19. Bootstrap-authority model

### 19.1 Bootstrap authority is operation-scoped

Bootstrap authority MUST identify:

- actor;
- target;
- operation;
- parameters;
- resource limits;
- validity;
- constraints;
- authority source;
- authority basis;
- revision;
- revocation path.

### 19.2 Bootstrap authority is externally grounded

BOOT MUST NOT establish bootstrap authority through:

- self-report;
- local execution;
- early execution;
- filesystem ownership;
- root operating-system access;
- physical access;
- emergency declaration;
- repeated request;
- prior success;
- session establishment.

### 19.3 Authority source and requester remain distinct

A recovery requester MUST NOT appoint itself as the authority source merely by naming itself in the request.

### 19.4 Authority evidence is not authority decision

Evidence that an actor may hold authority must still be evaluated through the applicable authority boundary.

### 19.5 Authority baseline

An implementation MAY identify operations that require no additional bootstrap-authority grant beyond BOOT’s declared local execution contract.

Examples may include:

- bounded non-mutating inspection;
- producing non-secret local diagnostics;
- refusing progression;
- requesting authority.

`AUTHORITY_NOT_REQUIRED` MUST be established by the owning contract.

It MUST NOT become a general escape from authority evaluation.

## 20. Authority-anchor categories

Public BOOT architecture recognizes candidate anchor categories without defining production anchors.

### 20.1 Operator-approved anchor

An operator-approved anchor derives from an authenticated operator acting within assigned authority.

It does not create universal operator privilege.

### 20.2 Installer-provisioned anchor

An installer-provisioned anchor is established through an eligible installer or provisioning process.

Installer possession of the target does not itself create the anchor.

### 20.3 Hardware-rooted anchor

A hardware-rooted anchor derives from security-provider evidence bound to hardware or secure execution state.

It does not automatically grant software, recovery, or installation authority.

### 20.4 Manufacturer-provisioned anchor

A manufacturer-provisioned anchor derives from an eligible manufacturer or platform provisioning path.

Manufacturer identity does not automatically govern unrelated Node policy.

### 20.5 Persisted-and-revalidated anchor

A persisted anchor may be reused only after applicable integrity, continuity, freshness, revocation, and policy revalidation.

### 20.6 Candidate anchor carriers

The following may carry candidate anchor evidence:

- installation media;
- removable tokens;
- local storage;
- secure hardware;
- operator devices;
- assistance sources;
- manufacturer records;
- recovery bundles.

The carrier is not the anchor merely because it possesses the evidence.

### 20.7 Anchor replacement

Trust-anchor or bootstrap-anchor replacement is a sensitive operation.

It MAY require:

- stronger authentication;
- concurrence;
- continuity evidence;
- operator presence;
- rollback preparation;
- post-change verification.

The exact policy remains private or deployment-specific.

## 21. Authority envelopes and capabilities

### 21.1 Authority envelope

A public authority envelope SHOULD identify:

```text
authority_identity
authority_revision
decision
authority_source_identity
authority_basis_reference
subject_identity
target_identity
operation_class
parameter_limits
resource_limits
validity_boundary
freshness_requirement
trust_domain
channel_binding
delegation_policy
concurrence_requirement
revocation_reference
audit_requirement
restrictions
```

### 21.2 Authority decisions

BOOT uses the BOOT-0002 decision vocabulary:

| Decision | Meaning |
|---|---|
| `NO_DECISION` | No authority decision exists. |
| `GRANTED` | The operation is authorized within its full declared envelope. |
| `GRANTED_NARROWED` | A reduced actor, target, action, scope, duration, or condition is authorized. |
| `DENIED` | The operation is not authorized. |
| `DEFERRED` | A final decision awaits additional evidence, authority, or review. |
| `EXPIRED` | Previously applicable authority is outside its validity period. |
| `REVOKED` | Previously applicable authority was withdrawn. |
| `CONFLICTING` | Eligible authority decisions conflict. |
| `UNAVAILABLE` | The required authority source or path is unavailable. |
| `INDETERMINATE` | Applicability cannot be established. |

### 21.3 Capability contents

A capability SHOULD identify or bind:

- capability identity;
- subject;
- issuer;
- target;
- operation;
- parameter limits;
- resource limits;
- validity;
- generation or epoch;
- delegation policy;
- concurrence requirements;
- channel binding;
- revocation path;
- audit requirements.

### 21.4 Narrow authority

Capabilities SHOULD grant the minimum authority required.

### 21.5 No ambient privilege

A participant MUST NOT gain privileged BOOT authority merely because it:

- runs locally;
- is process owner;
- has operating-system administrator access;
- hosts the installer;
- owns an endpoint;
- established a secure session;
- has strong trust evidence;
- possesses installation media;
- belongs to an infrastructure relationship.

### 21.6 Capability exercise

Exercising a capability remains subject to:

- authentication;
- current lifecycle;
- revocation;
- admission;
- resource availability;
- physical-safety ceilings;
- operation-specific validation.

## 22. Delegation

### 22.1 Delegable authority

Authority may be delegated only when the original authority explicitly permits delegation.

### 22.2 Delegation record

A delegation MUST identify:

- delegator;
- delegate;
- original authority;
- delegated operation;
- target;
- parameter limits;
- resource limits;
- validity;
- further-delegation permission;
- revocation path;
- audit requirements.

### 22.3 No amplification

Delegation MUST NOT expand:

- operation scope;
- target scope;
- parameter scope;
- resource scope;
- validity;
- priority ceiling;
- protected status;
- further-delegation rights.

### 22.4 Bounded depth

Delegation chains MUST remain bounded.

### 22.5 Delegation provenance

A verifier MUST be able to recover sufficient provenance to establish whether the current delegation is valid.

### 22.6 Parent expiry or revocation

A child delegation MUST NOT silently outlive the authority from which it was derived unless independently reissued through an eligible authority.

## 23. Concurrence

### 23.1 Sensitive operations

Sensitive BOOT operations MAY require concurrence.

Potential examples include:

- destructive installation on ambiguous storage;
- replacement of a root authority anchor;
- reassignment of durable participant identity;
- recovery from conflicting identity claims;
- release of protected recovery material;
- controlled propagation;
- permanent retirement;
- irreversible rollback decisions.

### 23.2 Concurrence record

Concurrence MUST define:

- participating authority categories;
- required scopes;
- independence requirements;
- validity window;
- cancellation behavior;
- conflict behavior;
- audit requirements.

### 23.3 Concurrence is not majority voting

Concurrence MUST NOT be assumed to mean simple majority voting.

### 23.4 Partial concurrence

Incomplete concurrence does not become full authorization.

It remains:

- pending;
- deferred;
- denied;
- expired;
- conflicting;
- unavailable

according to the applicable policy.

## 24. Authority intersection, conflict, and precedence

### 24.1 Authority intersection

Effective authority MUST be no broader than the intersection of all applicable hard constraints.

A broad grant does not override a specific restriction.

### 24.2 Authority conflict

A conflict exists when eligible-looking authorities produce incompatible requirements.

Examples include:

- one grants while another denies;
- two authorities claim incompatible ownership;
- an operator request conflicts with a physical-safety ceiling;
- one trust domain recognizes material another has revoked;
- two policy generations present incompatible grants.

### 24.3 Conflict remains explicit

Authority conflict MUST NOT be resolved solely by:

- arrival order;
- numerical priority;
- physical proximity;
- relationship strength;
- computational capacity;
- trust score;
- local preference;
- operator insistence;
- emergency declaration.

### 24.4 Public precedence rules

Public BOOT architecture establishes:

1. Hard physical-safety and constitutional ceilings cannot be overridden by ordinary grants.
2. Explicit applicable denial or restriction cannot be ignored because a broader grant exists.
3. Specific authority is applicable only when its issuer is authorized for that scope.
4. Conflict remains unresolved until a declared precedence or concurrence policy applies.
5. Absence of a precedence rule does not permit the most permissive outcome.
6. Privileged operations fail closed, defer, require review, or remain conflicting when authority cannot be established.

### 24.5 Conflict record

An authority-conflict record SHOULD identify:

- conflicting authority identities;
- subjects;
- targets;
- operations;
- revisions;
- applicable restrictions;
- unresolved precedence;
- resulting prohibited actions;
- review requirement.

## 25. Operator identity and authority

### 25.1 Explicit operator identity

Privileged operator actions MUST use an identity distinguishable from ordinary participant and service identities.

### 25.2 Intent resolution

Natural-language, graphical, physical, or remote operator input MUST be resolved into:

- operation;
- target;
- scope;
- parameters;
- authority requirement

before privileged execution.

### 25.3 No universal operator privilege

Authentication as an operator does not create unrestricted:

- installation authority;
- recovery authority;
- memory authority;
- key custody;
- release authority;
- propagation authority.

### 25.4 Operator presence

Sensitive operations MAY require current intentional operator presence.

Presence alone is not authorization.

### 25.5 Emergency operator actions

Emergency authority SHOULD favor narrow actions such as:

- safe stop;
- isolation;
- power reduction;
- rescue entry;
- bounded rollback;
- preservation of evidence.

Emergency authority MUST NOT silently become general administration.

## 26. Physical-safety boundary

No credential, capability, authority, operator role, secure session, or trust state may silently override hard physical-safety requirements.

BOOT and adjacent security mechanisms MUST preserve the ability to:

- reject unsafe directives;
- enter safe stop;
- isolate control paths;
- preserve critical safety communication;
- refuse destructive work;
- preserve evidence needed for review.

Physical-safety authority does not automatically grant broad software administration.

## 27. Revocation, expiry, and authorization caching

### 27.1 Revocable objects

The following MUST support revocation or equivalent invalidation where applicable:

- credentials;
- capabilities;
- delegations;
- secure sessions;
- trust assertions;
- admission grants;
- recovery grants;
- enrollment material;
- authority-anchor references.

### 27.2 Revocation scope

Revocation may target:

- one credential;
- one capability;
- one operation;
- one participant instance;
- one participant;
- one trust domain;
- one authority generation;
- one connection generation;
- one BOOT session.

### 27.3 Smallest effective scope

The smallest effective revocation scope SHOULD be preferred.

### 27.4 No voluntary dependency

Revocation MUST NOT depend upon cooperation by the revoked subject.

### 27.5 Cached authority

Authority decisions MAY be cached only within bounded scope.

Caches MUST account for:

- expiry;
- revocation;
- credential rotation;
- policy revision;
- authority revision;
- ownership change;
- connection-generation change;
- trust-domain change;
- lock-down;
- IMM restrictions processed through applicable authority.

### 27.6 Revocation uncertainty

Privileged operations MUST NOT continue indefinitely when revocation status cannot be established.

### 27.7 Restart does not restore authority

Expired, revoked, superseded, or stale authority MUST NOT regain validity because:

- BOOT restarted;
- the machine rebooted;
- the credential was reloaded;
- the same peer reconnected;
- the operation is urgent.

## 28. Offline and partitioned operation

### 28.1 Offline operation remains governed

Offline operation does not enlarge authority.

### 28.2 Existing grants

Existing authority MAY continue only within its declared:

- validity;
- offline profile;
- partition profile;
- resource limits;
- operation scope;
- revocation uncertainty policy.

### 28.3 New privileged authority

When required authority sources are unavailable, new privileged operations SHOULD:

- fail closed;
- defer;
- require operator review;
- remain unknown;
- use a separately authorized offline authority path.

### 28.4 Cached evidence

Cached identity or authority evidence MAY support bounded reduced operation only when explicitly permitted.

### 28.5 Clock unavailability

An unavailable trustworthy time source MUST remain explicit.

Implementations MAY use:

- monotonic sequence;
- challenge freshness;
- bounded boot counters;
- authority generations;
- secure counters;
- another eligible mechanism.

Clock unavailability MUST NOT automatically make stale authority current.

### 28.6 Post-partition reconciliation

After connectivity or provider availability returns, BOOT SHOULD reconcile:

- credential generations;
- authority generations;
- revocations;
- identity conflicts;
- delegations;
- secure-session state;
- enrollment consumption;
- cached grants.

## 29. First boot, installation media, and unattended installation

### 29.1 Substrate is not participant

A machine discovered by installation media is an observed substrate.

It is not automatically:

- a Node participant;
- an admitted ACS member;
- enrolled;
- owned by BOOT;
- eligible for destructive mutation.

### 29.2 Installation-media identity

Installation or rescue media may have:

- release identity;
- manifest identity;
- signer identity;
- build identity.

Those identities remain distinct from the target node’s identity.

### 29.3 Boot success

Successful image boot does not establish:

- image authenticity;
- release authority;
- target identity;
- target ownership;
- installation authority;
- storage eligibility.

### 29.4 Unattended does not mean ungoverned

Supported installation MAY proceed without interactive prompts when:

- identity requirements are satisfied;
- authority requirements are satisfied;
- target eligibility is unambiguous;
- required evidence is current;
- rollback and failure behavior are defined.

### 29.5 Ambiguous destructive targets

Ambiguous or protected storage MUST require:

- explicit authority;
- declared policy;
- or operator review

before destructive mutation.

### 29.6 Installer-provisioned identity

The installer MAY provision identity material only through an eligible authorized request.

Installer success does not establish that the provisioned identity is currently authenticated, admitted, or authorized for ordinary runtime use.

### 29.7 Post-installation revalidation

After installation or credential provisioning, BOOT or the next environment MUST revalidate applicable:

- identity;
- authority;
- generation;
- credential;
- continuation;
- admission requirements.

## 30. Dynamic self-assembly and source-bearing systems

### 30.1 Hardware evidence is not identity authority

Hardware inspection may inform:

- device identity evidence;
- compatibility;
- assembly planning.

It does not authorize enrollment or installation.

### 30.2 Source presence is not identity or authority

The presence of source, packages, manifests, or toolchains does not establish:

- source authority;
- release authority;
- compilation authority;
- execution authority;
- participant identity.

### 30.3 Private capability sources

Authentication to a private source may establish source-access eligibility.

It does not automatically authorize:

- local assembly;
- activation;
- propagation;
- disclosure to another node.

### 30.4 Assembly actor identity

The actor coordinating assembly, the installer, the release authority, the target node, and the runtime participant MUST remain distinguishable.

## 31. Release, artifact, and peer identities

### 31.1 Release authority is separate from peer identity

A peer may transfer a release without being the release authority.

### 31.2 Authenticated peer is not release authority

Successful peer authentication does not establish:

- artifact integrity;
- release approval;
- local compatibility;
- activation permission.

### 31.3 Repository identity

A repository location or branch name is not a release identity or authority source.

### 31.4 Signer identity

A valid signer identity establishes only the properties defined by the signing policy and provider result.

It does not automatically establish:

- local compatibility;
- installation authority;
- activation authority;
- propagation authority.

### 31.5 Restricted material

Protected source, packages, credentials, or modules MUST NOT propagate to a peer unless:

- the receiving identity is established;
- access authority is established;
- source classification permits transfer;
- local and remote policy requirements are met.

## 32. Recovery identity and security recovery

### 32.1 Recovery is restricted

Recovery credentials and roles SHOULD provide only authority required to:

- establish identity continuity;
- replace credentials;
- validate substrate state;
- reconcile revocation;
- restore ordinary security services;
- request an eligible installer or operator action.

### 32.2 Recovery identity is not ordinary identity authority

A recovery actor MUST NOT silently become:

- an ordinary production participant;
- a permanent operator;
- a universal security authority;
- a release authority.

### 32.3 Recovery evidence

Sensitive recovery SHOULD preserve:

- requesting actor;
- authority;
- prior identity state;
- proposed resulting state;
- credentials replaced;
- continuity evidence;
- unresolved conflicts;
- resulting restrictions.

### 32.4 New identity

When continuity cannot be established, creating a new identity MAY be safer than claiming restoration of the prior identity.

The decision requires applicable authority.

## 33. BOOT state-model integration

### 33.1 Identity records and state facets

Identity, authentication, trust, and authority records MUST use BOOT-0002 facets for:

- knowledge;
- availability;
- validity;
- freshness;
- progress;
- enforcement;
- resource state;
- outcome.

### 33.2 Authority dimension

Authority evaluation maps to `BOOT_DIM_AUTHORITY`.

The dimension phases remain:

- `AUTHORITY_NOT_REQUIRED`;
- `AUTHORITY_UNRESOLVED`;
- `AUTHORITY_EVIDENCE_PRESENT`;
- `AUTHORITY_EVALUATION_PENDING`;
- `AUTHORITY_EVALUATING`;
- `AUTHORITY_DECISION_AVAILABLE`;
- `AUTHORITY_REVIEW_PENDING`;
- `AUTHORITY_DIMENSION_CLOSED`.

BOOT-0003 does not replace those phases.

### 33.3 Assistance dimension

Authentication and admission phases in `BOOT_DIM_ASSISTANCE` remain separately observable.

Authentication success does not skip admission.

### 33.4 Lock-down

Identity or authority uncertainty MAY support a scoped restriction or lock-down only through applicable policy or authority.

Lock-down:

- is not proof of compromise;
- does not erase identity evidence;
- does not revoke unrelated grants automatically;
- must preserve applicable recovery and review paths.

### 33.5 Restored state

Restored identity and authority records MUST use `REVALIDATION_REQUIRED`, stale, unknown, conflicting, or another accurate state until current evaluation completes.

## 34. Public conceptual contracts

These contracts define semantics, not required wire formats or memory layouts.

### 34.1 Identity claim

```text
BootIdentityClaim
    claim_identity
    claim_revision
    claimed_subject_identity
    claimed_namespace
    claimant_identity
    claim_scope
    operation_identity
    session_identity
    evidence_references
    freshness_boundary
    trust_domain
```

### 34.2 Identity evidence reference

```text
BootIdentityEvidenceReference
    evidence_identity
    evidence_class
    subject_identity
    producer_identity
    issuer_identity
    provenance_reference
    integrity_state
    freshness_state
    revision
    correlation_identity
    classification
```

### 34.3 Authentication request

```text
BootAuthenticationRequest
    request_identity
    request_revision
    identity_claim_reference
    operation_scope
    assurance_profile_reference
    session_binding
    trust_domain
    bounded_challenge_requirements
    result_contract
```

### 34.4 Authentication result

```text
BootAuthenticationResult
    request_identity
    result_identity
    provider_identity
    authenticated_subject_identity
    result
    assurance_dimensions
    freshness_state
    credential_references
    revocation_state
    channel_binding
    restrictions
    bounded_issues
```

### 34.5 Trust evaluation

```text
BootTrustEvaluation
    evaluation_identity
    subject_identity
    target_identity
    operation_scope
    trust_domain
    evidence_references
    evaluation_state
    restrictions
    freshness_state
    evaluator_identity
    bounded_issues
```

### 34.6 Authority evidence

```text
BootAuthorityEvidence
    evidence_identity
    claimed_authority_source
    authority_basis_reference
    actor_identity
    target_identity
    operation_class
    scope
    revision
    freshness_state
    revocation_state
    provider_identity
```

### 34.7 Authority envelope

```text
BootAuthorityEnvelope
    authority_identity
    authority_revision
    decision
    authority_source_identity
    authority_basis_reference
    actor_identity
    target_identity
    operation_class
    parameter_limits
    resource_limits
    validity_boundary
    trust_domain
    channel_binding
    delegation_policy
    concurrence_requirement
    revocation_reference
    restrictions
    audit_requirement
```

### 34.8 Authority conflict

```text
BootAuthorityConflict
    conflict_identity
    subject_identity
    target_identity
    operation_class
    conflicting_authority_references
    conflicting_revisions
    applicable_restrictions
    precedence_state
    prohibited_actions
    review_requirement
```

### 34.9 Revalidation requirement

```text
BootRevalidationRequirement
    requirement_identity
    subject_identity
    prior_result_reference
    trigger
    required_provider
    required_assurance
    freshness_requirement
    revocation_requirement
    permitted_interim_scope
```

### 34.10 Enrollment evidence

```text
BootEnrollmentEvidence
    evidence_identity
    issuer_identity
    intended_subject
    intended_target
    operation_scope
    validity_boundary
    replay_state
    revocation_state
    consumption_state
    channel_binding
```

No conceptual record contains raw private-key material or reusable recovery secrets.

## 35. Boundedness and resource exhaustion

Identity and authority mechanisms MUST remain bounded.

Implementations MUST define finite limits for:

- identity claims per session;
- evidence references per claim;
- authentication attempts;
- challenge rounds;
- concurrent authentication operations;
- anchor candidates;
- delegation depth;
- concurrence participants;
- revocation checks;
- cached grants;
- authority conflicts;
- public issue records;
- audit references;
- diagnostic text.

### 35.1 Flood resistance

BOOT and adjacent providers MUST defend against:

- identity-claim flooding;
- authentication flooding;
- challenge flooding;
- capability probing;
- expensive credential validation;
- revocation-check exhaustion;
- replay floods;
- audit amplification.

### 35.2 Protected capacity

Implementations SHOULD preserve bounded capacity for:

- revocation;
- session closure;
- authority withdrawal;
- security-state reporting;
- critical authentication;
- recovery review.

### 35.3 Resource failure

Resource exhaustion MUST:

- remain explicit;
- leave authoritative state unchanged when required preparation failed;
- avoid partial authority grants;
- avoid silent evidence truncation;
- preserve prior revisions.

## 36. Conservative atomicity

Before committing authoritative identity, authentication, or authority state, an implementation MUST complete all applicable:

- claim validation;
- namespace validation;
- provider validation;
- revision validation;
- evidence-reference construction;
- freshness evaluation;
- revocation evaluation;
- scope validation;
- authority-envelope construction;
- conflict detection;
- bounded issue retention;
- result preparation;
- idempotency preparation.

If required preparation fails:

- authoritative state remains unchanged;
- no revision advances;
- no partial grant is committed;
- no favorable result is reported;
- uncertainty remains explicit.

### 36.1 Multi-part authority

A concurrence result MUST commit all required authority decisions as one coherent result or remain incomplete.

### 36.2 Duplicate requests

Repeated delivery of an authentication or authority request MUST NOT:

- create new authority;
- count as independent evidence;
- consume an enrollment token twice;
- advance revisions without material change.

## 37. Revocation and revalidation ordering

Where both revocation and renewal evidence exist:

1. applicable revisions MUST be compared;
2. scope and issuer authority MUST be evaluated;
3. conflict MUST remain explicit;
4. a newer-looking timestamp MUST NOT decide by itself;
5. ordinary progression MUST remain restricted until the owning policy resolves applicability.

A fresh wrapper MUST NOT revive stale or revoked enclosed authority.

## 38. Adjacent-system boundaries

### 38.1 ACS

ACS owns:

- logical participant identity;
- endpoint identity;
- relationships;
- admission;
- capability and authority semantics;
- connection lifecycle;
- revision and idempotency semantics.

BOOT consumes and preserves ACS results.

### 38.2 Security provider

The security provider owns:

- cryptographic authentication;
- credential validation;
- signature and digest verification;
- secure-session establishment;
- key custody;
- challenge generation;
- replay-protection primitives.

BOOT MUST use references or protected handles rather than raw keys.

### 38.3 Installer

The installer may provision durable identity material only through an authorized installer request.

BOOT remains the coordinator, not the credential issuer or durable mutator unless a separately defined provider role is explicitly assigned.

### 38.4 Transport

Transport provides reachability and byte movement.

Transport success does not create identity, trust, admission, or authority.

### 38.5 IMM

IMM may provide:

- identity-anomaly evidence;
- credential-misuse evidence;
- replay evidence;
- recovery recommendations;
- requests for revalidation.

IMM cannot:

- invent credentials;
- issue itself authority;
- select itself as bootstrap authority;
- revoke unrelated identities without applicable authority;
- expose private scoring through public BOOT records.

### 38.6 MEM

MEM governs:

- persistence;
- custody;
- retention;
- reconstruction;
- restoration;
- deletion.

Persisting an identity claim does not make it true.

Recovering an authority record does not make it current.

### 38.7 Resource management

Resource availability does not create authority.

Security or authority evaluation MUST remain within declared resource limits.

### 38.8 Normal runtime

Runtime startup does not:

- validate BOOT identity retroactively;
- restore expired authority;
- create MEM authority;
- create IMM authority;
- create ACS admission.

The runtime must establish its own applicable identity and admission state.

### 38.9 Physical safety

No BOOT or ACS authority silently overrides hard physical-safety ceilings.

### 38.10 Future OS and update architecture

Future OS and update specifications own:

- release manifests;
- release authority policy;
- source distribution;
- update channels;
- peer propagation;
- system-generation activation.

They MUST preserve the identity and authority separations defined here.

## 39. Public/private classification boundary

### 39.1 Public material

Public BOOT architecture MAY define:

- identity layers;
- identity-profile semantics;
- claim and evidence contracts;
- authentication-result categories;
- trust-state categories;
- authority envelopes;
- authority decisions;
- delegation and concurrence rules;
- revocation requirements;
- public conformance tests;
- failure behavior;
- public conceptual records.

### 39.2 Private material

Public BOOT documentation MUST NOT disclose:

- production credentials;
- private keys;
- recovery secrets;
- production trust anchors;
- infrastructure addresses;
- private repository addresses;
- private identity catalogs;
- private credential-issuer lists;
- trust thresholds;
- trust-scoring algorithms;
- private responder preferences;
- operator recovery playbooks;
- production concurrence rules;
- production authority precedence;
- private enrollment procedures;
- private release-signing policy;
- private source-access policy;
- protected module identities where disclosure is restricted.

### 39.3 Independent public implementation

Public contracts MUST remain sufficient to implement:

- identity claims;
- provider adapters;
- authentication results;
- trust-state handling;
- authority decisions;
- conflict handling;
- mock security providers;
- conformance tests.

A public implementation MUST remain functional without private modules.

## 40. Security and privacy requirements

BOOT identity and authority records MUST:

- avoid raw private keys;
- avoid reusable secrets;
- minimize pre-authentication disclosure;
- use protected references where appropriate;
- bound human-readable detail;
- preserve classification;
- preserve subject and issuer separation;
- avoid exposing unnecessary topology;
- prevent unauthorized propagation;
- preserve revocation and conflict state.

### 40.1 Credential minimization

Credentials SHOULD disclose only information required for their purpose.

### 40.2 Key custody

Owning a node, endpoint, service, or installer does not automatically grant key custody.

A key custodian may be permitted to perform cryptographic operations without exporting key material.

### 40.3 Audit privacy

Audit records MUST NOT expose:

- private keys;
- reusable tokens;
- unnecessary payload content;
- unnecessary cognitive content;
- hidden trust reasoning.

## 41. Conformance expectations

### 41.1 Architectural conformance

An architecture conforms when it:

- preserves every foundational distinction;
- keeps BOOT identity profiles separate from ACS identity lifecycle;
- uses provider-attributed authentication;
- preserves scoped trust evidence;
- represents operation-scoped authority;
- prevents self-granted authority;
- preserves revocation, expiry, and conflict;
- prevents location-derived identity;
- prevents token-derived identity;
- prevents rooted-evidence-derived universal authority;
- preserves offline restrictions;
- keeps release authority separate from transferring peers.

### 41.2 Implementation conformance

An implementation conforms when observable behavior demonstrates those properties under:

- first boot;
- rescue;
- persisted identity;
- cloned storage;
- stale credentials;
- revoked credentials;
- unavailable providers;
- conflicting identity claims;
- operator presence;
- offline operation;
- secure-session establishment;
- assistance-source discovery;
- enrollment-token replay;
- delegation;
- concurrence;
- authority conflict;
- revocation;
- resource exhaustion;
- restart;
- re-enrollment.

### 41.3 Required negative tests

Conformance evidence SHOULD include:

- session identity cannot become participant identity;
- address and hostname cannot establish continuity;
- first responder is not automatically selected;
- enrollment token cannot be used as identity;
- replayed enrollment evidence is rejected or remains unresolved;
- hardware-rooted evidence grants no automatic installer authority;
- persisted identity requires revalidation;
- copied credentials do not create a second legitimate identity;
- authentication does not create authority;
- secure session does not create admission;
- strong trust evidence does not create permission;
- broad grant does not override specific restriction;
- expired grant does not revive after reboot;
- revoked grant does not revive through retransmission;
- delegation cannot amplify scope;
- incomplete concurrence does not authorize;
- operator authentication does not create universal privilege;
- offline revocation uncertainty restricts privileged operation;
- authenticated peer does not create release authority;
- private material refuses unauthorized propagation;
- resource exhaustion does not create partial authority state.

### 41.4 Mock-provider conformance

A mock security provider MUST support truthful:

- authenticated;
- rejected;
- challenged;
- stale;
- expired;
- revoked;
- conflicting;
- unavailable;
- unsupported;
- indeterminate

results.

A mock MUST NOT return favorable results unavailable to the real provider contract.

### 41.5 Deployment policy

Deployment policy may define:

- accepted issuers;
- concrete anchors;
- assurance profiles;
- credential lifetimes;
- concurrence;
- operator roles;
- offline profiles;
- release authorities.

Policy MUST NOT redefine public semantics.

## 42. BOOT-P2 implementation proof

BOOT-P2 should demonstrate a bounded identity and assistance proof.

A suitable proof includes:

1. create a new BOOT session with `BOOT_EPHEMERAL`;
2. inspect for persisted or rooted identity evidence;
3. represent missing evidence explicitly;
4. obtain bounded provisional enrollment evidence from a mock provider;
5. authenticate through the mock security provider;
6. preserve authentication separately from authority;
7. request ACS admission separately;
8. establish a bounded assistance session;
9. minimize pre-authentication disclosure;
10. reject token replay;
11. reject an address-only identity claim;
12. reject a rooted-identity claim as automatic recovery authority;
13. produce structured final identity and authority state.

BOOT-P2 need not define production credentials or production trust anchors.

## 43. Initial implementation implications

### 43.1 Language-neutral values

Early implementation will require bounded values equivalent to:

- `BootIdentityClaim`;
- `BootIdentityEvidenceReference`;
- `BootAuthenticationRequest`;
- `BootAuthenticationResult`;
- `BootTrustEvaluation`;
- `BootAuthorityEvidence`;
- `BootAuthorityEnvelope`;
- `BootAuthorityConflict`;
- `BootRevalidationRequirement`;
- `BootEnrollmentEvidence`.

### 43.2 Provider handles

Public boundaries should use:

- provider identities;
- credential references;
- protected key handles;
- capability references;
- authority references;
- evidence references.

They SHOULD NOT transfer raw secret material.

### 43.3 Security-provider mock

The first provider may be a deterministic test implementation suitable for:

- QEMU;
- malformed-input testing;
- replay testing;
- expiry testing;
- revocation testing;
- conflict testing;
- unavailable-provider testing.

The test provider is not production security.

### 43.4 Bounded persistence

Minimal BOOT persistence may retain:

- prior identity references;
- credential generations;
- enrollment consumption markers;
- last authority revision;
- revocation references;
- revalidation requirements;
- unresolved conflicts.

Such persistence remains distinct from MEM semantic authority.

## 44. Relationship to later BOOT specifications

### BOOT-0004 — Discovery and Assistance Negotiation

BOOT-0004 will define:

- discovery;
- candidate offers;
- assistance-session identity;
- responder eligibility;
- pre-authentication disclosure;
- negotiation;
- transport-neutral operations.

It MUST preserve BOOT-0003 identity and authority boundaries.

### BOOT-0005 — Recovery Plans and Artifact Boundaries

BOOT-0005 will define:

- plan authority references;
- artifact signer and release identity;
- installer authority inputs;
- source and artifact eligibility.

It MUST not treat authentication or verification as operation authority.

### BOOT-0006 — Minimal Runtime and Language Constraints

BOOT-0006 will define measurable:

- identity-record bounds;
- evidence-reference limits;
- challenge limits;
- provider dependencies;
- memory requirements;
- parsing constraints.

### BOOT-0007 — ACS, IMM, and MEM Integration

BOOT-0007 will map:

- BOOT identity profiles to ACS concepts;
- ACS capability and admission results;
- IMM evidence;
- MEM persistence;
- runtime handoff identity.

### BOOT-0008 — Failure, Retry, and Operator Intervention

BOOT-0008 will refine:

- authentication retry;
- lockout;
- identity reconciliation;
- authority renewal;
- operator review;
- offline recovery;
- conflict intervention.

### BOOT-0009 — Public/Private Boundary and Implementation Roadmap

BOOT-0009 will refine publication rules and implementation checkpoints for identity, trust, and authority.

## 45. Open architectural decisions

The following remain intentionally unresolved:

- concrete participant-identity representation;
- concrete device-identity representation;
- concrete identity namespaces;
- identifier widths;
- credential formats;
- credential issuers;
- authentication protocols;
- assurance profiles;
- hardware-root mechanisms;
- manufacturer-provisioning mechanisms;
- secure-session protocols;
- channel-binding mechanisms;
- trust-state calculation;
- trust thresholds;
- authority-anchor formats;
- capability formats;
- delegation-depth limits;
- concurrence policy;
- operator-authentication methods;
- operator-presence evidence;
- recovery credentials;
- enrollment-token formats;
- enrollment-token consumption storage;
- revocation distribution;
- authorization-cache duration;
- offline authority profiles;
- clock and secure-time mechanisms;
- trust-domain federation;
- public release-authority contracts;
- production identity recovery;
- production authority assignments.

These open decisions do not authorize weaker behavior.

## 46. BOOT-0001 traceability registry

| BOOT-0001 invariant | BOOT-0003 coverage |
|---|---|
| BOOT-INV-002 — Execution context grants no authority | Sections 6, 19, 25, 28, and 29 |
| BOOT-INV-003 — Adjacent ownership preserved | Sections 15, 18, and 38 |
| BOOT-INV-004 — Stage separation | Sections 6, 15, 18, 21, and 31 |
| BOOT-INV-005 — Session identity separation | Sections 6, 8, and 9 |
| BOOT-INV-006 — Identity is not location | Sections 6, 10, and 41 |
| BOOT-INV-007 — Identity evidence revalidation | Sections 9 through 14 and 27 |
| BOOT-INV-008 — No self-granted bootstrap authority | Sections 19 through 25 |
| BOOT-INV-010 — Unknown remains explicit | Sections 11 through 17 and 24 |
| BOOT-INV-016 — Responsiveness is not authority | Sections 6 and 24 |
| BOOT-INV-017 — Transport is not semantic acceptance | Sections 18 and 38 |
| BOOT-INV-018 — Bounded assistance sessions | Sections 18, 35, and 42 |
| BOOT-INV-019 — Plans do not create authority | Sections 19 and 21 |
| BOOT-INV-024 — Image and discovery grant no authority | Section 29 |
| BOOT-INV-025 — Governed self-assembly | Section 30 |
| BOOT-INV-027 — Update availability grants no authority | Section 31 |
| BOOT-INV-028 — Governed peer propagation | Sections 31 and 39 |
| BOOT-INV-029 — Public/private separation | Sections 39 and 40 |
| BOOT-INV-030 — Handoff is not broader authority | Sections 38 and 44 |

## 47. BOOT-0002 integration registry

| BOOT-0002 concept | BOOT-0003 use |
|---|---|
| `BOOT_DIM_AUTHORITY` | Carries operation-scoped bootstrap-authority evaluation |
| `AUTHORITY_EVIDENCE_PRESENT` | Candidate anchor or authority evidence exists |
| `AUTHORITY_EVALUATION_PENDING` | Provider, revalidation, or concurrence remains pending |
| `AUTHORITY_DECISION_AVAILABLE` | A scoped authority decision exists |
| `AUTHORITY_REVIEW_PENDING` | Conflict, expiry, revocation, or ambiguity requires review |
| `GRANTED` | Full requested authority envelope approved |
| `GRANTED_NARROWED` | Reduced authority envelope approved |
| `DENIED` | Applicable authority denied the operation |
| `DEFERRED` | Decision postponed |
| `EXPIRED` | Prior authority ended |
| `REVOKED` | Prior authority withdrawn |
| `CONFLICTING` | Eligible authority results conflict |
| `UNAVAILABLE` | Authority source or path inaccessible |
| `INDETERMINATE` | Applicability cannot be established |
| Knowledge facet | Represents missing, partial, established, or conflicting evidence |
| Availability facet | Represents provider and evidence accessibility |
| Validity facet | Represents provider-scoped identity or authority evaluation |
| Freshness facet | Represents current, stale, expired, or unresolved status |
| Enforcement facet | Represents open, restricted, or locked-down progression |
| Resource facet | Represents bounded authentication and authority capacity |

## 48. Identity, trust, and authority distinction registry

| Concept | Establishes | Does not establish | Primary owner |
|---|---|---|---|
| BOOT session identity | Session correlation | Participant identity or authority | BOOT |
| Device identity | Device continuity evidence | Participant identity | ACS/security provider boundary |
| Participant identity | Logical subject | Credential validity or authority | ACS |
| Credential | Evidence | Identity acceptance or authority by itself | Security provider/issuer |
| Authentication | Scoped identity-evidence result | Authorization or admission | Security provider |
| Trust evidence | Scoped reliance evidence | Permission or truth | ACS/policy evaluator |
| Secure session | Declared communication-security properties | Relationship, admission, or authority | Security provider |
| Authority evidence | Basis for authority evaluation | Final decision | Authority provider |
| Authority decision | Bounded permission or denial | Admission, execution, or success | Applicable authority |
| Capability | Bounded operation permission | Identity or completion | ACS/security authority |
| Admission | Permission to consume current connection or operation capacity | Broader authority | ACS/admission owner |
| Installer acceptance | Installer agrees to consider work | Mutation completion | Installer |
| Release identity | Identified release | Local activation authority | Future update architecture |
| Peer identity | Identity of transferring peer | Release authority | ACS/security provider |
| Operator identity | Authenticated operator actor | Universal control | Operator authority system |

## 49. Prohibited interpretations

This specification MUST NOT be interpreted to mean that:

- a BOOT session is a durable participant;
- an address is identity;
- a hostname proves continuity;
- a disk path proves ownership;
- copied credentials create legitimate identity;
- installation media is authoritative through physical presence;
- an enrollment token is identity;
- an enrollment token grants recovery authority;
- `BOOT_PROVISIONAL` grants ordinary mesh membership;
- `BOOT_ROOTED` grants unrestricted authority;
- hardware identity proves software validity;
- authentication grants authorization;
- authorization grants admission;
- secure session grants relationship activation;
- trust state grants permission;
- high trust overrides revocation;
- operator identity grants universal control;
- emergency status restores expired authority;
- local root access creates bootstrap authority;
- first response creates responder eligibility;
- repeated evidence creates independent corroboration;
- repeated requests create authority;
- broad authority overrides a specific restriction;
- incomplete concurrence authorizes a sensitive action;
- reboot restores revoked credentials;
- offline operation permits indefinite cached authority;
- authenticated peer becomes release authority;
- signature verification creates local activation authority;
- private capability possession grants propagation authority;
- IMM evidence directly revokes identity;
- persisted identity automatically becomes current;
- runtime success validates prior identity retroactively.

## 50. Completion checklist

- [x] BOOT-0000 authority is preserved.
- [x] Approved BOOT-0001 invariants are preserved.
- [x] BOOT-0002 authority-state semantics are preserved.
- [x] ACS remains authoritative for participant identity and admission.
- [x] Security providers retain cryptographic operations and key custody.
- [x] BOOT session identity remains separate from durable identity.
- [x] `BOOT_EPHEMERAL`, `BOOT_PROVISIONAL`, and `BOOT_ROOTED` are defined.
- [x] Enrollment tokens remain evidence rather than identity.
- [x] Persisted identity requires revalidation.
- [x] Hardware-rooted evidence does not create unrestricted authority.
- [x] Authentication remains separate from authority.
- [x] Authority remains separate from admission.
- [x] Trust remains scoped and non-authorizing.
- [x] Secure sessions remain separate from relationships and assistance sessions.
- [x] Bootstrap authority is externally grounded and operation-scoped.
- [x] Candidate anchor categories are defined without exposing production anchors.
- [x] Delegation cannot amplify authority.
- [x] Concurrence remains explicit and is not assumed to be majority voting.
- [x] Authority conflict has no permissive default.
- [x] Operator identity grants no universal privilege.
- [x] Revocation, expiry, and stale caches remain explicit.
- [x] Offline operation does not enlarge authority.
- [x] First boot and unattended installation remain governed.
- [x] Peer identity remains separate from release authority.
- [x] Public/private boundaries are preserved.
- [x] No production credentials, trust topology, anchors, or private policy are exposed.
- [x] Public contracts remain independently implementable and testable.
- [x] No cryptographic suite, credential format, hardware root, or provider is mandated.
- [x] BOOT-P2 has a defined architecture basis.
- [x] Detailed discovery and assistance negotiation remain with BOOT-0004.
- [x] No blocking contradiction was identified.

## 51. Closing principle

> **BOOT may rely only on identity, trust evidence, and authority that remain explicit in subject, source, scope, revision, freshness, and limitation; everything else remains a claim.**

## Revision history

### Version 0.1 — 2026-07-18

- Defined BOOT-facing identity layers and namespaces.
- Defined `BOOT_EPHEMERAL`, `BOOT_PROVISIONAL`, and `BOOT_ROOTED`.
- Defined identity claims, evidence, persisted identity, enrollment evidence, and revalidation.
- Defined authentication and multidimensional assurance.
- Defined scoped trust evidence and trust-state interpretation.
- Defined secure-session and channel-binding boundaries.
- Defined bootstrap authority, candidate anchor categories, authority envelopes, and capabilities.
- Defined delegation, concurrence, authority intersection, conflict, and precedence.
- Defined operator, physical-safety, revocation, offline, and recovery behavior.
- Defined first-boot, unattended-installation, self-assembly, release, and peer-identity boundaries.
- Defined public conceptual contracts and BOOT-P2 implementation implications.
- Preserved ACS, security-provider, installer, IMM, MEM, resource, runtime, OS, and update ownership boundaries.
