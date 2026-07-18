# ASM-0002: Assembly Environment Contract

| Field | Value |
|---|---|
| Specification | ASM-0002 |
| Title | Assembly Environment Contract |
| Status | Planned |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ASM-PUB |
| Depends on | ASM-0000, ASM-0001; KRN-0003; applicable BOOT minimal-environment requirements |

> **Placeholder — this document defines environment capability, not BOOT progression.**

## Reserved purpose

Define what the assembly kernel and accompanying micro-OS must provide for the assembly role.

## Candidate required capabilities

- deterministic headless startup;
- bounded machine-readable status;
- storage and target-storage observation;
- required filesystem support;
- network capability when available;
- offline source and package-cache access where supported;
- compiler, linker, and build-tool execution;
- process supervision;
- security-provider access for verification results;
- bounded evidence and log retention;
- explicit time uncertainty;
- shutdown, reboot, and recovery-path preservation.

## Not inherently required

A desktop, display server, graphical installer, normal Node runtime, Julia, unrestricted package ecosystem, permanent networking, or local human-readable console.
