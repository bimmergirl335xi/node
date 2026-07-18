# Public assembly providers

`external_component_manifest.c` is a bounded parser for the public v1
declaration format. It fills caller-owned ABI structures, rejects unknown or
over-limit input, and returns a structured lifecycle/failure result.

`manifest_validator_main.c` exposes the parser as
`node_component_manifest_validator`. Use either:

```sh
node_component_manifest_validator --manifest /exact/path/to/manifest
```

or:

```sh
NODE_EXTERNAL_COMPONENT_MANIFEST=/exact/path/to/manifest \
  node_component_manifest_validator
```

Supplying both is rejected. Supplying neither succeeds with `not_supplied`,
which is the normal public-only result. The provider performs no directory
scan, network access, repository fetch, artifact lookup, integrity
dereference, acceptance decision, or launch.
