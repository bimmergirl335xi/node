# Node Assembly-Kernel Architecture Specifications

The **ASM** subseries defines the first concrete role within the Node kernel family: the assembly kernel and its accompanying assembly environment.

The assembly kernel exists to boot a minimal custom environment capable of inspecting the local substrate and providing the mechanisms needed to construct eligible Node artifacts. The assembly environment is not the assembled Node, the installer, BOOT, or the normal runtime.

## Classification

| Field | Value |
|---|---|
| Work-item family | `ASM-PUB` |
| Classification | `PUBLIC-ARCHITECTURE` |
| Parent series | `KRN-PUB` |
| Authoring lane | `lane/docs` |
| Owned path | `docs/architecture/kernel/assembly/` |

## Foundational boundary

> **ASM defines the assembly-kernel role and the capabilities of its environment. BOOT owns the governed assembly, authority, artifact, installer, activation, recovery, reconciliation, and runtime-handoff lifecycle.**

## Reserved specifications

| Specification | Title | Status |
|---|---|---|
| [ASM-0000](ASM-0000-assembly-kernel-role-charter.md) | Assembly-Kernel Role Charter | Planned |
| [ASM-0001](ASM-0001-assembly-kernel-invariants.md) | Assembly-Kernel Invariants | Planned |
| [ASM-0002](ASM-0002-assembly-environment-contract.md) | Assembly Environment Contract | Planned |
| [ASM-0003](ASM-0003-hardware-inspection-mechanisms.md) | Hardware Inspection Mechanisms | Planned |
| [ASM-0004](ASM-0004-build-capability-and-toolchain-boundary.md) | Build Capability and Toolchain Boundary | Planned |
| [ASM-0005](ASM-0005-resource-and-workspace-model.md) | Resource and Workspace Model | Planned |
| [ASM-0006](ASM-0006-kernel-specific-failure-model.md) | Kernel-Specific Failure Model | Planned |
| [ASM-0007](ASM-0007-assembly-kernel-conformance.md) | Assembly-Kernel Conformance | Planned |

## Explicit exclusions

ASM does not independently define:

- semantic assembly-operation identity or assembly generations;
- candidate boot disposition;
- bootstrap identity or authority;
- artifact acceptance, release, or installation eligibility;
- installer mutation or rollback;
- activation or recovery completion;
- reboot or normal-runtime handoff;
- ACS relationship or transport semantics;
- MEM truth or IMM verdicts.

Those meanings remain with BOOT and the other owning architectures.

## Placeholder status

These files reserve names and scope only. They do not authorize implementation.
