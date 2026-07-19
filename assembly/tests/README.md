# Assembly conformance tests

The assembly tests compile the C ABI provider and verify:

- a bounded valid declaration populates the expected v1 fields and opaque
  declaration revision reference;
- no manifest is a successful public-only condition;
- command-line and environment inputs work independently;
- simultaneous input sources are rejected as ambiguous;
- malformed and incomplete declarations are rejected;
- incompatible ABI versions and unsupported classes are rejected;
- capability-count overflow produces a bounded failure result;
- declaration parser states remain separate from launch and runtime lifecycle;
- P0 provider and candidate-output examples contain bounded correlation,
  resource, digest, cleanup, limitation, and unresolved-effect evidence
  without downstream acceptance claims.

The tests are registered in the repository CTest suite and the principal
public-only and negative cases also run under RAM Assembly P0 `make validate`.
