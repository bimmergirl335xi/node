# IMM-0001: Core Invariants

| Field | Value |
|---|---|
| Specification | IMM-0001 |
| Title | Core Invariants |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | IMM-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Pending review |
| Depends on | IMM-0000; ACS-0000 through ACS-0009; MEM-0000 through MEM-0010 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in durable public invariants and adjacent-authority preservation |

> **Immune protection is conforming only when evidence, judgment, authority, action, recovery, and accountability remain distinguishable under both normal operation and failure.**

## Architectural-intent notice

This specification defines mandatory public invariants for every conforming Node immune implementation, whether public or private.

It does not prescribe detection algorithms, private reasoning, thresholds, production policy, implementation language, runtime topology, or enforcement mechanism. Later IMM specifications may refine these invariants but must not silently weaken or contradict them.

## 1. Purpose and scope

These invariants apply to immune participants, observers, evidence validators, correlators, assessors, recommenders, authorization requesters, containment coordinators, restoration verifiers, auditors, and enforcement components acting under immune-triggered authority.

They apply across local and remote operation, healthy and degraded service, containment, quarantine, recovery, restoration, partition, overload, and immune-service failure.

## 2. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory requirements. **Should** and **should not** describe strong recommendations that require documented justification when departed from. **May** describes permitted behavior.

Conformance depends on observable behavior, not labels such as `immune`, `trusted`, `monitor`, `watchdog`, or `security`.

## 3. Core invariants

### Invariant 1 — Evidence is not verdict

An evidence item or evidence set shall not be treated as a final determination of compromise, intent, root cause, or required response. Evidence must retain its scope, provenance, freshness, integrity, uncertainty, and correlation limits.

### Invariant 2 — Assessment is not authority

An assessment may interpret evidence and produce a finding or risk characterization. It shall not independently authorize admission changes, capability changes, revocation, containment, quarantine, memory operations, runtime isolation, or restoration.

### Invariant 3 — Recommendation is not enforcement

A recommendation proposes an action. It shall remain distinguishable from an authorization, directive, enforcement result, and completed state transition.

### Invariant 4 — Privileged action requires explicit authority

Every privileged immune-triggered action shall require a current capability or equivalent authority issued by the domain that owns the affected operation. Immune role, evidence possession, confidence, urgency, local execution, or technical ability shall not substitute for authority.

### Invariant 5 — Containment is bounded and attributable

Every containment action shall identify the actor, approving authority, target, action, scope, reason, evidence references, start condition, duration or review boundary, resource limits, side effects, surviving safety and recovery paths, exit conditions, and lifecycle state.

Containment shall use the smallest safe scope and shall remain reviewable.

### Invariant 6 — Unknown remains unknown

Unknown, partial, stale, unavailable, conflicting, and indeterminate evidence or state shall not be silently converted into healthy, safe, current, trusted, absent, compromised, restored, or failed state.

Missing information is not confirmation.

### Invariant 7 — Degraded operation is distinct from compromise

Degradation may result from hardware failure, software defect, resource pressure, network partition, incompatibility, configuration error, attack, or unknown cause. Degraded state shall not independently establish compromise or malicious intent.

### Invariant 8 — Immune actions cannot bypass ACS authority

Immune observation, evidence, findings, recommendations, and action requests shall use applicable ACS identity, relationship, endpoint, port, signal, admission, budget, lifecycle, security, capability, delegation, revocation, and enforcement contracts.

IMM shall not redefine those contracts or create hidden alternatives.

### Invariant 9 — Immune actions cannot directly rewrite MEM

An immune participant shall not directly create, mutate, invalidate, retain, release, repair, reconstruct, restore, delete, or transfer custody of governed memory merely because the content is immune-relevant.

Every memory operation requires applicable MEM semantics and MEM-specific authority.

### Invariant 10 — Immune actions cannot mutate adaptive-state descriptors

An immune participant may observe adaptive-state evidence or request a governed change. It shall not directly rewrite descriptor identity, validity, ownership, history, or mutation state outside the adaptive-state architecture.

### Invariant 11 — Quarantine does not imply deletion

Quarantine shall not automatically delete memory or evidence, erase audit history, retire identity, transfer ownership or custody, destroy credentials, revoke unrelated capabilities, or destroy physical hardware.

Quarantine also does not establish guilt or confirmed compromise.

### Invariant 12 — Recovery requires explicit verification

A subject shall not be declared recovered solely because execution resumes, connectivity returns, a process restarts, a credential rotates, a repair completes, or symptoms become quieter.

Recovery claims shall identify the verified scope, evidence, verifier, remaining limitations, and unresolved uncertainty.

### Invariant 13 — Restoration is not automatic when symptoms disappear

Restrictions shall be reduced or removed only through a governed restoration decision supported by explicit verification and applicable authority.

Containment expiry, elapsed time, quiet behavior, or symptom disappearance shall not automatically restore prior permissions.

### Invariant 14 — False-positive handling is mandatory

Every conforming implementation shall support review, correction, evidence invalidation or supersession, proportionate and reversible restriction where practical, restoration, and audit of unnecessary impact.

Invalidated evidence shall not continue to justify repeated restriction without new governed basis.

### Invariant 15 — Every privileged action has complete lineage

Every privileged action shall preserve identity, provenance, target, scope, reason, evidence basis, approving authority, parameters, start state, lifecycle, result, side effects, review state, and restoration relationship.

An action lacking material attribution shall not be reported as an ordinary successful immune action.

### Invariant 16 — No component is trusted solely through self-report

A participant shall not establish its own identity, role, trust, health, correctness, recovery, or authority solely by asserting it. Required independent evidence, authentication, assignment, challenge, or verification must remain governed by the owning architecture.

### Invariant 17 — Security failure remains distinct from ordinary runtime failure

Failure affecting identity, authentication, credentials, capabilities, delegation, revocation, evidence integrity, audit integrity, containment, quarantine, recovery, or restoration shall not silently collapse into a generic runtime error.

The security-relevant dimension and remaining uncertainty must remain visible.

### Invariant 18 — Immune evidence and cognitive content remain distinct

Cognitive content shall not become immune evidence merely because it is observable. Immune evidence shall not become ordinary cognitive input merely because it concerns cognition.

Metadata access does not imply payload access. Retained evidence remains governed by MEM.

### Invariant 19 — Public contracts support private implementation without exposing it

A public IMM contract shall specify required behavior, state, boundaries, and conformance evidence without requiring disclosure of private heuristics, thresholds, threat models, topology, correlation rules, credentials, response policy, or operator procedures.

Private implementation freedom shall not weaken public invariants.

### Invariant 20 — Immune authority cannot be self-granted or amplified

No immune participant may assign itself a role, enlarge a capability, delegate authority it does not possess, convert recommendation into directive, or treat emergency status as unrestricted permission.

Combining immune roles in one component shall not silently amplify authority.

### Invariant 21 — Anomaly is not proof of compromise

Anomaly, unusual traffic, resource pressure, policy violation, failed challenge, repeated error, stale activity, or communication failure may contribute evidence. None independently proves compromise, malicious intent, or required containment.

### Invariant 22 — Permanent containment requires governed evidence and review

Permanent or indefinite containment shall require separately governed authority, preserved evidence, explicit rationale, declared scope, continuing review, audit visibility, and an appeal or reconsideration path where applicable.

Unresolved state, missing restoration evidence, or persistent symptoms alone shall not satisfy this requirement.

### Invariant 23 — Containment preserves essential paths

Containment and quarantine shall preserve required physical safety, operator, health, evidence, security, audit, and recovery paths unless a separately authorized safer mechanism replaces them.

Ordinary traffic shall not use those paths to bypass restriction.

### Invariant 24 — Restoration does not erase history

Restoration shall not delete evidence, remove valid audit history, conceal prior restrictions, or pretend that an incident did not occur. Historical evidence remains subject to MEM retention and privacy authority.

### Invariant 25 — Immune components remain accountable and containable

Immune participants may themselves be stale, degraded, overbroad, unavailable, faulty, or compromised. They shall remain subject to identity validation, authentication, ACS admission, capability limits, resource limits, lifecycle, revocation, audit, independent evidence, containment, and review.

### Invariant 26 — Repeated reports are not automatically independent evidence

Copies, retries, aggregates, or reports sharing a source, sensor, mediator, host, software, trust anchor, storage path, operator, or network path shall not be counted as independent corroboration without an explicit independence basis.

### Invariant 27 — Conflict remains explicit

Conflicting evidence, findings, authority claims, containment recommendations, and restoration conclusions shall remain represented as conflict until governed resolution occurs.

The system shall not resolve conflict solely by choosing the newest, first, most severe, least severe, highest-confidence, or most numerous correlated report.

### Invariant 28 — Failure of an immune action is not proof against the target

A failed observation, challenge, containment, recovery, or restoration operation may result from the immune service, authority path, runtime, resources, transport, target, or unknown causes. The failure shall not independently prove target compromise.

### Invariant 29 — Absence of alert is not evidence of safety

No alert shall not be interpreted as no harmful condition unless a declared observation contract actually established sufficient coverage, freshness, availability, and successful evaluation for that scoped conclusion.

### Invariant 30 — Private heuristics and thresholds are not public architecture

Production heuristics, weights, scores, thresholds, confidence cutoffs, escalation triggers, and private evidence-correlation rules shall not be committed to the public repository.

Public documentation may state that governed policy exists; it shall not expose the protected mechanism.

## 4. Adjacent-authority preservation

A conforming IMM implementation shall preserve these ownership boundaries:

| Domain | IMM may | IMM shall not |
|---|---|---|
| ACS | Observe ACS evidence, submit findings or requests, invoke explicitly granted actions | Redefine or bypass admission, trust, capability, delegation, revocation, lifecycle, or enforcement |
| MEM | Submit evidence and request authorized memory operations | Directly rewrite persistence, custody, retention, reconstruction, recovery, restoration, or deletion state |
| Adaptive state | Observe evidence and request governed review or change | Mutate descriptors directly |
| Runtime | Request or coordinate authorized enforcement and receive results | Treat process access, scheduling, isolation, or placement control as authority |
| Resource management | Report pressure or abuse and request bounded restriction | Allocate resources, exceed hard ceilings, or equate pressure with attack |
| Bootstrap | Provide evidence and request governed recovery or revalidation | Self-enroll, self-verify, establish identity, or replace verified-substrate authority |

## 5. State and lifecycle requirements

At minimum, implementations shall support scoped representation of:

- unknown;
- partial;
- stale;
- unavailable;
- degraded;
- contained;
- quarantined;
- recovering;
- restored;
- failed.

A state report shall identify the subject, dimension, evidence boundary, observation time or freshness, authority where relevant, and remaining uncertainty.

Contained and quarantined describe governed restrictions, not verdicts. Recovering describes work in progress, not successful restoration. Restored is scoped and does not imply universal trust. Failed describes an operation outcome and does not prove target compromise.

## 6. Public conformance expectations

Public conformance evidence should demonstrate that:

1. evidence does not automatically become verdict;
2. assessments and recommendations do not create authority;
3. privileged actions fail closed when authority is absent, stale, revoked, conflicting, or unavailable;
4. containment records are bounded and attributable;
5. unknown, partial, stale, unavailable, degraded, and conflicting states remain explicit;
6. degraded operation is not reported as compromise;
7. immune actions use ACS authority and enforcement paths;
8. immune actions cannot directly mutate MEM or adaptive state;
9. quarantine preserves identity, evidence, audit, and recovery paths and does not delete memory;
10. recovery and restoration require explicit verification;
11. symptom disappearance alone does not restore authority;
12. false-positive correction and restoration paths operate;
13. privileged action lineage is reconstructable;
14. no participant establishes trust or role solely by self-report;
15. security-relevant failure remains distinguishable from ordinary runtime failure;
16. cognitive content and immune evidence access remain separately authorized;
17. immune components can themselves be restricted and reviewed;
18. duplicate reports do not become independent corroboration;
19. conflicting evidence remains explicit;
20. public conformance does not disclose private heuristics or thresholds.

## 7. Prohibited interpretations

These invariants shall not be interpreted to mean that:

- immune systems own ACS, MEM, adaptive state, runtime, resource management, or bootstrap;
- an immune role creates universal inspection or containment authority;
- emergency status permits unrestricted action;
- quarantine is deletion, guilt, retirement, or destruction;
- restoration is automatic;
- the strongest recommendation always wins;
- immune systems are infallible;
- public conformance requires disclosure of private reasoning;
- private implementation permits violation of public contracts.

## 8. Initial architectural commitments

IMM-0001 establishes durable public requirements that:

1. evidence, assessment, recommendation, authorization, enforcement, recovery, restoration, and audit remain distinguishable;
2. authority is explicit, external to self-report, scoped, revocable, and lifecycle-bound;
3. containment is bounded, attributable, proportionate, and reviewable;
4. uncertainty and degradation remain honest;
5. ACS and MEM authority remain intact;
6. adaptive state cannot be directly rewritten by immune action;
7. quarantine preserves identity and does not imply deletion;
8. recovery and restoration require verification;
9. false-positive handling and immune accountability are mandatory;
10. public contracts support private implementation without exposing private heuristics or thresholds.

## Revision history

### Version 0.1 — 2026-07-16

- Defined the initial durable public IMM invariants.
- Preserved ACS and MEM authority boundaries.
- Required explicit uncertainty, bounded containment, verified restoration, false-positive handling, complete privileged-action lineage, and public/private separation.
