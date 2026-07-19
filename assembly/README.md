# Node assembly environment

This tree contains the public mechanisms used to construct and inspect a Node
system generation. It does not own release acceptance, generation membership,
activation authority, durable installation, or production policy.

The intended startup relationship is:

```text
Linux kernel
    -> Node micro-OS PID 1
    -> public Node runtime substrate
    -> public ACS substrate
    -> optional externally supplied runtime components
```

The public ACS implementation remains in `src/core/acs/`. It is not duplicated
under `assembly/`.

## Directory map

- `ram_assembly_p0/` is the bounded RAM-only kernel and initramfs mechanism
  proof.
- `micro_os/` states the early-userspace responsibility boundary.
- `init/` states PID 1 sequencing and handoff requirements.
- `profiles/` describes public, replaceable assembly inputs.
- `manifests/public/` defines the bounded external-component declaration
  format and carries a non-operational example.
- `providers/` implements public manifest parsing and validation.
- `tests/` contains ABI, parser, public-only, and negative-path conformance
  coverage.
- `../interfaces/` contains the versioned C-compatible component ABI.

## Public and external boundary

This repository contains public runtime code, public ACS substrate, public
interfaces, and public assembly mechanisms. Deployment-specific components,
policy, topology, trust decisions, routing, credentials, and production
behavior remain outside this repository.

An external component is an assembly-generation sibling. It is not a Linux
kernel module. Its manifest may describe an entry or launch mechanism, but the
manifest parser never invokes it. File presence or successful structural
validation establishes none of the following:

- acceptance;
- generation membership;
- authority;
- activation;
- readiness.

Those decisions require explicit results from their owning architecture. The
public-only assembly path remains complete when no external manifest is
supplied.

RAM Assembly P0 is governed by its public `AUTHORITY_NOT_REQUIRED` conformance
contract. That declaration applies only to disposable, tmpfs-only,
non-production mechanism proof. Its provider outputs remain ASM candidates
and cannot become KRN artifact acceptance, BOOT artifact acceptance,
generation members, installation input, activation or recovery material,
runtime-ready code, or normal Node code.

## Kernel source boundary

Linux kernel source remains external. RAM Assembly P0 accepts an explicitly
selected local source tree, Git source bundle, or repository URL together with
an immutable reviewed revision. It copies the selected revision into the RAM
workspace; it does not vendor kernel source into this repository.

## Local inputs

`.node-local/` is ignored for operator-owned local inputs. Nothing in this
repository scans that path automatically. A component manifest is considered
only when its exact path is supplied through the documented command-line
argument or environment variable.
