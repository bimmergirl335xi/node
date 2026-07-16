# MEM-0003: Logical Identity, Provenance, and Versioning

| Field | Value |
|---|---|
| Specification | MEM-0003 |
| Title | Logical Identity, Provenance, and Versioning |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Accepted as initial working draft |
| Depends on | MEM-0000, MEM-0001, MEM-0002 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in core distinctions; version branching, equivalence evidence, and identity-merge policy remain under review |

> **A memory remains identifiable through relocation and revision because logical identity, version lineage, provenance, and physical representation are separate governed concepts.**

## Architectural-intent notice

This specification defines how Node identifies memories, distinguishes their versions, records their origins, relates derived state, and preserves continuity across physical movement and representation change.

It defines the architectural meaning of:

- logical memory identity;
- version identity;
- representation identity;
- physical-copy identity;
- schema identity;
- aliases and locators;
- provenance;
- lineage;
- derivation;
- revision;
- branching;
- merging;
- supersession;
- staleness;
- conflict;
- invalidation;
- representation equivalence.

It does not define:

- one universal identifier encoding;
- one universal identifier length;
- one mandatory hashing algorithm;
- one serialization format;
- one database schema;
- one physical storage layout;
- one catalog implementation;
- proprietary indexing;
- latent reconstruction;
- non-public cognitive association policy;
- ACS payload-reference formats;
- exact retention and deletion procedures;
- exact commit or consistency protocols.

Implementations may select different identifier encodings, storage engines, schema systems, and representation formats.

They must preserve the semantic distinctions established here.

## 1. Purpose

MEM-0003 prevents physical storage details and implementation shortcuts from becoming the identity model of Node memory.

Without an explicit identity and lineage model, an implementation may incorrectly assume that:

- a file path is the memory;
- a database row is the memory;
- a payload reference is the memory;
- a content hash is always the memory;
- moving bytes creates a new memory;
- identical bytes prove identical logical meaning;
- changing encoding creates a new semantic version;
- a timestamp establishes authoritative order;
- the newest responding replica is the newest valid version;
- a reconstructed object is necessarily identical to the original;
- missing provenance means no provenance existed;
- a deleted identifier may safely be reused;
- one catalog entry defines all memory truth.

Those assumptions can cause:

- memory identity to change during migration;
- duplicate memories to be created by retries;
- distinct memories to be incorrectly merged;
- stale state to overwrite newer state;
- corrupted or unverified lineage to appear authoritative;
- recovery to lose the ability to distinguish originals, transformations, and reconstructions;
- deletion to be reversed by rejoining copies;
- provenance to disappear during format conversion;
- low-level storage implementation to silently redefine cognitive continuity.

This specification establishes enough identity and history to keep those failures visible and recoverable.

## 2. Scope

This specification governs:

- creation of logical memory identity;
- identifier scope and uniqueness;
- identifier persistence and non-reuse;
- identity resolution;
- aliases and human-readable names;
- version creation;
- version lineage;
- concurrent versions;
- derivation across memories;
- representation identity;
- physical-copy identity;
- schema identity;
- provenance claims;
- provenance evidence;
- transformation history;
- reconstruction history;
- conflict representation;
- stale and superseded versions;
- invalidation;
- representation equivalence;
- identity behavior during migration and recovery.

It applies to:

- transient governed memory;
- working memory that receives logical identity;
- retained short-term memory;
- long-term memory;
- episodic memory;
- semantic memory;
- procedural memory;
- continuity-critical state;
- journals;
- checkpoints;
- derived memories;
- reconstructed memories;
- metadata required to interpret or recover memory.

Not every transient value requires a logical memory identity.

Once state is admitted as governed memory, its identity and history must follow the applicable requirements of this specification.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements.

The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification.

The word **may** describes permitted behavior.

A concept may be implemented through one record, several records, or computed state.

Conformance depends on preserved meaning and observable behavior, not on field names or storage layout.

## 4. Identity layers

Node memory uses several distinct identity layers.

An implementation may encode them together for efficiency, but it must not erase their semantic distinction.

### 4.1 Logical memory identity

A **logical memory identifier** identifies one continuing governed memory.

It answers:

> Which memory is this?

Logical memory identity remains stable across:

- physical relocation;
- replication;
- cache creation;
- storage-tier migration;
- re-encoding;
- compression;
- encryption changes;
- reconstruction;
- process restart;
- service migration;
- custodian replacement.

A logical memory may possess many versions and many physical representations.

### 4.2 Version identity

A **version identifier** identifies one distinguishable semantic state of a logical memory.

It answers:

> Which governed state of this memory is this?

A version identifier must be unambiguous within its declared identity scope.

An implementation may use:

- globally unique version identifiers;
- identifiers unique within one logical memory;
- composite identity consisting of logical-memory and version identifiers;
- another collision-resistant scoped model.

The encoding is implementation-defined.

### 4.3 Representation identity

A **representation identifier** identifies one materialized encoding or form of a logical memory version.

It answers:

> In which encoded or transformed form is this version represented?

One version may have several representations, including:

- an uncompressed encoding;
- a compressed encoding;
- an encrypted encoding;
- a partitioned encoding;
- a schema-migrated encoding;
- a device-specific encoding;
- a streaming representation;
- a reconstructed byte representation.

A representation is not automatically a new semantic version.

### 4.4 Physical-copy identity

A **physical-copy identifier** identifies one physical instance of a representation under a particular custody assignment.

It answers:

> Which stored or cached instance is this?

Several physical copies may share:

- one logical memory identity;
- one version identity;
- one representation identity.

They remain separate physical instances with separate:

- location;
- health;
- integrity state;
- custody;
- failure-domain exposure;
- creation or reconstruction history.

### 4.5 Locator

A **locator** describes where or how memory material may currently be requested.

Examples may include:

- a custodian identity;
- a storage-object address;
- a database key;
- a filesystem location;
- a service-local handle;
- an ACS-accessible service and operation scope.

A locator is mutable operational metadata.

It is not durable logical identity.

### 4.6 Payload reference

An ACS payload reference may permit a participant to obtain memory material.

It may contain or resolve to:

- a locator;
- an access token;
- an object reference;
- a transfer handle;
- another transport-independent reference.

A payload reference is not, by itself:

- logical memory identity;
- version authority;
- provenance;
- commitment evidence;
- durability evidence;
- permission to retain the referenced material.

### 4.7 Alias

An **alias** is a scoped alternate name or reference that resolves to a logical memory, version, representation, or query target.

Aliases may include:

- human-readable names;
- application-local names;
- compatibility identifiers;
- redirected former identifiers;
- domain-specific labels.

An alias is not necessarily permanent or globally unique.

Alias resolution must identify:

- its scope;
- its target type;
- its authority;
- whether the resolution is current, stale, ambiguous, or unavailable.

### 4.8 Schema identity

A **schema identifier** identifies the semantic structure required to interpret a memory representation.

Schema identity must be distinguishable from representation encoding.

Two representations may use different encodings while conforming to the same semantic schema.

A schema may itself have versions.

### 4.9 Provenance identity

A **provenance identifier** identifies a governed provenance record or provenance set associated with:

- a logical memory;
- a version;
- a representation;
- a transformation;
- a reconstruction;
- a lifecycle event.

Provenance may be embedded with memory metadata or stored separately.

Its required identity and availability must remain durable enough for the memory policy it supports.

### 4.10 Operation identity

Operation identity identifies the semantic operation that created, transformed, validated, committed, migrated, or reconstructed memory state.

Operation identity connects MEM-0003 lineage to the operation contracts defined by MEM-0004.

It must remain distinct from:

- transport-message identity;
- retry delivery identity;
- process invocation identity;
- physical-copy identity.

## 5. Identifier requirements

### 5.1 Stable identity

A logical memory identifier shall remain stable for the lifetime of the logical memory and any required historical record.

Migration, replication, representation change, or custodian replacement shall not inherently change it.

### 5.2 Non-reuse

A logical memory identifier or version identifier shall not be reassigned to unrelated memory state after:

- deletion;
- expiration;
- archival;
- failed creation;
- identity merge;
- namespace reorganization.

Minimal historical metadata may remain after deletion to prevent unsafe reuse and stale resurrection.

### 5.3 Explicit scope

Every identifier shall be unambiguous within a declared scope.

When an identifier is not globally unique, the namespace or authority required to resolve it must be preserved as part of its effective identity.

### 5.4 Collision handling

Identifier generation and resolution must define behavior for:

- detected collisions;
- suspected collisions;
- malformed identifiers;
- unsupported identifier versions;
- ambiguous namespaces;
- spoofed identity claims.

A collision must not silently merge unrelated memories.

### 5.5 Identity is not inferred from location

A host address, file path, database key, endpoint, port, process identifier, or storage-object location shall not independently serve as the enduring logical identity of memory.

These values may participate in location or implementation metadata.

### 5.6 Identity is not inferred solely from content

Identical content does not prove identical logical memory identity.

Two memories may contain identical bytes while differing in:

- origin;
- context;
- authority;
- sensitivity;
- retention;
- cognitive meaning;
- lineage;
- lifecycle state.

A content digest may identify or validate an immutable representation.

It does not automatically establish logical-memory equivalence.

### 5.7 Content-addressed identity is limited by declared semantics

An implementation may use content-addressed identifiers for:

- immutable representations;
- chunks;
- manifests;
- schemas;
- integrity verification;
- deduplication.

It must not use deduplication to collapse distinct logical memories unless logical equivalence is separately established.

### 5.8 Identifiers should be non-semantic

Identifiers should not embed sensitive or mutable meaning unless required by explicit policy.

An identifier should not need to change because:

- a classification changes;
- a memory moves;
- a participant is renamed;
- retention changes;
- the representation format changes.

### 5.9 Identity issuance is authorized

Authoritative creation of logical memory identity must be performed by an explicitly assigned responsibility.

That responsibility may be exercised by:

- an admission evaluator;
- a commit authority;
- another authorized memory-role instance.

A proposer may suggest an identifier.

Suggestion does not automatically make the identifier authoritative.

### 5.10 Identity state is evidence-backed

The system must distinguish:

- proposed identity;
- reserved identity;
- accepted identity;
- committed identity;
- conflicting identity;
- invalid identity;
- deleted identity;
- unknown identity.

Not every implementation must expose these exact labels, but equivalent states must not be silently collapsed where the distinction affects behavior.

## 6. Conceptual identity records

This section defines required semantics, not a wire format or database schema.

### 6.1 Logical memory record

A logical memory record should make it possible to determine:

```text
LogicalMemoryRecord
    logical_memory_identity
    identity_namespace_or_authority
    creation_operation_identity
    creation_authority
    initial_provenance_reference
    applicable_policy_reference
    current_lifecycle_reference
    known_version_heads_or_resolution_reference
    identity_validity_state
```

Not every value must reside in one physical record.

The information must be recoverable according to the memory's durability requirements.

### 6.2 Version record

A version record should make it possible to determine:

```text
MemoryVersionRecord
    logical_memory_identity
    version_identity
    parent_version_relations
    cross_memory_derivation_relations
    creation_or_transformation_operation
    schema_identity
    representation_references
    provenance_reference
    commit_evidence_reference
    validation_state
    version_validity_state
```

### 6.3 Representation record

A representation record should make it possible to determine:

```text
MemoryRepresentationRecord
    representation_identity
    logical_memory_identity
    version_identity
    schema_identity
    encoding_or_format_identity
    integrity_descriptor
    equivalence_evidence
    size_or_extent
    required_interpretation_dependencies
```

### 6.4 Physical-copy record

A physical-copy record should make it possible to determine:

```text
PhysicalCopyRecord
    physical_copy_identity
    representation_identity
    custodian_identity
    current_locator
    failure_domain_observations
    integrity_state
    availability_state
    creation_or_reconstruction_operation
    custody_assignment
```

### 6.5 Provenance record

A provenance record should make it possible to determine:

```text
ProvenanceRecord
    provenance_identity
    subject_identity
    origin_claims
    evidence_references
    producing_or_observing_participants
    source_operation_identity
    transformation_relations
    prior_provenance_references
    relevant_time_evidence
    validation_state
    disclosure_or_redaction_state
```

These conceptual records may be:

- combined;
- partitioned;
- embedded;
- reconstructed;
- represented through immutable events;
- projected into a catalog.

Their semantic distinctions must remain recoverable.

## 7. Version model

### 7.1 Versions represent semantic change

A new version is required when governed state changes in a way that affects the memory's meaning under its applicable schema or policy.

Examples may include:

- an authorized correction;
- an updated assertion;
- consolidation that changes semantic content;
- addition or removal of governed content;
- a changed relation to source evidence;
- a change in interpretation required by schema migration;
- resolution of a conflict;
- accepted approximation replacing exact state.

### 7.2 Representation change does not automatically create a version

The following do not inherently require a new semantic version:

- copying;
- relocation;
- compression;
- decompression;
- encryption;
- decryption;
- chunking;
- reassembly;
- equivalent serialization;
- equivalent schema-preserving migration;
- restoration of an identical representation;
- creation of another physical replica.

A new representation identity or physical-copy identity may still be required.

### 7.3 Committed versions are immutable

Once a version is committed, its semantic content and required provenance shall not be silently changed.

A later change must be represented as:

- a new version;
- a correction;
- a superseding version;
- an invalidation event;
- another explicit governed relation.

### 7.4 Version lineage is explicit

Every non-initial version shall identify the prior version or versions from which it continues, except where policy explicitly permits unknown lineage.

Unknown parentage must remain explicit.

### 7.5 Version lineage is acyclic

Authoritative version-parent relations within one logical memory shall not form a cycle.

Semantic associations between memories may form graphs.

Version lineage must remain directionally interpretable.

### 7.6 Wall-clock time is not authoritative order

Timestamps may provide useful evidence.

They must not independently establish version authority or ordering because:

- clocks may drift;
- clocks may be unavailable;
- records may be delayed;
- restored systems may contain old timestamps;
- concurrent versions may be valid.

Authoritative ordering must rely on governed lineage, commit evidence, consistency rules, or another explicit ordering mechanism.

### 7.7 Concurrent versions are permitted

Two or more versions may descend from the same parent without either immediately being invalid.

Such versions may represent:

- concurrent authorized work;
- partitioned operation;
- competing corrections;
- unresolved conflict;
- independent transformations.

Concurrent branches must remain explicit until policy resolves them.

### 7.8 Merge versions preserve parents

A merge version shall preserve references to every version whose governed state materially contributed to the merge.

A merge shall not erase evidence that conflict or branching occurred.

### 7.9 Correction does not rewrite history

A correction creates a new governed state or explicit correction event.

The corrected version may be superseded or invalidated.

Its prior existence and relevant provenance must remain discoverable according to retention and security policy.

### 7.10 Current is contextual

The term **current version** is meaningful only within declared:

- logical-memory identity;
- consistency scope;
- authority scope;
- policy;
- observation time;
- conflict state.

A system must not claim one universal current version when unresolved authoritative branches remain.

### 7.11 Version identifiers are not sequence assumptions

A version identifier may be sequential, random, content-derived, composite, or otherwise encoded.

Consumers must not infer chronological or authoritative order from identifier formatting unless that behavior is explicitly defined.

## 8. New version versus new logical memory

Choosing between a new version and a new logical memory is a semantic decision.

It must not be determined solely by storage convenience.

### 8.1 Create a new version when

A new state remains the governed continuation of the same memory identity.

Indicators include:

- the memory answers the same enduring identity question;
- prior state remains its direct history;
- consumers should normally resolve versions as states of one memory;
- retention and authority treat the state as a continuation;
- correction or refinement preserves the same subject.

Examples may include:

- correcting an inaccurate attribute;
- updating an evolving fact;
- revising a retained procedure;
- migrating semantic content into a newer schema;
- resolving branches of the same memory.

### 8.2 Create a new logical memory when

The resulting state has independent identity and lifecycle.

Indicators include:

- it may exist independently of the source;
- it may have different retention or authorization;
- it represents a different subject or event;
- it is a summary, abstraction, or conclusion with distinct meaning;
- it combines several memories into a new semantic object;
- it may evolve independently after creation;
- treating it as a revision would erase meaningful source distinctions.

Examples may include:

- deriving a general concept from several episodes;
- creating a summary that must coexist with its sources;
- producing a reusable procedure from several observations;
- splitting one composite memory into independently governed memories.

### 8.3 Forking within one memory

A branch may remain within one logical memory when competing states represent unresolved versions of the same continuing identity.

Branching must not be used to conceal that states have become semantically independent memories.

### 8.4 Cross-memory derivation

A new logical memory derived from one or more source memories shall record cross-memory derivation lineage.

Its sources remain independently identifiable.

### 8.5 Identity merge

Two logical memories may be determined to represent one identity.

An identity merge is exceptional and must:

- be authorized;
- preserve both former identifiers;
- preserve provenance;
- preserve version history;
- record the merge decision;
- prevent identifier reuse;
- expose unresolved conflict;
- avoid silently deleting either history.

One identifier may become canonical for a declared scope.

Other identifiers should resolve as governed aliases or merged identities.

### 8.6 Identity split

One logical memory may be split into several independently governed memories when its content or lifecycle no longer has one coherent identity.

A split must preserve:

- the source identity;
- derivation relations;
- applicable provenance;
- authority for the decision;
- the distinction between source and resulting memories.

## 9. Lineage model

### 9.1 Version lineage

Version lineage relates states of the same logical memory.

Common relations may include:

- initial;
- successor;
- correction;
- branch;
- merge;
- schema-migrated successor;
- reconstructed successor where semantics changed;
- superseding version.

### 9.2 Derivation lineage

Derivation lineage relates one logical memory to another.

Common relations may include:

- derived from;
- summarized from;
- generalized from;
- transformed from;
- extracted from;
- merged from;
- split from;
- reconstructed from source evidence.

### 9.3 Representation lineage

Representation lineage relates encodings of the same version.

Relations may include:

- encoded from;
- compressed from;
- encrypted from;
- partitioned from;
- reassembled from;
- schema-preserving migration from;
- byte-identical reconstruction of.

### 9.4 Physical-copy history

Physical-copy history records custody-specific events such as:

- copied from;
- restored from;
- reconstructed from shards;
- migrated from;
- repaired from;
- validated against.

Physical-copy history does not become semantic version lineage unless meaning changed.

### 9.5 Association is not lineage

A semantic relationship between memories does not automatically imply derivation.

Examples include:

- refers to;
- occurred near;
- contradicts;
- supports;
- resembles;
- was recalled with.

Such associations may belong to another public or non-public architecture.

They must not be mislabeled as provenance or lineage merely because the memories are related.

### 9.6 Lineage evidence

A lineage claim must distinguish between:

- asserted relation;
- observed relation;
- validated relation;
- inferred relation;
- unknown relation.

An inferred relation is not automatically authoritative history.

## 10. Provenance model

### 10.1 Provenance is more than origin text

Provenance may describe:

- who or what produced state;
- what source was observed;
- which operation created it;
- which transformations occurred;
- which prior memories contributed;
- which schema applied;
- which validation was performed;
- which trust boundaries were crossed;
- whether reconstruction occurred;
- which information is missing or redacted.

### 10.2 Claims and evidence remain distinct

A provenance claim states that something occurred.

Provenance evidence supports, contradicts, or fails to verify that claim.

A participant may claim an origin without the system being able to validate it.

The memory system shall not silently present claimed provenance as validated provenance.

### 10.3 Provenance may be partial

A memory may possess:

- complete provenance;
- partial provenance;
- unknown provenance;
- conflicting provenance;
- redacted provenance;
- invalid provenance.

Missing provenance must not be silently replaced with invented certainty.

### 10.4 Provenance follows semantic change

When a new version or derived memory is created, its provenance shall include the material sources and transformations required by policy to interpret the result.

### 10.5 Provenance survives representation change

Equivalent re-encoding, copying, compression, encryption, and migration shall preserve required provenance.

Representation-local operational history may be added without replacing semantic provenance.

### 10.6 Reconstruction is recorded

A reconstructed physical copy or representation shall record:

- that reconstruction occurred;
- the reconstruction operation;
- source copies or shards where policy requires;
- validation performed;
- whether the result is byte-equivalent, semantically equivalent, approximate, or unresolved.

### 10.7 Provenance depth is bounded explicitly

Implementations may bound provenance expansion to protect resources.

They may use:

- summaries;
- checkpoints;
- compact lineage roots;
- manifests;
- cryptographic accumulators;
- archived detail;
- externally retained evidence.

A bounded provenance view must disclose that detail was summarized, omitted, archived, or unavailable.

### 10.8 Redaction is explicit

A retrieval result may redact provenance because of authorization or sensitivity.

Redaction must not imply that provenance is absent.

Where safe, the result should distinguish:

- no provenance recorded;
- provenance unavailable;
- provenance withheld;
- provenance partially disclosed.

### 10.9 Provenance is not automatic factual truth

Valid provenance proves history or evidence according to its validation state.

It does not necessarily prove that every proposition stored in the memory is objectively true.

### 10.10 Provenance has lifecycle dependencies

Provenance required to:

- interpret;
- validate;
- authorize;
- reconstruct;
- audit

a memory must not be reclaimed earlier than the policy allows for the memory that depends on it.

Detailed retention rules belong to MEM-0006.

## 11. Time evidence

Memory history may involve several distinct times:

- source-event time;
- observation time;
- proposal time;
- receipt time;
- commit time;
- representation-creation time;
- validation time;
- retrieval time;
- reconstruction time.

These times shall not be silently collapsed.

Time evidence may be:

- exact;
- approximate;
- bounded;
- participant-reported;
- externally verified;
- locally observed;
- unknown;
- conflicting.

Clock uncertainty and provenance of time observations should remain explicit where ordering or interpretation depends on them.

## 12. Schema and representation model

### 12.1 Schema identity is explicit

A representation that requires a schema for interpretation shall identify that schema or a resolvable equivalent.

### 12.2 Schema version is distinct from memory version

Changing the memory's semantic state creates a memory version.

Changing the schema used to encode or interpret that state may create:

- a new representation;
- a new version;
- both;

depending on whether semantic meaning changed.

### 12.3 Encoding identity is explicit

Representations should identify relevant encoding details required to interpret them.

Examples may include:

- serialization family;
- compression method;
- encryption envelope;
- numeric layout;
- tensor shape;
- byte order;
- partition manifest;
- schema version.

This specification does not define those formats.

### 12.4 Representation equivalence requires evidence

Two representations may be declared equivalent only when the applicable equivalence conditions have been established.

Evidence may include:

- lossless conversion;
- matching canonical digest;
- schema-aware comparison;
- deterministic reconstruction;
- validated transformation;
- application-specific equivalence proof.

### 12.5 Exact and semantic equivalence remain distinct

Representations may be:

- byte-identical;
- structurally equivalent;
- semantically equivalent;
- approximately equivalent;
- non-equivalent;
- equivalence unknown.

Approximate equivalence shall not be presented as exact equivalence.

### 12.6 Lossy conversion changes governed meaning

A lossy transformation that affects memory meaning must create:

- a new version; or
- a new derived logical memory,

according to the identity decision rules in this specification.

### 12.7 Unsupported schemas produce explicit state

A service unable to interpret a schema may still retain or transfer the representation.

It must not present the memory as interpreted, validated, or semantically available when required schema support is absent.

### 12.8 Schema dependencies participate in durability

When a schema is required to interpret retained memory, the durability claim for that memory must account for continued schema availability or a safe migration path.

## 13. Version and identity states

### 13.1 Proposed

Identity or version creation has been requested but is not yet authoritative.

### 13.2 Reserved

An identifier has been set aside to prevent collision or reuse, but memory creation or commitment may remain incomplete.

### 13.3 Committed

The version has entered authoritative history according to the applicable commit model.

### 13.4 Current

The version is selected as current within a declared authority and consistency scope.

### 13.5 Stale

The version is older than or does not include state required by the applicable operation scope.

Stale is contextual.

A version may remain historically valid while stale for a current-state request.

### 13.6 Superseded

A later governed version or decision has replaced the version for a declared purpose.

Superseded does not mean erased, invalid, or nonexistent.

### 13.7 Conflicting

The version participates in an unresolved set of incompatible valid-looking states.

### 13.8 Invalid

Required validation failed, authority was absent, lineage is malformed, or another rule establishes that the version must not be treated as valid memory.

Invalidation must preserve enough evidence to prevent silent reuse.

### 13.9 Unverified

Required validation has not completed or evidence is insufficient.

Unverified is not invalid.

### 13.10 Orphaned

The version or representation exists, but required identity, lineage, authority, or metadata cannot currently be resolved.

Orphaned material must not be silently adopted into authoritative memory.

### 13.11 Deleted

A governed lifecycle decision declares the identity or version deleted within a defined scope.

Deleted identifiers remain reserved against unrelated reuse.

Detailed deletion behavior belongs to MEM-0006.

### 13.12 Unknown

Available evidence is insufficient to establish the identity or version state.

## 14. Conflict requirements

### 14.1 Conflicts remain explicit

Conflicting identity, lineage, provenance, version, schema, or equivalence claims must remain visible until resolved.

### 14.2 Arrival order is not resolution

The first or latest-arriving claim shall not automatically replace another valid-looking claim.

### 14.3 Physical majority is not automatically authority

A larger number of physical copies does not automatically establish the authoritative version.

Copies may share:

- one stale source;
- one corrupt source;
- one failure domain;
- one unauthorized operation.

### 14.4 Conflict sets identify their scope

A conflict record should identify:

- subject identity;
- competing claims or versions;
- known lineage;
- relevant authorities;
- available evidence;
- missing evidence;
- affected operations;
- current resolution state.

### 14.5 Resolution is a governed event

Conflict resolution must record:

- the authority or policy used;
- the selected or merged outcome;
- rejected or superseded alternatives;
- relevant evidence;
- whether uncertainty remains.

### 14.6 Conflict resolution does not erase history

Resolved alternatives may become:

- superseded;
- invalid;
- retained historical branches;
- independently derived memories.

They must not silently disappear where their history remains required.

## 15. Physical-copy and catalog boundaries

### 15.1 A copy is not a version

Creating or deleting a physical copy does not inherently create or delete a semantic version.

### 15.2 A catalog entry is not identity authority by possession

A catalog may record logical identity and versions.

The existence of a catalog entry does not independently prove that all recorded claims are current or valid.

### 15.3 A missing catalog entry is not absence

Failure to locate an identity in one catalog does not prove the memory does not exist unless that catalog is authoritative and complete for the declared search scope.

### 15.4 Catalog repair must preserve identity

Rebuilding or migrating a catalog shall not mint new logical identities merely because metadata was reconstructed.

### 15.5 Physical loss does not authorize identity reuse

Loss of every currently known copy does not make the logical identifier available for unrelated memory.

The identity may remain:

- lost;
- unavailable;
- deleted;
- historically reserved;
- recoverable from unknown custody.

## 16. Recovery and reconstruction implications

### 16.1 Reconstruction preserves the appropriate identity layer

A byte-identical reconstruction may preserve:

- logical memory identity;
- version identity;
- representation identity.

It receives a new physical-copy identity.

A semantically equivalent re-encoding may preserve:

- logical memory identity;
- version identity.

It receives a new representation identity and physical-copy identity.

A lossy or approximate reconstruction may require:

- a new version; or
- a new derived logical memory.

### 16.2 Restored metadata is not automatically authoritative

Recovered identity, lineage, or provenance records must be reconciled with current authoritative state.

### 16.3 Stale restoration cannot resurrect deleted state

Restoring an old identity record, version, or representation shall not automatically reverse:

- deletion;
- invalidation;
- supersession;
- later commitment;
- authority revocation.

### 16.4 Incomplete reconstruction is explicit

A reconstruction with missing provenance, schema, parent versions, keys, or integrity evidence must report the missing dependencies.

### 16.5 Recovered identifiers are not reminted

Recovery should restore existing identity where evidence supports continuity.

It should not create replacement identities solely because metadata was temporarily unavailable.

## 17. Role boundaries

The roles defined by MEM-0002 may exercise identity responsibilities as follows.

### 17.1 Memory proposer

May propose:

- content;
- identity hints;
- aliases;
- provenance claims;
- parent or source relations.

It does not independently make those claims authoritative.

### 17.2 Admission evaluator

May determine whether:

- proposed identity is acceptable;
- required provenance is sufficient;
- the operation is a new version or new memory candidate;
- schema requirements are supported.

Admission is not commitment.

### 17.3 Commit authority

Establishes authoritative creation of:

- logical identity;
- committed version;
- version relation;
- governed conflict resolution;
- identity merge or split where assigned.

### 17.4 Custodian

Retains identity-related material under assignment.

Custody does not grant authority to rewrite identity, provenance, or lineage.

### 17.5 Catalog and locator

Records and resolves identity metadata and locations.

It must expose incomplete, stale, or conflicting resolution.

### 17.6 Validator

Evaluates:

- identifier integrity;
- lineage validity;
- provenance evidence;
- schema support;
- representation equivalence;
- reconstruction correctness.

Validation does not itself commit the result.

### 17.7 Retrieval-result coordinator

Reports which identity, version, representation, and provenance state a retrieval result actually established.

### 17.8 Lifecycle authority

May govern deletion, archival, expiration, or identity-merge lifecycle effects.

It must preserve identifier non-reuse and stale-state suppression.

### 17.9 Repair and reconstruction coordinator

Restores identity-supporting metadata and representations without overwriting newer governed history.

## 18. Boundary with ACS

ACS may carry or resolve:

- logical memory identifiers;
- version identifiers;
- aliases;
- schema identifiers;
- locators;
- payload references;
- provenance references;
- operation identifiers.

ACS does not determine:

- whether an identifier is authoritative;
- whether two identities are equivalent;
- whether a version is current;
- whether provenance is valid;
- whether a representation is semantically equivalent;
- whether a conflict is resolved.

MEM does not redefine:

- ACS endpoint identity;
- port identity;
- signal identity;
- relationship identity;
- attachment handling;
- payload-reference transport;
- route resolution;
- secure-session behavior.

A memory identifier may appear inside an ACS signal or referenced payload.

The ACS object remains distinct from the memory object it identifies.

## 19. Security and privacy

### 19.1 Identifiers may be sensitive

Even opaque identifiers may reveal:

- access patterns;
- correlation;
- memory existence;
- participant activity;
- retention behavior.

Identifier disclosure must remain authorized.

### 19.2 Provenance may be more sensitive than content

Provenance may expose:

- source identity;
- observation location;
- relationship history;
- system topology;
- transformation paths;
- security boundaries.

Access to memory content does not automatically grant access to all provenance.

### 19.3 Redacted views preserve identity honesty

A redacted result must not replace hidden provenance with false empty provenance.

### 19.4 Untrusted participants cannot mint authority

A participant may create local identifiers or claims.

Those identifiers and claims do not become authoritative Node memory identity without governed admission and commitment.

### 19.5 Replay does not recreate authority

Replaying an old identity-creation, version-creation, merge, or deletion message must not repeat the semantic operation when operation identity indicates it was already handled.

### 19.6 Identifier predictability must not bypass authorization

Knowledge or guessability of an identifier does not grant permission to retrieve, mutate, or disclose memory.

### 19.7 Identity metadata requires durability and integrity

Identity, lineage, provenance, schema references, and deletion markers must receive protection appropriate to the memory they govern.

Preserving content while losing authoritative identity metadata is not complete memory survival.

## 20. Prohibited interpretations

This specification shall not be interpreted to mean that:

- every transient value requires a durable identifier;
- a file path is logical memory identity;
- a database primary key is necessarily logical identity;
- a content digest always identifies the logical memory;
- identical content must be deduplicated into one memory;
- different bytes always mean different semantic versions;
- copying creates a new version;
- compression creates a new version;
- migration creates a new logical memory;
- a new schema always creates a new memory version;
- timestamps alone establish ordering;
- version identifiers must be sequential;
- one logical memory must have one linear history;
- concurrent branches are automatically corruption;
- a merge may discard parent history;
- superseded means deleted;
- stale means invalid;
- invalid means physically erased;
- redacted provenance means no provenance exists;
- a catalog is the sole identity authority;
- a missing index entry proves absence;
- a payload reference is durable identity;
- recovery may remint identities for convenience;
- deleted identifiers may be reused;
- public provenance requirements disclose non-public cognitive algorithms.

## 21. Open questions

The following questions remain for later specifications and implementation profiles:

- Which identifier scopes are mandatory for baseline conformance?
- Should logical and version identifiers be globally unique or namespace-scoped by default?
- Which responsibilities may reserve identifiers before commitment?
- How long must failed or abandoned identifier reservations remain protected from reuse?
- Which identity metadata is minimum for continuity-critical memory?
- Which version relations require independent validation?
- When should a branch become a new logical memory?
- Which merge decisions require multiple authorities?
- How should identity merges behave under later disagreement?
- Which schema migrations are semantically transparent?
- What evidence is sufficient for semantic representation equivalence?
- How should approximate reconstruction be classified?
- How should provenance summaries prove what detail was omitted?
- Which provenance relations may be garbage-collected after consolidation?
- How should identity resolution operate when catalogs disagree?
- What minimum tombstone information prevents stale resurrection?
- How should opaque identifiers be protected against correlation analysis?
- Which time-evidence classes are required for different memory types?
- How should version lineage be compacted without losing conflict evidence?
- Which identity and provenance structures require separate failure-domain protection?
- How should implementation-specific extensions add provenance without weakening or exposing public semantics?

These questions do not weaken the requirements already established.

## 22. Closing principle

> **Node must be able to answer not only “what bytes remain?” but “which memory is this, which state of it is this, where did it come from, and what changed?”**

Identity survives movement.

Versions preserve change.

Representations preserve form.

Physical copies preserve availability.

Provenance preserves history.

None of them may silently replace the others.

## Revision history

### Version 0.1 — 2026-07-16

- Replaced the planned MEM-0003 stub with the first normative working draft.
- Defined logical-memory, version, representation, physical-copy, schema, provenance, alias, locator, and operation identity.
- Established stable, scoped, non-reusable identifier requirements.
- Distinguished content identity from logical memory identity.
- Defined conceptual identity, version, representation, copy, and provenance records.
- Established immutable committed versions and explicit acyclic version lineage.
- Defined rules for new versions, new logical memories, branches, merges, splits, and cross-memory derivation.
- Distinguished version lineage, derivation lineage, representation lineage, physical-copy history, and semantic association.
- Defined provenance claims, evidence, partial state, reconstruction history, bounded depth, and redaction.
- Distinguished byte, structural, semantic, approximate, and unknown representation equivalence.
- Defined stale, superseded, conflicting, invalid, unverified, orphaned, deleted, and unknown states.
- Established conflict, catalog, recovery, role, ACS, security, and privacy boundaries.
