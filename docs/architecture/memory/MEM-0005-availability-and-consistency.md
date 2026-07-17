# MEM-0005: Availability, Completeness, and Consistency

| Field | Value |
|---|---|
| Specification | MEM-0005 |
| Title | Availability, Completeness, and Consistency |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | MEM-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Accepted as initial working draft |
| Depends on | MEM-0000 through MEM-0004 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in result-state separation and search-scope semantics; consistency profiles and degraded-read policy remain under review |

> **A memory result is trustworthy only when its availability, completeness, freshness, conflict state, and consistency expectations are explicit.**

## Architectural-intent notice

This specification defines how Node describes memory when memory services, catalogs, custodians, versions, or dependencies are:

- distributed;
- partially reachable;
- stale;
- divergent;
- under-protected;
- under repair;
- authorization-limited;
- incompletely searched;
- temporarily unavailable.

It defines the public architectural meaning of:

- availability;
- unavailability;
- degraded availability;
- unknown state;
- retrieval completeness;
- search completeness;
- result truncation;
- freshness;
- staleness;
- consistency expectations;
- consistency evidence;
- concurrent versions;
- conflict;
- authoritative absence;
- not found;
- partition behavior;
- conservative retrieval;
- stale-but-useful results;
- reduced-operation behavior.

It does not define:

- one mandatory consensus algorithm;
- one mandatory quorum system;
- fixed replica counts;
- fixed quorum sizes;
- one universal consistency model;
- one mandatory catalog architecture;
- one production topology;
- transport behavior;
- cognitive interpretation of retrieved content;
- private recall-selection policy;
- exact repair or replication algorithms.

An implementation may use different databases, journals, catalogs, indexes, caches, replicas, consistency protocols, and coordination mechanisms.

It must preserve the semantic distinctions established here.

## 1. Purpose

Distributed memory rarely operates under perfect knowledge.

At any moment:

- one custodian may be offline;
- one catalog may be stale;
- one replica may contain an older version;
- a repair may be incomplete;
- one branch may have committed during partition;
- a requester may lack authority to inspect part of the search space;
- an index may cover only part of a namespace;
- a cache may return a valid but stale representation;
- required schema or key material may be unavailable;
- a search may terminate because of a caller-imposed limit;
- the system may know that memory probably exists without being able to retrieve it.

A system that represents every such condition as either **success** or **failure** will eventually mislead itself.

A system that represents every missing result as **not found** will eventually mistake temporary blindness for forgetting.

MEM-0005 defines the evidence and semantic states required for honest memory retrieval.

The central rule is:

> **Every memory result must describe only what the operation actually established.**

## 2. Scope

This specification governs:

- memory availability claims;
- role and custodian availability as they affect memory;
- retrieval-result availability;
- physical representation availability;
- required dependency availability;
- catalog and index coverage;
- search-scope completion;
- result completeness;
- result truncation;
- result freshness;
- version currency;
- consistency expectations;
- consistency observations;
- conflict reporting;
- partition behavior;
- degraded reads;
- not-found decisions;
- stale-result handling;
- under-repair state;
- result evidence;
- uncertainty propagation.

It applies to:

- direct lookup by logical memory identity;
- lookup by version identity;
- retrieval by alias;
- catalog search;
- indexed search;
- semantic or structured recall;
- bounded multi-memory queries;
- retrieval from caches;
- retrieval from custodians;
- reconstructed results;
- historical-version retrieval;
- lifecycle-state queries;
- operation-status queries where memory evidence is involved.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements.

The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification.

The word **may** describes permitted behavior.

An implementation does not conform merely because it exposes one status field named `available`, `consistent`, or `complete`.

Its observable results must preserve the independent dimensions defined here.

## 4. Foundational distinctions

### 4.1 Existence is not availability

A logical memory may continue to exist while temporarily unavailable.

Unavailability does not establish:

- deletion;
- expiration;
- absence;
- invalidity;
- loss of identity;
- permanent loss.

### 4.2 Availability is not completeness

A service may successfully return one valid memory while failing to search the full requested scope.

The returned memory is available.

The retrieval operation may still be incomplete.

### 4.3 Completeness is not freshness

A result may contain every memory known within the required scope while all returned versions are stale relative to a newer committed state.

### 4.4 Freshness is not consistency

A recently created version may conflict with another recently created version.

Recency does not establish one authoritative history.

### 4.5 Consistency is not factual truth

A set of replicas may agree perfectly on incorrect or misleading content.

Consistency describes relationships among memory states and observations.

It does not prove that every proposition contained in memory is objectively true.

### 4.6 Conflict is not corruption

Two valid-looking concurrent versions may conflict without either being physically corrupted.

Conflict may arise from:

- partitioned operation;
- concurrent authorized mutation;
- competing corrections;
- incomplete lineage;
- incompatible authorities;
- unresolved merges.

### 4.7 Stale is not invalid

A stale version may remain:

- historically valid;
- useful for a bounded task;
- the newest locally available version;
- required for lineage or auditing.

It must not be presented as current when current state was requested.

### 4.8 Partial is not failure

A partial result may be legitimate when the operation contract permits partial completion.

Its incomplete scope must remain explicit.

### 4.9 Unknown is not unavailable

**Unavailable** means evidence establishes that required access cannot currently be provided.

**Unknown** means evidence is insufficient to determine whether required access can be provided.

### 4.10 Not found is not unknown

**Not found** is an evidence-backed negative result.

**Unknown** means the system cannot safely establish either presence or absence.

### 4.11 Authorization-limited is not absent

A requester may be unable to inspect a memory or part of a search scope.

That limitation must not become an internal claim that memory does not exist.

### 4.12 Under-protected is not unavailable

A memory may be readable while failing its declared durability requirements.

Availability and protection remain separate.

## 5. Multi-axis result model

A retrieval result shall not be represented solely by one vague state such as:

- success;
- healthy;
- present;
- valid.

A result may require several independent dimensions.

### 5.1 Primary operation outcome

The primary operation outcome is governed by MEM-0004.

Examples include:

- completed;
- partial;
- not found;
- unavailable;
- unauthorized;
- conflict;
- invalid;
- deferred;
- cancelled;
- indeterminate.

### 5.2 Availability dimension

Describes whether the requested memory material or evidence could be accessed.

### 5.3 Completeness dimension

Describes how much of the required search or result scope was covered.

### 5.4 Freshness dimension

Describes how current the returned state is relative to the request.

### 5.5 Consistency dimension

Describes which requested consistency expectation was achieved.

### 5.6 Conflict dimension

Describes whether incompatible valid-looking state was observed or remains possible.

### 5.7 Validation dimension

Describes which required integrity, schema, provenance, or authority checks were completed.

### 5.8 Protection dimension

Describes whether the returned memory currently satisfies its declared durability protection.

### 5.9 Authorization-disclosure dimension

Describes whether result scope or evidence was limited by authorization.

A result may therefore be:

- available;
- complete for a local catalog;
- stale;
- causally consistent;
- conflict-free within observed evidence;
- integrity-validated;
- under-protected;
- provenance-redacted.

That combination is more informative than one `SUCCESS` flag.

## 6. Conceptual result-state record

This section defines semantics, not a required wire format.

A memory result should make it possible to determine:

```text
MemoryResultState
    operation_identity
    primary_outcome
    requested_scope
    completed_scope
    incomplete_scope
    availability_state
    completeness_state
    freshness_state
    consistency_expectation
    consistency_achieved
    conflict_state
    validation_state
    protection_state
    authorization_limit_state
    evidence_observation_time
    evidence_references
    known_missing_dependencies
    result_finality
```

A search observation should make it possible to determine:

```text
SearchScopeEvidence
    query_or_subject_scope
    required_catalogs_or_participants
    participating_catalogs_or_participants
    unavailable_participants
    unknown_participants
    excluded_authorization_scopes
    search_limits
    truncation_state
    index_coverage
    observed_version_boundaries
    search_start_evidence
    search_completion_evidence
```

A consistency observation should make it possible to determine:

```text
ConsistencyEvidence
    subject_scope
    requested_expectation
    achieved_expectation
    observed_versions
    observed_lineage
    participating_authorities
    unavailable_authorities
    conflicting_claims
    observation_boundary
    evidence_age
```

Equivalent information may be:

- embedded in retrieval results;
- retained in operation history;
- represented through catalogs;
- reconstructed from journals;
- carried through governed references.

## 7. Availability model

### 7.1 Available

Memory is **available** when the requested operation can currently access the required memory material or evidence under the declared:

- identity;
- scope;
- authorization;
- representation;
- validation;
- consistency;
- resource conditions.

Availability is always relative to an operation.

A memory may be available for:

- metadata lookup;

while unavailable for:

- content retrieval;
- decryption;
- validated interpretation;
- mutation;
- historical reconstruction.

### 7.2 Degraded available

Memory is **degraded available** when useful access is possible but one or more requested non-essential or relaxable guarantees are not satisfied.

Examples include:

- stale-but-allowed representation;
- partial provenance;
- reduced catalog coverage;
- locally available copy while remote confirmation is unavailable;
- current content with under-protected durability;
- content available but preferred encoding unavailable.

The degraded conditions must be explicit.

### 7.3 Unavailable

Memory is **unavailable** when evidence establishes that a required operation cannot currently be performed.

Causes may include:

- no reachable eligible custodian;
- required key authority unavailable;
- required schema unavailable;
- required validation unavailable;
- required consistency participants unavailable;
- all known representations damaged;
- resource exhaustion;
- lifecycle policy blocking access;
- repair in progress with no safe readable source.

### 7.4 Unknown availability

Availability is **unknown** when the system lacks sufficient evidence to determine whether the requested operation can currently succeed.

Examples include:

- catalog coverage unknown;
- custodian state unknown;
- network partition with incomplete membership evidence;
- operation history unavailable;
- stale role observations;
- unresolved locator conflict.

### 7.5 Authorization-limited availability

A memory may be physically and operationally available while unavailable to a particular requester.

The result must distinguish:

- globally unavailable;
- unavailable within the requester's authority;
- existence concealed by policy;
- authorization decision unavailable.

### 7.6 Dependency-limited availability

Memory is not semantically available when required dependencies cannot be obtained.

Dependencies may include:

- schema;
- provenance;
- lineage;
- manifests;
- decryption authority;
- reconstruction information;
- validation evidence;
- identity metadata.

Raw bytes alone do not prove semantic availability.

### 7.7 Historical availability

A superseded or stale version may remain historically available even when it is not eligible to satisfy a request for current state.

### 7.8 Availability claim scope

Every availability claim must identify or imply:

- operation type;
- memory scope;
- representation requirements;
- consistency expectation;
- authorization context;
- observation boundary;
- evidence age.

### 7.9 Availability claims expire

Availability observations become stale.

A previously reachable custodian does not remain indefinitely available by assumption.

The system must not treat old reachability evidence as current without an explicit validity model.

### 7.10 Availability does not prove durability

A single readable copy may make memory available while leaving it severely under-protected.

## 8. Completeness model

### 8.1 Result completeness

Result completeness describes whether all requested result material was returned.

### 8.2 Search completeness

Search completeness describes whether all sources required by the operation contract were adequately searched or evaluated.

These are distinct.

A search may be complete and return no results.

A search may be incomplete and still return several valid results.

### 8.3 Complete for declared scope

A result is **complete** only relative to a declared scope.

Examples include:

- complete for one logical memory identity;
- complete for one catalog shard;
- complete for one namespace;
- complete for one consistency snapshot;
- complete for all authorized visible memories;
- complete for all currently participating custodians.

The result shall not imply broader completeness.

### 8.4 Partial

A result is **partial** when some required or requested scope was not completed.

The result must identify the incomplete portion as specifically as practical.

### 8.5 Truncated

A result is **truncated** when additional matching material may exist but a declared result limit ended delivery.

Truncation differs from infrastructure failure.

### 8.6 Authorization-limited completeness

A result may be complete for everything the requester is authorized to see while incomplete for the underlying global memory space.

The result should disclose this limitation where policy permits.

### 8.7 Index-limited completeness

A query using an incomplete index cannot claim complete search unless:

- the contract defines that index as the complete authoritative scope; or
- another mechanism accounts for unindexed state.

### 8.8 Catalog-limited completeness

One catalog cannot establish global completeness unless it is authoritative and adequately current for the declared scope.

### 8.9 Time-bounded completeness

A query may be complete for a declared observation boundary even when newer versions commit afterward.

### 8.10 Participant-bounded completeness

A query may be complete for all participating custodians while remaining incomplete for unavailable or unknown custodians.

The distinction must be reported.

### 8.11 Completeness evidence

A claim of completeness should be supported by evidence describing:

- expected participants;
- participating participants;
- search boundaries;
- catalog generations;
- snapshot or version boundaries;
- authorization exclusions;
- truncation limits;
- unresolved failures.

## 9. Not-found semantics

### 9.1 Required meaning

**Not found** means:

> The search required by the operation contract completed adequately within its declared scope, consistency expectation, and authority conditions, and no matching memory was found.

### 9.2 Not found requires adequate search

Not found must not be reported when:

- a required custodian was unavailable;
- a required catalog was unavailable;
- catalog coverage was unknown;
- required authorization could not be evaluated;
- the query timed out;
- result limits truncated the search;
- the index was known to be incomplete;
- a conflict prevented identity resolution;
- required schema interpretation failed;
- the operation was deferred;
- the operation result was indeterminate.

### 9.3 Not found is scoped

A valid not-found result may mean:

- no matching memory in this namespace;
- no matching committed version;
- no matching authorized visible memory;
- no matching representation under the requested schema;
- no matching memory as of the declared snapshot.

It does not mean no such memory could exist anywhere under any scope.

### 9.4 Direct identity lookup

A direct lookup by logical memory identity may report not found only when the authoritative identity scope establishes absence or valid deletion for that identifier.

Failure to locate a current physical copy may instead mean unavailable or lost.

### 9.5 Alias lookup

Failure to resolve an alias does not prove the target memory does not exist.

It proves only that the alias did not resolve within the declared alias scope.

### 9.6 Security concealment

External policy may intentionally provide an opaque response to unauthorized users.

Internal state must preserve the true distinction between:

- not found;
- unauthorized;
- existence concealed.

### 9.7 Negative-result evidence

Not-found evidence should remain available long enough to support:

- retry decisions;
- audit;
- duplicate handling;
- debugging;
- consistency review.

## 10. Freshness and staleness model

### 10.1 Freshness is contextual

Freshness describes how closely a returned state satisfies the version or observation boundary requested by an operation.

It is not an absolute property.

### 10.2 Current within scope

A result is **current** only within a declared:

- logical memory;
- authority scope;
- consistency model;
- version boundary;
- observation time;
- conflict state.

### 10.3 Bounded stale

A result is **bounded stale** when the system can establish a maximum accepted distance from a relevant current boundary.

A bound may be expressed through:

- version generations;
- commit sequence;
- causal position;
- elapsed observation time;
- another governed metric.

### 10.4 Stale

A result is **stale** when evidence establishes that a newer relevant state exists or that the result fails the requested freshness expectation.

### 10.5 Freshness unknown

Freshness is **unknown** when the system cannot establish whether a newer relevant state exists.

### 10.6 Stale-but-useful

A stale result may be returned when:

- the requester permits stale results;
- the result is explicitly marked stale;
- its observed version is identified;
- known newer state or uncertainty is disclosed;
- use does not violate safety or lifecycle policy.

### 10.7 Stale results cannot satisfy current-only requests

A request requiring current committed state must not be reported completed successfully with a stale result.

### 10.8 Cache freshness

A cache must identify or make recoverable:

- cached version;
- observation boundary;
- validation state;
- invalidation state;
- freshness evidence.

A cache hit does not independently prove current state.

### 10.9 Time is not sufficient freshness evidence

A recent timestamp does not prove a version is current.

Freshness may require lineage or commit evidence.

## 11. Consistency expectation model

Node does not require every memory or operation to use the same consistency expectation.

The applicable expectation must be declared by:

- memory policy;
- operation request;
- operation profile;
- another governed rule.

Implementations may use different names.

They must preserve the semantics of supported expectations.

### 11.1 Any validated version

The operation may return any version satisfying declared validation and authorization requirements.

This expectation prioritizes availability and is suitable only where version currency is not required.

### 11.2 Latest locally known version

The operation returns the newest eligible version known to the serving role instance.

The result must not imply global currentness.

### 11.3 Latest observed committed version

The operation returns the newest committed version established within the observation scope.

Unavailable authorities or custodians must be disclosed.

### 11.4 Authoritative committed version

The operation requires the version selected as authoritative under the applicable commit and conflict-resolution model.

If authority cannot be established, the result must be unavailable, conflicting, partial, or indeterminate rather than silently weakened.

### 11.5 Snapshot-consistent

All results in one operation reflect one declared consistent observation boundary.

The boundary may be represented through:

- commit generation;
- snapshot identity;
- lineage frontier;
- another governed mechanism.

### 11.6 Causally consistent

Returned state must not omit dependencies or prior states required by the applicable causal relation.

This specification does not mandate how causal relations are represented.

### 11.7 Monotonic read

A requester should not observe an older version after previously observing a newer version within the declared session or continuity scope.

### 11.8 Read-your-writes

A requester should observe its own committed mutation when later reading within the declared consistency scope.

Acceptance without commitment does not satisfy this expectation.

### 11.9 Bounded staleness

The returned result may be older than the latest authoritative state but must remain within a declared bound.

### 11.10 Conflict-aware

The operation must return or report all known authoritative or valid-looking conflicting branches relevant to the requested scope.

It must not silently select one.

### 11.11 Historical exact-version

The operation requests one identified historical version.

Currentness is irrelevant unless the version has been deleted, invalidated, or become unavailable.

### 11.12 Complete-through boundary

The operation requires every result committed through a declared boundary to be included.

Later state may be excluded.

### 11.13 Custom profile

An implementation may define additional consistency profiles.

Each profile must state:

- what guarantee it provides;
- its scope;
- required evidence;
- behavior under partition;
- behavior under conflict;
- permissible degradation;
- failure outcome when the guarantee cannot be met.

## 12. Achieved consistency

### 12.1 Requested and achieved consistency remain separate

A result must distinguish:

- requested consistency;
- achieved consistency;
- unmet consistency requirements.

### 12.2 Silent weakening is prohibited

An implementation must not silently replace:

- authoritative committed;
- snapshot;
- causal;
- bounded-stale;
- conflict-aware

with a weaker consistency result.

### 12.3 Permitted fallback

A request may explicitly permit fallback to weaker consistency.

The result must identify the fallback used.

### 12.4 Consistency evidence

A consistency claim should identify:

- participating authorities;
- observed versions;
- lineage boundary;
- unavailable participants;
- conflict observations;
- snapshot or generation boundary;
- evidence age.

### 12.5 Local agreement is not global consistency

Agreement among locally reachable replicas does not prove global agreement when required participants are absent.

### 12.6 Replica count is not authority

The version held by the largest number of copies is not automatically authoritative.

### 12.7 Recent response is not authority

The first, fastest, or newest-timestamped response is not automatically authoritative.

## 13. Conflict model

### 13.1 Conflict state

Memory is **conflicting** when incompatible valid-looking claims cannot currently be reconciled under the applicable authority and consistency model.

### 13.2 Conflict types

Conflict may involve:

- logical identity;
- version lineage;
- current-version selection;
- provenance;
- schema;
- lifecycle state;
- deletion state;
- retention state;
- operation outcome;
- custodian claims;
- authority claims.

### 13.3 Conflict evidence remains visible

A retrieval result must not hide conflict by returning only the easiest or fastest branch unless the request explicitly permits a named selection policy.

### 13.4 Conflict-free observation

A result may be conflict-free within its observed scope while conflict remains possible outside that scope.

The result must not overstate global conflict absence.

### 13.5 Conflict and partial state

A result may be both:

- partial;
- conflicting.

For example, two reachable custodians may disagree while a third required custodian is unavailable.

### 13.6 Conflict selection

Selecting one branch for use does not necessarily resolve the underlying conflict.

The result must distinguish:

- branch selected for this operation;
- conflict resolved authoritatively;
- conflict still present.

### 13.7 Conservative behavior

When unresolved conflict affects safety, identity, lifecycle, or authoritative mutation, the system should fail conservatively rather than invent one state.

### 13.8 Conflict resolution belongs to governed authority

Conflict resolution must follow the authority and lineage rules of MEM-0002 through MEM-0004.

## 14. Partition semantics

### 14.1 Partition is expected

Network or service partition is a normal distributed-system condition.

### 14.2 Partition does not prove failure

An unreachable participant may be:

- healthy but isolated;
- failed;
- overloaded;
- intentionally suspended;
- replaced;
- unknown.

### 14.3 Partition does not prove absence

Memory unreachable during partition remains unavailable or unknown unless authoritative evidence establishes otherwise.

### 14.4 Partitioned reads

During partition, a read may return:

- local validated state;
- stale state;
- partial state;
- conflicting state;
- unavailable;
- unknown.

The result must identify the limitations.

### 14.5 Partitioned mutation

A mutating operation may proceed during partition only when its applicable authority and consistency contract permits it.

Otherwise it must:

- defer;
- reject;
- remain pending;
- report unavailable;
- become indeterminate when outcome cannot be established.

### 14.6 Partitioned not found

Not found must not be reported if unreachable participants are required to establish absence.

### 14.7 Rejoining

When a partition heals, returning state must be reconciled before it is treated as current or authoritative.

### 14.8 Split authority

If partition permits competing authoritative-looking decisions, the resulting conflict must remain explicit.

## 15. Degraded operation

### 15.1 Degradation is explicit

Node may continue memory operation under reduced guarantees when policy permits.

### 15.2 Degraded-read contract

A degraded read must identify:

- guarantee requested;
- guarantee unavailable;
- fallback used;
- stale or partial scope;
- conflict state;
- known missing dependencies;
- safety limitations.

### 15.3 Degraded writes

A mutating operation must not silently weaken commitment or consistency guarantees merely to remain available.

### 15.4 Read-only degraded mode

When commitment authority is unavailable, Node may continue authorized retrieval of existing committed memory if validity and lifecycle state can still be established.

### 15.5 Local-only mode

A local-only result must be marked as local in scope.

It must not imply global completeness or currentness.

### 15.6 Metadata-only mode

When content is unavailable but identity or lifecycle metadata remains available, the result may report:

- known existence;
- known version;
- known unavailability;
- known deletion;
- known custody.

It must not invent content.

### 15.7 Raw-retention mode

A custodian may retain uninterpretable data when schema or key dependencies are unavailable.

The memory is physically retained but not semantically available.

## 16. Repair and reconstruction state

### 16.1 Repairing

Memory may be marked **repairing** when protection or availability restoration is actively underway.

### 16.2 Repairing does not imply unavailable

A safe validated representation may remain readable during repair.

### 16.3 Repairing does not imply protected

Until repair satisfies the declared durability target, the memory remains under-protected.

### 16.4 Reconstructed result

A reconstructed result must identify:

- reconstruction occurred;
- source evidence;
- validation performed;
- representation equivalence;
- remaining uncertainty.

### 16.5 Partial reconstruction

Incomplete reconstruction must not be reported as a complete original.

### 16.6 Repair observation

A repair coordinator's progress report is not proof that the repaired representation is available or validated.

## 17. Catalog and index semantics

### 17.1 Catalog availability

Catalog availability does not prove memory availability.

### 17.2 Memory availability

Memory availability does not require every catalog to be available when direct identity or locator evidence is sufficient.

### 17.3 Catalog staleness

Catalog entries must expose or allow determination of:

- observation age;
- known version;
- known locator state;
- coverage;
- authority.

### 17.4 Index results

An index result identifies candidate memories.

It does not independently prove:

- existence;
- current lifecycle state;
- retrieval availability;
- content validation;
- complete search.

### 17.5 Index miss

An index miss is not not found unless the index is complete and authoritative for the declared scope.

### 17.6 Catalog disagreement

Conflicting catalog claims must remain explicit until reconciled.

## 18. Cache semantics

### 18.1 Cache is supporting state

A cache does not become authoritative merely because it improves retrieval latency.

### 18.2 Cache hit

A cache hit must preserve:

- logical identity;
- version identity;
- representation identity;
- validation state;
- freshness evidence.

### 18.3 Cache miss

A cache miss does not prove absence.

### 18.4 Cache invalidation uncertainty

When invalidation state is unknown, freshness must be reported unknown or stale according to evidence.

### 18.5 Disconnected cache

A disconnected cache may serve stale data only when policy and the request permit it.

### 18.6 Cache conflict

A cache containing a version conflicting with authoritative state must not silently overwrite the authoritative result.

## 19. Availability aggregation

### 19.1 Memory-level availability

Availability of one physical copy contributes to memory availability only when that copy satisfies the operation's:

- version;
- schema;
- validation;
- authorization;
- representation requirements.

### 19.2 Role aggregation

A memory operation may require several roles.

The operation is available only when required role coverage is sufficient.

### 19.3 Dependency aggregation

One unavailable mandatory dependency may make the operation unavailable even when content bytes are reachable.

### 19.4 Partial aggregation

When some but not all independent results can be obtained, the operation may return partial according to contract.

### 19.5 No weakest-status collapse

An implementation should not collapse every composite state to one worst status when doing so removes useful distinctions.

For example, a result can report:

- three memories found and validated;
- one requested namespace unavailable;
- one result stale;
- overall operation partial.

## 20. Authorization and visibility

### 20.1 Visibility scope

Completeness and not-found claims are always bounded by authorized visibility.

### 20.2 Internal truth

Security concealment must not corrupt internal memory-state semantics.

### 20.3 Redacted scope

A result may disclose that scope was authorization-limited without revealing hidden memory identities.

### 20.4 Authorization unavailable

Failure to evaluate authorization must not become implicit permission or not found.

### 20.5 Changing authorization

A prior successful retrieval does not guarantee current authorization.

A prior denial does not establish deletion or absence.

## 21. Resource pressure

### 21.1 Resource-limited retrieval

A retrieval may become:

- deferred;
- partial;
- truncated;
- unavailable

because of resource limits.

The result must identify the condition.

### 21.2 Resource limits do not justify false completeness

Stopping because of:

- time;
- memory;
- bandwidth;
- result count;
- compute;
- energy

must not produce a complete-result claim unless the bounded contract was fully satisfied.

### 21.3 Repair pressure

Resource pressure may delay repair.

The resulting under-protection must remain explicit.

### 21.4 Consistency pressure

A system must not silently weaken consistency because the stronger operation is expensive.

Fallback requires explicit permission.

### 21.5 Communication backpressure

ACS backpressure may delay evidence acquisition.

It does not independently define memory unavailability or partiality.

## 22. Role boundaries

### 22.1 Catalog and locator

Report known identity and location coverage without overstating completeness.

### 22.2 Custodian

Reports physical representation availability, integrity state, and local version evidence.

A custodian does not independently establish global currentness.

### 22.3 Retrieval provider

Obtains available representations and reports source limitations.

### 22.4 Result coordinator

Determines the semantic result state from:

- participating evidence;
- missing evidence;
- requested scope;
- achieved consistency;
- conflict state.

### 22.5 Validator

Establishes validation evidence but does not independently establish completeness or currentness.

### 22.6 Commit authority

Provides authoritative commitment evidence required by applicable consistency expectations.

### 22.7 Repair coordinator

Reports under-protection and repair state without claiming completion prematurely.

### 22.8 Access authority

Defines visible scope and authorization limitations.

No role may claim broader availability, completeness, or consistency than its assignment and evidence support.

## 23. Boundary with ACS

ACS governs:

- communication availability;
- connections;
- relationships;
- routes;
- endpoint reachability;
- signal delivery;
- payload references;
- communication admission;
- backpressure;
- secure sessions.

MEM governs:

- memory availability;
- search completeness;
- result completeness;
- freshness;
- consistency expectations;
- conflict;
- not-found semantics;
- degraded retrieval;
- partitioned memory behavior.

The following rules apply:

1. Endpoint reachability does not prove memory availability.
2. Connection failure does not prove memory unavailability.
3. Communication success does not prove retrieval completeness.
4. Transport timeout does not prove not found.
5. ACS retry does not change consistency expectations.
6. Communication backpressure does not define search completeness.
7. A payload reference does not prove the referenced representation is current or valid.
8. MEM does not redefine ACS routing or connection health.
9. ACS does not define memory conflict or current-version selection.
10. Several ACS exchanges may contribute to one MEM consistency observation.

## 24. Security and privacy

### 24.1 Availability information may be sensitive

An availability result may reveal:

- memory existence;
- custodian placement;
- system topology;
- repair activity;
- key availability;
- operational weakness.

Disclosure must remain authorized.

### 24.2 Completeness evidence may be sensitive

Search-scope evidence may reveal:

- hidden namespaces;
- restricted custodians;
- administrative domains;
- unseen memory categories.

Results may redact details while preserving honest limitation.

### 24.3 Conflict information may be sensitive

Conflict may reveal:

- concurrent activity;
- compromise;
- administrative disagreement;
- hidden versions.

Conflict must remain internally explicit even when externally concealed.

### 24.4 Consistency must not bypass authorization

A stronger consistency request does not authorize access to additional memory.

### 24.5 Concealment and not found

Security concealment may intentionally make external responses indistinguishable.

Internal conformance evidence must retain the actual outcome.

## 25. Conformance expectations

A conforming implementation must demonstrate:

- existence and availability separation;
- independent availability and completeness fields or equivalent behavior;
- strict not-found semantics;
- incomplete-index handling;
- unreachable-custodian handling;
- stale-result reporting;
- freshness-unknown reporting;
- conflict visibility;
- requested-versus-achieved consistency;
- explicit consistency fallback;
- partitioned-read behavior;
- partitioned-mutation behavior;
- authorization-limited completeness;
- cache miss behavior;
- catalog disagreement behavior;
- resource-truncated search;
- under-repair reporting;
- under-protection separation;
- unknown-state preservation.

Detailed fault tests belong to MEM-0010.

## 26. Prohibited interpretations

This specification shall not be interpreted to mean that:

- unavailable means deleted;
- unavailable means not found;
- unknown means unavailable;
- partial means invalid;
- stale means corrupted;
- stale means useless;
- current means globally latest without declared scope;
- consistent means factually true;
- a catalog entry proves availability;
- a missing catalog entry proves absence;
- an index miss proves absence;
- a cache hit proves freshness;
- a cache miss proves absence;
- one reachable replica proves completeness;
- replica majority automatically defines authority;
- latest timestamp automatically defines current state;
- fastest response automatically defines authority;
- all memories require strong consistency;
- all reads may weaken consistency automatically;
- a partition requires the entire memory system to stop;
- partitioned mutation is always safe;
- local completeness means global completeness;
- authorization-limited results are globally complete;
- security concealment may overwrite internal truth;
- repair progress proves repaired durability;
- public consistency rules define cognitive belief.

## 27. Open questions

The following questions remain for later specifications and implementation profiles:

- Which consistency profiles are mandatory for baseline conformance?
- Which memory classes require authoritative committed reads?
- Which memory classes permit any-valid-version reads?
- How should rapidly changing working memory declare freshness?
- Which staleness metrics are meaningful across heterogeneous memory types?
- How should snapshot boundaries span several logical memories?
- Which causal relations belong in public memory metadata?
- How should monotonic reads persist across process migration?
- Which requesters require read-your-writes guarantees?
- How should conflicting lifecycle state affect retrieval?
- Which conflicts permit branch-selecting degraded reads?
- Which operations must stop under partition?
- Which searches require catalogs to prove complete membership?
- How should authorization-limited not-found responses be audited?
- How should stale-but-useful results influence operation finality?
- Which availability observations require durable evidence?
- How should consistency evidence be compacted?
- How should result coordinators combine partially trusted participants?
- Which memory classes may be served during key-authority degradation?
- How should locality preferences interact with stronger consistency?
- Which low-level consistency and result-state types should become shared C++ interfaces?

These questions do not weaken the semantic distinctions already established.

## 28. Closing principle

> **Node must never interpret the limits of what it can currently reach as the limits of what it remembers.**

Reachability is not existence.

Availability is not completeness.

Completeness is not freshness.

Freshness is not consistency.

Agreement is not truth.

Silence is not absence.

Every result must carry the boundaries of what Node actually knows.

## Revision history

### Version 0.1 — 2026-07-16

- Replaced the planned MEM-0005 stub with the first normative working draft.
- Defined availability independently of existence, completeness, freshness, consistency, and durability.
- Established a multi-axis result-state model.
- Defined available, degraded, unavailable, unknown, authorization-limited, and dependency-limited availability.
- Distinguished result completeness from search completeness.
- Established strict scoped not-found requirements.
- Defined current, stale, bounded-stale, and freshness-unknown states.
- Defined public consistency expectations without selecting one consistency algorithm.
- Established requested-versus-achieved consistency reporting.
- Defined conflict, partition, degraded-operation, repair, catalog, index, and cache semantics.
- Preserved ACS, authorization, security, resource, and role boundaries.
- Established implementation-facing conformance expectations.
