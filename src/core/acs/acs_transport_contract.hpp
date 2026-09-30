#ifndef SRC_CORE_ACS_ACS_TRANSPORT_CONTRACT_HPP
#define SRC_CORE_ACS_ACS_TRANSPORT_CONTRACT_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "acs_admission.hpp"

namespace prometheus::core::acs {

inline constexpr std::size_t kDefaultInlineSignalBytes = 4096;
inline constexpr std::size_t kAbsoluteInlineSignalBytes = 65536;

enum class BindingCondition : std::uint8_t {
    unknown = 0,
    proposed,
    establishing,
    active,
    degraded,
    blocked,
    migrating,
    draining,
    closed,
    failed,
    stale,
    conflicting,
};

enum class TransportProfileSupport : std::uint8_t {
    unknown = 0,
    supported,
    unsupported,
    unavailable,
};

enum class AttachmentCondition : std::uint8_t {
    unknown = 0,
    proposed,
    admitted,
    active,
    restricted,
    suspended,
    draining,
    revoked,
    expired,
    closed,
    conflicting,
};

enum class SignalDomain : std::uint8_t {
    unknown = 0,
    cognitive,
    operational,
    immune,
    security,
};

enum class SignalIntent : std::uint8_t {
    unknown = 0,
    observation,
    influence,
    request,
    directive,
    evidence,
    response,
};

struct TransportBindingSnapshot {
    BindingId id{};
    ConnectionId connection_id{};
    EndpointId source_endpoint{};
    EndpointId target_endpoint{};
    ScopeReference transport_profile{};
    ScopeReference signal_schema{};
    std::uint32_t signal_schema_version = 0;
    TransportProfileSupport profile_support = TransportProfileSupport::unknown;
    BindingCondition condition = BindingCondition::unknown;
    DescriptorRevision revision{};
};

struct AttachmentSnapshot {
    AttachmentId id{};
    ConnectionId connection_id{};
    PortId port_id{};
    ParticipantId participant{};
    AuthorityId authority{};
    ScopeReference scope{};
    AttachmentCondition condition = AttachmentCondition::unknown;
    DescriptorRevision revision{};
};

struct SignalEnvelope {
    SignalId id{};
    std::optional<CorrelationId> correlation_id{};
    ConnectionId connection_id{};
    BindingId binding_id{};
    AttachmentId attachment_id{};
    ParticipantId source{};
    ParticipantId target{};
    EndpointId source_endpoint{};
    EndpointId target_endpoint{};
    PortId source_port{};
    PortId target_port{};
    SignalDomain domain = SignalDomain::unknown;
    SignalIntent intent = SignalIntent::unknown;
    ScopeReference subtype{};
    ScopeReference schema{};
    std::uint32_t schema_version = 0;
    FreshnessCondition freshness = FreshnessCondition::unknown;
    std::vector<EvidenceId> provenance{};
    std::vector<std::uint8_t> inline_value{};
};

struct TransportContractOptions {
    std::size_t maximum_inline_signal_bytes = kDefaultInlineSignalBytes;
    std::size_t maximum_provenance_references = kDefaultEvidenceReferences;
    std::size_t maximum_diagnostic_bytes = kDefaultDiagnosticBytes;
};

enum class TransportValidationCode : std::uint8_t {
    valid = 0,
    valid_with_restrictions,
    invalid_configuration,
    malformed_contract,
    broken_reference,
    binding_unknown,
    binding_unavailable,
    binding_unsupported,
    binding_conflicting,
    attachment_unknown,
    attachment_inactive,
    attachment_conflicting,
    lifecycle_unknown,
    lifecycle_unavailable,
    lifecycle_incompatible,
    admission_denied,
    admission_unknown,
    admission_deferred,
    admission_conflicting,
    admission_unavailable,
    admission_stale,
    freshness_unknown,
    freshness_stale,
    freshness_conflicting,
    freshness_unavailable,
    provenance_invalid,
    schema_incompatible,
    direction_incompatible,
    signal_too_large,
    resource_exhausted,
};

struct TransportValidationResult {
    TransportValidationCode code = TransportValidationCode::valid;
    std::string subject{};
    std::string message{};
    bool diagnostic_truncated = false;
    [[nodiscard]] bool valid() const noexcept {
        return code == TransportValidationCode::valid ||
               code == TransportValidationCode::valid_with_restrictions;
    }
};

// Pure structural evaluation. It performs no admission decision, reservation,
// lifecycle transition, serialization, transport I/O, or registry mutation.
[[nodiscard]] TransportValidationResult validate_transport_submission(
    const AcsRegistrySnapshot& registry,
    const ConnectionStateSnapshot& connection_state,
    const AcsAdmissionResult& admission,
    const TransportBindingSnapshot& binding,
    const AttachmentSnapshot& attachment,
    const SignalEnvelope& envelope,
    const TransportContractOptions& options = {}) noexcept;

}  // namespace prometheus::core::acs

#endif
