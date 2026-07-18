# KRN-0003: Kernel and Micro-OS Compatibility Model

| Field | Value |
|---|---|
| Specification | KRN-0003 |
| Title | Kernel and Micro-OS Compatibility Model |
| Status | Planned |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | KRN-PUB |
| Depends on | KRN-0000 through KRN-0002; applicable BOOT architecture |

> **Placeholder — this document declares compatibility meaning; it does not authorize boot or installation.**

## Reserved purpose

Define the identities and compatibility declarations needed to reason about a kernel and its accompanying minimal operating environment without collapsing them into one object.

## Reserved identity layers

```text
KernelProfileIdentity
MicroOsProfileIdentity
InitramfsProfileIdentity
KernelMicroOsCompatibilityDeclaration
RequiredComponentSet
AssemblyEvidenceReference
```

## Required distinction

KRN may declare that a kernel profile and micro-OS profile are compatible under a stated scope, required component set, firmware assumption, ABI expectation, and validation boundary.

BOOT owns evaluation of a selected kernel/micro-OS pair as a scoped assembly and owns progression toward installer, activation, recovery, and runtime handoff.

The same kernel may participate in more than one declared pairing only when compatibility is demonstrated rather than assumed.
