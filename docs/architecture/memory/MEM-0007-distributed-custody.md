# MEM-0007: Distributed Custody and Locality

| Field | Value |
|---|---|
| Specification | MEM-0007 |
| Title | Distributed Custody and Locality |
| Status | Planned |
| Version | 0.0 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-15 |
| Approval | Scope reserved; normative content not yet drafted |
| Depends on | MEM-0000 through MEM-0006 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | Scope only |

> **Replica count alone is not resilience; custody must account for correlated failure, locality, finite resources, and explicit under-protection.**

## Planning notice

This file reserves the public responsibility and boundaries of MEM-0007.

It is not yet a normative specification. Listed subjects identify future work and must not be treated as settled implementation requirements.

## Purpose

Define public abstractions for distributed custody, physical placement independence, locality, replication, migration, and failure-domain-aware protection.

## This specification will define

- logical custody and physical placement
- replica and reconstruction-shard abstractions
- failure domains and correlated-loss awareness
- locality, migration, rebalance, and repair triggers
- under-protected and over-budget states
- bounded placement and replication behavior

## This specification will not define

- exact production node maps
- fixed replica or erasure-coding parameters
- private capacity-allocation policies
- storage-vendor-specific behavior

## Known architectural boundaries

- no important memory may depend exclusively on one ordinary physical node
- copies in one failure domain do not provide independent protection
- placement policy must expose unmet durability rather than claim success

## Initial open questions

- which failure domains every implementation must model
- when full replication versus reconstruction coding is appropriate
- how locality goals are balanced against correlated-failure risk

## Revision history

### Version 0.0 — 2026-07-15

- Reserved the scope of MEM-0007.
- Recorded its public classification, dependencies, boundaries, and initial questions.
