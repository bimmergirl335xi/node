# IMM-0003: Observation and Evidence Contracts

**Status:** Draft  
**Series:** Public Immune Architecture  
**Classification:** Public architecture contract  
**Depends on:** IMM-0000, IMM-0001, IMM-0002  
**Related architecture:** ACS-0006 through ACS-0009; MEM-0003 through MEM-0010  
**Supersedes:** None

---

## 1. Abstract

This document defines the public contracts by which Node’s immune architecture records observations, forms evidence, preserves provenance, represents uncertainty, identifies conflicts, and supplies evidence to later assessment and response processes.

An observation is a recorded account of a condition, event, state, absence, or failed attempt.

Evidence is an observation, record, artifact, or derived work product made available for a stated evaluative purpose.

Neither observation nor evidence is a verdict.

Evidence may be valid, authentic, complete, and relevant without proving compromise. Evidence may also be incomplete, stale, dependent, contradictory, reconstructed, or unavailable without proving that the observed subject is safe.

This document defines the semantics required for evidence to remain attributable, reviewable, bounded, and usable across a distributed mesh. It does not define private detection heuristics, scoring formulas, response thresholds, ACS authority, or MEM persistence policy.

---

## 2. Purpose

The purpose of this document is to establish:

1. What constitutes an observation.
2. What constitutes an evidence item.
3. The information required to attribute and interpret each item.
4. How direct, reported, derived, reconstructed, and self-produced evidence differ.
5. How freshness, completeness, availability, integrity, dependence, and uncertainty are represented.
6. How transformations and derivations preserve lineage.
7. How duplicates, contradictions, and conflicting reports are handled.
8. How failed collection and missing evidence are represented.
9. How immune evidence remains distinct from protected cognitive content.
10. How ACS and MEM retain authority over transport, access, persistence, custody, retention, reconstruction, and deletion.
11. The public conformance requirements for evidence handling.

This document prevents evidence from becoming an unattributed stream of claims whose origin, meaning, scope, and limitations cannot be reconstructed.

---

## 3. Normative Language

The terms **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **MAY**, and **OPTIONAL** are normative.

The following terms are used throughout this document:

**Subject**  
The node, component, connection, memory operation, runtime action, resource, role, or bounded system condition to which an observation refers.

**Source**  
The entity, mechanism, record, or process from which the reported information originated.

**Collector**  
The component that acquired, received, or recorded the observation.

**Producer**  
The component that created a particular evidence representation or derived evidence item.

**Observation**  
A bounded record that a condition, value, event, absence, response, or failure was perceived or reported.

**Evidence item**  
A uniquely attributable observation, artifact, record, or derivative offered for a stated evaluative purpose.

**Evidence set**  
A collection of evidence items assembled for analysis, correlation, assessment, verification, or audit.

**Claim**  
A proposition asserted by a source or producer.

**Lineage**  
The attributable chain connecting an evidence item to its origins, transformations, prior versions, and relevant handling events.

**Validation**  
An evaluation of whether evidence is usable for a stated purpose. Validation does not establish the guilt, safety, health, or intent of the subject.

**Unknown**  
A value or condition that has not been established.

**Unavailable**  
Information that may exist but cannot currently be accessed.

**Absent**  
A condition in which a defined collection attempt did not observe an expected item within the stated scope and time.

Absent does not automatically mean nonexistent.

---

## 4. Scope

This document defines public contracts for:

- Observation identity.
- Evidence identity.
- Subject attribution.
- Source attribution.
- Collection context.
- Observation timing.
- Evidence timing.
- Scope.
- Provenance.
- Integrity declarations.
- Transformation lineage.
- Validation state.
- Completeness.
- Freshness.
- Availability.
- Dependence.
- Duplication.
- Contradiction.
- Uncertainty.
- Evidence requests.
- Collection failures.
- Evidence-set composition.
- Cross-node evidence exchange semantics.
- Evidence about immune components.
- Public and private classification boundaries.

This document applies to evidence concerning:

- Node health.
- Runtime behavior.
- Security conditions.
- ACS activity.
- Memory-operation results.
- Resource behavior.
- Hardware behavior.
- Communication failures.
- Recovery.
- Restoration.
- Immune-role performance.
- Audit conformance.

---

## 5. Explicit Non-Goals

This document does not define:

- Detection algorithms.
- Anomaly models.
- Evidence-scoring formulas.
- Confidence calculations.
- Correlation weights.
- Threat rankings.
- Private indicators of compromise.
- Alert thresholds.
- Automatic escalation thresholds.
- Response-selection algorithms.
- Admission policy.
- Trust policy.
- Capability policy.
- Delegation or revocation policy.
- ACS enforcement behavior.
- Memory storage formats.
- Memory placement.
- Memory custody policy.
- Retention periods.
- Reconstruction algorithms.
- Deletion policy.
- Runtime scheduling.
- Resource-allocation policy.
- Cryptographic algorithms.
- Secret key management.
- Hardware-specific diagnostic values.
- A requirement that all evidence be globally visible.
- A requirement that all nodes collect the same evidence.
- A centralized evidence repository.
- A universal scalar measure of evidence quality.

Those concerns belong to ACS, MEM, runtime architecture, resource management, private implementation, or later immune specifications.

---

## 6. Foundational Rules

### 6.1 Observation is not interpretation

An observation records what was perceived, reported, attempted, or unavailable within a defined context.

Interpretation belongs to correlation and assessment roles.

An observation MUST NOT silently include an unstated verdict.

The observation:

> “Three responses failed to arrive within the stated observation window”

is distinct from:

> “The peer is malicious.”

The first may be an observation.

The second is an assessment or claim requiring supporting evidence.

### 6.2 Evidence is purpose-bounded

Evidence is offered for a stated purpose.

An item suitable for diagnosing runtime failure may not be suitable for establishing compromise.

An item suitable for confirming message origin may not be suitable for confirming message truth.

Validation for one purpose MUST NOT silently become validation for another.

### 6.3 Authenticity is not truth

Evidence may be cryptographically attributable to a source and still be:

- Mistaken.
- Incomplete.
- Stale.
- Misconfigured.
- Deceptive.
- Compromised.
- Outside the source’s knowledge.
- Correct only within a narrow scope.

Proof that a source produced a claim does not prove the claim is correct.

### 6.4 Absence is not proof

Failure to observe an event MUST NOT automatically be treated as proof that the event did not occur.

Absence becomes meaningful only within a stated:

- Observation scope.
- Collection method.
- Time interval.
- Coverage boundary.
- Availability condition.
- Expected reporting behavior.

### 6.5 Evidence does not create authority

No evidence item or evidence set creates authority to:

- Contain a subject.
- Quarantine a subject.
- Revoke trust.
- Restrict capability.
- Disconnect a peer.
- Alter memory.
- Delete memory.
- Rewrite adaptive state.
- Seize resources.
- Restore access.

Evidence may support assessment and recommendation.

Authority remains governed by the appropriate external architecture and authorization source.

### 6.6 Unknown remains unknown

Missing values MUST remain unknown, unavailable, absent, not collected, or not applicable as appropriate.

A conforming implementation MUST NOT replace unknown values with:

- Zero.
- Healthy.
- Safe.
- Trusted.
- Compromised.
- Successful.
- Failed.

unless the contract for that field explicitly defines such an interpretation.

---

## 7. Observation Contract

Every observation MUST be attributable as a distinct logical record.

A conforming observation contains or references the following information where applicable.

### 7.1 Observation identity

An observation MUST have an identity sufficient to distinguish it from other observations.

Observation identity MUST support:

- Reference by later evidence items.
- Duplicate detection.
- Version or derivation tracking.
- Audit.
- Conflict reporting.

The identity mechanism is implementation-defined.

Persistent identity and version semantics remain governed by MEM where the observation is retained as memory.

### 7.2 Subject

An observation MUST identify its subject as narrowly as reasonably possible.

The subject may be:

- A node.
- A device.
- A connection.
- A port.
- A process.
- A runtime task.
- A memory operation.
- A resource.
- An immune role.
- An authorization.
- An enforcement request.
- A recovery action.
- A bounded subsystem.
- An explicitly unknown subject.

An observation MUST NOT silently generalize from a component to an entire node, from a node to an entire mesh, or from one connection to every connection of the same participant.

### 7.3 Source

An observation MUST identify or characterize its source.

Where exact source identity is unavailable, the observation MUST state the limitation rather than inventing attribution.

Source description SHOULD distinguish among:

- Direct local sensing.
- Direct remote reporting.
- Subject self-report.
- Peer report.
- Runtime-generated report.
- ACS-generated report.
- MEM-operation report.
- Audit-derived report.
- Human-supplied report.
- Reconstructed report.
- Unknown source.

Source class does not establish reliability.

### 7.4 Collector

The component that collected or received the observation MUST be attributable.

The collector may differ from the source.

For example:

- A hardware sensor is the source.
- A runtime service is the collector.
- An immune Observer produces the observation record.

These identities MUST NOT be silently collapsed when the distinction matters.

### 7.5 Observation content

The observation MUST identify what was perceived or reported.

Observation content may include:

- A measured value.
- A state.
- A state transition.
- A returned result.
- A failed operation.
- A missing response.
- A malformed response.
- A timing condition.
- A contradiction.
- A resource condition.
- An explicit report from another entity.
- An inability to collect information.

The observation SHOULD preserve the source representation or a reference to it where doing so is permitted and useful.

### 7.6 Scope

An observation MUST state or imply a bounded scope.

Scope may include:

- The specific subject.
- Interface.
- Operation.
- Connection.
- Data range.
- Spatial region.
- Execution context.
- Time interval.
- Sampling window.
- Collection coverage.
- Known exclusions.

A result outside the observation’s scope MUST NOT be inferred without an explicit later assessment.

### 7.7 Collection method

The collection method MUST be attributable at a level sufficient to interpret the result.

The method description may identify:

- Direct measurement.
- Passive receipt.
- Active probe.
- Query.
- Runtime instrumentation.
- Log extraction.
- Peer attestation.
- Audit reconstruction.
- Statistical summary.
- Human report.
- Derived computation.

Private implementation details need not be disclosed publicly.

The public record MUST still distinguish materially different collection modes.

### 7.8 Time information

An observation SHOULD distinguish, where applicable:

- Event time: when the observed event is believed to have occurred.
- Collection time: when the observation was acquired.
- Receipt time: when the collector received it.
- Record time: when the observation record was created.

When these times are unknown, estimated, or based on unsynchronized clocks, the limitation MUST be explicit.

A timestamp MUST NOT imply precision that the collection environment could not provide.

### 7.9 Availability state

An observation MUST be able to represent:

- Available.
- Partially available.
- Delayed.
- Stale.
- Unavailable.
- Lost.
- Unknown.

Unavailable evidence MUST NOT be represented as evidence that the underlying condition did not occur.

### 7.10 Completeness declaration

An observation SHOULD state whether it is believed to be:

- Complete for its declared scope.
- Partial.
- Truncated.
- Sampled.
- Interrupted.
- Missing expected fields.
- Unknown in completeness.

Completeness is always relative to declared scope.

### 7.11 Uncertainty

An observation MUST preserve known uncertainty.

Uncertainty may concern:

- Subject identity.
- Source identity.
- Timing.
- Measurement accuracy.
- Coverage.
- Collection success.
- Interpretation of a source format.
- Whether a report is direct or relayed.
- Whether the item is duplicated.

Uncertainty MAY be represented categorically, structurally, or quantitatively.

This document does not require or define a universal confidence score.

---

## 8. Evidence-Item Contract

An evidence item makes one or more observations, records, or artifacts available for a stated evaluative purpose.

### 8.1 Evidence identity

Each evidence item MUST have an attributable identity.

An evidence item that is modified in meaning MUST NOT silently retain an identity that implies unchanged content.

A corrected, transformed, summarized, or superseding item MUST preserve its relationship to the earlier item.

### 8.2 Purpose

An evidence item MUST identify the purpose or purposes for which it is being supplied.

Examples include:

- Health assessment.
- Runtime-failure assessment.
- Security assessment.
- Scope determination.
- Recovery verification.
- Restoration verification.
- Enforcement verification.
- Audit.
- Immune-component review.

Purpose may be broad during early collection, but later validation MUST state the purpose against which usability was evaluated.

### 8.3 Evidence class

An evidence item SHOULD identify its class.

Public evidence classes include:

- Direct observation.
- Source report.
- Subject self-report.
- Peer report.
- Derived evidence.
- Correlated evidence.
- Reconstructed evidence.
- Enforcement receipt.
- Recovery evidence.
- Restoration evidence.
- Audit evidence.
- Negative observation.
- Collection-failure evidence.

An implementation MAY define additional classes, provided they do not obscure the public distinctions.

### 8.4 Origin lineage

An evidence item MUST identify or reference its origin.

Derived items MUST reference the source observations or evidence items from which they were produced.

Where complete lineage is unavailable, the evidence item MUST state that limitation.

### 8.5 Transformation lineage

Every material transformation MUST be attributable.

Material transformations include:

- Filtering.
- Aggregation.
- Normalization.
- Unit conversion.
- Redaction.
- Compression that may lose meaning.
- Summarization.
- Feature extraction.
- Reconstruction.
- Translation.
- Format conversion.
- Selection from a larger record.
- Removal of fields.
- Merging multiple items.

The transformation record SHOULD identify:

- Input items.
- Producer.
- Transformation class.
- Time.
- Known information loss.
- Known assumptions.
- Output item.

Private algorithms may remain private.

Their existence and material effects MUST NOT be concealed.

### 8.6 Integrity declaration

An evidence item SHOULD carry or reference available integrity information.

Integrity information may establish:

- Content consistency.
- Source attribution.
- Transport integrity.
- Storage integrity.
- Version identity.
- Detection of unauthorized alteration.

Integrity verification MUST NOT be represented as proof of semantic correctness.

An integrity failure MUST NOT automatically prove subject compromise. It may indicate collection, transport, storage, implementation, or adversarial failure.

### 8.7 Validation state

An evidence item MUST be able to carry a validation state for a stated purpose.

Permitted public states include:

- Validated.
- Partially validated.
- Unvalidated.
- Invalid for stated purpose.
- Stale.
- Conflicted.
- Unavailable.
- Unknown.

A validation record SHOULD identify:

- Validator.
- Purpose.
- Time.
- Checks performed.
- Limitations.
- Result.
- Related evidence.
- Supersession state.

Validation does not erase the original item.

### 8.8 Freshness state

Evidence MUST be capable of being marked:

- Current for the stated purpose.
- Aging.
- Stale.
- Superseded.
- Unknown in freshness.

Freshness is purpose-dependent.

A hardware reading may become stale for live scheduling while remaining useful for historical audit.

No universal freshness threshold is defined here.

### 8.9 Completeness state

Evidence MUST be capable of being marked:

- Complete for declared scope.
- Partial.
- Truncated.
- Sampled.
- Missing dependencies.
- Unknown.

A partial item MAY still be useful.

Its missing scope MUST remain visible.

### 8.10 Availability state

Evidence availability MUST be distinct from evidence existence.

An item may be:

- Available.
- Partially available.
- Temporarily unavailable.
- Permanently unavailable under current policy.
- Withheld by access control.
- Lost.
- Deleted under governed MEM policy.
- Unknown.

An immune component MUST NOT bypass ACS or MEM controls merely because unavailable evidence would be useful.

### 8.11 Sensitivity and handling reference

Evidence SHOULD identify or reference applicable handling restrictions.

Handling may depend on:

- Security sensitivity.
- Cognitive-content separation.
- Personal or environmental privacy.
- Node ownership.
- Operational secrecy.
- Key material.
- Protected topology.
- Incident sensitivity.

This document does not define the access-control or custody mechanism.

ACS governs applicable connection access and transport authority.

MEM governs persistent custody, retention, reconstruction, and deletion.

---

## 9. Evidence Source Classes

### 9.1 Direct local observation

A direct local observation is collected from the same execution or hardware environment as the subject.

It may provide strong locality but is not automatically correct.

It may be affected by:

- Local compromise.
- Shared hardware failure.
- Shared clock failure.
- Sensor failure.
- Runtime failure.
- Collector misconfiguration.
- Common-mode faults.

### 9.2 Direct remote observation

A direct remote observation is collected by another participant observing the subject across an interface.

It may provide external perspective but may be limited by:

- Network conditions.
- ACS restrictions.
- Partial visibility.
- Relay behavior.
- Clock differences.
- Observer health.

### 9.3 Subject self-report

A subject self-report is evidence about a subject produced by that same subject or under its direct control.

Self-report MAY be useful.

It MUST NOT be presented as independent confirmation.

Self-report alone MUST NOT establish:

- Trustworthiness.
- Successful containment.
- Complete recovery.
- Safe restoration.
- Absence of compromise.

### 9.4 Peer report

A peer report is evidence supplied by another participant.

Peer identity, access, reputation, or trust status MAY inform later assessment under applicable policy.

None of these automatically establishes truth.

Multiple peer reports MUST NOT be treated as independent when they derive from the same upstream source.

### 9.5 Derived evidence

Derived evidence is produced by transforming one or more prior items.

Derived evidence MUST preserve:

- Input lineage.
- Transformation attribution.
- Known information loss.
- Dependence on source items.
- Applicable uncertainty.

A derivative does not become independent merely because it has a new representation.

### 9.6 Reconstructed evidence

Reconstructed evidence is recreated from surviving records, replicas, traces, summaries, or other governed recovery inputs.

Reconstruction MUST remain explicitly identified.

Reconstructed evidence MUST NOT be presented as the original item.

MEM remains authoritative for governed reconstruction of persisted records.

### 9.7 Human-supplied evidence

Human-supplied evidence MAY be accepted where policy permits.

It MUST identify:

- Reporter attribution where available.
- Whether the report is firsthand or relayed.
- Collection time.
- Known uncertainty.
- Any transformation into machine-readable form.

Human authority and evidentiary reliability are separate questions.

### 9.8 Audit evidence

Audit evidence concerns compliance, lineage, authority use, role behavior, or record completeness.

Audit evidence is subject to the same provenance and uncertainty requirements as other evidence.

An audit record does not become unquestionable merely because it was produced by an Auditor.

---

## 10. Evidence Requests and Collection Attempts

### 10.1 Evidence request

An evidence request asks an authorized source, collector, or adjacent system to supply specified information.

A request SHOULD identify:

- Requester.
- Purpose.
- Requested subject.
- Requested evidence class.
- Scope.
- Time range.
- Required freshness.
- Required completeness where applicable.
- Applicable access authority.
- Response destination.
- Expiration or cancellation condition.

### 10.2 Request authority

An evidence request MUST NOT imply unlimited collection authority.

The requester may request only what applicable ACS, MEM, runtime, resource, privacy, and governance policy permits.

Refusal or inability to provide evidence MUST NOT automatically be interpreted as malicious behavior.

### 10.3 Collection result

A collection attempt MUST be able to report:

- Completed.
- Partially completed.
- Rejected.
- Unauthorized.
- Unavailable.
- Timed out.
- Interrupted.
- Malformed.
- Failed.
- Unknown.

The collection result is itself potentially useful evidence.

It is not automatically evidence against the requested subject.

### 10.4 Negative observation

A negative observation records that a defined collection method did not observe a specified condition within a bounded scope.

It MUST identify:

- What was sought.
- Where it was sought.
- How it was sought.
- The observation window.
- Known coverage.
- Known blind spots.
- Collector availability.
- Whether the expected reporting mechanism was functioning.

A negative observation MUST NOT be generalized beyond its declared scope.

---

## 11. Time, Freshness, and Ordering

### 11.1 Multiple time domains

A distributed mesh may lack perfectly synchronized time.

Evidence contracts MUST permit:

- Local monotonic time.
- Wall-clock time.
- Sequence counters.
- Causal ordering.
- Receipt ordering.
- Estimated time ranges.
- Unknown ordering.

An implementation MUST NOT fabricate precise global order when only partial order is known.

### 11.2 Event time versus receipt time

A recently received item may describe an old event.

An old item may arrive after newer evidence.

Receipt order MUST NOT automatically be treated as event order.

### 11.3 Staleness

Staleness depends on intended use.

Evidence MUST NOT be marked permanently useless merely because it is stale for one operational purpose.

Stale evidence may remain useful for:

- Historical correlation.
- Recovery analysis.
- Audit.
- Reconstruction.
- Pattern review.
- Immune-system evaluation.

### 11.4 Supersession

A newer item may supersede an earlier item for a stated purpose.

Supersession MUST NOT silently delete or invalidate history.

The relationship between the items MUST remain attributable where retention policy permits.

---

## 12. Completeness and Coverage

### 12.1 No implicit global coverage

A collector MUST NOT claim mesh-wide completeness unless its evidence actually supports that scope.

Local completion does not imply global completion.

### 12.2 Partial coverage

Partial evidence MUST identify known boundaries such as:

- Missing nodes.
- Missing time ranges.
- Missing interfaces.
- Unavailable sensors.
- Withheld records.
- Failed collectors.
- Unsupported data types.
- Partitioned network regions.

### 12.3 Sampling

Sampled evidence MUST be identifiable as sampled.

Sampling MUST NOT be represented as exhaustive collection.

Public architecture need not disclose private sampling strategies.

Material limitations caused by sampling MUST remain visible.

### 12.4 Coverage conflict

Two evidence items may make different completeness claims.

The conflict MUST remain explicit until evaluated.

The broader claim MUST NOT automatically override the narrower one.

---

## 13. Dependence, Duplication, and Independence

### 13.1 Source dependence

Evidence items are dependent when they rely materially on the same:

- Original observation.
- Sensor.
- Collector.
- Upstream report.
- Clock.
- Transformation.
- Reconstruction source.
- Runtime service.
- Communication path.
- Compromised component.
- Failure domain.

Known dependence MUST be preserved.

### 13.2 Duplicate evidence

Repeated transmission, storage replication, reformatting, or restatement does not create new independent evidence.

Duplicate items MAY be retained for availability or audit.

They MUST NOT be counted as independent confirmation solely because multiple copies exist.

### 13.3 Replicated observers

Multiple Observers may still share a common failure domain.

Observer count is not a substitute for independence analysis.

### 13.4 Independence claims

An evidence item or evidence set MAY claim independence only within a stated dimension.

Examples include:

- Independent collectors.
- Independent hardware sources.
- Independent network paths.
- Independent administrative control.
- Independent implementations.

Independence in one dimension does not imply independence in all dimensions.

### 13.5 Unknown dependence

When dependence cannot be established, it MUST remain unknown.

Unknown dependence MUST NOT be treated as independence.

---

## 14. Contradiction and Conflict

### 14.1 Contradictory evidence is preserved

Contradictory evidence MUST NOT be silently discarded, overwritten, or merged into false agreement.

Each conflicting item MUST retain its own:

- Identity.
- Source.
- Scope.
- Time.
- Validation state.
- Lineage.
- Limitations.

### 14.2 Conflict state

An evidence item or evidence set MAY be marked conflicted when credible items support materially incompatible claims.

Conflict does not automatically imply that one source is malicious.

Possible causes include:

- Different observation times.
- Different scopes.
- Partial visibility.
- Clock error.
- Runtime transition.
- Measurement error.
- Stale data.
- Source failure.
- Transformation error.
- Misconfiguration.
- Adversarial behavior.

### 14.3 Conflict resolution

Conflict may be reduced through:

- Additional observation.
- Source validation.
- Scope clarification.
- Timing reconciliation.
- Transformation review.
- Independent collection.
- Audit.
- Assessment.

Resolution MUST preserve the prior conflict history where applicable retention policy permits.

---

## 15. Transformation and Derivation

### 15.1 Original and derivative distinction

A derivative MUST NOT be represented as an original observation.

The original item remains distinct even when the derivative is more convenient for assessment.

### 15.2 Lossless transformation

A transformation may be described as lossless only when relevant meaning and required lineage are preserved for the stated purpose.

Losslessness is purpose-dependent.

### 15.3 Lossy transformation

Lossy transformations are permitted.

They MUST identify known or intended information loss.

Examples include:

- Summaries.
- Statistical aggregates.
- Feature vectors.
- Redacted records.
- Downsampled telemetry.
- Truncated logs.
- Compressed representations that discard detail.

### 15.4 Redaction

Redacted evidence MUST remain identifiable as redacted.

Redaction MUST NOT be used to imply that omitted information never existed.

Where permitted, the evidence SHOULD reference the authority or policy under which redaction occurred.

### 15.5 Merging

When evidence items are merged:

- Source identities MUST remain recoverable or referenced.
- Contradictions MUST remain visible.
- Duplicates MUST remain distinguishable.
- Dependence MUST not be hidden.
- Transformation attribution MUST be preserved.

### 15.6 Private transformation logic

Transformation algorithms may remain private.

A public evidence contract still requires disclosure of:

- That transformation occurred.
- What class of transformation occurred.
- Which inputs were used.
- Whether material information was lost.
- Which component produced the result.

---

## 16. Evidence Sets

### 16.1 Purpose

An evidence set groups evidence for a stated task.

An evidence set is not itself a verdict.

### 16.2 Required properties

An evidence set MUST identify or reference:

- Set identity.
- Producer.
- Purpose.
- Included items.
- Excluded items when material.
- Time of assembly.
- Scope.
- Known incompleteness.
- Known conflicts.
- Known duplication.
- Known dependence.
- Validation coverage.
- Supersession state.

### 16.3 Dynamic membership

Evidence-set membership MAY change as evidence arrives, becomes unavailable, is invalidated for purpose, or is superseded.

Changes MUST be attributable.

A later evidence set MUST NOT silently present itself as identical to an earlier set when membership changed.

### 16.4 Exclusion

Evidence may be excluded for reasons including:

- Irrelevance.
- Invalidity for purpose.
- Duplicate content.
- Access restriction.
- Excessive staleness.
- Failed integrity.
- Unsupported format.
- Known fabrication.
- Policy restriction.

Exclusion does not require deletion.

Material exclusions SHOULD be attributable to the later assessment process.

### 16.5 Competing evidence sets

Multiple roles MAY assemble different evidence sets for the same subject.

Differences in scope or membership MUST remain explicit.

No evidence set becomes canonical merely because it was assembled first or by a highly privileged component.

---

## 17. Evidence and Protected Cognitive Content

### 17.1 Separation principle

Immune evidence and protected cognitive content are distinct architectural concerns.

The immune system SHOULD collect the minimum evidence necessary for the declared immune purpose.

It MUST NOT treat general access to cognitive content as an inherent consequence of immune responsibility.

### 17.2 Derived indicators

Where feasible, immune processes SHOULD prefer bounded indicators, operation results, integrity records, or governed summaries over unrestricted cognitive-content exposure.

This preference does not create permission to fabricate summaries or conceal important limitations.

### 17.3 Access authority

Access to cognitive content requires authority under the architecture governing that content.

An immune role does not obtain such authority merely because the requested content might improve assessment.

### 17.4 Evidence identity

Evidence derived from cognitive content MUST remain identified as derived.

The transformation and access basis MUST be attributable at the level permitted by policy.

### 17.5 No semantic substitution

A compact indicator derived from cognitive content MUST NOT be represented as the full underlying content.

Likewise, absence of an indicator MUST NOT automatically prove absence of the underlying condition.

---

## 18. Cross-Architecture Boundaries

### 18.1 ACS boundary

ACS remains authoritative for:

- Connection identity.
- Admission.
- Trust.
- Capability.
- Delegation.
- Revocation.
- Transport authorization.
- Connection lifecycle.
- ACS enforcement.

Immune evidence may be exchanged through ACS-governed connections.

This document does not grant a collector permission to bypass ACS restrictions.

Transport success does not establish evidence truth.

Transport failure does not establish subject compromise.

### 18.2 MEM boundary

MEM remains authoritative for:

- Persistent identity and versioning of stored records.
- Persistence.
- Custody.
- Retention.
- Availability.
- Recovery.
- Reconstruction.
- Deletion.
- Memory-operation semantics.

This document defines the semantic information an evidence record must preserve.

It does not redefine how MEM stores or governs the record.

Marking evidence invalid, stale, conflicted, or excluded does not authorize deletion.

### 18.3 Runtime boundary

Runtime systems remain authoritative for their own instrumentation, execution, and lifecycle mechanisms.

An immune Observer may consume runtime evidence.

It MUST NOT silently redefine runtime state.

### 18.4 Resource-management boundary

Resource owners remain authoritative for device access, allocation, placement, and resource control.

Resource telemetry may become immune evidence.

Evidence collection does not grant control over the resource.

### 18.5 Adaptive-state boundary

Evidence may describe adaptive-state behavior or operation results.

Immune roles MUST NOT directly rewrite adaptive-state descriptors or cognitive state merely to make evidence appear favorable.

---

## 19. Evidence Failure and Adversarial Conditions

### 19.1 Collector failure

A failed collector creates uncertainty about coverage.

It does not prove that the observed subject is healthy or compromised.

### 19.2 Source failure

A failed source may produce:

- No evidence.
- Stale evidence.
- Malformed evidence.
- Contradictory evidence.
- Plausible but incorrect evidence.

Source failure MUST remain distinguishable from subject failure where possible.

### 19.3 Fabricated evidence

Suspected fabrication is an assessment requiring evidence.

An item MUST NOT be silently removed merely because fabrication is suspected.

The item may be:

- Isolated.
- Marked suspect.
- Excluded from a stated purpose.
- Retained for audit.
- Submitted for further validation.

Any containment action still requires proper authority.

### 19.4 Evidence poisoning

An adversary or failed component may attempt to:

- Flood collectors.
- Create duplicates.
- Forge independence.
- Suppress contradictory evidence.
- Delay evidence.
- Manipulate ordering.
- Exploit stale evidence.
- Cause harmful overcollection.
- Cause false restoration.
- Cause indefinite containment.

Public architecture requires that these possibilities be representable.

Private detection methods remain implementation-private.

### 19.5 Evidence unavailability

Evidence may be unavailable because of:

- Network partition.
- ACS restriction.
- MEM unavailability.
- Retention policy.
- Governed deletion.
- Hardware failure.
- Encryption failure.
- Missing capability.
- Collector failure.
- Policy withholding.
- Unknown cause.

Cause MUST remain unknown unless established.

### 19.6 Immune evidence failure

An immune component’s failed observation, validation, transformation, or evidence exchange is evidence about the immune process.

It is not automatically evidence against the evaluated subject.

---

## 20. Privacy, Minimization, and Bounded Collection

### 20.1 Minimum necessary collection

Evidence collection SHOULD be bounded to what is reasonably necessary for the declared purpose.

A collector SHOULD avoid collecting unrelated content merely because access is technically possible.

### 20.2 Scope expansion

Expanding collection scope SHOULD require:

- A stated reason.
- Appropriate authority.
- Updated purpose.
- Updated handling conditions.
- Updated lineage.

Emergency policy may permit rapid expansion.

The authority and resulting scope MUST remain attributable.

### 20.3 Continuous collection

Continuous collection MAY be used where policy permits.

Continuous availability does not eliminate the requirements for:

- Purpose.
- Scope.
- Attribution.
- Handling restrictions.
- Access authority.
- MEM governance.

### 20.4 Evidence retention

This document does not define retention duration.

Evidence MUST NOT be retained indefinitely merely because it belongs to the immune architecture.

Retention remains governed by MEM and applicable policy.

### 20.5 Deletion

Deletion of evidence requires MEM-governed authority and semantics.

Invalidity, conflict, age, containment, recovery, or restoration does not independently authorize deletion.

---

## 21. Public Evidence States

The following states are public semantic states and may be applied where relevant:

**Observed**  
The condition or report was recorded.

**Received**  
The item was received from another source.

**Unvalidated**  
No sufficient validation for the stated purpose has completed.

**Partially validated**  
Some required properties were validated; material limitations remain.

**Validated**  
The item was found usable for the stated purpose within declared limits.

**Invalid for purpose**  
The item is not usable for the stated purpose.

**Current**  
The item is sufficiently fresh for the stated purpose.

**Stale**  
The item is too old or temporally uncertain for the stated purpose.

**Superseded**  
A later item replaces its operational use for a stated purpose.

**Partial**  
The item covers only part of its declared or expected scope.

**Truncated**  
The item was cut short or lost part of its representation.

**Conflicted**  
The item materially conflicts with other relevant evidence.

**Duplicate**  
The item repeats the same underlying evidence.

**Dependent**  
The item materially relies on another item or common source.

**Reconstructed**  
The item was recreated from surviving records or derivatives.

**Unavailable**  
The item cannot currently be accessed.

**Lost**  
The item is believed to have existed but is no longer recoverable through available mechanisms.

**Deleted under governance**  
The item was deleted through applicable MEM authority and policy.

**Unknown**  
The relevant state has not been established.

These states do not constitute a threat ranking or response threshold.

---

## 22. Conforming Example: Runtime Degradation

A runtime Observer records that several tasks assigned to one accelerator returned invalid results.

Each observation identifies:

- The accelerator.
- The task.
- The runtime source.
- Event and receipt times.
- Collection method.
- The returned result.
- Known missing telemetry.

A second Observer records thermal information from the same device.

An Evidence Validator confirms the provenance of the runtime results but marks the thermal evidence stale.

A Correlator identifies that all invalid results share the same device and driver instance.

The evidence set preserves that the runtime and thermal observations originate from different collectors but share the same local node and clock domain.

The Assessor concludes that device or driver degradation is plausible.

The evidence does not prove compromise.

This handling is conforming because:

- Observations remain separate from interpretation.
- Source and subject are bounded.
- Staleness remains explicit.
- Dependence remains visible.
- Degradation is not silently converted into compromise.
- Evidence does not directly authorize restriction.

---

## 23. Conforming Example: Missing Peer Responses

A node expects periodic responses from a peer.

The collector records that no response arrived during a defined interval.

The observation identifies:

- The expected peer.
- The connection.
- The observation window.
- The expected response type.
- Local collector health.
- Known network partition.
- ACS state.
- Clock uncertainty.

The resulting evidence is classified as a negative observation.

It does not claim that:

- The peer is offline.
- The peer is malicious.
- The connection was revoked.
- The peer intentionally withheld a response.

Later evidence may support one of those assessments.

The original observation remains bounded to non-arrival within the declared context.

---

## 24. Conforming Example: Self-Reported Recovery

A contained component reports that its internal diagnostics now pass.

The report is accepted as subject self-report.

The evidence item records:

- The reporting subject.
- Diagnostic class.
- Time.
- Reported result.
- Software version.
- Known lack of external verification.

The evidence may support recovery evaluation.

It is not independent confirmation.

A Recovery Verifier requests separate runtime and hardware evidence before concluding that recovery conditions are satisfied.

The self-report is neither discarded nor treated as sufficient on its own.

---

## 25. Nonconforming Examples

The following behavior is nonconforming:

- Recording an inferred compromise verdict as a raw observation.
- Treating a signed report as proof that its contents are true.
- Replacing an unknown reading with zero.
- Treating lack of telemetry as proof of health.
- Claiming global coverage from one node’s local observations.
- Treating repeated copies as independent evidence.
- Omitting that several reports share one upstream source.
- Deleting contradictory evidence to create agreement.
- Presenting reconstructed evidence as the original.
- Hiding lossy transformation.
- Using a summary without preserving source lineage.
- Treating a stale item as current without reevaluation.
- Treating a peer’s refusal to disclose protected content as proof of compromise.
- Collecting unrestricted cognitive content merely because the collector performs an immune role.
- Bypassing ACS to obtain evidence.
- Bypassing MEM custody or deletion policy.
- Rewriting adaptive state to suppress unfavorable evidence.
- Treating a failed collection request as proof against the subject.
- Treating a collection timeout as an enforcement authorization.
- Allowing an evidence producer to claim independent confirmation from its own derived copies.
- Allowing restoration to erase incident evidence.
- Publishing private detection thresholds in the public contract.

---

## 26. Conformance Requirements

An implementation conforms to this document only when:

1. Every observation is attributable.
2. Every evidence item is attributable.
3. Subject and source remain distinguishable.
4. Collector and source remain distinguishable where material.
5. Observation remains distinct from assessment.
6. Evidence purpose is stated or referenceable.
7. Scope is bounded.
8. Known uncertainty remains explicit.
9. Unknown values remain unknown.
10. Time limitations remain explicit.
11. Staleness is purpose-dependent.
12. Completeness is relative to declared scope.
13. Missing evidence is not treated as proof of health.
14. Negative observations identify their collection boundary.
15. Direct, reported, self-produced, derived, and reconstructed evidence remain distinguishable.
16. Transformations preserve lineage.
17. Material information loss is disclosed.
18. Integrity verification is not represented as semantic truth.
19. Validation is tied to a stated purpose.
20. Contradictory evidence remains visible.
21. Duplicate evidence is not treated as independent.
22. Known dependencies are preserved.
23. Unknown dependence is not represented as independence.
24. Evidence sets preserve membership and scope.
25. Excluded evidence is not silently deleted.
26. Evidence does not create authority.
27. ACS restrictions are not bypassed for collection or exchange.
28. MEM authority over persistence, custody, retention, reconstruction, and deletion is preserved.
29. Cognitive content is not inherently exposed to immune roles.
30. Collection is bounded by purpose and authority.
31. Immune components are valid subjects of observation and evidence.
32. Failed immune evidence handling is not automatically treated as evidence against the evaluated subject.
33. Public contracts do not disclose private thresholds or detection heuristics.
34. Recovery and restoration do not erase evidence lineage.

---

## 27. Required Failure Semantics

Failure of an observation or evidence process MUST be representable without inventing a conclusion about the subject.

Specifically:

- Failed collection does not prove subject failure.
- Failed collection does not prove subject health.
- Failed validation does not prove the evidence false.
- Invalid evidence does not prove the opposite claim.
- Missing provenance does not prove malicious origin.
- Failed integrity does not identify the cause by itself.
- Contradiction does not prove fabrication.
- Staleness does not erase historical value.
- Unavailability does not imply deletion.
- Reconstruction failure does not prove the original never existed.
- Duplicate detection failure does not make copies independent.
- Evidence-set incompleteness does not authorize hidden assumptions.
- Observer compromise does not automatically compromise every observed subject.
- Subject compromise does not automatically invalidate every prior observation.
- Evidence poisoning against the immune system does not automatically establish guilt of the apparent source.

The failure state itself may become new immune evidence.

It remains subject to the same contracts.

---

## 28. Architectural Commitments

This document establishes the following commitments:

1. Observations are bounded records, not verdicts.
2. Evidence is purpose-specific.
3. Authenticity does not imply truth.
4. Absence does not automatically prove nonexistence.
5. Unknown values remain unknown.
6. Subject, source, collector, and producer remain attributable.
7. Direct, reported, self-produced, derived, and reconstructed evidence remain distinct.
8. Every material transformation preserves lineage.
9. Lossy transformation remains explicit.
10. Validation is purpose-bounded.
11. Freshness is purpose-dependent.
12. Completeness is relative to scope.
13. Contradiction is preserved.
14. Duplication does not create independence.
15. Dependence is not concealed.
16. Evidence sets remain attributable views, not canonical truth.
17. Evidence does not create authority.
18. ACS retains authority over connection access and transport.
19. MEM retains authority over persistent evidence lifecycle.
20. Immune collection does not grant unrestricted access to cognitive content.
21. Immune evidence handling is itself observable and auditable.
22. Recovery and restoration preserve evidence history.
23. Public contracts define semantics without exposing private detection methods.

---

## 29. Future Work

Later immune architecture documents may define:

- Assessment and recommendation contracts.
- Authorization-request contracts.
- Containment lifecycle and action envelopes.
- Recovery evidence requirements.
- Restoration evidence requirements.
- Audit-record contracts.
- Cross-node evidence synchronization.
- Evidence-discovery mechanisms.
- Evidence-set negotiation.
- Partition-aware evidence handling.
- Adversarial evidence testing.
- Immune-system self-observation.
- Public conformance scenarios.

Those documents must preserve the observation, evidence, authority, ACS, and MEM boundaries established here.
