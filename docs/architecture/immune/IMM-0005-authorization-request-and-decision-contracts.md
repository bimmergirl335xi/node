# IMM-0005: Authorization Request and Decision Contracts

**Status:** Draft  
**Series:** Public Immune Architecture  
**Classification:** Public architecture contract  
**Depends on:** IMM-0000, IMM-0001, IMM-0002, IMM-0003, IMM-0004  
**Related architecture:** ACS-0006 through ACS-0009; MEM-0003 through MEM-0010  
**Supersedes:** None

---

## 1. Abstract

This document defines the public contracts by which Node’s immune architecture submits recommendations for governed authorization and receives attributable authorization decisions.

An authorization request asks a recognized authority to permit, deny, narrow, modify, renew, suspend, revoke, or review a bounded action.

An authorization decision establishes whether a particular actor or invocation context may pursue a defined action against a defined target, within an explicit scope, under explicit constraints and validity conditions.

A recommendation is not an authorization request.

An authorization request is not an authorization.

An authorization is not an enforcement action.

An enforcement action is not proof that the authorized objective was achieved.

This document defines request structure, authority attribution, decision types, scope, conditions, expiration, renewal, suspension, revocation, supersession, partial approval, conflicting authority, emergency authorization, and failure semantics.

It does not create authority, define who ultimately holds authority, redefine ACS admission or trust, redefine MEM lifecycle governance, or define implementation-specific enforcement mechanisms.

---

## 2. Purpose

The purpose of this document is to establish:

1. What constitutes an immune authorization request.
2. What constitutes an authorization decision.
3. How recommendations remain distinguishable from requests.
4. How requests remain distinguishable from decisions.
5. How authority source and authority basis are attributed.
6. How requested and authorized targets, actions, scopes, constraints, and durations are represented.
7. How an authority may approve, deny, narrow, modify, defer, suspend, revoke, or supersede a request.
8. How partial authorization is represented.
9. How authorization validity and expiration are determined.
10. How authorization renewal and extension occur.
11. How conflicting or overlapping authorities are handled.
12. How emergency authorization remains governed and reviewable.
13. How authorization changes affect pending and active immune work.
14. How ACS, MEM, runtime, resource, adaptive-state, and governance boundaries remain intact.
15. How failed or unavailable authorization processes are represented.
16. Which authorization details belong in public architecture and which remain implementation-private.

This document prevents a recommendation from becoming an undeclared command and prevents a broad or ambiguous authorization from being interpreted as unlimited control.

---

## 3. Normative Language

The terms **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **MAY**, and **OPTIONAL** are normative.

The following terms are used throughout this document.

**Authorization request**  
An attributable request asking a recognized governing authority to decide whether a bounded action may proceed.

**Requester**  
The actor or component submitting the authorization request under the Authorization Requester role defined by IMM-0002.

**Authority source**  
The human, policy system, delegated authority, governance body, emergency policy, or other recognized mechanism empowered to issue the decision.

**Authority basis**  
The preexisting governance, policy, delegation, ownership, capability, jurisdiction, or other recognized foundation under which the authority source may decide.

**Authorization decision**  
An attributable determination approving, denying, narrowing, modifying, deferring, suspending, revoking, or otherwise disposing of an authorization request.

**Authorization envelope**  
The complete bounded meaning of an authorization, including actor, target, action class, scope, constraints, validity, and review conditions.

**Authorized actor**  
The actor, role, service, component, or invocation context permitted to invoke or coordinate the authorized action.

**Authorized target**  
The subject or bounded system element against which the authorized action may apply.

**Authorized action class**  
The public category of action permitted by the authorization.

**Validity condition**  
A condition that must remain true for an authorization to be usable.

**Expiration condition**  
A time, event, state, or review boundary after which an authorization is no longer valid.

**Renewal**  
A new decision extending or replacing authorization beyond its prior validity.

**Suspension**  
A temporary state in which an authorization remains historically valid but MUST NOT presently be exercised.

**Revocation**  
A governed decision ending future use of an authorization before its otherwise expected expiration.

**Supersession**  
Replacement of an earlier authorization by a later decision for a stated purpose and scope.

**Partial approval**  
Authorization of only part of the requested target, action, scope, duration, or constraints.

**Denial**  
A decision that the requested action is not authorized under the submitted request.

**Deferral**  
A decision postponing final approval or denial pending additional evidence, review, authority, or changed conditions.

---

## 4. Scope

This document defines public contracts for:

- Authorization-request identity.
- Requester attribution.
- Supporting recommendation lineage.
- Supporting assessment and evidence lineage.
- Authority destination.
- Requested actor.
- Requested target.
- Requested action class.
- Requested objective.
- Requested scope.
- Requested constraints.
- Requested duration.
- Requested review conditions.
- Requested exit conditions.
- Requested restoration implications.
- Authority-source attribution.
- Authority-basis attribution.
- Decision identity.
- Decision types.
- Authorized actor.
- Authorized target.
- Authorized action class.
- Authorized scope.
- Authorized constraints.
- Validity.
- Expiration.
- Renewal.
- Suspension.
- Revocation.
- Supersession.
- Partial approval.
- Conflicting authority.
- Emergency authorization.
- Decision withdrawal and correction.
- Decision acknowledgement.
- Authorization failure.
- Cross-architecture boundaries.
- Public and private classification boundaries.

This document applies to authorization concerning:

- Observation expansion.
- Evidence access.
- Independent assessment.
- Human or governance review.
- ACS-governed connection review.
- ACS-governed restriction.
- Runtime pause or isolation.
- Resource-placement exclusion.
- Resource restriction.
- Governed memory inspection.
- Governed memory recovery.
- Containment.
- Recovery activity.
- Recovery verification.
- Restoration.
- Immune-component review.
- Immune-component containment.
- Audit access or review.

---

## 5. Explicit Non-Goals

This document does not define:

- Which person always has final authority.
- A universal governance hierarchy.
- A universal approval quorum.
- A universal voting system.
- Private authorization policy.
- Human identity policy.
- Cryptographic identity mechanisms.
- Key-management mechanisms.
- ACS admission policy.
- ACS trust policy.
- ACS capability semantics.
- ACS delegation semantics.
- ACS revocation semantics.
- ACS enforcement mechanisms.
- Memory persistence policy.
- Memory custody policy.
- Memory retention policy.
- Memory reconstruction methods.
- Memory deletion policy.
- Runtime scheduling policy.
- Resource ownership policy.
- Hardware-specific restrictions.
- Detection thresholds.
- Threat scores.
- Confidence formulas.
- Recommendation-ranking formulas.
- Automatic enforcement.
- A centralized authorization server.
- A mandatory human in every decision.
- A mandatory machine decision-maker.
- A universal emergency trigger.
- A universal authorization duration.
- A universal severity taxonomy.

Those concerns belong to governance, ACS, MEM, runtime architecture, resource management, private implementation, or later immune specifications.

---

## 6. Foundational Rules

### 6.1 Authority must preexist the request

An authorization request does not create the authority being asked to decide.

The authority source MUST derive decision power from an already recognized basis.

The request MUST NOT appoint its own approver.

### 6.2 Recommendation is not request

A recommendation describes what should be considered.

An authorization request asks a recognized authority to decide.

The request MAY reference or carry the recommendation, but the two work products MUST remain distinct.

### 6.3 Request is not authorization

Submission, receipt, validation, or review of a request does not authorize the requested action.

A pending request remains pending.

Silence is not approval.

### 6.4 Authorization is not enforcement

An authorization permits a bounded action.

It does not prove that:

- The action was invoked.
- The responsible system accepted it.
- The action completed.
- The intended state was reached.
- The objective was achieved.
- Recovery occurred.
- Restoration is safe.

### 6.5 Authorization is bounded

Authorization MUST be interpreted within its complete envelope.

An authorization MUST NOT be broadened through implication, convenience, urgency, role privilege, or technical capability.

### 6.6 Ambiguity does not grant broader power

When authorization language is ambiguous, the authorized actor MUST NOT silently select the broadest or most severe interpretation.

The actor must:

- Seek clarification.
- Use a clearly authorized narrower interpretation.
- Decline to proceed.
- Follow an applicable emergency policy.

### 6.7 Authority does not establish truth

An authority may authorize action based on incomplete, conflicting, or emergency evidence.

The authorization does not prove that the supporting assessment is correct.

### 6.8 Denial does not establish safety

A denied request does not prove that the target is healthy, safe, or uncompromised.

Denial concerns authorization under the submitted request and authority context.

### 6.9 Authorization cannot redefine adjacent architecture

An immune authorization MUST NOT silently redefine:

- ACS admission.
- ACS trust.
- ACS capability.
- ACS delegation.
- ACS revocation.
- MEM persistence.
- MEM custody.
- MEM retention.
- MEM reconstruction.
- MEM deletion.
- Runtime ownership.
- Resource ownership.
- Adaptive-state authority.

### 6.10 Authorization changes require lineage

Approval, denial, narrowing, modification, renewal, suspension, revocation, correction, and supersession MUST remain attributable.

---

## 7. Authorization Request Contract

Every authorization request MUST be attributable as a distinct logical work product.

### 7.1 Request identity

A request MUST have an identity sufficient to support:

- Recommendation reference.
- Decision reference.
- Revision.
- Withdrawal.
- Supersession.
- Renewal.
- Conflict reporting.
- Audit.
- Enforcement lineage.
- Recovery review.
- Restoration review.

A materially changed request MUST NOT silently retain an identity implying unchanged meaning.

### 7.2 Requester identity and role

The requester MUST be attributable.

The request MUST identify that it was produced under the Authorization Requester role defined by IMM-0002.

Possessing this role does not make the requester an authority source.

### 7.3 Supporting recommendation

The request MUST identify or reference the recommendation being submitted.

The request MUST preserve:

- Recommendation identity.
- Recommender identity.
- Supporting assessment lineage.
- Material uncertainty.
- Proposed objective.
- Proposed target.
- Proposed action class.
- Proposed scope.
- Known collateral effects.
- Essential paths.
- Review and exit conditions where present.

### 7.4 Departure from recommendation

A request MAY differ from the recommendation.

When it does, the requester MUST identify:

- What changed.
- Why it changed.
- Whether scope increased or decreased.
- Whether the action class changed.
- Whether constraints changed.
- Whether collateral effects changed.
- Whether renewed assessment or recommendation review is needed.

A requester MUST NOT silently broaden a recommendation.

### 7.5 Authority destination

The request MUST identify the authority source or authority class being asked to decide.

Where the exact authority source is unknown, the request MAY target a governed authority-discovery mechanism.

The requester MUST NOT treat inability to identify authority as permission to act.

### 7.6 Requested actor

The request SHOULD identify who or what would be permitted to act.

The requested actor may be:

- A named component.
- An immune Containment Coordinator.
- An Enforcement Adapter.
- An ACS-controlled mechanism.
- A runtime authority.
- A resource owner.
- A MEM-governed operation.
- A human operator.
- A bounded service class.
- A separately authorized invocation context.

Authorization for one actor MUST NOT automatically transfer to another actor.

### 7.7 Requested target

The request MUST identify the proposed target as narrowly as reasonably possible.

The target may be:

- A connection.
- A capability.
- A delegation.
- A runtime task.
- A process.
- A device.
- A resource.
- A memory object.
- A memory operation.
- A node.
- A group of nodes.
- An immune component.
- A bounded subsystem.
- An explicitly unresolved target requiring scope determination.

A request MUST distinguish:

- Primary target.
- Potentially affected subjects.
- Protected subjects.
- Out-of-scope subjects.

### 7.8 Requested objective

The request MUST state the intended objective.

Objectives may include:

- Obtain evidence.
- Prevent further damage.
- Reduce exposure.
- Preserve safety.
- Preserve memory integrity.
- Preserve evidence.
- Maintain diagnostic access.
- Isolate a failing resource.
- Restrict communication.
- Enable recovery.
- Verify recovery.
- Restore bounded access.
- Review an immune component.

### 7.9 Requested action class

The request MUST identify the public action class requiring authorization.

Examples include:

- Expanded observation.
- Evidence access.
- ACS admission review.
- ACS trust review.
- ACS capability restriction.
- ACS delegation review.
- ACS revocation review.
- ACS connection containment.
- Runtime pause.
- Runtime isolation.
- Resource exclusion.
- Resource restriction.
- Governed memory inspection.
- Governed memory recovery.
- Containment continuation.
- Containment narrowing.
- Containment release.
- Recovery work.
- Recovery verification.
- Restoration.
- Audit review.

The action class MUST NOT be disguised as a vague request for “full immune control.”

### 7.10 Requested scope

The request MUST define the proposed scope.

Scope may include:

- Named target.
- Named actor.
- Connection subset.
- Capability subset.
- Operation class.
- Runtime task set.
- Resource set.
- Memory object set.
- Time interval.
- Geographic or hardware region.
- Diagnostic path.
- Recovery path.
- Excluded systems.
- Protected essential paths.

### 7.11 Requested constraints

The request SHOULD identify constraints that the authority should preserve.

Constraints may include:

- Preserve emergency communication.
- Preserve diagnostic access.
- Preserve audit.
- Preserve evidence.
- Preserve recovery control.
- Preserve memory custody.
- Exclude unrelated subjects.
- Prohibit destructive action.
- Prohibit memory deletion.
- Require periodic review.
- Require independent verification.
- Require explicit restoration authorization.
- End on a stated condition.

### 7.12 Requested duration

Where the requested action may persist, the request SHOULD identify:

- Requested start.
- Requested duration.
- Requested expiration.
- Requested review interval.
- Requested renewal condition.
- Requested exit condition.

A request MUST NOT silently ask for indefinite containment.

### 7.13 Collateral effects

The request MUST carry or reference known collateral effects from the recommendation.

It SHOULD also identify new collateral effects introduced by the requested actor, authority path, or implementation context.

### 7.14 Essential paths

The request SHOULD identify paths that must remain available.

Examples include:

- Governance communication.
- Emergency safety.
- Diagnostics.
- Recovery.
- Restoration control.
- Audit.
- Memory custody.
- Health reporting.
- Operator access.

### 7.15 Supporting evidence access

The request SHOULD identify how the authority may access the supporting evidence and assessment lineage.

Evidence access remains governed by ACS, MEM, privacy, and applicable policy.

The authority’s need to review evidence does not automatically grant unrestricted access.

### 7.16 Request urgency

A request MAY identify urgency.

Urgency MUST remain distinct from:

- Severity.
- Confidence.
- Authority.
- Approval.
- Emergency-policy eligibility.

### 7.17 Request disposition

A request SHOULD be capable of representing:

- Draft.
- Submitted.
- Received.
- Under review.
- Pending evidence.
- Pending authority discovery.
- Deferred.
- Approved.
- Partially approved.
- Denied.
- Withdrawn.
- Superseded.
- Expired.
- Closed.

Request disposition MUST NOT be confused with authorization validity.

---

## 8. Request Revision, Withdrawal, and Supersession

### 8.1 Revision

A request MAY be revised when:

- The recommendation changes.
- The assessment changes.
- Scope changes.
- The target changes.
- The proposed actor changes.
- The requested action class changes.
- Collateral effects become clearer.
- The authority asks for modification.

A material revision MUST preserve lineage.

### 8.2 Withdrawal

A requester MAY withdraw a pending request.

Withdrawal MUST identify:

- Request.
- Requester.
- Reason.
- Whether a replacement exists.
- Whether active authorization already exists.
- Whether downstream review is needed.

Withdrawal of a request does not automatically revoke an authorization already issued.

### 8.3 Supersession

A later request MAY supersede an earlier request.

Supersession MUST identify:

- Earlier request.
- Replacement request.
- Changed basis.
- Changed scope.
- Changed target.
- Changed action class.
- Changed constraints.
- Decision status of the earlier request.

### 8.4 Request expiration

A request MAY expire before decision because:

- Its evidence became stale.
- Its recommendation was withdrawn.
- Its subject changed.
- Its review window closed.
- A replacement request was submitted.
- Its target no longer exists.
- The proposed action is no longer relevant.

An expired request MUST NOT later be treated as approved.

---

## 9. Authority Source Contract

### 9.1 Authority attribution

Every authorization decision MUST identify or reference the authority source.

The source may be:

- A human operator.
- A governance body.
- A policy engine.
- A delegated authority.
- A resource owner.
- A runtime authority.
- A MEM authority.
- An ACS-recognized authority.
- An emergency policy.
- Another recognized decision mechanism.

### 9.2 Authority basis

The decision MUST identify or reference the basis under which the source may decide.

The authority basis may include:

- Ownership.
- Delegation.
- Policy.
- Role assignment.
- Jurisdiction.
- Emergency mandate.
- Resource control.
- Runtime control.
- MEM governance.
- ACS-recognized authority.

This document does not define the internal semantics of those bases.

### 9.3 Authority scope

An authority source may decide only within its recognized authority scope.

Authority over one domain does not automatically grant authority over another.

Examples:

- Runtime authority does not automatically grant memory-deletion authority.
- Resource ownership does not automatically grant ACS trust-revocation authority.
- Immune observation authority does not automatically grant containment authority.
- MEM custody authority does not automatically grant runtime isolation authority.

### 9.4 Authority identity uncertainty

Where authority identity or validity cannot be established, the decision MUST NOT be treated as valid authorization.

The state must remain:

- Unverified.
- Partially verified.
- Conflicted.
- Unknown.
- Invalid.

as appropriate.

### 9.5 Authority self-assertion

A component’s claim that it possesses authority is evidence of the claim.

It is not automatically proof of authority.

---

## 10. Authorization Decision Contract

Every authorization decision MUST be attributable as a distinct logical work product.

### 10.1 Decision identity

A decision MUST have an identity sufficient to support:

- Request reference.
- Enforcement reference.
- Renewal.
- Suspension.
- Revocation.
- Supersession.
- Correction.
- Audit.
- Recovery review.
- Restoration review.

### 10.2 Request reference

A decision SHOULD identify the request it answers.

A decision issued under standing or emergency authority without a conventional request MUST identify:

- Authority basis.
- Triggering evidence or event.
- Authorized objective.
- Required retrospective review.
- Missing ordinary workflow elements.

### 10.3 Decision type

A decision MUST identify its type.

Public decision types include:

- Approved.
- Partially approved.
- Approved with modifications.
- Denied.
- Deferred.
- Returned for revision.
- Suspended.
- Revoked.
- Renewed.
- Superseded.
- Corrected.
- Expired.
- Invalidated.

### 10.4 Authorized actor

An approval MUST identify the actor or invocation context permitted to use the authorization.

Authorization MUST NOT silently transfer because:

- A component restarted.
- A service moved.
- A role was reassigned.
- A new node inherited the workload.
- Another component has equivalent technical capability.

Transfer requires authority under the applicable governance model.

### 10.5 Authorized target

An approval MUST identify the authorized target.

The authorized target MUST NOT be inferred more broadly than the decision states.

### 10.6 Authorized objective

An approval SHOULD identify the objective for which the action is authorized.

The objective helps interpret scope but MUST NOT be used to broaden explicit action limits.

### 10.7 Authorized action class

An approval MUST identify the authorized action class.

The authorized actor MUST NOT substitute a more severe or materially different action class without fresh authority.

### 10.8 Authorized scope

An approval MUST define scope.

Scope SHOULD identify:

- Included targets.
- Excluded targets.
- Included action range.
- Excluded actions.
- Time or event boundaries.
- Protected paths.
- Required dependencies.
- Applicable architecture domain.

### 10.9 Authorized constraints

An approval MUST preserve mandatory constraints.

Constraints may include:

- Preserve diagnostic access.
- Preserve emergency communication.
- Preserve audit.
- Preserve evidence.
- Prohibit destructive operations.
- Prohibit memory deletion.
- Restrict action to ACS mechanisms.
- Restrict action to MEM-governed operations.
- Require rollback capability.
- Require independent verification.
- Require periodic review.
- Require human acknowledgement.
- Require restoration review.

### 10.10 Validity conditions

An approval MUST identify or reference conditions under which it is valid.

Validity may depend on:

- Authority identity.
- Actor identity.
- Target identity.
- Current assessment.
- Evidence freshness.
- Incident state.
- Connection state.
- Runtime state.
- Resource state.
- Time.
- Emergency status.
- Continued availability of essential paths.

### 10.11 Expiration

An approval SHOULD identify an expiration condition where continuing authority is involved.

Expiration may be based on:

- Time.
- Completion of one invocation.
- Completion of a bounded action sequence.
- State transition.
- Review deadline.
- Recovery condition.
- Restoration condition.
- Revocation.
- Supersession.

### 10.12 Review conditions

An approval SHOULD identify conditions requiring review.

Examples include:

- New contradictory evidence.
- Expanded target scope.
- Failed enforcement.
- Unexpected collateral effects.
- Loss of an essential path.
- Changed assessment.
- Changed recommendation.
- Changed authority basis.
- Recovery progress.
- Restoration readiness.
- Immune-component failure.

### 10.13 Decision rationale

A decision SHOULD include or reference sufficient rationale to support review and audit.

The authority may rely on private policy.

The public decision record need not disclose protected policy internals.

It MUST still preserve the decision’s bounded meaning and authority basis.

### 10.14 Decision acknowledgement

The authorized actor MUST acknowledge receipt and interpretation of the authorization before enforcement begins.

Acknowledgement MUST identify:

- Decision received.
- Actor.
- Interpreted target.
- Interpreted action class.
- Interpreted scope.
- Constraints.
- Validity.
- Any ambiguity.

If a recognized emergency authorization explicitly permits immediate invocation before a separate acknowledgement can be recorded, the invocation itself MAY serve as provisional acknowledgement.

The actor MUST record the complete acknowledgement as part of or immediately after the invocation, and the emergency exception MUST remain attributable.

Acknowledgement is not enforcement.

---

## 11. Decision Types

### 11.1 Approval

Approval authorizes the requested action only within the stated envelope.

### 11.2 Partial approval

Partial approval authorizes only a subset of the request.

It MUST identify:

- Approved portion.
- Denied or undecided portion.
- Changed scope.
- Changed duration.
- Changed constraints.
- Whether a revised request is required.

The authorized actor MUST NOT execute the unapproved remainder.

### 11.3 Approval with modification

An authority may change:

- Actor.
- Target.
- Action class.
- Scope.
- Duration.
- Constraints.
- Review conditions.
- Exit conditions.

Material modification creates a decision different from the submitted request.

The requester and authorized actor SHOULD review the modified envelope before execution.

### 11.4 Denial

A denial MUST identify the request or action being denied.

Where policy permits, it SHOULD identify:

- Reason.
- Missing authority.
- Missing evidence.
- Excessive scope.
- Unacceptable collateral effects.
- Available narrower alternative.
- Conditions for reconsideration.

Denial does not prove that the assessed condition is false.

### 11.5 Deferral

Deferral postpones final decision.

It SHOULD identify:

- Reason.
- Required evidence.
- Required review.
- Expected decision condition.
- Whether any provisional action is separately authorized.

Deferral does not authorize the requested action.

### 11.6 Return for revision

An authority may return a request for narrower scope, clearer target, different actor, stronger constraints, or additional lineage.

The returned request remains unauthorized unless separate provisional authority exists.

### 11.7 Suspension

Suspension temporarily prevents future use of an authorization.

Suspension MUST identify:

- Authorization affected.
- Effective time.
- Reason or authority basis.
- Conditions for resumption.
- Status of already executing actions.

### 11.8 Revocation

Revocation ends future use of an authorization.

Revocation MUST identify:

- Authorization.
- Revoking authority.
- Effective time.
- Scope.
- Required handling of pending or active actions.
- Whether containment or rollback review is required.

Revocation of authorization does not automatically reverse actions already completed.

### 11.9 Renewal

Renewal extends or replaces authorization beyond its prior validity.

Renewal MUST be a new attributable decision.

It SHOULD reconsider:

- Current assessment.
- Current recommendation.
- Continued need.
- Collateral effects.
- Recovery progress.
- Changed target state.
- Changed authority basis.

### 11.10 Supersession

A new decision may supersede an earlier one.

Supersession MUST identify:

- Prior authorization.
- New authorization.
- Effective transition.
- Remaining valid portions.
- Invalidated portions.
- Effect on pending and active actions.

### 11.11 Correction

A decision may be corrected when it contains an attribution, scope, target, actor, timing, or transcription error.

A correction MUST preserve the original decision and identify the corrected meaning.

A correction MUST NOT be used to conceal unauthorized expansion after action occurred.

---

## 12. Authorization Envelope

A usable authorization envelope MUST identify or reference, where applicable:

1. Decision identity.
2. Authority source.
3. Authority basis.
4. Request identity.
5. Supporting recommendation.
6. Authorized actor.
7. Authorized target.
8. Authorized objective.
9. Authorized action class.
10. Authorized scope.
11. Mandatory constraints.
12. Validity conditions.
13. Effective time.
14. Expiration condition.
15. Review conditions.
16. Exit or transition conditions.
17. Supersession state.
18. Suspension state.
19. Revocation state.
20. Audit lineage.

An authorization lacking enough information to determine its bounded meaning MUST NOT be treated as unlimited authority.

---

## 13. Scope Interpretation

### 13.1 Exact target interpretation

Authorization for one target does not authorize action against:

- Neighboring nodes.
- Related connections.
- Shared resources.
- Dependent services.
- Other instances.
- Future replacements.

unless they are explicitly included.

### 13.2 Exact action interpretation

Authorization for observation does not authorize containment.

Authorization for containment does not authorize deletion.

Authorization for runtime pause does not authorize memory modification.

Authorization for recovery does not authorize restoration.

### 13.3 Exact duration interpretation

Authorization valid for one interval MUST NOT be reused after expiration.

### 13.4 Exact actor interpretation

Authorization granted to one actor MUST NOT be treated as generally available to every immune component.

### 13.5 Unknown scope

When target, actor, or scope cannot be determined, the authorization MUST remain unusable until clarified, unless an applicable emergency contract explicitly defines a narrower safe interpretation.

---

## 14. Multiple Authorities

### 14.1 Domain-specific authority

A response may require decisions from multiple authority domains.

For example:

- ACS authority for connection restriction.
- Runtime authority for process isolation.
- Resource authority for device exclusion.
- MEM authority for governed recovery.
- Human authority for high-impact action.

Approval in one domain does not imply approval in another.

### 14.2 Composite authorization

A composite response MAY reference multiple authorizations.

The coordinator MUST preserve each authorization’s:

- Source.
- Domain.
- Scope.
- Constraints.
- Validity.
- Expiration.

The broadest component MUST NOT be treated as broadening the others.

### 14.3 Conflicting decisions

Authorities may issue conflicting decisions.

Conflict MUST remain explicit.

An authorized actor MUST NOT resolve conflict by silently choosing:

- The most severe decision.
- The least restrictive decision.
- The newest decision.
- The most privileged-looking source.

Conflict resolution must follow recognized governance.

### 14.4 Overlapping authority

Overlapping authorities MAY both be valid.

The implementation MUST preserve:

- Which authority governs which action.
- Whether both are required.
- Which constraints are cumulative.
- Which decision supersedes another.
- Whether conflict exists.

### 14.5 Missing required authority

Where an action requires multiple authorities and one is missing, the actor MUST NOT treat partial approval as complete authorization.

---

## 15. Emergency Authorization

### 15.1 Emergency authority must be preestablished

Emergency authorization must derive from a recognized emergency policy or delegated authority.

An Assessor, Recommender, Requester, Coordinator, or Adapter cannot create emergency authority by declaring an emergency.

### 15.2 Emergency request

An emergency request SHOULD identify:

- Emergency policy basis.
- Available evidence.
- Missing evidence.
- Supporting assessment.
- Supporting recommendation.
- Requested target.
- Requested action.
- Requested scope.
- Essential paths.
- Known collateral effects.
- Requested expiration.
- Required retrospective review.

### 15.3 Standing emergency authorization

A standing emergency authorization MAY permit predefined action when predefined conditions are met.

It MUST identify:

- Authorized actor.
- Authorized action class.
- Authorized target class.
- Maximum scope.
- Constraints.
- Activation conditions.
- Expiration or deactivation conditions.
- Required reporting.
- Required review.
- Prohibited actions.

Standing emergency authority MUST NOT become general immune authority.

### 15.4 Emergency decision

An emergency decision may rely on incomplete information.

The decision MUST preserve:

- Uncertainty.
- Authority basis.
- Bounded scope.
- Time limits.
- Review requirements.
- Accountability.

### 15.5 Emergency expiration

Emergency authorization SHOULD expire quickly enough to require governed reconsideration.

This document does not define a numeric duration.

### 15.6 Retrospective review

Emergency use SHOULD receive later review of:

- Trigger validity.
- Evidence.
- Assessment.
- Recommendation.
- Authority.
- Scope.
- Execution.
- Collateral effects.
- Continued need.
- Recovery.
- Restoration.
- Audit completeness.

---

## 16. Renewal and Continuing Authorization

### 16.1 No silent renewal

Authorization MUST NOT renew merely because:

- Containment remains active.
- The target has not recovered.
- No one reviewed the case.
- The original authority is unavailable.
- The system continues to enforce the restriction.

### 16.2 Renewal request

A renewal request SHOULD identify:

- Existing authorization.
- Current assessment.
- Current recommendation.
- Current target state.
- Actions completed.
- Actions failed.
- Collateral effects.
- Recovery progress.
- Continued need.
- Proposed new validity.
- Proposed exit conditions.

### 16.3 Renewal decision

A renewal decision MUST be attributable as new authority.

It MAY:

- Extend unchanged authority.
- Narrow authority.
- Modify constraints.
- Change actor.
- Change scope.
- Require new verification.
- Deny continuation.
- Transition toward recovery or restoration.

### 16.4 Indefinite action

A continuing restriction MUST NOT become indefinite merely through repeated administrative inattention.

Long-lived action requires governed review appropriate to its impact and authority basis.

---

## 17. Suspension, Revocation, and Active Work

### 17.1 Pending work

When authorization is suspended or revoked before invocation, the action MUST NOT begin.

### 17.2 Active work

When suspension or revocation occurs during active work, the applicable decision SHOULD specify whether the actor must:

- Stop immediately.
- Complete a safe bounded step.
- Enter rollback.
- Preserve containment.
- Await further authority.
- Transition to emergency safety behavior.

### 17.3 Completed work

Revocation does not make completed action never have occurred.

Completed action and its effects remain part of history.

### 17.4 Enforcement state

The authorized actor or adapter MUST NOT report authorization revocation as proof that enforcement reversed successfully.

Reversal requires separate execution and verification.

### 17.5 Restoration implications

Revocation of containment authority does not automatically authorize restoration.

Restoration requires its own governed basis and verification.

---

## 18. Authorization and Enforcement Boundary

### 18.1 Authorized plan

The Containment Coordinator may translate authorization into a bounded plan.

The plan MUST remain within the authorization envelope.

### 18.2 Enforcement invocation

The Enforcement Adapter must verify authorization and the required acknowledgement before invocation.

Verification SHOULD include:

- Decision identity.
- Authority source.
- Actor.
- Target.
- Action class.
- Scope.
- Constraints.
- Validity.
- Expiration.
- Suspension.
- Revocation.
- Supersession.

A recognized emergency authorization MAY explicitly permit immediate invocation before a separate acknowledgement is complete, but only under the exception and recording requirements of Section 10.14.

### 18.3 Enforcement acceptance

Acceptance of an enforcement request by ACS, MEM, runtime, or resource systems does not prove authorization was correct.

### 18.4 Enforcement refusal

A responsible system MAY refuse an authorized request when:

- The request exceeds its interface contract.
- Authority cannot be verified.
- The target is invalid.
- The request is stale.
- The authorization expired.
- Safety constraints prohibit execution.
- The action is unsupported.
- The system is unavailable.

Refusal does not automatically invalidate the authority decision.

### 18.5 Execution receipt

Execution results must remain separate from authorization.

A receipt SHOULD identify:

- Authorization used.
- Request submitted.
- System invoked.
- Acceptance.
- Completion.
- Partial completion.
- Failure.
- Unknown result.
- Resulting state where known.

---

## 19. Cross-Architecture Boundaries

### 19.1 ACS boundary

ACS remains authoritative for:

- Connection identity.
- Admission.
- Trust.
- Capability.
- Delegation.
- Revocation.
- Connection lifecycle.
- Communication restriction.
- ACS enforcement.

An immune authorization may permit a request to ACS.

It MUST NOT redefine ACS semantics or bypass ACS mechanisms.

### 19.2 MEM boundary

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

An immune authorization may permit a MEM-governed request.

It MUST NOT directly rewrite, reconstruct, relocate, retain, or delete memory outside MEM contracts.

### 19.3 Runtime boundary

Runtime systems remain authoritative for:

- Execution.
- Scheduling.
- Placement.
- Process lifecycle.
- Runtime isolation.
- Runtime recovery mechanisms.

An immune authorization may permit a runtime request.

It does not replace the runtime’s own safety and interface contracts.

### 19.4 Resource boundary

Resource owners remain authoritative for:

- Access.
- Allocation.
- Placement.
- Reservation.
- Exclusion.
- Device lifecycle.

Immune authorization does not itself seize a resource.

### 19.5 Adaptive-state boundary

Authorization to contain, inspect, or recover a system does not automatically authorize rewriting adaptive state, learned state, or cognitive content.

Such action requires its own governed contract.

### 19.6 Human authority boundary

Human approval is not inherently unlimited.

A human authority source remains bounded by its recognized authority basis.

---

## 20. Authorization Failure

### 20.1 Unavailable authority

When authority is unavailable:

- The request remains pending, deferred, expired, or unresolved.
- The requester MUST NOT invent approval.
- Existing authorization may be used only within its original validity.
- Emergency authority may be used only within its established envelope.

### 20.2 Invalid authority

A decision from an unrecognized or invalid authority source MUST NOT be treated as authorization.

The decision may remain as evidence.

### 20.3 Ambiguous decision

An ambiguous decision MUST be clarified or interpreted narrowly within clearly authorized meaning.

### 20.4 Stale decision

An expired or stale decision MUST NOT be treated as current authority.

### 20.5 Conflicting decision

Conflicting decisions require governed resolution.

Conflict is not permission to choose whichever decision enables action.

### 20.6 Decision-delivery failure

Failure to deliver a decision does not convert denial into approval or approval into denial.

### 20.7 Acknowledgement failure

Failure of the authorized actor to acknowledge a decision means enforcement MUST NOT begin, except under the explicit emergency exception defined in Section 10.14.

It does not prove that the decision was not received.

It does mean the system lacks a conforming confirmation of interpretation.

### 20.8 Authorization-system failure

Failure of the authorization system is evidence about governance infrastructure.

It is not automatically evidence against the target.

---

## 21. Immune Self-Authorization Prohibition

An immune component MUST NOT authorize itself merely because it:

- Detected the condition.
- Produced the evidence.
- Performed the assessment.
- Produced the recommendation.
- Submitted the request.
- Has technical ability to enforce.
- Is highly trusted.
- Is the only available component.
- Claims emergency status.

A component may act as an authority source only when it separately possesses recognized authority.

That authority context MUST remain distinct from its immune role.

---

## 22. Privacy and Protected Information

### 22.1 Minimum necessary disclosure

An authorization request SHOULD disclose only the information reasonably necessary for the authority to decide.

### 22.2 Protected evidence

Supporting evidence may be referenced without being fully embedded where access is restricted.

The authority may require governed access.

### 22.3 Cognitive content

An authorization request involving cognitive content MUST identify:

- Why access or action is required.
- Minimum necessary scope.
- Applicable authority.
- Handling restrictions.
- Retention implications.
- Review conditions.

### 22.4 Decision confidentiality

An authorization decision may contain protected details.

The public contract still requires that authorized actors can determine the bounded meaning necessary for conformance.

---

## 23. Conforming Example: Resource Exclusion

An assessment concludes that one accelerator is degraded but does not establish compromise.

A recommendation proposes temporary exclusion from new workload placement while preserving diagnostic access.

The Authorization Requester submits a request to the resource authority identifying:

- The named accelerator.
- The placement-exclusion action class.
- The responsible runtime adapter.
- Diagnostic access as an essential path.
- A review condition after diagnostics.
- No authority to affect other accelerators.

The resource authority partially approves the request:

- New placement is prohibited on the named accelerator.
- Existing diagnostic tasks may continue.
- Other accelerators remain unaffected.
- Authorization expires after review or explicit renewal.

The authorized actor acknowledges the decision envelope before the Enforcement Adapter invokes the resource-management interface.

This is conforming because the authorization is bounded by actor, target, action, scope, constraints, and review.

---

## 24. Conforming Example: ACS Restriction

An assessment finds suspicious behavior on one connection but does not establish peer-wide compromise.

The recommendation proposes restricting one capability on that connection while preserving diagnostic communication.

The request is sent to the appropriate ACS-recognized authority.

The authority approves:

- Restriction of the named capability.
- On the named connection only.
- Through ACS mechanisms.
- For a bounded review period.
- While preserving the diagnostic capability.
- With no change to unrelated delegations.

The authorized actor acknowledges the target, action class, scope, and constraints before enforcement begins.

This is conforming because immune authorization does not redefine ACS capability or enforcement semantics.

---

## 25. Conforming Example: Multi-Domain Response

A failing process is producing corrupted memory-operation requests.

The proposed response requires:

- Runtime isolation of the process.
- ACS restriction of its communication path.
- MEM review of affected operations.

Three separate authority domains issue bounded decisions.

The Containment Coordinator preserves each envelope separately and executes only after the required approvals and acknowledgements exist.

No authority is interpreted as granting power over another domain.

This is conforming because composite response does not collapse distinct authorities.

---

## 26. Conforming Example: Emergency Containment

A standing emergency policy authorizes a named safety component to temporarily block a narrowly defined communication class when direct physical safety evidence is present.

The policy defines:

- Actor.
- Target class.
- Maximum restriction.
- Preserved emergency channel.
- Short validity.
- Required report.
- Required review.
- Prohibited actions.
- Permission for invocation to serve as provisional acknowledgement when delay would defeat the safety purpose.

The trigger occurs.

The component invokes only the pre-authorized restriction, records the complete acknowledgement as part of or immediately after invocation, and submits the evidence, assessment, recommendation, and action record for review.

This is conforming because emergency timing is shortened without creating unlimited authority or removing acknowledgement accountability.

---

## 27. Nonconforming Examples

The following behavior is nonconforming:

- Treating a recommendation as authorization.
- Treating request submission as approval.
- Treating authority silence as approval.
- Allowing the requester to approve its own request without separate recognized authority.
- Accepting self-asserted authority without verification.
- Using authorization for one target against related targets.
- Using authorization for observation to perform containment.
- Using authorization for quarantine to delete memory.
- Using runtime authority to revoke ACS trust.
- Using resource authority to alter MEM custody.
- Reusing expired authorization.
- Continuing after revocation without governed safety instructions.
- Interpreting ambiguous authority in the broadest possible way.
- Treating partial approval as full approval.
- Treating one domain’s approval as approval in every domain.
- Treating denial as proof of target safety.
- Treating approval as proof of target compromise.
- Beginning non-emergency enforcement before the authorized actor acknowledges the decision envelope.
- Claiming an emergency acknowledgement exception that the authorization did not explicitly grant.
- Treating successful enforcement as proof that the authorization objective was achieved.
- Treating removal of containment authority as restoration authorization.
- Renewing containment automatically because no review occurred.
- Allowing emergency authorization to become permanent through inattention.
- Hiding modification of a request.
- Correcting a decision after execution to conceal unauthorized scope.
- Publishing private authority thresholds or emergency triggers in the public contract.

---

## 28. Public and Private Boundary

The following belongs in public architecture:

- Request identity.
- Requester attribution.
- Recommendation lineage.
- Requested actor.
- Requested target.
- Requested objective.
- Requested action class.
- Requested scope.
- Requested constraints.
- Requested duration.
- Authority-source attribution.
- Authority-basis attribution.
- Decision identity.
- Decision types.
- Authorized actor.
- Authorized target.
- Authorized action class.
- Authorized scope.
- Constraints.
- Validity.
- Expiration.
- Renewal.
- Suspension.
- Revocation.
- Supersession.
- Partial approval.
- Conflict semantics.
- Decision-acknowledgement requirements.
- Emergency-authorization boundaries.
- Enforcement separation.
- Cross-architecture boundaries.
- Failure semantics.
- Conformance requirements.

The following SHOULD remain implementation-private unless separately approved:

- Internal policy formulas.
- Private approval thresholds.
- Protected authority hierarchy details.
- Secret emergency triggers.
- Human identity-verification details.
- Credential material.
- Key material.
- Private delegation graphs.
- Protected escalation rules.
- Secret quorum rules.
- Internal decision-ranking logic.
- Protected operational topology.
- Adversarial authorization tests that would weaken security.

Private policy may evolve without changing the public contract.

Private policy MUST remain conformant with the public authority boundaries.

---

## 29. Conformance Requirements

An implementation conforms to this document only when:

1. Every authorization request is attributable.
2. Every decision is attributable.
3. Request and recommendation remain distinct.
4. Request and authorization remain distinct.
5. Authorization and enforcement remain distinct.
6. Supporting recommendation lineage is preserved.
7. Supporting assessment lineage remains referenceable.
8. The requested authority destination is identified or discoverable.
9. Requested actor is bounded where applicable.
10. Requested target is bounded.
11. Requested objective is explicit.
12. Requested action class is explicit.
13. Requested scope is bounded.
14. Known constraints remain visible.
15. Known collateral effects remain visible.
16. Essential paths are identified where material.
17. Continuing action includes duration, expiration, or review semantics.
18. Authority source is attributable.
19. Authority basis is attributable.
20. Authority scope is not silently broadened.
21. Self-asserted authority is not automatically accepted.
22. Approval identifies authorized actor.
23. Approval identifies authorized target.
24. Approval identifies authorized action class.
25. Approval identifies authorized scope.
26. Mandatory constraints are preserved.
27. Validity conditions are representable.
28. Expiration is representable.
29. Partial approval remains partial.
30. Denial is not interpreted as proof of safety.
31. Approval is not interpreted as proof of compromise.
32. Deferral is not interpreted as approval.
33. Suspension prevents new use.
34. Revocation ends future use.
35. Renewal requires a new attributable decision.
36. Supersession preserves lineage.
37. Ambiguity does not grant broader authority.
38. Multi-domain authority remains separated.
39. Missing required authority prevents composite action from being treated as fully authorized.
40. Emergency authority is preestablished.
41. Emergency authorization remains bounded and reviewable.
42. Authorization does not redefine ACS.
43. Authorization does not redefine MEM.
44. Authorization does not replace runtime authority.
45. Authorization does not replace resource ownership.
46. Authorization does not grant ungoverned adaptive-state modification.
47. Authorized actors MUST acknowledge the authorization envelope before enforcement, except where an explicit emergency authorization permits immediate invocation and requires acknowledgement during or immediately after invocation.
48. Authorized actors verify the envelope before invocation.
49. Execution receipts remain distinct from authorization.
50. Revocation does not prove reversal succeeded.
51. Restoration requires separate governed authority.
52. Immune roles cannot self-authorize merely by role possession.
53. Authorization-system failure does not automatically implicate the target.
54. Public contracts do not disclose private authority heuristics or triggers.

---

## 30. Required Failure Semantics

Authorization failures MUST be representable without inventing authority or conclusions.

Specifically:

- Missing authority does not imply approval.
- Missing authority does not imply denial.
- Authority unavailability does not prove target compromise.
- Authority unavailability does not prove target safety.
- Invalid authority does not invalidate the underlying assessment automatically.
- Denial does not prove the recommendation was unreasonable.
- Approval does not prove the recommendation was correct.
- Deferral does not permit provisional action unless separately authorized.
- Request expiration does not become denial.
- Authorization expiration does not become renewal.
- Suspension does not prove the target recovered.
- Revocation does not prove enforcement reversed.
- Supersession does not erase prior action.
- Conflicting authority does not permit arbitrary selection.
- Failed delivery does not change decision meaning.
- Failed acknowledgement blocks non-emergency enforcement but does not prove nonreceipt.
- Failed enforcement does not invalidate authority automatically.
- Successful enforcement does not prove the objective was achieved.
- Authorization-system failure is evidence about governance infrastructure, not automatically against the target.

The failure state may become evidence under IMM-0003 and may support new assessment under IMM-0004.

---

## 31. Architectural Commitments

This document establishes the following commitments:

1. Recommendations do not create authority.
2. Requests do not create authority.
3. Authority must preexist the decision.
4. Requests and decisions are attributable.
5. Authorization is actor-bounded.
6. Authorization is target-bounded.
7. Authorization is action-bounded.
8. Authorization is scope-bounded.
9. Authorization is constraint-bounded.
10. Authorization is validity-bounded.
11. Authorization may expire, suspend, revoke, or be superseded.
12. Silence is not approval.
13. Ambiguity does not grant broader power.
14. Partial approval remains partial.
15. Multi-domain responses preserve distinct authority envelopes.
16. Authorized actors acknowledge and confirm the authorization envelope before non-emergency enforcement.
17. Emergency acknowledgement exceptions must be explicit, bounded, attributable, and recorded during or immediately after invocation.
18. Emergency authority is preestablished and reviewable.
19. Authorization does not prove factual correctness.
20. Denial does not prove safety.
21. Authorization does not equal enforcement.
22. Enforcement does not equal successful outcome.
23. Revocation does not equal reversal.
24. Recovery authority does not equal restoration authority.
25. ACS retains ACS authority and enforcement semantics.
26. MEM retains memory lifecycle authority.
27. Runtime and resource owners retain operational authority.
28. Adaptive state cannot be rewritten through implied immune authority.
29. Immune roles cannot self-authorize by role possession.
30. Authorization history remains attributable through correction, renewal, suspension, revocation, and supersession.
31. Public contracts remain stable while private governance policy may evolve.

---

## 32. Future Work

Later immune architecture documents may define:

- Containment lifecycle and action-plan contracts.
- Enforcement invocation and receipt contracts.
- Recovery-verification contracts.
- Restoration-authorization and verification contracts.
- Audit-record contracts.
- Authority-discovery mechanisms.
- Multi-authority coordination.
- Delegated emergency-operation conformance.
- Partition-aware authorization.
- Offline authorization handling.
- Authorization-cache validity.
- Adversarial authorization testing.
- Immune authority self-review.
- Public conformance scenarios.

Those documents must preserve the evidence, assessment, recommendation, authority, ACS, MEM, runtime, resource, recovery, restoration, and accountability boundaries established in this series.
