# ACS-0002: Relationship Classes

| Field | Value |
|---|---|
| Specification | ACS-0002 |
| Title | Relationship Classes |
| Status | Draft |
| Version | 0.1 |
| Classification | Architecture Specification |
| Authors | Project Prometheus |
| Last updated | 2026-07-14 |
| Approval | Pending full review |
| Depends on | ACS-0000, ACS-0001 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | Medium; classification model requires further review |

> **A relationship class defines why an association exists within the organism, not where it runs or how its traffic is transported.**

## Architectural-intent notice

This specification defines the primary semantic classes of persistent relationships within the Adaptive Connection Substrate.

It does not prescribe data structures, transport protocols, scheduling mechanisms, cryptographic implementations, deployment locations, or lifecycle state machines unless explicitly stated.

Later ACS specifications may refine the behavior, admission, budgeting, security, and lifecycle of each relationship class. They must not silently redefine the purpose of a class established here.

## 1. Purpose

ACS-0002 defines the principal kinds of persistent logical relationships that may organize communication within Project Prometheus.

A relationship class identifies the primary role an association serves within the organism.

Relationship classes exist so that Prometheus can distinguish cognitive influence, organizational mediation, operational continuity, and protection from harm without deriving those meanings from sockets, processes, hardware placement, traffic volume, or implementation details.

This specification establishes four initial relationship classes:

1. synaptic;
2. bridge;
3. infrastructure;
4. immune.

The class set is intentionally small. New classes should be introduced only when an important semantic purpose cannot be represented safely by an existing class combined with explicit traits and bounded capabilities.

## 2. Scope

This specification governs:

- the distinction between relationship class and relationship traits;
- the primary-class rule;
- the semantic purpose of each relationship class;
- universal requirements applying to all classes;
- permitted interaction between classes;
- explicit class transitions;
- relationship authority boundaries;
- transport and hardware independence;
- prohibited interpretations of relationship classification.

This specification does not yet define:

- complete relationship lifecycle states;
- admission algorithms;
- resource-budget calculations;
- signal formats;
- trust-scoring systems;
- secure-session protocols;
- topology-learning algorithms;
- immune response policy;
- physical-node quarantine procedures;
- implementation interfaces.

Those subjects belong to later ACS specifications.

## 3. Terminology

### 3.1 Relationship

A relationship is a persistent logical association between identified participants within Prometheus.

A relationship records why interaction exists, what kind of interaction is permitted, and which architectural constraints govern it.

A relationship is not a socket, route, transport session, queue, device handle, or temporary exchange.

### 3.2 Connection

A connection is a bounded runtime mechanism through which an active relationship may exchange permitted signals, payload references, or operational information.

A relationship may temporarily have no active connection.

A relationship may survive connection loss, transport replacement, migration, or secure-session renewal when continuity can be safely established.

### 3.3 Relationship class

A relationship class identifies the primary semantic purpose of a relationship within the organism.

Class answers:

> Why does this relationship exist?

Class does not inherently answer:

- where the participants run;
- which transport they use;
- whether they are local or remote;
- whether communication is fast or slow;
- how much traffic they may carry;
- whether the relationship is currently healthy;
- which security mechanism protects it;
- whether it is temporary or long-lived.

### 3.4 Relationship trait

A relationship trait describes a bounded characteristic, permission, state, or behavior of a relationship without changing its primary semantic purpose.

Examples may include:

- local or remote;
- unidirectional or bidirectional;
- transient or persistent;
- adaptive or fixed;
- direct or mediated;
- recurrent or event-driven;
- provisional, trusted, restricted, or revoked;
- active, suspended, degraded, or recovering;
- low-volume or high-volume;
- permitted signal families;
- permitted payload categories.

Traits must not be used to conceal the creation of an undefined relationship class.

### 3.5 Capability

A capability is an explicitly granted and bounded permission to request or perform a defined operation.

Relationship class does not automatically grant every capability associated with that class.

Capabilities remain independently authorized, revocable, auditable, and limited.

### 3.6 Participant

A participant is a logical entity permitted to take part in a relationship.

Participants may include cognitive entities, regions, runtime services, infrastructure services, immune components, or other identified logical systems.

Physical machines and devices may host participants, but physical location does not define relationship identity.

## 4. Classification model

### 4.1 Semantic classification

Relationships shall be classified by their primary semantic purpose.

Classification shall not be derived solely from:

- transport type;
- network address;
- physical distance;
- process boundary;
- hardware architecture;
- latency;
- bandwidth;
- encryption method;
- deployment host;
- current traffic volume.

A relationship retains its class when its transport or physical placement changes, provided its semantic purpose remains unchanged.

### 4.2 One primary class

Every relationship shall have exactly one primary class at a given time.

A relationship may possess multiple traits and capabilities, but it shall not hold an arbitrary combination of primary classes simultaneously.

This rule exists to prevent ambiguous relationships whose purpose, authority, failure behavior, and resource protections cannot be determined.

Where two distinct purposes are both necessary, separate relationships should normally be created.

### 4.3 Class does not equal authority

Relationship class describes purpose.

It does not independently grant unrestricted authority.

An infrastructure relationship does not automatically control hardware.

An immune relationship does not automatically quarantine or destroy a participant.

A bridge relationship does not control the entities it connects.

A synaptic relationship does not bypass admission, security, budgets, or signal validation because it is cognitively important.

### 4.4 Class does not equal layer

Relationship classes may commonly operate at different organizational layers, but class and layer are not identical.

Infrastructure and immune relationships may influence physical, runtime, and cognitive conditions.

Synaptic and bridge relationships may depend on infrastructure services.

Immune relationships may observe evidence originating from several layers.

A relationship's class is determined by purpose, not by its vertical position in a conceptual stack.

## 5. Universal relationship requirements

Every relationship class shall define:

1. its primary purpose;
2. its permitted participant categories;
3. the information it may carry;
4. the information it must not carry;
5. the authority it may request or exercise;
6. the trust conditions required for activation;
7. whether adaptation is permitted;
8. the resource protections it requires;
9. its expected behavior during degradation;
10. its interaction with lifecycle, security, and immune systems.

All relationships remain subject to ACS-0001.

In particular:

- relationships remain logical rather than transport-defined;
- connections remain bounded;
- discovery does not create relationship;
- unknown and stale states remain explicit;
- adaptation cannot override hard ceilings;
- invalid traffic is rejected before cognitive processing;
- remote sessions require mutual protection;
- relationship class cannot independently override physical-node authority boundaries.

## 6. Synaptic relationships

### 6.1 Definition

A synaptic relationship is a direct cognitive association through which logical entities may influence one another as part of perception, memory, reasoning, learning, coordination, behavior, or other cognitive operation.

### 6.2 Purpose

Synaptic relationships exist to support structured cognitive influence between logical participants.

They represent meaningful cognitive association rather than incidental data exchange.

### 6.3 What synaptic relationships enable

Synaptic relationships may enable:

- cognitive signal exchange;
- recurrent influence;
- activation propagation;
- association formation;
- learned strengthening or weakening;
- selective information transfer;
- coordination between cognitive roles;
- persistence of cognitive association across migration;
- local or remote cognitive interaction.

### 6.4 What synaptic relationships forbid

A synaptic relationship shall not inherently:

- grant infrastructure-control authority;
- grant immune authority;
- bypass security or admission;
- create unbounded cognitive traffic;
- imply permanent importance;
- require one transport or physical host;
- treat discovery as cognitive attachment;
- treat traffic volume as relationship strength.

### 6.5 Adaptation

A synaptic relationship may be adaptive or fixed.

Adaptive behavior may alter permitted traits such as weight, priority, confidence, signal preference, or activation tendency, subject to hard resource and security limits.

A fixed cognitive pathway may still be synaptic when its purpose is direct cognitive influence.

Adaptation is therefore permitted but is not required for classification.

### 6.6 Failure behavior

Failure of a synaptic relationship should normally reduce or alter cognitive capability without destroying organism identity.

Synaptic failure may be reported to health or immune systems when the failure is persistent, suspicious, or inconsistent with expected behavior.

## 7. Bridge relationships

### 7.1 Definition

A bridge relationship is a selective and mediated association that connects participants, groups, or regions whose interaction requires an explicit organizational boundary.

### 7.2 Purpose

Bridge relationships exist to preserve separation while permitting controlled interaction.

They allow information to cross boundaries without collapsing distinct structures into one undifferentiated network.

### 7.3 What bridge relationships enable

Bridge relationships may enable:

- communication between cognitive regions;
- selective cross-region influence;
- mediation between differently specialized systems;
- summarization or transformation at a boundary;
- constrained propagation;
- trust-boundary crossing;
- rate-limited or filtered information exchange;
- communication across expensive or unreliable paths;
- coordination between independently organized groups.

### 7.4 What bridge relationships forbid

A bridge relationship shall not inherently:

- merge the participants it connects;
- grant authority over either side;
- expose unrestricted internal traffic;
- bypass per-side security policy;
- become an unlimited all-to-all pathway;
- be defined merely by physical distance;
- be treated as a slow synaptic relationship without mediation semantics.

### 7.5 Region independence

A bridge commonly connects regions, but region membership alone shall not automatically create bridge classification.

A bridge may connect any participants for which selective mediation is the primary purpose.

Likewise, a cross-region synaptic relationship may exist when direct cognitive influence is explicitly admitted and does not violate regional boundaries.

### 7.6 Failure behavior

Bridge failure may partition capabilities or reduce coordination between otherwise functioning structures.

Participants on either side should remain as coherent as their local dependencies permit.

A bridge must not become a single unbounded failure point without explicit architectural justification.

## 8. Infrastructure relationships

### 8.1 Definition

An infrastructure relationship is an operational association that coordinates the conditions required for the organism and its other relationships to remain available, coherent, observable, secure, and recoverable.

### 8.2 Purpose

Infrastructure relationships exist to sustain operation.

They support the substrate through which cognitive, bridge, and immune relationships can function without themselves becoming ordinary cognitive pathways.

### 8.3 What infrastructure relationships enable

Infrastructure relationships may enable:

- health reporting;
- resource and capacity reporting;
- time and synchronization services;
- lifecycle coordination;
- secure-session establishment;
- admission negotiation;
- topology maintenance;
- transport maintenance;
- recovery coordination;
- service discovery;
- congestion reporting;
- shutdown and restart coordination;
- operational-state propagation.

### 8.4 What infrastructure relationships forbid

An infrastructure relationship shall not inherently:

- carry unrestricted private cognitive payloads;
- grant total authority over a physical node;
- bypass capability authorization;
- silently modify cognitive state;
- become an unbounded monitoring channel;
- be starved by ordinary cognitive traffic;
- be assumed trustworthy solely because it is classified as infrastructure.

### 8.5 Cross-layer effects

Infrastructure relationships may operate beneath ordinary cognitive activity while producing effects observable throughout the organism.

A power limit, thermal condition, failed transport, unavailable accelerator, clock discontinuity, or reduced memory budget may originate in infrastructure and alter higher-level behavior.

Such influence does not make infrastructure cognitive. It reflects the dependence of higher organizational layers on operational conditions.

### 8.6 Failure behavior

Infrastructure failure may affect broad portions of the organism and must therefore degrade conservatively.

Unknown infrastructure state must remain explicit.

Where possible, loss of one infrastructure participant should reduce observability or coordination rather than silently creating false healthy state.

## 9. Immune relationships

### 9.1 Definition

An immune relationship is a protective association through which identified immune participants observe, exchange evidence, challenge behavior, assess suspected harm, coordinate containment, or verify recovery.

### 9.2 Purpose

Immune relationships exist to protect the integrity and continued health of the organism.

They are distinct from infrastructure relationships because sustaining operation and evaluating possible harm are related but different responsibilities.

### 9.3 What immune relationships enable

Immune relationships may enable:

- anomaly reporting;
- integrity evidence exchange;
- corruption suspicion;
- challenge-response evaluation;
- trust degradation recommendations;
- containment coordination;
- recovery verification;
- detection of inconsistent behavior;
- comparison of observations from independent locations;
- escalation to separately authorized lifecycle or security systems;
- maintenance of bounded immune memory.

### 9.4 What immune relationships forbid

An immune relationship shall not inherently:

- destroy a participant;
- erase cognitive state;
- quarantine a physical node without separate authority;
- assume that suspicion proves compromise;
- bypass evidence validation;
- conduct unrestricted surveillance;
- conceal its own authority or resource use;
- exempt itself from authentication, budgets, auditing, or lifecycle rules;
- propagate suspicion indefinitely without expiration or review;
- become immune to immune evaluation.

### 9.5 Independent immune shards

The architecture may permit multiple independently placed immune components or shards.

An immune shard is a bounded logical immune participant, not an independent authority. Its independence provides diversity of observation and fault containment; it does not grant unilateral control over participants, infrastructure, or organism state.

An immune shard may observe a limited portion of the organism, maintain bounded local evidence, perform defined integrity checks or challenge-response tests, corroborate observations made elsewhere, or verify recovery.

Immune shards may be distributed across physical machines, runtimes, regions, organizational layers, or trust boundaries.

Independent placement may improve:

- fault detection;
- evidence diversity;
- resistance to localized compromise;
- verification across layers;
- survival when one immune component fails;
- detection of inconsistencies hidden from a central observer.

Immune shards must not automatically share unrestricted authority or complete organism state.

Each shard must operate within explicit scope, trust, capability, retention, lifecycle, and resource limits.

No single shard's suspicion shall automatically become organism-wide fact.

Independent observations may be combined, but correlation, disagreement handling, consensus, escalation, and containment policy belong to later immune and security specifications.

### 9.6 Persistent and temporary immune shards

Immune shards may be persistent or temporary.

A persistent shard may provide continuing protective coverage for a defined subsystem, layer, region, trust boundary, or class of risk.

A temporary shard may be created for a bounded assignment, including:

- investigating conflicting evidence;
- examining unusual participant behavior;
- evaluating a newly admitted machine or service;
- verifying a repair, restart, migration, or recovery action;
- restoring coverage after another shard becomes unavailable;
- independently checking the behavior of another immune component.

Temporary shards shall have explicit scope, budget, authority, retention, and retirement conditions.

A temporary shard shall not become permanent merely because it was once useful. Continued existence requires deliberate admission as a persistent participant or a renewed bounded assignment.

Persistent shards are not permanently trusted by definition. They remain observable, revocable, replaceable, and subject to immune evaluation.

### 9.7 Immune specialization

Immune shards need not be identical.

A shard may hold a bounded specialization such as:

- transport-integrity observation;
- memory or storage consistency checking;
- runtime-behavior monitoring;
- security-event correlation;
- cognitive-pattern anomaly observation;
- recovery verification;
- immune-system self-observation.

These are roles or capabilities, not additional relationship classes.

Specialization shall not expand a shard's authority beyond its explicitly granted scope.

### 9.8 Codependence without subordination

The immune system may operate with partial independence while remaining codependent with the rest of Prometheus.

It depends on infrastructure for execution, communication, identity, and resource allocation.

The organism depends on immune functions for integrity assessment and coordinated protection.

Neither dependency makes the immune system an unrestricted supervisor of cognition or infrastructure.

### 9.9 Immune self-regulation

Immune components may themselves fail, become stale, overreact, disagree, or become compromised.

Immune relationships must therefore support:

- explicit uncertainty;
- evidence provenance;
- bounded suspicion;
- disagreement between observers;
- expiration and reassessment;
- auditability;
- revocation;
- independent verification;
- recovery from false-positive containment.

Immune shards may inspect and challenge other immune shards within separately defined authority boundaries.

An immune participant shall not be considered permanently trustworthy merely because it belongs to the immune system.

An immune system capable of protecting the organism must also be designed to avoid becoming a source of uncontrolled harm.

### 9.10 Failure behavior

Loss of one immune relationship or shard should reduce protective coverage rather than automatically halt the organism.

Loss of sufficient immune coverage may require explicit degraded-security state, restricted operation, or admission limits.

The exact thresholds and responses belong to later specifications.

## 10. Relationship traits

Traits supplement class without replacing it.

Potential trait families include:

### 10.1 Directionality

- unidirectional;
- bidirectional;
- asymmetric.

### 10.2 Persistence

- transient;
- session-persistent;
- long-lived;
- developmental.

### 10.3 Adaptation

- fixed;
- adaptive;
- externally tuned;
- temporarily frozen.

### 10.4 Mediation

- direct;
- mediated;
- aggregated;
- filtered.

### 10.5 Trust state

- provisional;
- authenticated;
- trusted;
- restricted;
- degraded;
- revoked.

### 10.6 Operational state

- proposed;
- admitted;
- active;
- suspended;
- degraded;
- recovering;
- retired.

### 10.7 Locality

- same process;
- same runtime;
- same physical node;
- same region;
- remote;
- location unknown.

Locality shall remain descriptive and must not define relationship identity or class.

### 10.8 Information permissions

Traits or capabilities may constrain:

- allowed signal families;
- payload categories;
- maximum sensitivity;
- allowed direction;
- frequency;
- rate;
- retention;
- transformation;
- forwarding.

Detailed trait definitions belong to later ACS specifications.

## 11. Interaction between relationship classes

Different classes may cooperate without being merged.

An example interaction may be:

1. a synaptic relationship exhibits unexpected behavior;
2. an immune relationship receives or collects evidence;
3. independent immune shards compare observations;
4. an immune component reports a bounded assessment;
5. an infrastructure relationship carries an authorized lifecycle request;
6. a separately authorized security or lifecycle system decides whether action is permitted;
7. recovery state is later verified through immune and infrastructure mechanisms.

No step automatically grants the next step's authority.

A cognitive observation is not an immune verdict.

An immune suspicion is not a quarantine order.

An infrastructure request is not authorization.

A lifecycle action is not proof of recovery.

Each boundary must remain explicit.

## 12. Class transitions

A relationship may change primary class only through an explicit architectural transition.

A class transition shall require:

- a stated reason;
- authorization appropriate to the destination class;
- validation of participant eligibility;
- review of trust requirements;
- review of resource budgets;
- review of permitted signal and payload categories;
- preservation or deliberate termination of relationship identity;
- an auditable transition event.

Class transition must not occur merely because traffic patterns changed.

In many cases, ending one relationship and creating another may be safer than changing class.

ACS-0005 will define lifecycle mechanics for transitions.

## 13. Prohibited interpretations

The following shall not be treated as relationship classes:

- discovered peer;
- network neighbor;
- socket;
- transport binding;
- secure session;
- authenticated identity;
- endpoint;
- port;
- queue;
- region membership;
- physical proximity;
- hardware device;
- runtime process;
- temporary packet exchange;
- high-priority traffic;
- administrative permission.

These objects or properties may participate in relationship establishment, but they do not answer why the relationship exists.

## 14. Relationship to later specifications

ACS-0002 defines semantic classes.

Later specifications are expected to define:

- **ACS-0003:** signal families and signal semantics;
- **ACS-0004:** endpoints and ports;
- **ACS-0005:** relationship and connection lifecycles;
- **ACS-0006:** admission and resource budgets;
- **ACS-0007:** security, identity, authorization, and trust;
- **ACS-0008:** immune behavior, evidence, escalation, containment, shard lifecycle, and recovery verification;
- **ACS-0009:** runtime integration and execution boundaries.

Later specifications may add detail to a class but shall not silently change its primary purpose.

## 15. Design rationale

A distributed cognitive organism requires more than one kind of association.

Treating every association as an undifferentiated connection would obscure major differences between:

- cognitive influence;
- selective organizational mediation;
- operational continuity;
- protection from suspected harm.

These purposes require different information permissions, priorities, failure behavior, trust boundaries, and authority limits.

At the same time, creating a new class for every combination of locality, direction, trust, adaptation, or traffic pattern would produce an unmanageable type system.

ACS-0002 therefore separates a small set of primary semantic classes from orthogonal traits and capabilities.

The four initial classes are intended to be sufficient without being assumed complete.

## 16. Architectural consequences

This classification model requires:

- relationship purpose to be explicit;
- one primary class per relationship;
- class-independent transport binding;
- separately granted capabilities;
- separate treatment of class, trait, lifecycle state, and trust state;
- protected infrastructure capacity;
- bounded immune authority;
- explicit cross-class escalation;
- deliberate class transitions;
- support for persistent and temporary distributed immune observation;
- clear distinction between suspicion, recommendation, authorization, action, and verification.

These consequences are intentional.

## 17. Open questions

The following questions remain unresolved:

- Should relationships always have exactly two participants, or may some be group relationships?
- Can one logical relationship safely change class while retaining identity?
- When should class transition be replaced by relationship retirement and recreation?
- Can mixed cognitive and infrastructure information share one relationship under strict permissions?
- What minimum evidence should permit immune escalation?
- How should independent immune shards corroborate observations?
- How much state may an immune shard retain?
- How are immune shards prevented from forming a privileged uncontrolled network?
- Should persistent immune shards be placed randomly, deliberately, adaptively, or through multiple strategies?
- What conditions justify creation and retirement of temporary immune shards?
- How is adequate immune coverage measured?
- Can a bridge relationship mediate immune traffic without becoming immune-class itself?
- Can infrastructure relationships carry public cognitive metadata without becoming synaptic?
- How are relationship classes represented without locking implementations into one language or ABI?
- Under what conditions may new relationship classes be introduced?

These questions should remain open until later specifications provide enough context to answer them safely.

## 18. Future evolution

New relationship classes may be introduced when later work demonstrates that:

1. the proposed purpose is architecturally distinct;
2. the purpose cannot be represented safely through an existing class;
3. traits and capabilities are insufficient;
4. the new class requires meaningfully different authority, failure behavior, or lifecycle rules;
5. the addition does not create avoidable ambiguity.

Existing classes may be revised while this specification remains Draft.

Changes should preserve the distinction between semantic purpose, traits, capabilities, lifecycle state, trust state, and physical transport.

## 19. Closing principle

> **Different systems may protect, sustain, mediate, and shape the organism, but no relationship gains unlimited authority merely because its purpose is important.**

## Revision history

### Version 0.1 — 2026-07-14

- Established the initial four-class relationship model.
- Defined synaptic, bridge, infrastructure, and immune relationships.
- Separated primary class from traits and capabilities.
- Established one primary class per relationship.
- Defined transport and hardware independence.
- Introduced bounded persistent and temporary immune shards as architectural possibilities.
- Established that immune-shard independence provides observational diversity rather than unilateral authority.
- Established explicit cross-class authority boundaries.
- Recorded unresolved questions for later ACS specifications.
