# Adaptive Connection Substrate Specifications

The **Adaptive Connection Substrate (ACS)** specification series defines the architectural intent governing persistent relationships, communication, adaptation, trust, and resilience within Project Prometheus.

ACS specifications are written before production implementation so that code serves a deliberate architecture rather than silently defining it.

## Specification authority

The series is hierarchical:

1. **ACS-0000 — Charter** defines enduring philosophy, purpose, scope, and design direction.
2. **ACS-0001 — Core Principles and Invariants** defines enforceable architectural rules.
3. Later specifications refine individual ACS concerns without contradicting higher-authority specifications.

Lower-numbered specifications carry broader architectural authority. Later specifications may refine earlier documents but must not silently contradict them.

## Current specifications

| Specification | Title | Status |
|---|---|---|
| [ACS-0000](ACS-0000-charter.md) | Adaptive Connection Substrate Charter | Draft |
| [ACS-0001](ACS-0001-core-principles.md) | Core Principles and Invariants | Draft |
| [ACS-0002](ACS-0002-relationship-classes.md) | Relationship Classes | Draft |
| [ACS-0003](ACS-0003-signal-taxonomy.md) | Signal Taxonomy | Draft |
| ACS-0004 | Endpoints and Ports | Planned |
| ACS-0005 | Connection Lifecycle | Planned |
| ACS-0006 | Admission and Budgets | Planned |
| ACS-0007 | Security and Trust | Planned |
| ACS-0008 | Immune Integration | Planned |
| ACS-0009 | Runtime Integration | Planned |

## Document status

ACS documents use the following status values:

- **Draft:** under active review and subject to substantial revision.
- **Stable:** accepted architectural guidance suitable for implementation.
- **Frozen:** foundational intent that should change only through explicit supersession.
- **Deprecated:** retained for history but no longer recommended.
- **Superseded:** replaced by a named later specification.

## Prometheus engineering method

Each architectural concept should answer four questions:

1. **What is it?** — definition.
2. **Why does it exist?** — design rationale.
3. **What does it enable?** — architectural consequences.
4. **What does it forbid?** — boundaries and prohibited coupling.

Ideas remain inexpensive and open to challenge. Decisions become architecture only after deliberate review.

> Before writing the organism, define the conditions under which it can remain healthy, coherent, and capable of growth.
