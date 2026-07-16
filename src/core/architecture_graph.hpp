#ifndef SRC_CORE_ARCHITECTURE_GRAPH_HPP
#define SRC_CORE_ARCHITECTURE_GRAPH_HPP

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "backends/compute_backend.hpp"
#include "execution_policy.hpp"

namespace prometheus::core {

struct ArchitectureLimits {
    std::size_t maximum_components = 256;
    std::size_t maximum_edges = 1024;
    std::size_t maximum_dependencies_per_component = 32;
    std::size_t maximum_adapters_per_component = 32;
    std::size_t maximum_contract_references_per_component = 16;
    std::size_t maximum_backend_kinds_per_component = 8;
    std::size_t maximum_proposals_per_transaction = 64;
    std::size_t maximum_evidence_entries = 256;
    std::size_t maximum_history_entries = 128;
    std::size_t maximum_identifier_length = 128;
    std::size_t maximum_display_name_length = 256;
    std::size_t maximum_metadata_length = 1024;
};

enum class ArchitectureComponentType : std::uint8_t {
    service = 0,
    backend_adapter,
    adaptive_state,
    memory_interface,
    communication_interface,
    compute_stage,
    control_stage,
    unknown,
};

enum class ArchitectureEdgeType : std::uint8_t {
    data_flow = 0,
    control_flow,
    evidence_flow,
    logical_affinity,
    unknown,
};

struct ArchitectureDependencyRequirement {
    std::string component_id{};
    ArchitectureComponentType required_type =
        ArchitectureComponentType::unknown;
};

struct ArchitectureResourceRequirement {
    std::vector<backends::ComputeBackendKind> allowed_backend_kinds{};
    ResourceTenancy tenancy = ResourceTenancy::unknown;
    std::uint64_t required_memory_bytes = 0;
    std::uint64_t reservation_headroom_bytes = 0;
    std::size_t requested_execution_lanes = 0;
    std::vector<std::string> operation_adapters{};
};

struct ArchitectureComponentDescriptor {
    std::string component_id{};
    std::string display_name{};
    ArchitectureComponentType component_type =
        ArchitectureComponentType::unknown;
    bool enabled = true;
    std::vector<ArchitectureDependencyRequirement> dependencies{};
    double weight = 1.0;
    ArchitectureResourceRequirement resource_requirement{};
    std::vector<std::string> memory_contract_references{};
    std::vector<std::string> acs_contract_references{};
    std::uint64_t schema_version = 1;
};

struct ArchitectureEdgeDescriptor {
    std::string edge_id{};
    std::string source_component_id{};
    std::string target_component_id{};
    ArchitectureEdgeType edge_type = ArchitectureEdgeType::unknown;
    double weight = 1.0;
    bool enabled = true;
    std::uint64_t schema_version = 1;
};

struct ArchitectureGraphSnapshot {
    std::uint64_t graph_revision = 0;
    std::uint64_t schema_version = 1;
    std::vector<ArchitectureComponentDescriptor> components{};
    std::vector<ArchitectureEdgeDescriptor> edges{};
};

enum class ArchitectureRegistrationCode : std::uint8_t {
    success = 0,
    invalid_configuration,
    invalid_identifier,
    invalid_descriptor,
    duplicate_identifier,
    capacity_exhausted,
    resource_exhausted,
};

struct ArchitectureRegistrationResult {
    ArchitectureRegistrationCode code = ArchitectureRegistrationCode::success;
    std::string identity{};
    std::string message{};

    [[nodiscard]] bool ok() const noexcept {
        return code == ArchitectureRegistrationCode::success;
    }
};

enum class ArchitectureGraphValidationCode : std::uint8_t {
    valid = 0,
    invalid,
    unknown,
    resource_exhausted,
};

enum class ArchitectureGraphIssueCode : std::uint8_t {
    none = 0,
    invalid_component,
    invalid_edge,
    duplicate_component,
    duplicate_edge,
    missing_dependency,
    self_dependency,
    dependency_cycle,
    dependency_type_mismatch,
    dependency_type_unknown,
    disabled_required_dependency,
    missing_edge_endpoint,
    duplicate_semantic_edge,
    capacity_exceeded,
};

struct ArchitectureGraphIssue {
    ArchitectureGraphIssueCode code = ArchitectureGraphIssueCode::none;
    std::string subject_id{};
    std::string related_id{};
    std::string message{};
};

struct ArchitectureGraphValidationResult {
    ArchitectureGraphValidationCode code =
        ArchitectureGraphValidationCode::invalid;
    std::vector<ArchitectureGraphIssue> issues{};
    std::vector<std::string> dependency_order{};

    [[nodiscard]] bool valid() const noexcept {
        return code == ArchitectureGraphValidationCode::valid;
    }
};

class ArchitectureGraph {
public:
    explicit ArchitectureGraph(
        ArchitectureLimits limits = {},
        std::uint64_t schema_version = 1);

    ArchitectureGraph(const ArchitectureGraph&) = delete;
    ArchitectureGraph& operator=(const ArchitectureGraph&) = delete;
    ArchitectureGraph(ArchitectureGraph&&) = delete;
    ArchitectureGraph& operator=(ArchitectureGraph&&) = delete;

    [[nodiscard]] ArchitectureRegistrationResult register_component(
        ArchitectureComponentDescriptor descriptor);
    [[nodiscard]] ArchitectureRegistrationResult register_edge(
        ArchitectureEdgeDescriptor descriptor);
    [[nodiscard]] ArchitectureGraphSnapshot snapshot() const;
    [[nodiscard]] ArchitectureGraphValidationResult validate() const;
    [[nodiscard]] std::uint64_t revision() const;
    [[nodiscard]] std::size_t component_count() const;
    [[nodiscard]] std::size_t edge_count() const;
    [[nodiscard]] const ArchitectureLimits& limits() const noexcept;

private:
    ArchitectureLimits limits_{};
    bool limits_valid_ = false;
    std::uint64_t schema_version_ = 1;
    // One non-recursive mutex protects the graph. No method invokes external
    // code or holds any ServiceManager/adaptive-state registry lock.
    mutable std::mutex mutex_{};
    std::vector<ArchitectureComponentDescriptor> components_{};
    std::vector<ArchitectureEdgeDescriptor> edges_{};
    std::uint64_t graph_revision_ = 0;
};

enum class ArchitectureProposalSourceCategory : std::uint8_t {
    automatic_internal = 0,
    externally_authorized,
    operator_directed,
    recovery,
    system_policy,
    unknown,
};

enum class ArchitectureProposalOperation : std::uint8_t {
    add_component = 0,
    remove_component,
    enable_component,
    disable_component,
    replace_dependency,
    add_edge,
    remove_edge,
    adjust_weight,
    replace_resource_requirement,
    unknown,
};

struct ArchitectureNoPayload {};

struct ArchitectureAddComponentPayload {
    ArchitectureComponentDescriptor component{};
};

struct ArchitectureReplaceDependencyPayload {
    std::string old_dependency_id{};
    ArchitectureDependencyRequirement new_dependency{};
};

struct ArchitectureAddEdgePayload {
    ArchitectureEdgeDescriptor edge{};
};

enum class ArchitectureWeightTarget : std::uint8_t {
    component = 0,
    edge,
    unknown,
};

struct ArchitectureAdjustWeightPayload {
    ArchitectureWeightTarget target = ArchitectureWeightTarget::unknown;
    double weight = 0.0;
};

struct ArchitectureReplaceResourcePayload {
    ArchitectureResourceRequirement requirement{};
};

using ArchitectureProposalPayload = std::variant<
    ArchitectureNoPayload,
    ArchitectureAddComponentPayload,
    ArchitectureReplaceDependencyPayload,
    ArchitectureAddEdgePayload,
    ArchitectureAdjustWeightPayload,
    ArchitectureReplaceResourcePayload>;

struct ArchitectureProposal {
    std::string proposal_id{};
    std::string source_identity{};
    ArchitectureProposalSourceCategory source_category =
        ArchitectureProposalSourceCategory::unknown;
    std::string proposal_authority_reference{};
    std::uint64_t expected_graph_revision = 0;
    std::string target_identity{};
    ArchitectureProposalOperation operation =
        ArchitectureProposalOperation::unknown;
    ArchitectureProposalPayload payload{ArchitectureNoPayload{}};
    std::string correlation_id{};
    std::string evidence_reference{};
    std::string explanation{};
};

struct ArchitectureTransaction {
    std::string transaction_id{};
    std::string coordinator_identity{};
    std::uint64_t expected_graph_revision = 0;
    std::vector<ArchitectureProposal> proposals{};
};

enum class ArchitectureProposalShapeCode : std::uint8_t {
    valid = 0,
    invalid_identity,
    invalid_metadata,
    invalid_operation,
    payload_mismatch,
    resource_exhausted,
};

struct ArchitectureProposalShapeResult {
    ArchitectureProposalShapeCode code =
        ArchitectureProposalShapeCode::invalid_metadata;
    std::string message{};

    [[nodiscard]] bool valid() const noexcept {
        return code == ArchitectureProposalShapeCode::valid;
    }
};

[[nodiscard]] bool is_valid_architecture_identifier(
    const std::string& value,
    std::size_t maximum_length) noexcept;
[[nodiscard]] ArchitectureGraphValidationResult validate_architecture_graph(
    const ArchitectureGraphSnapshot& graph,
    const ArchitectureLimits& limits);
[[nodiscard]] ArchitectureProposalShapeResult
validate_architecture_proposal_shape(
    const ArchitectureProposal& proposal,
    const ArchitectureLimits& limits);

[[nodiscard]] const char* to_string(ArchitectureComponentType value) noexcept;
[[nodiscard]] const char* to_string(ArchitectureEdgeType value) noexcept;
[[nodiscard]] const char* to_string(ArchitectureRegistrationCode value) noexcept;
[[nodiscard]] const char* to_string(
    ArchitectureGraphValidationCode value) noexcept;
[[nodiscard]] const char* to_string(ArchitectureGraphIssueCode value) noexcept;
[[nodiscard]] const char* to_string(
    ArchitectureProposalSourceCategory value) noexcept;
[[nodiscard]] const char* to_string(
    ArchitectureProposalOperation value) noexcept;
[[nodiscard]] const char* to_string(ArchitectureProposalShapeCode value) noexcept;

}  // namespace prometheus::core

#endif  // SRC_CORE_ARCHITECTURE_GRAPH_HPP
