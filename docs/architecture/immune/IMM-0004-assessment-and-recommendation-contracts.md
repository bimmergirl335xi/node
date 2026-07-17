# IMM-0004: Assessment and Recommendation Contracts

**Status:** Draft  
**Series:** Public Immune Architecture  
**Classification:** Public architecture contract  
**Depends on:** IMM-0000, IMM-0001, IMM-0002, IMM-0003  
**Related architecture:** ACS-0006 through ACS-0009; MEM-0003 through MEM-0010  
**Supersedes:** None

---

## 1. Abstract

This document defines the public contracts by which Node’s immune architecture converts evidence into assessments and converts assessments into bounded recommendations.

An assessment is an attributable interpretation of available evidence concerning a defined subject, condition, risk, failure, or possible threat.

A recommendation is an attributable proposal for a bounded response, further investigation, continued observation, recovery evaluation, restoration evaluation, or deliberate non-action.

An assessment is not a verdict.

A recommendation is not an authorization.

Neither assessment confidence, evidence volume, role privilege, urgency, nor apparent severity creates authority to alter ACS state, memory state, runtime state, resource ownership, adaptive state, or any other governed system condition.

This document defines the required structure, uncertainty semantics, scope limits, conflict handling, supersession rules, and failure behavior of assessments and recommendations. It does not define private detection heuristics, scoring models, threat thresholds, authorization policy, or enforcement mechanisms.

---

## 2. Purpose

The purpose of this document is to establish:

1. What constitutes an immune assessment.
2. What constitutes an immune recommendation.
3. The required lineage between evidence, assessment, and recommendation.
4. How observed facts remain distinguishable from inference.
5. How uncertainty, incompleteness, conflict, freshness, and alternative explanations are represented.
6. How degradation, failure, suspicious behavior, possible compromise, and confirmed conditions remain distinct.
7. How multiple assessors and recommenders may agree, disagree, abstain, or produce competing outputs.
8. How recommendations remain bounded by target, objective, action class, scope, duration, constraints, and review conditions.
9. How collateral effects, essential paths, reversibility, and recovery needs are represented.
10. How emergency conditions affect timing without collapsing authority.
11. How assessment and recommendation failures are represented.
12. How ACS, MEM, runtime, resource-management, adaptive-state, and governing authorization boundaries remain preserved.
13. Which details belong in public architecture and which remain implementation-private.

This document prevents evidence interpretation from becoming an opaque verdict and prevents a proposed response from becoming an undeclared enforcement command.

---

## 3. Normative Language

The terms **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **MAY**, and **OPTIONAL** are normative.

The following terms are used throughout this document.

**Assessment**  
An attributable interpretation of evidence concerning a bounded subject and purpose.

**Assessment claim**  
A proposition asserted within an assessment.

**Finding**  
A conclusion supported by the assessment’s declared evidence and reasoning.

**Alternative explanation**  
A materially plausible interpretation that differs from the assessment’s primary conclusion.

**Assessment state**  
The public semantic state describing the assessment’s completeness, certainty, freshness, conflict, or disposition.

**Recommendation**  
An attributable proposal for a bounded action, investigation, observation, review, recovery step, restoration step, or non-action.

**Recommendation target**  
The subject or bounded system element to which the proposed response would apply.

**Recommended action class**  
The public category of action being proposed without defining implementation-private execution detail.

**Objective**  
The intended outcome of the recommendation.

**Constraint**  
A condition that limits how the recommendation may be considered or carried out.

**Essential path**  
A communication, diagnostic, recovery, safety, governance, memory, or operational path that should remain available during a proposed response.

**Review condition**  
A condition requiring reevaluation of an assessment, recommendation, authorization, or continuing action.

**Exit condition**  
A condition under which a proposed or authorized response should end, narrow, transition, or enter restoration review.

**Abstention**  
An explicit decision not to produce a substantive assessment or recommendation because the responsible role cannot do so conformingly.

**No-action recommendation**  
An affirmative recommendation that no new intervention should presently be requested.

Abstention and a no-action recommendation are distinct.

---

## 4. Scope

This document defines public contracts for:

- Assessment identity.
- Assessment purpose.
- Assessment subject.
- Evidence lineage.
- Fact and inference separation.
- Assessment claims.
- Assessment scope.
- Assessment time.
- Assessment state.
- Uncertainty.
- Alternative explanations.
- Confidence representation.
- Evidence sufficiency.
- Missing and conflicting evidence.
- Multiple assessments.
- Assessment revision and supersession.
- Recommendation identity.
- Recommendation objective.
- Recommendation target.
- Recommended action class.
- Recommendation scope.
- Recommendation constraints.
- Collateral effects.
- Essential paths.
- Duration and review conditions.
- Exit conditions.
- Reversibility.
- Follow-up evidence.
- Recommendation conflict.
- Recommendation withdrawal.
- Emergency recommendations.
- No-action recommendations.
- Abstention.
- Assessment and recommendation failure.
- Cross-architecture responsibility boundaries.
- Public and private classification boundaries.

This document applies to assessments and recommendations concerning:

- Node health.
- Component health.
- Runtime failure.
- Hardware degradation.
- Communication abnormality.
- ACS-observed behavior.
- Security concerns.
- Suspected compromise.
- Memory-operation concerns.
- Resource conditions.
- Recovery readiness.
- Restoration readiness.
- Immune-role performance.
- Immune-system degradation.
- Audit findings that require further immune evaluation.

---

## 5. Explicit Non-Goals

This document does not define:

- Detection algorithms.
- Classifiers.
- Model architectures.
- Anomaly formulas.
- Threat scores.
- Risk scores.
- Confidence formulas.
- Evidence weights.
- Reputation calculations.
- Correlation algorithms.
- Private indicators of compromise.
- Escalation thresholds.
- Automatic response thresholds.
- Severity cutoffs.
- Secret emergency triggers.
- ACS admission policy.
- ACS trust policy.
- ACS capability policy.
- ACS delegation policy.
- ACS revocation policy.
- ACS enforcement mechanisms.
- Memory persistence semantics.
- Memory custody rules.
- Memory retention periods.
- Memory reconstruction methods.
- Memory deletion authority.
- Runtime scheduling policy.
- Resource-allocation policy.
- Hardware-specific limits.
- Authorization-decision policy.
- A mandatory human approval requirement.
- A universal numeric confidence system.
- A universal threat taxonomy.
- A centralized assessment authority.
- A mandatory single recommender.
- An implementation-specific response playbook.

Those concerns belong to private implementation, ACS, MEM, runtime architecture, resource management, governing policy, or later immune specifications.

---

## 6. Foundational Rules

### 6.1 Assessment is interpretation

An assessment interprets evidence.

It MUST identify which statements are:

- Observed facts.
- Source claims.
- Derived facts.
- Inferences.
- Assumptions.
- Unknowns.
- Alternative explanations.
- Conclusions.

An assessment MUST NOT present inference as direct observation.

### 6.2 Assessment is not authority

An assessment may conclude that a condition is dangerous, urgent, suspicious, degraded, or likely compromised.

That conclusion does not grant authority to:

- Disconnect a participant.
- Deny admission.
- Revoke trust.
- Restrict capability.
- Alter delegation.
- Contain a connection.
- Quarantine a node.
- Stop a runtime.
- Seize a resource.
- Rewrite memory.
- Delete evidence.
- Rewrite adaptive state.
- Restore access.

Authority must arise through the appropriate governing architecture and authorization process.

### 6.3 Recommendation is not authorization

A recommendation proposes what should be considered.

It does not permit the proposed action.

A recommendation MUST remain distinguishable from:

- A request for authorization.
- An authorization decision.
- An enforcement request.
- An executed action.
- A recovery verification.
- A restoration verification.

### 6.4 Confidence is not authority

High confidence does not create authority.

Low confidence does not automatically prohibit all action.

Confidence informs interpretation and governance.

It does not replace governance.

### 6.5 Urgency does not erase lineage

Urgency may reduce latency.

Urgency MUST NOT erase:

- Evidence lineage.
- Assessment attribution.
- Recommendation attribution.
- Target scope.
- Requested action class.
- Authority lineage.
- Constraints.
- Accountability.

### 6.6 Missing evidence remains missing

An assessment MUST NOT silently replace missing evidence with assumptions favorable to either intervention or non-intervention.

Missing evidence may increase uncertainty, reduce scope, cause abstention, or motivate further collection.

### 6.7 Competing explanations remain visible

A primary assessment MAY identify one explanation as more strongly supported.

Material alternatives MUST remain visible when the evidence does not reasonably exclude them.

### 6.8 Recommendation should be proportionate and bounded

A recommendation SHOULD propose no more intervention than is reasonably necessary to achieve its declared objective within the known evidence and uncertainty.

This principle does not define an authorization policy.

It defines the required character of a conforming recommendation.

### 6.9 Recommendation must preserve recovery possibility

A recommendation SHOULD preserve the ability to:

- Observe the target.
- Diagnose the condition.
- Communicate with governing authority.
- Perform authorized recovery.
- Verify recovery.
- Conduct restoration review.
- Audit the incident.

A recommendation that removes all recovery paths MUST explicitly state that consequence and the authority implications.

---

## 7. Assessment Contract

Every assessment MUST be attributable as a distinct logical work product.

An assessment contains or references the following information where applicable.

### 7.1 Assessment identity

An assessment MUST have an identity sufficient to support:

- Evidence reference.
- Recommendation reference.
- Revision.
- Supersession.
- Conflict reporting.
- Authorization lineage.
- Audit.
- Recovery review.
- Restoration review.

An assessment whose meaning materially changes MUST NOT silently retain an identity implying unchanged content.

### 7.2 Assessor identity and role

The Assessor that produced the assessment MUST be attributable.

The work product MUST identify that it was produced under the Assessor role defined by IMM-0002.

Where the component also performs other roles, the assessment MUST remain distinguishable from work produced under those roles.

### 7.3 Purpose

An assessment MUST state its evaluative purpose.

Examples include:

- Determine whether a component is degraded.
- Determine whether repeated failures share a likely cause.
- Evaluate whether evidence supports suspected compromise.
- Evaluate whether containment remains necessary.
- Evaluate whether recovery conditions may have been reached.
- Evaluate whether the immune system itself is malfunctioning.
- Determine whether evidence is insufficient for a responsible conclusion.

An assessment validated for one purpose MUST NOT silently be reused as though it answered a different question.

### 7.4 Subject

An assessment MUST identify its subject as narrowly as reasonably possible.

The subject may be:

- A node.
- A component.
- A device.
- A connection.
- A runtime task.
- A resource.
- A memory operation.
- An immune role.
- A group of related subjects.
- A bounded subsystem.
- An explicitly unknown subject.

An assessment MUST distinguish:

- The directly evaluated subject.
- Related subjects.
- Potentially affected subjects.
- Subjects outside the assessment’s scope.

### 7.5 Assessment scope

The assessment MUST define its scope.

Scope may include:

- Subject boundary.
- Interface boundary.
- Operation class.
- Evidence interval.
- Incident interval.
- Runtime context.
- Connection context.
- Resource context.
- Memory-operation context.
- Geographic or hardware locality.
- Known exclusions.
- Unobserved regions.
- Missing participants.

An assessment MUST NOT silently generalize beyond its declared scope.

### 7.6 Evidence lineage

An assessment MUST identify or reference the evidence used.

Evidence lineage SHOULD make it possible to determine:

- Which evidence items materially supported the findings.
- Which evidence contradicted the findings.
- Which evidence was excluded.
- Why material evidence was excluded.
- Which evidence was stale.
- Which evidence was unvalidated.
- Which evidence was dependent or duplicated.
- Which relevant evidence was unavailable.
- Whether the evidence set changed during assessment.

The assessment MUST NOT make repeated copies of the same underlying evidence appear independently corroborative.

### 7.7 Fact and inference separation

An assessment MUST preserve the distinction between:

- Direct observation.
- Source report.
- Validated evidence.
- Derived evidence.
- Correlated evidence.
- Assessor inference.
- Assumption.
- Unknown.
- Conclusion.

A reader or downstream role SHOULD be able to identify which findings depend on inference rather than direct observation.

### 7.8 Assessment claims

Each material assessment claim SHOULD identify:

- The proposition being asserted.
- The subject.
- Supporting evidence.
- Contradictory evidence.
- Known assumptions.
- Known limitations.
- Applicable time.
- Scope.
- Uncertainty.
- Disposition.

An assessment MAY contain multiple claims with different levels of support.

The state of one claim MUST NOT automatically determine the state of every other claim in the assessment.

### 7.9 Temporal context

An assessment SHOULD identify:

- The evidence interval.
- The assessment creation time.
- The condition time being evaluated.
- Known clock limitations.
- Whether the assessment describes current, historical, or predicted conditions.
- When reassessment is expected or required.

A current assessment MUST NOT be inferred from a recent creation time when its evidence is stale.

### 7.10 Assessment state

A conforming assessment MUST be capable of representing the public states defined in Section 11.

The assessment state MUST be interpreted in relation to:

- Its purpose.
- Its subject.
- Its scope.
- Its evidence.
- Its time.
- Its uncertainty.

### 7.11 Uncertainty

An assessment MUST express known uncertainty.

Uncertainty may arise from:

- Missing evidence.
- Conflicting evidence.
- Stale evidence.
- Incomplete coverage.
- Unknown source dependence.
- Uncertain subject identity.
- Clock disagreement.
- Transformation loss.
- Competing explanations.
- Possible observer failure.
- Possible assessor failure.
- Limited domain knowledge.
- Unavailable external state.

An implementation MAY represent uncertainty:

- Qualitatively.
- Categorically.
- Structurally.
- Quantitatively.
- Through ranges.
- Through competing hypotheses.

This document does not require a universal confidence score.

### 7.12 Confidence representation

An assessment MAY include a confidence representation.

Where included, confidence MUST:

- Be attributable to the assessment or specific claim.
- Be tied to a stated purpose and scope.
- Preserve material uncertainty.
- Avoid implying mathematical precision unsupported by the method.
- Remain distinguishable from evidence validation.
- Remain distinguishable from authority.
- Remain distinguishable from severity.
- Remain distinguishable from urgency.

A confidence value MUST NOT be interpreted outside the context that produced it.

### 7.13 Alternative explanations

An assessment MUST preserve material alternative explanations when available evidence does not reasonably exclude them.

Alternative explanations may include:

- Hardware degradation.
- Runtime failure.
- Network partition.
- Clock failure.
- Collector failure.
- Misconfiguration.
- Resource exhaustion.
- Stale state.
- Software defect.
- Operator error.
- Benign adaptation.
- Adversarial activity.
- Unknown cause.

The Assessor MAY identify a preferred explanation.

Preference MUST NOT erase alternatives.

### 7.14 Evidence sufficiency

An assessment MUST be capable of stating whether evidence is:

- Sufficient for the stated conclusion within declared limits.
- Partially sufficient.
- Insufficient.
- Conflicted.
- Unavailable.
- Unknown in sufficiency.

Evidence sufficiency is purpose-dependent.

Evidence sufficient to conclude that a device is malfunctioning may be insufficient to conclude that it is compromised.

### 7.15 Assessment limitations

An assessment MUST identify material limitations.

Limitations may include:

- Missing subjects.
- Missing evidence.
- Missing time ranges.
- Unknown dependencies.
- Stale information.
- Restricted access.
- Shared failure domains.
- Self-report dependence.
- Unverified transformations.
- Lack of independent observation.
- Limited assessor capability.
- Degraded immune infrastructure.

Limitations MUST NOT be hidden merely because the assessment is urgent.

### 7.16 Assessment disposition

An assessment SHOULD identify its current disposition.

Possible dispositions include:

- Continue observation.
- Seek additional evidence.
- Seek independent assessment.
- Support recommendation development.
- Do not support intervention.
- Support recovery evaluation.
- Support restoration evaluation.
- Abstain from conclusion.
- Superseded.
- Withdrawn.
- Closed for stated purpose.

Disposition does not create authority.

---

## 8. Assessment Condition Classes

This section defines public semantic distinctions.

It does not define detection thresholds.

### 8.1 Normal within observed scope

Available evidence does not presently establish a material abnormal condition within the declared scope.

This state does not prove:

- Global health.
- Absence of compromise.
- Future safety.
- Completeness of observation.
- Correctness outside the declared scope.

### 8.2 Unknown

The relevant condition has not been established.

Unknown MUST NOT be silently converted into normal, degraded, failed, or compromised.

### 8.3 Insufficient evidence

Available evidence does not support a responsible conclusion for the stated purpose.

Insufficient evidence is an assessment result.

It is not a failure to produce useful output.

### 8.4 Degraded

The subject is operating with reduced capability, reliability, performance, availability, or assurance.

Degradation does not inherently imply:

- Malicious intent.
- Security compromise.
- Trust revocation.
- Required quarantine.
- Permanent failure.

### 8.5 Failed

The subject cannot perform one or more required functions within the assessed scope.

Failure may be:

- Temporary.
- Persistent.
- Partial.
- Recoverable.
- Irrecoverable under current means.
- Unknown in cause.

Failure does not inherently imply compromise.

### 8.6 Suspicious

Available evidence indicates behavior or conditions that warrant additional scrutiny.

Suspicious is not equivalent to compromised.

### 8.7 Suspected compromise

Available evidence supports compromise as a material explanation, but confirmation remains incomplete or contested.

The assessment MUST identify:

- Supporting evidence.
- Alternative explanations.
- Uncertainty.
- Scope.
- Known affected and unaffected regions where possible.

### 8.8 Compromise established within scope

Evidence supports that unauthorized influence, control, modification, disclosure, or subversion occurred within the declared scope under the applicable governing standard.

This assessment still does not itself create enforcement authority.

The conclusion MUST NOT silently generalize beyond the established scope.

### 8.9 Containment state observed

The subject appears to be under one or more restrictions.

This assessment concerns observed state.

It does not prove:

- The restriction was authorized.
- The restriction is sufficient.
- The restriction is correctly targeted.
- The subject is compromised.
- The restriction should continue.

### 8.10 Recovering

The subject is undergoing governed repair, reconstruction, reconfiguration, replacement, or stabilization.

Recovering is not equivalent to recovered.

### 8.11 Recovery conditions appear satisfied

Available evidence supports submitting the subject for formal Recovery Verifier evaluation.

An Assessor MUST NOT represent this state as completed recovery verification.

### 8.12 Restoration conditions appear supportable

Available evidence supports consideration of a bounded restoration recommendation.

This state does not authorize restoration and does not prove that restoration will succeed.

### 8.13 Immune-process degradation

The observation, evidence, assessment, recommendation, authorization-request, coordination, enforcement, verification, or audit process may itself be degraded.

Immune-process degradation MUST remain distinguishable from the condition of the evaluated subject.

---

## 9. Assessment Reasoning Requirements

### 9.1 Reasoning must be attributable

A conforming assessment MUST preserve enough reasoning structure to explain how material evidence supports its findings.

This requirement does not require disclosure of private algorithms or model internals.

It requires that the public work product preserve:

- Inputs.
- Material transformations.
- Material assumptions.
- Material conflicts.
- Resulting claims.
- Limitations.

### 9.2 Private reasoning mechanisms

An implementation MAY use private:

- Statistical methods.
- Learned models.
- Rule systems.
- Pattern libraries.
- Correlation engines.
- Hardware diagnostics.
- Adversarial indicators.
- Confidence calculations.

The public assessment MUST NOT expose protected details merely to demonstrate conformance.

The public assessment MUST still identify the class of reasoning and its material limitations.

### 9.3 No circular support

An assessment MUST NOT use its own conclusion as independent support for itself.

Prior assessments may be referenced as prior work products.

They MUST remain distinguishable from underlying evidence.

### 9.4 No privilege substitution

The authority, reputation, role, location, or privilege of an Assessor MUST NOT be substituted for evidentiary support.

A highly privileged Assessor may still be wrong, stale, degraded, or compromised.

### 9.5 No silence-based confirmation

A lack of disagreement MUST NOT automatically be interpreted as confirmation.

Other assessors may be:

- Unavailable.
- Unaware.
- Restricted.
- Stale.
- Failed.
- Unwilling to assert a conclusion.
- Observing a different scope.

### 9.6 No majority-truth assumption

Multiple agreeing assessments do not automatically establish truth.

Agreement may result from:

- Shared evidence.
- Shared models.
- Shared failure domains.
- Shared configuration.
- Common upstream error.
- Replication.
- Coordinated manipulation.

Agreement and independence MUST remain distinct.

---

## 10. Assessment Revision, Withdrawal, and Supersession

### 10.1 Revision

An assessment MAY be revised when:

- New evidence arrives.
- Existing evidence is invalidated.
- Scope changes.
- Timing changes.
- A transformation error is discovered.
- A conflict is resolved.
- The Assessor’s capability changes.
- A prior assumption is disproven.

A material revision MUST preserve lineage to the prior assessment.

### 10.2 Correction

A correction SHOULD identify:

- What was incorrect.
- Why it was incorrect.
- Which claims are affected.
- Which downstream recommendations may be affected.
- Whether authorization requests or actions may require review.

### 10.3 Withdrawal

An assessment MAY be withdrawn when the Assessor no longer stands behind its claims.

Withdrawal MUST NOT erase:

- The assessment’s existence.
- Its prior use.
- Its evidence lineage.
- Recommendations based upon it.
- Authority or action lineage associated with it.

### 10.4 Supersession

A later assessment MAY supersede an earlier assessment for a stated purpose.

Supersession MUST identify:

- The superseding assessment.
- The superseded assessment.
- The purpose and scope of supersession.
- Whether prior conclusions remain historically valid.
- Which downstream work products require reevaluation.

### 10.5 Stale assessment

An assessment may become stale because:

- Evidence aged.
- The subject changed.
- The environment changed.
- Scope changed.
- Recovery occurred.
- Restoration occurred.
- New contradictory evidence arrived.

A stale assessment MUST NOT silently retain current operational status.

### 10.6 Downstream impact

When a material assessment changes, the system SHOULD identify affected:

- Recommendations.
- Authorization requests.
- Pending authorization decisions.
- Active containment plans.
- Enforcement actions.
- Recovery evaluations.
- Restoration evaluations.
- Audit findings.

Identification does not automatically revoke downstream authority.

The appropriate governing process must decide what follows.

---

## 11. Public Assessment States

The following public states MAY apply to an assessment or assessment claim.

**Preliminary**  
The assessment is an early interpretation subject to material change.

**Partial**  
The assessment covers only part of its intended or declared scope.

**Current**  
The assessment is sufficiently fresh for its stated purpose.

**Stale**  
The assessment is no longer sufficiently fresh for its stated purpose.

**Conflicted**  
Material evidence or competing assessments support incompatible conclusions.

**Supported**  
Available evidence supports the claim within declared limits.

**Partially supported**  
Some support exists, but material gaps or conflicts remain.

**Unsupported**  
Available evidence does not support the claim.

**Insufficient evidence**  
The available evidence cannot support a responsible conclusion.

**Unknown**  
The assessment state has not been established.

**Abstained**  
The Assessor explicitly declined to produce a substantive conclusion.

**Withdrawn**  
The Assessor no longer presents the assessment as active.

**Superseded**  
A later assessment replaces it for a stated purpose.

**Closed**  
The assessment process has ended for its stated purpose without implying deletion or permanent truth.

These states are not threat scores, enforcement thresholds, or authorization decisions.

---

## 12. Multiple Assessments

### 12.1 Independent assessments

Multiple Assessors MAY evaluate the same subject and evidence.

Each assessment MUST remain independently attributable.

### 12.2 Shared evidence

Assessments based on the same evidence MUST NOT be represented as independent evidentiary confirmation merely because they were produced by different Assessors.

They may still provide independent interpretation.

### 12.3 Competing assessments

Competing assessments MAY differ in:

- Scope.
- Evidence selection.
- Evidence validation.
- Reasoning method.
- Alternative explanations.
- Confidence.
- Timing.
- Conclusion.
- Disposition.

The conflict MUST remain visible.

### 12.4 Assessment aggregation

Assessments MAY be aggregated.

Aggregation MUST preserve:

- Source assessments.
- Scope differences.
- Evidence overlap.
- Known dependencies.
- Conflicts.
- Minority conclusions.
- Unknowns.
- Aggregation method class.

Aggregation MUST NOT create false unanimity.

### 12.5 Assessment selection

A governing process MAY select an assessment for a particular decision.

Selection MUST NOT rewrite non-selected assessments as invalid unless separately established.

### 12.6 Assessor disagreement

Disagreement may motivate:

- Additional evidence.
- Independent observation.
- Revalidation.
- Scope narrowing.
- Multiple recommendations.
- Human review.
- Delayed decision.
- Emergency action under separate authority.

Disagreement does not itself prove immune failure.

---

## 13. Assessment Abstention

### 13.1 Permitted abstention

An Assessor MAY abstain when:

- Evidence is insufficient.
- Evidence is inaccessible.
- Scope is undefined.
- The Assessor lacks necessary capability.
- A conflict of interest cannot be acceptably managed.
- The Assessor is degraded.
- Required reasoning inputs are stale.
- Producing a conclusion would exceed the role’s competence.
- The assessment would improperly expose protected content.

### 13.2 Abstention record

An abstention SHOULD identify:

- Assessor.
- Subject.
- Purpose.
- Reason.
- Known evidence.
- Missing requirements.
- Whether reassessment may become possible.
- Suggested next step where appropriate.

### 13.3 Abstention is not a no-action recommendation

Abstention means the Assessor did not produce a substantive conclusion.

It does not mean that no action is appropriate.

It does not mean that action is appropriate.

A separate role and governing process must interpret the operational consequence.

---

## 14. Recommendation Contract

Every recommendation MUST be attributable as a distinct logical work product.

A recommendation contains or references the following information where applicable.

### 14.1 Recommendation identity

A recommendation MUST have an identity sufficient to support:

- Assessment reference.
- Authorization-request reference.
- Revision.
- Withdrawal.
- Supersession.
- Conflict reporting.
- Audit.
- Recovery review.
- Restoration review.

### 14.2 Recommender identity and role

The Recommender MUST be attributable.

The work product MUST identify that it was produced under the Recommender role defined by IMM-0002.

A component acting as both Assessor and Recommender MUST preserve the logical distinction between those roles.

### 14.3 Supporting assessment lineage

A recommendation MUST identify or reference the assessment or assessments that support it.

Where the recommendation departs from the preferred assessment disposition, it MUST state the reason.

A recommendation based directly on an emergency policy without a completed ordinary assessment MUST identify:

- The exceptional policy basis.
- Available evidence.
- Missing assessment elements.
- Resulting uncertainty.
- Required later review.

### 14.4 Objective

A recommendation MUST state its intended objective.

Objectives may include:

- Obtain additional evidence.
- Reduce immediate exposure.
- Prevent further damage.
- Preserve safety.
- Preserve memory integrity.
- Preserve diagnostic access.
- Isolate a failing resource.
- Limit communication.
- Pause new workload placement.
- Protect unaffected participants.
- Enable recovery.
- Verify recovery.
- Consider restoration.
- Continue observation.
- Seek human review.
- Take no new action.

The objective MUST NOT be hidden behind an implementation-specific command.

### 14.5 Target

A recommendation MUST identify its proposed target as narrowly as reasonably possible.

The target may be:

- A connection.
- A capability.
- A delegation.
- A runtime task.
- A process.
- A device.
- A resource.
- A memory operation.
- A node.
- A group of nodes.
- An immune component.
- A bounded subsystem.
- An explicitly unknown target requiring further scope determination.

A recommendation MUST distinguish:

- Primary target.
- Potentially affected subjects.
- Protected subjects.
- Out-of-scope subjects.

### 14.6 Recommended action class

A recommendation MUST identify the public action class being proposed.

Public action classes may include:

- Continue observation.
- Increase bounded observation.
- Request additional evidence.
- Request independent assessment.
- Request human review.
- Request ACS admission review.
- Request ACS trust review.
- Request ACS capability restriction.
- Request ACS delegation review.
- Request ACS revocation review.
- Request ACS connection containment.
- Request runtime pause.
- Request runtime isolation.
- Request resource-placement exclusion.
- Request resource restriction.
- Request governed memory inspection.
- Request governed memory recovery.
- Request containment continuation.
- Request containment narrowing.
- Request containment release review.
- Request recovery verification.
- Request restoration review.
- Recommend no new intervention.

These action classes do not redefine the authority or mechanisms of adjacent systems.

### 14.7 Proposed scope

A recommendation MUST bound the proposed scope.

Scope may include:

- Named target.
- Connection subset.
- Capability subset.
- Operation class.
- Runtime task class.
- Resource class.
- Time interval.
- Geographic or hardware region.
- Memory object or operation scope.
- Diagnostic access scope.
- Recovery-access scope.
- Excluded systems.
- Protected essential paths.

A recommendation MUST NOT silently request mesh-wide action when evidence supports only local scope.

### 14.8 Proposed constraints

A recommendation SHOULD identify applicable constraints.

Constraints may include:

- Preserve diagnostic access.
- Preserve authority communication.
- Preserve emergency shutdown.
- Preserve recovery channels.
- Preserve audit collection.
- Preserve specified memory custody.
- Exclude unrelated nodes.
- Exclude unaffected capabilities.
- Avoid destructive operations.
- Avoid irreversible operations.
- Require periodic review.
- Require independent verification.
- Require explicit restoration authorization.
- End when the stated condition occurs.

### 14.9 Intended duration

Where the proposed response would persist, the recommendation SHOULD identify:

- Intended duration.
- Expiration condition.
- Review interval.
- Maximum proposed duration.
- Renewal requirement.
- Exit condition.

The document does not define numeric durations or private thresholds.

A recommendation MUST NOT silently propose indefinite containment.

### 14.10 Review conditions

A recommendation SHOULD identify conditions requiring review.

Review conditions may include:

- New contradictory evidence.
- Target-state change.
- Scope expansion.
- Unexpected collateral effects.
- Failure of an essential path.
- Enforcement failure.
- Authorization expiry.
- Recovery progress.
- Restoration readiness.
- Assessor withdrawal.
- Recommendation supersession.
- Immune-system degradation.

### 14.11 Exit conditions

A recommendation SHOULD identify conditions under which the proposed response should:

- End.
- Narrow.
- Pause.
- Escalate for fresh review.
- Transition to recovery.
- Transition to restoration evaluation.
- Return to observation.

Exit conditions do not self-execute.

They inform authorization and coordination.

### 14.12 Collateral effects

A recommendation MUST identify reasonably foreseeable collateral effects.

Collateral effects may include:

- Loss of communication.
- Loss of diagnostic access.
- Workload interruption.
- Reduced redundancy.
- Increased latency.
- Memory unavailability.
- Loss of sensor coverage.
- Isolation of healthy dependents.
- Recovery delay.
- Safety degradation.
- Audit gaps.
- Evidence loss.
- Operator impact.
- Resource underutilization.

Unknown collateral effects MUST remain unknown.

### 14.13 Essential paths

A recommendation SHOULD identify essential paths that must remain available where possible.

Essential paths may include:

- Governing authorization.
- Emergency safety.
- Diagnostic communication.
- Evidence exchange.
- Recovery control.
- Restoration control.
- Audit.
- Memory custody.
- Health reporting.
- Operator access.
- Minimal runtime supervision.

A recommendation that cannot preserve an essential path MUST state:

- Which path may be lost.
- Why loss may be necessary.
- Expected consequence.
- Recovery implication.
- Need for governing review.

### 14.14 Reversibility

A recommendation SHOULD classify the proposed response as:

- Readily reversible.
- Reversible with cost.
- Partially reversible.
- Irreversible.
- Unknown in reversibility.

A recommendation SHOULD prefer reversible responses where they can reasonably achieve the objective.

This preference does not prohibit irreversible action under proper authority.

### 14.15 Follow-up evidence

A recommendation SHOULD identify the evidence needed to determine whether the proposed response:

- Began.
- Completed.
- Achieved its objective.
- Caused collateral effects.
- Should continue.
- Should narrow.
- Should end.
- Should enter recovery review.
- Should enter restoration review.

### 14.16 Recommendation uncertainty

A recommendation MUST preserve uncertainty inherited from its supporting assessments.

It MUST also identify uncertainty introduced by:

- Proposed action effects.
- Enforcement availability.
- Target identity.
- Scope.
- Duration.
- Collateral consequences.
- Recovery feasibility.
- Restoration feasibility.

### 14.17 Recommendation disposition

A recommendation SHOULD identify its current disposition.

Possible dispositions include:

- Draft.
- Active for authorization consideration.
- Pending additional evidence.
- Pending independent review.
- Withdrawn.
- Superseded.
- Expired.
- Rejected by governing authority.
- Narrowed by governing authority.
- Converted into an authorization request.
- Closed.

A recommendation’s disposition MUST NOT be confused with authorization status.

---

## 15. Recommendation Design Principles

### 15.1 Bounded response

A recommendation MUST be bounded by:

- Target.
- Objective.
- Action class.
- Scope.
- Constraints.
- Duration or review condition where applicable.
- Known collateral effects.

### 15.2 Least necessary intervention

A recommendation SHOULD propose the least intervention reasonably expected to achieve the declared objective under known conditions.

This principle does not require under-response.

It requires that broader or more destructive proposals state why narrower measures appear inadequate.

### 15.3 Preserve unaffected function

A recommendation SHOULD avoid restricting unrelated:

- Nodes.
- Connections.
- Capabilities.
- Memory objects.
- Runtime tasks.
- Resources.
- Sensors.
- Recovery mechanisms.

Where separation is not possible, the limitation MUST be explicit.

### 15.4 Preserve evidence

A recommendation MUST NOT propose destruction, alteration, or concealment of evidence merely to remove an alert or simplify the incident.

Evidence handling remains governed by MEM and applicable authority.

### 15.5 Preserve future review

A recommendation SHOULD preserve enough state and access to support:

- Reassessment.
- Audit.
- Recovery verification.
- Restoration verification.
- Authority review.

### 15.6 Avoid permanent conclusions from temporary conditions

A recommendation MUST NOT infer permanent untrustworthiness, permanent quarantine, or permanent exclusion solely from a temporary failure or incomplete assessment.

Permanent or indefinite restrictions require separately governed authority and review.

### 15.7 No punishment semantics

Immune recommendations exist to protect, diagnose, contain, recover, and restore.

They MUST NOT frame containment as punishment.

### 15.8 No deletion disguised as quarantine

A recommendation MUST NOT use terms such as quarantine, containment, isolation, invalidation, or exclusion to conceal a proposal for memory deletion.

---

## 16. Recommendation Types

### 16.1 Observation recommendation

Proposes continued or expanded bounded observation.

It SHOULD identify:

- Subject.
- Purpose.
- Scope.
- Collection class.
- Required authority.
- Expected review point.
- Privacy or cognitive-content limitations.

### 16.2 Evidence recommendation

Proposes collection, validation, reconstruction, comparison, or independent sourcing of evidence.

It MUST preserve ACS and MEM access boundaries.

### 16.3 Assessment recommendation

Proposes additional, independent, repeated, narrowed, or specialized assessment.

It MUST NOT imply that a particular conclusion is required.

### 16.4 Containment recommendation

Proposes a bounded restriction intended to reduce exposure, propagation, damage, or interference.

It SHOULD identify:

- Protected objective.
- Target.
- Scope.
- Essential paths.
- Proposed duration.
- Review conditions.
- Exit conditions.
- Recovery implications.
- Restoration implications.

### 16.5 Recovery recommendation

Proposes that the appropriate system begin or continue governed repair, reconstruction, replacement, stabilization, or diagnostic work.

It MUST NOT redefine MEM reconstruction or runtime repair semantics.

### 16.6 Recovery-verification recommendation

Proposes formal evaluation by the Recovery Verifier.

It MUST distinguish apparent recovery progress from verified recovery.

### 16.7 Restoration recommendation

Proposes consideration of returning bounded access, capability, placement, communication, or service.

It SHOULD identify:

- Recovery evidence.
- Remaining uncertainty.
- Proposed restored scope.
- Restrictions that should remain.
- Verification requirements.
- Rollback conditions.

### 16.8 No-action recommendation

A no-action recommendation affirmatively proposes that no new intervention be requested at present.

It MUST identify:

- Supporting assessment.
- Scope.
- Time.
- Conditions requiring reconsideration.
- Known uncertainty.
- Whether observation should continue.

No-action does not mean:

- The subject is globally safe.
- The incident never occurred.
- Evidence may be deleted.
- Future review is prohibited.

### 16.9 Human-review recommendation

Proposes review by an authorized human or governance body.

It SHOULD identify:

- Question requiring review.
- Decision urgency.
- Available evidence.
- Missing information.
- Consequences of delay.
- Available bounded alternatives.

### 16.10 Emergency recommendation

Proposes action under an established emergency policy.

It MUST identify:

- Emergency policy basis.
- Evidence available.
- Ordinary steps omitted or shortened.
- Proposed scope.
- Constraints.
- Review requirement.
- Expiration or transition condition.
- Required retrospective audit.

Emergency recommendation does not itself establish emergency authority.

---

## 17. Multiple and Competing Recommendations

### 17.1 Multiple recommendations

A Recommender MAY produce multiple alternatives.

Alternatives may differ in:

- Scope.
- Intrusiveness.
- Reversibility.
- Duration.
- Evidence needs.
- Collateral effects.
- Recovery implications.
- Urgency.

### 17.2 Preferred recommendation

A Recommender MAY identify a preferred option.

The preference SHOULD state why it is preferred.

Non-preferred alternatives SHOULD remain visible when materially viable.

### 17.3 Competing recommenders

Multiple Recommenders MAY produce conflicting recommendations.

Each recommendation MUST remain attributable.

Conflict MUST NOT be silently resolved by:

- Privilege.
- Role count.
- Earliest submission.
- Broadest scope.
- Greatest severity.
- Highest claimed confidence.

### 17.4 Recommendation aggregation

Recommendations MAY be aggregated into an options set.

Aggregation MUST preserve:

- Original recommendations.
- Supporting assessments.
- Scope differences.
- Authority needs.
- Collateral differences.
- Conflicts.
- Dependencies.
- Minority proposals.

### 17.5 Incompatible recommendations

Incompatible recommendations MAY require:

- Additional evidence.
- Independent assessment.
- Governing review.
- Narrow provisional action.
- Emergency policy.
- Delayed intervention.
- Parallel non-conflicting actions.

No recommendation may declare itself authorized because alternatives are incompatible.

---

## 18. Recommendation Revision, Withdrawal, and Supersession

### 18.1 Revision

A recommendation MAY be revised when:

- Its supporting assessment changes.
- New evidence arrives.
- Scope changes.
- Collateral effects become clearer.
- An essential path changes.
- Enforcement availability changes.
- Recovery progress changes.
- Governing feedback narrows the acceptable proposal.

A material revision MUST preserve lineage.

### 18.2 Withdrawal

A Recommender MAY withdraw a recommendation.

Withdrawal SHOULD identify:

- Reason.
- Affected authorization requests.
- Affected pending actions.
- Replacement recommendation where applicable.
- Whether urgent review is needed.

Withdrawal does not automatically revoke existing authorization.

The governing authority must decide the effect on authorization.

### 18.3 Supersession

A later recommendation MAY supersede an earlier one for a stated purpose.

Supersession MUST identify:

- Earlier recommendation.
- New recommendation.
- Changed basis.
- Changed scope.
- Changed constraints.
- Downstream work products requiring review.

### 18.4 Expiration

A recommendation MAY expire because:

- Its assessment became stale.
- Its review window passed.
- Its target changed.
- Its objective was achieved.
- It was rejected.
- It was replaced.
- Recovery or restoration changed the situation.

An expired recommendation MUST NOT be treated as current authority or current advice.

---

## 19. Recommendation Abstention

### 19.1 Permitted abstention

A Recommender MAY abstain when:

- No conforming recommendation can be supported.
- Assessment evidence is insufficient.
- Every known option exceeds the Recommender’s competence.
- Material collateral effects are unknown.
- Required authority cannot be identified.
- Scope cannot be bounded.
- The Recommender is conflicted or degraded.
- Protected information cannot be safely incorporated.
- The recommendation would improperly redefine ACS or MEM authority.

### 19.2 Abstention record

The abstention SHOULD identify:

- Recommender.
- Assessment considered.
- Reason.
- Missing information.
- Known options not responsibly evaluated.
- Suggested next step.
- Urgency where applicable.

### 19.3 Abstention is not recommendation denial

A Recommender’s abstention does not establish that no action should occur.

Another conforming Recommender or governing process may proceed.

---

## 20. Assessment-to-Recommendation Relationship

### 20.1 Required lineage

A recommendation MUST be traceable to one or more assessments, except where a separately governed emergency policy explicitly permits an abbreviated path.

### 20.2 No automatic conversion

An assessment MUST NOT automatically become a recommendation merely because it identifies a serious condition.

A recommendation requires separate consideration of:

- Objective.
- Target.
- Action class.
- Scope.
- Constraints.
- Collateral effects.
- Essential paths.
- Duration.
- Review.
- Recovery.
- Restoration.

### 20.3 One assessment, multiple recommendations

A single assessment may support:

- Further observation.
- Narrow containment.
- Broader governed review.
- Recovery work.
- No action.
- Multiple alternatives.

### 20.4 Multiple assessments, one recommendation

A recommendation may rely on multiple assessments.

It MUST preserve:

- Conflicts.
- Different scopes.
- Different freshness.
- Shared evidence.
- Material minority conclusions.

### 20.5 Assessment changes

When a supporting assessment materially changes, the recommendation MUST be eligible for review.

The system MUST NOT assume that the recommendation remains valid.

---

## 21. Emergency Handling

### 21.1 Emergency policy is external

Emergency authority must be established by governing policy outside this document.

An Assessor or Recommender cannot create emergency authority by labeling a condition urgent.

### 21.2 Abbreviated assessment

An emergency policy MAY permit assessment with:

- Partial evidence.
- Reduced independent review.
- Limited correlation.
- Unresolved alternatives.
- Shortened timing.

The resulting limitations MUST remain explicit.

### 21.3 Abbreviated recommendation

An emergency policy MAY permit a recommendation with limited time for:

- Full collateral analysis.
- Multiple alternatives.
- Complete duration planning.
- Independent review.

The recommendation MUST still identify, at minimum:

- Subject.
- Available evidence.
- Assessment basis.
- Objective.
- Target.
- Action class.
- Proposed scope.
- Known constraints.
- Known essential paths.
- Emergency policy basis.
- Required review.

### 21.4 Emergency review

Emergency recommendations SHOULD require later review of:

- Evidence sufficiency.
- Scope.
- Proportionality.
- Authority.
- Execution.
- Collateral effects.
- Continued need.
- Recovery.
- Restoration.
- Audit completeness.

### 21.5 Emergency expiration

Emergency recommendations and resulting authorization SHOULD have explicit expiration or review conditions.

Urgency MUST NOT silently create indefinite containment.

---

## 22. Cross-Architecture Boundaries

### 22.1 ACS boundary

ACS remains authoritative for:

- Connection identity.
- Admission.
- Trust.
- Capability.
- Delegation.
- Revocation.
- Connection lifecycle.
- Communication restrictions.
- ACS enforcement.

An assessment may evaluate ACS-related evidence.

A recommendation may request ACS-governed review or action.

Neither may:

- Redefine ACS semantics.
- Directly alter ACS state.
- Bypass ACS enforcement.
- Invent ACS authority.
- Treat ACS trust state as proof of factual correctness.
- Treat an immune conclusion as automatic ACS revocation.

### 22.2 MEM boundary

MEM remains authoritative for:

- Memory identity.
- Persistence.
- Provenance.
- Custody.
- Retention.
- Availability.
- Recovery.
- Reconstruction.
- Deletion.
- Memory-operation semantics.

An assessment may evaluate memory-related evidence.

A recommendation may propose a governed MEM operation.

Neither may:

- Directly rewrite memory.
- Delete memory.
- Alter retention.
- Change custody.
- Perform reconstruction outside MEM.
- Reclassify invalid evidence as deletable.
- Treat quarantine as deletion.

### 22.3 Runtime boundary

Runtime systems remain authoritative for:

- Execution.
- Scheduling.
- Placement.
- Process lifecycle.
- Runtime isolation.
- Runtime repair mechanisms.

An assessment may conclude that runtime degradation is plausible.

A recommendation may propose runtime review, pause, isolation, or recovery.

The immune architecture does not gain runtime authority merely by making the recommendation.

### 22.4 Resource-management boundary

Resource owners remain authoritative for:

- Device access.
- Allocation.
- Placement.
- Reservation.
- Exclusion.
- Resource lifecycle.

A recommendation may propose resource restriction or placement exclusion.

It does not directly control the resource.

### 22.5 Adaptive-state boundary

An assessment may evaluate behavior involving adaptive state.

A recommendation MUST NOT directly rewrite adaptive-state descriptors, learned state, cognitive content, or internal decision structure merely to make the assessed behavior disappear.

Any governed adaptive-state operation requires its own authority and contract.

### 22.6 Authorization boundary

Assessment and recommendation precede authorization consideration.

They MUST NOT impersonate, embed, or silently replace the governing authorization decision.

---

## 23. Cognitive Content and Privacy

### 23.1 Minimum necessary interpretation

Assessments SHOULD use the minimum evidence necessary for the stated immune purpose.

A role MUST NOT request unrestricted cognitive content merely because broader access might increase confidence.

### 23.2 Derived indicators

Assessments MAY use bounded indicators derived from protected content where authorized.

The assessment MUST preserve that:

- Derivation occurred.
- The indicator is not the full underlying content.
- Material limitations exist.
- Access was governed.

### 23.3 Recommendation scope

A recommendation involving protected cognitive content MUST identify:

- Why access or action is proposed.
- The minimum required scope.
- Applicable authority.
- Privacy implications.
- Handling constraints.
- Review conditions.

### 23.4 No content-based punishment

Assessment of cognitive content MUST NOT be used to frame containment as punishment for thought, preference, uncertainty, or ordinary internal variation.

Immune action must remain tied to governed protection objectives and evidence of relevant system conditions.

---

## 24. Assessment and Recommendation Failure

### 24.1 Assessor failure

An Assessor may:

- Crash.
- Become unavailable.
- Produce stale output.
- Misinterpret evidence.
- Use invalid evidence.
- Conceal conflict.
- Become compromised.
- Exceed scope.
- Produce an unsupported conclusion.

Assessor failure is evidence about the immune process.

It is not automatically evidence against the assessed subject.

### 24.2 Recommender failure

A Recommender may:

- Propose excessive scope.
- Omit collateral effects.
- Ignore essential paths.
- Hide uncertainty.
- Recommend an unauthorized action class.
- Treat containment as punishment.
- Conceal irreversibility.
- Become compromised.
- Produce stale advice.

Recommender failure is evidence about the immune process.

It does not create authority.

### 24.3 Failed assessment generation

Failure to produce an assessment MUST NOT automatically be interpreted as:

- Normality.
- Compromise.
- Approval.
- Denial.
- No-action recommendation.
- Emergency authorization.

### 24.4 Failed recommendation generation

Failure to produce a recommendation MUST NOT automatically be interpreted as:

- No action required.
- Action prohibited.
- Existing containment should continue.
- Existing containment should end.
- Broader action authorized.

### 24.5 Incorrect assessment

An incorrect assessment MUST remain correctable.

Correction SHOULD trigger review of affected downstream work products.

The existence of an error MUST NOT authorize deletion of the erroneous assessment from history.

### 24.6 Harmful recommendation

A harmful or nonconforming recommendation may itself become immune evidence.

It may support:

- Recommender review.
- Role restriction.
- Independent assessment.
- Audit.
- Governed containment of the immune component.

The same evidence and authority boundaries apply.

---

## 25. Immune Self-Assessment

Immune roles and components are valid assessment subjects.

An immune component MAY be assessed for:

- Availability.
- Freshness.
- Evidence handling.
- Scope adherence.
- Authority-boundary adherence.
- Conflict disclosure.
- Self-report dependence.
- Failure.
- Compromise.
- Harmful recommendation.
- Audit nonconformance.

An immune component MUST NOT:

- Declare its own assessment uniquely authoritative.
- Prevent independent assessment.
- Treat immune status as proof of trustworthiness.
- Clear itself solely through self-produced evidence.
- Suppress competing assessments.
- Grant itself exemption from containment.
- Delete assessments documenting its own failure.

---

## 26. Conforming Example: Hardware Degradation

An evidence set shows that tasks assigned to one accelerator returned invalid results.

The evidence also shows:

- Driver resets.
- Stale thermal telemetry.
- Normal results from other accelerators.
- No verified unauthorized access.
- Shared dependence on one driver instance.

The Assessor produces a partial assessment:

- The named accelerator or its driver path is degraded.
- Compromise is not established.
- Hardware and software causes remain plausible.
- Scope is limited to the named accelerator and shared driver path.
- Evidence is sufficient for resource-risk evaluation but insufficient for a security-compromise conclusion.

The Recommender proposes:

- Temporarily exclude the accelerator from new workload placement.
- Preserve diagnostic access.
- Preserve existing evidence.
- Avoid changing unrelated accelerators.
- Review after driver and hardware diagnostics.
- Submit for recovery verification before normal placement resumes.

This is conforming because:

- Degradation remains distinct from compromise.
- Scope remains bounded.
- Recommendation remains separate from authority.
- The responsible resource system retains control.
- Diagnostic and recovery paths remain available.
- Restoration is not assumed.

---

## 27. Conforming Example: Suspicious Connection Behavior

Evidence shows that one connection attempted operations outside its currently visible capability scope.

The evidence is authentic but incomplete because:

- The peer’s current delegation view is unavailable.
- Clock ordering differs.
- A recent ACS update may not have propagated.
- Similar behavior has not been observed elsewhere.

The Assessor concludes:

- The behavior is suspicious.
- A stale or inconsistent capability view remains a plausible explanation.
- Compromise is not established.
- The assessment is partial and conflicted.

The Recommender proposes:

- Request an ACS capability and delegation review.
- Preserve the connection for bounded diagnostic exchange where ACS policy permits.
- Avoid mesh-wide revocation.
- Collect synchronized capability-state evidence.
- Reassess after the evidence is available.

This is conforming because the recommendation uses ACS review rather than redefining capability semantics or directly enforcing revocation.

---

## 28. Conforming Example: Apparent Recovery

A contained node reports that diagnostics pass.

Independent runtime evidence confirms stable execution for the tested workload.

Memory-related evidence shows no unresolved reconstruction errors.

However:

- Network behavior has not been retested.
- The node’s prior self-report was unreliable.
- Full capability restoration has not been evaluated.

The Assessor concludes:

- Recovery conditions appear satisfied for runtime execution.
- Recovery remains unestablished for communication behavior.
- Full restoration readiness is unknown.

The Recommender proposes:

- Submit the runtime portion for Recovery Verifier evaluation.
- Maintain communication restrictions.
- Collect bounded network evidence.
- Delay full restoration consideration.
- Reassess when the missing evidence is available.

This is conforming because partial recovery does not become global recovery or automatic restoration.

---

## 29. Conforming Example: No New Action

Several alerts originate from a collector later found to have a clock fault.

Independent evidence does not reproduce the reported sequence.

The Assessor concludes:

- The original timeline is unreliable.
- Subject compromise is unsupported.
- Collector degradation is established within scope.
- Some subject behavior remains unknown.

The Recommender proposes:

- No new restriction against the original subject.
- Continue ordinary bounded observation.
- Repair and review the collector.
- Preserve the original evidence for audit.
- Reassess if independent evidence appears.

This is a conforming no-action recommendation because it is affirmative, scoped, evidence-based, reviewable, and does not claim global safety.

---

## 30. Nonconforming Examples

The following behavior is nonconforming:

- Presenting an inference as a direct observation.
- Declaring compromise because evidence is authentic.
- Treating high confidence as enforcement authority.
- Treating urgency as permission to omit attribution.
- Converting an assessment directly into an ACS revocation command.
- Treating degradation as malicious behavior.
- Treating failure as proof of compromise.
- Treating lack of contradiction as confirmation.
- Counting duplicated assessments as independent evidence.
- Hiding a materially plausible benign explanation.
- Generalizing a component assessment to the entire mesh without support.
- Producing a recommendation without identifying its target.
- Proposing mesh-wide containment from node-local evidence without justification.
- Omitting known collateral effects.
- Removing diagnostic and recovery paths without stating the consequence.
- Recommending indefinite quarantine without governed review.
- Treating containment as punishment.
- Recommending memory deletion under the label of quarantine.
- Treating a recommendation as approved because no authority responded.
- Treating an expired recommendation as current.
- Treating Assessor abstention as proof that no action is needed.
- Treating Recommender abstention as denial.
- Treating a no-action recommendation as proof of global safety.
- Treating apparent recovery as verified recovery.
- Treating recovery as automatic restoration.
- Rewriting adaptive state to make an assessment favorable.
- Deleting an incorrect assessment to conceal the error.
- Publishing private confidence formulas or response thresholds in the public contract.

---

## 31. Public and Private Boundary

The following belongs in public architecture:

- Assessment identity.
- Assessment purpose.
- Subject and scope.
- Evidence lineage requirements.
- Fact and inference separation.
- Public assessment states.
- Uncertainty requirements.
- Alternative-explanation requirements.
- Conflict handling.
- Revision and supersession semantics.
- Recommendation identity.
- Recommendation objective.
- Target.
- Public action class.
- Scope.
- Constraints.
- Collateral effects.
- Essential-path requirements.
- Duration and review semantics.
- Exit-condition semantics.
- Reversibility classification.
- No-action semantics.
- Abstention semantics.
- Authority separation.
- Cross-architecture boundaries.
- Failure semantics.
- Self-accountability.
- Conformance requirements.

The following SHOULD remain implementation-private unless separately approved for publication:

- Detection thresholds.
- Confidence formulas.
- Evidence weights.
- Feature weights.
- Model parameters.
- Threat-scoring formulas.
- Recommendation-ranking formulas.
- Private indicators.
- Protected pattern libraries.
- Secret escalation rules.
- Adversarial detection methods.
- Exact emergency triggers.
- Suppression thresholds.
- Internal response-selection heuristics.
- Protected deployment details.
- Credential or key material.

Private implementation may evolve without changing this public contract.

Private implementation MUST remain conformant with the public assessment, recommendation, and authority boundaries.

---

## 32. Conformance Requirements

An implementation conforms to this document only when:

1. Every assessment is attributable.
2. Every recommendation is attributable.
3. Assessment purpose is stated or referenceable.
4. Assessment subject is bounded.
5. Assessment scope is bounded.
6. Supporting evidence is referenceable.
7. Material excluded evidence is attributable where policy permits.
8. Fact remains distinguishable from inference.
9. Assumption remains distinguishable from evidence.
10. Unknown values remain unknown.
11. Material uncertainty remains explicit.
12. Material alternative explanations remain visible.
13. Evidence sufficiency is purpose-dependent.
14. Degradation remains distinct from compromise.
15. Failure remains distinct from compromise.
16. Suspicion remains distinct from established compromise.
17. Assessment confidence remains distinct from authority.
18. Assessment severity remains distinct from authority.
19. Assessment urgency remains distinct from authority.
20. Competing assessments remain visible.
21. Shared evidence is not misrepresented as independent confirmation.
22. Revision preserves lineage.
23. Withdrawal does not erase history.
24. Supersession identifies affected prior work.
25. Every recommendation references supporting assessment lineage.
26. Recommendation objective is explicit.
27. Recommendation target is bounded.
28. Recommended action class is explicit.
29. Recommendation scope is bounded.
30. Known collateral effects remain visible.
31. Essential paths are identified where material.
32. Duration or review conditions are present where continuing action is proposed.
33. Exit conditions are identified where reasonably possible.
34. Reversibility is considered.
35. Follow-up evidence needs are identified where material.
36. Recommendation uncertainty remains explicit.
37. Recommendation remains distinct from authorization.
38. Emergency handling preserves authority lineage.
39. No-action recommendation remains distinct from abstention.
40. ACS authority and mechanisms remain preserved.
41. MEM authority and lifecycle semantics remain preserved.
42. Runtime authority remains preserved.
43. Resource-management authority remains preserved.
44. Adaptive state is not directly rewritten by an immune recommendation.
45. Containment is not represented as punishment.
46. Quarantine is not represented as deletion.
47. Recovery remains distinct from restoration.
48. Immune components remain assessable and accountable.
49. Failed assessment or recommendation does not automatically implicate the evaluated subject.
50. Private heuristics and thresholds are not disclosed in the public contract.

---

## 33. Required Failure Semantics

Assessment and recommendation failures MUST be representable without inventing a conclusion or authority.

Specifically:

- Assessment failure does not prove subject health.
- Assessment failure does not prove subject compromise.
- Assessment abstention does not recommend action or non-action.
- Insufficient evidence does not prove the opposite claim.
- Conflicted assessment does not prove that every claim is false.
- Stale assessment does not erase historical relevance.
- Withdrawn assessment does not erase prior downstream use.
- Recommender failure does not authorize an alternative action.
- Recommendation abstention does not deny action.
- Rejected recommendation does not prove the assessment false.
- Accepted recommendation does not prove the assessment true.
- No-action recommendation does not establish global safety.
- Emergency recommendation does not create emergency authority.
- Failed authorization request does not invalidate the recommendation.
- Failed enforcement does not prove the recommendation was incorrect.
- Harm caused by enforcement does not automatically prove malicious recommendation.
- Recovery failure does not automatically prove compromise.
- Restoration failure does not automatically disprove recovery.
- Immune-role failure is evidence about the immune system, not automatically about the evaluated subject.

The failure state may become new evidence under IMM-0003.

It remains subject to the same assessment, recommendation, authority, and audit contracts.

---

## 34. Architectural Commitments

This document establishes the following commitments:

1. Assessments are attributable interpretations, not raw observations.
2. Assessment claims preserve evidence lineage.
3. Fact, inference, assumption, and unknown remain distinguishable.
4. Assessment is purpose-bounded.
5. Assessment is scope-bounded.
6. Uncertainty remains explicit.
7. Material alternatives remain visible.
8. Confidence does not create authority.
9. Urgency does not create authority.
10. Degradation, failure, suspicion, and compromise remain distinct.
11. Multiple assessments may coexist.
12. Agreement does not automatically establish independence.
13. Assessment revision preserves history.
14. Recommendations are attributable proposals.
15. Recommendation is not authorization.
16. Recommendations identify objective, target, action class, and scope.
17. Recommendations identify known collateral effects.
18. Recommendations preserve essential paths where possible.
19. Recommendations consider duration, review, exit, and reversibility.
20. Recommendations do not silently become indefinite containment.
21. No-action is an affirmative, bounded recommendation.
22. Abstention is neither approval nor denial.
23. Emergency paths may shorten timing but not authority lineage.
24. ACS retains authority over ACS-owned state and enforcement.
25. MEM retains authority over persistent memory lifecycle.
26. Runtime and resource owners retain operational authority.
27. Adaptive state cannot be rewritten merely to satisfy immune conclusions.
28. Recovery remains distinct from restoration.
29. Immune components remain assessable, containable, and auditable.
30. Public contracts remain stable while private reasoning methods may evolve.

---

## 35. Future Work

Later immune architecture documents may define:

- Authorization-request and decision contracts.
- Containment lifecycle and action-envelope contracts.
- Enforcement coordination and receipt contracts.
- Recovery-verification contracts.
- Restoration-verification contracts.
- Immune audit-record contracts.
- Cross-node assessment exchange.
- Assessment quorum and diversity semantics.
- Recommendation option-set negotiation.
- Emergency-policy conformance.
- Partition-aware assessment.
- Adversarial assessment testing.
- Immune self-assessment and role-health protocols.
- Public conformance scenarios.

Those documents must preserve the evidence, assessment, recommendation, authority, ACS, MEM, recovery, restoration, and accountability boundaries established in this series.
