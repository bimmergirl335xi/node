# Public runtime interfaces

`node_external_component_v1.h` is the smallest public C-compatible declaration
contract currently needed for an optional runtime component. All ABI-carried
categories use fixed-width integer fields. Text and reference storage is
caller-owned, inline, length-tagged, and bounded; no allocator ownership or
language exception crosses the boundary.

The v1 declaration includes:

- component identity, opaque declaration revision reference, and ABI
  major/minor;
- public component class;
- launch kind and bounded entry reference;
- bounded required and provided public capability references;
- bounded dependency references;
- provenance and integrity references.

The accompanying parser result is declaration-specific. It reports only not
evaluated, not supplied, structurally valid, malformed, ABI incompatible,
unsupported, or indeterminate, plus a bounded failure and detail. It does not
contain provider launch, process-start, runtime activation, or readiness
lifecycle state. Those subjects require separate future contracts.

The ABI does not encode deployment policy, topology, endpoints, routing,
credentials, trust thresholds, BOOT acceptance, generation membership,
authority, activation, or readiness.

ABI v1 accepts major version 1 and minor versions through 0. A higher major or
minor is incompatible until a public implementation explicitly supports it.
