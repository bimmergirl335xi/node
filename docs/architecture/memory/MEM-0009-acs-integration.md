# MEM-0009: ACS Integration

| Field | Value |
|---|---|
| Specification | MEM-0009 |
| Title | ACS Integration |
| Status | Planned |
| Version | 0.0 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-15 |
| Approval | Scope reserved; normative content not yet drafted |
| Depends on | MEM-0000 through MEM-0008, applicable ACS specifications |
| Supersedes | None |
| Superseded by | None |
| Current confidence | Scope only |

> **MEM defines memory semantics through ACS communication without becoming a second endpoint, routing, security, or transport system.**

## Planning notice

This file reserves the public responsibility and boundaries of MEM-0009.

It is not yet a normative specification. Listed subjects identify future work and must not be treated as settled implementation requirements.

## Purpose

Define how public memory-service contracts bind to ACS participants, relationships, endpoints, ports, signals, payload references, admission, and backpressure.

## This specification will define

- memory-service participant and endpoint expectations
- semantic mapping of memory operations onto typed ACS interactions
- memory authorization boundaries after ACS authentication
- communication versus memory admission and backpressure
- retry, correlation, cancellation, and result-reporting boundaries
- failure-state translation without semantic collapse

## This specification will not define

- ACS endpoint or port mechanics
- route selection and mediation algorithms
- secure-session implementation
- attachment formats or transport retransmission

## Known architectural boundaries

- ACS remains the communication authority
- MEM remains the memory-semantic authority
- valid ACS delivery cannot be treated as proof of storage or retrieval success

## Initial open questions

- which memory contracts require dedicated typed ports
- how payload-reference authorization is bound to memory authorization
- how ACS cancellation maps to memory-operation lifecycle state

## Revision history

### Version 0.0 — 2026-07-15

- Reserved the scope of MEM-0009.
- Recorded its public classification, dependencies, boundaries, and initial questions.
