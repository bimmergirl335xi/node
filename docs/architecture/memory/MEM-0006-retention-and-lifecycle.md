# MEM-0006: Retention and Memory Lifecycle

| Field | Value |
|---|---|
| Specification | MEM-0006 |
| Title | Retention and Memory Lifecycle |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Accepted as initial working draft |
| Depends on | MEM-0000 through MEM-0005 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in lifecycle separation and deletion safety; restoration boundaries, derived-memory policy, and reclamation evidence remain under review |

> **Forgetting must be a governed lifecycle decision, not an accidental side effect of pressure, failure, or disconnected storage.**

## Architectural-intent notice

This specification defines the public architectural semantics governing how memory enters, remains within, changes service level within, and leaves the Node memory system.

It defines:

- lifecycle identity;
- lifecycle scope;
- retention intent;
- retention policy;
- lifecycle state;
- lifecycle transitions;
- archival;
- expiration;
- review;
- hold;
- deletion proposal;
- deletion authorization;
- deletion commitment;
- tombstones;
- deletion propagation;
- stale-state suppression;
- restoration;
- physical reclamation;
- sanitization;
- garbage-collection safety;
- lifecycle conflict;
- resource-pressure behavior;
- lifecycle evidence.

It does not define:

- private memory-selection policy;
- cognitive salience;
- emotional weighting;
- learning heuristics;
- consolidation algorithms;
- decay algorithms;
- exact retention durations;
- exact production capacity;
- one storage engine;
- one garbage collector;
- one physical sanitization mechanism;
- operator recovery procedures;
- legal-compliance rules for a particular deployment;
- exact production authority thresholds;
- exact replication or custody topology.

An implementation may use different storage systems, journals, catalogs, policy engines, clocks, compaction systems, and erasure mechanisms.

It must preserve the semantic distinctions established here.

## 1. Purpose

Memory cannot grow without limit.

Node operates under finite:

- volatile memory;
- persistent storage;
- bandwidth;
- compute;
- repair capacity;
- indexing capacity;
- energy;
- administrative attention.

Some memory must therefore:

- remain immediately available;
- move into lower-cost custody;
- become less frequently indexed;
- expire from active use;
- be deliberately forgotten;
- be deleted;
- eventually be physically reclaimed.

Those actions affect continuity and identity.

Without explicit lifecycle semantics, an implementation may incorrectly treat:

- cache eviction as forgetting;
- storage pressure as deletion authority;
- expiration as immediate physical erasure;
- archival as invalidation;
- an inaccessible copy as deleted;
- physical disappearance as authorized deletion;
- a tombstone as proof of sanitization;
- successful deletion on one custodian as global deletion;
- a stale restored copy as current memory;
- loss of a catalog entry as lifecycle completion;
- a failed repair as permission to reclaim remaining copies;
- derived memory as automatically deleted with its source;
- a deleted identifier as reusable.

MEM-0006 exists so that Node can intentionally preserve, archive, expire, forget, delete, and reclaim memory without confusing deliberate lifecycle behavior with accidental loss.

## 2. Scope

This specification governs lifecycle behavior for:

- logical memories;
- memory versions;
- memory representations;
- physical copies;
- aliases;
- catalog and index entries;
- provenance records;
- lineage records;
- schema dependencies;
- operation-history records;
- tombstones;
- checkpoints;
- journals;
- repair material;
- derived memories;
- reconstructed memories;
- lifecycle evidence.

It applies to lifecycle actions including:

- initial retention assignment;
- retention-policy revision;
- service-level change;
- archival;
- expiration;
- hold;
- release from hold;
- deletion proposal;
- deletion authorization;
- deletion commitment;
- deletion propagation;
- tombstone creation;
- restoration;
- physical reclamation;
- sanitization;
- metadata compaction;
- garbage collection.

Not every transient value becomes governed memory.

Once state is admitted as governed memory, lifecycle change must follow the applicable rules of this specification.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements.

The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification.

The word **may** describes permitted behavior.

A lifecycle implementation does not conform merely because it exposes states with names similar to those in this specification.

Its behavior must preserve the required semantic distinctions.

## 4. Foundational distinctions

### 4.1 Retention is not durability

Retention describes how long or under which conditions memory should remain governed and preserved.

Durability describes which failures the memory is currently protected against.

A memory may have long retention intent while temporarily under-protected.

### 4.2 Retention is not availability

Retained memory may be:

- immediately available;
- archived;
- temporarily unavailable;
- available only after reconstruction;
- inaccessible because required keys are unavailable.

Retention does not require continuous low-latency access.

### 4.3 Expiration is not deletion

Expiration means a retention condition has been reached and lifecycle evaluation is required or permitted.

Expiration does not independently authorize deletion.

### 4.4 Archival is not forgetting

Archival changes service level, placement, indexing, or expected retrieval cost.

The archived memory remains governed memory.

### 4.5 Supersession is not deletion

A superseded version remains part of memory history unless a separately governed lifecycle decision changes its retention.

### 4.6 Invalidation is not deletion

An invalid memory or version may need to remain available as:

- evidence;
- audit history;
- conflict history;
- repair input;
- security evidence.

Invalidation determines whether state may be trusted or used.

It does not automatically determine whether the state should be erased.

### 4.7 Logical deletion is not physical reclamation

Logical deletion establishes that memory must no longer be treated as active governed content within the declared scope.

Physical reclamation releases storage occupied by material.

These are separate stages.

### 4.8 Physical reclamation is not sanitization

Reclamation makes storage reusable.

Sanitization applies an assurance process intended to make prior material unrecoverable under a declared threat and medium model.

One does not automatically prove the other.

### 4.9 Forgetting is not accidental loss

Deliberate forgetting is a governed lifecycle outcome.

Accidental loss results from failure, corruption, missing custody, lost keys, or implementation defect.

Accidental loss must not be rewritten in history as authorized forgetting.

### 4.10 A tombstone is not the deleted memory

A tombstone is lifecycle evidence that prevents deleted state from being treated as active.

It is not a retained substitute for the deleted content.

### 4.11 Deletion propagation is not deletion authorization

A custodian receiving a deletion instruction does not determine whether the original deletion was authorized.

It enforces an already governed decision within its assigned scope.

### 4.12 Deletion completion is not necessarily global sanitization

A lifecycle operation may complete after logical deletion and sufficient propagation while some inaccessible or offline physical material remains pending reclamation.

The remaining limitation must be explicit.

### 4.13 Loss of identity metadata is not deletion

If content or copies cannot be resolved because metadata is lost, the memory is:

- unavailable;
- orphaned;
- lost;
- unknown;

not validly deleted.

### 4.14 Resource pressure is not lifecycle authority

Capacity pressure may trigger:

- admission limits;
- archival proposals;
- retention review;
- migration;
- compaction;
- deletion proposals.

It does not independently authorize forgetting protected memory.

## 5. Lifecycle model

Memory lifecycle is not necessarily one universal linear sequence.

A memory may carry several independent lifecycle dimensions, including:

- retention state;
- availability tier;
- deletion state;
- hold state;
- reclamation state;
- sanitization state;
- protection state.

An implementation may encode these dimensions in one state machine or several coordinated records.

It must not collapse materially different states into one ambiguous label.

## 6. Lifecycle identity and scope

### 6.1 Lifecycle event identity

Every lifecycle event capable of changing governed memory state must have stable operation identity under MEM-0004.

Examples include:

- retention assignment;
- archival;
- expiration decision;
- hold placement;
- deletion authorization;
- deletion commitment;
- restoration;
- reclamation approval;
- sanitization confirmation.

### 6.2 Lifecycle scope

A lifecycle operation must identify the exact subject it affects.

Possible scopes include:

- one logical memory;
- one version;
- a bounded version range;
- one representation;
- one physical copy;
- one alias;
- one provenance record;
- one catalog entry;
- one retention-policy assignment;
- one derived-memory relation;
- one operation-history class.

### 6.3 Scope must not silently expand

Deleting one physical copy must not become deletion of the logical memory.

Deleting one version must not automatically delete:

- later versions;
- derived memories;
- independent provenance;
- unrelated aliases;
- other logical memories.

### 6.4 Scope dependencies are explicit

A lifecycle decision must account for dependencies required to:

- interpret;
- validate;
- authorize;
- recover;
- suppress stale resurrection;
- audit

the affected memory.

### 6.5 Lifecycle state is versioned

A material lifecycle change shall produce a distinguishable lifecycle event or state version.

Current lifecycle state must be traceable to its governing decisions.

## 7. Conceptual lifecycle records

These records define semantics, not a mandatory wire format or database schema.

### 7.1 Retention-policy record

```text
RetentionPolicyRecord
    policy_identity
    policy_version
    applicable_memory_scope
    retention_class
    minimum_retention_conditions
    maximum_or_review_conditions
    archival_conditions
    expiration_conditions
    deletion_eligibility_conditions
    hold_behavior
    required_authority
    required_evidence
    effective_boundary
    superseded_policy_reference
```

### 7.2 Lifecycle-state record

```text
MemoryLifecycleState
    subject_identity
    lifecycle_state
    retention_policy_reference
    lifecycle_state_version
    governing_operation_identity
    governing_authority
    effective_boundary
    hold_state
    deletion_state
    reclamation_state
    sanitization_state
    propagation_state
    unresolved_conflicts
    evidence_references
```

### 7.3 Tombstone record

```text
MemoryTombstone
    tombstone_identity
    deleted_subject_identity
    deleted_scope
    deletion_operation_identity
    deletion_commit_evidence
    lifecycle_generation
    effective_boundary
    authority_reference
    stale_suppression_requirements
    restoration_policy
    minimum_tombstone_retention
    propagation_observations
```

### 7.4 Reclamation record

```text
ReclamationRecord
    subject_or_copy_identity
    governing_deletion_reference
    reclamation_authority
    dependency_check_result
    hold_check_result
    propagation_requirement
    physical_action
    physical_action_result
    sanitization_claim
    evidence_reference
    remaining_known_material
```

Equivalent information may be:

- journaled;
- event-sourced;
- embedded in catalogs;
- distributed among authorities;
- represented through immutable manifests;
- reconstructed from governed evidence.

Its meaning must remain recoverable.

## 8. Retention policy

### 8.1 Retention is explicit

Governed memory shall have an explicit retention policy, inherited policy, or identifiable default policy.

### 8.2 Policy identity

Retention policy must be identifiable and versioned.

A later policy revision shall not silently rewrite the historical policy under which earlier lifecycle actions occurred.

### 8.3 Policy scope

A policy may apply to:

- a memory class;
- a namespace;
- one logical memory;
- one version class;
- one representation class;
- continuity-critical state;
- operation history;
- provenance;
- tombstones.

### 8.4 Retention conditions

Retention may be governed by:

- elapsed time;
- lifecycle events;
- replacement by a newer version;
- successful consolidation;
- successful archival;
- successful reconstruction;
- explicit review;
- dependency state;
- policy change;
- storage pressure combined with authority;
- another governed condition.

### 8.5 Exact durations are profile-defined

This specification does not define universal retention durations.

Durations and conditions belong to implementation or memory-class policy.

### 8.6 Minimum retention

A minimum-retention condition prevents ordinary deletion before a boundary is reached.

Emergency or overriding authority, when supported, must remain explicit and auditable.

### 8.7 Maximum retention or review boundary

A policy may require that memory be:

- reviewed;
- archived;
- transformed;
- deleted;
- reauthorized

after a declared condition.

Reaching that condition does not silently perform the transition.

### 8.8 Retention inheritance

A memory may inherit policy from:

- memory class;
- namespace;
- source operation;
- producing role;
- applicable schema;
- continuity designation.

The effective policy must remain resolvable.

### 8.9 Retention override

An override must identify:

- authorizing role;
- affected scope;
- reason or policy basis;
- effective boundary;
- override duration;
- policy precedence.

### 8.10 Policy conflict

When applicable retention policies conflict, the conflict must remain explicit until resolved.

A custodian must not choose whichever policy permits earliest deletion.

## 9. Retention classes

Implementations may define their own class names.

Supported classes should preserve distinctions such as the following.

### 9.1 Transient

State may be discarded after its bounded operation or context ends.

Transient state is not automatically governed durable memory.

### 9.2 Ephemeral governed memory

State has logical identity and lifecycle tracking but is expected to exist for a short bounded period.

Its expiration and deletion must still be governed.

### 9.3 Active retained memory

Memory remains within its ordinary service and retrieval expectations.

### 9.4 Archived memory

Memory remains governed but may have:

- slower retrieval;
- reduced indexing;
- lower-cost placement;
- offline custody;
- reconstruction requirements.

### 9.5 Long-retained memory

Memory is expected to remain governed for an extended or indefinite policy horizon, subject to explicit lifecycle authority.

### 9.6 Continuity-critical memory

Memory whose loss would materially harm Node continuity requires stronger lifecycle safeguards.

Such safeguards may include:

- stronger deletion authority;
- independent approval;
- extended tombstone retention;
- validated recovery points;
- explicit dependency analysis.

### 9.7 Evidence-retained memory

Invalid, rejected, conflicting, or security-relevant material may be retained primarily as evidence.

Its use authority may differ from its retention requirement.

### 9.8 Retention class does not define cognitive importance

Public retention classes describe lifecycle behavior.

They do not expose or prescribe private cognitive significance policy.

## 10. Lifecycle states

Implementations may use different names.

They must preserve equivalent semantic distinctions.

### 10.1 Proposed

Memory or a lifecycle transition has been proposed but not admitted or committed.

### 10.2 Provisional

Memory is temporarily governed while required validation, commitment, or retention assignment remains incomplete.

Provisional state must not be presented as fully committed long-term memory.

### 10.3 Active

Memory is governed and eligible for ordinary operations under its applicable policy.

### 10.4 Archived

Memory remains governed but is assigned to a reduced or different availability tier.

### 10.5 Expiration due

A policy condition requiring lifecycle evaluation has been reached.

No deletion is implied.

### 10.6 Under review

A lifecycle authority is evaluating whether memory should:

- remain active;
- be archived;
- receive a policy extension;
- be transformed;
- be deleted.

### 10.7 Held

A hold blocks or constrains one or more lifecycle actions.

A held memory may remain active or archived.

### 10.8 Deletion proposed

A request to delete governed memory has been created but not yet authorized or committed.

### 10.9 Deletion authorized

The required authority has approved deletion within a declared scope.

The authoritative lifecycle transition may still be pending commitment.

### 10.10 Deletion committed

The governed memory is logically deleted within the declared scope.

It must no longer be returned as active memory unless a permitted restoration process succeeds.

### 10.11 Propagation pending

The deletion decision is committed, but required custodians, catalogs, caches, or indexes have not all confirmed enforcement.

### 10.12 Reclamation eligible

The semantic and safety conditions permitting physical reclamation have been established.

### 10.13 Reclamation pending

Physical material is scheduled or awaiting reclamation.

### 10.14 Reclaimed

Known physical material within the declared scope has been released or removed according to the reclamation contract.

### 10.15 Sanitization pending

A required sanitization action has not yet been completed or evidenced.

### 10.16 Sanitized

The applicable sanitization mechanism reports completion under a declared assurance model.

### 10.17 Restoration proposed

A request has been made to reverse or supersede a prior lifecycle decision where permitted.

### 10.18 Restored

A governed restoration decision has returned memory to an allowed active or archived state.

### 10.19 Lost

Required memory material is believed to have been destroyed or become unrecoverable without a valid deletion decision.

Lost is a failure state, not a lifecycle success.

### 10.20 Unknown

Available evidence is insufficient to establish lifecycle state.

### 10.21 Conflicting

Incompatible valid-looking lifecycle claims exist.

## 11. Lifecycle transitions

### 11.1 Transitions are governed operations

Material transitions must use the operation semantics of MEM-0004.

### 11.2 Preconditions are explicit

A transition may require:

- current lifecycle version;
- retention-policy version;
- absence of hold;
- required authorization;
- dependency analysis;
- successful archival;
- successful replacement;
- validated recovery point;
- propagation evidence.

### 11.3 Compare-and-transition

Where stale decisions are possible, a lifecycle request should identify the expected current state or lifecycle generation.

A mismatch must produce conflict or failed precondition rather than silent overwrite.

### 11.4 Transition finality

A result must distinguish:

- accepted;
- authorized;
- committed;
- propagated;
- reclaimed;
- sanitized.

### 11.5 Failed transition

A failed lifecycle transition must not partially change state unless partial behavior was explicitly permitted and reported.

### 11.6 Indeterminate transition

If the system cannot establish whether a transition committed, it must remain indeterminate and be reconciled using the same operation identity.

### 11.7 Transition history

Material lifecycle history must remain available long enough to:

- reject stale decisions;
- prevent resurrection;
- audit deletion;
- reconcile disconnected custodians;
- interpret reclamation.

## 12. Archival

### 12.1 Archival preserves logical identity

Moving memory into archival custody shall not create a new logical memory merely because placement or representation changes.

### 12.2 Archival changes service expectations

Archival may change:

- latency;
- locality;
- availability target;
- indexing;
- preferred representation;
- retrieval cost;
- restoration requirements.

### 12.3 Archival does not weaken honesty

Archived memory must remain distinguishable from:

- deleted;
- unavailable;
- lost;
- invalid;
- not found.

### 12.4 Archival completion

An archival result must identify:

- archived scope;
- new custody or representation state;
- achieved durability;
- retrieval expectations;
- remaining active copies;
- pending reclamation of prior copies.

### 12.5 Reclamation after archival

Prior active-tier copies may be reclaimed only after the archival policy's durability and validation requirements are satisfied.

### 12.6 Archive retrieval

Retrieval from archive may require:

- restoration;
- staging;
- reconstruction;
- key retrieval;
- schema support.

Temporary retrieval difficulty does not change retention state.

## 13. Expiration

### 13.1 Expiration is a policy trigger

Expiration indicates that a lifecycle condition has been met.

It may trigger:

- review;
- archival;
- deletion proposal;
- retention extension;
- consolidation;
- transformation.

### 13.2 Expiration does not self-authorize deletion

A custodian shall not erase governed memory solely because a local clock or timer says it expired.

### 13.3 Expiration evidence

Expiration evaluation should identify:

- policy version;
- triggering condition;
- time evidence where relevant;
- applicable hold;
- required authority;
- dependencies.

### 13.4 Clock uncertainty

Time-based expiration must account for:

- clock source;
- clock uncertainty;
- unavailable clock evidence;
- restored systems;
- delayed processing.

### 13.5 Expired but retained

Memory may remain retained after expiration because:

- review is pending;
- a hold applies;
- deletion authority is unavailable;
- a dependency requires continued retention;
- policy was extended;
- reclamation cannot safely proceed.

## 14. Holds

### 14.1 Hold purpose

A hold prevents or constrains lifecycle transitions for a declared reason and scope.

### 14.2 Hold scope

A hold may apply to:

- logical deletion;
- physical reclamation;
- sanitization;
- policy change;
- archival;
- one version;
- an entire lineage;
- associated provenance;
- operation evidence.

### 14.3 Hold identity

A hold must identify:

- authority;
- scope;
- effective boundary;
- reason category where disclosure is permitted;
- release conditions;
- expiration or review behavior.

### 14.4 Hold precedence

The relationship between holds and retention policies must be explicit.

A custodian must not silently ignore a hold because another policy permits deletion.

### 14.5 Hold release

Releasing a hold is a governed lifecycle operation.

Release does not automatically delete memory.

### 14.6 Conflicting holds

Conflicting hold claims must remain explicit until resolved.

## 15. Deletion proposal and authorization

### 15.1 Deletion begins as a proposal

A deletion proposal identifies requested scope and effect.

It is not deletion.

### 15.2 Required proposal information

A deletion proposal should identify:

- operation identity;
- requester;
- authority context;
- target identity and scope;
- expected lifecycle state;
- retention-policy basis;
- hold state;
- dependencies;
- propagation requirements;
- restoration policy;
- sanitization requirement;
- requested effective boundary.

### 15.3 Authorization is explicit

Deletion requires authority assigned to the lifecycle operation and target scope.

### 15.4 Stronger authority may be required

Policies may require stronger or independent approval for:

- continuity-critical memory;
- broad namespace deletion;
- deletion of identity metadata;
- deletion of provenance required by other memory;
- deletion during conflict;
- deletion after suspected compromise;
- deletion of recovery material.

This specification does not define exact approval counts.

### 15.5 Self-deletion is not implicit

A proposer, producer, custodian, or consumer does not automatically possess authority to delete memory it created, stores, or uses.

### 15.6 Deletion under conflict

Deletion should not commit while unresolved conflict could cause the operation to target:

- the wrong identity;
- the wrong version;
- an incomplete scope;
- state under a valid hold.

A policy permitting deletion under conflict must state how conflicting evidence is preserved.

## 16. Logical deletion

### 16.1 Commitment

Logical deletion occurs when an authorized deletion event enters authoritative lifecycle history.

### 16.2 Effect

After deletion commitment, the affected memory must not be returned as ordinary active or archived memory within the deleted scope.

### 16.3 Historical evidence

Deletion must preserve enough lifecycle evidence to establish:

- what was deleted;
- which scope was affected;
- which authority acted;
- which operation committed;
- which lifecycle generation applies;
- whether restoration is permitted;
- which stale state must be suppressed.

### 16.4 Identifier non-reuse

Deleted logical identifiers and version identifiers remain protected against reuse for unrelated memory.

### 16.5 Deletion does not erase history instantly

Required operation, authority, and tombstone evidence may remain after content deletion.

### 16.6 Deletion visibility

A retrieval may report:

- deleted;
- not found within active scope;
- existence concealed;
- unavailable;

according to authorization and operation contract.

Internally, deleted and never-found must remain distinct.

### 16.7 Version-scoped deletion

Deleting one version does not erase its historical effect from later version lineage.

Later versions may retain a reference to a deleted parent without retaining the parent's full content.

### 16.8 Memory-scoped deletion

Deleting a logical memory must define treatment of:

- all versions;
- representations;
- copies;
- aliases;
- indexes;
- provenance;
- derived memories;
- operation evidence;
- tombstones.

## 17. Tombstones

### 17.1 Purpose

A tombstone records enough deletion state to prevent:

- stale resurrection;
- identifier reuse;
- incorrect not-found interpretation;
- reimport of deleted copies;
- catalog repair from recreating active memory.

### 17.2 Tombstone durability

Tombstone protection must reflect the duration and failure conditions under which stale copies may reappear.

### 17.3 Tombstone scope

A tombstone must identify whether it applies to:

- logical memory;
- version;
- representation;
- physical copy;
- alias;
- another bounded scope.

### 17.4 Tombstone propagation

Required catalogs, custodians, caches, recovery systems, and repair systems must receive or reconcile against the applicable tombstone before treating returning state as active.

### 17.5 Tombstone compaction

Tombstones may be compacted only when equivalent stale-suppression and non-reuse guarantees remain.

### 17.6 Tombstone reclamation

A tombstone must not be reclaimed merely because currently known copies are gone.

Its retention must account for:

- disconnected custodians;
- old backups;
- offline archives;
- delayed replication;
- restored snapshots;
- external media;
- maximum supported rejoin interval.

### 17.7 Tombstone conflict

If a tombstone conflicts with a later valid restoration or a competing lifecycle history, the conflict must remain explicit until resolved.

## 18. Deletion propagation

### 18.1 Propagation scope

Deletion propagation may affect:

- active custodians;
- archival custodians;
- catalogs;
- locators;
- indexes;
- caches;
- repair queues;
- reconstruction manifests;
- backup or checkpoint inventories;
- operation-status systems.

### 18.2 Offline participants

An offline custodian may not immediately enforce deletion.

The lifecycle result must distinguish:

- deletion committed;
- propagation complete;
- propagation pending;
- known unreachable custodian;
- unknown custody.

### 18.3 Rejoining custodians

A rejoining custodian must reconcile retained material against current tombstones and lifecycle generations before serving or repairing it.

### 18.4 Propagation failure

Propagation failure must not reverse deletion commitment.

It creates an incomplete enforcement or reclamation state.

### 18.5 New copies after deletion

Repair, caching, replication, or migration systems must not create new active copies from deleted material.

### 18.6 Deletion acknowledgment

A custodian acknowledgment proves only the enforcement action it actually completed.

It does not independently prove global deletion.

## 19. Physical reclamation

### 19.1 Eligibility

Physical material becomes reclamation-eligible only when:

- governing lifecycle authority is valid;
- applicable holds permit reclamation;
- required tombstone or equivalent evidence exists;
- required dependencies have been evaluated;
- required copies or recovery points remain where policy demands;
- reclamation scope is unambiguous;
- pending operations are reconciled.

### 19.2 Reclamation authority

The authority to reclaim a physical copy may differ from authority to logically delete memory.

### 19.3 Copy reclamation without logical deletion

A redundant or obsolete physical copy may be reclaimed while the logical memory remains active, provided durability and custody policy remain satisfied.

### 19.4 Metadata reclamation

Metadata must not be reclaimed while it remains necessary to:

- resolve identity;
- validate memory;
- enforce deletion;
- prevent resurrection;
- reconstruct memory;
- interpret provenance;
- audit authority.

### 19.5 Reclamation evidence

Reclamation should record:

- material scope;
- physical action;
- governing authority;
- completion state;
- remaining known copies;
- sanitization state where required.

### 19.6 Reclamation failure

Failure to reclaim physical material does not restore logical memory.

It leaves physical cleanup incomplete.

## 20. Sanitization

### 20.1 Sanitization is policy-specific

Sanitization requirements depend on:

- storage medium;
- encryption model;
- threat model;
- deployment policy;
- physical custody.

### 20.2 Sanitization claim

A sanitization claim must identify:

- affected material;
- method or assurance profile;
- executing authority;
- evidence;
- limitations;
- completion state.

### 20.3 Encryption-key destruction

Destruction or revocation of decryption authority may contribute to sanitization.

It must not be claimed as universal physical erasure without a declared assurance model.

### 20.4 Shared media

Reclamation of one logical object on shared media may not immediately overwrite its former physical blocks.

The result must not overstate sanitization.

### 20.5 Sanitization unknown

When sanitization cannot be verified, state must remain:

- pending;
- unknown;
- incomplete;

rather than falsely confirmed.

## 21. Restoration

### 21.1 Restoration is governed

Restoration is not automatic rediscovery of old bytes.

It is an authorized lifecycle operation.

### 21.2 Restoration boundaries

A policy must define whether restoration is permitted after:

- deletion proposal;
- deletion authorization;
- deletion commitment;
- reclamation;
- sanitization.

### 21.3 Restoration before deletion commitment

A pending deletion may be cancelled or superseded when policy permits and no committed deletion has occurred.

### 21.4 Restoration after deletion commitment

Restoration after committed deletion must:

- preserve deletion history;
- use a new lifecycle event;
- reconcile tombstones;
- identify restored scope;
- prevent unrelated stale copies from becoming current;
- preserve authority evidence.

### 21.5 Restoration after content reclamation

When content was physically reclaimed, restoration may require:

- archive recovery;
- reconstruction;
- external evidence;
- re-creation as a derived memory.

If semantic continuity cannot be established, the result may require a new logical memory rather than restoration of the old one.

### 21.6 Restoration after sanitization

A successful sanitization claim may make direct restoration impossible.

Recreating similar content later does not automatically recreate the deleted memory's identity.

### 21.7 Stale copy is not restoration authority

Discovery of an old copy does not independently authorize restoration.

## 22. Protection against stale-state resurrection

### 22.1 Definition

Stale-state resurrection occurs when deleted, superseded, invalidated, or older lifecycle state returns and is incorrectly treated as current active memory.

### 22.2 Required prevention

Implementations must protect against resurrection from:

- restored backups;
- offline custodians;
- old checkpoints;
- delayed messages;
- stale caches;
- disconnected replicas;
- abandoned repair jobs;
- reconstructed catalogs;
- replayed operation requests.

### 22.3 Lifecycle generation

Lifecycle records should provide a generation, ordering relation, or equivalent evidence sufficient to reject stale state.

### 22.4 Repair checks

Repair and reconstruction systems must consult current lifecycle evidence before recreating material.

### 22.5 Cache checks

Caches must not serve deleted state after learning of an applicable deletion.

### 22.6 Backup restoration

Restored backups must be reconciled against lifecycle history newer than the backup boundary.

### 22.7 Unknown deletion coverage

When the system cannot establish whether restored material is covered by a deletion, the material must remain quarantined, orphaned, unknown, or otherwise unavailable for ordinary use.

## 23. Derived memories and lifecycle

### 23.1 Derivation does not imply shared identity

A derived memory normally possesses independent logical identity under MEM-0003.

### 23.2 Source deletion does not automatically delete derivatives

Deleting a source memory does not automatically delete derived memories unless explicit policy governs that dependency.

### 23.3 Derivative retention must remain interpretable

A derived memory may require retained provenance indicating that deleted or unavailable sources once contributed to it.

### 23.4 Dependency-sensitive deletion

Before deleting a source, lifecycle evaluation should determine whether the source remains required to:

- validate a derivative;
- explain provenance;
- reproduce a result;
- resolve a conflict;
- support recovery;
- satisfy audit policy.

### 23.5 Redacted provenance

A source may be deleted while a derivative retains a restricted or redacted provenance reference.

The derivative must not falsely claim independent origin.

### 23.6 Cascade deletion is explicit

Any policy that cascades deletion through derivation relationships must define:

- relation types included;
- authority;
- stopping conditions;
- conflict behavior;
- independent retention overrides;
- evidence preservation.

### 23.7 Consolidation is not automatic deletion

Successful consolidation may make source memories eligible for lifecycle review.

It does not independently authorize source deletion.

## 24. Provenance, schema, and dependency lifecycle

### 24.1 Dependency retention

A dependency required to interpret, validate, authorize, or recover retained memory must remain available or safely replaceable.

### 24.2 Schema lifecycle

A schema may be reclaimed only when:

- no retained memory requires it; or
- retained memory has been safely migrated; or
- equivalent interpretation remains available.

### 24.3 Provenance lifecycle

Provenance may have different retention and disclosure policy from memory content.

Required provenance must not disappear earlier than the memory relationship it supports.

### 24.4 Key lifecycle

Required decryption authority must be retained or deliberately destroyed according to the lifecycle policy of the governed memory.

### 24.5 Operation-history lifecycle

Operation identity and result history must remain long enough to prevent unsafe duplicate lifecycle effects.

### 24.6 Dependency conflict

Conflicting evidence about whether a dependency remains needed must block unsafe reclamation.

## 25. Garbage-collection safety

### 25.1 Garbage collection is an implementation mechanism

Garbage collection identifies and reclaims material that lifecycle policy declares safely reclaimable.

It does not create deletion authority.

### 25.2 Reachability is not sufficient

An object being unreachable from one index, process, graph root, or catalog does not prove it is safe to reclaim.

### 25.3 Required liveness evidence

Garbage collection must consider, where applicable:

- logical lifecycle state;
- retention policy;
- holds;
- tombstones;
- active operations;
- version lineage;
- derivation dependencies;
- reconstruction dependencies;
- offline custodians;
- archival state;
- required provenance;
- repair state.

### 25.4 Marking errors

Uncertain liveness must not be treated as confirmed garbage.

### 25.5 Concurrent operations

Garbage collection must not reclaim material that an admitted operation is authorized and expected to use unless the operation contract permits that outcome.

### 25.6 Distributed garbage collection

A local custodian must not infer global reclaimability solely from local references.

### 25.7 Garbage-collection result

A collection result should identify:

- evaluated scope;
- reclaimed material;
- deferred material;
- blocked material;
- unknown dependencies;
- failed actions.

### 25.8 Compaction

Compaction may reduce metadata or history size only when required semantic and recovery evidence remains equivalent.

## 26. Resource pressure

### 26.1 Pressure is explicit

Memory services must report relevant pressure before protected memory is silently lost.

### 26.2 Permitted responses

Resource pressure may cause:

- rejection of new proposals;
- deferral;
- reduced replication work;
- migration;
- archival;
- cache eviction;
- index reduction;
- lifecycle review;
- deletion proposals.

### 26.3 Cache eviction

Evicting a cache entry is not deletion of logical memory.

### 26.4 Index eviction

Removing an index entry is not deletion of logical memory and may reduce search completeness.

### 26.5 Emergency behavior

An emergency profile may define extraordinary lifecycle actions.

It must state:

- authority;
- memory scope;
- safeguards;
- evidence;
- expected loss;
- recovery implications.

### 26.6 Accidental loss under pressure

If implementation failure causes protected memory to be lost under pressure, the result must be recorded as loss or failure—not rewritten as expiration or forgetting.

## 27. Availability and lifecycle interaction

### 27.1 Archived availability

Archived memory may be retained despite temporary retrieval unavailability.

### 27.2 Deleted availability

Deleted memory must not be returned as active content merely because a physical copy remains readable.

### 27.3 Lifecycle unknown

When lifecycle state is unknown, a service must not assume active eligibility.

### 27.4 Conflicting lifecycle

A memory with conflicting active and deleted claims must be treated conservatively until resolved.

### 27.5 Not-found behavior

A deleted result and a never-existing result may both be externally reported through policy-limited responses.

Internally they remain distinct.

### 27.6 Under-protection

Lifecycle actions must not claim successful long-term retention while the memory is known to be under-protected without disclosing that state.

## 28. Partition and disconnected-custodian behavior

### 28.1 Deletion during partition

A deletion may commit during partition only when the applicable authority and consistency model permits it.

### 28.2 Incomplete propagation

When disconnected custodians cannot receive deletion state, propagation remains incomplete.

### 28.3 Local expiration

A disconnected custodian must not independently convert local expiration into logical deletion unless explicitly assigned that authority and policy permits offline action.

### 28.4 Returning material

Material returning after partition must be checked against:

- lifecycle generation;
- tombstones;
- restoration events;
- current retention policy;
- version history.

### 28.5 Split lifecycle authority

Competing lifecycle decisions created during partition must remain conflicting until governed reconciliation.

## 29. Failure behavior

### 29.1 Lifecycle-service failure

Failure of lifecycle authority does not automatically change existing lifecycle state.

### 29.2 Catalog failure

Loss of a lifecycle catalog does not mean memory became active, expired, or deleted.

### 29.3 Tombstone loss

Loss of required tombstone state is a protection failure and may create resurrection risk.

### 29.4 Key loss

Unintentional key loss causing memory inaccessibility is accidental loss or unavailability, not deliberate sanitization.

### 29.5 Reclamation failure

Failure to reclaim material leaves cleanup incomplete.

It does not reverse deletion.

### 29.6 Sanitization failure

Failure to prove sanitization must remain explicit.

### 29.7 Unknown state

When lifecycle evidence is missing or conflicting, the system must represent unknown or conflicting state rather than select the most convenient outcome.

## 30. Role boundaries

### 30.1 Memory proposer

May propose initial retention requirements.

It does not independently assign authoritative lifecycle policy.

### 30.2 Admission evaluator

May evaluate whether requested retention and durability are supportable.

Admission is not lifecycle commitment.

### 30.3 Commit authority

May establish authoritative lifecycle events when assigned.

### 30.4 Lifecycle authority

Evaluates and authorizes:

- retention changes;
- archival;
- expiration decisions;
- holds;
- deletion;
- restoration;
- reclamation eligibility.

### 30.5 Custodian

Enforces assigned lifecycle decisions on physical material.

Custody does not create deletion authority.

### 30.6 Catalog and locator

Records lifecycle and location state without inventing lifecycle decisions.

### 30.7 Validator

Evaluates:

- lifecycle evidence;
- policy applicability;
- tombstone validity;
- reclamation preconditions;
- sanitization evidence.

Validation does not itself authorize deletion.

### 30.8 Repair coordinator

Must not repair or reconstruct deleted memory as active state.

### 30.9 Observer and auditor

Records lifecycle evidence but does not independently mutate lifecycle state.

### 30.10 Access authority

Determines whether lifecycle state or retained content may be disclosed.

No role may exceed its assigned lifecycle authority.

## 31. Boundary with ACS

ACS governs:

- relationships;
- connections;
- endpoints;
- ports;
- signals;
- payload references;
- delivery;
- communication admission;
- routing;
- secure sessions;
- communication backpressure.

MEM governs:

- lifecycle identity;
- retention;
- archival;
- expiration;
- hold;
- deletion authority;
- deletion commitment;
- tombstones;
- propagation state;
- reclamation;
- restoration;
- stale-state suppression.

The following rules apply:

1. Connection loss is not deletion.
2. Endpoint disappearance is not expiration.
3. Payload-reference expiry is not logical forgetting.
4. Delivery of a deletion request is not deletion commitment.
5. Delivery acknowledgment is not global deletion completion.
6. Communication retry must preserve deletion operation identity.
7. Communication failure must not reactivate deleted memory.
8. ACS backpressure may delay propagation but does not redefine lifecycle state.
9. MEM does not redefine connection or payload-reference lifecycle.
10. ACS does not define memory retention or deletion meaning.

## 32. Security and privacy

### 32.1 Lifecycle metadata may be sensitive

Lifecycle evidence may reveal:

- memory existence;
- deletion activity;
- retention class;
- recovery capability;
- custodian placement;
- security incidents.

Access must remain authorized.

### 32.2 Deletion authorization is least-privilege

Authority must be limited by:

- subject scope;
- operation type;
- retention class;
- lifecycle state;
- time or policy boundary.

### 32.3 Replay protection

Replaying an old deletion, restoration, or hold request must not repeat or broaden its semantic effect.

### 32.4 Compromise containment

Compromise of one custodian must not permit deletion of unrelated memory.

### 32.5 Security evidence retention

Evidence of unauthorized deletion attempts may require retention even when the targeted memory is later deleted.

### 32.6 Concealed lifecycle

External policy may conceal whether memory is:

- deleted;
- retained;
- unauthorized;
- nonexistent.

Internal lifecycle truth must remain accurate.

## 33. Conformance expectations

A conforming implementation must demonstrate:

- deliberate forgetting versus accidental loss;
- retention versus durability;
- retention versus availability;
- expiration without automatic deletion;
- archival without identity loss;
- deletion proposal, authorization, commitment, propagation, reclamation, and sanitization separation;
- stable lifecycle operation identity;
- deletion retry safety;
- tombstone creation and retention;
- stale-custodian reconciliation;
- backup-restoration reconciliation;
- identifier non-reuse;
- hold enforcement;
- resource-pressure behavior;
- garbage-collection dependency checks;
- derived-memory non-cascade by default;
- lifecycle conflict reporting;
- restoration behavior;
- unknown lifecycle preservation;
- physical-copy deletion distinct from logical-memory deletion.

Detailed failure testing belongs to MEM-0010.

## 34. Prohibited interpretations

This specification shall not be interpreted to mean that:

- every memory must be retained forever;
- expiration means deletion;
- archival means invalidity;
- superseded means deleted;
- stale means deletion-eligible;
- invalid means safe to erase;
- cache eviction is forgetting;
- index removal is deletion;
- storage pressure grants lifecycle authority;
- local disk cleanup establishes logical deletion;
- deletion authorization proves deletion commitment;
- deletion commitment proves propagation completion;
- propagation completion proves sanitization;
- reclamation proves sanitization;
- key loss proves authorized forgetting;
- inaccessible means deleted;
- missing catalog entry means deleted;
- one custodian may delete every copy because it stores one;
- a tombstone may be discarded as soon as known copies disappear;
- a restored backup may ignore newer tombstones;
- source deletion automatically deletes derived memory;
- successful consolidation automatically deletes sources;
- deleted identifiers may be reused;
- garbage collection may infer global liveness from one local index;
- restoration may erase deletion history;
- public lifecycle policy defines private cognitive importance.

## 35. Open questions

The following questions remain for later specifications and implementation profiles:

- Which lifecycle states are mandatory for baseline conformance?
- Which memory classes require holds or independent deletion authority?
- Which memories may expire automatically into review?
- Which memories may move automatically into archive?
- What evidence is sufficient for reclamation eligibility?
- How long must tombstones remain protected?
- How should maximum supported custodian-disconnection time affect tombstone retention?
- Which lifecycle operations require quorum or independent approval?
- How should deletion interact with unresolved version conflict?
- How should deletion interact with conflicting retention policies?
- Which derived-memory relationships require dependency review?
- When may provenance outlive deleted content?
- How should redacted provenance reference deleted sources?
- Which recovery points may delay reclamation?
- How should old offline backups be retired or reconciled?
- When does restoration preserve identity versus create a new derived memory?
- Which sanitization assurance profiles should public conformance recognize?
- How should encrypted archival media report key-destruction state?
- Which lifecycle evidence must survive sanitization?
- How should garbage-collection liveness be represented across distributed custodians?
- Which low-level lifecycle, tombstone, and retention-policy types should become shared C++ interfaces?

These questions do not weaken the lifecycle distinctions already established.

## 36. Closing principle

> **Node must know the difference between choosing to forget, being unable to remember, and merely moving memory somewhere harder to reach.**

Retention is not durability.

Archival is not deletion.

Expiration is not authority.

Deletion is not reclamation.

Reclamation is not sanitization.

A stale copy is not restoration.

Forgetting must remain intentional, evidenced, bounded, and resistant to reversal by accident.

## Revision history

### Version 0.1 — 2026-07-16

- Replaced the planned MEM-0006 stub with the first normative working draft.
- Defined retention policy, lifecycle identity, lifecycle scope, and lifecycle-state semantics.
- Distinguished retention, durability, availability, archival, expiration, deletion, reclamation, and sanitization.
- Defined conceptual retention-policy, lifecycle-state, tombstone, and reclamation records.
- Established lifecycle transitions, preconditions, finality, and conflict handling.
- Defined archival and expiration behavior without automatic deletion.
- Defined holds, deletion proposal, authorization, commitment, and propagation.
- Established tombstone durability and stale-state-resurrection protection.
- Defined physical reclamation and sanitization as separate stages.
- Defined governed restoration behavior.
- Established derived-memory, provenance, schema, key, and operation-history lifecycle boundaries.
- Defined garbage-collection safety and resource-pressure behavior.
- Preserved role, ACS, security, availability, and failure boundaries.
- Established implementation-facing lifecycle conformance expectations.
