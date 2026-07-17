# Node Immune Architecture Specifications

The **Node Immune Architecture (IMM)** specification series defines the public architectural contracts governing observation, evidence, assessment, recommendation, authorized protective action, containment, recovery verification, restoration, and audit within Node.

IMM specifications are written before production implementation so that protective behavior remains bounded, attributable, reversible where practical, and subordinate to established authority rather than being defined accidentally by private detection logic or runtime convenience.

## Classification

| Field | Value |
|---|---|
| Work-item family | IMM-PUB |
| Classification | PUBLIC-ARCHITECTURE |
| Repository branch | `main` |
| Owned path | `docs/architecture/immune/` |

This directory contains independently authored public architecture.

Public IMM specifications must not be produced by deleting, sanitizing, paraphrasing, or selectively redacting restricted designs. Private threat models, heuristics, thresholds, evidence-correlation rules, production response procedures, credentials, topology, and operator playbooks remain outside this public series.

## Specification authority

The series is hierarchical:

1. **IMM-0000 — Charter and Scope** defines enduring purpose, scope, ownership boundaries, classification boundaries, state vocabulary, and prohibited authority collapse.
2. **IMM-0001 — Core Invariants** defines mandatory architectural rules that every conforming public or private implementation must preserve.
3. Later IMM specifications may refine evidence contracts, assessment contracts, containment coordination, restoration verification, audit, and conformance without contradicting higher-authority specifications.

Lower-numbered specifications carry broader architectural authority. Later specifications may refine earlier documents but must not silently contradict them.

## Foundational boundaries

IMM defines protective interpretation and coordination contracts. It does not absorb the responsibilities of adjacent architecture:

- **ACS** remains authoritative for participant identity, relationships, endpoints, ports, signals, admission, budgets, connection lifecycle, security and trust, capabilities, delegation, revocation, and ACS enforcement.
- **MEM** remains authoritative for memory identity, persistence, provenance, retention, custody, reconstruction, recovery, deletion, and memory-operation outcomes.
- **Adaptive state** remains authoritative for its descriptors and governed mutation semantics.
- **Runtime systems** remain responsible for concrete execution and enforcement within explicitly granted authority.
- **Resource management** remains responsible for resource allocation, accounting, ceilings, reservations, pressure, and reclamation.
- **Bootstrap systems** remain responsible for establishing the initial verified substrate, enrollment, identity, and startup or recovery prerequisites assigned to them.

IMM may observe, assess, recommend, request, coordinate, and verify within declared scope. It may invoke privileged action only through authority granted by the owning architecture.

## Current specifications

| Specification | Title | Status |
|---|---|---|
| [IMM-0000](IMM-0000-charter-and-scope.md) | Charter and Scope | Draft |
| [IMM-0001](IMM-0001-core-invariants.md) | Core Invariants | Draft |

## Document status

IMM documents use the following status values:

- **Planned:** responsibility and boundaries are reserved, but normative architecture has not yet been drafted.
- **Draft:** under active review and subject to substantial revision.
- **Stable:** accepted architectural guidance suitable for implementation.
- **Frozen:** foundational intent that should change only through explicit supersession.
- **Deprecated:** retained for history but no longer recommended.
- **Superseded:** replaced by a named later specification.

## Publication gate

Before an IMM document is committed publicly, it must pass:

1. independent-authorship and public-source review;
2. ACS authority-boundary review;
3. MEM authority-boundary review;
4. adaptive-state, runtime, resource-management, and bootstrap boundary review;
5. restricted-information contamination review;
6. private-heuristic and threshold review;
7. terminology, numbering, and state-model review;
8. security, privacy, and production-detail review;
9. substantive approval;
10. a fresh read of the target file and current `main`.

Undecided private mechanism is not public architecture.

## Node engineering method

Each architectural concept should answer:

1. **What is it?**
2. **Why does it exist?**
3. **What does it enable?**
4. **What does it forbid?**
5. **Who owns the authority?**
6. **How does it fail?**
7. **How is uncertainty represented?**
8. **How is restoration verified?**

> Protection is trustworthy only when evidence, authority, action, and recovery remain distinguishable.
