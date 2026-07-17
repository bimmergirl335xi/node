# MEM-0000: Memory Architecture Charter

| Field | Value |
|---|---|
| Specification | MEM-0000 |
| Title | Memory Architecture Charter |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-15 |
| Approval | Accepted as initial working draft |
| Depends on | ACS-0000, ACS-0001 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in principle; terminology and boundaries remain under review |

> **Memory continuity exists so that Node may preserve meaningful state while the hardware, processes, connections, and storage beneath it change.**

## Architectural-intent notice

This specification defines the enduring architectural intent of the Node memory system.

It does not prescribe implementation unless explicitly stated.

Implementations may change as storage technologies, hardware capabilities, network conditions, cognitive architectures, and operational requirements evolve. The architectural meaning of memory, continuity, provenance, availability, retention, and recovery should remain stable unless deliberately revised or superseded.

When an implementation and this specification disagree, the discrepancy must be investigated rather than silently accepted.

This specification is independently written as a public architectural abstraction. It defines system boundaries and guarantees without disclosing production-sensitive procedures, proprietary cognitive algorithms, or implementation-specific internal policy.

## 1. Purpose

The Node memory architecture establishes the foundational principles governing how meaningful state is identified, proposed, validated, retained, located, retrieved, reconstructed, migrated, repaired, forgotten, and made available across a distributed cognitive organism.

Node may operate across many physical machines, storage devices, processes, accelerators, and network paths. Those resources are temporary and replaceable. Memories and the continuity they provide must therefore not be defined by one machine, one process, one file, one address, one connection, or one storage technology.

The memory architecture exists so that:

- important state can survive ordinary component failure;
- memory identity can survive physical relocation;
- retrieval can remain meaningful under partial availability;
- provenance and version lineage remain inspectable;
- uncertainty and inconsistency are represented honestly;
- memory operations remain bounded by finite resources;
- recovery is planned before failure occurs;
- memory services can use ACS without duplicating it;
- implementation choices serve deliberate architecture rather than silently defining it.

Memory continuity is a property of Node and its governed memory services, not of any individual physical node.

## 2. Design rationale

Distributed cognitive systems create and consume state continuously.

Some state is temporary and recomputable. Some state preserves immediate context. Some state represents learned knowledge, relationships, experiences, skills, commitments, or identity. These forms of state differ in meaning, lifetime, importance, sensitivity, and recovery requirements.

A memory design based only on local files, process-owned buffers, host addresses, or isolated databases would create several unacceptable risks:

- loss of one node could destroy a disproportionate amount of memory;
- relocation could change or erase memory identity;
- transport failure could be mistaken for memory absence;
- retries could duplicate mutating operations;
- stale copies could overwrite newer valid state;
- corrupted data could be returned as trustworthy memory;
- storage pressure could cause uncontrolled forgetting;
- encrypted memory could become unrecoverable through key loss;
- retrieval results could conceal incomplete or conflicting evidence;
- implementation convenience could gradually redefine what memory means.

This charter exists before production memory code so that memory behavior is deliberate, reviewable, testable, and consistent with the wider Node architecture.

## 3. Foundational premise

Node is one distributed cognitive organism.

Physical machines provide temporary execution, storage, transport, and acceleration resources. Logical participants perform work. Memory services preserve and expose meaningful state. ACS provides the communication and relationship substrate through which authorized participants interact.

No physical node is the memory of Node by definition.

A physical node may:

- create a storage proposal;
- temporarily host memory data;
- maintain an index or cache;
- coordinate a memory operation;
- validate a result;
- reconstruct unavailable state;
- provide local retrieval services;
- retain recovery material.

None of those responsibilities inherently makes that physical node the sole logical owner of the memory it handles.

The loss of an ordinary physical node should normally reduce availability, locality, performance, or redundancy rather than erase a large portion of Node's meaningful history or continuity.

## 4. Memory continuity

### 4.1 Definition

Memory continuity is the ability to preserve the identity, meaning, provenance, interpretability, and required availability of memory across changes in physical substrate.

Such changes may include:

- process restart;
- service migration;
- storage replacement;
- node disconnection;
- node loss;
- network partition;
- transport replacement;
- topology reorganization;
- software upgrade;
- hardware generation change;
- partial corruption;
- planned shutdown;
- recovery from checkpoint or journaled state.

Continuity does not require every transient value to survive every possible failure.

It requires the system to define:

- which state is memory;
- which state is expendable;
- which state must survive;
- how much recent loss is acceptable;
- which evidence is required before a result is trusted;
- how incomplete recovery is represented;
- when a memory operation is considered accepted;
- when memory is considered durable;
- how memory identity survives physical change.

### 4.2 Continuity is not availability

A memory may continue to exist logically while being temporarily unavailable.

Availability describes whether the memory can presently be accessed under the required conditions.

Continuity describes whether the memory remains a valid, identifiable part of Node despite changes or interruption.

The system must not equate temporary unavailability with deletion, absence, or loss.

### 4.3 Continuity is not indefinite retention

Memory continuity does not require every accepted memory to be retained forever.

Retention, expiration, consolidation, archival, forgetting, deletion, and garbage collection are legitimate memory lifecycle operations when governed by explicit policy.

A memory that is deliberately and validly forgotten is different from one that is accidentally lost.

## 5. Core design philosophy

### 5.1 Logical memory identity precedes physical placement

A memory shall be represented as a logical entity whose identity does not depend solely on:

- a host;
- a process;
- a filesystem path;
- a storage device;
- a database row;
- a network address;
- an endpoint;
- a port;
- a connection;
- a transport session;
- a payload reference;
- a particular serialized representation.

Physical placement may change without inherently changing logical memory identity.

### 5.2 Meaning precedes medium

Memory architecture begins with what stored state means and what guarantees surround it.

It does not begin by selecting:

- databases;
- object stores;
- filesystems;
- replication libraries;
- serialization formats;
- network protocols;
- consensus systems;
- cache implementations;
- storage hardware.

Those technologies may implement memory responsibilities. They do not independently define the semantics of memory.

### 5.3 Memory and communication remain separate

ACS defines how identified participants establish relationships and exchange permitted signals or payload references.

MEM specifications define the semantics of memory services and operations.

A connection may carry a recall request, but the connection is not the memory.

A relationship may authorize access, but the relationship is not the stored state.

A payload reference may identify retrievable data, but the reference is not durable memory identity.

A transport acknowledgment may confirm delivery, but it does not independently confirm memory acceptance, commitment, retention, replication, or reconstruction.

The memory architecture shall reuse ACS rather than create a competing communication substrate.

### 5.4 Failure is expected

Nodes fail. Storage devices fail. Processes crash. Networks partition. Memory cells produce errors. Software contains defects. Clocks disagree. Replicas become stale. Indexes become incomplete. Credentials expire. Operators make mistakes.

Recovery is a normal operating condition, not an exceptional afterthought.

Memory services must define behavior for partial failure before claiming durability or continuity.

### 5.5 Correctness precedes optimization

Memory operations must be understandable and correct before they become fast.

Performance improvements must not silently weaken:

- identity;
- provenance;
- integrity;
- durability;
- consistency;
- authorization;
- recoverability;
- boundedness;
- lifecycle correctness;
- explicit uncertainty.

Caching, batching, compression, locality, approximation, asynchronous processing, and speculative retrieval may be used only when their effects remain explicit and governed.

### 5.6 Resources are finite

Memory competes for:

- volatile memory;
- persistent storage;
- network bandwidth;
- processing time;
- accelerator capacity;
- index capacity;
- reconstruction capacity;
- replication capacity;
- power;
- operator attention.

Every meaningful memory mechanism must therefore operate within explicit and enforceable limits.

No memory category, participant, retrieval request, retention rule, or adaptive policy may assume unlimited capacity.

Resource exhaustion must produce defined behavior rather than uncontrolled growth, silent corruption, or accidental deletion of protected memory.

### 5.7 Unknown and degraded states remain explicit

Distributed memory operates with incomplete information.

The following states must remain distinguishable where relevant:

- unavailable;
- unknown;
- partial;
- stale;
- conflicting;
- corrupted;
- under-protected;
- unverified;
- unauthorized;
- deferred;
- expired;
- deleted;
- not found.

Missing information must not be silently interpreted as false, absent, healthy, current, complete, or trustworthy.

### 5.8 Provenance is part of memory meaning

A memory is not only stored content.

Depending on its class and use, its meaning may also depend on:

- origin;
- producing participant;
- observation context;
- transformation history;
- derivation history;
- version lineage;
- validation history;
- confidence;
- applicable schema;
- relevant dependencies;
- trust-boundary crossings;
- retention status.

Provenance requirements may vary by memory class, but required provenance must not be discarded merely for implementation convenience.

### 5.9 Memory operations have explicit lifecycles

Meaningful memory operations possess lifecycle state.

Examples include:

- storage proposal;
- validation;
- acceptance;
- rejection;
- commitment;
- replication;
- indexing;
- availability;
- recall;
- retrieval;
- reconstruction;
- migration;
- repair;
- retention;
- archival;
- expiration;
- deletion;
- reclamation.

Transport delivery or process completion must not be substituted for the semantic completion of a memory operation.

### 5.10 Retrying communication must not duplicate memory intent

Memory operations that may be retried through ACS or another permitted communication mechanism require stable operation identity and defined duplicate handling.

A repeated delivery may represent the same semantic operation.

The memory system must not silently convert transport retries into repeated storage, repeated deletion, repeated mutation, or conflicting retrieval state.

### 5.11 Security is architectural

Memory may contain sensitive cognitive, operational, personal, learned, or identity-related state.

Authentication, authorization, confidentiality, integrity, replay resistance, provenance validation, compartmentalization, revocation, and key durability must therefore be treated as architectural concerns.

Security must not be added only after storage and retrieval behavior are complete.

Authentication of a participant does not automatically authorize every memory operation.

### 5.12 Node remains coherent

The memory architecture exists to support continuity of one organism across a changing physical substrate.

No ordinary physical machine, storage device, process, or connection is cognitively irreplaceable by definition.

Critical memory identity, authority, metadata, and recovery behavior must be designed so that individual failures generally produce explicit degradation rather than silent amnesia or false continuity.

## 6. Architectural scope

The public MEM specification family governs the architectural intent of:

- logical memory identity;
- memory-service roles;
- storage proposals;
- validation and acceptance;
- memory versions and lineage;
- provenance;
- retrieval and recall semantics;
- retrieval-result semantics;
- availability and completeness;
- stale, partial, unknown, unavailable, and conflicting states;
- durability expectations;
- distributed custody;
- physical placement independence;
- locality and migration;
- replication and reconstruction abstractions;
- consistency expectations;
- recovery and repair;
- retention and expiration;
- deletion and forgetting;
- garbage-collection safety;
- memory-specific resource budgets;
- memory-specific admission and deferral;
- memory lifecycle;
- interaction with ACS;
- conformance and failure validation.

Later MEM specifications may refine these responsibilities without contradicting the intent established here.

## 7. Non-goals

The public memory architecture does not attempt to:

- define cognitive learning algorithms;
- define which experiences Node should remember;
- define salience, curiosity, emotion, or attention heuristics;
- define proprietary memory-selection behavior;
- define exact weight-state reconstruction mechanisms;
- define model architectures;
- define latent indexing techniques;
- define exact consolidation or decay algorithms;
- define one mandatory storage engine;
- define one mandatory database;
- define one mandatory consistency protocol;
- define one mandatory replication strategy;
- define one mandatory erasure-coding system;
- define exact production capacity allocations;
- define exact production topology;
- define production operator recovery procedures;
- invent cryptographic algorithms;
- replace ACS communication responsibilities;
- replace runtime scheduling;
- replace storage-device health systems;
- replace physical-node lifecycle management;
- define robotic behavior or actuator safety;
- require that every transient intermediate value become durable memory;
- guarantee survival against every imaginable simultaneous failure.

These exclusions preserve clear boundaries without weakening the requirement for robust public memory abstractions.

## 8. Boundary with ACS

### 8.1 ACS responsibility

ACS governs how participants communicate.

Its responsibilities include:

- logical relationships;
- connections;
- endpoints;
- ports;
- signal semantics;
- payload references;
- admission;
- mediation;
- connection lifecycle;
- communication security;
- communication backpressure;
- routing and transport independence.

### 8.2 MEM responsibility

MEM governs what memory-service interactions mean.

Its responsibilities include:

- the meaning of a storage proposal;
- the meaning of acceptance or rejection;
- the meaning of a recall request;
- the meaning of a retrieval result;
- memory identity;
- provenance;
- versioning;
- retention;
- consistency;
- availability;
- reconstruction;
- recovery;
- memory lifecycle.

### 8.3 Integration rule

Memory services may own ACS endpoints and expose typed ACS ports.

Memory operations may be requested, reported, mediated, authorized, or observed through ACS relationships and signals.

MEM defines the semantic contract of those operations.

ACS defines how the participants establish permitted communication and how associated signals or payload references are carried.

Neither subsystem may silently absorb the responsibilities of the other.

### 8.4 Required separations

The following distinctions shall remain explicit:

1. A memory is not a connection.
2. A relationship is not stored state.
3. A payload reference is not durable memory identity.
4. Delivery is not semantic acceptance.
5. Authentication is not memory authorization.
6. Communication backpressure is not memory admission.
7. Connection lifetime is not memory lifetime.
8. Transport state is not memory state.
9. A retry is not automatically a new memory operation.
10. Communication failure is not proof that memory is absent.

Later specifications shall turn these distinctions into enforceable invariants and operation contracts.

## 9. Relationship to adjacent systems

### 9.1 Runtime and scheduler systems

Runtime systems determine where memory-service work executes and which physical resources are available.

MEM defines the meaning and required behavior of memory operations independently of their current execution host.

A scheduler may move a memory service without inherently changing the identity of the memories it serves.

### 9.2 Storage systems

Storage systems provide physical persistence mechanisms.

MEM governs the logical objects, lifecycle, provenance, validation, and continuity requirements implemented through those mechanisms.

A storage device or database does not independently determine memory meaning.

### 9.3 Security systems

Security systems provide identity, authentication, authorization primitives, key custody, cryptographic protection, and revocation mechanisms.

MEM defines the sensitivity, authority, integrity, and continuity expectations that those systems must support for memory operations.

Memory-service implementations must not directly assume unrestricted custody of long-term private keys.

### 9.4 Health and immune systems

Health and immune systems may observe:

- corruption;
- unexplained divergence;
- failed validation;
- suspicious access;
- repeated unauthorized requests;
- inconsistent replicas;
- stale custodians;
- abnormal deletion behavior;
- reconstruction failure.

Memory systems may provide evidence and request evaluation.

They must not independently acquire unrestricted authority to quarantine, erase, rebuild, or destroy physical nodes.

### 9.5 Cognitive systems

Cognitive systems create, interpret, use, associate, and request memory.

MEM provides governed services and semantics without prescribing the cognitive algorithms that decide:

- what is significant;
- what should be recalled;
- what should be consolidated;
- what should influence reasoning;
- what should be forgotten.

## 10. Specification hierarchy

The public MEM specification family is hierarchical:

- **MEM-0000** defines purpose, philosophy, scope, boundaries, and enduring design direction.
- **MEM-0001** defines core terminology and enforceable invariants.
- **MEM-0002** defines memory-service roles and authority boundaries.
- **MEM-0003** defines logical memory identity, provenance, and versioning.
- **MEM-0004** defines memory operation contracts and outcomes.
- **MEM-0005** defines availability, completeness, conflict, and consistency expectations.
- **MEM-0006** defines retention and memory lifecycle.
- **MEM-0007** defines distributed custody, locality, migration, and protection.
- **MEM-0008** defines recovery, repair, validation, and reconstruction.
- **MEM-0009** defines integration with ACS.
- **MEM-0010** defines conformance and failure validation.

Lower-numbered specifications carry broader architectural authority.

Later specifications may clarify or specialize earlier intent. They must not silently contradict it.

When a conflict is discovered, the specification set must be deliberately corrected, revised, deprecated, or superseded before the conflict is normalized in implementation.

## 11. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements.

The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification.

The word **may** describes permitted behavior.

A planned specification stub does not create mandatory requirements merely by naming a future subject. Normative authority begins when the relevant requirement is explicitly drafted and reviewed.

## 12. Engineering discipline

Node follows specification-driven engineering for foundational memory architecture.

The intended progression is:

1. identify the problem;
2. define architectural boundaries;
3. record open questions;
4. draft public abstractions;
5. challenge assumptions;
6. revise;
7. approve;
8. implement;
9. validate under failure;
10. optimize.

Concepts precede interfaces.

Interfaces precede implementations.

Implementations may change frequently. Foundational memory meaning and continuity guarantees should change carefully and transparently.

Each significant architectural concept should answer:

1. **What is it?**
2. **Why does it exist?**
3. **What does it enable?**
4. **What does it forbid?**
5. **How does it fail?**
6. **How is uncertainty represented?**

Every abstraction must earn its existence by preserving an important distinction, reducing ambiguity, containing failure, or enabling behavior that a simpler model cannot safely provide.

## 13. Initial architectural commitments

The MEM specification family begins with the following commitments:

1. Important memory shall not depend exclusively on one ordinary physical node.

2. Logical memory identity shall remain independent of physical placement.

3. Memory semantics shall remain separate from ACS communication mechanics.

4. Delivery shall remain distinct from acceptance, commitment, durability, and retrieval success.

5. Memory absence shall remain distinct from unavailability, uncertainty, incomplete search, and communication failure.

6. Required provenance and version information shall be preserved.

7. Memory operations shall be bounded and resource-accounted.

8. Retried communication shall not silently duplicate memory intent.

9. Corruption, staleness, conflict, and under-protection shall be represented explicitly.

10. Recovery and repair shall be designed before production durability is claimed.

11. Retention and deletion shall be governed lifecycle operations rather than incidental storage behavior.

12. Security, authorization, integrity, and key continuity shall be architectural concerns.

13. Individual failures should normally cause explicit degradation rather than broad silent memory loss.

14. Public architectural requirements shall remain independent of proprietary cognitive policy and production-sensitive procedure.

These commitments will be refined into enforceable invariants in MEM-0001.

## 14. Open research questions

The following subjects remain open areas of investigation:

- appropriate memory-class taxonomy;
- boundaries between working, short-term, episodic, semantic, procedural, and continuity-critical state;
- promotion from transient state into governed memory;
- durability expectations for rapidly changing working state;
- consistency models appropriate to different memory classes;
- distributed catalog and metadata authority;
- conflict representation and resolution;
- provenance depth and retention;
- memory reconstruction across hardware generations;
- placement across correlated failure domains;
- locality-aware retrieval;
- bounded replication and repair;
- checkpoint and journal relationships;
- partial recall semantics;
- retrieval under partition;
- safe lifecycle transitions;
- deliberate forgetting;
- deletion authorization;
- protection against stale-state resurrection;
- validation of reconstructed state;
- interaction between memory importance and finite resources;
- recovery when required decryption material is unavailable;
- behavior when the system cannot satisfy declared durability;
- safe operation when only a reduced subset of memory services is available.

These open questions are not architectural failures.

They are explicitly identified research directions. Later specifications may resolve them without violating this charter.

## 15. Future evolution

The public MEM architecture is expected to evolve through:

- new specifications;
- careful refinement;
- implementation experiments;
- failure testing;
- recovery exercises;
- reviewed architectural change.

Where practical, specifications should be extended rather than replaced.

Compatibility should be preserved when doing so does not compromise correctness, security, continuity, or architectural integrity.

Draft status permits substantial revision.

Stable or frozen status should be granted only after the relevant ideas have survived review and, where appropriate, practical validation under failure.

## 16. Closing principle

> **Node must not mistake the continued operation of its machines for the continued existence of its memory.**

Memory continuity requires more than surviving hardware.

It requires that meaningful state remain identifiable, interpretable, honestly available, and recoverable while the physical substrate changes beneath it.

## Revision history

### Version 0.1 — 2026-07-15

- Established the initial public Memory Architecture Charter.
- Defined memory continuity independently of physical placement.
- Established the boundary between memory semantics and ACS communication mechanics.
- Defined initial scope, non-goals, adjacent-system responsibilities, and specification hierarchy.
- Recorded the initial architectural commitments and open research questions.
- Adopted Node as the public project identity throughout the MEM series.
