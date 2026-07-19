# P0 provider and candidate-output records

P0 creates bounded JSON records beneath the tmpfs `records/` workspace. The
records preserve three distinct identities:

- the P0 conformance operation;
- its ASM provider operation;
- its ASM provider attempt.

The provider-operation record references provider/toolchain identity,
reviewed kernel-source revision, configuration evidence, resource limits and
outcome, candidate outputs, cleanup state, limitations, and unresolved
effects. Each kernel or initramfs candidate record identifies its provider,
output class, tmpfs path or handle, digest, size, provider-validation state,
cleanup state, limitations, and unresolved effects.

These records are provider evidence only. They contain no KRN artifact
acceptance, BOOT scoped artifact acceptance, generation membership,
installation eligibility, activation, recovery, or readiness decision.
`examples/` contains inert machine-readable examples used by host validation.
