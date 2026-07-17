#ifndef SRC_CORE_ACS_ACS_REGISTRY_HPP
#define SRC_CORE_ACS_ACS_REGISTRY_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

#include "acs_types.hpp"

namespace prometheus::core::acs {

inline constexpr std::size_t kAbsoluteParticipants = 4096;
inline constexpr std::size_t kAbsoluteAuthorities = 8192;
inline constexpr std::size_t kAbsoluteCapabilities = 8192;
inline constexpr std::size_t kAbsoluteEndpoints = 8192;
inline constexpr std::size_t kAbsoluteRelationships = 16384;
inline constexpr std::size_t kAbsoluteConnections = 16384;

struct RegistryOptions {
    std::size_t maximum_participants = 256;
    std::size_t maximum_authorities = 512;
    std::size_t maximum_capabilities = 512;
    std::size_t maximum_delegations = 512;
    std::size_t maximum_revocations = 512;
    std::size_t maximum_evidence = 512;
    std::size_t maximum_endpoints = 512;
    std::size_t maximum_ports_per_endpoint = 32;
    std::size_t maximum_relationships = 1024;
    std::size_t maximum_connections = 1024;
    std::size_t maximum_identifier_bytes = kDefaultIdentifierBytes;
    std::size_t maximum_evidence_references = kDefaultEvidenceReferences;
    std::size_t maximum_budget_dimensions = kDefaultBudgetDimensions;
};

enum class PortDirection : std::uint8_t { ingress = 0, egress, bidirectional, unknown };
enum class RelationshipClass : std::uint8_t {
    synaptic = 0, bridge, infrastructure, immune, unknown,
};
enum class FailureBehavior : std::uint8_t {
    reject = 0, defer, degrade, unavailable, unknown,
};
enum class BudgetUnit : std::uint8_t {
    bytes = 0, messages, operations, nanoseconds, abstract_units, unknown,
};

struct BudgetDeclaration {
    ScopeReference dimension{};
    BudgetUnit unit = BudgetUnit::unknown;
    std::uint64_t hard_limit = 0;
};

struct ParticipantDescriptor {
    ParticipantId id{};
    DescriptorRevision revision{1};
    IdentityCondition identity_condition = IdentityCondition::unknown;
    bool enabled = true;
};
struct AuthorityDescriptor {
    AuthorityId id{};
    ParticipantId owner{};
    ScopeReference scope{};
    AuthorityCondition condition = AuthorityCondition::unknown;
    DescriptorRevision revision{1};
};
struct CapabilityDescriptor {
    CapabilityId id{};
    AuthorityId issuer{};
    ParticipantId subject{};
    ScopeReference scope{};
    AuthorityCondition condition = AuthorityCondition::unknown;
    DescriptorRevision revision{1};
};
struct DelegationReference {
    DelegationId id{};
    AuthorityId issuer{};
    CapabilityId capability{};
    ParticipantId delegate{};
    ScopeReference scope{};
    AuthorityCondition condition = AuthorityCondition::unknown;
    DescriptorRevision revision{1};
};
struct RevocationReference {
    RevocationId id{};
    AuthorityId issuer{};
    CapabilityId capability{};
    ParticipantId subject{};
    ScopeReference scope{};
    AuthorityCondition condition = AuthorityCondition::unknown;
    DescriptorRevision revision{1};
};
struct EndpointDescriptor {
    EndpointId id{};
    ParticipantId owner{};
    DescriptorRevision revision{1};
    bool enabled = true;
};
struct PortDescriptor {
    PortId id{};
    EndpointId endpoint{};
    ScopeReference purpose{};
    PortDirection direction = PortDirection::unknown;
    std::vector<ScopeReference> accepted_schemas{};
    std::optional<CapabilityId> required_capability{};
    std::vector<BudgetDeclaration> budgets{};
    FailureBehavior failure_behavior = FailureBehavior::unknown;
    DescriptorRevision revision{1};
    bool enabled = true;
};
struct RelationshipDescriptor {
    RelationshipId id{};
    ParticipantId first{};
    ParticipantId second{};
    RelationshipClass relationship_class = RelationshipClass::unknown;
    DescriptorRevision revision{1};
    bool enabled = true;
};
struct ConnectionDescriptor {
    ConnectionId id{};
    RelationshipId relationship{};
    EndpointId source_endpoint{};
    PortId source_port{};
    EndpointId target_endpoint{};
    PortId target_port{};
    std::vector<CapabilityId> required_capabilities{};
    std::vector<EvidenceId> required_evidence{};
    std::optional<RestrictionId> restriction{};
    DescriptorRevision revision{1};
};

enum class RegistryCode : std::uint8_t {
    success = 0,
    invalid_configuration,
    invalid_descriptor,
    duplicate_identity,
    missing_reference,
    capacity_exhausted,
    generation_overflow,
    resource_exhausted,
};
struct RegistryResult {
    RegistryCode code = RegistryCode::success;
    std::string identity{};
    std::string message{};
    RegistryGeneration generation{};
    [[nodiscard]] bool ok() const noexcept { return code == RegistryCode::success; }
};

struct AcsRegistrySnapshot {
    RegistryGeneration generation{};
    std::vector<ParticipantDescriptor> participants{};
    std::vector<AuthorityDescriptor> authorities{};
    std::vector<CapabilityDescriptor> capabilities{};
    std::vector<DelegationReference> delegations{};
    std::vector<RevocationReference> revocations{};
    std::vector<EvidenceReference> evidence{};
    std::vector<RestrictionReference> restrictions{};
    std::vector<ContainmentAuthorizationReference> containment_authorizations{};
    std::vector<RestorationClearanceReference> restoration_clearances{};
    std::vector<EndpointDescriptor> endpoints{};
    std::vector<PortDescriptor> ports{};
    std::vector<RelationshipDescriptor> relationships{};
    std::vector<ConnectionDescriptor> connections{};
};

class AcsRegistry {
public:
    explicit AcsRegistry(RegistryOptions options = {});
    AcsRegistry(const AcsRegistry&) = delete;
    AcsRegistry& operator=(const AcsRegistry&) = delete;

    [[nodiscard]] RegistryResult register_participant(ParticipantDescriptor value) noexcept;
    [[nodiscard]] RegistryResult register_authority(AuthorityDescriptor value) noexcept;
    [[nodiscard]] RegistryResult register_capability(CapabilityDescriptor value) noexcept;
    [[nodiscard]] RegistryResult register_delegation(DelegationReference value) noexcept;
    [[nodiscard]] RegistryResult register_revocation(RevocationReference value) noexcept;
    [[nodiscard]] RegistryResult register_evidence(EvidenceReference value) noexcept;
    [[nodiscard]] RegistryResult register_restriction(RestrictionReference value) noexcept;
    [[nodiscard]] RegistryResult register_containment_authorization(ContainmentAuthorizationReference value) noexcept;
    [[nodiscard]] RegistryResult register_restoration_clearance(RestorationClearanceReference value) noexcept;
    [[nodiscard]] RegistryResult register_endpoint(EndpointDescriptor value) noexcept;
    [[nodiscard]] RegistryResult register_port(PortDescriptor value) noexcept;
    [[nodiscard]] RegistryResult register_relationship(RelationshipDescriptor value) noexcept;
    [[nodiscard]] RegistryResult register_connection(ConnectionDescriptor value) noexcept;

    [[nodiscard]] AcsRegistrySnapshot snapshot() const;
    [[nodiscard]] RegistryGeneration generation() const;
    [[nodiscard]] const RegistryOptions& options() const noexcept { return options_; }
    [[nodiscard]] bool contains_connection(const ConnectionId& id) const;

private:
    RegistryOptions options_{};
    bool options_valid_ = false;
    mutable std::shared_mutex mutex_{};
    RegistryGeneration generation_{};
    AcsRegistrySnapshot data_{};
};

}  // namespace prometheus::core::acs
#endif
