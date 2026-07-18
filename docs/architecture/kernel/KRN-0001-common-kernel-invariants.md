# KRN-0001: Common Kernel Invariants

| Field | Value |
|---|---|
| Specification | KRN-0001 |
| Title | Common Kernel Invariants |
| Status | Planned |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | KRN-PUB |
| Depends on | KRN-0000 and applicable adjacent architecture |

> **Placeholder — invariant identifiers and normative wording are not yet approved.**

## Reserved purpose

Define the mandatory rules shared by every Node kernel profile regardless of architecture, role, era, implementation mechanism, or deployment environment.

## Candidate invariant areas

- no one kernel is permanently universal;
- role and era remain explicit;
- kernel identity exceeds a version string;
- kernel and micro-OS identities remain distinct;
- physical control location does not determine semantic ownership;
- kernel mediation does not create architectural authority;
- hardware discovery does not create Node registration;
- driver availability does not grant device-use authority;
- telemetry is observation rather than assessment or verdict;
- unknown, unsupported, degraded, unavailable, and conflicting remain explicit;
- privileged or early execution does not enlarge authority;
- patch sets remain attributable to a declared requirement and profile;
- security and maintenance state affect eligibility;
- retirement and replacement remain explicit and continuity-aware.

## Drafting gate

Numbered invariants must be reconciled with BOOT-0001, ACS-0001 and ACS-0009, MEM invariants, IMM-0001, resource authority, installer authority, and device-backend boundaries before approval.
