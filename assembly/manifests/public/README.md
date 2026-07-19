# Public external-component manifest v1

This line-oriented format maps one bounded candidate declaration into
`node_component_declaration_v1`. Blank lines and lines beginning with `#` are
ignored. Every other line is exactly one `key=value` pair; unknown keys and
duplicate scalar keys are rejected.

Required scalar keys are:

```text
manifest_version=1
component_id=<bounded public identifier>
declaration_revision_ref=<bounded opaque exact revision reference>
abi_major=1
abi_minor=0
component_class=runtime|service|provider|diagnostic
launch_kind=none|executable|entrypoint
provenance_ref=<bounded opaque reference>
integrity_ref=<bounded opaque reference>
```

`entry` is required for `executable` and `entrypoint`, and forbidden for
`none`. `required_capability`, `provided_capability`, and `dependency` may each
repeat only up to the ABI limit. Values are bounded printable references; this
validator does not dereference them.

The result state is declaration-specific: not evaluated, not supplied,
structurally valid, malformed, ABI incompatible, unsupported, or
indeterminate. Structural validity says only that the bounded declaration can
be represented by this ABI. The opaque revision reference keeps selection and
correlation from relying on `component_id` alone.

The manifest carries declarations only. Validation does not verify referenced
content, accept a component, assign it to a generation, grant authority,
launch it, activate it, or establish readiness. Future launch, process,
activation, and readiness results require separate provider-owned contracts.
`example.manifest` is inert public test data and contains no usable artifact.
