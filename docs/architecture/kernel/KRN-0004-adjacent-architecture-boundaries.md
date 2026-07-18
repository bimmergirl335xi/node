# KRN-0004: Adjacent Architecture Boundaries

| Field | Value |
|---|---|
| Specification | KRN-0004 |
| Title | Adjacent Architecture Boundaries |
| Status | Planned |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | KRN-PUB |
| Depends on | KRN-0000 through KRN-0003; applicable ACS, BOOT, MEM, and IMM architecture |

> **Placeholder — ownership must be resolved before implementation language is drafted.**

## Reserved purpose

Define how kernel mechanisms relate to BOOT, ACS, MEM, IMM, security providers, installers, runtime, resource management, device backends, firmware, package systems, and future update architecture.

## Foundational rule

> Kernel mediation, kernel implementation, direct userspace control, and direct device control are possible realization paths. None independently determines architectural ownership.

## Required distinctions

- a socket is not an ACS connection;
- a numeric port is not an ACS port;
- a mapped PCIe BAR is not a participant, endpoint, connection, or backend registration;
- a DMA queue is not semantic operation completion;
- a kernel event or interrupt is an observation, not a domain result;
- device access is not device-use authority;
- BOOT coordination is not kernel ownership;
- installer mutation is not BOOT or KRN mutation;
- kernel telemetry is not IMM assessment;
- kernel storage is not automatically MEM-governed semantic memory.

## Drafting artifact

This specification should contain a controller / semantic owner / authorizing domain / evidence producer / final verifier matrix for representative mechanisms.
