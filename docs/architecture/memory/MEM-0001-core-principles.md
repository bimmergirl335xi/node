# MEM-0001: Core Principles and Invariants

| Field | Value |
|---|---|
| Specification | MEM-0001 |
| Title | Core Principles and Invariants |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-15 |
| Approval | Accepted as initial working draft |
| Depends on | MEM-0000, ACS-0000, ACS-0001 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in principle; terminology and invariant grouping remain under review |

> **Memory implementations may evolve, but the conditions required for continuity, honesty, boundedness, and recovery must remain enforceable.**

## Architectural-intent notice

This specification defines the mandatory terminology and architectural invariants that every conforming Node memory implementation must preserve.

It does not prescribe:

- programming languages;
- data structures;
- binary layouts;
- serialization formats;
- storage engines;
- databases;
- filesystems;
- consensus algorithms;
- replication counts;
- erasure-coding parameters;
- network protocols;
- cryptographic algorithms;
- production topology;
- cognitive memory-selection policy.

Later MEM specifications may refine these invariants through more specific roles, contracts, lifecycle states, consistency expectations, recovery procedures, and conformance requirements.

They must not silently weaken or contradict this specification.

When implementation behavior conflicts with an invariant, the conflict must be corrected, explicitly documented as non-conforming, or resolved through deliberate revision of the specification.

## 1. Purpose

MEM-0001 establishes the architectural rules that preserve memory meaning and continuity across changing physical infrastructure.

The invariants exist to prevent implementation details from gradually redefining:

- what a memory is;
- what identifies it;
- what it means for memory to be accepted;
- what it means for memory to be durable;
- what a retrieval result proves;
- what failure states must remain visible;
- what recovery is permitted to change;
- how memory operations consume resources;
- how memory interacts with ACS;
- which authority memory services possess.

The memory system must remain understandable under failure.

A result that appears correct only while every node, route, storage device, index, key, and process is healthy is not a sufficient memory architecture.

## 2. Scope

These invariants apply to:

- logical memories;
- memory versions;
- memory representations;
- storage proposals;
- memory mutations;
- recall requests;
- retrieval results;
- indexes and catalogs;
- caches;
- journals;
- checkpoints;
- replicas and reconstruction shards;
- retention and deletion operations;
- repair and recovery;
- migration;
- memory-service admission;
- memory-service authorization;
- memory-related metadata;
- dependencies required to interpret or recover memory.

They apply regardless of:

- hardware architecture;
- operating system;
- storage medium;
- deployment scale;
- process model;
- network topology;
- implementation language;
- local or remote placement;
- use of replication or erasure coding;
- use of centralized or distributed coordination.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements.

The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification.

The word **may** describes permitted behavior.

An implementation does not satisfy an invariant merely by using terminology similar to this specification. Its observable behavior must preserve the required distinction.

## 4. Core terminology

Later specifications may refine these terms. They must preserve the distinctions established here.

### 4.1 Logical memory

A logical memory is identified meaningful state governed by the Node memory architecture.

Its identity and semantics exist independently of any particular physical representation or storage location.

A logical memory may have:

- multiple versions;
- multiple representations;
- multiple physical copies;
- multiple indexes;
- multiple authorized custodians;
- periods of temporary unavailability.

### 4.2 Memory version

A memory version is a distinguishable state of a logical memory.

A version may represent:

- initial creation;
- an authorized update;
- a transformation;
- a correction;
- consolidation;
- reconstruction;
- migration into a new representation;
- another governed state transition.

The exact version model is defined by later specifications.

### 4.3 Memory representation

A memory representation is an encoded, serialized, transformed, compressed, partitioned, or otherwise materialized form of a logical memory or memory version.

A representation is not automatically the memory's enduring identity.

Different representations may describe the same logical memory version when equivalence can be established according to policy.

### 4.4 Physical memory material

Physical memory material is data held in a particular physical resource.

Examples include:

- bytes in volatile memory;
- a file;
- a database record;
- an object-store object;
- a replica;
- a reconstruction shard;
- a journal segment;
- a checkpoint fragment;
- an index entry;
- a cached representation.

Physical presence does not independently prove logical validity, authority, freshness, completeness, or durability.

### 4.5 Custodian

A custodian is a logical participant or service currently responsible for holding, serving, validating, protecting, or reconstructing some memory material.

Custody is a governed responsibility.

Custody does not inherently grant:

- logical ownership;
- mutation authority;
- deletion authority;
- unrestricted access;
- authority over other custodians;
- authority over a physical node.

### 4.6 Memory service

A memory service is a logical service that performs one or more governed memory responsibilities.

A memory service may:

- evaluate storage proposals;
- retain memory material;
- maintain metadata;
- support recall;
- produce retrieval results;
- validate integrity;
- coordinate repair;
- manage lifecycle transitions;
- report availability.

A memory service is not defined by one physical process or host.

### 4.7 Memory operation

A memory operation is a semantically identified request, proposal, decision, transition, or result governed by MEM.

Examples include:

- proposing storage;
- accepting or rejecting a proposal;
- committing a version;
- retrieving memory;
- repairing a copy;
- changing retention;
- deleting a memory;
- validating reconstruction.

Transport delivery is not itself a memory operation outcome.

### 4.8 Operation identity

Operation identity distinguishes one semantic memory operation from another.

A repeated delivery carrying the same operation identity may represent a retry of the same operation rather than a new operation.

The exact identifier format is implementation-defined unless a later specification requires otherwise.

### 4.9 Storage proposal

A storage proposal is a request that identified state be evaluated for admission into governed memory.

A proposal does not itself prove that the state has been:

- accepted;
- committed;
- made durable;
- indexed;
- replicated;
- retained;
- made available for retrieval.

### 4.10 Acceptance

Acceptance means that a memory service has admitted a proposal or operation according to the applicable semantic and policy requirements.

Acceptance does not independently mean:

- commitment;
- durability;
- completion;
- permanent retention;
- universal availability.

The accepting service must make the scope of its acceptance unambiguous.

### 4.11 Commitment

Commitment means that a memory version or lifecycle decision has entered the authoritative memory history according to its declared consistency requirements.

Commitment does not independently prove that the committed state currently satisfies its durability target.

### 4.12 Durability

Durability describes the failures a committed memory version is declared and demonstrated to survive.

Durability is a policy-backed property.

It is not equivalent to:

- one successful local write;
- one acknowledgment;
- one physical copy;
- current availability;
- indefinite retention.

### 4.13 Availability

Availability describes whether a requested memory operation can currently be performed under its required scope, authorization, consistency, and resource conditions.

A memory may remain logically valid while temporarily unavailable.

### 4.14 Retention

Retention describes the policy governing how long or under which conditions a memory should remain preserved.

Retention intent does not independently prove current durability or availability.

### 4.15 Recall request

A recall request asks memory services to locate, evaluate, or retrieve memory according to a defined scope and semantic contract.

A recall request is not proof that matching memory exists.

### 4.16 Retrieval result

A retrieval result is the governed outcome of a recall or retrieval operation.

Depending on the operation, a result may include:

- memory content or a reference to it;
- logical identity;
- version information;
- provenance;
- validation state;
- freshness;
- completeness;
- consistency state;
- authorization-limited scope;
- an explicit non-success condition.

A retrieval result must not imply more certainty than the operation established.

### 4.17 Provenance

Provenance describes the origin and relevant history of memory.

It may include:

- producing participant;
- observation source;
- creation context;
- transformation history;
- derivation history;
- version lineage;
- validation history;
- trust-boundary crossings;
- reconstruction history.

Required provenance is part of the memory's governed meaning.

### 4.18 Failure domain

A failure domain is a set of resources that may fail together because they share a dependency or exposure.

Examples may include:

- one process;
- one storage device;
- one physical node;
- one chassis;
- one power source;
- one network path;
- one administrative boundary;
- one physical site;
- one software defect domain.

The exact domains relevant to a deployment are implementation- and policy-dependent.

### 4.19 Under-protected

A memory is under-protected when its current physical custody, metadata protection, key availability, or reconstruction capability does not satisfy its declared durability requirements.

Under-protected memory may remain valid and available.

It must not be represented as fully protected.

### 4.20 Not found

**Not found** means that the required search or evaluation completed within its declared scope and found no matching memory according to the applicable consistency and authority conditions.

Not found is not interchangeable with:

- unavailable;
- unknown;
- partial;
- unauthorized;
- timed out;
- disconnected;
- deferred;
- failed;
- unindexed;
- incompletely searched.

### 4.21 Indeterminate

An operation is indeterminate when the system cannot safely establish which semantic outcome occurred.

Indeterminate is not success.

Indeterminate is not failure.

It requires explicit reconciliation, retry, inspection, or recovery behavior according to the operation contract.

## 5. Core invariants

### 5.1 Logical identity and subsystem boundaries

#### Invariant 1 — Memory is a logical object

A memory shall be represented as logical state rather than as a physical file, database row, buffer, replica, shard, cache entry, index record, address, or device allocation.

**Rationale**

Physical resources are temporary. Defining memory through those resources would cause identity to change or disappear when storage moves.

**This enables**

- migration;
- replication;
- reconstruction;
- representation changes;
- storage replacement;
- location-independent recall.

**This forbids**

Treating one physical storage artifact as the enduring identity of a memory.

---

#### Invariant 2 — Logical identity survives placement and representation change

The identity of a memory shall survive ordinary relocation, replication, re-encoding, compression, partitioning, reconstruction, or storage-medium replacement when semantic continuity can be established.

**Rationale**

Physical layout should be replaceable without redefining what the memory is.

**This enables**

- hardware replacement;
- tiered storage;
- format migration;
- reconstruction;
- storage optimization;
- heterogeneous custodians.

**This forbids**

Assigning a new logical identity merely because a memory moved or changed physical encoding.

---

#### Invariant 3 — Memory remains separate from ACS objects

A memory shall not be identified as an ACS relationship, connection, endpoint, port, signal, transport session, or payload reference.

ACS objects may carry, reference, authorize, or organize memory operations without becoming the stored memory.

**Rationale**

Communication state and memory state possess different identities, lifecycles, and failure behavior.

**This enables**

- reconnection without memory loss;
- route replacement;
- persistent memory across session expiry;
- independent memory and ACS evolution.

**This forbids**

Treating connection loss, signal expiry, endpoint migration, or payload-reference replacement as deletion of the referenced memory.

---

#### Invariant 4 — Physical custody does not grant logical authority

Holding memory material shall not independently grant authority to mutate, commit, disclose, retain, delete, or redefine the corresponding logical memory.

Authority must be established separately.

**Rationale**

Storage possession and semantic authority are distinct responsibilities.

**This enables**

- untrusted or restricted custodians;
- least-privilege storage;
- replicated custody;
- independent validation;
- safer compromise containment.

**This forbids**

Assuming that a service may alter or expose a memory merely because it stores a copy.

---

#### Invariant 5 — Supporting structures do not silently become memory authority

Caches, indexes, catalogs, journals, checkpoints, replicas, and reconstruction shards may support memory operation, but none shall independently redefine logical memory truth beyond its explicitly granted role.

**Rationale**

Optimization and recovery structures may be incomplete, stale, damaged, or specialized.

**This enables**

- replaceable indexes;
- disposable caches;
- independent catalog repair;
- multiple retrieval strategies;
- explicit authority boundaries.

**This forbids**

Treating a missing cache entry as proof of absence or an index entry as proof that a memory is valid and current.

---

#### Invariant 6 — Continuity, availability, commitment, durability, and retention remain distinct

A conforming implementation shall represent these properties separately.

No one property shall be silently inferred from another.

**Rationale**

A committed memory may be unavailable. An available memory may be under-protected. A retained memory may not yet be durable. A durable memory may later be deliberately deleted.

**This enables**

- honest degradation;
- precise recovery;
- meaningful service guarantees;
- correct lifecycle handling.

**This forbids**

Using one generic “stored” or “healthy” state to conceal materially different memory conditions.

### 5.2 Memory-operation semantics

#### Invariant 7 — Delivery is not semantic completion

Transport delivery, acknowledgment, queue admission, process completion, or local persistence shall not independently mean that a memory operation was accepted, committed, made durable, or successfully retrieved.

**Rationale**

Communication mechanics cannot determine memory semantics.

**This enables**

- explicit operation contracts;
- asynchronous processing;
- staged commitment;
- meaningful failure reporting.

**This forbids**

Reporting a successful memory write solely because bytes reached a recipient.

---

#### Invariant 8 — Retried operations are duplicate-safe

A memory operation capable of retry shall possess sufficient identity and state handling to distinguish a retry from a new semantic operation.

A conforming implementation shall not depend on transport-level exactly-once delivery.

**Rationale**

Distributed communication may duplicate, delay, reorder, or replay delivery.

**This enables**

- safe retries;
- reconnect recovery;
- duplicate suppression;
- deterministic reconciliation.

**This forbids**

Silently creating repeated storage, mutation, deletion, or lifecycle transitions from duplicate delivery.

---

#### Invariant 9 — Operation scope and side effects are explicit

Every memory operation shall define the scope within which it applies and the side effects it is permitted to produce.

A retrieval operation shall not silently become a mutation operation.

**Rationale**

Ambiguous scope makes results impossible to interpret and retries unsafe.

**This enables**

- bounded recall;
- authorization checks;
- predictable mutation;
- auditable operation behavior.

**This forbids**

A read or recall operation silently changing semantic memory state merely because it accessed the memory.

Operational metadata updates may occur only when explicitly governed.

---

#### Invariant 10 — Semantic outcomes remain explicit

Memory operations shall report outcomes that distinguish success, rejection, deferral, unavailability, partial completion, conflict, invalidity, unauthorized access, failure, and indeterminate state where relevant.

**Rationale**

A generic success or failure flag cannot represent distributed memory honestly.

**This enables**

- safe retry decisions;
- recovery;
- user and cognitive uncertainty;
- correct resource handling;
- conformance testing.

**This forbids**

Collapsing every non-success condition into “not found” or every acknowledgment into “completed.”

---

#### Invariant 11 — Not found requires completed evidence

A memory service shall report not found only when the search required by the operation contract completed adequately within its declared scope.

**Rationale**

A disconnected custodian, unavailable index, denied request, incomplete search, or expired route does not prove absence.

**This enables**

- trustworthy negative results;
- conservative recall;
- correct partition behavior;
- honest uncertainty.

**This forbids**

Using not found as a convenient substitute for timeout, unavailability, authorization failure, or incomplete participation.

### 5.3 Truth, provenance, versions, and integrity

#### Invariant 12 — Unknown and degraded memory states are explicit

Unknown, unavailable, partial, stale, conflicting, corrupted, unverified, under-protected, unauthorized, deferred, expired, deleted, and not-found states shall remain distinguishable where relevant.

**Rationale**

Distributed systems frequently operate with incomplete or delayed evidence.

**This enables**

- conservative reasoning;
- safe recovery;
- targeted repair;
- accurate health reporting;
- meaningful retrieval results.

**This forbids**

Silently converting missing or uncertain state into absent, false, current, complete, healthy, or trusted state.

---

#### Invariant 13 — Committed versions are not silently mutated

Once a memory version is committed, any semantic change shall produce a distinguishable governed version, correction, or lifecycle event.

**Rationale**

Silent in-place mutation destroys history, provenance, retry safety, and recovery confidence.

**This enables**

- lineage;
- rollback;
- conflict detection;
- auditing;
- deterministic reconstruction.

**This forbids**

Changing committed memory while continuing to present it as the same unchanged version.

---

#### Invariant 14 — Required provenance and version information are preserved

A memory implementation shall preserve all provenance and version information required by the memory's applicable policy and semantic use.

**Rationale**

Content without required origin, lineage, schema, or transformation context may no longer carry the same meaning.

**This enables**

- validation;
- trust assessment;
- reconstruction;
- conflict analysis;
- historical inspection.

**This forbids**

Discarding required provenance merely to reduce storage or simplify an index.

---

#### Invariant 15 — Integrity is validated before trusted use

Memory content, metadata, versions, and dependencies shall receive the integrity validation required by policy before they are treated as trusted memory.

**Rationale**

Physical presence does not prove correctness.

**This enables**

- corruption detection;
- quarantine;
- safe repair;
- trustworthy retrieval;
- independent replica comparison.

**This forbids**

Returning known-unvalidated or integrity-failed material as ordinary valid memory.

---

#### Invariant 16 — Derived and reconstructed results remain identifiable

Transformed, summarized, merged, approximated, reconstructed, or otherwise derived memory results shall remain distinguishable from directly retained source state when that distinction affects meaning.

**Rationale**

A reconstructed or derived result may be valid without being identical in provenance or certainty to its source.

**This enables**

- honest confidence;
- lineage tracking;
- validation;
- multiple representations;
- safe approximation.

**This forbids**

Presenting reconstructed, transformed, or inferred material as an unchanged original when equivalence has not been established.

---

#### Invariant 17 — Conflict is not silently erased

When valid-looking memory evidence conflicts and policy cannot establish one authoritative outcome, the conflict shall remain explicit.

**Rationale**

Arbitrary selection can silently rewrite history or conceal corruption.

**This enables**

- later reconciliation;
- immune evaluation;
- provenance comparison;
- conservative retrieval;
- operator review where required.

**This forbids**

Choosing whichever replica responded first and presenting it as unquestionably authoritative.

### 5.4 Durability, custody, and recovery

#### Invariant 18 — Required survival cannot depend on one ordinary failure domain

A memory whose declared durability requires survival of an ordinary failure shall not depend exclusively on one applicable failure domain.

**Rationale**

Multiple copies inside one shared failure domain may be lost together.

**This enables**

- node-loss survival;
- storage-loss survival;
- correlated-failure planning;
- meaningful durability classes.

**This forbids**

Claiming node-failure durability when every required copy and dependency resides on the same physical node.

---

#### Invariant 19 — Durability is declared and evidence-backed

Durability shall be an explicit policy-backed claim supported by current evidence.

It shall not be inferred solely from successful storage, copy count, elapsed time, or present availability.

**Rationale**

Durability describes failure survival, not ordinary-operation success.

**This enables**

- honest guarantees;
- conformance testing;
- under-protection detection;
- repair prioritization.

**This forbids**

Labeling a memory durable merely because one write completed.

---

#### Invariant 20 — Durability includes required dependencies

A durability claim shall include all dependencies required to identify, locate, interpret, validate, authorize, decrypt, and reconstruct the memory.

Such dependencies may include:

- metadata;
- version lineage;
- schemas;
- catalogs;
- manifests;
- integrity evidence;
- reconstruction information;
- cryptographic key material.

**Rationale**

Memory bytes are unusable when the information required to interpret or recover them has been lost.

**This enables**

- complete recovery planning;
- key-continuity protection;
- metadata repair;
- meaningful disaster recovery.

**This forbids**

Claiming memory survival while its only catalog, schema, manifest, or decryption path remains a single point of failure.

---

#### Invariant 21 — Failure-domain independence matters more than copy count

Placement and protection decisions shall consider correlated failure domains rather than relying only on the number of copies or shards.

**Rationale**

Several copies may provide little protection when they share power, storage, software, administrative, or physical dependencies.

**This enables**

- realistic resilience;
- topology-aware placement;
- honest capacity planning;
- failure-domain testing.

**This forbids**

Treating three copies on one device, process, or host as equivalent to three independent protections.

---

#### Invariant 22 — Under-protection and durability downgrade are explicit

When a memory no longer satisfies its declared durability, the system shall mark it under-protected or otherwise explicitly degraded.

A stronger durability state shall not be silently downgraded.

**Rationale**

Node loss, repair backlog, key loss, resource pressure, or partition may reduce current protection without deleting the memory.

**This enables**

- repair scheduling;
- conservative behavior;
- durability monitoring;
- honest service reporting.

**This forbids**

Continuing to report full durability after required protection has been lost.

---

#### Invariant 23 — Recovery and repair precede durability claims

A memory design shall define and validate recovery and repair behavior before claiming production durability.

**Rationale**

Copies without tested reconstruction may create false confidence.

**This enables**

- fault injection;
- repair automation;
- recovery exercises;
- conformance evidence.

**This forbids**

Claiming resilience solely because redundant data exists somewhere.

---

#### Invariant 24 — Stale state cannot overwrite or resurrect newer governed state

A stale, disconnected, restored, or rejoining custodian shall not automatically overwrite a newer committed version or resurrect validly deleted memory.

**Rationale**

Recovered nodes may contain old but internally valid-looking state.

**This enables**

- safe rejoining;
- tombstone enforcement;
- split-brain recovery;
- version reconciliation.

**This forbids**

Treating the return of an old copy as proof that its former state should become current again.

### 5.5 Lifecycle and resource safety

#### Invariant 25 — Memory lifecycle state is explicit

Memory and memory operations shall expose the lifecycle distinctions required to interpret their current state.

Retention, expiration, archival, deletion, reclamation, repair, and migration shall not occur as invisible side effects.

**Rationale**

Lifecycle changes alter availability, authority, and recovery expectations.

**This enables**

- safe deletion;
- auditing;
- retention enforcement;
- recovery;
- garbage-collection validation.

**This forbids**

Treating physical disappearance as a sufficient semantic deletion record.

---

#### Invariant 26 — Forgetting and deletion are governed operations

Deletion or deliberate forgetting of governed memory shall require an explicit authorized lifecycle decision.

Physical erasure, eviction, cache loss, resource pressure, or custodian failure shall not independently count as valid forgetting.

**Rationale**

Accidental loss and deliberate forgetting have different meaning.

**This enables**

- deletion authorization;
- tombstones;
- grace periods;
- stale-state suppression;
- auditable forgetting.

**This forbids**

Claiming that missing data was intentionally forgotten when no governed deletion occurred.

---

#### Invariant 27 — Resource pressure must not cause silent amnesia

Storage, memory, bandwidth, compute, power, repair, or index pressure shall produce explicit policy-governed degradation.

Protected memory shall not be silently discarded to satisfy local convenience.

**Rationale**

Finite resources require tradeoffs, but hidden deletion destroys continuity.

**This enables**

- admission control;
- explicit deferral;
- retention prioritization;
- capacity signaling;
- safe degraded operation.

**This forbids**

Deleting protected memory merely because one custodian became full.

---

#### Invariant 28 — Memory operations are bounded and resource-accounted

Every memory operation shall operate within explicit limits appropriate to its function.

Relevant limits may include:

- request size;
- result size;
- search scope;
- validation cost;
- execution time;
- queue occupancy;
- transfer volume;
- retry behavior;
- fan-out;
- replication work;
- reconstruction work;
- retained history.

**Rationale**

Unbounded memory operations can destabilize the organism and starve recovery.

**This enables**

- predictable degradation;
- admission;
- prioritization;
- capacity planning;
- denial-of-service resistance.

**This forbids**

Unlimited recall fan-out, unbounded queues, uncontrolled repair storms, and infinite retry loops.

---

#### Invariant 29 — Memory admission remains separate from ACS backpressure

ACS communication state may delay or reject delivery.

Memory services shall independently report whether a memory operation was admitted, deferred, rejected, or limited by memory-specific policy and resources.

**Rationale**

Transport congestion and memory-resource exhaustion are different conditions with different recovery behavior.

**This enables**

- accurate retry;
- independent scaling;
- meaningful observability;
- clean subsystem boundaries.

**This forbids**

Representing memory-capacity rejection solely as a broken connection or representing transport failure as memory-policy rejection.

---

#### Invariant 30 — Optimization cannot silently weaken memory meaning

Caching, batching, compression, approximation, locality, speculative execution, asynchronous commitment, and other optimizations may operate only when their semantic effects remain explicit and governed.

**Rationale**

Performance mechanisms must not redefine correctness.

**This enables**

- safe optimization;
- replaceable implementations;
- measured tradeoffs;
- heterogeneous storage tiers.

**This forbids**

Returning stale cache data as current, weakening durability without disclosure, or discarding provenance for speed.

### 5.6 Security and authority

#### Invariant 31 — Authentication is not memory authorization

Establishing participant identity or an authenticated ACS session shall not automatically authorize access to every memory or memory operation.

Memory authorization must remain explicit, scoped, and revocable.

**Rationale**

Identity proves who a participant is, not what it may do.

**This enables**

- least privilege;
- sensitivity boundaries;
- operation-specific capabilities;
- revocation;
- compartmentalization.

**This forbids**

Granting unrestricted memory access merely because a participant is connected or authenticated.

---

#### Invariant 32 — Memory authority is least-privilege and operation-specific

Authority to retrieve, propose, commit, mutate, retain, repair, reconstruct, delete, or disclose memory shall be granted separately where the distinction affects safety or confidentiality.

**Rationale**

Memory responsibilities carry different risks.

**This enables**

- restricted custodians;
- independent validators;
- protected deletion;
- limited retrieval services;
- safer compromise containment.

**This forbids**

Treating general memory-service participation as universal memory authority.

---

#### Invariant 33 — Key custody remains separate and recoverable

Ordinary memory objects, indexes, and custodians shall not directly assume unrestricted custody of long-term private keys.

The availability and durability of required decryption authority shall be protected according to the memory it controls.

**Rationale**

Encrypted memory without recoverable key authority is functionally unavailable and may be permanently lost.

**This enables**

- key rotation;
- revocation;
- hardware-backed custody;
- threshold recovery;
- compartmentalized access.

**This forbids**

Claiming memory durability while the only required decryption key exists in one ordinary process or storage record.

---

#### Invariant 34 — Compromise and corruption are compartmentalized

Compromise, corruption, or unauthorized access affecting one custodian, memory service, or failure domain must not automatically expose, alter, or invalidate unrelated memory.

**Rationale**

One failure should not become organism-wide memory compromise.

**This enables**

- independent trust;
- segmented keying;
- bounded repair;
- selective revocation;
- limited breach impact.

**This forbids**

Global unrestricted memory secrets or authority whose compromise grants control over every memory.

---

#### Invariant 35 — Memory services do not control physical-node disposition

A memory service shall not independently possess unrestricted authority to erase, quarantine, rebuild, disable, restart, or destroy a physical node.

Such actions belong to separately authorized lifecycle, security, immune, recovery, or administrative systems.

**Rationale**

Memory semantics must not silently acquire infrastructure-control authority.

**This enables**

- auditable recovery;
- safer immune integration;
- separation of concerns;
- constrained repair behavior.

**This forbids**

A memory-repair service destroying a physical node merely because it found inconsistent data.

## 6. Invariant interaction and precedence

These invariants are intended to operate together.

An implementation shall not satisfy one invariant by violating another.

Examples include:

- boundedness must not be achieved through silent deletion of protected memory;
- availability must not be improved by returning unvalidated or stale data as current;
- repair must not restore a deleted version without lifecycle reconciliation;
- security must not erase required provenance;
- caching must not redefine authoritative state;
- ACS retry must not duplicate a memory mutation;
- durability must not be claimed without recoverable metadata and key authority;
- conflict resolution must not silently discard valid competing evidence;
- low latency must not convert unknown into not found.

When two requirements appear to conflict, the implementation must preserve:

1. safety;
2. integrity;
3. explicit uncertainty;
4. authorization;
5. continuity;
6. bounded operation.

Later specifications may define more precise precedence for particular operations.

## 7. Public architecture independence

Public MEM requirements shall remain independently understandable and implementable without access to non-public cognitive algorithms, production topology, internal selection heuristics, or operator procedures.

A conforming public abstraction may define:

- semantic contracts;
- authority boundaries;
- lifecycle states;
- availability states;
- durability expectations;
- recovery principles;
- failure behavior;
- conformance evidence.

It shall not require disclosure of proprietary cognitive policy merely to explain basic memory correctness.

This boundary exists so that the public architecture remains complete rather than becoming an incomplete redaction of another design.

## 8. Conformance expectations

A conforming implementation must be able to demonstrate how it preserves every applicable invariant.

Conformance evidence should identify:

- supported memory categories;
- supported operation types;
- declared durability behavior;
- applicable failure domains;
- known single points of failure;
- under-protection reporting;
- duplicate-operation handling;
- integrity validation;
- stale-state handling;
- conflict behavior;
- deletion and resurrection protection;
- resource ceilings;
- authorization boundaries;
- recovery limitations.

An implementation may be partially conforming only when the unsupported scope is explicit.

Unsupported behavior must not be reported as safely implemented.

Detailed conformance levels and required fault tests belong to MEM-0010.

## 9. Prohibited interpretations

This specification shall not be interpreted to mean that:

- every transient value must become durable memory;
- every memory must use the same durability policy;
- every memory requires global consistency;
- every memory requires full replication;
- every memory must always be available;
- one database must store all memory;
- decentralization forbids coordination;
- coordination permits one irreplaceable authority;
- a content hash alone necessarily defines logical identity;
- a replica count alone proves durability;
- encryption alone proves security;
- authenticated access implies authorization;
- a successful response proves a complete search;
- absence of a result proves absence of memory;
- reconstruction must reproduce an undocumented internal implementation;
- deletion means immediate physical erasure in every case;
- physical persistence means indefinite retention;
- recovery may silently alter memory meaning;
- memory services may replace ACS;
- ACS may define memory semantics;
- public conformance requires disclosure of non-public cognitive policy.

## 10. Open questions

The following questions remain for later specifications:

- Which core memory classes should receive distinct default durability expectations?
- Which operation outcomes must be universal across all memory services?
- How should a consistency expectation be declared and inherited?
- Which provenance fields are universal and which are memory-class-specific?
- What evidence is sufficient to establish representation equivalence?
- How should conflicting but valid-looking versions be reconciled?
- Which failure domains are mandatory for baseline public conformance?
- How should temporary key unavailability differ from permanent key loss?
- Which lifecycle transitions require multi-party authorization?
- How should under-protected memory influence new storage admission?
- What minimum metadata must survive to reconstruct a logical memory?
- How should incomplete indexes affect recall scope and not-found claims?
- Which access-related metadata changes are permissible during a read?
- How should memory repair be rate-limited during widespread failure?
- How should implementations report durability that varies over time?
- Which invariants require stronger rules for continuity-critical memory?

These are open design questions, not permission to violate the invariants already established.

## 11. Closing principle

> **Node must never gain the appearance of remembering by concealing what it cannot verify, cannot retrieve, or may already have lost.**

Memory continuity depends on preserving distinctions.

Identity is not placement.

Delivery is not commitment.

Availability is not durability.

Silence is not absence.

A copy is not recovery.

Forgetting is not failure.

## Revision history

### Version 0.1 — 2026-07-15

- Replaced the planned MEM-0001 stub with the first normative working draft.
- Defined core memory terminology.
- Established thirty-five architectural invariants.
- Formalized the separation between memory semantics and ACS communication.
- Distinguished continuity, availability, commitment, durability, and retention.
- Established retry safety and strict not-found semantics.
- Required explicit unknown, partial, stale, conflicting, corrupted, and under-protected states.
- Required durability claims to include metadata, schema, integrity, reconstruction, and key dependencies.
- Established failure-domain-aware protection and stale-state resurrection prevention.
- Defined lifecycle, boundedness, authorization, key-custody, and compromise-containment rules.
- Preserved the independence of public architecture from non-public cognitive and production policy.
