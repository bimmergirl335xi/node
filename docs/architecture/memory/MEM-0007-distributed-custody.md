# MEM-0007: Distributed Custody and Locality

| Field | Value |
|---|---|
| Specification | MEM-0007 |
| Title | Distributed Custody and Locality |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Accepted as initial working draft |
| Depends on | MEM-0000 through MEM-0006 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in custody, failure-domain, and under-protection semantics; baseline placement profiles and coding-policy boundaries remain under review |

> **Replica count alone is not resilience; custody must account for correlated failure, locality, finite resources, and explicit under-protection.**

## Architectural-intent notice

This specification defines the public architectural semantics governing where memory material is physically held, how custody responsibility is assigned, how independent protection is evaluated, and how placement changes without changing logical memory identity.

It defines:

- custody assignments;
- physical placement;
- replicas;
- reconstruction shards;
- failure domains;
- correlated failure;
- durability targets;
- protection evidence;
- locality;
- placement constraints;
- placement preferences;
- migration;
- replication;
- reconstruction distribution;
- rebalancing;
- evacuation;
- custodian joining and departure;
- capacity pressure;
- under-protection;
- over-placement;
- placement conflict;
- partition behavior;
- custody observability.

It does not define:

- exact production node maps;
- one mandatory storage technology;
- one mandatory replication algorithm;
- one mandatory erasure-coding algorithm;
- fixed replica counts;
- fixed shard counts;
- fixed quorum sizes;
- implementation-specific capacity-allocation policy;
- proprietary cognitive placement policy;
- exact device-selection heuristics;
- storage-vendor-specific behavior;
- production operator procedures;
- ACS transport mechanics;
- exact repair execution algorithms.

An implementation may use:

- full replicas;
- partitioned representations;
- reconstruction coding;
- archival media;
- local disks;
- network storage;
- volatile memory;
- remote custodians;
- heterogeneous devices;
- combinations of these mechanisms.

It must preserve the semantic distinctions and evidence requirements established here.

## 1. Purpose

A logical memory is not a disk block, file, database row, storage device, process, or physical node.

Nevertheless, logical memory depends on physical material.

That material must exist somewhere.

When memory is distributed, Node must know:

- which participants are expected to retain material;
- which participants actually retain it;
- which failure domains contain the material;
- whether the available material can reconstruct the memory;
- whether required metadata and keys are protected;
- whether placement satisfies the applicable durability target;
- whether locality goals are satisfied;
- whether migration is complete;
- whether a departing custodian can be safely released;
- whether apparently numerous copies share one correlated failure;
- whether capacity pressure has silently weakened protection.

Without explicit custody semantics, an implementation may incorrectly assume that:

- three copies on one machine provide three-machine protection;
- two disks behind one controller are independent;
- two services using the same backing storage are independent custodians;
- a copy observed last week is still present;
- a successful write to one node satisfies durability;
- a cache is a durable replica;
- a reconstruction shard is independently useful;
- additional copies always improve resilience;
- the nearest copy is always the safest copy;
- migration is complete when transfer ends;
- an offline node may be removed from protection accounting immediately;
- replication may grow without resource bounds;
- a full storage device may discard protected memory;
- physical placement determines memory identity.

MEM-0007 establishes a custody model that keeps physical reality visible without allowing physical placement to redefine memory meaning.

## 2. Scope

This specification governs custody and placement for:

- logical memory versions;
- memory representations;
- physical copies;
- reconstruction shards;
- manifests;
- catalogs;
- indexes where required for recovery;
- schemas;
- provenance;
- lineage metadata;
- lifecycle metadata;
- tombstones;
- commit evidence;
- operation-history evidence;
- cryptographic dependencies;
- recovery metadata.

It applies to:

- active memory;
- archived memory;
- continuity-critical memory;
- short-retained governed memory;
- long-retained memory;
- provisional memory where custody is permitted;
- repaired material;
- reconstructed material;
- migrating material;
- material retained during partition.

It governs operations including:

- initial placement;
- replica creation;
- shard creation;
- custody assignment;
- custody acceptance;
- custody verification;
- migration;
- rebalance;
- evacuation;
- repair placement;
- custodian retirement;
- capacity-driven placement change;
- failure-domain reassessment;
- protection-state reporting.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements.

The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification.

The word **may** describes permitted behavior.

A deployment does not conform merely because it stores several copies.

It must be able to demonstrate the custody, independence, reconstructability, and protection properties it claims.

## 4. Foundational distinctions

### 4.1 Logical memory is not custody

Logical memory identifies governed state.

Custody identifies responsibility for physical material supporting that state.

Changing custody does not inherently change logical identity.

### 4.2 Custody is not authority

A participant may retain memory material without authority to:

- interpret it;
- disclose it;
- mutate it;
- delete it;
- change its lifecycle;
- declare it authoritative.

### 4.3 Custody assignment is not physical presence

A participant may be assigned custody before transfer completes.

A participant may retain material after its custody assignment is revoked.

Assignment and observed physical state must remain distinct.

### 4.4 Physical presence is not validated custody

Bytes may exist on a device without satisfying:

- integrity;
- schema;
- version;
- authorization;
- lifecycle;
- reconstruction;
- durability requirements.

### 4.5 Replica count is not failure tolerance

Several copies exposed to one correlated failure may provide little additional protection.

### 4.6 Failure-domain diversity is not geographic distance alone

Two distant copies may still depend on one:

- administrative authority;
- encryption key;
- catalog;
- power source;
- software defect;
- storage service;
- account;
- network path;
- controller.

### 4.7 Locality is not authority

A nearby copy may improve latency.

It does not become authoritative because it is nearby.

### 4.8 Placement preference is not a durability guarantee

A desired location or topology is not satisfied until evidence establishes actual compliant placement.

### 4.9 Replication is not commitment

Creating a copy does not independently commit a memory version.

### 4.10 Commitment is not sufficient custody

A committed version may remain under-protected until its required physical material and dependencies are placed.

### 4.11 Transfer completion is not migration completion

Migration is complete only after required validation, catalog updates, protection checks, and retirement conditions are satisfied.

### 4.12 Reconstruction coding is not ordinary replication

A reconstruction shard may be insufficient by itself to provide a readable memory representation.

### 4.13 Availability is not independent protection

Several readable copies may all disappear in one failure.

### 4.14 Under-protected is not lost

A memory may still be readable while failing its required protection target.

### 4.15 Over-placement is not automatically waste

Additional copies may temporarily be required during:

- migration;
- repair;
- partition;
- evacuation;
- policy transition.

They must still remain bounded and observable.

### 4.16 Custodian departure is not deletion

Removing one custodian assignment does not delete the logical memory.

## 5. Custody model

### 5.1 Custodian

A custodian is an assigned memory-service role responsible for retaining or serving physical memory material.

A custodian may hold:

- complete representations;
- partial representations;
- reconstruction shards;
- manifests;
- integrity evidence;
- lifecycle evidence;
- recovery dependencies.

### 5.2 Custody assignment

A custody assignment is a governed responsibility requiring an identified participant to retain defined physical material under declared conditions.

A custody assignment should identify:

- assignment identity;
- custodian identity;
- logical memory identity;
- version identity;
- representation or shard identity;
- retention obligations;
- integrity requirements;
- availability expectations;
- failure-domain claims;
- resource limits;
- applicable lifecycle policy;
- permitted operations;
- assignment generation;
- activation state;
- release conditions.

### 5.3 Custody proposal

A custody proposal requests that a participant accept responsibility for defined material.

Proposal does not mean acceptance.

### 5.4 Custody acceptance

Custody is accepted when the participant confirms that it is prepared to satisfy the assignment.

Acceptance does not prove that all material has been received or validated.

### 5.5 Custody established

Custody is established when required material is:

- transferred or created;
- identified;
- validated;
- associated with the assignment;
- reportable;
- available under the assignment's declared conditions.

### 5.6 Custody verified

Custody is verified when sufficiently current evidence establishes that the participant continues to satisfy the assignment.

### 5.7 Custody degraded

Custody is degraded when the participant still satisfies part of its assignment but fails one or more declared requirements.

Examples include:

- increased retrieval latency;
- loss of preferred locality;
- reduced validation;
- capacity pressure;
- stale integrity evidence;
- reduced availability window.

### 5.8 Custody unavailable

Custody is unavailable when required physical material or service cannot currently be used under the assignment.

### 5.9 Custody unknown

Custody is unknown when evidence is insufficient to establish whether the assignment remains satisfied.

### 5.10 Custody released

Custody is released when the participant is no longer required to retain the assigned material and all release preconditions have been satisfied.

Release does not independently prove physical reclamation.

### 5.11 Custody revoked

An assignment may be revoked because of:

- policy change;
- compromise;
- failure;
- lifecycle transition;
- migration;
- resource reassignment.

Revocation does not authorize the custodian to ignore deletion, hold, audit, or evidence-preservation requirements.

## 6. Conceptual custody records

These records define semantics, not mandatory wire or database formats.

### 6.1 Custody assignment record

```text
CustodyAssignment
    custody_assignment_identity
    assignment_generation
    custodian_identity
    logical_memory_identity
    version_identity
    representation_or_shard_identity
    custody_state
    retention_requirement
    availability_requirement
    integrity_requirement
    placement_policy_reference
    claimed_failure_domains
    permitted_operations
    resource_limit
    activation_boundary
    release_conditions
    governing_operation_identity
```

### 6.2 Placement requirement record

```text
PlacementRequirement
    placement_policy_identity
    subject_scope
    required_protection_target
    required_failure_domain_diversity
    required_representation_types
    locality_constraints
    locality_preferences
    capacity_constraints
    security_constraints
    lifecycle_constraints
    permissible_degradation
    repair_priority
```

### 6.3 Placement observation record

```text
PlacementObservation
    subject_identity
    observed_physical_material
    observed_custodians
    observed_failure_domains
    validation_state
    observation_time
    evidence_age
    satisfied_requirements
    unmet_requirements
    unknown_requirements
    protection_state
```

### 6.4 Shard-set record

```text
ReconstructionSet
    reconstruction_set_identity
    logical_memory_identity
    version_identity
    representation_identity
    coding_profile_identity
    shard_identities
    reconstruction_threshold
    required_manifest
    required_schema
    required_key_dependencies
    validation_requirements
```

### 6.5 Migration record

```text
CustodyMigration
    migration_operation_identity
    subject_identity
    source_assignments
    target_assignments
    migration_reason
    required_protection_during_migration
    copied_or_reconstructed_material
    validation_state
    catalog_update_state
    source_release_state
    final_protection_state
```

Equivalent information may be distributed among:

- catalogs;
- custodians;
- placement coordinators;
- repair services;
- immutable operation histories.

Its required meaning must remain recoverable.

## 7. Physical material types

### 7.1 Complete replica

A complete replica contains one complete usable representation of a memory version, subject to required dependencies.

A complete replica may be:

- directly readable;
- encrypted;
- compressed;
- archived;
- staged;
- device-specific.

### 7.2 Representation replica

A representation replica is a copy of one particular representation.

Several representation replicas may support one memory version.

### 7.3 Reconstruction shard

A reconstruction shard contains part of a coded representation.

A shard may be unusable alone.

Its value depends on:

- the coding profile;
- threshold;
- manifest;
- compatible shards;
- schema;
- integrity;
- key dependencies.

### 7.4 Manifest

A manifest describes how physical material relates to:

- logical memory;
- version;
- representation;
- shard set;
- integrity evidence;
- reconstruction requirements.

A shard set without a durable usable manifest may not provide meaningful protection.

### 7.5 Metadata replica

Metadata necessary to identify, locate, validate, reconstruct, decrypt, or govern memory is itself protected physical material.

### 7.6 Cache copy

A cache copy is retained primarily for performance.

It is not counted toward required durability unless explicitly assigned and verified as durable custody.

### 7.7 Temporary transfer copy

A temporary transfer copy exists during movement or staging.

It is not counted as independent durable custody unless it satisfies the applicable assignment and failure-domain requirements.

### 7.8 Backup or checkpoint copy

A backup may contribute to protection when:

- its identity is known;
- its lifecycle state is current enough;
- its restoration path is validated;
- its failure-domain exposure is understood;
- newer tombstones and lifecycle events can be reconciled.

## 8. Failure domains

### 8.1 Definition

A failure domain is a set of physical material or services exposed to one correlated failure.

Failure domains may overlap.

### 8.2 Failure-domain classes

Implementations should be capable of modeling relevant classes such as:

- process;
- host;
- storage device;
- controller;
- chassis;
- power supply;
- power circuit;
- rack;
- site;
- network segment;
- storage service;
- administrative account;
- software version;
- deployment generation;
- key authority;
- catalog authority;
- operator domain.

Not every deployment must model every class.

It must model the classes material to its durability claims.

### 8.3 Nested failure domains

Failure domains may be nested.

For example:

- devices exist within hosts;
- hosts exist within racks;
- racks exist within sites.

Protection across devices does not prove protection across hosts or sites.

### 8.4 Overlapping failure domains

Two custodians may be independent by host but correlated by:

- shared power;
- shared storage;
- shared keys;
- shared software defect;
- shared administrative authority.

### 8.5 Unknown failure domain

When the system cannot establish whether custodians are independent, independence must remain unknown.

Unknown independence must not be counted as confirmed diversity.

### 8.6 Dynamic failure domains

Failure-domain relationships may change because of:

- virtualization migration;
- storage remapping;
- network reconfiguration;
- power changes;
- shared-service changes;
- software rollout;
- administrative reassignment.

Protection must be reassessed when relevant domain evidence changes.

### 8.7 Logical labels require evidence

A configured label such as `zone-a` or `rack-2` is not sufficient by itself.

The system should preserve evidence or authority supporting the failure-domain claim.

### 8.8 Common dependencies participate in failure analysis

Durability accounting must include dependencies required to recover usable memory, including:

- keys;
- schemas;
- manifests;
- identity catalogs;
- lifecycle records;
- reconstruction metadata.

## 9. Protection targets

### 9.1 Protection target

A protection target declares the failures or loss conditions memory should survive.

A target is more meaningful than a raw replica count.

### 9.2 Target scope

A protection target must identify:

- memory or memory class;
- required representation;
- required dependencies;
- applicable failure domains;
- required reconstruction capability;
- required availability or recovery condition;
- permissible degradation.

### 9.3 Example target categories

Implementations may define profiles such as:

- single-custodian transient;
- host-loss tolerant;
- device-loss tolerant;
- rack-loss tolerant;
- site-loss tolerant;
- administrative-domain tolerant;
- offline archival recovery;
- continuity-critical multi-domain protection.

These names are examples, not mandatory profiles.

### 9.4 Protection target versus observation

The target describes what should be true.

The observation describes what current evidence establishes.

They must remain separate.

### 9.5 Protection satisfied

Protection is satisfied when evidence establishes that sufficient valid material and dependencies exist across the required independent domains.

### 9.6 Under-protected

Memory is under-protected when:

- it remains usable;
- but current evidence does not satisfy the required protection target.

### 9.7 Protection unknown

Protection is unknown when required custody or domain evidence is unavailable or stale.

### 9.8 Protection violated

Protection is violated when evidence establishes that required survival conditions cannot currently be met.

An implementation may use under-protected and violated as separate severities.

### 9.9 Protection impossible

A requested target may be impossible under current resources or topology.

The operation must report the unmet target rather than falsely claim success.

### 9.10 Protection claims are time-bound

A protection claim applies to an observation boundary.

It must not remain permanently valid without renewed evidence.

## 10. Replica and shard accounting

### 10.1 Valid material only

Protection accounting may count only material satisfying required:

- identity;
- version;
- integrity;
- lifecycle;
- representation;
- schema;
- key;
- custody conditions.

### 10.2 Duplicate physical references

Two catalog entries pointing to the same underlying physical material count as one physical instance.

### 10.3 Shared storage

Two custodians backed by the same physical storage do not provide independent storage protection.

### 10.4 Shared origin

Several replicas created from one corrupt source may all contain identical corruption.

Copy count does not replace validation.

### 10.5 Shard independence

Reconstruction shards count toward protection only when their failure-domain placement and coding relationships satisfy the reconstruction target.

### 10.6 Reconstruction threshold

A shard-set protection claim must account for:

- total valid shards;
- required threshold;
- unavailable shards;
- unknown shards;
- correlated placement;
- manifest availability;
- required keys and schemas.

### 10.7 Excess replicas

Extra replicas may be retained temporarily or permanently when policy permits.

They must remain:

- identifiable;
- bounded;
- lifecycle-governed;
- capacity-accounted.

### 10.8 Invalid copies

Corrupt, stale, deleted, unauthorized, or unverified copies must not be counted as valid protection unless the applicable policy explicitly permits their limited use.

## 11. Placement requirements

### 11.1 Hard constraint

A hard placement constraint must be satisfied before the placement operation may claim success.

Examples include:

- required jurisdiction or administrative scope;
- required encryption support;
- exclusion of one failure domain;
- minimum available capacity;
- continuity-critical separation;
- forbidden custodian;
- required archival medium.

### 11.2 Preference

A placement preference improves utility but may be relaxed when policy permits.

Examples include:

- low retrieval latency;
- proximity to likely consumers;
- lower energy cost;
- preferred device type;
- reduced network traffic.

### 11.3 Constraint precedence

Protection, lifecycle, authorization, and integrity constraints must not be silently weakened to satisfy locality or cost preferences.

### 11.4 Unsatisfied constraint

An unsatisfied hard constraint must produce:

- rejection;
- deferral;
- under-protected state;
- another explicit non-success outcome.

### 11.5 Conflicting constraints

Conflicting placement constraints must remain explicit until resolved by applicable authority or policy.

### 11.6 Bounded placement search

Placement evaluation must operate within explicit resource and topology-search bounds.

Failure to examine every possible placement must not be represented as proof that no valid placement exists.

## 12. Locality

### 12.1 Locality is multidimensional

Locality may describe proximity by:

- process;
- host;
- device;
- network latency;
- bandwidth;
- administrative boundary;
- geographic site;
- consumer;
- compute accelerator;
- archival tier.

### 12.2 Locality objective

A locality objective declares where memory access is preferred or expected.

### 12.3 Locality constraint

A locality constraint declares where memory must or must not be placed.

### 12.4 Read locality

Read locality may reduce:

- latency;
- bandwidth;
- energy;
- dependency fan-out.

### 12.5 Write locality

Write locality may improve admission and commit speed but must not silently weaken required independent protection.

### 12.6 Compute locality

Memory may be placed near compute likely to use it.

The compute participant does not gain memory authority through proximity.

### 12.7 Locality versus resilience

Placing every copy near one consumer may increase correlated-loss risk.

The placement result must disclose any protection tradeoff.

### 12.8 Local-only custody

A local-only custody profile may be valid for explicitly transient or low-protection memory.

It must not be applied silently to memory requiring distributed survival.

### 12.9 Local cache

A local cache does not become required custody unless explicitly assigned.

## 13. Initial placement

### 13.1 Placement follows admission

A storage proposal may request placement.

Admission and commitment remain distinct from physical placement completion.

### 13.2 Initial assignment

Initial placement should identify:

- required protection target;
- candidate custodians;
- representation strategy;
- failure-domain requirements;
- locality goals;
- resource budget;
- completion criteria.

### 13.3 Placement pending

A memory may be committed while physical placement remains pending only when the applicable commit and durability policy permits that state.

The memory must be reported as under-protected or pending protection as appropriate.

### 13.4 Placement completion

Initial placement completes only when:

- required assignments are established;
- material is validated;
- required dependencies are protected;
- catalogs reflect current custody;
- target protection is verified.

### 13.5 Failed initial placement

Failure to satisfy requested placement must not silently downgrade the target.

## 14. Replication and shard creation

### 14.1 Replication operation identity

Each governed replication or shard-creation operation must carry stable operation identity.

### 14.2 Source validation

A source should be validated sufficiently before it is used to create new protection material.

### 14.3 Target validation

Created material must be validated before counting toward protection.

### 14.4 Duplicate-safe behavior

Retrying replication must not create uncontrolled extra copies.

### 14.5 Resource accounting

Replication and shard creation must account for:

- storage;
- bandwidth;
- compute;
- energy;
- verification work;
- catalog updates.

### 14.6 Partial replication

A partially completed replication operation must identify:

- completed targets;
- incomplete targets;
- validated material;
- temporary material;
- achieved protection.

### 14.7 Representation choice

A replica operation may create:

- an identical representation;
- a semantically equivalent representation;
- reconstruction shards.

The identity and equivalence rules of MEM-0003 apply.

## 15. Migration

### 15.1 Migration purpose

Migration moves custody while preserving logical memory identity and required protection.

### 15.2 Migration triggers

Migration may be triggered by:

- custodian retirement;
- capacity pressure;
- hardware health;
- policy change;
- locality change;
- failure-domain reassessment;
- lifecycle transition;
- security concern;
- balancing;
- archival.

### 15.3 Copy-before-release

When protection requires uninterrupted custody, target material must be established and validated before source custody is released.

### 15.4 Reduced-protection migration

A policy may permit temporary reduced protection during migration.

The reduced state must be explicit and bounded.

### 15.5 Migration phases

Migration should distinguish:

- planned;
- target assigned;
- transfer or reconstruction active;
- target validated;
- catalog reconciled;
- protection verified;
- source release authorized;
- source reclaimed;
- completed;
- failed;
- indeterminate.

### 15.6 Source remains authoritative only by role

The source does not retain logical authority merely because it held the original copy.

### 15.7 Migration conflict

If source and target contain incompatible material, migration must stop or remain conflicting until governed resolution occurs.

### 15.8 Migration cancellation

Cancellation must identify:

- which target copies were created;
- which source assignments remain;
- whether temporary material requires reclamation;
- current protection state.

## 16. Rebalancing

### 16.1 Definition

Rebalancing changes placement to improve compliance, distribution, capacity, or locality without changing logical memory meaning.

### 16.2 Rebalancing goals

Goals may include:

- reducing custodian hotspots;
- restoring failure-domain diversity;
- improving locality;
- evacuating unhealthy devices;
- reducing over-placement;
- satisfying new policy;
- preparing for maintenance.

### 16.3 Rebalancing is bounded

Rebalancing must have limits on:

- concurrent migrations;
- bandwidth;
- storage amplification;
- repair interference;
- energy;
- queue depth.

### 16.4 Repair precedence

Policy should define whether repair of under-protected memory takes precedence over optimization-oriented rebalancing.

### 16.5 Stability

Placement logic should avoid uncontrolled oscillation in which material repeatedly moves between equally acceptable locations.

Exact stabilization algorithms are implementation-defined.

### 16.6 Rebalance evidence

A rebalance result must report achieved rather than intended placement.

## 17. Custodian joining

### 17.1 Discovery is not eligibility

A newly discovered participant is not automatically eligible for custody.

### 17.2 Eligibility evaluation

Custody eligibility may require:

- authenticated identity;
- role assignment;
- storage capability;
- health evidence;
- supported schemas or representations;
- security state;
- failure-domain claims;
- capacity;
- lifecycle support;
- repair support.

### 17.3 Joining does not create immediate diversity

A new participant sharing existing failure domains may add capacity without adding independent protection.

### 17.4 Initial trust

A participant may require limited or staged assignments before receiving more sensitive or continuity-critical custody.

The exact policy is implementation-defined.

## 18. Custodian departure and evacuation

### 18.1 Planned departure

A planned departure must identify material whose protection depends on the departing custodian.

### 18.2 Safe release

A custodian may be safely released only when:

- replacement protection is established; or
- policy explicitly accepts the resulting reduction; or
- the memory lifecycle permits removal.

### 18.3 Forced departure

Unexpected failure may immediately reduce protection.

The system must report the resulting state and trigger repair according to policy.

### 18.4 Evacuation

Evacuation prioritizes movement away from a custodian or failure domain at elevated risk.

### 18.5 Departure does not authorize deletion

A departing custodian must still honor lifecycle and evidence obligations.

### 18.6 Unknown retained material

If a custodian departs without confirmed cleanup, remaining material state must be reported unknown or pending rather than assumed reclaimed.

## 19. Under-protection

### 19.1 Definition

Memory is under-protected when current verified custody does not satisfy its required protection target.

### 19.2 Causes

Under-protection may result from:

- custodian failure;
- device failure;
- partition;
- expired evidence;
- corrupted copy;
- invalid shard;
- lost key;
- lost manifest;
- capacity pressure;
- migration;
- changed failure-domain mapping;
- new policy requirement;
- deletion of one copy;
- delayed repair.

### 19.3 Severity

Implementations may classify under-protection by:

- remaining reconstruction margin;
- affected failure domains;
- memory class;
- time exposed;
- repair feasibility;
- current availability.

### 19.4 Reporting

Under-protection must identify, where possible:

- required target;
- observed valid material;
- missing domains;
- missing dependencies;
- repair state;
- availability impact;
- uncertainty.

### 19.5 Admission behavior

Policy may restrict new memory admission while important existing memory is under-protected.

### 19.6 Mutation behavior

A mutation may be deferred when committing a new version would worsen unacceptable protection.

### 19.7 Under-protection is not hidden by overcounting

Caches, temporary copies, duplicate references, and correlated copies must not be used to falsely satisfy protection.

## 20. Over-placement and over-budget state

### 20.1 Over-placement

Memory is over-placed when physical custody exceeds its declared ordinary target or budget.

### 20.2 Legitimate temporary over-placement

Temporary excess may occur during:

- migration;
- repair;
- evacuation;
- policy transition;
- partition reconciliation.

### 20.3 Over-budget

A placement operation is over-budget when it consumes more assigned storage, bandwidth, compute, or energy than permitted.

### 20.4 Over-placement does not authorize arbitrary deletion

Excess material may be reclaimed only through governed copy-release and lifecycle rules.

### 20.5 Selection for removal

Removal should preserve:

- target protection;
- failure-domain diversity;
- locality requirements;
- lifecycle constraints;
- repair capability.

### 20.6 Unknown extra copies

Untracked or unexpected copies must not be silently ignored.

They may create:

- security risk;
- lifecycle risk;
- deletion-propagation risk;
- false durability assumptions.

## 21. Capacity pressure

### 21.1 Capacity is finite

Custodians must report relevant capacity and pressure state.

### 21.2 Pressure responses

Capacity pressure may cause:

- rejection of new custody;
- placement deferral;
- migration;
- archival;
- rebalancing;
- cache eviction;
- copy-release proposals;
- lifecycle review.

### 21.3 Silent eviction prohibited

A custodian must not silently discard assigned durable material because local capacity is low.

### 21.4 Emergency loss

If physical failure or implementation defect causes loss, the event must be reported as loss or custody failure.

It must not be relabeled as successful rebalancing.

### 21.5 Reservation

Implementations may reserve capacity for:

- repair;
- migration;
- continuity-critical memory;
- metadata;
- tombstones.

Exact reservation policy is outside this public specification.

## 22. Partition behavior

### 22.1 Partition affects evidence

A partition may make custody:

- unavailable;
- unknown;
- stale in observation;
- apparently conflicting.

### 22.2 Unreachable is not absent

An unreachable custodian must not be immediately removed from physical-existence history.

### 22.3 Protection during partition

Protection calculations must distinguish:

- confirmed available material;
- confirmed retained but unreachable material;
- unknown material;
- confirmed lost material.

### 22.4 Partitioned placement

New placement during partition may create temporary over-placement or conflicting assignments.

The resulting state must be reconciled.

### 22.5 Partitioned release

Custody release should not be assumed complete when required participants cannot confirm current state.

### 22.6 Rejoining custodian

A rejoining custodian must reconcile:

- assignment generation;
- lifecycle state;
- tombstones;
- version state;
- integrity;
- current placement policy.

## 23. Placement conflict

### 23.1 Conflict sources

Placement conflict may involve:

- incompatible custody assignments;
- duplicate assignment generations;
- contradictory failure-domain claims;
- catalog disagreement;
- conflicting lifecycle state;
- incompatible shard manifests;
- disputed ownership of physical material.

### 23.2 Conflict visibility

Conflict must remain explicit until resolved.

### 23.3 Conservative accounting

Conflicting material should not be counted toward required protection unless policy establishes that it is safe to do so.

### 23.4 Conflict resolution

Resolution must preserve:

- evidence;
- involved assignments;
- selected outcome;
- rejected claims;
- remaining uncertainty.

## 24. Metadata and dependency custody

### 24.1 Metadata is part of survivability

Content protection is incomplete if Node loses required:

- identity metadata;
- lineage;
- schema;
- manifests;
- lifecycle state;
- tombstones;
- commit evidence;
- key authority.

### 24.2 Independent metadata protection

Metadata may require protection separate from content to avoid one failure destroying both data and its interpretation path.

### 24.3 Catalog dependency

A global or regional catalog must not become an unacknowledged single point of memory loss.

### 24.4 Key dependency

Copies encrypted under one unavailable or destroyed key authority may all become unusable simultaneously.

### 24.5 Software dependency

A representation requiring one unsupported software generation may be under-protected even when many bytes remain.

### 24.6 Dependency migration

Schema, key, manifest, and catalog migrations must preserve the recoverability of retained memory.

## 25. Security and administrative locality

### 25.1 Custody disclosure

Placement and custodian information may reveal:

- topology;
- memory existence;
- failure domains;
- security boundaries;
- repair priorities.

Disclosure must remain authorized.

### 25.2 Custody permission

Permission to hold material does not automatically grant permission to read plaintext.

### 25.3 Administrative separation

Independent administrative control may be part of a protection target.

### 25.4 Compromise containment

Compromise of one custodian should not grant authority over unrelated custodians or global lifecycle state.

### 25.5 Placement restriction

Security policy may restrict placement according to:

- trust domain;
- key support;
- jurisdiction;
- hardware capability;
- isolation level.

### 25.6 Revoked custodian

A revoked custodian must not continue serving or repairing material as authoritative custody.

Physical cleanup may remain pending and explicit.

## 26. Role boundaries

### 26.1 Proposer

May request durability and locality characteristics.

It does not independently choose authoritative physical placement.

### 26.2 Admission evaluator

Determines whether requested custody and protection are feasible under current policy and resources.

### 26.3 Commit authority

May commit memory while recording actual protection state according to applicable policy.

It must not claim unsupported durability.

### 26.4 Custodian

Retains and serves assigned physical material.

### 26.5 Catalog and locator

Records known custody and location evidence.

It does not create physical custody merely by recording it.

### 26.6 Validator

Evaluates:

- integrity;
- representation identity;
- shard compatibility;
- failure-domain evidence;
- placement compliance;
- reconstruction feasibility.

### 26.7 Repair coordinator

Restores missing protection and selects eligible target custody according to policy.

### 26.8 Lifecycle authority

Governs copy release, deletion, archival, and reclamation eligibility.

### 26.9 Observer and auditor

Reports placement and protection evidence without gaining mutation or deletion authority.

### 26.10 Runtime

May schedule and migrate implementations.

Runtime placement does not independently establish memory custody authority.

## 27. Boundary with ACS

ACS governs:

- relationships;
- endpoint identity;
- port exposure;
- signal delivery;
- payload references;
- routing;
- communication admission;
- secure sessions;
- connection health;
- communication backpressure.

MEM governs:

- custody assignment;
- physical-material identity;
- placement requirements;
- failure-domain accounting;
- replication meaning;
- shard-set meaning;
- migration completion;
- protection state;
- under-protection;
- locality semantics.

The following rules apply:

1. ACS connectivity does not establish custody.
2. ACS disconnection does not prove custody loss.
3. A payload reference is not a custody assignment.
4. Successful transfer does not prove validated custody.
5. Communication retries preserve the same placement-operation identity when semantic intent is unchanged.
6. ACS backpressure may delay placement but does not redefine the protection target.
7. Endpoint locality does not prove storage locality.
8. Routing diversity does not prove failure-domain diversity.
9. MEM does not redefine ACS routing or transport.
10. Several ACS exchanges may participate in one custody or migration operation.

## 28. Boundary with lifecycle

MEM-0006 governs whether memory or physical material may be:

- archived;
- deleted;
- released;
- reclaimed;
- sanitized.

MEM-0007 governs where retained material is held and whether current custody satisfies protection.

The following distinctions apply:

- releasing one custody assignment is not logical deletion;
- deleting logical memory requires propagation to relevant custodians;
- reclaiming an excess copy must preserve required protection;
- a tombstone is itself custody-relevant protected metadata;
- archived placement remains governed custody;
- lifecycle conflict may block placement or release.

## 29. Boundary with recovery

MEM-0007 defines:

- what material exists;
- where it is expected;
- which domains protect it;
- what protection is missing.

MEM-0008 defines:

- how damaged or missing protection is repaired;
- how material is reconstructed;
- how recovered state is validated and reintegrated.

Placement may trigger repair.

Placement does not independently define reconstruction correctness.

## 30. Conformance expectations

A conforming implementation must demonstrate:

- logical identity independent of physical placement;
- explicit custody assignment;
- assignment versus observed-presence separation;
- complete-replica versus shard distinction;
- cache exclusion from durability accounting by default;
- failure-domain-aware protection;
- overlapping-domain awareness;
- dependency-aware durability;
- requested-versus-observed placement;
- explicit under-protection;
- locality-versus-resilience tradeoff reporting;
- bounded replication;
- duplicate-safe migration;
- copy-before-release behavior where required;
- safe custodian departure;
- partitioned-custody uncertainty;
- rejoining-custodian reconciliation;
- capacity-pressure behavior;
- over-placement handling;
- catalog disagreement handling;
- deletion and tombstone placement awareness;
- protection-claim expiration.

Detailed failure validation belongs to MEM-0010.

## 31. Prohibited interpretations

This specification shall not be interpreted to mean that:

- memory identity is its storage path;
- custody grants mutation authority;
- custody grants deletion authority;
- assigned custody proves physical presence;
- physical presence proves valid custody;
- two copies always provide two independent protections;
- two devices in one host necessarily provide host-loss protection;
- two services necessarily use independent storage;
- geographic distance proves all-domain independence;
- replica majority automatically defines authority;
- a cache automatically counts as durability;
- a temporary transfer copy automatically counts as durability;
- a shard is equivalent to a full replica;
- transfer completion proves migration completion;
- the nearest copy is necessarily current;
- locality may silently weaken resilience;
- storage pressure permits silent eviction;
- custodian failure proves logical memory loss;
- custodian departure means deletion;
- extra copies may grow without bound;
- an unknown failure domain may be treated as independent;
- configured topology labels are unquestionable evidence;
- content durability can ignore keys, schemas, manifests, or tombstones;
- public placement rules disclose proprietary cognitive allocation policy.

## 32. Open questions

The following questions remain for later specifications and implementation profiles:

- Which failure-domain classes are mandatory for baseline conformance?
- Which protection profiles should the public implementation expose?
- Which memory classes require protection beyond host loss?
- When should full replication be preferred over reconstruction coding?
- Which reconstruction-coding metadata must itself be fully replicated?
- How should unknown failure-domain relationships affect admission?
- Which placement observations require durable history?
- How long may protection evidence remain valid?
- Which locality dimensions should be represented by shared types?
- How should compute locality interact with accelerator scheduling?
- Which migration stages require durable operation history?
- How should assignment generations be reconciled after partition?
- When may a source be released before target catalog convergence?
- How much temporary over-placement is permissible during repair?
- Which memory classes receive priority under repair-capacity pressure?
- How should administrative independence be verified?
- How should software-version correlation be represented as a failure domain?
- Which key-authority failures should immediately mark memory under-protected?
- Which low-level custody, placement, failure-domain, and protection-state structures should become shared C++ interfaces?
- How should implementation-specific placement extensions declare additional constraints without changing public custody meaning?

These questions do not weaken the custody and protection distinctions already established.

## 33. Closing principle

> **Node must know not merely how many copies exist, but which failures those copies can survive together.**

Identity is not placement.

Custody is not authority.

Copy count is not independence.

Locality is not resilience.

Transfer is not migration completion.

Availability is not protection.

A memory remains protected only when valid material, dependencies, and evidence survive across the failure domains its policy requires.

## Revision history

### Version 0.1 — 2026-07-16

- Replaced the planned MEM-0007 stub with the first normative working draft.
- Defined custody assignments, custody states, physical material types, and placement evidence.
- Distinguished complete replicas, representation replicas, reconstruction shards, caches, transfer copies, and backups.
- Defined failure domains, overlapping correlation, nested domains, dynamic domains, and unknown independence.
- Established protection targets independently of raw copy count.
- Defined dependency-aware replica and shard accounting.
- Defined hard placement constraints, preferences, and locality semantics.
- Established initial placement, replication, migration, rebalancing, joining, departure, and evacuation behavior.
- Defined under-protection, over-placement, over-budget, and capacity-pressure states.
- Established partition, conflict, metadata, key, schema, security, lifecycle, recovery, role, and ACS boundaries.
- Added implementation-facing custody and placement conformance expectations.
