#include <cstdlib>
#include <type_traits>
#include <utility>

#include "acs_test_fixture.hpp"
#include "core/acs/acs_transport_contract.hpp"

namespace acs = prometheus::core::acs;

template <typename Id>
Id id(const char* name_space, const char* value) {
    return *Id::parse(name_space, value);
}

acs::AcsAdmissionResult admission(
    const acs::AcsRegistrySnapshot& registry,
    acs::ConnectionId connection,
    acs::AcsAdmissionOutcome outcome) {
    acs::AcsAdmissionResult result{};
    result.outcome = outcome;
    result.request_id = id<acs::AdmissionRequestId>("admission", "transport");
    result.connection_id = connection;
    result.evaluated_generation = registry.generation;
    return result;
}

int main() {
    static_assert(noexcept(acs::validate_transport_submission(
        std::declval<const acs::AcsRegistrySnapshot&>(),
        std::declval<const acs::ConnectionStateSnapshot&>(),
        std::declval<const acs::AcsAdmissionResult&>(),
        std::declval<const acs::TransportBindingSnapshot&>(),
        std::declval<const acs::AttachmentSnapshot&>(),
        std::declval<const acs::SignalEnvelope&>())));

    acs::AcsRegistry registry{};
    acs_test::FixtureIds ids{};
    if (!acs_test::populate(registry, ids)) return EXIT_FAILURE;
    const auto snapshot = registry.snapshot();

    acs::ConnectionStateSnapshot state{};
    state.connection_id = ids.connection;
    state.lifecycle = acs::ConnectionLifecycle::active;
    state.operational = acs::OperationalCondition::nominal;
    state.enforcement = acs::EnforcementCondition::unrestricted;

    acs::TransportBindingSnapshot binding{};
    binding.id = id<acs::BindingId>("binding", "primary");
    binding.connection_id = ids.connection;
    binding.source_endpoint = ids.first_endpoint;
    binding.target_endpoint = ids.second_endpoint;
    binding.transport_profile = {"transport.profile.v1"};
    binding.signal_schema = {"schema.signal.v1"};
    binding.signal_schema_version = 1;
    binding.profile_support = acs::TransportProfileSupport::supported;
    binding.condition = acs::BindingCondition::active;
    binding.revision = acs::DescriptorRevision{1};

    acs::AttachmentSnapshot attachment{};
    attachment.id = id<acs::AttachmentId>("attachment", "target-port");
    attachment.connection_id = ids.connection;
    attachment.port_id = ids.second_port;
    attachment.participant = ids.first;
    attachment.authority = ids.authority;
    attachment.scope = {"connection.use"};
    attachment.condition = acs::AttachmentCondition::active;
    attachment.revision = acs::DescriptorRevision{1};

    acs::SignalEnvelope envelope{};
    envelope.id = id<acs::SignalId>("signal", "one");
    envelope.correlation_id = id<acs::CorrelationId>("correlation", "one");
    envelope.connection_id = ids.connection;
    envelope.binding_id = binding.id;
    envelope.attachment_id = attachment.id;
    envelope.source = ids.first;
    envelope.target = ids.second;
    envelope.source_endpoint = ids.first_endpoint;
    envelope.target_endpoint = ids.second_endpoint;
    envelope.source_port = ids.first_port;
    envelope.target_port = ids.second_port;
    envelope.domain = acs::SignalDomain::operational;
    envelope.intent = acs::SignalIntent::request;
    envelope.subtype = {"node.transport.probe"};
    envelope.schema = {"schema.signal.v1"};
    envelope.schema_version = 1;
    envelope.freshness = acs::FreshnessCondition::current;
    envelope.provenance = {ids.evidence};
    envelope.inline_value = {0x01, 0x02};

    const auto admitted = admission(
        snapshot, ids.connection, acs::AcsAdmissionOutcome::admitted);

    const auto validated = acs::validate_transport_submission(
        snapshot, state, admitted,
        binding, attachment, envelope);
    if (!validated.valid() ||
        validated.code != acs::TransportValidationCode::valid) {
        return EXIT_FAILURE;
    }

    auto degraded = binding;
    degraded.condition = acs::BindingCondition::degraded;
    if (acs::validate_transport_submission(
            snapshot, state, admitted,
            degraded, attachment, envelope).code !=
        acs::TransportValidationCode::valid_with_restrictions) {
        return EXIT_FAILURE;
    }

    auto unknown_binding = binding;
    unknown_binding.condition = acs::BindingCondition::unknown;
    if (acs::validate_transport_submission(
            snapshot, state, admitted,
            unknown_binding, attachment, envelope).code !=
        acs::TransportValidationCode::binding_unknown) {
        return EXIT_FAILURE;
    }
    unknown_binding = binding;
    unknown_binding.profile_support = acs::TransportProfileSupport::unsupported;
    if (acs::validate_transport_submission(
            snapshot, state, admitted,
            unknown_binding, attachment, envelope).code !=
        acs::TransportValidationCode::binding_unsupported) {
        return EXIT_FAILURE;
    }
    unknown_binding.profile_support = acs::TransportProfileSupport::unavailable;
    if (acs::validate_transport_submission(
            snapshot, state, admitted,
            unknown_binding, attachment, envelope).code !=
        acs::TransportValidationCode::binding_unavailable) {
        return EXIT_FAILURE;
    }

    auto inactive = attachment;
    inactive.condition = acs::AttachmentCondition::suspended;
    if (acs::validate_transport_submission(
            snapshot, state, admitted,
            binding, inactive, envelope).code !=
        acs::TransportValidationCode::attachment_inactive) {
        return EXIT_FAILURE;
    }
    inactive.condition = acs::AttachmentCondition::unknown;
    if (acs::validate_transport_submission(
            snapshot, state, admitted,
            binding, inactive, envelope).code !=
        acs::TransportValidationCode::attachment_unknown) {
        return EXIT_FAILURE;
    }
    inactive.condition = acs::AttachmentCondition::conflicting;
    if (acs::validate_transport_submission(
            snapshot, state, admitted,
            binding, inactive, envelope).code !=
        acs::TransportValidationCode::attachment_conflicting) {
        return EXIT_FAILURE;
    }

    auto establishing = state;
    establishing.lifecycle = acs::ConnectionLifecycle::establishing;
    if (acs::validate_transport_submission(
            snapshot, establishing, admitted,
            binding, attachment, envelope).code !=
        acs::TransportValidationCode::lifecycle_incompatible) {
        return EXIT_FAILURE;
    }
    establishing.lifecycle = acs::ConnectionLifecycle::unknown;
    if (acs::validate_transport_submission(
            snapshot, establishing, admitted,
            binding, attachment, envelope).code !=
        acs::TransportValidationCode::lifecycle_unknown) {
        return EXIT_FAILURE;
    }
    establishing.lifecycle = acs::ConnectionLifecycle::failed;
    if (acs::validate_transport_submission(
            snapshot, establishing, admitted,
            binding, attachment, envelope).code !=
        acs::TransportValidationCode::lifecycle_unavailable) {
        return EXIT_FAILURE;
    }
    if (acs::validate_transport_submission(
            snapshot, state,
            admission(snapshot, ids.connection, acs::AcsAdmissionOutcome::denied),
            binding, attachment, envelope).code !=
        acs::TransportValidationCode::admission_denied) {
        return EXIT_FAILURE;
    }
    if (acs::validate_transport_submission(
            snapshot, state,
            admission(snapshot, ids.connection, acs::AcsAdmissionOutcome::unknown),
            binding, attachment, envelope).code !=
        acs::TransportValidationCode::admission_unknown) {
        return EXIT_FAILURE;
    }
    if (acs::validate_transport_submission(
            snapshot, state,
            admission(snapshot, ids.connection, acs::AcsAdmissionOutcome::deferred),
            binding, attachment, envelope).code !=
        acs::TransportValidationCode::admission_deferred) {
        return EXIT_FAILURE;
    }
    if (acs::validate_transport_submission(
            snapshot, state,
            admission(snapshot, ids.connection, acs::AcsAdmissionOutcome::conflicting),
            binding, attachment, envelope).code !=
        acs::TransportValidationCode::admission_conflicting) {
        return EXIT_FAILURE;
    }
    if (acs::validate_transport_submission(
            snapshot, state,
            admission(snapshot, ids.connection, acs::AcsAdmissionOutcome::unavailable),
            binding, attachment, envelope).code !=
        acs::TransportValidationCode::admission_unavailable) {
        return EXIT_FAILURE;
    }
    auto mismatched_admission = admitted;
    mismatched_admission.connection_id =
        id<acs::ConnectionId>("connection", "other");
    if (acs::validate_transport_submission(
            snapshot, state, mismatched_admission,
            binding, attachment, envelope).code !=
        acs::TransportValidationCode::admission_conflicting) {
        return EXIT_FAILURE;
    }
    auto stale_admission = admitted;
    stale_admission.evaluated_generation = acs::RegistryGeneration{};
    if (acs::validate_transport_submission(
            snapshot, state, stale_admission,
            binding, attachment, envelope).code !=
        acs::TransportValidationCode::admission_stale) {
        return EXIT_FAILURE;
    }

    acs::TransportContractOptions small{};
    small.maximum_inline_signal_bytes = 1;
    if (acs::validate_transport_submission(
            snapshot, state, admitted,
            binding, attachment, envelope, small).code !=
        acs::TransportValidationCode::signal_too_large) {
        return EXIT_FAILURE;
    }

    acs::TransportContractOptions invalid_options{};
    invalid_options.maximum_inline_signal_bytes = 0;
    if (acs::validate_transport_submission(
            snapshot, state, admitted,
            binding, attachment, envelope, invalid_options).code !=
        acs::TransportValidationCode::invalid_configuration) {
        return EXIT_FAILURE;
    }

    auto excessive_provenance = envelope;
    excessive_provenance.provenance.assign(
        acs::kDefaultEvidenceReferences + 1, ids.evidence);
    if (acs::validate_transport_submission(
            snapshot, state, admitted,
            binding, attachment, excessive_provenance).code !=
        acs::TransportValidationCode::malformed_contract) {
        return EXIT_FAILURE;
    }

    auto stale_signal = envelope;
    stale_signal.freshness = acs::FreshnessCondition::stale;
    if (acs::validate_transport_submission(
            snapshot, state, admitted,
            binding, attachment, stale_signal).code !=
        acs::TransportValidationCode::freshness_stale) {
        return EXIT_FAILURE;
    }

    auto missing_provenance = envelope;
    missing_provenance.provenance = {
        id<acs::EvidenceId>("evidence", "absent")};
    if (acs::validate_transport_submission(
            snapshot, state, admitted,
            binding, attachment, missing_provenance).code !=
        acs::TransportValidationCode::provenance_invalid) {
        return EXIT_FAILURE;
    }

    auto stale_authority = snapshot;
    stale_authority.authorities.front().condition = acs::AuthorityCondition::stale;
    if (acs::validate_transport_submission(
            stale_authority, state, admitted,
            binding, attachment, envelope).code !=
        acs::TransportValidationCode::broken_reference) {
        return EXIT_FAILURE;
    }

    auto bad_schema = envelope;
    bad_schema.schema = {"schema.unsupported.v1"};
    if (acs::validate_transport_submission(
            snapshot, state, admitted,
            binding, attachment, bad_schema).code !=
        acs::TransportValidationCode::schema_incompatible) {
        return EXIT_FAILURE;
    }
    bad_schema = envelope;
    bad_schema.schema_version = 2;
    if (acs::validate_transport_submission(
            snapshot, state, admitted,
            binding, attachment, bad_schema).code !=
        acs::TransportValidationCode::schema_incompatible) {
        return EXIT_FAILURE;
    }

    auto bad_binding = binding;
    bad_binding.target_endpoint = ids.first_endpoint;
    if (acs::validate_transport_submission(
            snapshot, state, admitted,
            bad_binding, attachment, envelope).code !=
        acs::TransportValidationCode::broken_reference) {
        return EXIT_FAILURE;
    }

    auto restricted = attachment;
    restricted.condition = acs::AttachmentCondition::restricted;
    return acs::validate_transport_submission(
               snapshot, state,
               admission(snapshot, ids.connection,
                         acs::AcsAdmissionOutcome::admitted_with_restrictions),
               binding, restricted, envelope).code ==
           acs::TransportValidationCode::valid_with_restrictions
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}
