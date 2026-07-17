# MEM-0010: Conformance and Failure Validation

| Field | Value |
|---|---|
| Specification | MEM-0010 |
| Title | Conformance and Failure Validation |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Accepted as initial working draft |
| Depends on | MEM-0000 through MEM-0009; applicable ACS specifications |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in claim scope, evidence requirements, and mandatory failure categories; standardized profiles and long-duration validation thresholds remain under review |

> **Durability and continuity are claims to be demonstrated under failure, not labels granted by successful normal operation.**

## Architectural-intent notice

This specification defines the public evidence required for an implementation or deployment to claim conformance with the MEM specification series.

It defines conformance subjects, scopes, levels, capability profiles, test environments, fault models, evidence, result classifications, semantic validation, fault injection, partition testing, storage-loss testing, corruption testing, stale-state testing, duplicate-operation testing, lifecycle testing, deletion and resurrection testing, custody and failure-domain testing, recovery and reconstruction testing, ACS integration testing, degraded-operation testing, long-duration validation, unsupported-behavior disclosure, revalidation, and public reporting.

It does not define vendor certification, a mandatory test framework, programming language, CI platform, exact production thresholds, exact production topology, private failure-domain maps, operator procedures, internal memory algorithms, private cognitive-state formats, cognitive quality, intelligence, physical-actuation safety certification, or deployment-specific legal compliance.

Implementations may use unit, integration, simulation, property, model-checking, physical hardware, long-duration, controlled deployment, or mixed validation methods. The method may vary; required semantic evidence must remain equivalent.

## 1. Purpose

The MEM specifications define what Node memory must mean. MEM-0010 defines what an implementation must demonstrate before claiming it preserves those meanings.

A memory system can appear correct during ordinary operation while failing when a request is retried; a process crashes after commitment but before response; a catalog becomes stale; a custodian disappears; several copies share one failure domain; a cache returns an obsolete version; deletion is only partially propagated; an old backup is restored; a journal has a gap; a checkpoint is incomplete; a service reconnects with stale authority; payload transfer fails after semantic completion; authentication succeeds but memory authorization fails; capacity is exhausted; a key or schema disappears; repair uses corrupt source material; a partition permits concurrent versions; or an unavailable dependency is reported as not found.

Happy-path testing cannot establish continuity.

A conforming implementation must prove that it fails honestly, degrades explicitly, preserves uncertainty, rejects stale authority, avoids duplicate semantic effects, prevents accidental forgetting, protects deletion state, reconstructs only from validated evidence, and reports the limits of its guarantees.

## 2. Scope

This specification governs conformance claims concerning logical memory identity, versions, representations, provenance, lineage, roles, operation identity, idempotency, admission, commitment, retrieval, availability, completeness, freshness, consistency, conflict, retention, archival, deletion, tombstones, custody, failure domains, durability, repair, reconstruction, recovery, ACS integration, authorization, resources, degraded operation, and explicit unknown and indeterminate states.

It applies to libraries, runtime components, memory services, service groups, storage backends, catalogs, custodians, recovery systems, complete deployments, deployment profiles, and interoperability profiles.

A component may conform within a limited scope. It must not imply conformance for responsibilities it does not implement.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements. The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification. The word **may** describes permitted behavior.

A test passes when observed behavior satisfies the applicable requirement. Correct rejection, explicit degradation, unavailability, or uncertainty may be the required passing behavior. Success is not always the correct outcome.

## 4. Foundational distinctions

Implementation conformance is not deployment conformance. A correct implementation may be deployed without enough custodians, failure-domain independence, capacity, key protection, recovery evidence, or topology to satisfy a durability target.

Deployment conformance is scoped to configuration, software version, hardware environment, failure-domain model, memory classes, features, and evidence boundary.

Normal-operation success is not failure validation. Test coverage does not prove untested behavior. Unsupported optional capability is not nonconforming when disclosed; unsupported mandatory capability invalidates the claim. Not evaluated is not passed. Partial execution is not broad partial conformance. Fault injection is not identical to production failure. Simulation does not alone prove physical failure-domain independence. Long-running success does not replace deliberate fault testing. Absence of observed loss does not prove durability. Test-environment identity matters. Correct uncertainty is conforming behavior. MEM conformance is not cognitive evaluation.

## 5. Conformance subject

Every claim must identify the evaluated subject.

A component subject may be an identity library, operation journal, catalog, custodian, retrieval provider, lifecycle service, repair coordinator, or ACS integration layer. A service subject is one deployable logical service exercising one or more MEM roles. A composite subject combines cooperating services. A deployment subject includes configured software, hardware, storage, security dependencies, failure domains, and policies. A profile subject defines a repeatable configuration class and must state which deployment properties remain assumptions.

The subject must identify relevant implementation version, build identity, configuration version, schema versions, MEM versions, ACS contract versions, and enabled features.

## 6. Conceptual conformance records

```text
MemConformanceClaim
    claim_identity
    subject_identity
    subject_type
    implementation_version
    configuration_identity
    mem_specification_versions
    applicable_acs_versions
    conformance_level
    claimed_capabilities
    excluded_capabilities
    memory_classes
    durability_profiles
    tested_failure_domains
    assumed_failure_domains
    untested_failure_domains
    evidence_boundary
    evidence_age
    known_limitations
    claim_result
```

```text
MemConformanceTestResult
    test_identity
    requirement_references
    subject_identity
    environment_identity
    fault_model
    initial_state
    injected_or_observed_faults
    expected_behavior
    observed_behavior
    result_class
    evidence_references
    unresolved_uncertainty
    test_time
    reproducibility_information
```

```text
MemDeploymentEvidence
    deployment_identity
    topology_profile
    custodian_inventory
    failure_domain_model
    dependency_inventory
    enabled_memory_classes
    protection_targets
    exercised_faults
    recovery_objectives
    measured_recovery_results
    remaining_under_protection
    known_exclusions
```

These records define semantics, not mandatory encodings.

## 7. Conformance levels

### 7.1 Level 0 — Profile declared

The implementation documents supported specifications, capabilities, unsupported behavior, state model, known limitations, and intended failure behavior. Level 0 is preparation, not conformance.

### 7.2 Level 1 — Semantic conformance

The subject demonstrates that interfaces and state transitions preserve applicable MEM meanings under ordinary and edge-case testing. It must include identity distinctions, role boundaries, operation identity, duplicate handling, result-state separation, lifecycle separation, explicit unknown and conflict states, and ACS/MEM boundary preservation. Level 1 does not authorize a durability claim.

### 7.3 Level 2 — Failure-validated conformance

The subject satisfies Level 1 and demonstrates applicable mandatory behavior under interruption, duplicate delivery, timeout, stale state, partial availability, corruption, partition, resource pressure, lifecycle conflict, and recovery faults. A Level 2 claim applies only to the exercised fault model and environment.

### 7.4 Level 3 — Deployment continuity validation

A specific deployment satisfies Level 2 and demonstrates declared continuity and durability targets using the actual or materially equivalent topology, storage systems, failure domains, key dependencies, schemas, catalogs, recovery paths, and configuration.

Level 3 evidence must distinguish physical and simulated failures, measured recovery behavior, observed loss boundary, remaining assumptions, and untested correlated failures.

### 7.5 Level ordering and no inflation

A higher level includes lower-level requirements within the same scope. A component may be Level 2 while the deployment remains Level 1. Terms such as production-ready, resilient, durable, fault-tolerant, or continuity-preserving must not replace a scoped level and evidence statement.

## 8. Capability profiles

Claims may cover identity and versioning; operation contracts; retrieval state; lifecycle; custody and durability; recovery; ACS integration; or a declared composite of these profiles.

Interactions between claimed profiles must be tested where one affects another.

## 9. Claim scope requirements

Every public claim must state:

1. conformance subject;
2. conformance level;
3. MEM versions;
4. applicable ACS versions;
5. claimed capability profiles;
6. excluded capabilities;
7. tested memory classes;
8. tested operation types;
9. tested failure domains;
10. assumed failure domains;
11. untested failure domains;
12. test environment;
13. evidence date or boundary;
14. known limitations;
15. unresolved failures;
16. whether evidence is simulated, physical, or mixed.

A claim omitting material scope is not a valid public MEM conformance claim.

## 10. Result classifications

Results must distinguish pass, fail, not applicable, unsupported, not run, blocked, indeterminate, invalid test, and expected degraded result.

Unsupported is not permitted for a mandatory requirement of the claimed profile. Skipped, blocked, and indeterminate tests do not pass. A test may pass by producing explicit degraded, unavailable, partial, conflicting, stale, unknown, or indeterminate state when that is the required behavior.

## 11. Evidence requirements

A conformance result must have observable evidence. Design intent or source inspection alone is insufficient for failure-validated conformance.

Evidence may include structured logs, state snapshots, operation and lifecycle histories, custody observations, test records, integrity proofs, fault-injection records, recovery results, metrics, operation traces, model-checking output, or physical test records.

Evidence must identify the test, subject, environment, and observation boundary; should be protected against unnoticed modification; and must not be silently replaced by a pass when missing.

Public reports may redact memory content, topology, credentials, participant identities, exact failure-domain locations, and proprietary details. Redaction must not conceal whether a requirement was tested, which result occurred, which assumptions remain, or whether evidence was simulated or physical.

## 12. Test-environment requirements

The environment must identify relevant hardware, operating system or runtime, storage, network model, service topology, memory configuration, security dependencies, clock model, and fault mechanism.

Tests must establish initial logical memory, versions, lifecycle, custody, operation history, connection state, and failure-domain state. Fault tests should be isolated from unrelated retained memory and production services.

Where practical, tests should preserve deterministic inputs, fault schedule, seed, configuration, expected state, and evidence method. Non-deterministic systems may use repeated trials and statistical reporting, but rare failures must not be hidden by averages. Instrumentation effects must be considered.

## 13. Fault-model declaration

Each fault test must declare what is modeled. Fault classes may include process crash or pause, host loss, device loss, storage unavailability or corruption, network partition, message loss, duplication, delay, reordering, clock discontinuity, key unavailability, schema unavailability, catalog loss, stale restore, resource exhaustion, incompatible software, authority revocation, partial operation, interrupted migration, interrupted deletion, or interrupted recovery.

The claim must distinguish injected, simulated, physically exercised, and naturally observed failures.

## 14. Mandatory semantic tests

Every Level 1 claim must test applicable foundational distinctions:

- logical identity versus placement;
- version versus representation;
- memory versus ACS signal, payload reference, endpoint, port, relationship, and connection;
- role versus physical service;
- delivery versus signal acceptance, memory admission, commitment, durability, and completion;
- authentication versus authorization;
- availability versus completeness;
- stale versus invalid;
- expiration versus deletion;
- logical deletion versus reclamation and sanitization;
- repair versus reconstruction;
- preservation of unknown, partial, unavailable, conflicting, and indeterminate states.

## 15. Operation identity and retry tests

Repeated identical operation identity must not create duplicate semantic effects. Same identity with changed intent must produce conflict or rejection. A new identity may represent a new operation even with identical content, subject to policy.

Lost response after success must reconcile with the completed operation. Timeout after possible commitment must remain pending or indeterminate. Retry after reconnect must not recreate proposals, mutations, deletion, retention changes, or repair. Duplicate repair must not create uncontrolled replicas. Duplicate deletion must not broaden scope or erase newer unrelated state. Journal replay must not apply one semantic operation twice.

## 16. Availability and not-found tests

Required tests include unreachable custodian, incomplete catalog, missing index entry, truncated search, authorization-limited search, unknown membership, direct identity with missing copies, stale cache, and conflicting recall.

Not found may be returned only when the required authoritative search completed within the declared scope. Infrastructure failure, incomplete coverage, timeout, authorization limits, unavailable dependencies, and conflict must retain their own states.

## 17. Consistency and partition tests

Tests must verify requested versus achieved consistency, refusal or explicit fallback when stronger consistency is unavailable, partitioned reads, partitioned mutation, concurrent branches, partition healing, and rejection of first-wins, fastest-wins, nearest-wins, or timestamp-only authority.

## 18. Corruption and integrity tests

Tests must cover corrupt representations, corrupt repair sources, shared corruption, semantic corruption, corrupt manifests, corrupt catalogs, quarantine, and unavailable validation.

Corrupt, invalid, or unverified material must not silently serve as valid current memory or repair source.

## 19. Custody and failure-domain tests

Tests must cover duplicate physical references, same-host copies, shared storage, shared key authority, unknown independence, claimed-domain loss, explicit under-protection, impossible protection targets, migration, interrupted migration, custodian departure, and bounded over-placement.

Replica or shard count must not substitute for failure-domain and dependency evidence.

## 20. Resource-pressure tests

Tests must cover storage exhaustion, queue exhaustion, retrieval pressure, repair storms, per-source isolation, protected control capacity, pressure-versus-deletion separation, and pressure-versus-trust separation.

Assigned durable memory must not disappear silently. Resource limits must produce bounded and explicit behavior.

## 21. Lifecycle and deletion tests

Tests must cover expiration without automatic deletion, holds, hold release, deletion-stage separation, offline-custodian propagation, rejoining deleted copies, tombstone durability, backup restoration, identifier non-reuse, physical-copy deletion versus logical deletion, logical deletion versus sanitization, source versus derived memory, restoration, and stale lifecycle rejection.

## 22. Recovery and reconstruction tests

Tests must cover restart versus recovery; valid recovery source selection; stale and conflicting sources; incomplete checkpoints; journal gaps; duplicate-safe replay; external-side-effect suppression; exact, semantic, approximate, and partial reconstruction; missing provenance; missing schema; missing key; rejoining custodian; availability versus durability recovery; and degraded resumption.

A surviving copy is not automatically authoritative. Reconstruction does not become restoration without validation and reconciliation.

## 23. ACS integration tests

Tests must cover endpoint versus role availability, attachment versus memory admission, signal delivery versus signal acceptance, signal acceptance versus MEM outcome, payload transfer versus semantic retrieval, connection loss before and after possible delivery, attachment revocation, connection closure, ACS backpressure, local fast-path equivalence, and mediation.

ACS connectivity and lifecycle must not redefine MEM semantic state.

## 24. Authorization tests

Tests must cover authenticated-but-unauthorized participants, wrong-subject authority, recall-versus-mutation authority, custody without disclosure, lifecycle authority separation, revoked authority, locality not bypassing authorization, and mediator non-amplification.

## 25. Unknown, partial, conflict, and indeterminate tests

A conforming implementation must deliberately test non-success semantic states.

Remove enough evidence to require unknown; make only bounded scope available to require partial; provide incompatible valid-looking state to require conflict; interrupt after possible semantic effect to require indeterminate; make a required dependency unavailable while preserving existence evidence; return an older version while a newer version is known to require stale; remove authority while preserving reachability to require unauthorized; and exhaust soft semantic resources to require resource-deferred where policy permits reconsideration.

## 26. Durability validation

A durability claim must identify the failure conditions memory is intended to survive. Validation must include content, identity metadata, lineage, schema, manifest, lifecycle, tombstones, operation history, and keys or a key-recovery path.

Level 3 claims must exercise or justify material failure domains, including correlated dependencies. Durability under mutation and deletion should be tested. Protection evidence must have an observation age. Long under-protection periods must be reported. Prior testing does not prove current durability after topology, configuration, or dependency changes.

## 27. Recovery-objective reporting

MEM does not mandate universal recovery-time or loss-window targets. Claimed targets must report target recovery boundary, expected and measured loss window, expected and measured recovery duration, tested memory scope, tested fault, starting and ending protection, degraded-resumption point, full-recovery point, and unresolved limitations.

Measurements from one environment are not universal guarantees.

## 28. Long-duration validation

Long-duration reports should identify duration, workload, memory volume, operation mix, mutation rate, fault rate, repair activity, lifecycle activity, resource pressure, observed failures, and evidence gaps.

They should monitor duplicate operations, unresolved indeterminate state, tombstone growth, catalog drift, stale copies, repair backlog, under-protection, resource growth, and evidence retention.

No universal duration is mandated. Long-running success supplements but does not replace deliberate fault testing.

## 29. Model checking and formal methods

Formal methods may strengthen evidence for state-machine correctness, retry safety, stale-state rejection, lifecycle ordering, duplicate suppression, conflict preservation, boundedness, and recovery invariants.

Proof of a model does not prove implementation equivalence. Claims must identify modeled behavior, assumptions, properties, and uncovered implementation behavior.

## 30. Interoperability validation

Independently implemented components should validate schema compatibility, operation identity, result-state compatibility, unsupported-feature reporting, port-contract negotiation, payload references, retry, cancellation, unknown-state preservation, and version and lifecycle interpretation.

One implementation must not reinterpret another’s partial, stale, conflict, unknown, under-protected, or indeterminate state as success.

## 31. Negative and adversarial validation

Testing should include malformed identities, unsupported versions, conflicting operation payloads, replayed requests, stale connection generations, unauthorized payload references, oversized requests, unbounded queries, invalid shard sets, forged custody claims, inconsistent lifecycle evidence, repeated cancellation, and refusal to honor backpressure.

This validates architectural resilience but does not constitute complete security certification.

## 32. Test safety and containment

Fault injection must remain bounded to the declared scope. Testing on active deployments requires separate operational authorization. Destructive hardware testing is not required when a safer equivalent establishes the semantics, but Level 3 claims must disclose assumed equivalence.

Test-created memories, copies, journals, tombstones, and temporary payloads must be lifecycle-governed. Tests must not accidentally destroy the last valid copy outside an explicitly authorized exercise. Replay and fault injection must not cause unintended physical actions or external transactions.

## 33. Conformance-report requirements

A public report should include claim identity, subject identity, implementation version, configuration, MEM and ACS versions, level, capability profiles, unsupported features, environment, fault model, tested, assumed, and untested failure domains, results, failures, blocked and indeterminate tests, evidence references, limitations, evidence date, and revalidation triggers.

Reports may be human-readable, machine-readable, or both.

## 34. Honest partial implementation

An implementation supporting only part of MEM may still be useful. It must state which specifications and sections it implements; roles it exercises; operation families it supports; guarantees it does not provide; failures it has not tested; and states it cannot represent.

It must not claim broad “MEM compliant” status without a declared level and scope. A failed mandatory requirement narrows or invalidates the claim; it is not an undocumented exception.

## 35. Revalidation triggers

Conformance evidence must be reconsidered after material change to MEM implementation, operation journal, identity model, schema, serialization, storage engine, catalog behavior, consistency, lifecycle, tombstones, recovery, key system, failure-domain topology, custodian placement, ACS contract, retry, cancellation, resource limits, operating environment, dependencies, or hardware architecture.

Not every change requires every test. The report must identify which evidence remains applicable and why.

## 36. Regression requirements

A corrected failure should become a regression test where practical. Regression evidence should preserve the original failure class, reproducing condition, expected behavior, corrected behavior, and affected requirement.

A previously passing test that later becomes blocked or indeterminate must not remain reported as currently passing without explanation.

## 37. Conformance authority and independence

An implementation may publish self-evaluated results and must state that fact. Independent evaluation may provide stronger evidence, but MEM does not require one centralized certification authority.

Where practical, critical continuity claims should be reviewed or exercised independently of the component producing the claimed state. Conformance is not permanent trust; regression, environment drift, and revalidation remain possible.

## 38. Relationship to specification status

Claims must identify the status and version of specifications used. Draft-specification claims may change when the specification changes.

A revision may preserve evidence, require targeted revalidation, invalidate part of a claim, or introduce a new profile. Evidence gathered against an older materially different version must not silently support a newer claim.

## 39. Prohibited interpretations

This specification shall not be interpreted to mean that normal-operation success proves continuity; unit tests prove deployment durability; simulation proves physical failure-domain independence; long-duration success covers every fault; unsupported mandatory behavior may be ignored; skipped, blocked, or indeterminate tests pass; absence of observed corruption proves integrity; absence of observed loss proves durability; copy count proves domain resilience; one test framework is mandatory; public reporting must expose private content or topology; redaction may hide failed requirements; Level 1 permits durability claims; Level 2 proves every deployment configuration; Level 3 evidence applies forever; one environment represents every architecture; correct degradation is failure; service or refusal during partition is universally conforming or nonconforming; restored deleted copies are acceptable; authentication tests replace memory authorization tests; ACS connectivity tests replace MEM semantic tests; conformance proves cognitive quality; or public conformance requires private algorithms or production topology.

## 40. Initial mandatory baseline

Until specialized profiles are defined, a Level 2 claim covering the complete MEM architecture must include at least:

1. logical identity surviving physical relocation;
2. version and representation separation;
3. role and authority separation;
4. duplicate-safe operation retry;
5. same-operation-identity conflict detection;
6. timeout-after-possible-commit reconciliation;
7. strict not-found behavior;
8. partial-search reporting;
9. stale-result reporting;
10. conflict preservation;
11. requested-versus-achieved consistency;
12. resource-pressure behavior;
13. deletion-stage separation;
14. tombstone-based stale-resurrection prevention;
15. identifier non-reuse;
16. failure-domain-aware protection accounting;
17. explicit under-protection;
18. validated repair source selection;
19. checkpoint and journal-gap handling;
20. duplicate-safe replay;
21. rejoining-custodian reconciliation;
22. availability-versus-durability recovery separation;
23. ACS delivery-versus-MEM-outcome separation;
24. authentication-versus-memory-authorization separation;
25. connection-loss indeterminate-operation handling;
26. local-fast-path boundary equivalence;
27. explicit unknown, unavailable, partial, stale, conflict, and indeterminate states.

An implementation may claim narrower profiles when scope is explicit.

## 41. Open questions

- Which Level 2 tests should be mandatory for every component rather than only complete deployments?
- Should standardized profile names be introduced for identity, retrieval, lifecycle, custody, recovery, and ACS integration?
- Which failure-domain classes should be mandatory for Level 3 continuity claims?
- How should virtualized failure domains be validated?
- Which durability observations require periodic renewal?
- How should evidence age affect current claim validity?
- Which evidence should be machine-readable?
- Should public implementations expose a conformance-manifest endpoint?
- Which results may be aggregated without hiding rare failure?
- How should probabilistic or eventually convergent behavior be evaluated?
- How many trials are appropriate for non-deterministic faults?
- Which formal properties should be mandatory for critical state machines?
- How should long-duration reports be compared?
- Which recovery objectives should public deployment profiles standardize?
- How should unavailable physical hardware tests be reported?
- When is simulation materially equivalent to physical fault?
- Which changes mandate complete revalidation?
- How should independent implementations share reusable public fault suites?
- Which shared C++ test interfaces and fixtures should represent MEM states and fault injection?
- How should future private implementations publish public evidence without revealing restricted mechanisms?

These questions do not weaken the baseline evidence requirements.

## 42. Closing principle

> **Node must prove that it remembers correctly when the easy path has already failed.**

A successful write is not durability.

A successful read is not completeness.

A replica count is not independence.

A restart is not recovery.

A timeout is not failure.

A deletion request is not deletion completion.

A passing happy path is not continuity.

Conformance exists only where meaning survives the faults, uncertainty, and resource limits the claim says it can survive.

## Revision history

### Version 0.1 — 2026-07-16

- Replaced the planned MEM-0010 stub with the first normative working draft.
- Defined conformance subjects, claim records, levels, capability profiles, result classes, and evidence requirements.
- Distinguished implementation, component, profile, and deployment conformance.
- Established Level 0 profile declaration, Level 1 semantic conformance, Level 2 failure-validated conformance, and Level 3 deployment continuity validation.
- Defined mandatory semantic, retry, availability, consistency, corruption, custody, resource, lifecycle, deletion, recovery, ACS integration, authorization, and uncertainty tests.
- Established durability, recovery-objective, long-duration, formal-method, interoperability, negative, and adversarial validation requirements.
- Defined safe fault-injection and cleanup boundaries.
- Established public conformance-report contents, honest limited-scope claims, revalidation triggers, regression requirements, and evidence independence.
- Defined an initial mandatory baseline for complete Level 2 MEM claims.
- Preserved private implementation, production-topology, operator-procedure, and cognitive-quality boundaries.
