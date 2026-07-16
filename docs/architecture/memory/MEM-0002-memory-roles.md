# MEM-0002: Memory Service Roles and Boundaries

| Field | Value |
|---|---|
| Specification | MEM-0002 |
| Title | Memory Service Roles and Boundaries |
| Status | Planned |
| Version | 0.0 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-15 |
| Approval | Scope reserved; normative content not yet drafted |
| Depends on | MEM-0000, MEM-0001 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | Scope only |

> **A memory-service role describes responsibility and authority, not a permanent process, machine, or topology position.**

## Planning notice

This file reserves the public responsibility and boundaries of MEM-0002.

It is not yet a normative specification. Listed subjects identify future work and must not be treated as settled implementation requirements.

## Purpose

Define the logical roles that propose, validate, retain, locate, retrieve, reconstruct, repair, and govern memory without making any physical node irreplaceable.

## This specification will define

- memory proposer and consumer roles
- custodian, catalog, validator, retrieval, and recovery responsibilities
- role authority and separation
- role migration, coexistence, and degraded operation
- minimum evidence required to claim a role is available

## This specification will not define

- fixed production deployment topology
- one-process-per-role requirements
- ACS endpoint or route mechanics
- private cognitive arbitration and capacity policy

## Known architectural boundaries

- roles are logical and may share or change physical hosts
- role names do not grant unrestricted authority
- service availability must not be inferred solely from connection state

## Initial open questions

- which roles are foundational versus optional
- which authorities must be separated for safety
- how reduced Node operation behaves when some roles are unavailable

## Revision history

### Version 0.0 — 2026-07-15

- Reserved the scope of MEM-0002.
- Recorded its public classification, dependencies, boundaries, and initial questions.
