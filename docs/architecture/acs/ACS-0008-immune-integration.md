# ACS-0008: Immune Integration

| Field | Value |
|---|---|
| Specification | ACS-0008 |
| Title | Immune Integration |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ACS-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Pending review |
| Depends on | ACS-0000 through ACS-0007 |
| Related specifications | ACS-0009, MEM-0000 through MEM-0010, future IMM specification series |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in authority separation, evidence handling, and bounded enforcement integration; automated containment thresholds, distributed immune coordination, and restoration policy remain under review |

> **The immune system may observe, assess, recommend, and—when explicitly authorized—invoke bounded ACS restrictions, but it shall not invent authority, redefine communication semantics, or become the owner of the systems it protects.**

## Architectural-intent notice

This specification defines the public boundary between the Adaptive Connection Substrate and Node immune architecture.

It is independently authored public architecture.

It is not produced by deleting, sanitizing, paraphrasing, or selectively redacting restricted architecture.

This document defines the public integration concepts and guarantees required for independently implementable Node-compatible immune participation without disclosing:

- production immune heuristics;
- private threat models;
- proprietary anomaly-detection algorithms;
- production containment thresholds;
- private topology;
- restricted operator procedures;
- production trust roots;
- private cognitive monitoring policy;
- internal evidence-correlation rules;
- production quarantine procedures;
- protected endpoint catalogs;
- production immune deployment strategy.

This specification does not define:

- the complete public IMM architecture;
- one immune detection algorithm;
- one anomaly-scoring formula;
- one containment policy;
- one quarantine implementation;
- one distributed immune-consensus mechanism;
- one forensic storage format;
- one production response playbook;
- private evidence thresholds;
- proprietary cognitive-health models;
- physical destruction or remediation procedures.

Those subjects belong to the future IMM specification series, operator-control specifications, runtime architecture, security policy, and implementation profiles.

ACS-0008 defines only how immune systems interact with ACS-defined identity, relationships, connections, endpoints, ports, signals, admission, budgets, lifecycle, security, trust, and authority.

## 1. Purpose

Node requires mechanisms capable of detecting and responding to conditions such as:

- identity inconsistency;
- credential misuse;
- repeated unauthorized operations;
- replay;
- malformed or hostile traffic;
- ignored backpressure;
- resource exhaustion;
- abnormal connection churn;
- stale-instance activity;
- mediation anomalies;
- provenance corruption;
- conflicting authority claims;
- unusual payload behavior;
- persistent contract violations;
- suspicious recovery attempts;
- potentially compromised participants;
- systemic degradation.

Those mechanisms must not be allowed to become an undefined super-administrator.

Without a clear ACS–immune boundary, an immune implementation may incorrectly assume that:

- suspicion grants quarantine authority;
- observation grants payload access;
- anomaly scoring grants credential revocation;
- communication failure proves compromise;
- resource pressure proves attack;
- health degradation proves malicious intent;
- endpoint visibility grants inspection authority;
- evidence retention grants memory ownership;
- containment grants lifecycle ownership;
- physical access grants participant authority;
- emergency status permits unrestricted action;
- an immune relationship bypasses ACS admission;
- an immune service may invent new connection states or resource controls.

ACS-0008 prevents those failures by defining:

- what immune systems may observe;
- how immune evidence enters ACS;
- which ACS actions may be requested;
- which actions require explicit capabilities;
- how restrictions remain bounded and reversible;
- how containment preserves recovery and evidence;
- how uncertainty and conflict remain visible;
- how immune systems remain subject to the same security, admission, lifecycle, and resource rules as other participants.

## 2. Scope

This specification governs:

- immune participants in ACS;
- immune relationship use;
- immune-facing endpoints and ports;
- immune observation;
- immune evidence;
- evidence provenance;
- evidence confidence;
- evidence freshness;
- evidence independence;
- evidence correlation boundaries;
- immune assessments;
- immune recommendations;
- immune requests;
- immune challenges;
- immune-triggered admission restrictions;
- immune-triggered connection restrictions;
- attachment restriction;
- budget reduction;
- mandatory mediation;
- directive suppression;
- suspension;
- quarantine integration;
- containment integration;
- credential and capability-review requests;
- recovery and restoration integration;
- escalation;
- concurrence;
- audit;
- privacy;
- degraded and partitioned operation;
- immune-system self-protection;
- failure of immune services;
- interaction with MEM;
- public implementation and conformance requirements.

This specification does not define:

- immune cognition;
- biological analogies as implementation requirements;
- production threat classification;
- private anomaly models;
- production evidence thresholds;
- exact quarantine duration;
- physical-node destruction;
- memory deletion policy;
- operator emergency policy;
- complete forensic procedure;
- one universal incident taxonomy;
- one public IMM port catalog;
- proprietary response coordination.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements.

The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification.

The word **may** describes permitted behavior.

An implementation does not conform merely because it labels one service as `immune`, `security`, `monitor`, or `watchdog`.

Its observable behavior must preserve the authority, evidence, containment, privacy, and restoration distinctions defined here.

## 4. Foundational distinctions

### 4.1 Observation is not authority

The ability to observe an interaction does not grant permission to:

- alter it;
- block it;
- inspect every payload;
- revoke credentials;
- suspend participants;
- retire relationships;
- destroy state.

### 4.2 Evidence is not proof

Immune evidence may indicate:

- anomaly;
- inconsistency;
- policy violation;
- elevated risk;
- possible compromise.

Evidence does not automatically establish:

- malicious intent;
- confirmed compromise;
- root cause;
- required response.

### 4.3 Assessment is not decision authority

An immune assessment interprets evidence.

It does not automatically authorize an ACS lifecycle or security transition.

### 4.4 Recommendation is not directive

An immune recommendation proposes a bounded action.

It does not compel execution.

### 4.5 Request is not successful containment

An admitted containment request may still be:

- denied;
- narrowed;
- deferred;
- challenged;
- partially executed;
- unsuccessful;
- indeterminate.

### 4.6 Immune relationship is not unrestricted access

An immune relationship class defines why the association exists.

It does not independently grant:

- complete traffic visibility;
- payload access;
- credential access;
- memory access;
- directive authority;
- lifecycle control.

### 4.7 Security authority is not immune authority

Security systems govern identity, credentials, capabilities, sessions, and revocation.

Immune systems may produce evidence or requests affecting those systems.

They do not absorb security authority.

### 4.8 Lifecycle authority is not immune authority

Immune systems may request lifecycle restriction.

They do not automatically own connection, endpoint, relationship, or participant lifecycle.

### 4.9 Health failure is not compromise

Failure, degradation, overload, or unavailability may result from:

- hardware faults;
- software defects;
- network partitions;
- configuration errors;
- resource exhaustion;
- incompatible versions;
- attack;
- unknown causes.

### 4.10 Resource abuse evidence is not proof of intent

Excessive use may be:

- legitimate demand;
- faulty behavior;
- implementation mismatch;
- accidental retry storm;
- malicious activity;
- unknown.

### 4.11 Containment is not punishment

Containment exists to reduce risk, preserve evidence, and enable recovery.

It is not a mechanism for irreversible retaliation.

### 4.12 Quarantine is not retirement

A quarantined participant or connection remains logically identifiable unless separately retired through authorized lifecycle policy.

### 4.13 Isolation is not destruction

Restricting communication does not authorize destruction of:

- physical hardware;
- memory;
- credentials;
- audit records;
- participant identity.

### 4.14 Immune evidence is not memory ownership

Evidence retained through MEM remains governed memory.

The immune system does not become its universal owner or custodian merely because the content is security-relevant.

### 4.15 Immune protection does not remove immune accountability

Immune participants remain subject to:

- authentication;
- authorization;
- admission;
- resource budgets;
- lifecycle;
- audit;
- revocation;
- immune observation where appropriate.

## 5. Core terminology

### 5.1 Immune participant

An immune participant is a logical ACS participant assigned one or more bounded immune roles.

### 5.2 Immune role

An immune role is a declared responsibility such as:

- observation;
- evidence collection;
- evidence validation;
- correlation;
- assessment;
- challenge;
- recommendation;
- containment coordination;
- restoration verification.

Roles do not automatically combine into universal authority.

### 5.3 Immune observation

An immune observation is a bounded record of behavior, state, or evidence relevant to possible risk or abnormality.

### 5.4 Immune evidence

Immune evidence is information eligible for assessment under an immune operation contract.

### 5.5 Immune assessment

An immune assessment is a reasoned interpretation of evidence within a declared scope and confidence boundary.

### 5.6 Immune finding

An immune finding is a declared assessment outcome such as:

- normal within observed scope;
- anomalous;
- policy-violating;
- suspicious;
- likely compromised;
- confirmed compromised;
- unresolved;
- conflicting;
- unknown.

The future IMM series may refine finding categories.

### 5.7 Immune recommendation

An immune recommendation proposes an action without claiming authority to execute it.

### 5.8 Immune request

An immune request asks an ACS, security, lifecycle, runtime, operator, or domain authority to consider an action.

### 5.9 Immune directive

An immune directive is an authorized operation invocation issued under an explicit capability.

Immune directives are not implied by observations or findings.

### 5.10 Containment

Containment is a bounded set of restrictions intended to reduce potential harm while preserving essential identity, evidence, recovery, and safety paths.

### 5.11 Quarantine

Quarantine is a restricted lifecycle and admission profile applied to one or more ACS objects.

### 5.12 Restoration

Restoration is the governed reduction or removal of immune restrictions after sufficient evidence and authority establish that broader operation may safely resume.

### 5.13 Immune evidence channel

An immune evidence channel is a port, attachment, connection, or mediated path authorized to carry bounded immune evidence.

### 5.14 Immune action capability

An immune action capability authorizes one bounded immune-triggered operation through ACS-defined admission, security, and lifecycle mechanisms.

## 6. Immune roles

The public integration model recognizes at least the following conceptual roles.

### 6.1 Observer

Collects bounded observations.

The observer does not automatically:

- correlate globally;
- inspect every payload;
- issue directives;
- impose containment.

### 6.2 Evidence validator

Evaluates evidence integrity, provenance, freshness, schema, and admissibility.

### 6.3 Correlator

Combines several evidence items into a derived evidence set or assessment input.

Correlation must preserve:

- source provenance;
- evidence independence;
- time boundaries;
- uncertainty;
- transformation history.

### 6.4 Assessor

Produces bounded immune assessments or findings.

### 6.5 Challenger

Issues authorized requests for additional evidence, reauthentication, continuity proof, or protocol confirmation.

### 6.6 Recommender

Produces bounded recommended actions.

### 6.7 Containment coordinator

Coordinates authorized containment across several ACS objects or authorities.

Coordination does not automatically grant execution authority over all participants.

### 6.8 Restoration verifier

Evaluates whether conditions for reducing or removing restrictions are satisfied.

### 6.9 Audit participant

Produces or protects bounded evidence concerning immune actions and decisions.

### 6.10 Role composition

One participant may perform several roles when explicitly assigned.

Combining roles shall not silently amplify authority.

## 7. Immune identity and authentication

Immune participants shall possess ACS identities and credentials governed by ACS-0007.

Immune identity shall not be inferred solely from:

- process name;
- executable path;
- host placement;
- network address;
- relationship label;
- access to monitoring data.

Immune participants shall authenticate at assurance appropriate to:

- evidence sensitivity;
- requested action;
- target scope;
- containment severity;
- operator involvement.

A participant claiming immune status without valid assignment and authentication shall receive no immune authority.

## 8. Immune relationships

### 8.1 Relationship class

Immune participants may use ACS immune relationships as defined by ACS-0002.

### 8.2 Scope

An immune relationship shall declare or reference:

- participants;
- purpose;
- observation scope;
- eligible evidence classes;
- permitted challenge behavior;
- permitted recommendation behavior;
- permitted action requests;
- resource limits;
- privacy restrictions;
- lifecycle behavior.

### 8.3 No general inspection right

An immune relationship does not grant unrestricted inspection of every signal, payload, memory, or internal state.

### 8.4 Relationship restriction

Immune relationships may themselves be:

- narrowed;
- suspended;
- revoked;
- quarantined;
- retired.

### 8.5 Self-protection

Immune relationships and ports may require stronger controls because misuse could create:

- surveillance;
- denial of service;
- arbitrary isolation;
- credential disruption;
- evidence forgery.

## 9. Immune-facing endpoints and ports

Immune systems shall use typed ACS endpoints and ports.

Potential public port purposes may include:

- evidence submission;
- observation subscription;
- challenge request;
- challenge response;
- assessment submission;
- recommendation submission;
- containment request;
- containment result;
- restoration request;
- restoration verification;
- immune health.

This specification does not define a mandatory public port catalog.

Every implemented port shall define:

- direction;
- signal classes;
- admission;
- authority;
- payload policy;
- resource limits;
- visibility;
- lifecycle;
- failure behavior.

## 10. Observation scope

Immune observation shall be bounded by:

- target participant;
- target endpoint;
- target port;
- relationship;
- connection;
- signal domain;
- signal intent;
- metadata category;
- payload category;
- time window;
- purpose;
- authority;
- resource budget.

An observation grant shall not silently expand because:

- additional participants become reachable;
- a region grows;
- a bridge exposes more traffic;
- the observer gains trust;
- an incident occurs elsewhere.

## 11. Metadata observation

Immune systems may be authorized to inspect metadata such as:

- participant identity;
- endpoint identity;
- port identity;
- relationship class;
- connection generation;
- signal identity;
- signal domain;
- signal intent;
- schema version;
- timing;
- size;
- rate;
- admission result;
- resource pressure;
- lifecycle transitions;
- mediation path;
- provenance summary;
- replay state;
- error category.

Metadata access remains bounded and privacy-sensitive.

## 12. Payload observation

Payload inspection requires separate explicit authority.

Authorization to inspect metadata shall not imply payload access.

Payload observation shall define:

- eligible payload classes;
- sensitivity;
- scope;
- purpose;
- retention;
- redaction;
- transformation;
- access logging;
- downstream disclosure.

Payload access should be minimized.

## 13. Evidence sources

Immune evidence may originate from:

- ACS admission systems;
- security systems;
- lifecycle systems;
- runtime systems;
- health systems;
- mediators;
- endpoint owners;
- operators;
- MEM services;
- physical safety systems;
- immune observers;
- external authorized systems.

Source category does not establish correctness automatically.

## 14. Evidence identity

Every material immune evidence item shall possess or reference identity sufficient for:

- duplicate detection;
- provenance;
- correlation;
- lifecycle;
- retention;
- review;
- contradiction;
- invalidation.

Evidence identity shall remain distinct from:

- transport message identity;
- connection identity;
- memory identity;
- assessment identity;
- incident identity.

## 15. Evidence record

This section defines semantics, not a required wire format.

An immune evidence record should make it possible to determine:

```text
ImmuneEvidence
    evidence_identity
    evidence_type
    subject_scope
    source_identity
    producing_role
    observation_time
    received_time
    validity_boundary
    confidence
    provenance
    independence_or_correlation
    schema_identity
    integrity_state
    confidentiality_state
    authorization_scope
    transformation_history
    retention_reference
    supersession_or_invalidation_state
```

Not every recipient must receive every field.

The required meaning must remain recoverable by authorized systems.

## 16. Evidence provenance

Evidence provenance shall preserve enough information to determine:

- original source;
- observer;
- mediator;
- transformation;
- aggregation;
- redaction;
- validation;
- storage or reconstruction history where relevant.

A derived assessment shall not masquerade as a direct observation.

## 17. Evidence freshness

Evidence shall carry or imply a freshness boundary appropriate to its subject.

Old evidence shall not remain current indefinitely.

Freshness requirements may differ for:

- credential misuse;
- replay;
- overload;
- connection churn;
- long-term protocol violations;
- historical compromise evidence.

## 18. Evidence confidence

Confidence describes the support for an evidence claim.

Confidence shall not create authority.

High-confidence evidence may still require:

- concurrence;
- security validation;
- lifecycle admission;
- operator review.

## 19. Evidence independence

Repeated copies of one observation shall not be treated as several independent evidence sources.

Correlation shall account for shared:

- source;
- sensor;
- mediator;
- software;
- host;
- trust anchor;
- storage;
- operator;
- network path.

## 20. Evidence conflict

Conflicting evidence shall remain explicit.

The system shall not resolve conflict solely by:

- choosing the newest evidence;
- choosing the highest confidence number;
- choosing the most trusted source;
- choosing the first report;
- counting duplicated reports as majority.

## 21. Evidence admissibility

An immune service may perform domain-specific evidence admission after ACS signal admission.

Evidence admission may evaluate:

- source authority;
- schema;
- freshness;
- provenance;
- integrity;
- relevance;
- duplication;
- correlation;
- privacy;
- resource cost;
- incident scope.

ACS admission does not establish immune evidentiary value.

## 22. Evidence retention

Immune evidence retained through MEM shall remain governed by MEM specifications.

Retention shall define:

- logical memory identity where applicable;
- versioning;
- provenance;
- sensitivity;
- custody;
- retention period;
- deletion or hold policy;
- reconstruction requirements;
- access authority.

Immune systems shall not create a private ungoverned evidence store outside MEM semantics merely to avoid retention controls.

## 23. Assessment model

An immune assessment shall identify or imply:

- assessment identity;
- assessor identity;
- evidence scope;
- assessed subject;
- finding;
- confidence;
- uncertainty;
- conflicting evidence;
- evidence freshness;
- recommended action if any;
- validity boundary;
- review requirements.

Assessments are derived evidence.

## 24. Assessment outcomes

Public implementations should support outcomes equivalent to:

- no material anomaly observed;
- anomaly observed;
- policy violation observed;
- elevated risk;
- suspected compromise;
- likely compromise;
- confirmed compromise;
- recovering;
- unresolved;
- conflicting;
- unknown.

The future IMM series may define a more complete taxonomy.

The words used do not themselves grant authority.

## 25. Recommendations

An immune recommendation shall define:

- target scope;
- recommended action;
- reason;
- evidence references;
- urgency;
- expected duration;
- expected risk reduction;
- side effects;
- restoration condition;
- required authority.

A recommendation is not a directive.

## 26. Challenges

Immune systems may request bounded challenges such as:

- reauthentication;
- credential freshness proof;
- connection-generation proof;
- endpoint-continuity proof;
- provenance evidence;
- protocol confirmation;
- resource-accounting explanation;
- runtime-health attestation.

Challenges shall be:

- authorized;
- bounded;
- replay-resistant;
- resource-limited;
- privacy-aware.

Failure to answer a challenge is evidence.

It is not automatically proof of compromise.

## 27. ACS actions available to immune integration

Immune-triggered action shall use existing ACS mechanisms.

Potential actions include:

1. deny new relationship admission;
2. deny new connection admission;
3. deny new attachments;
4. narrow attachment scope;
5. restrict signal domains or intents;
6. disable directives;
7. permit observations only;
8. reduce rate or burst budgets;
9. reduce payload access;
10. require mandatory mediation;
11. require stronger authentication;
12. require capability revalidation;
13. suspend one attachment;
14. suspend one connection;
15. suspend a relationship;
16. enter quarantine profile;
17. revoke a capability through authorized security policy;
18. terminate a secure session through authorized security policy;
19. require recovery-mode entry;
20. preserve evidence and recovery channels.

This list defines integration vocabulary.

It does not grant every immune participant authority to invoke every action.

## 28. Immune action capability

Every privileged immune action shall require an explicit capability or equivalent authority.

The capability shall define:

- subject;
- target;
- action;
- scope;
- duration;
- parameters;
- evidence requirements;
- concurrence requirements;
- restoration behavior;
- resource limits;
- revocation;
- audit.

## 29. No implicit containment authority

The following shall not independently grant containment authority:

- immune role assignment;
- immune relationship;
- evidence possession;
- high confidence;
- suspected compromise;
- repeated alerts;
- endpoint ownership;
- local execution;
- operator visibility.

## 30. Containment severity

Containment may be grouped into bounded severity levels such as:

### 30.1 Observation-only

No operational restriction.

Additional monitoring may occur within existing authority.

### 30.2 Increased scrutiny

May require:

- stronger validation;
- reauthentication;
- mandatory mediation;
- shorter grants;
- reduced disclosure.

### 30.3 Limited restriction

May restrict:

- selected attachments;
- selected signal intents;
- payload access;
- rates;
- new connection creation.

### 30.4 Strong isolation

May suspend ordinary communication while preserving:

- immune evidence;
- health;
- operator;
- recovery;
- physical safety paths.

### 30.5 Emergency containment

May invoke narrow emergency controls under separately defined authority.

The exact public severity taxonomy may be refined by IMM.

## 31. Quarantine profile

A quarantine profile shall define:

- affected ACS objects;
- permitted ports;
- prohibited ports;
- permitted signal domains;
- permitted intents;
- surviving relationships;
- surviving connections;
- payload access;
- resource budgets;
- health reporting;
- immune evidence paths;
- operator access;
- recovery access;
- duration;
- review;
- exit conditions.

## 32. Quarantine invariants

Quarantine shall not automatically:

- retire participant identity;
- transfer endpoint ownership;
- delete memory;
- erase evidence;
- destroy credentials;
- shut down physical hardware;
- revoke unrelated capabilities;
- expose all internal state.

## 33. Containment scope

Containment shall use the smallest safe scope.

Possible scopes include:

- one signal;
- one attachment;
- one port;
- one connection;
- one endpoint;
- one relationship;
- one participant instance;
- one logical participant;
- one trust domain;
- one region;
- one operation family.

Broad organism-wide containment requires stronger authority and evidence than a narrow attachment restriction.

## 34. Containment duration

Containment shall be:

- time-bounded;
- condition-bounded;
- review-bounded;
- or explicitly indefinite under exceptional authorized policy.

Indefinite containment must remain observable and reviewable.

Containment expiry does not automatically restore broader authority unless restoration conditions are satisfied.

## 35. Containment side effects

Before strong containment, the system should consider:

- in-flight directives;
- physical safety;
- memory consistency;
- custody obligations;
- ongoing recovery;
- operator control;
- other participants depending on the target;
- evidence preservation;
- protected capacity.

Urgency may limit complete analysis.

Known side effects must remain explicit.

## 36. Containment and physical safety

Containment shall not disable required physical safety communication unless a separately authorized safety mechanism provides a safer alternative.

Physical safety ceilings remain above ordinary immune action.

## 37. Containment and protected capacity

Quarantined or restricted systems may retain protected capacity for:

- health reporting;
- immune evidence;
- security state;
- lifecycle control;
- recovery;
- operator communication;
- physical safety.

Ordinary traffic shall not use those paths to bypass quarantine.

## 38. Containment concurrence

Sensitive containment may require concurrence from:

- immune authority;
- security authority;
- lifecycle authority;
- physical safety authority;
- operator authority;
- endpoint owner or steward.

Concurrence requirements shall be explicit.

Concurrence is not automatically majority voting.

## 39. Emergency containment

Emergency containment may be permitted when delay presents substantial risk.

Emergency authority shall be:

- narrow;
- short-lived;
- attributable;
- capability-bound;
- resource-bounded;
- subject to later review.

Emergency action should favor reversible restriction and preservation over destruction.

## 40. Security integration

Immune systems may submit evidence or requests concerning:

- identity conflict;
- credential misuse;
- replay;
- stale capabilities;
- anomalous session behavior;
- suspicious delegation;
- trust-anchor inconsistency;
- revocation failure.

Security systems retain authority over:

- credential issuance;
- credential revocation;
- capability issuance;
- capability revocation;
- secure-session requirements;
- trust-anchor management.

## 41. Admission integration

Immune restrictions may influence ACS-0006 admission through:

- denied scopes;
- reduced grants;
- stronger challenge requirements;
- reduced budgets;
- shorter grant validity;
- mandatory mediation;
- protected-only operation.

Immune systems shall not bypass admission evaluators unless explicitly assigned admission authority for the affected scope.

## 42. Lifecycle integration

Immune actions affecting lifecycle shall use ACS-0005 transitions.

Examples include:

- restrict;
- suspend;
- drain;
- close;
- quarantine;
- recover;
- restore.

An immune system shall not invent undefined hidden lifecycle states.

## 43. Trust integration

Immune evidence may affect trust evidence considered by ACS-0007.

Immune evidence shall not become an unbounded global trust score.

Trust changes remain:

- scoped;
- evidence-backed;
- time-sensitive;
- reversible where appropriate.

## 44. Runtime integration

Runtime systems may enforce immune-authorized actions through:

- queue restrictions;
- connection blocking;
- process isolation;
- resource reduction;
- scheduling restriction;
- sandboxing;
- logging;
- recovery execution.

Runtime implementation does not create broader immune authority.

## 45. Health integration

Health systems may provide evidence concerning:

- crashes;
- thermal pressure;
- degraded devices;
- resource exhaustion;
- transport instability;
- process failure;
- stale services.

Immune systems shall preserve the distinction between health failure and compromise.

## 46. MEM integration

### 46.1 MEM remains authoritative for memory semantics

MEM defines:

- evidence-memory identity;
- provenance;
- versioning;
- retention;
- custody;
- recovery;
- deletion;
- lifecycle;
- operation results.

### 46.2 ACS remains authoritative for communication

ACS defines:

- evidence transport;
- immune endpoints;
- immune ports;
- attachments;
- secure sessions;
- admission;
- budgets;
- lifecycle actions;
- capabilities.

### 46.3 Evidence storage is not ACS state

Persisting immune evidence through MEM does not make that evidence part of ACS connection or relationship state.

### 46.4 Communication failure is not evidence deletion

Loss of an immune evidence connection does not imply that retained evidence is absent.

### 46.5 Quarantine is not memory deletion

ACS quarantine shall not silently:

- delete memory;
- revoke memory identity;
- alter retention;
- transfer custody;
- remove provenance.

### 46.6 Memory action authority

Immune systems require MEM-specific authority for memory operations such as:

- hold;
- retrieval;
- repair;
- retention extension;
- deletion prevention;
- disclosure.

An ACS immune capability does not automatically grant those operations.

## 47. Evidence preservation

Containment and recovery shall preserve enough evidence to support:

- later review;
- conflict resolution;
- restoration;
- security analysis;
- operator accountability;
- conformance.

Evidence preservation shall remain bounded by:

- privacy;
- retention;
- resource limits;
- legal or operator policy where applicable;
- MEM authority.

## 48. Restoration

### 48.1 Restoration principle

Restrictions shall not remain broader or longer than justified.

### 48.2 Restoration evidence

Restoration may require:

- successful challenge;
- reauthentication;
- credential replacement;
- secure-session replacement;
- recovery completion;
- software repair;
- health stabilization;
- evidence reconciliation;
- operator review;
- sustained compliant behavior.

### 48.3 Partial restoration

Restoration may proceed gradually.

Examples include:

- health-only to observation-only;
- observation-only to request-only;
- request-only to ordinary operation;
- selected ports restored before others;
- local operation restored before remote operation.

### 48.4 Restoration authority

Authority to impose containment does not automatically grant authority to restore every capability.

### 48.5 Restoration does not erase history

Restoration shall not delete evidence or pretend that containment never occurred.

## 49. False positives

The architecture shall acknowledge false-positive risk.

A false positive may cause:

- unnecessary restriction;
- lost availability;
- delayed cognition;
- resource waste;
- broken relationships;
- operator burden.

Containment policy should favor:

- proportionality;
- reversibility;
- narrow scope;
- explicit uncertainty;
- review;
- restoration.

## 50. False negatives

The architecture shall acknowledge false-negative risk.

A false negative may permit:

- continued misuse;
- privilege abuse;
- evidence corruption;
- resource exhaustion;
- lateral propagation;
- trust damage.

No implementation can assume perfect detection.

## 51. Immune-system compromise

Immune participants may themselves become:

- faulty;
- stale;
- compromised;
- malicious;
- overbroad;
- unavailable.

The system shall not assume immune infallibility.

Immune participants shall be subject to:

- identity validation;
- capability limits;
- admission;
- resource limits;
- audit;
- revocation;
- independent evidence;
- containment.

## 52. Immune conflicts

Immune services may disagree concerning:

- evidence validity;
- severity;
- containment scope;
- restoration readiness;
- compromise status.

Conflict shall remain explicit.

The architecture shall not automatically select:

- the most severe recommendation;
- the least severe recommendation;
- the highest numerical score;
- the first response;
- the largest group of correlated observers.

## 53. Distributed immune coordination

Several immune participants may cooperate.

Coordination shall define:

- role assignments;
- evidence sharing;
- confidentiality;
- correlation;
- duplicate handling;
- authority scope;
- conflict handling;
- partition behavior;
- resource limits.

Distribution does not automatically create consensus or organism-wide authority.

## 54. Partition behavior

During partition:

- existing immune grants remain bounded by validity;
- new strong containment may require local emergency authority or defer;
- evidence may be stored for later reconciliation;
- local restrictions may continue within pre-authorized scope;
- exclusive organism-wide authority shall not be duplicated silently;
- restoration should remain conservative where revocation or evidence state is unknown.

After partition, systems shall reconcile:

- findings;
- restrictions;
- revocations;
- restored scopes;
- evidence identity;
- duplicate reports;
- authority generations.

## 55. Immune overload

Immune systems themselves may become overloaded.

Overload behavior shall preserve capacity for:

- critical evidence;
- strong identity anomalies;
- active containment;
- restoration;
- operator communication;
- immune health.

Overload may cause:

- sampling;
- aggregation;
- deferred analysis;
- reduced evidence detail;
- lower-priority observation loss.

The loss or reduction shall remain explicit.

## 56. Immune backpressure

Immune evidence channels may apply backpressure.

Backpressure shall not cause ordinary systems to:

- suppress mandatory safety evidence;
- retry without bound;
- discard required audit evidence silently;
- infer that the immune system found no anomaly.

## 57. Evidence prioritization

Priority may consider:

- potential impact;
- scope;
- freshness;
- confidence;
- ongoing activity;
- physical safety relevance;
- credential severity;
- propagation risk;
- evidence loss risk.

Priority does not create finding truth or containment authority.

## 58. Privacy

Immune systems may observe highly sensitive information.

Privacy requirements shall govern:

- observation scope;
- payload inspection;
- evidence retention;
- evidence sharing;
- operator access;
- public disclosure;
- cross-domain transfer;
- restoration review.

Immune architecture shall not become unrestricted surveillance.

## 59. Redaction

Evidence may be redacted before disclosure.

Redaction shall preserve enough information to support the declared purpose.

A redacted record shall not falsely imply that omitted evidence does not exist.

## 60. Audit

Sensitive immune actions should preserve bounded audit evidence concerning:

- evidence source;
- assessor;
- finding;
- requested action;
- approving authority;
- applied restriction;
- target scope;
- duration;
- side effects;
- restoration;
- unresolved conflicts.

Audit shall not expose reusable secrets or unnecessary payload content.

## 61. Public implementation requirements

A public implementation claiming support for ACS-0008 shall document:

- immune participant identity model;
- supported immune roles;
- immune relationship use;
- observation scope;
- metadata and payload access boundaries;
- evidence identity and provenance;
- assessment representation;
- recommendation and request behavior;
- supported immune action capabilities;
- quarantine profile behavior;
- containment scope and duration;
- concurrence behavior;
- restoration behavior;
- immune conflict handling;
- partition behavior;
- resource and overload behavior;
- immune-system self-protection;
- privacy and audit;
- MEM evidence-storage integration;
- unsupported features;
- known limitations.

Public documentation need not reveal private heuristics, thresholds, topologies, evidence catalogs, or response procedures.

## 62. Conformance expectations

Conformance evidence should demonstrate that:

1. immune status does not bypass authentication;
2. immune relationship does not grant unrestricted payload access;
3. observations do not automatically become directives;
4. assessments do not automatically become containment;
5. containment actions require explicit capabilities;
6. an immune participant cannot invent new lifecycle states;
7. immune actions use ACS admission, security, lifecycle, and budget mechanisms;
8. containment uses the smallest safe scope;
9. quarantine preserves identity and recovery channels;
10. quarantine does not silently delete memory;
11. security authority remains separate from immune authority;
12. credential revocation requires authorized security action;
13. immune evidence retains provenance and freshness;
14. duplicated evidence does not become independent corroboration;
15. conflicting evidence remains explicit;
16. false-positive uncertainty can produce reversible restrictions;
17. emergency containment is narrow and time-bounded;
18. restoration is explicit and does not erase history;
19. immune participants themselves can be restricted or revoked;
20. immune overload does not report false absence of anomalies;
21. partition does not duplicate exclusive organism-wide authority;
22. MEM retains authority over evidence memory and retention;
23. audit remains useful without becoming unrestricted surveillance;
24. public conformance does not require disclosure of private detection or containment policy.

## 63. Prohibited interpretations

This specification shall not be interpreted to mean that:

- immune observation grants universal inspection;
- immune evidence proves compromise;
- high anomaly score grants authority;
- immune relationship bypasses admission;
- suspected compromise requires permanent quarantine;
- quarantine retires identity;
- containment permits memory deletion;
- containment permits physical destruction;
- immune systems own security credentials;
- immune systems own endpoint lifecycle;
- immune systems may invent new ACS restrictions;
- evidence retention grants memory ownership;
- resource overload proves attack;
- connection failure proves compromise;
- the most severe immune recommendation always wins;
- immune systems are assumed infallible;
- operator review is never required;
- public conformance requires disclosure of private immune heuristics.

## 64. Initial architectural commitments

ACS-0008 establishes that:

1. immune participants remain ordinary governed ACS participants with specialized roles;
2. observation, evidence, assessment, recommendation, request, directive, containment, and restoration remain distinct;
3. immune relationships do not grant unrestricted access;
4. payload inspection requires separate authority;
5. immune evidence is identified, scoped, fresh, provenance-preserving, and uncertainty-aware;
6. duplicated evidence does not become independent corroboration;
7. conflicting evidence remains explicit;
8. immune findings do not create authority automatically;
9. privileged immune actions require explicit capabilities;
10. immune-triggered restrictions use ACS-0005, ACS-0006, and ACS-0007 mechanisms;
11. ACS-0008 does not invent new admission, lifecycle, security, or revocation semantics;
12. containment uses the smallest safe scope;
13. quarantine preserves identity, evidence, recovery, operator, and safety paths;
14. containment does not automatically delete memory, revoke identity, or destroy hardware;
15. emergency containment remains narrow, reversible where practical, and reviewable;
16. security authority remains separate from immune authority;
17. lifecycle authority remains separate from immune authority;
18. MEM remains authoritative for evidence memory, retention, custody, and recovery;
19. immune systems remain subject to authentication, admission, budgets, audit, revocation, and containment;
20. false positives and false negatives remain recognized architectural risks;
21. restoration is explicit and evidence-backed;
22. distributed immune coordination does not automatically create consensus or universal authority;
23. partition behavior does not duplicate exclusive authority silently;
24. immune monitoring remains privacy-bounded;
25. public implementation remains possible without exposing private immune heuristics or production policy.

## 65. Open questions

The following questions remain for future IMM specifications, later ACS profiles, or implementation profiles:

- Which immune roles are mandatory for baseline Node conformance?
- Which metadata categories may immune observers inspect by default?
- Which payload classes may ever be inspected automatically?
- Which evidence schemas should be standardized publicly?
- Which assessment outcomes belong in the public IMM taxonomy?
- Which containment severities should be standardized?
- Which restrictions may one immune authority apply without concurrence?
- Which actions require security authority?
- Which actions require lifecycle authority?
- Which actions require operator authority?
- Which actions require physical safety concurrence?
- Which emergency actions may proceed during partition?
- How long may emergency containment remain active before review?
- Which evidence is sufficient for confirmed compromise?
- How should correlated immune observers be recognized?
- Which evidence may be retained during privacy-sensitive operation?
- How should encrypted evidence be inspected or validated?
- Which evidence must survive hibernation and recovery?
- How should immune findings affect trust without creating one universal trust score?
- Which restoration conditions should be standardized?
- How should false-positive harm be measured?
- How should immune systems be tested for overreach?
- How should compromised immune participants be detected?
- How should distributed immune conflicts be resolved?
- How should organism-wide containment avoid centralized fragility?
- Which evidence may be shared with external systems?
- How should public IMM architecture interact with private production immune policy?
- Which ACS-0008 concepts require formal public schemas?
- Which conformance failures should block implementation claims?
- How should IMM distinguish cognitive abnormality from security compromise?
- How should immune systems observe memory access without redefining MEM authorization?
- Which audit records should operators be allowed to inspect?
- How should immune resource budgets scale across minimal nodes and large servers?

These questions do not permit implementations to invent authority or weaken the boundaries already established.

## 66. Closing principle

> **Node’s immune architecture may increase awareness and request protection, but every restriction must still be authorized, bounded, attributable, reviewable, and expressed through the communication, security, lifecycle, and resource mechanisms ACS already defines.**

Evidence may justify concern.

Assessment may justify caution.

Recommendation may justify review.

Capability may authorize action.

Admission may permit execution.

Lifecycle may impose restriction.

Restoration may remove it.

None of those steps may be silently skipped.

## Revision history

### Version 0.1 — 2026-07-16

- Established the public ACS–immune integration boundary.
- Defined immune participants, roles, relationships, endpoints, ports, observation, evidence, assessments, recommendations, requests, directives, containment, quarantine, and restoration.
- Required explicit evidence identity, provenance, freshness, confidence, independence, conflict, and admissibility.
- Defined the ACS enforcement vocabulary available to immune integration.
- Required explicit capabilities for privileged immune actions.
- Established bounded containment severity, scope, duration, concurrence, side-effect, and emergency behavior.
- Preserved separate security, lifecycle, admission, runtime, health, physical safety, and immune authority.
- Defined false-positive, false-negative, immune-compromise, conflict, partition, overload, privacy, redaction, and audit requirements.
- Preserved MEM authority over evidence memory, retention, custody, recovery, and memory-operation semantics.
- Established public implementation requirements, conformance expectations, prohibited interpretations, commitments, and open questions.
- Prepared the boundary required before beginning the public IMM specification series.
