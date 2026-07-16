#ifndef SRC_CORE_ADAPTIVE_STATE_HPP
#define SRC_CORE_ADAPTIVE_STATE_HPP

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace prometheus::core {

enum class AdaptiveValueType : std::uint8_t {
    boolean = 0,
    signed_integer,
    unsigned_integer,
    floating_point,
    symbolic,
    signed_integer_vector,
    unsigned_integer_vector,
    floating_point_vector,
};

using AdaptiveValue = std::variant<
    bool,
    std::int64_t,
    std::uint64_t,
    double,
    std::string,
    std::vector<std::int64_t>,
    std::vector<std::uint64_t>,
    std::vector<double>>;

using AdaptiveNumericBound =
    std::variant<std::int64_t, std::uint64_t, double>;

struct AdaptiveNumericBounds {
    bool enabled = false;
    AdaptiveNumericBound minimum{std::int64_t{0}};
    AdaptiveNumericBound maximum{std::int64_t{0}};
};

enum class AdaptivePersistenceClass : std::uint8_t {
    ephemeral = 0,
    checkpoint_eligible,
    memory_proposal_eligible,
    unknown,
};

enum class AdaptiveSensitivity : std::uint8_t {
    public_value = 0,
    internal,
    restricted,
    secret,
    unknown,
};

enum class AdaptiveProvenanceRequirement : std::uint8_t {
    source_identity = 0,
    source_and_correlation,
    validation_evidence,
};

enum class AdaptiveVariableCondition : std::uint8_t {
    unknown = 0,
    ready,
    disabled,
    degraded,
    invalid,
    unavailable,
};

enum class AdaptiveUpdateCategory : std::uint8_t {
    unknown = 0,
    initial_registration,
    mutation,
    rollback,
};

struct AdaptiveVariableDescriptor {
    std::string variable_id{};
    std::string display_name{};
    std::string description{};
    AdaptiveValueType value_type = AdaptiveValueType::boolean;

    // Scalars use an empty shape. Vectors require an explicit shape whose
    // product equals the value's fixed element count.
    std::vector<std::size_t> shape{};
    AdaptiveValue default_value{false};
    AdaptiveNumericBounds hard_bounds{};
    AdaptiveNumericBounds soft_bounds{};

    bool allow_automatic_mutation = false;
    bool allow_external_mutation = false;
    std::optional<double> maximum_mutation_magnitude{};
    std::uint64_t minimum_revision_interval = 0;

    AdaptivePersistenceClass persistence =
        AdaptivePersistenceClass::ephemeral;
    AdaptiveSensitivity sensitivity = AdaptiveSensitivity::internal;
    AdaptiveProvenanceRequirement provenance_requirement =
        AdaptiveProvenanceRequirement::source_identity;
    bool rollback_eligible = true;
    std::uint64_t descriptor_version = 1;
    std::uint64_t schema_version = 1;
    bool enabled = true;
};

struct AdaptiveStateLimits {
    std::size_t maximum_variables = 1024;
    std::size_t maximum_identifier_length = 128;
    std::size_t maximum_display_name_length = 256;
    std::size_t maximum_description_length = 1024;
    std::size_t maximum_dimensions = 4;
    std::size_t maximum_vector_elements = 4096;
    std::size_t maximum_symbolic_value_length = 1024;
    std::size_t maximum_proposals_per_transaction = 64;
    std::size_t maximum_checkpoints = 16;
    std::size_t maximum_checkpoint_bytes = 16U * 1024U * 1024U;
    std::size_t maximum_history_entries = 128;
    std::size_t maximum_message_length = 512;
    std::size_t maximum_metadata_length = 512;
};

enum class AdaptiveSnapshotMode : std::uint8_t {
    metadata_only = 0,
    include_non_sensitive_values,
    include_all_values,
};

struct AdaptiveVariableSnapshot {
    AdaptiveVariableDescriptor descriptor{};
    std::optional<AdaptiveValue> value{};
    bool value_redacted = true;
    std::uint64_t version = 0;
    std::uint64_t descriptor_version = 0;
    std::string last_mutation_id{};
    std::string last_source_identity{};
    AdaptiveUpdateCategory last_update_category =
        AdaptiveUpdateCategory::unknown;
    std::string last_validation_evidence{};
    AdaptiveVariableCondition condition = AdaptiveVariableCondition::unknown;
    std::uint64_t last_changed_registry_revision = 0;
};

struct AdaptiveRegistrySnapshot {
    std::uint64_t registry_revision = 0;
    std::vector<AdaptiveVariableSnapshot> variables{};
};

enum class AdaptiveRegistryCondition : std::uint8_t {
    ready = 0,
    invalid_configuration,
    degraded,
    unavailable,
};

struct AdaptiveRegistryStatus {
    AdaptiveRegistryCondition condition =
        AdaptiveRegistryCondition::invalid_configuration;
    AdaptiveStateLimits limits{};
    std::size_t registered_variables = 0;
    std::uint64_t registry_revision = 0;
    std::string message{};

    [[nodiscard]] bool usable() const noexcept {
        return condition == AdaptiveRegistryCondition::ready ||
               condition == AdaptiveRegistryCondition::degraded;
    }
};

enum class AdaptiveRegistrationCode : std::uint8_t {
    success = 0,
    invalid_configuration,
    invalid_identifier,
    invalid_descriptor,
    duplicate_identifier,
    capacity_exhausted,
    resource_exhausted,
};

struct AdaptiveRegistrationResult {
    AdaptiveRegistrationCode code = AdaptiveRegistrationCode::success;
    std::string variable_id{};
    std::string message{};

    [[nodiscard]] bool ok() const noexcept {
        return code == AdaptiveRegistrationCode::success;
    }
};

enum class AdaptiveMutationSourceCategory : std::uint8_t {
    automatic_internal = 0,
    externally_authorized,
    operator_directed,
    recovery,
    system_policy,
    unknown,
};

enum class AdaptiveMutationOperation : std::uint8_t {
    replace = 0,
    add,
    scale,
    bounded_adjust,
    unknown,
};

enum class AdaptiveAuthorityDecision : std::uint8_t {
    unknown = 0,
    authorized,
    denied,
};

struct AdaptiveProposalAuthority {
    AdaptiveAuthorityDecision decision = AdaptiveAuthorityDecision::unknown;
    std::string authority_identity{};
    std::string evidence_reference{};
};

struct AdaptiveValidationAuthority {
    AdaptiveAuthorityDecision decision = AdaptiveAuthorityDecision::unknown;
    std::string authority_identity{};
    std::string evidence_reference{};
};

struct AdaptiveMutationProposal {
    std::string proposal_id{};
    std::string target_variable_id{};
    std::string source_identity{};
    AdaptiveMutationSourceCategory source_category =
        AdaptiveMutationSourceCategory::unknown;
    AdaptiveProposalAuthority proposal_authority{};
    std::string correlation_id{};
    std::string validation_evidence_reference{};
    std::uint64_t expected_variable_version = 0;
    AdaptiveMutationOperation operation = AdaptiveMutationOperation::unknown;
    AdaptiveValue operand{false};
    bool require_rollback_eligibility = false;
};

enum class AdaptiveMutationValidationCode : std::uint8_t {
    accepted = 0,
    rejected,
    deferred,
    not_found,
    version_conflict,
    type_mismatch,
    shape_mismatch,
    out_of_bounds,
    soft_bound_violation,
    magnitude_exceeded,
    arithmetic_overflow,
    authority_denied,
    disabled,
    rate_limited,
    resource_exhausted,
    duplicate,
    invalid_proposal,
    incompatible_rollback,
    unknown,
};

struct AdaptiveMutationValidationResult {
    std::string proposal_id{};
    std::string target_variable_id{};
    AdaptiveMutationValidationCode code =
        AdaptiveMutationValidationCode::unknown;
    std::string message{};
    std::uint64_t expected_variable_version = 0;
    std::uint64_t current_variable_version = 0;
    std::uint64_t resulting_variable_version = 0;
    std::string validation_authority_identity{};
    std::string evidence_reference{};

    [[nodiscard]] bool accepted() const noexcept {
        return code == AdaptiveMutationValidationCode::accepted;
    }
};

// Runtime-local owner of live adaptive values. Descriptors are copied during
// registration and never exposed mutably. This initial registry provides no
// direct value mutation API; all later changes pass through transactions.
class AdaptiveStateRegistry {
public:
    explicit AdaptiveStateRegistry(AdaptiveStateLimits limits = {});

    AdaptiveStateRegistry(const AdaptiveStateRegistry&) = delete;
    AdaptiveStateRegistry& operator=(const AdaptiveStateRegistry&) = delete;
    AdaptiveStateRegistry(AdaptiveStateRegistry&&) = delete;
    AdaptiveStateRegistry& operator=(AdaptiveStateRegistry&&) = delete;

    [[nodiscard]] AdaptiveRegistrationResult register_variable(
        AdaptiveVariableDescriptor descriptor);

    // Validation is pure metadata/value processing. It never commits a value
    // and never invokes caller-provided code.
    [[nodiscard]] AdaptiveMutationValidationResult validate_mutation(
        const AdaptiveMutationProposal& proposal,
        const AdaptiveValidationAuthority& authority) const;

    [[nodiscard]] std::optional<AdaptiveVariableSnapshot> find(
        const std::string& variable_id,
        AdaptiveSnapshotMode mode =
            AdaptiveSnapshotMode::include_non_sensitive_values) const;
    [[nodiscard]] AdaptiveRegistrySnapshot snapshot(
        AdaptiveSnapshotMode mode =
            AdaptiveSnapshotMode::include_non_sensitive_values) const;
    [[nodiscard]] AdaptiveRegistryStatus status() const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] std::uint64_t revision() const;

private:
    struct PreparedMutation {
        AdaptiveMutationValidationResult validation{};
        AdaptiveValue resulting_value{false};
    };

    struct VariableRecord {
        AdaptiveVariableDescriptor descriptor{};
        AdaptiveValue value{false};
        std::uint64_t version = 1;
        std::string last_mutation_id{};
        std::string last_source_identity{};
        AdaptiveUpdateCategory last_update_category =
            AdaptiveUpdateCategory::initial_registration;
        std::string last_validation_evidence{};
        AdaptiveVariableCondition condition = AdaptiveVariableCondition::ready;
        std::uint64_t last_changed_registry_revision = 0;
    };

    [[nodiscard]] AdaptiveVariableSnapshot make_snapshot_locked(
        const VariableRecord& record,
        AdaptiveSnapshotMode mode) const;
    [[nodiscard]] PreparedMutation validate_mutation_locked(
        const AdaptiveMutationProposal& proposal,
        const AdaptiveValidationAuthority& authority) const;

    AdaptiveStateLimits limits_{};
    bool limits_valid_ = false;
    mutable std::mutex mutex_{};
    std::vector<VariableRecord> variables_{};
    std::vector<std::string> processed_proposal_ids_{};
    std::uint64_t registry_revision_ = 0;
};

[[nodiscard]] bool is_valid_adaptive_identifier(
    const std::string& value,
    std::size_t maximum_length) noexcept;
[[nodiscard]] AdaptiveValueType adaptive_value_type(
    const AdaptiveValue& value) noexcept;
[[nodiscard]] const char* to_string(AdaptiveValueType value) noexcept;
[[nodiscard]] const char* to_string(AdaptivePersistenceClass value) noexcept;
[[nodiscard]] const char* to_string(AdaptiveSensitivity value) noexcept;
[[nodiscard]] const char* to_string(AdaptiveVariableCondition value) noexcept;
[[nodiscard]] const char* to_string(AdaptiveUpdateCategory value) noexcept;
[[nodiscard]] const char* to_string(AdaptiveRegistryCondition value) noexcept;
[[nodiscard]] const char* to_string(AdaptiveRegistrationCode value) noexcept;
[[nodiscard]] const char* to_string(
    AdaptiveMutationSourceCategory value) noexcept;
[[nodiscard]] const char* to_string(AdaptiveMutationOperation value) noexcept;
[[nodiscard]] const char* to_string(AdaptiveAuthorityDecision value) noexcept;
[[nodiscard]] const char* to_string(
    AdaptiveMutationValidationCode value) noexcept;

}  // namespace prometheus::core

#endif  // SRC_CORE_ADAPTIVE_STATE_HPP
