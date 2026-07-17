#include <cstdlib>
#include <string>
#include <type_traits>

#include "core/acs/acs_registry.hpp"

namespace acs = prometheus::core::acs;

template <typename Id>
Id id(const char* ns, const char* value) { return *Id::parse(ns, value); }

int main() {
    static_assert(!std::is_copy_constructible_v<acs::AcsRegistry>);
    acs::RegistryOptions invalid_options{};
    invalid_options.maximum_connections = acs::kAbsoluteConnections + 1U;
    acs::AcsRegistry invalid_registry{invalid_options};
    if (invalid_registry.register_participant({id<acs::ParticipantId>("participant", "blocked")}).code !=
        acs::RegistryCode::invalid_configuration) return EXIT_FAILURE;

    acs::RegistryOptions options{};
    options.maximum_participants = 2;
    options.maximum_ports_per_endpoint = 1;
    acs::AcsRegistry registry{options};

    const auto p1 = id<acs::ParticipantId>("participant", "one");
    const auto p2 = id<acs::ParticipantId>("participant", "two");
    const auto authority = id<acs::AuthorityId>("authority", "one");
    const auto capability = id<acs::CapabilityId>("capability", "connect");
    const auto evidence = id<acs::EvidenceId>("evidence", "one");
    const auto endpoint1 = id<acs::EndpointId>("endpoint", "one");
    const auto endpoint2 = id<acs::EndpointId>("endpoint", "two");
    const auto port1 = id<acs::PortId>("port", "one");
    const auto port2 = id<acs::PortId>("port", "two");
    const auto relationship = id<acs::RelationshipId>("relationship", "one");
    const auto connection = id<acs::ConnectionId>("connection", "one");

    if (!registry.register_participant({p2, acs::DescriptorRevision{1}, acs::IdentityCondition::valid, true}).ok() ||
        !registry.register_participant({p1, acs::DescriptorRevision{1}, acs::IdentityCondition::valid, true}).ok() ||
        registry.register_participant({p1}).code != acs::RegistryCode::duplicate_identity ||
        registry.register_participant({id<acs::ParticipantId>("participant", "three")}).code != acs::RegistryCode::capacity_exhausted)
        return EXIT_FAILURE;

    if (registry.register_authority({authority, id<acs::ParticipantId>("participant", "missing"), {"connection"}, acs::AuthorityCondition::valid, acs::DescriptorRevision{1}}).code != acs::RegistryCode::missing_reference ||
        !registry.register_authority({authority, p1, {"connection"}, acs::AuthorityCondition::valid, acs::DescriptorRevision{1}}).ok() ||
        !registry.register_capability({capability, authority, p1, {"connection.use"}, acs::AuthorityCondition::valid, acs::DescriptorRevision{1}}).ok() ||
        !registry.register_evidence({evidence, authority, p1, {"connection.use"}, acs::DescriptorRevision{1}, acs::FreshnessCondition::current}).ok())
        return EXIT_FAILURE;

    if (!registry.register_endpoint({endpoint1, p1}).ok() || !registry.register_endpoint({endpoint2, p2}).ok()) return EXIT_FAILURE;
    acs::PortDescriptor first{};
    first.id = port1; first.endpoint = endpoint1; first.purpose = {"signal.request"};
    first.direction = acs::PortDirection::egress; first.accepted_schemas = {{"schema.request.v1"}};
    first.required_capability = capability; first.budgets = {{{"messages"}, acs::BudgetUnit::messages, 10}};
    first.failure_behavior = acs::FailureBehavior::defer;
    if (!registry.register_port(first).ok()) return EXIT_FAILURE;
    first.id = id<acs::PortId>("port", "overflow");
    if (registry.register_port(first).code != acs::RegistryCode::capacity_exhausted) return EXIT_FAILURE;
    acs::PortDescriptor second{};
    second.id = port2; second.endpoint = endpoint2; second.purpose = {"signal.request"};
    second.direction = acs::PortDirection::ingress; second.accepted_schemas = {{"schema.request.v1"}};
    second.required_capability = capability; second.failure_behavior = acs::FailureBehavior::reject;
    if (!registry.register_port(second).ok() ||
        !registry.register_relationship({relationship, p1, p2, acs::RelationshipClass::infrastructure}).ok()) return EXIT_FAILURE;

    acs::ConnectionDescriptor descriptor{};
    descriptor.id = connection; descriptor.relationship = relationship;
    descriptor.source_endpoint = endpoint1; descriptor.source_port = port1;
    descriptor.target_endpoint = endpoint2; descriptor.target_port = port2;
    descriptor.required_capabilities = {capability}; descriptor.required_evidence = {evidence};
    const auto before = registry.generation().value();
    if (!registry.register_connection(descriptor).ok() || registry.generation().value() != before + 1) return EXIT_FAILURE;
    descriptor.id = id<acs::ConnectionId>("connection", "broken");
    descriptor.target_port = id<acs::PortId>("port", "missing");
    const auto failed_generation = registry.generation();
    if (registry.register_connection(descriptor).code != acs::RegistryCode::missing_reference ||
        registry.generation() != failed_generation) return EXIT_FAILURE;

    const auto snapshot = registry.snapshot();
    return snapshot.participants.size() == 2 && snapshot.participants[0].id == p1 &&
           snapshot.participants[1].id == p2 && snapshot.connections.size() == 1 &&
           snapshot.generation == registry.generation() ? EXIT_SUCCESS : EXIT_FAILURE;
}
