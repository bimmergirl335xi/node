# ASM-0001: Assembly-Kernel Invariants

| Field | Value |
|---|---|
| Specification | ASM-0001 |
| Title | Assembly-Kernel Invariants |
| Status | Planned |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ASM-PUB |
| Depends on | ASM-0000; KRN-0001; applicable BOOT invariants |

> **Placeholder — candidate invariant areas require cross-series review before numbering.**

## Candidate invariant areas

- the assembly station is not the assembled Node;
- assembly capability is not installation authority;
- the running assembly kernel may differ from the kernel being assembled;
- hardware discovery is not profile eligibility or registration;
- source presence is not source acceptance or permission to compile;
- compilation success is not artifact verification;
- artifact verification is not installation or activation;
- local root or device access does not create Node authority;
- partial and interrupted work remains partial or indeterminate;
- cached material requires profile-scoped revalidation;
- inputs and outputs retain bounded provenance;
- resource exhaustion and unsupported hardware remain explicit;
- the environment remains headless and preserves a recovery or retry path.

## Boundary

ASM invariants constrain the assembly-kernel role. They must not duplicate BOOT’s canonical semantic operation, authority, installer, recovery, or handoff invariants.
