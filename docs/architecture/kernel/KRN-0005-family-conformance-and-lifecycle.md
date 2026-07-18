# KRN-0005: Kernel Family Conformance and Lifecycle

| Field | Value |
|---|---|
| Specification | KRN-0005 |
| Title | Kernel Family Conformance and Lifecycle |
| Status | Planned |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | KRN-PUB |
| Depends on | KRN-0000 through KRN-0004 |

> **Placeholder — lifecycle states and evidence requirements remain to be reviewed.**

## Reserved purpose

Define how a kernel profile is proposed, validated, released, maintained, restricted, superseded, deprecated, retired, revoked, or replaced.

## Candidate lifecycle vocabulary

- proposed;
- experimental;
- validated;
- released;
- maintained;
- restricted;
- deprecated;
- retired;
- revoked;
- superseded.

## Reserved conformance evidence

- attributable upstream, patch, configuration, and toolchain revisions;
- architecture and hardware-class evidence;
- kernel/micro-OS compatibility evidence;
- required and optional capability results;
- security-support and maintenance eligibility;
- boot and headless-operation evidence;
- failure and resource-exhaustion behavior;
- known limitations and unsupported states;
- revalidation after source, compiler, package, firmware, or configuration changes.

A retired or revoked profile must not silently return to eligible status. BOOT consumes profile lifecycle evidence but retains its own assembly and handoff decisions.
