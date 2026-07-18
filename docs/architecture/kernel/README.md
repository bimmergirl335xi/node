# Node Kernel-Family Architecture Specifications

The **KRN** series defines the public architecture of the Node-compatible kernel family.

KRN defines kernel roles, hardware/software eras, profile identity, kernel and micro-OS compatibility declarations, adjacent-system boundaries, and kernel-family conformance. It does not own BOOT coordination, installer mutation, ACS semantics, MEM truth, IMM verdicts, or normal-runtime readiness.

## Classification

| Field | Value |
|---|---|
| Work-item family | `KRN-PUB` |
| Classification | `PUBLIC-ARCHITECTURE` |
| Authoring lane | `lane/docs` |
| Owned path | `docs/architecture/kernel/` |
| Concrete first role | `docs/architecture/kernel/assembly/` |

## Foundational boundary

> **KRN defines what a kernel profile is and what mechanisms it provides. BOOT decides whether a declared kernel/micro-OS assembly is eligible to proceed through assembly, installation, activation, recovery, or runtime handoff.**

Physical control location does not determine architectural ownership. Kernel mediation, direct userspace control, device-runtime control, firmware control, and other authorized realization paths may implement mechanisms without redefining the semantic owner of the operation or object.

## Reserved specifications

| Specification | Title | Status |
|---|---|---|
| [KRN-0000](KRN-0000-kernel-family-charter.md) | Kernel Family Charter | Planned |
| [KRN-0001](KRN-0001-common-kernel-invariants.md) | Common Kernel Invariants | Planned |
| [KRN-0002](KRN-0002-role-era-and-profile-model.md) | Role, Era, and Profile Model | Planned |
| [KRN-0003](KRN-0003-kernel-and-micro-os-compatibility-model.md) | Kernel and Micro-OS Compatibility Model | Planned |
| [KRN-0004](KRN-0004-adjacent-architecture-boundaries.md) | Adjacent Architecture Boundaries | Planned |
| [KRN-0005](KRN-0005-family-conformance-and-lifecycle.md) | Family Conformance and Lifecycle | Planned |

The first concrete role subseries is indexed under [`assembly/`](assembly/README.md).

## Parallel-authoring rule

KRN, ASM, and BOOT may be drafted in parallel only when:

- KRN owns kernel-family identity, mechanisms, role, era, profile, maintenance, and conformance;
- ASM owns the assembly-kernel role and the capabilities of its accompanying assembly environment;
- BOOT retains semantic assembly generations, authority evaluation, artifact state, installer handoff, activation, recovery, reconciliation, and runtime-handoff eligibility;
- discovered conflicts are recorded and resolved before prose is treated as stable architecture.

## Placeholder status

These files reserve names and scope only. `Planned` does not mean the architecture is approved, complete, or ready for implementation.
