# ASM-0005: Assembly Resource and Workspace Model

| Field | Value |
|---|---|
| Specification | ASM-0005 |
| Title | Assembly Resource and Workspace Model |
| Status | Planned |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ASM-PUB |
| Depends on | ASM-0000 through ASM-0004; applicable resource-management and BOOT boundedness requirements |

> **Placeholder — this model governs assembly-environment resources, not global resource authority.**

## Reserved purpose

Define bounded workspace, temporary storage, cache, memory, CPU, device, network, process, and diagnostic requirements for the assembly-kernel role.

## Candidate topics

- required and optional workspace classes;
- volatile versus persistent workspace;
- scratch, cache, staging, evidence, and recovery-material separation;
- capacity estimation and explicit uncertainty;
- hard limits and resource-exhaustion outcomes;
- protected recovery-path capacity;
- cleanup, retry, and interruption behavior;
- target-storage isolation;
- concurrent assembly-work limits;
- headless diagnostics and bounded retained issues.

The assembly environment may enforce granted limits but does not create global allocation authority or exceed resource-management ceilings.
