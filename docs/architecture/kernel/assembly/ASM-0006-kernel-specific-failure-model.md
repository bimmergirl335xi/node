# ASM-0006: Assembly-Kernel-Specific Failure Model

| Field | Value |
|---|---|
| Specification | ASM-0006 |
| Title | Assembly-Kernel-Specific Failure Model |
| Status | Planned |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ASM-PUB |
| Depends on | ASM-0000 through ASM-0005; KRN-0001; BOOT-0002 |

> **Placeholder — this document covers mechanism failures, not the complete BOOT assembly lifecycle.**

## Reserved purpose

Define failures attributable to the assembly kernel or assembly environment and how those failures are reported without fabricating BOOT, installer, MEM, IMM, or runtime conclusions.

## Candidate failure classes

- unsupported or partially supported hardware;
- unavailable driver or kernel mechanism;
- filesystem or storage-access failure;
- compiler, linker, or build-tool execution failure;
- process-supervision failure;
- network or offline-input unavailability;
- workspace or memory exhaustion;
- device-control failure;
- evidence-collection failure;
- shutdown, reboot, or handoff-mechanism failure.

## Boundary

ASM reports provider-scoped mechanism outcomes. BOOT retains multidimensional operation state, indeterminate mutation, retry, reconciliation, installer, recovery, and runtime-handoff meanings.
