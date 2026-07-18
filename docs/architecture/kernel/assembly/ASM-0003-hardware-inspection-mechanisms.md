# ASM-0003: Hardware Inspection Mechanisms

| Field | Value |
|---|---|
| Specification | ASM-0003 |
| Title | Hardware Inspection Mechanisms |
| Status | Planned |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ASM-PUB |
| Depends on | ASM-0000 through ASM-0002; KRN-0004; BOOT local-inspection contracts |

> **Placeholder — inspection produces evidence, not registration, authority, or verdict.**

## Reserved purpose

Define the kernel and assembly-environment mechanisms that expose bounded local evidence concerning the substrate.

## Candidate evidence areas

- CPU architecture, features, topology, and execution capacity;
- memory capacity and relevant constraints;
- storage and boot mode;
- PCIe and device topology;
- accelerators and network devices;
- firmware and driver observations;
- IOMMU and security-hardware observations;
- build capacity, thermal limits, and power limits;
- existing installation and recovery-path observations.

## Mandatory distinction

```text
hardware observed
    != Node object registered
    != ACS participant created
    != backend admitted
    != driver selected
    != device authorized
    != health or compatibility verdict
```

BOOT owns how inspection evidence contributes to candidate disposition and assembly evaluation.
