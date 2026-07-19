# P0 conformance authority contract

`p0-conformance-authority-v1.json` is the owning public contract that declares
RAM Assembly P0 `AUTHORITY_NOT_REQUIRED` under BOOT-0005 section 14.1. It
applies only to the isolated, bounded, non-production conformance operation
implemented in this directory.

The contract identifies permitted source, configuration, and test inputs;
candidate-output classes; RAM and execution limits; tmpfs-only retention;
cleanup and disposal; and every prohibited downstream use. P0 outputs cannot
be accepted, installed, activated, propagated, retained as production
material, executed as normal Node code, or substituted for an authorized
build.

P0 has no BOOT recovery plan or BOOT semantic operation. It creates no KRN
artifact acceptance, BOOT scoped artifact acceptance, assembly generation,
installed instance, activation result, recovery result, handoff result, or
runtime-readiness result. Its operation identities are P0 conformance and ASM
provider identities only.
