#include "acs_transport_contract.hpp"

#include <algorithm>
#include <utility>

namespace prometheus::core::acs {
namespace {

template <typename T, typename Id>
const T* locate(const std::vector<T>& values, const Id& id) {
    const auto found = std::find_if(values.begin(), values.end(),
                                    [&](const auto& value) { return value.id == id; });
    return found == values.end() ? nullptr : &*found;
}

bool supports_schema(const PortDescriptor& port, const ScopeReference& schema) {
    return std::any_of(port.accepted_schemas.begin(), port.accepted_schemas.end(),
                       [&](const auto& value) { return value.value == schema.value; });
}

bool supports_egress(PortDirection direction) {
    return direction == PortDirection::egress ||
           direction == PortDirection::bidirectional;
}

bool supports_ingress(PortDirection direction) {
    return direction == PortDirection::ingress ||
           direction == PortDirection::bidirectional;
}

TransportValidationResult result(
    TransportValidationCode code,
    std::string subject,
    std::string message,
    const TransportContractOptions& options) {
    TransportValidationResult value{};
    value.code = code;
    value.subject = std::move(subject);
    if (message.size() > options.maximum_diagnostic_bytes) {
        message.resize(options.maximum_diagnostic_bytes);
        value.diagnostic_truncated = true;
    }
    value.message = std::move(message);
    return value;
}

bool options_valid(const TransportContractOptions& options) {
    return options.maximum_inline_signal_bytes > 0 &&
           options.maximum_inline_signal_bytes <= kAbsoluteInlineSignalBytes &&
           options.maximum_provenance_references > 0 &&
           options.maximum_provenance_references <= kAbsoluteEvidenceReferences &&
           options.maximum_diagnostic_bytes > 0 &&
           options.maximum_diagnostic_bytes <= kAbsoluteDiagnosticBytes;
}

TransportValidationResult validate_impl(
    const AcsRegistrySnapshot& registry,
    const ConnectionStateSnapshot& state,
    const AcsAdmissionResult& admission,
    const TransportBindingSnapshot& binding,
    const AttachmentSnapshot& attachment,
    const SignalEnvelope& envelope,
    const TransportContractOptions& options) {
    if (!options_valid(options)) {
        return result(TransportValidationCode::invalid_configuration, {},
                      "invalid transport-contract options", options);
    }
    if (!binding.id.valid() || !binding.connection_id.valid() ||
        !binding.source_endpoint.valid() || !binding.target_endpoint.valid() ||
        !binding.transport_profile.valid() || !binding.signal_schema.valid() ||
        binding.signal_schema_version == 0 || binding.revision.value() == 0 ||
        !attachment.id.valid() || !attachment.connection_id.valid() ||
        !attachment.port_id.valid() || !attachment.participant.valid() ||
        !attachment.authority.valid() || !attachment.scope.valid() ||
        attachment.revision.value() == 0 || !envelope.id.valid() ||
        !envelope.connection_id.valid() || !envelope.binding_id.valid() ||
        !envelope.attachment_id.valid() || !envelope.source.valid() ||
        !envelope.target.valid() || !envelope.source_endpoint.valid() ||
        !envelope.target_endpoint.valid() || !envelope.source_port.valid() ||
        !envelope.target_port.valid() || envelope.domain == SignalDomain::unknown ||
        envelope.intent == SignalIntent::unknown || !envelope.subtype.valid() ||
        !envelope.schema.valid() || envelope.schema_version == 0 ||
        (envelope.correlation_id && !envelope.correlation_id->valid())) {
        return result(TransportValidationCode::malformed_contract,
                      envelope.id.canonical(),
                      "transport-facing contract contains an invalid field", options);
    }
    if (envelope.inline_value.size() > options.maximum_inline_signal_bytes) {
        return result(TransportValidationCode::signal_too_large,
                      envelope.id.canonical(),
                      "inline signal value exceeds its configured bound", options);
    }
    if (envelope.provenance.size() > options.maximum_provenance_references ||
        std::any_of(envelope.provenance.begin(), envelope.provenance.end(),
                    [](const auto& id) { return !id.valid(); })) {
        return result(TransportValidationCode::malformed_contract,
                      envelope.id.canonical(),
                      "signal provenance is invalid or exceeds its bound", options);
    }
    switch (envelope.freshness) {
        case FreshnessCondition::current: break;
        case FreshnessCondition::unknown:
            return result(TransportValidationCode::freshness_unknown,
                          envelope.id.canonical(),
                          "signal freshness is unknown", options);
        case FreshnessCondition::stale:
        case FreshnessCondition::expired:
            return result(TransportValidationCode::freshness_stale,
                          envelope.id.canonical(),
                          "signal is stale or expired", options);
        case FreshnessCondition::conflicting:
            return result(TransportValidationCode::freshness_conflicting,
                          envelope.id.canonical(),
                          "signal freshness evidence conflicts", options);
        case FreshnessCondition::unavailable:
            return result(TransportValidationCode::freshness_unavailable,
                          envelope.id.canonical(),
                          "signal freshness evidence is unavailable", options);
    }

    const auto* connection = locate(registry.connections, envelope.connection_id);
    const auto* relationship = connection
        ? locate(registry.relationships, connection->relationship)
        : nullptr;
    const auto* source_endpoint = locate(registry.endpoints, envelope.source_endpoint);
    const auto* target_endpoint = locate(registry.endpoints, envelope.target_endpoint);
    const auto* source_port = locate(registry.ports, envelope.source_port);
    const auto* target_port = locate(registry.ports, envelope.target_port);
    const auto* source = locate(registry.participants, envelope.source);
    const auto* target = locate(registry.participants, envelope.target);
    const auto* authority = locate(registry.authorities, attachment.authority);
    if (!connection || !relationship || !source_endpoint || !target_endpoint ||
        !source_port || !target_port || !source || !target || !authority) {
        return result(TransportValidationCode::broken_reference,
                      envelope.connection_id.canonical(),
                      "required registry descriptor is absent", options);
    }
    for (const auto& evidence_id : envelope.provenance) {
        const auto* evidence = locate(registry.evidence, evidence_id);
        if (!evidence || evidence->freshness != FreshnessCondition::current) {
            return result(TransportValidationCode::provenance_invalid,
                          evidence_id.canonical(),
                          "signal provenance is absent or not current", options);
        }
    }
    const bool participants_match =
        (relationship->first == envelope.source &&
         relationship->second == envelope.target) ||
        (relationship->second == envelope.source &&
         relationship->first == envelope.target);
    if (!participants_match || source_endpoint->owner != envelope.source ||
        target_endpoint->owner != envelope.target ||
        connection->source_endpoint != envelope.source_endpoint ||
        connection->target_endpoint != envelope.target_endpoint ||
        connection->source_port != envelope.source_port ||
        connection->target_port != envelope.target_port ||
        source_port->endpoint != envelope.source_endpoint ||
        target_port->endpoint != envelope.target_endpoint) {
        return result(TransportValidationCode::broken_reference,
                      envelope.connection_id.canonical(),
                      "envelope does not match the registered connection path", options);
    }
    if (!relationship->enabled || !source_endpoint->enabled ||
        !target_endpoint->enabled || !source_port->enabled || !target_port->enabled ||
        !source->enabled || !target->enabled ||
        source->identity_condition != IdentityCondition::valid ||
        target->identity_condition != IdentityCondition::valid ||
        authority->condition != AuthorityCondition::valid ||
        authority->owner != attachment.participant ||
        authority->scope.value != attachment.scope.value) {
        return result(TransportValidationCode::broken_reference,
                      envelope.connection_id.canonical(),
                      "required public descriptor is disabled or not current", options);
    }
    if (binding.connection_id != envelope.connection_id ||
        binding.id != envelope.binding_id ||
        binding.source_endpoint != envelope.source_endpoint ||
        binding.target_endpoint != envelope.target_endpoint) {
        return result(TransportValidationCode::broken_reference,
                      envelope.binding_id.canonical(),
                      "binding does not match the signal connection", options);
    }
    if (attachment.connection_id != envelope.connection_id ||
        attachment.id != envelope.attachment_id ||
        attachment.port_id != envelope.target_port ||
        attachment.participant != envelope.source) {
        return result(TransportValidationCode::broken_reference,
                      envelope.attachment_id.canonical(),
                      "attachment does not authorize this participant and target port", options);
    }

    bool restricted = false;
    switch (binding.profile_support) {
        case TransportProfileSupport::supported: break;
        case TransportProfileSupport::unknown:
            return result(TransportValidationCode::binding_unknown,
                          binding.id.canonical(),
                          "binding profile support is unknown", options);
        case TransportProfileSupport::unsupported:
            return result(TransportValidationCode::binding_unsupported,
                          binding.id.canonical(),
                          "binding profile is unsupported", options);
        case TransportProfileSupport::unavailable:
            return result(TransportValidationCode::binding_unavailable,
                          binding.id.canonical(),
                          "binding profile evidence is unavailable", options);
    }
    switch (binding.condition) {
        case BindingCondition::active: break;
        case BindingCondition::degraded: restricted = true; break;
        case BindingCondition::conflicting:
            return result(TransportValidationCode::binding_conflicting,
                          binding.id.canonical(),
                          "binding state conflicts", options);
        case BindingCondition::blocked:
        case BindingCondition::draining:
        case BindingCondition::failed:
        case BindingCondition::closed:
        case BindingCondition::stale:
            return result(TransportValidationCode::binding_unavailable,
                          binding.id.canonical(), "binding is unavailable", options);
        case BindingCondition::unknown:
        case BindingCondition::proposed:
        case BindingCondition::establishing:
        case BindingCondition::migrating:
            return result(TransportValidationCode::binding_unknown,
                          binding.id.canonical(),
                          "binding readiness is not established", options);
    }
    switch (attachment.condition) {
        case AttachmentCondition::active: break;
        case AttachmentCondition::restricted: restricted = true; break;
        case AttachmentCondition::unknown:
        case AttachmentCondition::proposed:
        case AttachmentCondition::admitted:
            return result(TransportValidationCode::attachment_unknown,
                          attachment.id.canonical(),
                          "attachment readiness is not established", options);
        case AttachmentCondition::suspended:
        case AttachmentCondition::revoked:
        case AttachmentCondition::expired:
        case AttachmentCondition::closed:
        case AttachmentCondition::draining:
            return result(TransportValidationCode::attachment_inactive,
                          attachment.id.canonical(), "attachment is not active", options);
        case AttachmentCondition::conflicting:
            return result(TransportValidationCode::attachment_conflicting,
                          attachment.id.canonical(),
                          "attachment state conflicts", options);
    }
    if (state.connection_id != envelope.connection_id) {
        return result(TransportValidationCode::broken_reference,
                      envelope.connection_id.canonical(),
                      "connection state does not match the signal connection", options);
    }
    if (state.lifecycle == ConnectionLifecycle::unknown ||
        state.operational == OperationalCondition::unknown ||
        state.enforcement == EnforcementCondition::unknown) {
        return result(TransportValidationCode::lifecycle_unknown,
                      envelope.connection_id.canonical(),
                      "connection state is not known", options);
    }
    if (state.lifecycle == ConnectionLifecycle::closed ||
        state.lifecycle == ConnectionLifecycle::failed ||
        state.operational == OperationalCondition::unavailable ||
        state.operational == OperationalCondition::failed) {
        return result(TransportValidationCode::lifecycle_unavailable,
                      envelope.connection_id.canonical(),
                      "connection is unavailable", options);
    }
    if (state.lifecycle != ConnectionLifecycle::active ||
        state.enforcement == EnforcementCondition::quarantined ||
        state.enforcement == EnforcementCondition::revoked) {
        return result(TransportValidationCode::lifecycle_incompatible,
                      envelope.connection_id.canonical(),
                      "connection state is not eligible for signal submission", options);
    }
    if (state.operational == OperationalCondition::degraded ||
        state.enforcement == EnforcementCondition::restricted) {
        restricted = true;
    }
    if (!admission.request_id.valid() ||
        admission.connection_id != envelope.connection_id) {
        return result(TransportValidationCode::admission_conflicting,
                      envelope.connection_id.canonical(),
                      "admission result does not match this signal context", options);
    }
    if (admission.evaluated_generation != registry.generation) {
        return result(TransportValidationCode::admission_stale,
                      envelope.connection_id.canonical(),
                      "admission result does not match the registry generation", options);
    }
    switch (admission.outcome) {
        case AcsAdmissionOutcome::admitted: break;
        case AcsAdmissionOutcome::admitted_with_restrictions:
            restricted = true;
            break;
        case AcsAdmissionOutcome::unknown:
            return result(TransportValidationCode::admission_unknown,
                          envelope.connection_id.canonical(),
                          "signal admission is unknown", options);
        case AcsAdmissionOutcome::conflicting:
            return result(TransportValidationCode::admission_conflicting,
                          envelope.connection_id.canonical(),
                          "signal admission evidence conflicts", options);
        case AcsAdmissionOutcome::unavailable:
            return result(TransportValidationCode::admission_unavailable,
                          envelope.connection_id.canonical(),
                          "signal admission is unavailable", options);
        case AcsAdmissionOutcome::deferred:
            return result(TransportValidationCode::admission_deferred,
                          envelope.connection_id.canonical(),
                          "signal admission is deferred", options);
        case AcsAdmissionOutcome::denied:
        case AcsAdmissionOutcome::rejected:
            return result(TransportValidationCode::admission_denied,
                          envelope.connection_id.canonical(),
                          "admission denied signal submission", options);
    }
    if (!supports_egress(source_port->direction) ||
        !supports_ingress(target_port->direction)) {
        return result(TransportValidationCode::direction_incompatible,
                      envelope.id.canonical(),
                      "port direction does not permit this signal path", options);
    }
    if (!supports_schema(*source_port, envelope.schema) ||
        !supports_schema(*target_port, envelope.schema) ||
        binding.signal_schema.value != envelope.schema.value ||
        binding.signal_schema_version != envelope.schema_version) {
        return result(TransportValidationCode::schema_incompatible,
                      envelope.schema.value,
                      "signal schema is not accepted by both ports", options);
    }
    return result(restricted
                      ? TransportValidationCode::valid_with_restrictions
                      : TransportValidationCode::valid,
                  envelope.id.canonical(), {}, options);
}

}  // namespace

TransportValidationResult validate_transport_submission(
    const AcsRegistrySnapshot& registry,
    const ConnectionStateSnapshot& connection_state,
    const AcsAdmissionResult& admission,
    const TransportBindingSnapshot& binding,
    const AttachmentSnapshot& attachment,
    const SignalEnvelope& envelope,
    const TransportContractOptions& options) noexcept {
    try {
        return validate_impl(registry, connection_state, admission,
                             binding, attachment, envelope, options);
    } catch (...) {
        TransportValidationResult result{};
        result.code = TransportValidationCode::resource_exhausted;
        return result;
    }
}

const char* to_string(TransportValidationCode code) noexcept {
    switch (code) {
        case TransportValidationCode::valid: return "valid";
        case TransportValidationCode::valid_with_restrictions:
            return "valid_with_restrictions";
        case TransportValidationCode::invalid_configuration:
            return "invalid_configuration";
        case TransportValidationCode::malformed_contract: return "malformed_contract";
        case TransportValidationCode::broken_reference: return "broken_reference";
        case TransportValidationCode::binding_unknown: return "binding_unknown";
        case TransportValidationCode::binding_unavailable: return "binding_unavailable";
        case TransportValidationCode::binding_unsupported: return "binding_unsupported";
        case TransportValidationCode::binding_conflicting: return "binding_conflicting";
        case TransportValidationCode::attachment_unknown: return "attachment_unknown";
        case TransportValidationCode::attachment_inactive: return "attachment_inactive";
        case TransportValidationCode::attachment_conflicting:
            return "attachment_conflicting";
        case TransportValidationCode::lifecycle_unknown: return "lifecycle_unknown";
        case TransportValidationCode::lifecycle_unavailable:
            return "lifecycle_unavailable";
        case TransportValidationCode::lifecycle_incompatible:
            return "lifecycle_incompatible";
        case TransportValidationCode::admission_denied: return "admission_denied";
        case TransportValidationCode::admission_unknown: return "admission_unknown";
        case TransportValidationCode::admission_deferred: return "admission_deferred";
        case TransportValidationCode::admission_conflicting:
            return "admission_conflicting";
        case TransportValidationCode::admission_unavailable:
            return "admission_unavailable";
        case TransportValidationCode::admission_stale: return "admission_stale";
        case TransportValidationCode::freshness_unknown: return "freshness_unknown";
        case TransportValidationCode::freshness_stale: return "freshness_stale";
        case TransportValidationCode::freshness_conflicting:
            return "freshness_conflicting";
        case TransportValidationCode::freshness_unavailable:
            return "freshness_unavailable";
        case TransportValidationCode::provenance_invalid: return "provenance_invalid";
        case TransportValidationCode::schema_incompatible: return "schema_incompatible";
        case TransportValidationCode::direction_incompatible:
            return "direction_incompatible";
        case TransportValidationCode::signal_too_large: return "signal_too_large";
        case TransportValidationCode::resource_exhausted: return "resource_exhausted";
    }
    return "malformed_contract";
}

}  // namespace prometheus::core::acs
