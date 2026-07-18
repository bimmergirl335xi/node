# Assembly conformance tests

The assembly tests compile the C ABI provider and verify:

- a bounded valid declaration populates the expected v1 fields;
- no manifest is a successful public-only condition;
- command-line and environment inputs work independently;
- simultaneous input sources are rejected as ambiguous;
- malformed and incomplete declarations are rejected;
- incompatible ABI versions and unsupported classes are rejected;
- capability-count overflow produces a bounded failure result;
- successful validation still reports no acceptance, membership, authority,
  activation, or readiness.

The tests are registered in the repository CTest suite and the principal
public-only and negative cases also run under RAM Assembly P0 `make validate`.
