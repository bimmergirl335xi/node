# KRN-0000: Node Kernel Family Charter

| Field | Value |
|---|---|
| Specification | KRN-0000 |
| Title | Node Kernel Family Charter |
| Status | Planned |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | KRN-PUB |
| Authoring lane | `lane/docs` |
| Depends on | Applicable ACS, BOOT, MEM, and IMM public architecture |

> **Placeholder — scope is reserved; normative prose has not been drafted.**

## Reserved purpose

Define why Node maintains a family of compatible kernels rather than one permanently universal kernel, and define what makes a kernel profile Node-compatible.

## Reserved scope

- upstream kernel lineage versus Node kernel source;
- kernel role and hardware/software era;
- kernel profile, build, artifact, and deployed-instance identity;
- relationship to micro-OS profiles and validated assemblies;
- common ownership boundaries;
- public/private and licensing boundaries;
- maintenance, supersession, retirement, and replacement.

## Boundary that must remain intact

KRN owns kernel-family meaning and profile declarations. BOOT retains boot disposition, dynamic assembly generations, authority evaluation, installer handoff, activation, recovery, reconciliation, and runtime-handoff eligibility.

## Drafting gate

Before promotion to Draft, review against BOOT-0000 through BOOT-0004, ACS runtime integration, MEM authority, IMM authority, installer ownership, security-provider ownership, and applicable upstream licensing obligations.
