# IMM-0000: Node Immune Architecture Charter and Scope

| Field | Value |
|---|---|
| Specification | IMM-0000 |
| Title | Node Immune Architecture Charter and Scope |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | IMM-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Pending review |
| Depends on | ACS-0000 through ACS-0009; MEM-0000 through MEM-0010 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in public authority separation, state honesty, and classification boundaries |

> **Node immune architecture reduces harm through governed evidence and bounded action without converting suspicion into truth or protection into unchecked authority.**

## Architectural-intent notice

This specification defines independently authored public architecture. It establishes contracts, states, ownership boundaries, and prohibitions that a Node-compatible immune implementation must preserve.

It does not disclose or summarize private immune reasoning. It is not produced by deleting, sanitizing, paraphrasing, or selectively redacting restricted designs.

Implementations may differ in detection, correlation, assessment, and response mechanisms. Their observable behavior must preserve the public distinctions and authority boundaries defined here.

## 1. Purpose

The Node immune architecture provides a governed way to:

- observe bounded behavior and state relevant to possible harm;
- identify and preserve evidence;
- assess anomaly, inconsistency, degradation, policy violation, and possible compromise;
- recommend proportionate responses;
- request authority from the architecture that owns the affected operation;
- coordinate bounded containment when explicitly authorized;
- preserve identity, evidence, safety, and recovery paths during restriction;
- verify recovery and restoration conditions;
- retain attributable audit history;
- represent uncertainty, partial knowledge, and failure honestly.

The immune architecture protects Node without becoming an undefined super-administrator. Observation, evidence possession, confidence, physical proximity, runtime access, or immune role assignment does not itself create authority.

## 2. Public scope

The public IMM series may define:

- immune roles and role boundaries;
- observation and evidence contracts;
- evidence identity, provenance, freshness, integrity, confidence, independence, and admissibility requirements;
- assessment and recommendation contracts;
- authorization-request and action-attribution requirements;
- containment and quarantine semantics;
- recovery- and restoration-verification contracts;
- false-positive and false-negative handling requirements;
- conflict, uncertainty, degradation, and failure states;
- privacy and cognitive-content boundaries;
- immune-service accountability, failure, and compromise handling;
- audit and public conformance requirements;
- interoperability boundaries with ACS, MEM, adaptive state, runtime, resource management, and bootstrap.

Public IMM architecture defines what must remain distinguishable and what behavior is prohibited. It does not publish the private reasoning by which a production implementation reaches an assessment or chooses among eligible responses.

## 3. Explicit non-goals

The public IMM series does not define or publish:

- private threat models;
- proprietary anomaly-detection or evidence-correlation algorithms;
- production heuristics, weights, scores, thresholds, cutoffs, or escalation triggers;
- production containment or restoration decision policy;
- protected participant, endpoint, port, capability, topology, or service catalogs;
- credentials, trust roots, key material, or recovery secrets;
- production operator procedures or incident-response playbooks;
- deployment-specific incident history;
- unrestricted cognitive-content inspection;
- one mandatory detection, assessment, consensus, enforcement, or voting mechanism;
- physical destruction procedures;
- automatic evidence or memory deletion;
- ACS admission, trust, capability, delegation, revocation, lifecycle, or enforcement semantics;
- MEM persistence, custody, retention, reconstruction, recovery, or deletion semantics;
- adaptive-state mutation semantics;
- runtime scheduling or placement policy;
- resource-allocation algorithms or hard-ceiling values;
- bootstrap enrollment, identity-establishment, or verified-substrate procedures.

Biological immune systems may provide analogy. They do not prescribe implementation.

## 4. Responsibility separation

Immune behavior is a governed sequence of distinct responsibilities. One implementation may combine responsibilities only when their meanings, inputs, outputs, authority, and audit remain independently observable.

### 4.1 Observation

Observation is bounded collection or receipt of state, behavior, metadata, or other information for a declared protective purpose.

Observation does not grant unrestricted payload access, interpretation rights, authority to alter the subject, or authority to impose restrictions.

### 4.2 Evidence

Evidence is identified information admitted for immune assessment under a declared evidence contract.

Evidence must preserve applicable provenance, freshness, scope, integrity, uncertainty, transformation history, and correlation boundaries. Evidence is not a verdict and does not independently establish compromise, intent, root cause, or required action.

### 4.3 Assessment

Assessment is a scoped interpretation of evidence that produces a finding, confidence boundary, uncertainty statement, and conflict state.

Assessment is derived information, not direct observation. Assessment does not create authority.

### 4.4 Recommendation

A recommendation proposes a bounded action and identifies its evidence basis, scope, expected duration, side effects, verification conditions, and required authority.

A recommendation is not enforcement and does not compel execution.

### 4.5 Authorization

Authorization is the governed decision that an identified actor may invoke a declared privileged action within explicit scope and lifecycle.

Authorization belongs to the architecture or authority domain that owns the affected operation. IMM may request authority and provide evidence. It shall not invent, broaden, delegate, or grant itself authority.

### 4.6 Enforcement

Enforcement is the concrete application of an authorized restriction or protective operation by an eligible enforcement component.

Enforcement must remain bound to the granted actor, target, action, scope, duration, conditions, resource limits, reason, and audit requirements. Technical ability or runtime proximity does not create enforcement authority.

### 4.7 Restoration

Restoration is the governed reduction or removal of restrictions after explicit verification establishes that required recovery and safety conditions are satisfied.

Restoration is not automatic merely because symptoms disappear, a process restarts, connectivity returns, a credential rotates, or time passes.

### 4.8 Audit

Audit is attributable evidence of observations, assessments, recommendations, authorizations, actions, reviews, side effects, recovery, restoration, and unresolved uncertainty.

Audit records do not themselves grant authority. Audit must preserve accountability without exposing reusable secrets or unnecessary cognitive content.

## 5. Ownership boundaries

| Architecture | Retained authority | IMM relationship |
|---|---|---|
| ACS | Participant identity, relationships, endpoints, ports, signals, admission, budgets, connection lifecycle, security and trust, capabilities, delegation, revocation, and ACS enforcement | IMM uses ACS contracts and may submit evidence or authorized requests; it does not redefine or bypass them |
| MEM | Memory identity, persistence, provenance, operation semantics, custody, retention, reconstruction, recovery, restoration, and deletion | IMM may request MEM operations under MEM-specific authority; it cannot directly rewrite governed memory |
| Adaptive state | Descriptor identity, validity, ownership, and governed mutation semantics | IMM may report evidence or request change; it cannot directly mutate adaptive-state descriptors |
| Runtime | Concrete execution, isolation, scheduling, placement, and enforcement within granted authority | Runtime may enforce immune-authorized actions; implementation access does not create immune or runtime authority |
| Resource management | Allocation, accounting, reservations, pressure handling, reclamation, and hard ceilings | IMM may report abuse or request bounded restriction; it cannot allocate resources or exceed ceilings |
| Bootstrap | Initial verified substrate, enrollment, identity establishment, startup, and assigned recovery prerequisites | IMM may provide evidence or request recovery; it cannot self-enroll, self-verify, or replace bootstrap authority |
| IMM | Evidence interpretation, bounded findings, recommendations, protective coordination, recovery verification, and immune audit | IMM remains subordinate to all owning authority domains for privileged action |

No boundary may be bypassed by relabeling an ordinary operation as immune, emergency, local, trusted, or restorative.

## 6. Public and private classification boundary

### 6.1 Public material

The public repository may contain:

- role and contract definitions;
- mandatory distinctions and invariants;
- explicit state vocabularies;
- authority and ownership boundaries;
- minimum provenance, attribution, lifecycle, review, and verification requirements;
- failure and conformance expectations;
- prohibited interpretations;
- implementation-neutral examples that reveal no protected mechanism.

### 6.2 Private material

The public repository shall not contain:

- private heuristics or thresholds;
- proprietary evidence-correlation rules;
- production threat classifications or response selection logic;
- protected monitoring coverage or topology;
- production credentials, authority assignments, or capability catalogs;
- operator playbooks, escalation paths, or recovery secrets;
- private cognitive-health models;
- production incident evidence whose disclosure weakens security or privacy.

Public contracts must be sufficient to support and test a private implementation without requiring disclosure of its private reasoning, heuristics, thresholds, topology, or procedures.

## 7. Required public states

IMM state must be scoped to a declared subject and dimension. A single label must not silently replace evidence, assessment, authorization, action, and recovery state.

- **Unknown:** available evidence is insufficient to establish the state. Unknown is not healthy, safe, compromised, absent, or failed.
- **Partial:** some required observations, evidence, actions, or verification steps are complete and others are not.
- **Stale:** information was previously valid or useful but has exceeded its applicable freshness boundary.
- **Unavailable:** a required participant, dependency, evidence source, authority, or operation is known to be inaccessible.
- **Degraded:** operation continues with reduced capability, assurance, coverage, or performance. Degraded does not imply compromise.
- **Contained:** one or more bounded restrictions are active to reduce risk while preserving required safety, evidence, and recovery paths.
- **Quarantined:** a governed isolation profile restricts ordinary interaction pending review, recovery, or verification. Quarantine does not imply guilt, retirement, deletion, or destruction.
- **Recovering:** governed repair, reconciliation, revalidation, or other recovery work is in progress. Recovering is not restored.
- **Restored:** explicit verification and applicable authority have established that a declared scope may resume a stated level of operation.
- **Failed:** a declared immune operation or responsibility did not satisfy its required completion conditions. Failure of an action does not prove compromise of its target.

Partial, stale, unavailable, degraded, contained, quarantined, recovering, restored, and failed may coexist across different dimensions. Implementations must report the scope and dimension of each state.

Missing, conflicting, stale, or unavailable evidence shall not be silently converted into healthy, safe, trusted, absent, current, or compromised state.

## 8. Authority and containment principles

### 8.1 No self-granted authority

No immune participant may grant itself authority, enlarge its capabilities, assign itself a role, declare its own evidence sufficient, or treat implementation access as permission.

No component may declare itself trusted solely through self-report.

### 8.2 Anomaly is not proof of compromise

Anomaly, policy violation, degradation, failed challenge, resource pressure, unusual behavior, and communication failure may support assessment. None independently proves compromise, malicious intent, or root cause.

### 8.3 Containment is bounded and attributable

Containment must identify the actor, approving authority, target, action, scope, reason, evidence references, start condition, duration or review boundary, permitted recovery and safety paths, side effects, and exit conditions.

The smallest safe scope should be preferred. Containment is protective and reversible where practical; it is not punishment or ownership transfer.

### 8.4 No unreviewed permanent containment

Permanent or indefinite containment shall not be imposed merely because a condition remains unresolved, a symptom persists, or restoration evidence is unavailable.

Any permanent or indefinite containment requires separately governed authority, preserved evidence, explicit rationale, declared scope, continuing review, audit visibility, and an appeal or reconsideration path where applicable.

### 8.5 Quarantine is not deletion

Quarantine shall not automatically delete memory or evidence, retire identity, transfer custody or ownership, destroy credentials, erase audit history, or destroy physical hardware.

### 8.6 Recovery and restoration are verified

Recovery requires explicit verification appropriate to the affected scope. Restoration requires both verification and applicable authority. Expiry of containment alone does not restore broader authority.

## 9. Immune evidence and cognitive content

Immune evidence and cognitive content are distinct architectural categories.

Cognitive content does not become immune evidence merely because an observer can access it. Immune evidence does not become ordinary cognitive input merely because it concerns cognition.

Metadata access does not imply payload access. Payload inspection requires separate authority, declared purpose, minimal scope, retention controls, privacy protection, and audit. Retained evidence remains governed by MEM.

The public IMM series shall not define unrestricted cognitive surveillance.

## 10. False positives, false negatives, and failure

False-positive handling is mandatory. It must support explicit uncertainty, proportionate and bounded action, reversible restriction where practical, review, correction of invalid assessments, restoration paths, and audit of unnecessary impact.

False-negative handling must preserve later evidence admission, finding revision, authorized escalation, evidence-gap reporting, and honest disclosure of detection limits. No implementation may claim perfect coverage.

Immune services may themselves become stale, degraded, unavailable, overbroad, compromised, or failed. They remain subject to identity validation, authentication, ACS admission, capability limits, resource limits, lifecycle, revocation, containment, audit, and independent review.

Security-relevant failure must not silently become ordinary runtime failure. Failures affecting authority, evidence integrity, audit, containment, quarantine, recovery, or restoration must remain visible through appropriate security and immune state paths.

## 11. Prohibited interpretations

This specification shall not be interpreted to mean that:

- observation grants authority;
- evidence is a verdict;
- assessment is authorization;
- recommendation is enforcement;
- anomaly proves compromise;
- degraded operation means compromise;
- high confidence creates authority;
- an immune relationship bypasses ACS;
- immune action may directly rewrite MEM;
- immune action may mutate adaptive-state descriptors;
- runtime enforcement creates runtime authority;
- resource pressure grants immune authority;
- bootstrap failure permits immune self-enrollment;
- quarantine implies deletion, guilt, retirement, or destruction;
- restoration occurs automatically when symptoms disappear;
- permanent containment is acceptable without governed evidence and review;
- public conformance requires disclosure of private heuristics or thresholds.

## 12. Initial architectural commitments

IMM-0000 establishes that:

1. observation, evidence, assessment, recommendation, authorization, enforcement, restoration, and audit remain distinct;
2. IMM cannot self-grant authority;
3. anomaly is not proof of compromise;
4. containment is bounded, attributable, reviewable, and proportionate;
5. permanent or indefinite containment requires governed evidence, authority, and continuing review;
6. explicit unknown, partial, stale, unavailable, degraded, contained, quarantined, recovering, restored, and failed states are mandatory;
7. ACS retains its admission, security, authority, lifecycle, and enforcement semantics;
8. MEM retains persistence, custody, retention, reconstruction, recovery, and deletion semantics;
9. adaptive state, runtime, resource management, and bootstrap retain their owned responsibilities;
10. quarantine does not imply deletion or guilt;
11. recovery and restoration require explicit verification;
12. false-positive handling is mandatory;
13. immune evidence and cognitive content remain distinct;
14. public contracts support private implementation without exposing private reasoning;
15. private heuristics and thresholds are prohibited from the public repository.

## Revision history

### Version 0.1 — 2026-07-16

- Established the public Node immune architecture charter and scope.
- Defined public and private classification boundaries.
- Preserved ACS, MEM, adaptive-state, runtime, resource-management, and bootstrap ownership.
- Established explicit immune states, authority prohibitions, containment boundaries, recovery verification, and false-positive responsibility.
