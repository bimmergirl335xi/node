# Public external-component manifest v1

This line-oriented format maps one bounded candidate declaration into
`node_component_declaration_v1`. Blank lines and lines beginning with `#` are
ignored. Every other line is exactly one `key=value` pair; unknown keys and
duplicate scalar keys are rejected.

Required scalar keys are:

```text
manifest_version=1
component_id=<bounded public identifier>
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

The manifest carries declarations only. Validation does not verify referenced
content, accept a component, assign it to a generation, grant authority,
launch it, or establish readiness. `example.manifest` is inert public test
data and contains no usable artifact.
