# Node Memory Architecture Specifications

The **Node Memory Architecture (MEM)** specification series defines the public architectural intent governing logical memory identity, memory-service roles, storage and recall semantics, provenance, retention, consistency, distributed custody, reconstruction, recovery, and lifecycle within Node.

MEM specifications are written before production implementation so that code serves a deliberate architecture rather than silently defining what memory means.

## Classification

| Field | Value |
|---|---|
| Work-item family | MEM-PUB |
| Classification | PUBLIC-ARCHITECTURE |
| Repository branch | `main` |
| Owned path | `docs/architecture/memory/` |

This directory contains only independently authored public architecture.

Public specifications must not be produced by deleting, sanitizing, or paraphrasing restricted designs. Production-sensitive mechanisms, proprietary cognitive policy, and operator recovery details are outside this public series.

## Specification authority

The series is hierarchical:

1. **MEM-0000 — Memory Architecture Charter** defines enduring philosophy, purpose, scope, and design direction.
2. **MEM-0001 — Core Principles and Invariants** defines enforceable architectural rules.
3. Later specifications refine individual memory concerns without contradicting higher-authority specifications.

Lower-numbered specifications carry broader architectural authority. Later specifications may refine earlier documents but must not silently contradict them.

## Boundary with ACS

ACS defines how participants communicate through relationships, endpoints, ports, signals, payload references, admission, mediation, lifecycle, security, backpressure, and transport-independent connections.

MEM defines what memory services and memory operations mean.

A memory service may own an ACS endpoint and expose typed ACS ports. MEM defines the semantics of operations such as storage proposal, acceptance, recall, retrieval, retention, repair, and deletion. It does not redefine routing, authentication, attachment handling, secure sessions, or transport behavior.

The following separations are foundational:

- a memory is not a connection;
- a relationship is not stored state;
- a payload reference is not durable memory identity;
- delivery is not semantic acceptance;
- authentication is not memory authorization;
- communication backpressure is not memory admission;
- connection lifetime is not memory lifetime;
- transport state is not memory state;
- a retry is not automatically a new memory operation;
- communication failure is not proof that memory is absent.

## Current specifications

| Specification | Title | Status |
|---|---|---|
| [MEM-0000](MEM-0000-charter.md) | Memory Architecture Charter | Draft |
| [MEM-0001](MEM-0001-core-principles.md) | Core Principles and Invariants | Planned |
| [MEM-0002](MEM-0002-memory-roles.md) | Memory Service Roles and Boundaries | Planned |
| [MEM-0003](MEM-0003-identity-and-versioning.md) | Logical Identity, Provenance, and Versioning | Planned |
| [MEM-0004](MEM-0004-operation-contracts.md) | Memory Operation Contracts | Planned |
| [MEM-0005](MEM-0005-availability-and-consistency.md) | Availability, Completeness, and Consistency | Planned |
| [MEM-0006](MEM-0006-retention-and-lifecycle.md) | Retention and Memory Lifecycle | Planned |
| [MEM-0007](MEM-0007-distributed-custody.md) | Distributed Custody and Locality | Planned |
| [MEM-0008](MEM-0008-recovery-and-reconstruction.md) | Recovery, Repair, and Reconstruction | Planned |
| [MEM-0009](MEM-0009-acs-integration.md) | ACS Integration | Planned |
| [MEM-0010](MEM-0010-conformance.md) | Conformance and Failure Validation | Planned |

## Document status

MEM documents use the following status values:

- **Planned:** responsibility and boundaries are reserved, but normative architecture has not yet been drafted.
- **Draft:** under active review and subject to substantial revision.
- **Stable:** accepted architectural guidance suitable for implementation.
- **Frozen:** foundational intent that should change only through explicit supersession.
- **Deprecated:** retained for history but no longer recommended.
- **Superseded:** replaced by a named later specification.

A planned stub preserves scope and sequencing. It does not make the listed subjects normative.

## Publication gate

Before a MEM document is committed publicly, it must pass:

1. public-source and independent-authorship review;
2. ACS-boundary review;
3. restricted-information contamination review;
4. terminology and numbering review;
5. security and production-detail review;
6. substantive approval;
7. a fresh read of the target file and current `main`.

Undecided material is not public material.

## Node engineering method

Each architectural concept should answer:

1. **What is it?**
2. **Why does it exist?**
3. **What does it enable?**
4. **What does it forbid?**
5. **How does it fail?**
6. **How is uncertainty represented?**

Ideas remain inexpensive and open to challenge. Decisions become architecture only after deliberate review.

> Before implementing memory, define the conditions under which Node can remember, recover, and forget without mistaking machine survival for continuity.
