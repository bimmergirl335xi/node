# MEM-0008: Recovery, Repair, and Reconstruction

| Field | Value |
|---|---|
| Specification | MEM-0008 |
| Title | Recovery, Repair, and Reconstruction |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Accepted as initial working draft |
| Depends on | MEM-0000 through MEM-0007 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in recovery boundaries, validation, and stale-state prevention; distributed recovery boundaries and degraded-resumption profiles remain under review |

> **Recovery must reconstruct validated memory state without allowing stale, corrupt, deleted, or ambiguous participants to rewrite continuity.**

## Architectural-intent notice

This specification defines the public architectural semantics governing how Node detects damaged memory, identifies trustworthy surviving evidence, repairs lost protection, reconstructs missing representations, restores recoverable state, and safely reintegrates returning custodians.

It defines:

- damage detection;
- corruption evidence;
- missing-material evidence;
- recovery incidents;
- repair;
- reconstruction;
- restoration;
- recovery boundaries;
- checkpoints;
- journals;
- replay;
- source selection;
- quarantine;
- recovered-state validation;
- rejoining custodians;
- stale-state suppression;
- incomplete recovery;
- degraded resumption;
- recovery finality;
- recovery conflicts;
- recovery evidence;
- recovery resource limits.

It does not define:

- exact reconstruction algorithms;
- implementation-specific cognitive-state reconstruction;
- exact checkpoint frequency;
- exact journal implementation;
- exact coding algorithms;
- exact repair scheduling;
- production operator runbooks;
- physical-node quarantine authority;
- forensic incident-response procedures;
- proprietary indexing restoration;
- cognitive arbitration policy;
- automatic replay of physical actions;
- one universal disaster-recovery topology.

Implementations may use:

- complete replicas;
- reconstruction shards;
- checkpoints;
- write-ahead journals;
- append-only event histories;
- snapshots;
- content-addressed material;
- deterministic regeneration;
- archival copies;
- externally retained evidence;
- combinations of these mechanisms.

They must preserve the semantic boundaries and validation requirements established here.

## 1. Purpose

Memory continuity requires more than retaining physical bytes.

Node may experience:

- damaged storage;
- corrupted representations;
- missing shards;
- stale replicas;
- incomplete commits;
- lost catalogs;
- unavailable keys;
- unsupported schemas;
- interrupted migration;
- partitioned mutation;
- old backups returning after deletion;
- custodians rejoining with outdated state;
- conflicting journals;
- partially written checkpoints;
- repair operations interrupted by failure;
- physical material whose identity cannot be established.

A recovery system that simply selects the first available copy may:

- restore stale state over newer committed state;
- resurrect deleted memory;
- reproduce corruption across additional custodians;
- replay an operation twice;
- reissue physical commands;
- discard valid conflict evidence;
- claim complete recovery from incomplete sources;
- confuse reconstruction with semantic equivalence;
- restore content while losing its provenance or identity;
- declare durability without restoring required dependencies.

MEM-0008 establishes the conditions under which recovered material may become trusted governed memory again.

The central rule is:

> **Recovery does not create authority merely because normal operation has failed.**

## 2. Scope

This specification governs recovery behavior for:

- logical memory identities;
- memory versions;
- representations;
- physical copies;
- reconstruction shards;
- manifests;
- catalogs;
- indexes required for recovery;
- provenance;
- lineage;
- schemas;
- lifecycle state;
- tombstones;
- retention state;
- commit evidence;
- operation history;
- checkpoints;
- journals;
- key dependencies;
- custody assignments;
- placement evidence.

It applies to recovery from:

- process failure;
- custodian failure;
- device failure;
- storage corruption;
- incomplete writes;
- interrupted operations;
- network partition;
- catalog loss;
- index loss;
- stale restoration;
- failed migration;
- partial deletion propagation;
- lost protection;
- inconsistent replicas;
- incomplete reconstruction;
- software-version incompatibility.

It governs operations including:

- damage assessment;
- quarantine;
- source validation;
- checkpoint selection;
- journal replay;
- representation reconstruction;
- replica repair;
- metadata repair;
- catalog rebuilding;
- lifecycle reconciliation;
- custodian reintegration;
- degraded resumption;
- recovery completion.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements.

The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification.

The word **may** describes permitted behavior.

An implementation does not conform merely because it can restore files or restart processes.

It must preserve the logical identity, authority, lifecycle, provenance, version, and uncertainty semantics required by the MEM specification series.

## 4. Foundational distinctions

### 4.1 Recovery is not restart

Restart restores execution capability.

Recovery restores or reconciles governed memory state.

A process may restart successfully while memory remains:

- unavailable;
- stale;
- conflicting;
- corrupted;
- under-protected;
- unknown.

### 4.2 Repair is not reconstruction

**Repair** restores required protection or custody using existing valid memory material.

**Reconstruction** creates missing material from surviving sources, shards, journals, deterministic transformations, or other governed evidence.

A repair may use reconstruction.

The terms remain distinct.

### 4.3 Reconstruction is not restoration

Reconstruction creates material.

Restoration changes whether recovered material is admitted back into governed memory service.

A reconstructed copy must still be validated and reconciled.

### 4.4 Physical recovery is not semantic recovery

Recovering bytes does not prove that Node recovered:

- logical identity;
- correct version;
- provenance;
- lifecycle state;
- commit status;
- schema;
- authority;
- deletion history.

### 4.5 Availability restoration is not durability restoration

One readable recovered copy may restore availability while memory remains under-protected.

### 4.6 Recovery source is not authority

A surviving copy is evidence.

It does not become authoritative merely because other copies failed.

### 4.7 Newest timestamp is not newest valid state

Clock values alone must not determine recovery order or authority.

### 4.8 Majority is not automatically correct

Several copies may share:

- one corrupt origin;
- one stale checkpoint;
- one unauthorized mutation;
- one failure domain;
- one software defect.

### 4.9 Quarantine is not deletion

Quarantined material remains isolated pending validation or investigation.

It must not be treated as active memory or automatically erased.

### 4.10 Incomplete recovery is not complete recovery

Restoring some content while losing required metadata, provenance, schemas, keys, or lifecycle state must be reported as incomplete.

### 4.11 Degraded resumption is not full recovery

Node may resume bounded operation before every recovery target is satisfied.

The remaining limitations must stay explicit.

### 4.12 Replay is not re-execution of intent

Journal replay reconstructs governed memory effects.

It must not blindly repeat external side effects or physical actions.

### 4.13 Rejoining is not automatic reintegration

A custodian returning after failure, partition, rollback, or restoration must reconcile before serving authoritative state.

### 4.14 Recovery does not override deletion

Deleted memory must not be restored as active merely because old physical material survives.

### 4.15 Recovery does not erase conflict

Recovery must preserve valid conflicting histories until governed authority resolves them.

### 4.16 Unknown recovery state is not failure or success

When evidence is insufficient, the recovery state remains unknown or indeterminate.

## 5. Recovery model

### 5.1 Recovery incident

A recovery incident is an identified condition requiring assessment, repair, reconstruction, reconciliation, or restoration.

A recovery incident may affect:

- one physical copy;
- one representation;
- one version;
- one logical memory;
- one catalog scope;
- one failure domain;
- one recovery boundary;
- a bounded set of memories.

### 5.2 Recovery scope

Every recovery operation must define finite scope.

Scope may include:

- damaged subjects;
- required sources;
- required dependencies;
- recovery boundary;
- target protection;
- target availability;
- validation requirements;
- permitted degraded outcomes;
- resource limits.

### 5.3 Recovery objective

A recovery objective defines what the operation is expected to restore.

Objectives may include:

- readable access;
- authoritative current version;
- exact historical version;
- required provenance;
- lifecycle consistency;
- catalog discoverability;
- minimum durability;
- failure-domain diversity;
- reconstruction capability;
- operation-history continuity.

### 5.4 Recovery boundary

A recovery boundary identifies the governed point or state through which memory must be reconstructed consistently.

A boundary may be defined through:

- committed version frontier;
- checkpoint identity;
- journal position;
- snapshot identity;
- lifecycle generation;
- commit generation;
- another governed ordering mechanism.

### 5.5 Recovery candidate

A recovery candidate is material or evidence that may contribute to recovery but has not yet been accepted as valid recovery input.

### 5.6 Recovery source

A recovery source is a candidate that has passed the validation required for its assigned recovery use.

A source may be valid for one purpose but not another.

For example, a stale copy may be valid for historical reconstruction but invalid for restoring current state.

### 5.7 Recovery plan

A recovery plan identifies:

- incident scope;
- objective;
- recovery boundary;
- selected sources;
- source confidence;
- missing dependencies;
- expected reconstruction;
- validation steps;
- target custody;
- resource bounds;
- completion criteria;
- degraded alternatives.

### 5.8 Recovery result

A recovery result reports:

- recovered subjects;
- unrecovered subjects;
- reconstructed material;
- restored material;
- achieved boundary;
- validation completed;
- protection restored;
- unresolved conflict;
- remaining uncertainty;
- remaining under-protection;
- finality.

## 6. Conceptual recovery records

These records define semantics, not mandatory storage or wire formats.

### 6.1 Recovery-incident record

```text
RecoveryIncident
    recovery_incident_identity
    detected_subjects
    incident_class
    detection_evidence
    observation_boundary
    affected_operations
    affected_protection_targets
    initial_availability_state
    initial_consistency_state
    initial_lifecycle_state
    incident_status
```

### 6.2 Recovery-plan record

```text
RecoveryPlan
    recovery_operation_identity
    incident_identity
    recovery_scope
    recovery_objective
    target_recovery_boundary
    candidate_sources
    accepted_sources
    rejected_sources
    required_dependencies
    validation_requirements
    target_custody
    resource_limits
    permissible_degradation
    completion_criteria
```

### 6.3 Recovery-source record

```text
RecoverySource
    source_identity
    source_type
    logical_memory_identity
    version_identity
    representation_or_shard_identity
    observed_lifecycle_state
    integrity_state
    provenance_state
    authority_state
    freshness_state
    compatibility_state
    source_validation_evidence
    permitted_recovery_uses
```

### 6.4 Reconstruction record

```text
ReconstructionRecord
    reconstruction_operation_identity
    target_subject_identity
    source_identities
    reconstruction_method_profile
    target_representation_identity
    expected_equivalence
    achieved_equivalence
    validation_state
    remaining_uncertainty
    resulting_physical_copy_identity
```

### 6.5 Recovery-result record

```text
RecoveryResult
    recovery_operation_identity
    recovery_scope
    achieved_recovery_boundary
    recovered_subjects
    unrecovered_subjects
    reconstructed_subjects
    restored_subjects
    quarantined_subjects
    validation_state
    consistency_state
    lifecycle_state
    protection_state
    degraded_resumption_state
    unresolved_conflicts
    finality
    evidence_references
```

Equivalent information may be distributed across journals, catalogs, recovery coordinators, custodians, and immutable operation histories.

## 7. Damage and failure classes

### 7.1 Missing material

Expected physical material cannot be located or accessed.

Missing material may be:

- unavailable;
- lost;
- moved;
- deleted;
- orphaned;
- unknown.

### 7.2 Integrity corruption

Material fails required integrity validation.

### 7.3 Semantic corruption

Material is structurally readable but violates required:

- schema;
- lineage;
- provenance;
- lifecycle;
- authority;
- version semantics.

### 7.4 Stale material

Material represents an older valid state that cannot satisfy the requested recovery boundary.

### 7.5 Conflicting material

Two or more valid-looking sources provide incompatible state.

### 7.6 Incomplete material

A representation, shard set, checkpoint, journal, or metadata set is missing required components.

### 7.7 Orphaned material

Physical material exists, but its required logical identity, lineage, authority, or lifecycle state cannot currently be resolved.

### 7.8 Incompatible material

Material requires unsupported:

- schema;
- encoding;
- software generation;
- key system;
- reconstruction profile.

### 7.9 Unauthorized material

Material was created, modified, or retained without sufficient authority.

It may remain relevant as evidence but must not automatically restore governed state.

### 7.10 Lifecycle-conflicting material

Material conflicts with:

- deletion;
- tombstone;
- hold;
- restoration;
- retention;
- reclamation state.

### 7.11 Indeterminate operation state

An operation may have committed or partially completed, but available evidence cannot establish its outcome.

### 7.12 Dependency loss

Required content may survive while a necessary:

- key;
- schema;
- manifest;
- catalog;
- provenance record;
- commit record;
- tombstone

is missing or unavailable.

## 8. Detection

### 8.1 Detection sources

Recovery conditions may be detected through:

- failed integrity checks;
- custody reports;
- missing heartbeat evidence;
- failed reads;
- catalog disagreement;
- shard-count reduction;
- journal discontinuity;
- checkpoint failure;
- lifecycle conflict;
- version divergence;
- repair verification;
- health systems;
- observer or auditor evidence.

### 8.2 Detection is not diagnosis

A failed read does not by itself establish whether the cause is:

- corruption;
- authorization;
- network failure;
- missing key;
- missing schema;
- custodian loss;
- lifecycle restriction.

### 8.3 Detection evidence

A recovery incident should preserve:

- observation source;
- observation time;
- affected identity;
- expected state;
- observed state;
- validation performed;
- uncertainty;
- relevant failure-domain context.

### 8.4 Detection confidence

An incident may be:

- suspected;
- confirmed;
- partially confirmed;
- conflicting;
- unknown.

### 8.5 False-positive safety

A suspected fault must not automatically cause valid material to be overwritten or deleted.

### 8.6 Latent corruption

Stored material may remain corrupted without immediate read failure.

Policies should support periodic or event-triggered validation appropriate to memory class.

Exact schedules are implementation-defined.

## 9. Quarantine

### 9.1 Purpose

Quarantine prevents questionable material from participating in ordinary:

- retrieval;
- commitment;
- repair;
- replication;
- reconstruction;
- current-version selection.

### 9.2 Quarantine triggers

Material may be quarantined because of:

- failed integrity;
- unknown identity;
- conflicting lineage;
- stale lifecycle state;
- unauthorized origin;
- unsupported schema;
- suspected compromise;
- incomplete reconstruction;
- uncertain deletion coverage.

### 9.3 Quarantine identity

A quarantine action must identify:

- affected material;
- reason category;
- authority;
- scope;
- permitted inspection;
- release conditions;
- preservation requirements.

### 9.4 Quarantine does not grant physical control

A memory role may classify material as quarantined without gaining authority to disable, erase, seize, or rebuild the physical node containing it.

### 9.5 Quarantined evidence

Quarantined material may be retained as:

- repair input;
- conflict evidence;
- security evidence;
- provenance evidence;
- historical state.

### 9.6 Release from quarantine

Release requires governed validation or resolution.

Discovery that material is readable is not sufficient by itself.

## 10. Source selection

### 10.1 Source selection is evidence-based

Recovery sources must be selected using evidence appropriate to the recovery objective.

### 10.2 Source-selection dimensions

Relevant dimensions may include:

- logical identity;
- version identity;
- commit status;
- lineage;
- integrity;
- provenance;
- lifecycle state;
- freshness;
- authority;
- failure-domain independence;
- representation compatibility;
- reconstruction compatibility;
- observation age.

### 10.3 No first-response authority

The first reachable source must not automatically become the selected recovery source.

### 10.4 No timestamp-only authority

The source with the latest timestamp must not automatically win.

### 10.5 No copy-count-only authority

A majority of identical copies does not automatically establish semantic correctness.

### 10.6 Independent evidence

Where practical, recovery should use evidence independent of the damaged or disputed material.

Examples include:

- commit records;
- manifests;
- lineage records;
- external integrity roots;
- independently retained lifecycle records;
- verified checkpoint metadata.

### 10.7 Conflicting sources

When sources conflict, the recovery plan must:

- preserve all relevant candidates;
- identify the conflict;
- avoid silent branch selection;
- apply the required consistency and authority model.

### 10.8 Source rejection

Rejected sources should retain a reason category where useful and authorized.

## 11. Checkpoints

### 11.1 Definition

A checkpoint is an identified recoverable representation of governed memory state at a declared boundary.

### 11.2 Checkpoint identity

A checkpoint should identify:

- checkpoint identity;
- covered memory scope;
- recovery boundary;
- schema;
- included versions;
- included lifecycle state;
- included provenance or references;
- integrity evidence;
- required journals;
- required keys;
- creation operation;
- validation state.

### 11.3 Checkpoint completeness

A checkpoint must not claim complete coverage beyond its declared scope.

### 11.4 Checkpoint consistency

A checkpoint claiming consistency across several memories must identify the boundary or mechanism that establishes that consistency.

### 11.5 Partial checkpoint

An interrupted or partial checkpoint must remain explicitly incomplete.

### 11.6 Checkpoint validation

A checkpoint must be validated before trusted recovery use.

### 11.7 Checkpoint age

An older checkpoint may remain valid as a recovery base while requiring later journal replay.

### 11.8 Checkpoint lifecycle

Checkpoints are governed memory material and require:

- retention;
- custody;
- lifecycle;
- deletion;
- tombstone;
- dependency handling.

### 11.9 Checkpoint frequency

This specification does not define checkpoint intervals.

Frequency must account for:

- recovery objectives;
- journal volume;
- memory class;
- resource budget;
- tolerated loss window.

## 12. Journals and logs

### 12.1 Journal purpose

A journal records governed memory operations or transitions required to reconstruct state after a checkpoint or interruption.

### 12.2 Journal identity

Journal entries must preserve semantic operation identity.

Transport retry or duplicate delivery must not produce duplicate semantic effects during replay.

### 12.3 Journal ordering

A journal must expose enough governed ordering to determine replay sequence within its scope.

Wall-clock time alone is insufficient where authoritative order matters.

### 12.4 Journal continuity

Missing, duplicated, corrupted, or conflicting journal ranges must remain explicit.

### 12.5 Journal authority

A locally retained journal is not automatically authoritative beyond its declared scope.

### 12.6 Journal retention

Journal material must remain until dependent checkpoints and recovery requirements no longer need it.

### 12.7 Journal compaction

Compaction may replace detailed entries with equivalent checkpoint or summary evidence only when:

- replay correctness remains;
- operation deduplication remains;
- lifecycle history remains;
- conflict evidence remains as required.

### 12.8 Sensitive journal contents

Journals may expose:

- memory content;
- access patterns;
- participants;
- lifecycle events;
- internal operations.

Disclosure must remain authorized.

## 13. Replay

### 13.1 Replay purpose

Replay applies governed memory operations or state transitions after a valid recovery base.

### 13.2 Replay scope

Replay must identify:

- starting boundary;
- ending boundary;
- journal scope;
- target state;
- operation identities;
- excluded entries;
- unresolved gaps.

### 13.3 Duplicate-safe replay

An operation already represented in the recovery base must not produce a second semantic effect.

### 13.4 Authority reevaluation

Replay must not blindly renew expired or revoked authority.

The recovery contract must define which historical authority is being reconstructed and which current authority is required to restore it.

### 13.5 Lifecycle-aware replay

Replay must honor:

- deletion;
- tombstones;
- holds;
- restoration;
- retention changes;
- supersession;
- invalidation.

### 13.6 External side effects

Memory replay must not automatically repeat:

- motor commands;
- actuator commands;
- financial transactions;
- outbound communication;
- physical-device changes;
- other external side effects.

### 13.7 Side-effect record versus action

A memory record that an action occurred may be restored.

The physical action itself must not be blindly reissued.

### 13.8 Replay gaps

A replay with missing required entries must remain:

- partial;
- conflicting;
- unavailable;
- indeterminate;

according to the effect of the gap.

### 13.9 Replay completion

Replay completes only when the resulting state is validated against the target recovery boundary.

## 14. Repair

### 14.1 Purpose

Repair restores required custody, availability, integrity, or protection using valid existing or reconstructed memory material.

### 14.2 Repair target

A repair operation should identify:

- affected memory;
- failed protection target;
- source material;
- target custody;
- target failure domains;
- required validation;
- resource budget;
- operation identity.

### 14.3 Repair does not alter semantic version by default

Repair of byte-identical or semantically equivalent material should preserve logical and version identity according to MEM-0003.

### 14.4 Repair source validation

Material must be validated before being propagated as a repair source.

### 14.5 Repair target validation

A repaired copy must be validated before it counts toward restored protection.

### 14.6 Duplicate-safe repair

Retrying one repair operation must not create uncontrolled replica growth.

### 14.7 Repair priority

Policy may prioritize repairs according to:

- memory class;
- remaining reconstruction margin;
- continuity importance;
- current availability;
- failure-domain exposure;
- time under-protected.

Exact prioritization policy is outside this public specification.

### 14.8 Repair storms

Repair work must be bounded to prevent one failure from causing uncontrolled:

- bandwidth use;
- storage amplification;
- compute use;
- energy use;
- additional failures.

### 14.9 Repair finality

Repair completion must identify the protection target actually restored.

## 15. Reconstruction

### 15.1 Purpose

Reconstruction creates missing memory material from governed surviving evidence.

### 15.2 Reconstruction sources

Sources may include:

- complete replicas;
- reconstruction shards;
- checkpoints;
- journals;
- deterministic transformations;
- validated derived material;
- archived representations;
- externally retained evidence.

### 15.3 Reconstruction target

The target must identify:

- logical memory;
- version;
- representation;
- expected equivalence;
- schema;
- provenance requirements;
- lifecycle state;
- destination custody.

### 15.4 Exact reconstruction

An exact reconstruction produces material established as byte-identical or canonically identical to the target representation.

### 15.5 Semantic reconstruction

A semantic reconstruction produces a representation established as semantically equivalent but not necessarily byte-identical.

### 15.6 Approximate reconstruction

An approximate reconstruction recovers only an approximation of the original governed state.

Approximate reconstruction must not be presented as exact.

It may require:

- a new version;
- a derived logical memory;
- explicit degraded state.

### 15.7 Partial reconstruction

Partial reconstruction must identify:

- recovered portions;
- missing portions;
- unsupported dependencies;
- uncertainty;
- permitted uses.

### 15.8 Reconstruction provenance

Reconstruction must preserve or add provenance identifying:

- reconstruction operation;
- source material;
- method profile;
- validation;
- achieved equivalence;
- remaining uncertainty.

### 15.9 Reconstruction authority

Ability to reconstruct bytes does not grant authority to admit them as current memory.

## 16. Reconstruction-shard recovery

### 16.1 Threshold evidence

Shard reconstruction requires evidence that the valid shard set satisfies the applicable reconstruction threshold.

### 16.2 Shard compatibility

Shards must belong to the correct:

- reconstruction set;
- logical memory;
- version;
- representation;
- coding profile.

### 16.3 Manifest dependency

A shard set without a valid manifest or equivalent reconstruction metadata must not be assumed reconstructable.

### 16.4 Correlated shard failure

Shard count must account for failure-domain correlation.

### 16.5 Invalid shard handling

Invalid or conflicting shards must be quarantined or excluded rather than silently used.

### 16.6 Reconstruction verification

The reconstructed representation must be validated independently of merely completing the coding operation.

## 17. Metadata recovery

### 17.1 Metadata is memory-critical

Recovering content while losing required metadata is incomplete recovery.

### 17.2 Identity recovery

Logical and version identity should be restored from governed evidence rather than reminted for convenience.

### 17.3 Catalog recovery

A catalog may be rebuilt from:

- custodians;
- manifests;
- journals;
- checkpoints;
- identity records.

Rebuilding must preserve:

- existing identity;
- lifecycle;
- conflict;
- uncertainty;
- tombstones.

### 17.4 Index recovery

Indexes may be regenerated from authoritative memory.

An incomplete regenerated index must not claim complete search coverage.

### 17.5 Provenance recovery

Missing provenance must remain:

- partial;
- unknown;
- conflicting;
- unavailable;

rather than invented.

### 17.6 Schema recovery

When required schema is unavailable, raw material may be retained but not presented as semantically recovered.

### 17.7 Lifecycle recovery

Recovered lifecycle state must be reconciled against the newest valid:

- deletion;
- tombstone;
- hold;
- restoration;
- retention decision.

## 18. Validation of recovered state

### 18.1 Validation before trusted use

Recovered or reconstructed material must satisfy the validation required for its intended use before it becomes trusted memory service state.

### 18.2 Validation dimensions

Validation may include:

- physical integrity;
- representation identity;
- schema compatibility;
- version lineage;
- provenance;
- commit status;
- lifecycle state;
- authorization;
- reconstruction equivalence;
- dependency availability;
- failure-domain placement.

### 18.3 Validation levels

A recovered result may be:

- fully validated for declared use;
- partially validated;
- unverified;
- conflicting;
- invalid;
- validation unavailable.

### 18.4 Validation is use-specific

Material may be valid for:

- historical inspection;

while invalid for:

- current-state restoration;
- repair source;
- authoritative mutation.

### 18.5 Independent validation

Where failure risk warrants it, validation should rely on evidence independent of the reconstructed material or producing participant.

### 18.6 Validation failure

Failed recovered material must not silently become a repair source or active memory.

## 19. Restoration and reintegration

### 19.1 Restoration purpose

Restoration admits validated recovered state into governed memory service according to current authority and lifecycle rules.

### 19.2 Restoration preconditions

Restoration may require:

- valid identity;
- valid version or derivation classification;
- current lifecycle compatibility;
- required authorization;
- validation;
- conflict resolution;
- catalog update;
- custody establishment.

### 19.3 Restoration does not erase incident history

Recovery and restoration evidence should remain available according to policy.

### 19.4 Restored current state

A recovered version may be restored as current only when the applicable authority and consistency model establish that status.

### 19.5 Historical restoration

A stale or superseded version may be restored as historical memory without becoming current.

### 19.6 Restoring deleted memory

Deleted memory must follow the governed restoration rules of MEM-0006.

Physical survival alone does not reverse deletion.

### 19.7 Restoration finality

A restoration result must distinguish:

- material reconstructed;
- material validated;
- custody established;
- memory restored;
- currentness established;
- durability restored.

## 20. Rejoining custodians

### 20.1 Rejoining condition

A custodian is rejoining when it returns after:

- partition;
- outage;
- rollback;
- snapshot restoration;
- software recovery;
- authority interruption;
- custody suspension.

### 20.2 Rejoining evidence

A rejoining custodian should present:

- participant identity;
- role assignment;
- assignment generation;
- local material inventory;
- version evidence;
- lifecycle evidence;
- operation history;
- last known synchronization boundary;
- integrity state.

### 20.3 No immediate serving

A rejoining custodian must not immediately serve retained material as current or authoritative until reconciliation completes.

### 20.4 Assignment generation

Stale custody assignments must not regain authority merely because the participant returned.

### 20.5 Lifecycle reconciliation

Returning material must be checked against:

- deletion;
- tombstones;
- restoration;
- holds;
- current retention;
- reclamation state.

### 20.6 Version reconciliation

Returning versions must be checked against:

- newer committed versions;
- branch conflicts;
- supersession;
- invalidation;
- merge history.

### 20.7 Operation reconciliation

Unfinished local operations may have been:

- received;
- accepted;
- committed;
- partially executed;
- acknowledged incompletely.

Their state must be reconciled using stable operation identity.

### 20.8 Rejoining outcomes

A rejoining custodian may become:

- reintegrated;
- restricted;
- repair-only;
- historical-source-only;
- quarantined;
- rejected;
- indeterminate.

## 21. Stale-state suppression

### 21.1 Stale overwrite prohibited

Recovered or rejoining state must not overwrite newer committed state without governed conflict resolution.

### 21.2 Tombstone precedence

Applicable tombstones must suppress restoration of deleted material.

### 21.3 Lifecycle generation

Lifecycle generations or equivalent ordering evidence should be used to reject stale lifecycle state.

### 21.4 Operation deduplication

Replayed or rejoining operations must not duplicate previously committed semantic effects.

### 21.5 Catalog rebuilding

Catalog reconstruction must not promote stale copies simply because newer copies are temporarily unavailable.

### 21.6 Repair cancellation

Old repair jobs must not recreate copies of deleted, invalidated, or superseded material.

### 21.7 Backup reconciliation

A restored backup must be reconciled against all governed state newer than its recovery boundary.

## 22. Recovery completeness

### 22.1 Complete recovery is scoped

Recovery is complete only relative to a declared:

- memory scope;
- recovery boundary;
- objective;
- validation level;
- lifecycle state;
- protection target.

### 22.2 Content completeness

All required memory content was recovered.

### 22.3 Metadata completeness

Required identity, provenance, lineage, schema, and lifecycle metadata were recovered.

### 22.4 Operational completeness

Required operation history and deduplication evidence were recovered.

### 22.5 Protection completeness

Required custody and failure-domain protection were restored.

### 22.6 Search completeness

Recovered catalogs and indexes provide the declared coverage.

### 22.7 Partial recovery

A partial recovery result must identify which dimensions remain incomplete.

### 22.8 Unknown completeness

When membership or required source scope is unknown, recovery completeness must remain unknown.

## 23. Degraded resumption

### 23.1 Purpose

Degraded resumption allows bounded Node operation before full recovery completes.

### 23.2 Required declarations

A degraded-resumption profile must identify:

- available memory classes;
- unavailable memory classes;
- stale memory permitted;
- mutation permissions;
- consistency guarantees;
- physical-action restrictions;
- repair priorities;
- unresolved conflicts;
- exit conditions.

### 23.3 Read-only resumption

Node may resume authorized read-only access to validated committed memory while commitment or repair roles remain unavailable.

### 23.4 Historical-only resumption

Only identified historical or stale state may be available.

It must not be presented as current.

### 23.5 Metadata-only resumption

Node may know that memory exists while content remains unavailable.

### 23.6 Restricted mutation

Mutation should remain blocked when:

- current authoritative state is unknown;
- lifecycle state is conflicting;
- required operation history is unavailable;
- commitment would worsen unrecoverable divergence.

### 23.7 Physical-action restriction

Recovery uncertainty must not be converted directly into physical action.

Safety-sensitive actuation may require separate current-state validation beyond memory recovery.

### 23.8 Degraded state remains observable

Degraded operation must not silently become normal operation.

## 24. Unavailable keys, schemas, and provenance

### 24.1 Key unavailable

Encrypted memory may be physically intact but semantically unavailable.

### 24.2 Key lost

Unintentional key loss is a recovery failure or accidental loss.

It must not be relabeled as authorized deletion or sanitization.

### 24.3 Schema unavailable

Material requiring an unavailable schema may be retained and transferred but not interpreted as fully recovered.

### 24.4 Provenance unavailable

Recovered content with missing required provenance remains partially recovered or unverified.

### 24.5 Dependency reconstruction

A dependency may itself be reconstructed when governed evidence permits.

### 24.6 Dependency substitution

Substituting a new schema, key mechanism, or provenance summary must preserve or explicitly change the applicable semantic identity.

### 24.7 Unsupported dependency

Unsupported dependencies must remain explicit rather than causing silent data conversion.

## 25. Distributed recovery boundaries

### 25.1 Boundary purpose

A distributed recovery boundary establishes a coherent point across several memories, custodians, or services.

### 25.2 Boundary evidence

A boundary may depend on:

- coordinated checkpoint;
- commit generation;
- journal frontier;
- causal frontier;
- lifecycle generation;
- another governed consistency mechanism.

### 25.3 Local boundary is not global boundary

One custodian’s latest valid checkpoint does not establish the latest consistent state of all memory.

### 25.4 Mixed boundaries

Recovery from several boundaries may create valid but inconsistent state.

The resulting conflict or partial state must remain explicit.

### 25.5 Boundary selection

Boundary selection must account for:

- required consistency;
- available evidence;
- missing participants;
- lifecycle state;
- operation dependencies;
- safety constraints.

### 25.6 Boundary fallback

A recovery plan may fall back to an older coherent boundary rather than combine incompatible newer fragments.

The loss window must be reported.

## 26. Partition and concurrent recovery

### 26.1 Recovery during partition

Recovery may proceed during partition only within the authority and evidence available for the declared scope.

### 26.2 Competing recovery operations

Several participants may begin recovery independently.

Stable operation identity and recovery-plan identity must prevent uncontrolled duplicate effects.

### 26.3 Conflicting restored state

Competing recoveries may produce incompatible restored branches.

The conflict must remain explicit.

### 26.4 Repair over-placement

Concurrent repair may create excess copies.

Extra material must remain bounded, identified, and lifecycle-governed.

### 26.5 Reconciliation after partition

Recovered and rejoining state must be reconciled when communication is restored.

## 27. Lifecycle interaction

### 27.1 Deleted material

Deleted material may be retained temporarily as inaccessible physical residue or evidence.

It must not become active recovery input without governed restoration authority.

### 27.2 Tombstone recovery

Tombstones are recovery-critical metadata.

Loss of tombstones may permit stale resurrection.

### 27.3 Hold recovery

Holds must be recovered before reclamation or deletion decisions resume.

### 27.4 Archival recovery

Archived memory may require staging or reconstruction without changing lifecycle identity.

### 27.5 Reclamation evidence

Recovery must not assume that material survives after confirmed reclamation or sanitization.

### 27.6 Conflicting lifecycle evidence

Conflicting active and deleted states must cause conservative recovery behavior.

## 28. Security and compromise

### 28.1 Recovery authority is least-privilege

Recovery roles receive only the authority required for the declared incident and scope.

### 28.2 Recovery is a sensitive path

Recovery may expose:

- broad memory inventories;
- historical versions;
- deleted residue;
- keys;
- provenance;
- topology;
- operation history.

### 28.3 Compromised source

A source suspected of compromise must not be trusted merely because it passes basic integrity checks.

### 28.4 Integrity is not authenticity

A digest may prove bytes are unchanged relative to one reference.

It does not independently prove authorized origin.

### 28.5 Recovery replay attacks

Old checkpoints, journals, deletion requests, restoration requests, and custody assignments must not regain authority through replay.

### 28.6 Key custody

Recovery-key access must remain separate from unrestricted memory authority.

### 28.7 Evidence preservation

Security-relevant recovery evidence may require retention even after the affected material is repaired or deleted.

## 29. Resource management

### 29.1 Recovery is bounded

Recovery operations must use explicit limits for:

- bandwidth;
- storage;
- compute;
- energy;
- concurrent repairs;
- source fan-out;
- queue depth;
- retry;
- temporary copies.

### 29.2 Recovery priority

Policy may prioritize:

- continuity-critical memory;
- memories near irrecoverable loss;
- required metadata;
- tombstones;
- manifests;
- key dependencies;
- active operational memory.

### 29.3 Recovery pressure

Recovery work must not silently starve all normal memory operations unless a declared emergency profile requires it.

### 29.4 Temporary material

Staging and reconstruction material must be:

- identified;
- bounded;
- lifecycle-governed;
- reclaimed safely.

### 29.5 Failed recovery cleanup

Cleanup of failed recovery material must not delete the last valid source or required evidence.

## 30. Recovery states

Implementations may use different names while preserving equivalent distinctions.

### 30.1 Detected

A possible recovery incident has been observed.

### 30.2 Assessing

The scope, cause, and available evidence are being evaluated.

### 30.3 Planned

A recovery plan has been accepted.

### 30.4 Awaiting source

Required recovery material is unavailable or pending.

### 30.5 Quarantined

Questionable material has been isolated.

### 30.6 Repairing

Existing valid material is being copied or redistributed.

### 30.7 Reconstructing

Missing material is being generated from surviving evidence.

### 30.8 Validating

Recovered material is undergoing required checks.

### 30.9 Reconciling

Recovered state is being compared with current identity, version, lifecycle, and operation history.

### 30.10 Restoring

Validated state is being admitted back into governed service.

### 30.11 Degraded resumed

Bounded operation has resumed before full recovery.

### 30.12 Recovered

The declared recovery objective has been met.

### 30.13 Partially recovered

Some required recovery scope remains incomplete.

### 30.14 Failed

Evidence establishes that the recovery objective could not be met.

### 30.15 Indeterminate

The system cannot safely establish whether recovery effects occurred.

### 30.16 Conflicting

Incompatible valid-looking recovery outcomes remain.

### 30.17 Unknown

Evidence is insufficient to classify recovery state.

## 31. Role boundaries

### 31.1 Observer and health systems

May detect and report symptoms.

They do not independently authorize recovery mutation.

### 31.2 Repair and reconstruction coordinator

Creates and coordinates recovery plans within assigned authority.

### 31.3 Custodian

Provides retained material and accepts repaired or reconstructed custody.

### 31.4 Validator

Evaluates source and recovered material.

Validation does not independently commit or restore state.

### 31.5 Catalog and locator

Provide identity and location evidence and may be rebuilt from governed sources.

### 31.6 Commit authority

Determines whether reconstructed or replayed state enters authoritative history where a new commitment is required.

### 31.7 Lifecycle authority

Determines whether deleted, held, archived, or reclaimed material may participate in recovery or restoration.

### 31.8 Access authority

Controls disclosure and use of recovery material.

### 31.9 Runtime

May restart or place recovery-service implementations.

Runtime restart does not establish memory recovery correctness.

### 31.10 Immune systems

May evaluate suspected corruption or compromise.

Memory recovery roles do not gain unrestricted immune or physical-node control.

## 32. Boundary with ACS

ACS governs:

- relationships;
- connections;
- endpoint identity;
- port semantics;
- signals;
- payload references;
- routing;
- secure sessions;
- delivery;
- communication retry;
- communication backpressure.

MEM governs:

- recovery incidents;
- recovery operation identity;
- source validation;
- checkpoint and journal meaning;
- replay semantics;
- repair;
- reconstruction;
- quarantine;
- restoration;
- rejoining;
- recovery completeness;
- degraded resumption.

The following rules apply:

1. Reconnection is not recovery completion.
2. Successful transfer is not recovered-state validation.
3. ACS retry must preserve the same semantic recovery-operation identity when intent is unchanged.
4. Connection loss does not prove source loss.
5. Endpoint reachability does not establish source authority.
6. Payload references do not prove recovered material identity or validity.
7. Communication timeout does not prove recovery failure.
8. ACS backpressure may delay repair but does not redefine recovery priority or correctness.
9. MEM does not redefine ACS routing, sessions, or transport replay.
10. Several ACS exchanges may participate in one recovery operation.

## 33. Boundary with physical actuation

Memory recovery may restore records describing:

- commands;
- intentions;
- plans;
- actions;
- sensor observations;
- physical outcomes.

It must not automatically repeat the associated physical action.

A conforming design must distinguish:

- reconstructing that an action was commanded;
- reconstructing that an action completed;
- deciding whether a new action should occur.

Physical action requires current authorization, safety state, and execution policy outside memory replay.

## 34. Conformance expectations

A conforming implementation must demonstrate:

- restart versus recovery separation;
- repair versus reconstruction separation;
- reconstruction versus restoration separation;
- source validation;
- stale-source rejection;
- conflicting-source preservation;
- checkpoint identity and completeness;
- journal-gap handling;
- duplicate-safe replay;
- external-side-effect suppression;
- quarantine behavior;
- exact versus semantic versus approximate reconstruction;
- reconstruction provenance;
- metadata recovery;
- tombstone reconciliation;
- rejoining-custodian restrictions;
- lifecycle-aware recovery;
- partial-recovery reporting;
- degraded-resumption limits;
- requested-versus-achieved recovery boundary;
- availability versus durability restoration;
- unavailable key and schema handling;
- bounded repair and reconstruction;
- indeterminate recovery handling.

Detailed fault injection and failure validation belong to MEM-0010.

## 35. Prohibited interpretations

This specification shall not be interpreted to mean that:

- process restart proves memory recovery;
- readable bytes prove semantic recovery;
- a surviving copy is automatically authoritative;
- the newest timestamp automatically wins;
- replica majority automatically establishes correctness;
- reconstruction automatically establishes restoration;
- reconstruction may ignore provenance;
- a partial checkpoint is complete;
- missing journal entries may be silently skipped;
- journal replay may repeat external side effects;
- physical commands should be replayed after restart;
- rejoining custodians are automatically current;
- old backups may ignore tombstones;
- recovered catalogs may mint replacement identities for convenience;
- stale state may overwrite newer committed state;
- deleted memory may return because a copy survived;
- corrupt material must be deleted immediately;
- quarantine grants physical-node authority;
- one recovered copy proves restored durability;
- approximate reconstruction is exact memory;
- missing provenance may be invented;
- recovery authority is unrestricted emergency authority;
- public recovery contracts expose proprietary reconstruction algorithms.

## 36. Open questions

The following questions remain for later specifications and implementation profiles:

- Which recovery states are mandatory for baseline conformance?
- What constitutes a consistent distributed recovery boundary for each memory class?
- Which memory classes require coordinated checkpoints?
- Which journals require durable global ordering versus local ordering?
- How long must operation history remain replay-safe?
- Which recovery operations require independent validation?
- Which memories permit approximate reconstruction?
- When does approximate reconstruction create a new version versus a new logical memory?
- Which memory classes permit degraded resumption?
- Which mutations remain allowed during degraded resumption?
- Which physical-action restrictions are mandatory after recovery?
- How should recovery proceed when current lifecycle state cannot be established?
- How should unavailable keys affect recovery priority?
- Which schema dependencies require independent recovery custody?
- How should provenance gaps affect restored trust?
- How should conflicting checkpoints be represented?
- Which recovery evidence must remain after successful repair?
- How should recovery plans coordinate across several repair coordinators?
- Which recovery resources should be reserved before failure?
- Which low-level checkpoint, journal, recovery-state, quarantine, and reconstruction types should become shared C++ interfaces?
- How should implementation-specific recovery mechanisms extend the public contracts without exposing proprietary reconstruction logic?

These questions do not weaken the recovery boundaries established here.

## 37. Closing principle

> **Node must recover continuity from evidence, not from desperation.**

A restart is not recovery.

A surviving copy is not authority.

A checkpoint is not complete merely because it exists.

Replay is not renewed intent.

Reconstruction is not trusted restoration.

A stale custodian is not current because it returned.

Recovery succeeds only when identity, version, provenance, lifecycle, authority, and uncertainty are reconciled together.

## Revision history

### Version 0.1 — 2026-07-16

- Replaced the planned MEM-0008 stub with the first normative working draft.
- Defined recovery incidents, scope, objectives, boundaries, plans, sources, and results.
- Distinguished restart, repair, reconstruction, restoration, availability recovery, and durability recovery.
- Defined corruption, missing-material, stale, conflicting, orphaned, incompatible, and dependency-loss classes.
- Established detection evidence and quarantine semantics.
- Defined evidence-based source selection.
- Defined checkpoints, journals, replay, gap handling, and external-side-effect suppression.
- Defined repair, exact reconstruction, semantic reconstruction, approximate reconstruction, and partial reconstruction.
- Established reconstruction provenance and shard-set validation.
- Defined metadata, identity, catalog, index, provenance, schema, and lifecycle recovery.
- Established recovered-state validation and restoration requirements.
- Defined rejoining-custodian reconciliation and stale-state suppression.
- Defined recovery completeness and degraded-resumption semantics.
- Established unavailable-key, unavailable-schema, distributed-boundary, partition, lifecycle, security, resource, role, ACS, and physical-actuation boundaries.
- Added implementation-facing recovery conformance expectations.
