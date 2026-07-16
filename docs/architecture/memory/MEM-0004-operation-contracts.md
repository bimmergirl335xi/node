# MEM-0004: Memory Operation Contracts

| Field | Value |
|---|---|
| Specification | MEM-0004 |
| Title | Memory Operation Contracts |
| Status | Planned |
| Version | 0.0 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-15 |
| Approval | Scope reserved; normative content not yet drafted |
| Depends on | MEM-0000, MEM-0001, MEM-0002, MEM-0003 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | Scope only |

> **Transport delivery is only communication progress; memory acceptance and retrieval require explicit semantic outcomes.**

## Planning notice

This file reserves the public responsibility and boundaries of MEM-0004.

It is not yet a normative specification. Listed subjects identify future work and must not be treated as settled implementation requirements.

## Purpose

Define typed semantic contracts for proposing storage, validating, recalling, retrieving, updating, retaining, and deleting memory.

## This specification will define

- stable operation identity and idempotency
- storage proposal and validation outcomes
- acceptance, rejection, commitment, and deferral
- recall request scope and retrieval-result semantics
- mutation and deletion proposal behavior
- duplicate, timeout, cancellation, and retry handling

## This specification will not define

- ACS routing, authentication, attachment, or retransmission mechanics
- private recall arbitration
- concrete request encodings
- production timeout values

## Known architectural boundaries

- delivery acknowledgment is not semantic acceptance
- transport retry must not silently duplicate memory intent
- not-found must remain distinct from unavailable or incomplete search

## Initial open questions

- which operations require idempotency versus explicit uniqueness
- how partial retrieval results report searched scope
- when an accepted proposal becomes durable

## Revision history

### Version 0.0 — 2026-07-15

- Reserved the scope of MEM-0004.
- Recorded its public classification, dependencies, boundaries, and initial questions.
