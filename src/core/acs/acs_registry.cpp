#include "acs_registry.hpp"

#include <algorithm>
#include <exception>
#include <mutex>
#include <utility>

namespace prometheus::core::acs {
namespace {

template <typename T, typename Id>
bool contains(const std::vector<T>& values, const Id& id) {
    return std::any_of(values.begin(), values.end(), [&](const auto& value) {
        return value.id == id;
    });
}

bool valid_options(const RegistryOptions& value) {
    return value.maximum_participants > 0 && value.maximum_participants <= kAbsoluteParticipants &&
           value.maximum_authorities > 0 && value.maximum_authorities <= kAbsoluteAuthorities &&
           value.maximum_capabilities > 0 && value.maximum_capabilities <= kAbsoluteCapabilities &&
           value.maximum_delegations > 0 && value.maximum_delegations <= kAbsoluteCapabilities &&
           value.maximum_revocations > 0 && value.maximum_revocations <= kAbsoluteCapabilities &&
           value.maximum_evidence > 0 && value.maximum_evidence <= kAbsoluteCapabilities &&
           value.maximum_endpoints > 0 && value.maximum_endpoints <= kAbsoluteEndpoints &&
           value.maximum_ports_per_endpoint > 0 && value.maximum_ports_per_endpoint <= 256 &&
           value.maximum_relationships > 0 && value.maximum_relationships <= kAbsoluteRelationships &&
           value.maximum_connections > 0 && value.maximum_connections <= kAbsoluteConnections &&
           value.maximum_identifier_bytes > 0 && value.maximum_identifier_bytes <= kAbsoluteIdentifierBytes &&
           value.maximum_evidence_references > 0 && value.maximum_evidence_references <= kAbsoluteEvidenceReferences &&
           value.maximum_budget_dimensions > 0 && value.maximum_budget_dimensions <= kAbsoluteBudgetDimensions;
}

template <typename T>
void sort_values(std::vector<T>& values) {
    std::sort(values.begin(), values.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.id < rhs.id;
    });
}

}  // namespace

AcsRegistry::AcsRegistry(RegistryOptions options)
    : options_(std::move(options)), options_valid_(valid_options(options_)) {}

#define ACS_REGISTER_BEGIN(Value, Collection, Limit) \
    std::unique_lock<std::shared_mutex> lock(mutex_); \
    if (!options_valid_) return {RegistryCode::invalid_configuration, {}, "invalid registry options", generation_}; \
    const std::string identity = (Value).id.canonical(); \
    if (!(Value).id.valid()) return {RegistryCode::invalid_descriptor, identity, "invalid identity", generation_}; \
    if (contains(data_.Collection, (Value).id)) return {RegistryCode::duplicate_identity, identity, "duplicate identity", generation_}; \
    if (data_.Collection.size() >= (Limit)) return {RegistryCode::capacity_exhausted, identity, "capacity exhausted", generation_}

#define ACS_REGISTER_END(Value, Collection) \
    const auto next = generation_.next(); \
    if (!next) return {RegistryCode::generation_overflow, identity, "generation overflow", generation_}; \
    try { data_.Collection.push_back(std::move(Value)); } \
    catch (const std::exception&) { return {RegistryCode::resource_exhausted, identity, "allocation failed", generation_}; } \
    generation_ = *next; \
    return {RegistryCode::success, identity, {}, generation_}

RegistryResult AcsRegistry::register_participant(ParticipantDescriptor value) {
    ACS_REGISTER_BEGIN(value, participants, options_.maximum_participants);
    if (value.revision.value() == 0) return {RegistryCode::invalid_descriptor, identity, "zero revision", generation_};
    ACS_REGISTER_END(value, participants);
}

RegistryResult AcsRegistry::register_authority(AuthorityDescriptor value) {
    ACS_REGISTER_BEGIN(value, authorities, options_.maximum_authorities);
    if (!value.owner.valid() || !contains(data_.participants, value.owner) || !value.scope.valid() || value.revision.value() == 0)
        return {RegistryCode::missing_reference, identity, "invalid authority reference", generation_};
    ACS_REGISTER_END(value, authorities);
}

RegistryResult AcsRegistry::register_capability(CapabilityDescriptor value) {
    ACS_REGISTER_BEGIN(value, capabilities, options_.maximum_capabilities);
    if (!contains(data_.authorities, value.issuer) || !contains(data_.participants, value.subject) || !value.scope.valid() || value.revision.value() == 0)
        return {RegistryCode::missing_reference, identity, "invalid capability reference", generation_};
    ACS_REGISTER_END(value, capabilities);
}

RegistryResult AcsRegistry::register_delegation(DelegationReference value) {
    ACS_REGISTER_BEGIN(value, delegations, options_.maximum_delegations);
    if (!contains(data_.authorities, value.issuer) || !contains(data_.capabilities, value.capability) || !contains(data_.participants, value.delegate) || !value.scope.valid())
        return {RegistryCode::missing_reference, identity, "invalid delegation reference", generation_};
    ACS_REGISTER_END(value, delegations);
}

RegistryResult AcsRegistry::register_revocation(RevocationReference value) {
    ACS_REGISTER_BEGIN(value, revocations, options_.maximum_revocations);
    if (!contains(data_.authorities, value.issuer) || !contains(data_.capabilities, value.capability) || !contains(data_.participants, value.subject) || !value.scope.valid())
        return {RegistryCode::missing_reference, identity, "invalid revocation reference", generation_};
    ACS_REGISTER_END(value, revocations);
}

RegistryResult AcsRegistry::register_evidence(EvidenceReference value) {
    ACS_REGISTER_BEGIN(value, evidence, options_.maximum_evidence);
    if (!contains(data_.authorities, value.issuer) || !contains(data_.participants, value.subject) || !value.scope.valid())
        return {RegistryCode::missing_reference, identity, "invalid evidence reference", generation_};
    ACS_REGISTER_END(value, evidence);
}

RegistryResult AcsRegistry::register_restriction(RestrictionReference value) {
    ACS_REGISTER_BEGIN(value, restrictions, options_.maximum_evidence);
    if (!contains(data_.authorities, value.issuer) || !contains(data_.participants, value.subject) || !value.scope.valid() || value.evidence.size() > options_.maximum_evidence_references)
        return {RegistryCode::missing_reference, identity, "invalid restriction reference", generation_};
    for (const auto& item : value.evidence) if (!contains(data_.evidence, item))
        return {RegistryCode::missing_reference, identity, "missing restriction evidence", generation_};
    ACS_REGISTER_END(value, restrictions);
}

RegistryResult AcsRegistry::register_containment_authorization(ContainmentAuthorizationReference value) {
    ACS_REGISTER_BEGIN(value, containment_authorizations, options_.maximum_authorities);
    if (!contains(data_.authorities, value.issuer) || !contains(data_.participants, value.subject) || !value.scope.valid())
        return {RegistryCode::missing_reference, identity, "invalid containment authorization", generation_};
    ACS_REGISTER_END(value, containment_authorizations);
}

RegistryResult AcsRegistry::register_restoration_clearance(RestorationClearanceReference value) {
    ACS_REGISTER_BEGIN(value, restoration_clearances, options_.maximum_evidence);
    if (!contains(data_.authorities, value.issuer) || !contains(data_.participants, value.subject) || !value.scope.valid() || value.evidence.size() > options_.maximum_evidence_references)
        return {RegistryCode::missing_reference, identity, "invalid restoration clearance", generation_};
    for (const auto& item : value.evidence) if (!contains(data_.evidence, item))
        return {RegistryCode::missing_reference, identity, "missing clearance evidence", generation_};
    ACS_REGISTER_END(value, restoration_clearances);
}

RegistryResult AcsRegistry::register_endpoint(EndpointDescriptor value) {
    ACS_REGISTER_BEGIN(value, endpoints, options_.maximum_endpoints);
    if (!contains(data_.participants, value.owner) || value.revision.value() == 0)
        return {RegistryCode::missing_reference, identity, "missing endpoint owner", generation_};
    ACS_REGISTER_END(value, endpoints);
}

RegistryResult AcsRegistry::register_port(PortDescriptor value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (!options_valid_) return {RegistryCode::invalid_configuration, {}, "invalid registry options", generation_};
    const std::string identity = value.id.canonical();
    if (!value.id.valid() || !value.purpose.valid() || value.direction == PortDirection::unknown ||
        value.failure_behavior == FailureBehavior::unknown || value.revision.value() == 0 ||
        value.accepted_schemas.size() > options_.maximum_evidence_references ||
        value.budgets.size() > options_.maximum_budget_dimensions)
        return {RegistryCode::invalid_descriptor, identity, "invalid port", generation_};
    if (contains(data_.ports, value.id)) return {RegistryCode::duplicate_identity, identity, "duplicate identity", generation_};
    if (!contains(data_.endpoints, value.endpoint) ||
        (value.required_capability && !contains(data_.capabilities, *value.required_capability)))
        return {RegistryCode::missing_reference, identity, "missing port reference", generation_};
    const auto count = std::count_if(data_.ports.begin(), data_.ports.end(), [&](const auto& port) { return port.endpoint == value.endpoint; });
    if (static_cast<std::size_t>(count) >= options_.maximum_ports_per_endpoint)
        return {RegistryCode::capacity_exhausted, identity, "endpoint port capacity exhausted", generation_};
    for (const auto& schema : value.accepted_schemas) if (!schema.valid())
        return {RegistryCode::invalid_descriptor, identity, "invalid schema", generation_};
    for (const auto& budget : value.budgets) if (!budget.dimension.valid() || budget.unit == BudgetUnit::unknown)
        return {RegistryCode::invalid_descriptor, identity, "invalid budget", generation_};
    const auto next = generation_.next();
    if (!next) return {RegistryCode::generation_overflow, identity, "generation overflow", generation_};
    try { data_.ports.push_back(std::move(value)); } catch (const std::exception&) {
        return {RegistryCode::resource_exhausted, identity, "allocation failed", generation_};
    }
    generation_ = *next;
    return {RegistryCode::success, identity, {}, generation_};
}

RegistryResult AcsRegistry::register_relationship(RelationshipDescriptor value) {
    ACS_REGISTER_BEGIN(value, relationships, options_.maximum_relationships);
    if (!contains(data_.participants, value.first) || !contains(data_.participants, value.second) ||
        value.first == value.second || value.relationship_class == RelationshipClass::unknown || value.revision.value() == 0)
        return {RegistryCode::missing_reference, identity, "invalid relationship reference", generation_};
    ACS_REGISTER_END(value, relationships);
}

RegistryResult AcsRegistry::register_connection(ConnectionDescriptor value) {
    ACS_REGISTER_BEGIN(value, connections, options_.maximum_connections);
    if (!contains(data_.relationships, value.relationship) || !contains(data_.endpoints, value.source_endpoint) ||
        !contains(data_.endpoints, value.target_endpoint) || !contains(data_.ports, value.source_port) ||
        !contains(data_.ports, value.target_port) || value.required_capabilities.size() > options_.maximum_evidence_references ||
        value.required_evidence.size() > options_.maximum_evidence_references || value.revision.value() == 0)
        return {RegistryCode::missing_reference, identity, "invalid connection reference", generation_};
    const auto port_matches = [&](const PortId& id, const EndpointId& endpoint) {
        return std::any_of(data_.ports.begin(), data_.ports.end(), [&](const auto& port) { return port.id == id && port.endpoint == endpoint; });
    };
    if (!port_matches(value.source_port, value.source_endpoint) || !port_matches(value.target_port, value.target_endpoint))
        return {RegistryCode::missing_reference, identity, "port endpoint mismatch", generation_};
    for (const auto& capability : value.required_capabilities) if (!contains(data_.capabilities, capability))
        return {RegistryCode::missing_reference, identity, "missing connection capability", generation_};
    for (const auto& item : value.required_evidence) if (!contains(data_.evidence, item))
        return {RegistryCode::missing_reference, identity, "missing connection evidence", generation_};
    if (value.restriction && !contains(data_.restrictions, *value.restriction))
        return {RegistryCode::missing_reference, identity, "missing connection restriction", generation_};
    ACS_REGISTER_END(value, connections);
}

AcsRegistrySnapshot AcsRegistry::snapshot() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    AcsRegistrySnapshot result = data_;
    result.generation = generation_;
    sort_values(result.participants); sort_values(result.authorities); sort_values(result.capabilities);
    sort_values(result.delegations); sort_values(result.revocations); sort_values(result.evidence);
    sort_values(result.restrictions); sort_values(result.containment_authorizations);
    sort_values(result.restoration_clearances); sort_values(result.endpoints); sort_values(result.ports);
    sort_values(result.relationships); sort_values(result.connections);
    return result;
}

RegistryGeneration AcsRegistry::generation() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return generation_;
}

bool AcsRegistry::contains_connection(const ConnectionId& id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return contains(data_.connections, id);
}

#undef ACS_REGISTER_BEGIN
#undef ACS_REGISTER_END
}  // namespace prometheus::core::acs
