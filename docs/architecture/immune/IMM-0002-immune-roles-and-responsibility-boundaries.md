# IMM-0002: Immune Roles and Responsibility Boundaries

**Status:** Draft  
**Series:** Public Immune Architecture  
**Classification:** Public architecture contract  
**Depends on:** IMM-0000, IMM-0001  
**Related architecture:** ACS-0006 through ACS-0009; MEM-0008 through MEM-0010  
**Supersedes:** None

---

## 1. Abstract

This document defines the logical roles that participate in Node’s immune architecture and the responsibility boundaries between them.

The immune architecture observes conditions, develops evidence, assesses possible harm, recommends responses, requests authorization, coordinates authorized containment, verifies recovery, verifies restoration, and audits the resulting process.

It does not independently create authority.

Immune roles do not replace the Adaptive Connection Substrate, the memory architecture, runtime management, resource ownership, or external governance. They must use the mechanisms and authority supplied by those systems.

This document defines roles rather than processes, services, nodes, threads, or deployment units. A conforming implementation may distribute, replicate, combine, or relocate roles, provided that logical responsibility boundaries, authority separation, evidence lineage, and accountability remain intact.

---

## 2. Purpose

The purpose of this document is to establish:

1. The public logical roles of the immune architecture.
2. The responsibilities assigned to each role.
3. The actions each role may and may not perform.
4. The separation between observation, assessment, recommendation, authorization, enforcement, recovery, restoration, and audit.
5. The rules for combining or distributing roles.
6. The behavior required when a role is unavailable, degraded, stale, conflicted, or compromised.
7. The accountability requirements that apply to immune participants themselves.

This document prevents an immune implementation from collapsing observation, judgment, authority, and enforcement into an unreviewable control path.

---

## 3. Normative Language

The terms **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **MAY**, and **OPTIONAL** are normative.

A role is a logical responsibility boundary.

A component is an implementation unit that may perform one or more roles.

An authority is a separately established right to permit, deny, constrain, or revoke an action.

Possessing a role does not imply possessing authority.

---

## 4. Scope

This document defines the following immune roles:

1. Observer
2. Evidence Validator
3. Correlator
4. Assessor
5. Recommender
6. Authorization Requester
7. Containment Coordinator
8. Enforcement Adapter
9. Recovery Verifier
10. Restoration Verifier
11. Auditor

These roles cover the public immune workflow from initial observation through post-restoration review.

This document also defines:

- Role composition.
- Role replication.
- Separation-of-duty requirements.
- Conflicting outputs.
- Degraded role availability.
- Immune self-accountability.
- Role-level conformance requirements.

---

## 5. Explicit Non-Goals

This document does not define:

- Detection algorithms.
- Anomaly thresholds.
- Confidence formulas.
- Evidence scoring weights.
- Reputation calculations.
- Private escalation triggers.
- Model architectures.
- Secret indicators of compromise.
- Cryptographic key material.
- ACS admission rules.
- ACS trust establishment.
- ACS capability semantics.
- ACS delegation rules.
- ACS revocation rules.
- ACS enforcement mechanisms.
- Memory persistence semantics.
- Memory custody rules.
- Memory retention rules.
- Memory reconstruction procedures.
- Memory deletion procedures.
- Runtime scheduling.
- Resource allocation.
- Hardware-specific health thresholds.
- A centralized immune controller.
- A mandatory incident-response sequence.
- A required physical deployment topology.

Those concerns belong to other architecture documents, implementation-private specifications, or future immune documents.

---

## 6. Role Model

### 6.1 Roles are logical

A role is defined by:

- The information it may consume.
- The conclusions or requests it may produce.
- The authority it may invoke.
- The actions it is prohibited from performing.
- The accountability records it must produce.

A role is not defined by:

- A process identifier.
- A network address.
- A machine.
- A container.
- A thread.
- A particular source file.
- A specific language or runtime.

One component may perform multiple roles, and one role may be performed by multiple components.

Logical boundaries remain in force even when roles are colocated.

### 6.2 No central immune brain is required

The immune architecture MUST support distributed participation.

Roles MAY operate:

- On the observed node.
- On a neighboring node.
- On a dedicated supervisory node.
- Across multiple independent nodes.
- Through replicated services.
- Through temporarily available mesh participants.

No single role instance is assumed to be globally authoritative.

No role may infer global completeness merely because it has completed its local work.

### 6.3 Roles do not create authority

A role assignment grants permission to perform that role’s defined work.

It does not grant permission to:

- Admit or reject a connection.
- Establish or revoke trust.
- Grant or withdraw capabilities.
- Modify delegation.
- Disconnect a participant.
- Quarantine a participant.
- Alter persistent memory.
- Delete memory.
- Rewrite adaptive state.
- Seize resources.
- Restore a contained participant.
- Override governance.

Such actions require authority established outside the role itself.

### 6.4 Components may hold multiple roles

A component MAY hold multiple immune roles when necessary for deployment efficiency or availability.

Combining roles MUST NOT:

- Merge their outputs into an untraceable decision.
- Remove required authorization.
- Convert a recommendation into permission.
- Allow a component to approve its own request.
- Allow an enforcement action to exceed its authorization.
- Conceal reduced independence.
- Conceal degraded assurance.
- Allow the component to become its sole source of exonerating evidence.

Where role combination reduces independence, that reduction MUST be explicit in the resulting records.

---

## 7. External Authorities and Adjacent Systems

The following responsibilities are outside the immune role model.

### 7.1 Governing authorization source

A governing authorization source decides whether a requested action is permitted.

It may be implemented through:

- Human authority.
- Machine policy.
- Delegated authority.
- Pre-authorized emergency policy.
- Another governed decision mechanism.

The authorization source is not created by this document and is not inherently an immune role.

An authorization source MUST operate through authority already recognized by the relevant architecture.

### 7.2 Adaptive Connection Substrate

ACS remains authoritative for its established responsibilities, including:

- Connection identity.
- Admission.
- Trust.
- Capability.
- Delegation.
- Revocation.
- Connection lifecycle.
- Communication restrictions.
- ACS enforcement.

Immune roles may observe ACS information, request ACS action, coordinate authorized ACS action, and verify the results.

Immune roles MUST NOT independently redefine or bypass ACS contracts.

### 7.3 Memory architecture

The memory architecture remains authoritative for:

- Memory identity.
- Persistence.
- Provenance.
- Custody.
- Retention.
- Recovery and reconstruction.
- Deletion.
- Memory-operation semantics.

Immune roles may identify memory-related concerns, submit evidence, recommend action, and request authorized memory operations.

Immune roles MUST NOT directly rewrite, erase, reconstruct, relocate, or reclassify memory outside MEM contracts.

### 7.4 Runtime and resource owners

Runtime and resource-management systems remain responsible for:

- Execution.
- Scheduling.
- Placement.
- Resource allocation.
- Device ownership.
- Process lifecycle.
- Hardware-specific recovery work.

Immune roles may recommend or request runtime changes.

They do not acquire runtime or resource authority merely by identifying a risk.

---

## 8. Immune Work Products

Each role produces a distinct class of work product.

| Role | Primary work product |
|---|---|
| Observer | Observation |
| Evidence Validator | Validation statement |
| Correlator | Correlated evidence set |
| Assessor | Assessment |
| Recommender | Recommendation |
| Authorization Requester | Authorization request |
| Containment Coordinator | Authorized action plan |
| Enforcement Adapter | Enforcement request and execution receipt |
| Recovery Verifier | Recovery verification |
| Restoration Verifier | Restoration verification |
| Auditor | Audit finding |

These work products MUST remain distinguishable.

A downstream role may reference or transform an upstream work product, but it MUST NOT silently relabel one class as another.

An observation is not an assessment.

An assessment is not a recommendation.

A recommendation is not authorization.

An authorization is not proof that execution succeeded.

Successful execution is not proof that recovery occurred.

Recovery is not proof that restoration is safe.

Restoration is not permission to erase incident history.

---

## 9. Role Definitions

## 9.1 Observer

### 9.1.1 Responsibility

The Observer detects, receives, or records information that may be relevant to immune evaluation.

Observations may originate from:

- Local component state.
- Hardware state.
- Runtime state.
- ACS state.
- Memory-operation results.
- Peer reports.
- Audit records.
- Environmental signals.
- Explicit fault reports.
- Failed or incomplete operations.

### 9.1.2 Required behavior

An Observer MUST:

- Identify the observed subject.
- Identify the observation source.
- Preserve the distinction between direct and reported observation.
- Record known incompleteness.
- Record known staleness.
- Preserve unknown values as unknown.
- Avoid presenting interpretation as raw observation.
- Provide sufficient provenance for later validation.

### 9.1.3 Permitted actions

An Observer MAY:

- Collect allowed telemetry.
- Receive reports.
- Normalize representation without changing meaning.
- Mark an observation partial, stale, unavailable, or conflicted.
- Forward observations to downstream roles.

### 9.1.4 Prohibited actions

An Observer MUST NOT:

- Declare a subject compromised solely because an anomaly was observed.
- Convert an observation directly into authority.
- Enforce containment.
- Revoke ACS trust or capability.
- Modify memory.
- Rewrite the observed subject to make the observation disappear.
- Represent absence of observation as proof of health.
- Represent self-report as independently verified fact.

---

## 9.2 Evidence Validator

### 9.2.1 Responsibility

The Evidence Validator evaluates whether an observation or evidence item is usable for a stated immune purpose.

Validation concerns the evidence item, not the guilt, safety, or intent of the subject.

### 9.2.2 Required behavior

An Evidence Validator MUST evaluate, where applicable:

- Provenance.
- Attribution.
- Integrity.
- Completeness.
- Freshness.
- Scope.
- Collection context.
- Internal consistency.
- Dependence on other evidence.
- Known limitations.

The validator MUST express uncertainty and unresolved limitations.

### 9.2.3 Permitted actions

An Evidence Validator MAY classify evidence as:

- Validated for a stated purpose.
- Partially validated.
- Unvalidated.
- Stale.
- Conflicted.
- Unavailable.
- Invalid for a stated purpose.

Validation for one purpose MUST NOT imply validation for all purposes.

### 9.2.4 Prohibited actions

An Evidence Validator MUST NOT:

- Declare the observed subject compromised merely because evidence is valid.
- Treat invalid evidence as proof that the subject is healthy.
- Invent missing provenance.
- Suppress contradictory evidence.
- Authorize containment.
- Enforce a restriction.
- Change ACS or MEM state.

---

## 9.3 Correlator

### 9.3.1 Responsibility

The Correlator identifies relationships between observations and evidence items.

Correlation may identify:

- Shared origin.
- Shared target.
- Temporal association.
- Causal possibility.
- Duplicate reporting.
- Dependency.
- Contradiction.
- Reinforcement.
- Common collection failure.
- Common upstream source.

### 9.3.2 Required behavior

A Correlator MUST:

- Preserve the identities of source evidence.
- Distinguish independent evidence from repeated reports of the same evidence.
- Preserve contradictions.
- Preserve uncertainty.
- Identify known dependencies.
- Avoid counting duplicated evidence as independent confirmation.

### 9.3.3 Permitted actions

A Correlator MAY:

- Group evidence.
- Link related evidence.
- Identify repeated evidence.
- Identify possible incident scope.
- Identify subjects that may share a causal dependency.
- Produce multiple competing correlation sets.

### 9.3.4 Prohibited actions

A Correlator MUST NOT:

- Convert correlation into proof of causation.
- Resolve contradictions by silently discarding evidence.
- Declare a final verdict.
- Grant authority.
- Initiate enforcement.
- Broaden a containment target without authorization.

---

## 9.4 Assessor

### 9.4.1 Responsibility

The Assessor interprets available evidence and produces an explicit assessment of a condition, risk, failure, or possible threat.

An assessment is a reasoned conclusion with uncertainty.

It is not an authorization.

### 9.4.2 Required behavior

An Assessor MUST:

- Identify the evidence used.
- Identify material missing evidence.
- Identify conflicting evidence.
- State the assessed subject and scope.
- Distinguish observed fact from inference.
- Distinguish degradation from compromise.
- Express uncertainty.
- Identify whether the assessment is preliminary, partial, stale, or superseded.
- Preserve alternative plausible explanations when material.

### 9.4.3 Permitted actions

An Assessor MAY assess:

- Health degradation.
- Runtime failure.
- Security concern.
- Suspected compromise.
- Communication abnormality.
- Memory-operation concern.
- Evidence insufficiency.
- Recovery progress.
- Possible immune-system failure.

Multiple assessors MAY produce different assessments.

Disagreement MUST remain explicit.

### 9.4.4 Prohibited actions

An Assessor MUST NOT:

- Convert suspicion into fact without evidentiary support.
- Grant itself authority.
- Enforce containment.
- Rewrite ACS trust state.
- Rewrite memory state.
- Hide an assessment’s uncertainty.
- Treat lack of contradictory evidence as proof.
- Treat its own prior assessment as independent evidence.

---

## 9.5 Recommender

### 9.5.1 Responsibility

The Recommender proposes a bounded response based on one or more assessments.

A recommendation explains what action may be appropriate and why.

It does not permit the action.

### 9.5.2 Required behavior

A Recommender MUST:

- Identify the assessment or assessments supporting the recommendation.
- Identify the proposed target.
- Identify the proposed action class.
- State the intended objective.
- Bound the requested scope.
- Identify foreseeable collateral effects.
- Identify essential paths that must be preserved.
- Identify review or exit conditions where known.
- Identify material alternatives where appropriate.
- Preserve uncertainty inherited from the assessment.

### 9.5.3 Permitted actions

A Recommender MAY recommend:

- Additional observation.
- Evidence revalidation.
- Reduced trust consideration through ACS.
- Capability restriction through ACS.
- Connection containment through ACS.
- Runtime isolation through the responsible runtime.
- Resource restriction through the responsible resource owner.
- Memory inspection or governed recovery through MEM.
- Continued containment.
- Recovery evaluation.
- Restoration evaluation.
- No action.
- Human review.

### 9.5.4 Prohibited actions

A Recommender MUST NOT:

- Present a recommendation as an authorization.
- Directly execute the proposed response.
- Omit known material consequences.
- Widen the assessed subject without explanation.
- Recommend memory deletion as a substitute for quarantine.
- Define new ACS or MEM authority.

---

## 9.6 Authorization Requester

### 9.6.1 Responsibility

The Authorization Requester converts a recommendation into a request addressed to an appropriate governing authority.

The requester is responsible for accurately carrying the requested action and its supporting lineage.

### 9.6.2 Required behavior

An Authorization Requester MUST include or reference:

- The recommendation.
- Supporting assessment lineage.
- Supporting evidence lineage.
- The requested action.
- The requested target.
- Requested scope.
- Requested duration or review condition where applicable.
- Requested constraints.
- Known collateral effects.
- The authority being asked to decide.
- The identity of the requester.

### 9.6.3 Permitted actions

An Authorization Requester MAY:

- Ask for approval.
- Ask for denial.
- Ask for narrower scope.
- Ask for emergency review.
- Ask for renewal.
- Ask for containment release.
- Ask for restoration.
- Withdraw a pending request.

### 9.6.4 Prohibited actions

An Authorization Requester MUST NOT:

- Approve its own request by virtue of being the requester.
- Represent a pending request as approved.
- Alter the recommendation without preserving the change.
- Conceal rejected or narrowed authorization.
- Reuse expired authorization as current authority.
- Treat silence as approval.
- Execute the requested action.

A pre-authorized policy may reduce decision latency, but it does not eliminate the requirement for explicit authority lineage.

Pre-authorization may collapse timing.

It MUST NOT collapse authority.

---

## 9.7 Containment Coordinator

### 9.7.1 Responsibility

The Containment Coordinator translates an approved containment decision into a bounded action plan and coordinates its execution through the systems that possess enforcement authority.

The coordinator coordinates containment.

It does not independently create or expand containment authority.

### 9.7.2 Required behavior

A Containment Coordinator MUST:

- Verify that authorization exists.
- Verify the authorized target.
- Verify the authorized action class.
- Verify scope and constraints.
- Verify expiration or review conditions.
- Preserve essential communication or recovery paths required by the authorization.
- Sequence actions without exceeding the authorization.
- Track partial success and failure.
- Stop or request review when the authorized envelope cannot be followed.
- Preserve action lineage.

### 9.7.3 Permitted actions

A Containment Coordinator MAY:

- Divide an authorized response into bounded steps.
- Select among equivalent permitted enforcement paths.
- Coordinate multiple ACS, runtime, resource, or MEM requests.
- Pause execution when conditions change.
- Request narrower, broader, renewed, or replacement authorization.
- Coordinate authorized containment review.
- Coordinate authorized release or restoration activity.

### 9.7.4 Prohibited actions

A Containment Coordinator MUST NOT:

- Add targets not covered by the authorization.
- Increase severity beyond the authorization.
- Extend duration without authority.
- Remove required safety or recovery paths.
- Perform ACS enforcement directly unless it separately performs the Enforcement Adapter role.
- Treat incomplete execution as complete.
- Treat containment as proof of compromise.
- Convert quarantine into deletion.
- Conceal failed containment steps.

---

## 9.8 Enforcement Adapter

### 9.8.1 Responsibility

The Enforcement Adapter invokes an authorized action through the system that owns the relevant enforcement mechanism.

For ACS-governed actions, the adapter invokes ACS mechanisms.

For runtime, resource, or MEM requests, the adapter invokes the responsible architecture’s governed interface.

The adapter does not replace those systems.

### 9.8.2 Required behavior

Before invocation, an Enforcement Adapter MUST verify:

- The authorization source.
- The authorization’s validity.
- The target.
- The action class.
- Scope.
- Constraints.
- Expiration or review conditions.
- That the requested interface is appropriate.
- That the action has not already been superseded or revoked.

After invocation, the adapter MUST produce or preserve an execution receipt identifying:

- What was requested.
- Where it was requested.
- What was accepted.
- What was rejected.
- What completed.
- What failed.
- What remains unknown.

### 9.8.3 Permitted actions

An Enforcement Adapter MAY:

- Submit an authorized request.
- Retry where retry is permitted.
- Report partial enforcement.
- Report unavailable enforcement.
- Stop when authorization is withdrawn.
- Request fresh authorization when the existing authorization is insufficient.

### 9.8.4 Prohibited actions

An Enforcement Adapter MUST NOT:

- Invent authority.
- Widen the target.
- Widen the action.
- Extend authorization duration.
- Substitute a more severe action without authorization.
- Bypass ACS for ACS-owned enforcement.
- Directly mutate memory outside MEM operations.
- Hide partial or failed execution.
- Treat successful invocation as proof of intended system state.
- Continue after authorization has expired, been revoked, or become inapplicable.

---

## 9.9 Recovery Verifier

### 9.9.1 Responsibility

The Recovery Verifier evaluates whether the subject has completed the recovery conditions relevant to the incident or failure.

The Recovery Verifier verifies recovery.

It does not perform the recovery work and does not automatically authorize restoration.

### 9.9.2 Required behavior

A Recovery Verifier MUST:

- Identify the recovery conditions being evaluated.
- Identify the evidence used.
- Identify unresolved conditions.
- Distinguish symptom disappearance from verified recovery.
- Distinguish repaired function from restored trust.
- Distinguish local recovery from system-wide recovery.
- Identify stale or incomplete recovery evidence.
- Report whether recovery is verified, partial, unverified, failed, or unknown.

### 9.9.3 Permitted actions

A Recovery Verifier MAY:

- Request additional evidence.
- Request repeated testing.
- Verify a subset of recovery conditions.
- Reject unsupported recovery claims.
- Recommend continued containment.
- Recommend restoration evaluation.
- Report that recovery cannot currently be verified.

### 9.9.4 Prohibited actions

A Recovery Verifier MUST NOT:

- Perform ungoverned repair.
- Restore ACS capabilities.
- Lift containment.
- Rewrite memory.
- Erase incident history.
- Treat elapsed time as proof of recovery.
- Treat silence as proof of recovery.
- Clear itself solely on the basis of its own self-report.

---

## 9.10 Restoration Verifier

### 9.10.1 Responsibility

The Restoration Verifier evaluates whether an authorized restoration completed as intended and whether the resulting state satisfies the applicable restoration conditions.

Recovery verification occurs before or during a restoration decision.

Restoration verification evaluates the result of restoration.

### 9.10.2 Required behavior

A Restoration Verifier MUST:

- Identify the restoration authorization.
- Identify the restoration actions performed.
- Confirm which restrictions were lifted.
- Confirm which restrictions remain.
- Identify unexpected residual restrictions.
- Identify unexpected re-enabled access.
- Verify required communication, capability, runtime, resource, or memory conditions.
- Report partial, failed, unknown, or complete restoration.
- Preserve the incident and restoration lineage.

### 9.10.3 Permitted actions

A Restoration Verifier MAY:

- Recommend further restoration.
- Recommend renewed containment.
- Recommend narrower access.
- Request additional verification.
- Identify residual damage.
- Identify unsafe restoration.
- Identify restoration drift from the authorized plan.

### 9.10.4 Prohibited actions

A Restoration Verifier MUST NOT:

- Treat authorization as proof that restoration succeeded.
- Treat successful reconnection as proof of restored trust.
- Clear incident history.
- Delete evidence.
- Grant capability.
- Restore access without authorization.
- Expand restoration beyond the authorized scope.

---

## 9.11 Auditor

### 9.11.1 Responsibility

The Auditor examines whether immune work complied with architecture contracts, authority boundaries, evidence requirements, and declared policy.

Audit may occur during or after an incident.

The Auditor does not become the authority merely by identifying nonconformance.

### 9.11.2 Required behavior

An Auditor MUST be able to examine, where available:

- Role attribution.
- Observation lineage.
- Evidence validation.
- Correlation lineage.
- Assessment reasoning.
- Recommendation scope.
- Authorization lineage.
- Containment planning.
- Enforcement receipts.
- Recovery verification.
- Restoration verification.
- Known degraded or unavailable roles.
- Conflicts of interest.
- Role combinations.
- Unresolved failures.
- Changes to scope or authority.

### 9.11.3 Permitted actions

An Auditor MAY:

- Produce conformance findings.
- Identify missing records.
- Identify unsupported authority.
- Identify excessive scope.
- Identify evidence-handling failures.
- Identify role conflicts.
- Recommend remediation.
- Recommend review of an immune participant.
- Supply audit evidence to a new immune process.

### 9.11.4 Prohibited actions

An Auditor MUST NOT:

- Rewrite history.
- Delete unfavorable evidence.
- Convert a finding directly into enforcement authority.
- Hide audit gaps.
- Claim completeness when relevant records were unavailable.
- Exempt immune components from review.
- Become the sole judge of its own conduct.

Persistent audit records remain governed by MEM architecture.

---

## 10. Separation of Duties

### 10.1 Logical separation is mandatory

The following distinctions MUST always remain explicit:

- Observation versus validation.
- Validation versus assessment.
- Assessment versus recommendation.
- Recommendation versus authorization.
- Authorization request versus authorization decision.
- Authorization versus enforcement.
- Enforcement versus recovery verification.
- Recovery verification versus restoration authorization.
- Restoration action versus restoration verification.
- Operational activity versus audit.

These distinctions remain mandatory even when the same component performs several roles.

### 10.2 Authorization is external to the role chain

No immune role defined in this document is inherently an authorization source.

A component that separately possesses governing authority MUST identify when it is acting under that authority rather than under its immune role.

The two capacities MUST remain attributable and distinguishable.

### 10.3 Request and approval separation

An Authorization Requester MUST NOT infer approval from its own request.

Where the same physical component participates in both requesting and governed decision-making, the decision MUST be attributable to a separately established authority context.

### 10.4 Decision and enforcement separation

Assessment and recommendation MUST NOT be treated as enforcement instructions.

The Enforcement Adapter MUST require valid authority regardless of the identity or confidence of the assessor.

### 10.5 Action and verification separation

The actor that performs enforcement SHOULD NOT be the sole verifier of recovery or restoration for consequential actions.

When deployment constraints require colocation:

- The combined roles MUST remain logically distinct.
- The reduced independence MUST be recorded.
- Self-generated success reports MUST NOT be the sole evidence.
- External or independently derived evidence SHOULD be obtained where available.

### 10.6 Evidence generation and validation

An evidence producer MAY also participate in validation, but self-validation MUST NOT be presented as independent confirmation.

Evidence created by a component MUST NOT be the sole basis for:

- Establishing that component’s trustworthiness.
- Clearing that component from containment.
- Proving that its own enforcement action succeeded.
- Proving that its own recovery is complete.

---

## 11. Role Composition

### 11.1 Permitted composition

Roles MAY be combined to support:

- Minimal nodes.
- Intermittently connected nodes.
- Headless deployments.
- Embedded systems.
- Temporary mesh partitions.
- Reduced-resource operation.
- Emergency operation.

Composition MUST preserve all normative boundaries.

### 11.2 Role declarations

A component performing immune work MUST make its active role or roles attributable.

A work product MUST identify the role under which it was produced.

A component MUST NOT rely on an ambiguous general label such as “immune service” when the distinction affects authority or interpretation.

### 11.3 Dynamic role assignment

Roles MAY be assigned dynamically.

Dynamic assignment MUST NOT:

- Create authority that the assignee did not already possess.
- Erase the prior role holder’s records.
- Make stale outputs current.
- Conceal a gap in role coverage.
- Allow a role to declare itself uniquely trusted.
- Turn replication count into proof of correctness.

### 11.4 Role replication

A role MAY have multiple active instances.

Replicated outputs may:

- Agree.
- Disagree.
- Overlap.
- Cover different scopes.
- Have different freshness.
- Have different evidence access.

Agreement among replicated roles is not automatically independent confirmation.

Disagreement MUST remain visible until resolved by a governed process.

---

## 12. Conflicts of Interest

A conflict of interest exists when a role’s ability to evaluate a condition may be materially affected by its own prior action, authority, ownership, or exposure.

Examples include:

- An adapter verifying its own enforcement.
- A subject validating its own health report.
- An assessor evaluating evidence it fabricated or transformed without attribution.
- A coordinator auditing its own scope expansion.
- A compromised immune component assessing itself.
- A requester acting as an undeclared approver.
- A restorer acting as the sole restoration verifier.

A conflict of interest does not automatically invalidate the output.

It MUST be disclosed.

The output MUST NOT be represented as independent.

Where independent review is unavailable, assurance MUST be marked reduced, partial, or unknown as appropriate.

---

## 13. Degraded and Unavailable Roles

### 13.1 Explicit role availability

Each required role instance MUST be representable as:

- Available.
- Partially available.
- Degraded.
- Stale.
- Unavailable.
- Unknown.

A missing role MUST NOT be silently simulated by reusing an incompatible work product.

### 13.2 Missing observation

When observation coverage is missing:

- The blind area MUST remain explicit.
- Lack of alerts MUST NOT be interpreted as health.
- Downstream assessments MUST carry the resulting limitation.

### 13.3 Missing evidence validation

Unvalidated evidence may still be retained and considered where policy permits.

It MUST remain identified as unvalidated.

It MUST NOT silently acquire validated status because a response is urgent.

### 13.4 Missing correlation

Evidence MAY proceed without correlation where policy permits.

Independent evidence items MUST remain separate.

A missing correlator MUST NOT be replaced by assuming all evidence shares one cause.

### 13.5 Missing assessment or recommendation

Without a valid assessment, a new recommendation MUST NOT be fabricated from raw observation.

Without a valid recommendation, an authorization request MUST identify the exceptional policy or pre-authorized condition permitting the shortened path.

### 13.6 Missing authorization path

When authorization cannot be obtained:

- A role MUST NOT invent it.
- Pending requests remain pending.
- Previously granted authority may be used only within its original validity and scope.
- Pre-authorized actions may proceed only within the pre-authorized envelope.
- The inability to act MUST remain explicit.

### 13.7 Missing enforcement path

When enforcement is unavailable:

- The action MUST be reported unavailable, partial, failed, or unknown.
- The coordinator MUST NOT report containment as complete.
- Alternative enforcement requires authorization when it changes scope or severity.

### 13.8 Missing recovery verification

Restoration MUST NOT be inferred solely because:

- Symptoms disappeared.
- A timer expired.
- The subject requested release.
- Communication resumed.
- No new alert was observed.

A governing authority may define emergency exceptions, but the absence of verification MUST remain explicit.

### 13.9 Missing restoration verification

An executed restoration request MUST NOT be assumed successful.

The system MUST preserve an unknown or partial restoration state until sufficient verification exists.

### 13.10 Missing audit capability

Missing audit capability MUST be visible.

An implementation MUST NOT fabricate audit completeness after the fact.

Whether a particular action may proceed without available audit capability is determined by external policy and authority, not by the immune role itself.

---

## 14. Immune Self-Accountability

Immune components are not exempt from the systems they observe.

An immune component:

- MAY fail.
- MAY become degraded.
- MAY provide stale information.
- MAY be misconfigured.
- MAY become compromised.
- MAY exceed its authority.
- MAY require containment.
- MAY require recovery.
- MAY require restoration review.
- MAY be audited.

An immune component MUST NOT:

- Declare itself permanently trusted.
- Exempt itself from ACS enforcement.
- Exempt its records from MEM governance.
- Conceal its own degraded state.
- Use its role to prevent external review.
- Clear its own containment solely from self-produced evidence.
- Delete evidence of its own failure.
- Treat immune status as superior authority.

Immune roles may observe and assess other immune roles.

Such evaluation is subject to the same evidence, authority, enforcement, recovery, and audit boundaries defined in this series.

---

## 15. Authority Envelope Requirements

When an immune workflow reaches an authorized action, the authority used MUST be attributable.

At minimum, the applicable authority information MUST identify or reference:

- The authority source.
- The authorized actor or invocation context.
- The authorized target.
- The authorized action class.
- Scope.
- Constraints.
- Validity conditions.
- Expiration or review conditions where applicable.
- Supporting request or reason.
- Supersession or revocation state where applicable.

An authority envelope MUST NOT be interpreted more broadly than its explicit meaning.

Ambiguity MUST NOT be resolved by silently choosing the most severe action.

When authority is insufficient, the appropriate role must request clarification, narrowing, renewal, or replacement.

---

## 16. Workflow Relationship

A common immune workflow may proceed as follows:

1. An Observer records a condition.
2. An Evidence Validator evaluates the resulting evidence.
3. A Correlator relates it to other evidence.
4. An Assessor produces an assessment.
5. A Recommender proposes a bounded response.
6. An Authorization Requester submits the proposal.
7. A governing authority approves, denies, narrows, or modifies the request.
8. A Containment Coordinator creates a plan within the authorization.
9. An Enforcement Adapter invokes the responsible enforcement system.
10. The responsible runtime, ACS, resource, or MEM system performs the authorized work.
11. A Recovery Verifier evaluates recovery.
12. A new recommendation and authorization process addresses restoration.
13. Authorized restoration is executed through the responsible system.
14. A Restoration Verifier evaluates the resulting state.
15. An Auditor reviews the lineage and conformance.

This sequence is illustrative rather than centrally mandatory.

A distributed implementation may perform compatible work concurrently.

A governed emergency policy may shorten the sequence.

No shortened sequence may silently remove:

- Authority lineage.
- Scope limits.
- Execution accountability.
- Recovery uncertainty.
- Restoration verification.
- Auditability required by policy.

---

## 17. Conforming Example

A runtime observer reports repeated execution failures from a node.

The evidence validator confirms the report’s provenance but marks hardware telemetry unavailable.

The correlator identifies similar failures across tasks that share the same accelerator.

The assessor concludes that accelerator degradation is likely but explicitly states that compromise is not established.

The recommender proposes temporarily preventing new workloads from being placed on that accelerator while preserving diagnostic access.

The authorization requester submits the bounded proposal to the responsible resource authority.

The resource authority authorizes temporary scheduling exclusion for the named accelerator only.

The containment coordinator prepares the limited action.

The enforcement adapter invokes the runtime or resource-management interface.

The execution receipt reports that new placement was blocked while existing diagnostic access remained available.

After repair, the recovery verifier confirms successful diagnostic execution but does not claim restored trust.

A separate restoration request is authorized.

The enforcement adapter invokes the resource-management interface to restore scheduling eligibility.

The restoration verifier confirms that scheduling was restored only for the authorized accelerator.

The auditor preserves the complete decision and action lineage.

This sequence is conforming because degradation was not treated as compromise, recommendation remained separate from authorization, enforcement used the responsible system, scope remained bounded, and restoration was independently verified.

---

## 18. Nonconforming Examples

The following behavior is nonconforming:

- An Observer disconnects a peer after detecting an unusual packet.
- A validator labels a node compromised because a report was cryptographically valid.
- A correlator deletes contradictory evidence.
- An assessor converts uncertainty into a definitive verdict without support.
- A recommender directly invokes ACS revocation.
- A requester treats its own request as approval.
- A coordinator adds neighboring nodes to a containment plan without authorization.
- An enforcement adapter continues after authorization expires.
- An adapter reports success merely because an API accepted the request.
- A recovery verifier lifts containment.
- A restoration verifier grants capabilities.
- An auditor deletes records that make the immune system appear unreliable.
- A component uses multiple internal role labels to make one self-produced report appear independently confirmed.
- A compromised immune component exempts itself from containment.
- A role treats missing telemetry as proof that no threat exists.
- A role deletes memory under the label of quarantine.
- A role interprets symptom disappearance as verified recovery.
- A role expands a temporary restriction into indefinite containment without governed review.

---

## 19. Public and Private Boundary

The following role information belongs in public architecture:

- Role names.
- Role purposes.
- Responsibility boundaries.
- Permitted work-product classes.
- Prohibited authority.
- Required attribution.
- Required lineage.
- Separation-of-duty rules.
- Degraded-state behavior.
- Self-accountability.
- Conformance requirements.
- Failure semantics.

The following information SHOULD remain implementation-private unless separately approved for publication:

- Detection thresholds.
- Correlation weights.
- Confidence formulas.
- Source-ranking formulas.
- Private indicators.
- Escalation cutoffs.
- Suppression rules.
- Secret bypass-detection techniques.
- Exact emergency triggers.
- Protected deployment topology.
- Signing keys.
- Credential material.
- Adversarial test details that would materially weaken the system.
- Internal heuristics used to select containment targets or severity.

Private implementation may evolve without changing this public contract.

Private implementation MUST remain conformant with the public role and authority boundaries.

---

## 20. Conformance Requirements

An immune implementation conforms to this document only when:

1. Every immune work product is attributable to a logical role.
2. Role outputs remain distinguishable.
3. Observation is not treated as verdict.
4. Assessment is not treated as authority.
5. Recommendation is not treated as authorization.
6. Authorization is attributable to an external governing source.
7. Enforcement remains within authorized scope.
8. ACS-owned actions use ACS mechanisms.
9. MEM-owned operations use MEM mechanisms.
10. Role combination does not erase logical separation.
11. Reduced independence is disclosed.
12. Missing roles remain explicit.
13. Unknown, partial, stale, conflicted, and unavailable states are preserved.
14. Conflicting evidence or assessments remain visible.
15. Replication is not misrepresented as independent confirmation.
16. Containment coordination does not expand authority.
17. Execution receipts distinguish requested, accepted, completed, failed, and unknown outcomes.
18. Recovery requires explicit verification.
19. Restoration requires authorization and subsequent verification.
20. Incident and authority lineage survives restoration.
21. Immune components remain subject to observation, containment, recovery, and audit.
22. Persistent immune records remain governed by MEM.
23. Private heuristics do not override public authority boundaries.
24. No role grants itself new authority.

---

## 21. Required Failure Semantics

A role failure MUST be representable without falsely implicating the subject it was evaluating.

Failure of:

- An Observer does not prove subject health or compromise.
- A Validator does not invalidate the subject.
- A Correlator does not prove evidence independence.
- An Assessor does not prove the opposite assessment.
- A Recommender does not authorize action.
- An Authorization Requester does not imply denial or approval.
- A Coordinator does not prove containment failed or succeeded.
- An Enforcement Adapter does not prove the target resisted enforcement.
- A Recovery Verifier does not prove recovery failed.
- A Restoration Verifier does not prove restoration failed.
- An Auditor does not erase the underlying history.

Immune-role failure is evidence about the immune system.

It is not automatically evidence against the evaluated subject.

---

## 22. Architectural Commitments

This document establishes the following commitments:

1. Immune architecture is role-based rather than centralized by definition.
2. Roles are logical and may be distributed, replicated, or colocated.
3. No immune role inherently creates governing authority.
4. ACS remains authoritative for ACS-owned connection and enforcement concerns.
5. MEM remains authoritative for MEM-owned persistence and lifecycle concerns.
6. Runtime and resource owners remain authoritative for their operations.
7. Observation, assessment, recommendation, authorization, and enforcement remain distinct.
8. Containment coordination cannot enlarge an authorization.
9. Recovery verification does not restore a subject.
10. Restoration verification does not grant authority.
11. Audit does not rewrite history.
12. Immune components remain accountable and containable.
13. Missing or degraded roles remain explicit.
14. Public role contracts remain stable while private heuristics may evolve.
15. A conforming implementation preserves evidence lineage, authority lineage, and action lineage from observation through restoration.

---

## 23. Future Work

Later immune architecture documents may define:

- Observation and evidence contracts.
- Assessment and recommendation contracts.
- Authorization-request contracts.
- Containment lifecycle.
- Recovery and restoration protocols.
- Immune audit records.
- Cross-node evidence exchange.
- Immune role discovery.
- Replicated assessment handling.
- Immune-system health and self-protection.
- Conformance and adversarial validation.

Those documents must preserve the role and authority boundaries established here.
