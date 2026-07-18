# KRN-0002: Kernel Role, Era, and Profile Model

| Field | Value |
|---|---|
| Specification | KRN-0002 |
| Title | Kernel Role, Era, and Profile Model |
| Status | Planned |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | KRN-PUB |
| Depends on | KRN-0000, KRN-0001 |

> **Placeholder — role and era catalogs remain proposals, not commitments.**

## Reserved purpose

Define three independent dimensions:

```text
Role
    what the kernel is expected to do

Era
    which hardware, firmware, ABI, toolchain, package, and maintenance generation it supports

Profile
    the exact attributable realization of one role and era
```

## Reserved topics

- role identity and role-specific requirements;
- architecture-family and hardware-class eligibility;
- firmware, driver, toolchain, package, and userspace ABI generation;
- patch-set and configuration revisions;
- expected micro-OS profile;
- required, optional, unsupported, and unknown capabilities;
- known limitations and validation state;
- maintenance and support era.

## Boundary

KRN declares profile facts and compatibility claims. BOOT selects and evaluates candidate assemblies under BOOT-owned evidence, authority, state, and handoff contracts.

Exact year ranges and hardware-era labels must not be frozen without evidence.
