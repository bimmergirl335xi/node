# MEM-0006: Retention and Memory Lifecycle

| Field | Value |
|---|---|
| Specification | MEM-0006 |
| Title | Retention and Memory Lifecycle |
| Status | Planned |
| Version | 0.0 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-15 |
| Approval | Scope reserved; normative content not yet drafted |
| Depends on | MEM-0000 through MEM-0005 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | Scope only |

> **Forgetting must be a governed lifecycle decision, not an accidental side effect of pressure, failure, or disconnected storage.**

## Planning notice

This file reserves the public responsibility and boundaries of MEM-0006.

It is not yet a normative specification. Listed subjects identify future work and must not be treated as settled implementation requirements.

## Purpose

Define memory lifecycle states and the public semantics of retention, expiration, archival, deletion, tombstones, and reclamation.

## This specification will define

- memory lifecycle states and transitions
- retention intent and expiration
- archival and reduced-availability states
- deletion proposals, authorization, and tombstones
- garbage-collection safety
- protection against stale-state resurrection

## This specification will not define

- private consolidation or decay algorithms
- private salience and selection heuristics
- exact retention durations
- operator production procedures

## Known architectural boundaries

- deliberate forgetting is distinct from accidental loss
- resource pressure must not silently erase protected memory
- reclamation must not precede validated lifecycle authority

## Initial open questions

- how lifecycle policy interacts with derived memories
- how deletion propagates through temporarily disconnected custodians
- which recovery points may delay physical reclamation

## Revision history

### Version 0.0 — 2026-07-15

- Reserved the scope of MEM-0006.
- Recorded its public classification, dependencies, boundaries, and initial questions.
