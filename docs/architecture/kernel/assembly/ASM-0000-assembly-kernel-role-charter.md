# ASM-0000: Assembly-Kernel Role Charter

| Field | Value |
|---|---|
| Specification | ASM-0000 |
| Title | Assembly-Kernel Role Charter |
| Status | Planned |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ASM-PUB |
| Parent series | KRN-PUB |
| Depends on | KRN-0000 through KRN-0005; BOOT-0000 through BOOT-0004 |

> **Placeholder — the role is reserved; normative responsibilities are not yet drafted.**

## Reserved purpose

Define the first concrete Node kernel role: boot a minimal custom assembly environment and provide the mechanisms needed to inspect a substrate and construct eligible Node artifacts.

## Candidate responsibilities

- initialize enough hardware for the assembly environment;
- provide storage and filesystem access required by the profile;
- provide networking when available without requiring it for every path;
- expose bounded hardware evidence;
- support source, package, compiler, linker, test, and staging mechanisms;
- support structured diagnostics, shutdown, reboot, and BOOT/installer handoff mechanisms.

## Non-responsibilities

The assembly kernel does not independently establish Node identity, ACS membership, bootstrap authority, release authority, installation eligibility, durable installer mutation, activation, recovery, or runtime readiness.
