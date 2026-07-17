#ifndef TESTS_UNIT_CORE_ACS_TEST_FIXTURE_HPP
#define TESTS_UNIT_CORE_ACS_TEST_FIXTURE_HPP

#include "core/acs/acs_registry.hpp"

namespace acs_test {
namespace acs = prometheus::core::acs;

template <typename Id>
Id make_id(const char* ns, const char* value) { return *Id::parse(ns, value); }

struct FixtureIds {
    acs::ParticipantId first = make_id<acs::ParticipantId>("participant", "first");
    acs::ParticipantId second = make_id<acs::ParticipantId>("participant", "second");
    acs::AuthorityId authority = make_id<acs::AuthorityId>("authority", "connection");
    acs::CapabilityId capability = make_id<acs::CapabilityId>("capability", "connection-use");
    acs::EvidenceId evidence = make_id<acs::EvidenceId>("evidence", "admission");
    acs::EndpointId first_endpoint = make_id<acs::EndpointId>("endpoint", "first");
    acs::EndpointId second_endpoint = make_id<acs::EndpointId>("endpoint", "second");
    acs::PortId first_port = make_id<acs::PortId>("port", "first");
    acs::PortId second_port = make_id<acs::PortId>("port", "second");
    acs::RelationshipId relationship = make_id<acs::RelationshipId>("relationship", "first-second");
    acs::ConnectionId connection = make_id<acs::ConnectionId>("connection", "first-second");
    acs::ContainmentAuthorizationId containment = make_id<acs::ContainmentAuthorizationId>("containment", "one");
    acs::RestorationClearanceId clearance = make_id<acs::RestorationClearanceId>("clearance", "one");
};

inline bool populate(acs::AcsRegistry& registry, const FixtureIds& ids) {
    if (!registry.register_participant({ids.first, acs::DescriptorRevision{1}, acs::IdentityCondition::valid, true}).ok() ||
        !registry.register_participant({ids.second, acs::DescriptorRevision{1}, acs::IdentityCondition::valid, true}).ok() ||
        !registry.register_authority({ids.authority, ids.first, {"connection.use"}, acs::AuthorityCondition::valid, acs::DescriptorRevision{1}}).ok() ||
        !registry.register_capability({ids.capability, ids.authority, ids.first, {"connection.use"}, acs::AuthorityCondition::valid, acs::DescriptorRevision{1}}).ok() ||
        !registry.register_evidence({ids.evidence, ids.authority, ids.first, {"connection.use"}, acs::DescriptorRevision{1}, acs::FreshnessCondition::current}).ok() ||
        !registry.register_containment_authorization({ids.containment, ids.authority, ids.first, {"connection.use"}, acs::PublicReferenceCondition::current, acs::DescriptorRevision{1}, acs::FreshnessCondition::current}).ok() ||
        !registry.register_restoration_clearance({ids.clearance, ids.authority, ids.first, {"connection.use"}, acs::PublicReferenceCondition::cleared, {ids.evidence}, acs::DescriptorRevision{1}, acs::FreshnessCondition::current}).ok() ||
        !registry.register_endpoint({ids.first_endpoint, ids.first}).ok() ||
        !registry.register_endpoint({ids.second_endpoint, ids.second}).ok()) return false;
    acs::PortDescriptor source{}; source.id = ids.first_port; source.endpoint = ids.first_endpoint;
    source.purpose = {"connection.use"}; source.direction = acs::PortDirection::egress;
    source.accepted_schemas = {{"schema.signal.v1"}}; source.required_capability = ids.capability;
    source.failure_behavior = acs::FailureBehavior::defer;
    acs::PortDescriptor target = source; target.id = ids.second_port; target.endpoint = ids.second_endpoint;
    target.direction = acs::PortDirection::ingress;
    if (!registry.register_port(source).ok() || !registry.register_port(target).ok() ||
        !registry.register_relationship({ids.relationship, ids.first, ids.second, acs::RelationshipClass::infrastructure}).ok()) return false;
    acs::ConnectionDescriptor connection{}; connection.id = ids.connection; connection.relationship = ids.relationship;
    connection.source_endpoint = ids.first_endpoint; connection.source_port = ids.first_port;
    connection.target_endpoint = ids.second_endpoint; connection.target_port = ids.second_port;
    connection.required_capabilities = {ids.capability}; connection.required_evidence = {ids.evidence};
    return registry.register_connection(connection).ok();
}
}  // namespace acs_test
#endif
