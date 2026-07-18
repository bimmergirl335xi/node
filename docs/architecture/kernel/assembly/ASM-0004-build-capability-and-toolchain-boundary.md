# ASM-0004: Build Capability and Toolchain Boundary

| Field | Value |
|---|---|
| Specification | ASM-0004 |
| Title | Build Capability and Toolchain Boundary |
| Status | Planned |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ASM-PUB |
| Depends on | ASM-0000 through ASM-0003; KRN-0002 and KRN-0003; applicable BOOT artifact boundaries |

> **Placeholder — mechanism availability is not input acceptance, release authority, or artifact eligibility.**

## Reserved purpose

Define the facilities the assembly environment may provide for source acquisition, package access, compilation, linking, testing, artifact construction, and staging.

## Candidate capability declarations

- compiler and linker families and generations;
- build tools and language runtimes;
- headers, SDKs, firmware, and binary dependencies;
- package repository and snapshot access mechanisms;
- offline media and peer-provided input mechanisms;
- source and artifact cache mechanisms;
- cross-compilation and target-generation support;
- test and validation execution capability.

## Boundary

ASM declares what build mechanisms are available and how they fail. BOOT and future artifact, release, security, package, and update architecture retain input eligibility, authority, provenance acceptance, compatibility evaluation, and progression decisions.
