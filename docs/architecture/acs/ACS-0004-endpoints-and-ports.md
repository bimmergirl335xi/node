# ACS-0004: Endpoints and Ports

| Field | Value |
|---|---|
| Specification | ACS-0004 |
| Title | Endpoints and Ports |
| Status | Draft |
| Version | 0.1 |
| Classification | PUBLIC-ARCHITECTURE |
| Work-item family | ACS-PUB |
| Authors | Node |
| Last updated | 2026-07-16 |
| Approval | Pending review |
| Depends on | ACS-0000, ACS-0001, ACS-0002, ACS-0003 |
| Related specifications | MEM-0000, MEM-0001, MEM-0002 |
| Supersedes | None |
| Superseded by | None |
| Current confidence | High in foundational distinctions; contract details and lifecycle interaction remain under review |

> **An endpoint identifies a logical communication boundary; a port defines the bounded meaning that may cross that boundary.**

## Architectural-intent notice

This specification defines the public architectural model for endpoints and ports within the Adaptive Connection Substrate.

It is independently authored public architecture.

It is not produced by deleting, sanitizing, paraphrasing, or selectively redacting a restricted design.

This specification defines the distinctions and guarantees required for independently implementable Node-compatible communication boundaries without disclosing:

- production endpoint catalogs;
- proprietary cognitive interfaces;
- private topology policy;
- private trust thresholds;
- restricted operator procedures;
- production routing algorithms;
- internal security credentials;
- non-public adaptive behavior.

This specification does not prescribe:

- network protocols;
- socket APIs;
- numeric network-port assignments;
- binary layouts;
- identifier widths;
- programming-language interfaces;
- transport libraries;
- queue implementations;
- cryptographic algorithms;
- production deployment topology;
- complete admission algorithms;
- complete resource-allocation algorithms;
- complete connection lifecycle state machines.

Later ACS specifications and implementation profiles may refine these subjects.

They must preserve the distinctions established here.

## 1. Purpose

ACS-0004 defines how logical participants expose bounded communication capabilities within Node.

Participants require stable ways to:

- receive signals;
- emit signals;
- expose services;
- restrict access;
- distinguish communication purposes;
- survive transport or placement changes;
- mediate communication across boundaries;
- report availability and failure honestly;
- enforce resource and authority limits.

A design based only on:

- socket addresses;
- process identifiers;
- hostnames;
- queue names;
- transport topics;
- device paths;
- numeric ports;

would cause communication identity and meaning to change whenever implementation details changed.

Endpoints and ports therefore exist as logical ACS objects independent of the runtime mechanisms that currently carry them.

An endpoint identifies a communication boundary belonging to a participant.

A port defines one typed and bounded capability exposed through that endpoint.

## 2. Scope

This specification governs:

- endpoint identity;
- endpoint ownership;
- endpoint continuity;
- endpoint availability;
- port identity;
- port contracts;
- port direction;
- port visibility;
- port discovery;
- port compatibility;
- port versioning;
- bindings;
- attachments;
- direct and mediated access;
- facade ports;
- admission boundaries;
- authority boundaries;
- resource-contract requirements;
- failure and unknown states;
- interaction with relationships and connections;
- interaction with MEM and adjacent architecture;
- public conformance expectations.

This specification does not define:

- the complete connection lifecycle;
- the complete attachment lifecycle;
- relationship-creation policy;
- trust-scoring systems;
- secure-session protocols;
- production routing;
- topology adaptation;
- exact queue sizes;
- exact rate limits;
- production endpoint names;
- production port catalogs;
- proprietary cognitive operations;
- memory-operation semantics;
- physical-node lifecycle;
- operating-system service discovery.

Those subjects belong to later specifications, adjacent architecture, or implementation profiles.

## 3. Normative language

The words **must**, **must not**, **shall**, and **shall not** describe mandatory architectural requirements.

The words **should** and **should not** describe strong recommendations that may be departed from only with documented justification.

The word **may** describes permitted behavior.

An implementation does not conform merely because it uses the words *endpoint* and *port*.

Its observable behavior must preserve the distinctions defined here.

## 4. Core terminology

### 4.1 Participant

A participant is an identified logical entity permitted to take part in ACS relationships and communication.

Participants may include:

- cognitive services;
- cognitive regions;
- memory services;
- runtime services;
- infrastructure services;
- security services;
- immune services;
- bridges;
- authorized external systems.

A participant is not permanently identified with one host, process, thread, device, or transport session.

### 4.2 Endpoint

An endpoint is an identified logical communication boundary owned by a participant.

An endpoint answers:

> Which logical communication boundary is being addressed?

An endpoint does not, by itself, define:

- which operations are available;
- which signals are permitted;
- whether the observer may use it;
- whether a relationship exists;
- whether the endpoint is currently reachable;
- where it is physically hosted.

An endpoint is not:

- a socket;
- an IP address;
- a hostname;
- a process;
- a thread;
- a queue;
- a file descriptor;
- a transport topic;
- a device;
- a physical node.

Those objects may implement or carry an endpoint.

They do not define its logical identity.

### 4.3 Port

A port is an identified, typed, bounded semantic interface belonging to an endpoint.

A port answers:

> What communication capability is exposed at this endpoint?

A port contract may define:

- permitted signal domains;
- permitted signal intents;
- permitted subtype families;
- direction;
- payload-reference policy;
- relationship-class eligibility;
- admission requirements;
- authority requirements;
- visibility;
- compatibility;
- resource limits;
- mediation requirements;
- failure behavior.

A port is not an operating-system network port.

### 4.4 Port contract

A port contract is the declared semantic and operational boundary governing use of a port.

The contract defines what conforming participants may expect and what they must not assume.

A port contract may reference schemas and policies defined by other specifications.

The contract shall not depend on undocumented transport behavior for its meaning.

### 4.5 Binding

A binding is a temporary association between a logical endpoint or port and concrete runtime resources.

A binding may include:

- a network address;
- a secure session;
- a queue;
- a local dispatch mechanism;
- a shared-memory region;
- a bridge path;
- a serialization profile;
- a process or host placement.

Bindings are replaceable.

Changing a binding does not inherently change endpoint or port identity.

### 4.6 Attachment

An attachment is a bounded association authorizing a relationship or connection to attempt interaction with a port under a declared scope.

Attachment does not automatically imply:

- acceptance of every signal;
- successful operation;
- directive authority;
- payload access;
- permanent availability;
- unlimited resource use.

The detailed attachment lifecycle belongs to ACS-0005.

### 4.7 Facade port

A facade port is a distinct port that presents a bounded interface backed by one or more other capabilities.

A facade may:

- reduce the exposed operation set;
- hide internal topology;
- aggregate compatible services;
- translate representations;
- mediate a trust boundary;
- provide a stable compatibility interface.

A facade is not identical to the ports or services behind it.

### 4.8 Visibility

Visibility describes who may learn that an endpoint or port exists and what portion of its contract may be disclosed.

Visibility does not grant use authority.

### 4.9 Direction

Direction describes the permitted flow of communication relative to the endpoint owning a port.

The initial direction forms are:

- ingress;
- egress;
- bidirectional;
- asymmetric bidirectional.

### 4.10 Availability

Endpoint or port availability describes whether the communication boundary can currently support interaction within a declared scope.

Availability does not prove:

- semantic service readiness;
- operation success;
- role coverage;
- memory availability;
- authority;
- trust.

## 5. Core model

The public endpoint model is:

```text
Participant
    owns one or more Endpoints

Endpoint
    owns one or more Ports

Port
    defines a bounded semantic capability

Relationship
    defines why participants are associated

Connection
    provides a bounded runtime communication mechanism

Attachment
    permits a relationship or connection to attempt scoped port use

Binding
    maps logical communication objects onto runtime resources
```

These concepts may be represented by fewer runtime objects for efficiency.

Their architectural meanings must remain distinguishable.

## 6. Endpoint identity

### 6.1 Logical identity

An endpoint shall possess logical identity independent of its current:

- host;
- process;
- address;
- route;
- transport;
- queue;
- secure session;
- runtime instance.

A participant may own several endpoints when it requires independently governed communication boundaries.

### 6.2 Identity scope

Endpoint identity may be:

- globally meaningful within a deployment;
- meaningful within one trust domain;
- meaningful within one region;
- meaningful only through an authorized mediator.

The applicable identity scope must be explicit enough to prevent accidental collision or false continuity.

### 6.3 Continuity

Endpoint identity may survive:

- process restart;
- service migration;
- host replacement;
- transport replacement;
- address change;
- secure-session renewal;
- temporary unavailability;
- recovery onto replacement substrate;

when continuity can be established according to policy.

### 6.4 No address-derived identity

An implementation shall not establish endpoint continuity solely because:

- the same network address reappears;
- the same process name is used;
- the same host responds;
- the same endpoint label is claimed;
- a previous route becomes reachable.

Addresses and labels may contribute evidence.

They are not sufficient proof by themselves.

### 6.5 Conflicting identity

When incompatible endpoint-identity claims exist, the state shall remain explicit.

The system must not choose one claimant solely because it:

- responded first;
- is physically closer;
- has lower latency;
- has greater computational capacity;
- occupies the previous network address.

Conflict resolution belongs to lifecycle and security policy.

## 7. Endpoint ownership

### 7.1 Logical ownership

Every endpoint shall have one authoritative logical owner within its declared ownership scope.

The owner is responsible for:

- the endpoint’s purpose;
- its port set;
- its ordinary lifecycle;
- its availability representation;
- its contract compatibility;
- its exposure policy.

Ownership is semantic.

It is not physical possession.

### 7.2 Hosting is not ownership

A host, process, container, virtual machine, or device does not acquire endpoint ownership merely because it currently carries or executes the endpoint.

Migration shall not silently transfer ownership.

### 7.3 Composite participants

A composite logical participant, such as a region or distributed service, may own an endpoint through a stable composite identity.

A temporary coordinator or active replica does not become the owner merely because it currently responds.

### 7.4 Replication

One logical endpoint may be represented through several bindings or service replicas when the implementation declares compatible replication behavior.

Every active representation shall preserve the endpoint’s declared contract.

Replica-specific:

- load;
- latency;
- health;
- locality;
- optional capacity;

may differ.

The semantic meaning of the endpoint must not silently differ.

### 7.5 Physical devices

Physical devices do not directly own ACS semantic endpoints merely because they exist.

A logical service representing a device may own an endpoint.

For example:

```text
Physical accelerator
    represented by a logical backend service

Logical backend service
    may own capability and health endpoints
```

This prevents physical inventory from becoming semantic authority.

## 8. Port identity and contracts

### 8.1 Port identity

Every port shall possess identity within its owning endpoint.

A port identity shall distinguish it from other ports even when several ports share:

- one transport;
- one queue;
- one secure session;
- one process;
- one message format.

### 8.2 Semantic contract

Every port shall define a semantic contract.

At minimum, the contract shall declare or reference:

- purpose;
- direction;
- supported signal domains;
- supported intents;
- subtype or schema compatibility;
- payload-reference behavior;
- admission expectations;
- authority expectations;
- resource boundaries;
- failure behavior.

### 8.3 Typed communication

A port shall not accept arbitrary untyped traffic merely because the source is authenticated or trusted.

Unsupported traffic shall be rejected before ordinary semantic or cognitive processing.

### 8.4 Contract stability

A port shall not silently change meaning while retaining an identity that causes existing participants to interpret it under an obsolete contract.

Material changes may require:

- a new contract version;
- renewed compatibility negotiation;
- renewed attachment;
- a new port identity;
- retirement of the previous port.

### 8.5 Optional features

A port contract may contain mandatory and optional capabilities.

Optional support must be reported explicitly.

Unsupported optional behavior must not be represented as available.

### 8.6 Implementation independence

The same port contract may be implemented through:

- local dispatch;
- shared memory;
- message queues;
- local sockets;
- remote secure sessions;
- bridge mediation;
- store-and-forward exchange;
- future transport mechanisms.

Changing implementation shall not silently change meaning.

## 9. Directionality

### 9.1 Direction is semantic

Direction is defined relative to the endpoint owning the port.

A physically bidirectional transport does not make every port bidirectional.

### 9.2 Ingress

An ingress port permits admitted participants to submit signals or payload references toward the owner.

Ingress does not guarantee:

- acceptance;
- processing;
- retention;
- resulting influence;
- acknowledgement;
- successful execution.

### 9.3 Egress

An egress port permits the owner to emit signals or payload references toward admitted recipients.

Egress does not grant unrestricted broadcast.

Recipient scope must remain bounded.

### 9.4 Bidirectional

A bidirectional port supports related communication in both directions under one coherent contract.

Unrelated inbound and outbound capabilities should normally use separate ports.

### 9.5 Asymmetric bidirectional

An asymmetric bidirectional port applies different:

- signal families;
- permissions;
- resources;
- authority;
- acknowledgement behavior;

to each direction.

Permission in one direction shall not imply equal permission in the other.

### 9.6 Direction changes

A material direction change is a contract change.

Changing a port from request-only to directive-capable, or from ingress-only to bidirectional, must not occur silently.

## 10. Endpoint and port visibility

### 10.1 Visibility is independent

The following remain separate:

```text
Endpoint visibility
    ≠ port visibility

Port visibility
    ≠ reachability

Reachability
    ≠ attachment

Attachment
    ≠ signal acceptance

Signal acceptance
    ≠ authority
```

### 10.2 Visibility policies

An endpoint or port may be:

- concealed;
- capability-referenced;
- relationship-visible;
- authenticated-visible;
- admitted-visible;
- publicly discoverable.

Implementations may use fewer categories when the resulting behavior preserves the same distinctions.

### 10.3 Concealed

A concealed endpoint or port is not advertised through ordinary discovery.

Concealment is not a substitute for authentication, authorization, or validation.

### 10.4 Capability-referenced

A capability-referenced endpoint or port may be learned or addressed through a bounded opaque reference.

The reference must not silently grant broader authority than its declared scope.

### 10.5 Relationship-visible

A relationship-visible port may be disclosed to participants possessing an eligible relationship.

Relationship eligibility does not guarantee attachment or use.

### 10.6 Authenticated-visible

An authenticated-visible port may disclose bounded compatibility information after identity authentication.

Authentication alone does not grant use.

### 10.7 Admitted-visible

An admitted-visible port becomes visible only after a defined admission step.

Visibility admission may remain narrower than operational admission.

### 10.8 Publicly discoverable

A publicly discoverable port may advertise an approved public contract.

Publicly discoverable does not mean:

- unauthenticated;
- unrestricted;
- trusted;
- free of budgets;
- directive-capable.

## 11. Progressive disclosure

A port may disclose different portions of its contract to different observers.

For example:

```text
Unknown observer
    no information or minimal compatibility hint

Authenticated observer
    broad purpose and supported public versions

Eligible relationship
    attachment requirements and permitted signal families

Authorized attachment
    complete contract for the granted scope
```

Reduced disclosure may omit capabilities.

It must not provide materially false compatibility, direction, authority, or delivery information.

No endpoint is required to provide complete port enumeration.

## 12. Discovery boundaries

Discovery may reveal:

- an endpoint identity;
- a bounded availability state;
- one or more permitted port descriptions;
- supported public compatibility versions;
- requirements for further admission.

Discovery shall not automatically:

- create a relationship;
- create a connection;
- create an attachment;
- grant authority;
- reveal every port;
- prove service readiness;
- prove ownership continuity.

Repeated discovery and enumeration shall remain bounded to prevent resource exhaustion and information leakage.

## 13. Bindings

### 13.1 Replaceability

Bindings are runtime implementation state and shall be replaceable.

An endpoint may have:

- no current binding;
- one binding;
- several compatible bindings;
- a local and remote binding;
- a direct and mediated binding.

### 13.2 Binding availability

A failed binding does not automatically mean:

- endpoint retirement;
- relationship loss;
- port removal;
- participant loss.

Another binding may remain or become available.

### 13.3 Binding security

A binding shall use the security protections required by its endpoint, port, relationship, and environment.

Transport reachability through a binding shall not bypass semantic admission.

### 13.4 Binding conflict

Incompatible bindings claiming to represent one endpoint must remain explicit until continuity and authority are established.

## 14. Attachments

### 14.1 Purpose

An attachment records that a relationship or connection may attempt a declared class of interaction with a port.

It connects:

- one relationship or connection;
- one port;
- one bounded scope.

### 14.2 Attachment scope

Scope may constrain:

- direction;
- signal domains;
- signal intents;
- subtype families;
- payload categories;
- rate;
- priority ceiling;
- recipient scope;
- mediation path;
- validity interval;
- acknowledgement behavior;
- capability requirements.

### 14.3 Minimum authority

Attachments should grant the minimum communication scope required.

Attachment to one port shall not grant access to every port on the endpoint.

### 14.4 Per-signal validation

Every signal remains individually subject to:

- schema validation;
- freshness;
- authorization;
- replay handling;
- payload policy;
- resource availability;
- target state.

A valid attachment does not guarantee signal acceptance.

### 14.5 Lifecycle

Attachments shall be:

- revocable;
- suspendable where supported;
- bounded in scope;
- distinguishable from relationships;
- distinguishable from connections.

The complete attachment state machine belongs to ACS-0005.

## 15. Admission and authority boundaries

### 15.1 Communication admission

ACS admission determines whether communication may:

- discover;
- attach;
- submit;
- receive;
- mediate;
- use resources;

through a port.

Domain-specific systems may perform an additional semantic admission step after ACS communication admission.

### 15.2 Relationship class

A port may restrict eligible relationship classes.

Relationship class describes why the association exists.

It does not grant unrestricted authority.

For example:

- a synaptic relationship does not bypass validation;
- an infrastructure relationship does not automatically control hardware;
- an immune relationship does not automatically quarantine;
- a bridge relationship does not automatically transform payloads.

### 15.3 Capability requirements

Privileged port operations should require explicit bounded capabilities.

A capability may constrain:

- subject;
- target;
- operation;
- signal family;
- parameters;
- resources;
- validity;
- delegation;
- audit requirements.

### 15.4 Requests and directives

A request asks a recipient to consider an operation.

A directive invokes an operation under explicitly granted authority.

A port accepting requests does not automatically accept directives.

### 15.5 Reachability is not authority

The ability to send bytes to a binding does not authorize port use.

Unauthorized traffic shall be rejected before ordinary semantic processing.

### 15.6 Locality is not authority

Same-process, same-host, or same-node placement shall not grant semantic authority that would be denied remotely.

Efficient local implementations may reduce transport overhead.

They shall not weaken architectural boundaries.

## 16. Mediation

### 16.1 Direct interaction

A direct interaction reaches the owning endpoint without a semantic intermediary altering the meaning of the interaction.

Direct does not necessarily mean:

- one transport hop;
- no encryption;
- same process;
- same machine.

### 16.2 Semantic mediation

A semantic mediator may, when authorized:

- filter;
- aggregate;
- redact;
- summarize;
- translate;
- enforce policy;
- apply backpressure;
- expose a facade.

Mediation must be represented when it materially affects:

- meaning;
- provenance;
- authority;
- freshness;
- confidence;
- payload access;
- delivery expectations.

### 16.3 Transport intermediaries

Routers, secure-session layers, schedulers, and transport relays that preserve semantic meaning are not necessarily semantic mediators.

Implementations must not confuse transport participation with semantic authority.

### 16.4 No authority amplification

A mediator shall not create greater authority than it is authorized to exercise.

Mediation may:

- preserve authority;
- narrow authority;
- reject authority;
- translate an equivalent bounded authority.

It shall not silently amplify it.

### 16.5 Provenance

Material semantic transformation shall preserve enough provenance to identify:

- original source;
- mediator;
- transformation;
- resulting source;
- relevant trust-boundary crossings.

### 16.6 Bounded mediation

Mediation chains shall remain bounded.

The architecture shall prevent:

- routing loops;
- transformation loops;
- unbounded latency;
- unbounded provenance growth;
- hidden authority amplification;
- repeated representation changes that evade duplicate detection.

## 17. Facade ports

### 17.1 Independent identity

A facade port shall possess its own identity and contract.

It must not impersonate an underlying port when its meaning differs materially.

### 17.2 Public compatibility

A public facade may expose a stable and independently implementable interface backed by internal services.

Its public behavior must be truthful and complete within its declared scope.

### 17.3 Reduced capability

A facade may expose less functionality than the underlying system.

Reduced scope must remain explicit.

### 17.4 Failure behavior

When underlying capabilities are unavailable, the facade shall return an honest state such as:

- unavailable;
- degraded;
- unsupported;
- partial;
- unknown.

It shall not invent success to conceal internal failure.

## 18. Resource boundaries

### 18.1 Every port is bounded

Every port shall declare or reference limits appropriate to its function.

Relevant limits may include:

- signal size;
- payload-reference size;
- referenced payload size;
- queue depth;
- queued bytes;
- rate;
- burst size;
- outstanding requests;
- recipient fan-out;
- retry count;
- acknowledgement volume;
- validation cost;
- compute cost;
- retention time;
- mediation cost.

### 18.2 Small does not mean cheap

A small signal may trigger expensive:

- validation;
- computation;
- fan-out;
- retrieval;
- transformation;
- response generation.

Admission shall consider bounded downstream cost where required.

### 18.3 Per-source isolation

One source or attachment shall not consume all shared port capacity merely by remaining below one global limit.

Ports serving several participants should preserve source or relationship accountability.

### 18.4 Backpressure

When capacity is insufficient, ports shall use explicit backpressure, rejection, deferral, throttling, or load shedding.

Unbounded queue growth is prohibited.

### 18.5 Protected communication

Port-level resource handling must preserve capacity required for:

- infrastructure;
- security;
- immune operation;
- lifecycle;
- recovery;
- physical safety;

according to applicable policy.

Detailed budget allocation belongs to ACS-0006.

### 18.6 Resource state is not trust state

Backpressure or overload does not automatically mean:

- distrust;
- relationship weakening;
- semantic rejection;
- immune suspicion;
- permanent failure.

Repeated refusal to honor valid backpressure may become separate security or immune evidence.

## 19. Port and endpoint state

Endpoints and ports may require states such as:

- declared;
- available;
- available with restrictions;
- degraded;
- suspended;
- migrating;
- unavailable;
- retired;
- conflicting;
- unknown.

The complete lifecycle and transitions belong to ACS-0005.

This specification requires that materially different states remain distinguishable.

## 20. Failure and uncertainty

### 20.1 Explicit states

The architecture shall distinguish where relevant:

- absent;
- concealed;
- unauthorized;
- unsupported;
- unavailable;
- unreachable;
- degraded;
- stale;
- conflicting;
- retired;
- unknown.

### 20.2 External disclosure

Security policy may intentionally present the same external response for several internal states.

Internal reasoning must preserve the actual state when known.

### 20.3 Nondisclosure is not absence

A participant shall not infer that a port is absent merely because:

- discovery omitted it;
- enumeration did not list it;
- a mediator refused disclosure;
- a generic rejection occurred;
- a facade exposes no equivalent capability.

### 20.4 Unreachable is not retired

Temporary route or binding loss shall not automatically retire an endpoint or port.

### 20.5 Unknown is valid

When identity, compatibility, continuity, authority, or availability cannot be established, the system shall retain explicit unknown or conflicting state.

Missing evidence must not become implied success.

## 21. Boundary with relationships and connections

### 21.1 Relationships

A relationship records why identified participants are associated.

An endpoint does not create a relationship merely by being visible.

A port does not create a relationship merely by being compatible.

### 21.2 Connections

A connection is a bounded runtime mechanism through which a relationship may exchange permitted communication.

A connection may bind to one or more eligible ports.

Connection loss does not automatically remove the endpoint, port, or relationship.

### 21.3 Attachments

An attachment associates scoped port use with a relationship or connection.

It does not replace either object.

### 21.4 Separation summary

The following remain distinct:

```text
Relationship
    why association exists

Endpoint
    which logical boundary is addressed

Port
    which semantic capability is exposed

Connection
    which runtime mechanism carries interaction

Binding
    where and how the mechanism currently operates

Attachment
    which scoped port use is admitted
```

## 22. Boundary with MEM

### 22.1 ACS responsibility

ACS defines:

- endpoint identity;
- port identity;
- communication contracts;
- visibility;
- attachments;
- signal transport;
- mediation;
- communication admission;
- communication backpressure;
- connection lifecycle.

### 22.2 MEM responsibility

MEM defines:

- logical memory identity;
- memory versions;
- memory roles;
- storage proposals;
- memory-specific admission;
- commitment;
- durability;
- retrieval;
- retention;
- deletion;
- recovery.

### 22.3 Memory services and ports

A memory service may own ACS endpoints and expose ACS ports.

The ACS port contract may define:

- which signal family carries a recall request;
- which relationships may attach;
- which payload references are allowed;
- which communication resources are available.

MEM defines:

- what recall means;
- what result states are possible;
- what memory scope was searched;
- whether commitment or durability occurred.

### 22.4 Required distinctions

The following shall remain explicit:

1. endpoint availability is not memory-role availability;
2. memory-role availability is not memory availability;
3. ACS attachment admission is not memory-operation admission;
4. an ACS acknowledgement is not memory acceptance or commitment;
5. a payload reference is not logical memory identity;
6. connection loss is not memory deletion;
7. ACS retry is not automatically a new memory operation.

Neither substrate may silently absorb the responsibilities of the other.

## 23. Boundary with runtime, security, health, and immune systems

### 23.1 Runtime

Runtime systems place, execute, migrate, and bind endpoint implementations.

Runtime placement does not define endpoint meaning or ownership.

### 23.2 Security

Security systems provide:

- identity;
- authentication;
- cryptographic protection;
- key custody;
- capability validation;
- revocation.

ACS defines which communication boundaries require those facilities.

Endpoint owners shall not automatically control private keys.

### 23.3 Health

Health systems may report:

- binding state;
- process state;
- transport state;
- capacity;
- latency;
- resource pressure.

Health evidence contributes to endpoint availability.

Process liveness alone does not prove semantic port readiness.

### 23.4 Immune systems

Immune systems may observe bounded evidence concerning:

- repeated invalid traffic;
- attachment abuse;
- unauthorized enumeration;
- conflicting endpoint identity;
- abnormal mediation;
- ignored backpressure;
- contract violations.

Observation does not automatically grant payload access, mutation authority, or physical-node control.

## 24. Public implementation requirements

A public ACS implementation claiming support for ACS-0004 shall document:

- endpoint identity model;
- endpoint ownership model;
- supported port direction forms;
- port-contract representation;
- visibility behavior;
- binding model;
- attachment model;
- compatibility and version behavior;
- resource ceilings;
- backpressure behavior;
- failure-state representation;
- unsupported features;
- known continuity limitations.

An implementation may support only a subset of optional capabilities.

Unsupported scope must remain explicit.

## 25. Conformance expectations

Conformance evidence should demonstrate that:

1. endpoint identity survives ordinary binding replacement;
2. address reuse does not silently create false continuity;
3. endpoint ownership is separate from hosting;
4. port meaning is separate from transport;
5. unsupported signal types are rejected;
6. port direction is enforced;
7. visibility does not grant authority;
8. one port attachment does not expose all endpoint ports;
9. attachment does not guarantee signal acceptance;
10. mediated transformation preserves required provenance;
11. resource exhaustion does not cause unbounded queue growth;
12. connection loss does not automatically erase endpoint identity;
13. unknown and conflicting states remain explicit;
14. public facades report degraded or unsupported behavior honestly;
15. MEM semantics are not redefined by ACS communication state.

Detailed fault and conformance testing may be refined by later specifications.

## 26. Prohibited interpretations

This specification shall not be interpreted to mean that:

- every participant must own an endpoint;
- every endpoint must be publicly discoverable;
- every endpoint must enumerate every port;
- every port requires a distinct socket;
- every port requires a separate process;
- every port is bidirectional;
- a visible port is an authorized port;
- an authenticated participant may use every visible port;
- endpoint ownership grants unlimited authority;
- local communication may bypass validation;
- one failed binding proves endpoint loss;
- a facade is identical to its backing services;
- a relationship is an endpoint;
- a port is a connection;
- a payload reference is a payload;
- ACS defines memory-operation meaning;
- public conformance requires disclosure of proprietary interfaces;
- concealment alone provides adequate security;
- endpoint continuity may be inferred solely from an address or name.

## 27. Initial architectural commitments

ACS-0004 establishes the following commitments:

1. endpoints and ports are logical ACS objects;
2. endpoint identity is independent of physical placement;
3. endpoint ownership is distinct from hosting;
4. ports belong to endpoints and expose bounded semantic capabilities;
5. port meaning is independent of transport implementation;
6. every port has an explicit contract;
7. communication direction is semantic rather than transport-derived;
8. visibility is distinct from reachability, attachment, and authority;
9. discovery does not require complete enumeration;
10. bindings are replaceable runtime associations;
11. attachments grant limited port-use scope;
12. every signal remains individually admissible;
13. mediation must not silently alter meaning or amplify authority;
14. facade ports possess independent identities and contracts;
15. every port operates within explicit resource limits;
16. overload produces bounded backpressure or degradation;
17. endpoint and port failure states remain explicit;
18. connection loss does not automatically destroy endpoint or port identity;
19. ACS and MEM preserve independent semantic authority;
20. public implementations may remain independently useful without disclosing restricted internal interfaces.

## 28. Open questions

The following questions remain for later specifications or implementation profiles:

- Which endpoint identity scopes are required for baseline conformance?
- Which port-contract fields must be universally represented?
- When does a contract change require a new port identity?
- May a port move between endpoints while preserving identity?
- Which visibility categories should be mandatory?
- How should opaque capability references be represented?
- Which endpoints should support enumeration?
- Which endpoint states belong to ACS-0005?
- How should attachments migrate across binding replacement?
- When must attachments be renewed after port-version changes?
- Which port classes require mandatory mediation?
- How should public facade compatibility be negotiated?
- What maximum mediation-chain behavior should baseline conformance require?
- How should endpoint replicas expose differing optional capabilities?
- Which resource dimensions must every port declare?
- How should endpoint availability be composed from several bindings?
- How should endpoint ownership transfer be represented?
- Which conflicts require security or immune evaluation?
- How should endpoint state be preserved across hibernation?
- Which ACS status should be exposed when a healthy endpoint’s domain-specific service is unavailable?
- How should implementations prove that local fast paths preserve the same authority boundaries as remote paths?

These questions do not permit implementations to weaken the distinctions already established.

## 29. Closing principle

> **Node must not mistake a reachable address for a participant, a visible endpoint for permission, a connected port for acceptance, or a successful transport exchange for semantic completion.**

Endpoints preserve logical communication boundaries while runtime infrastructure changes.

Ports preserve meaning while transport mechanisms change.

Relationships explain why communication exists.

Connections carry it.

Attachments bound it.

Authority remains explicit.

## Revision history

### Version 0.1 — 2026-07-16

- Established the public endpoint and port architecture.
- Defined endpoint, port, contract, binding, attachment, facade, visibility, direction, and availability.
- Separated endpoint identity from address, placement, hosting, and transport.
- Established logical endpoint ownership.
- Defined typed port contracts and semantic directionality.
- Defined visibility and progressive-disclosure requirements.
- Defined binding and attachment boundaries.
- Established direct, mediated, and facade communication models.
- Established port-level admission, authority, and resource requirements.
- Required explicit unavailable, concealed, unauthorized, conflicting, and unknown states.
- Defined the boundary between ACS endpoints and MEM operation semantics.
- Established public conformance and prohibited interpretations.
