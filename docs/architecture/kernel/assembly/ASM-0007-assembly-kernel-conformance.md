# ASM-0007: Assembly-Kernel Conformance

| Field | Value |
|---|---|
| Specification | ASM-0007 |
| Title | Assembly-Kernel Conformance |
| Status | Planned |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ASM-PUB |
| Depends on | ASM-0000 through ASM-0006; KRN-0005; applicable BOOT conformance requirements |

> **Placeholder — conformance proves only the declared assembly-kernel profile and scope.**

## Reserved purpose

Define the evidence required to claim that an assembly-kernel profile and accompanying assembly environment satisfy their declared role.

## Candidate evidence

- headless boot on declared architectures;
- bounded startup and diagnostics;
- hardware-inspection evidence quality and unknown-state behavior;
- required storage and filesystem mechanisms;
- optional and unavailable network behavior;
- toolchain and build-capability declarations;
- workspace and resource-exhaustion behavior;
- direct-control and kernel-mediated mechanism boundaries;
- interruption and recovery-path preservation;
- no unauthorized installer, ACS, MEM, IMM, or runtime claims;
- declared limitations and unsupported cases.

ASM conformance does not establish that a particular Node assembly is compatible, authorized, installed, activated, recovered, or runtime-ready. Those results remain with BOOT and adjacent owners.
