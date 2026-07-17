#ifndef SRC_CORE_ARCHITECTURE_SHADOW_HPP
#define SRC_CORE_ARCHITECTURE_SHADOW_HPP

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "architecture_graph.hpp"

namespace prometheus::core {

enum class ArchitectureAuthorityDecision : std::uint8_t {
    unknown = 0,
    authorized,
    denied,
};

struct ArchitectureProposalAuthority {
    ArchitectureAuthorityDecision decision =
        ArchitectureAuthorityDecision::unknown;
    std::string authority_identity{};
    std::string evidence_reference{};
};

struct ArchitectureValidationAuthority {
    ArchitectureAuthorityDecision decision =
        ArchitectureAuthorityDecision::unknown;
    std::string authority_identity{};
    std::string evidence_reference{};
};

struct ArchitectureShadowAuthority {
    ArchitectureAuthorityDecision decision =
        ArchitectureAuthorityDecision::unknown;
    std::string authority_identity{};
    std::string evidence_reference{};
};

enum class ArchitectureProposalValidationCode : std::uint8_t {
    valid = 0,
    invalid,
    not_found,
    duplicate,
    authority_denied,
    version_conflict,
    type_mismatch,
    orphaning,
    resource_exhausted,
    unknown,
};

struct ArchitectureProposalValidationResult {
    std::string proposal_id{};
    std::string target_identity{};
    ArchitectureProposalValidationCode code =
        ArchitectureProposalValidationCode::unknown;
    std::string message{};

    [[nodiscard]] bool valid() const noexcept {
        return code == ArchitectureProposalValidationCode::valid;
    }
};

enum class ArchitectureImpactOutcome : std::uint8_t {
    valid = 0,
    invalid,
    deferred,
    unsupported,
    unknown,
    policy_conflict,
    resource_unsatisfied,
    adapter_unsatisfied,
    authority_denied,
    version_conflict,
    resource_exhausted,
};

enum class ArchitectureLifecycleImpact : std::uint8_t {
    restart_not_likely = 0,
    restart_likely,
    unknown,
};

enum class ArchitectureRollbackFeasibility : std::uint8_t {
    shadow_discard_feasible = 0,
    live_rollback_infeasible,
    unknown,
};

struct ArchitectureTenancyImpact {
    std::string component_id{};
    ResourceTenancy before = ResourceTenancy::unknown;
    ResourceTenancy after = ResourceTenancy::unknown;
};

struct ArchitectureMemoryImpact {
    std::string component_id{};
    std::uint64_t before_bytes = 0;
    std::uint64_t after_bytes = 0;
};

struct ArchitectureAdapterImpact {
    std::string component_id{};
    std::string adapter_id{};
};

struct ArchitectureImpactAnalysis {
    ArchitectureImpactOutcome outcome = ArchitectureImpactOutcome::unknown;
    std::vector<std::string> affected_components{};
    std::vector<std::string> affected_dependencies{};
    std::vector<std::string> introduced_components{};
    std::vector<std::string> removed_components{};
    std::vector<std::string> introduced_edges{};
    std::vector<std::string> removed_edges{};
    std::vector<backends::ComputeBackendKind> required_backend_kinds{};
    std::vector<ArchitectureTenancyImpact> tenancy_changes{};
    std::vector<ArchitectureMemoryImpact> memory_changes{};
    std::vector<ArchitectureAdapterImpact> adapter_requirements{};
    std::vector<std::string> unresolved_references{};
    std::vector<std::string> unresolved_assumptions{};
    ArchitectureLifecycleImpact lifecycle_impact =
        ArchitectureLifecycleImpact::unknown;
    ArchitectureRollbackFeasibility rollback_feasibility =
        ArchitectureRollbackFeasibility::unknown;
    std::vector<std::string> adaptive_state_impacts{};
};

struct ArchitectureEvaluationEvidence {
    std::vector<ExecutionPolicyResource> resources{};
};

struct ArchitectureShadowResult {
    std::string transaction_id{};
    ArchitectureImpactOutcome outcome = ArchitectureImpactOutcome::unknown;
    std::string message{};
    std::uint64_t live_graph_revision = 0;
    std::uint64_t shadow_graph_revision = 0;
    bool shadow_created = false;
    bool live_state_modified = false;
    std::vector<ArchitectureProposalValidationResult> proposal_results{};
    ArchitectureGraphValidationResult structural_validation{};
    ArchitectureImpactAnalysis impact{};
    std::optional<ArchitectureGraphSnapshot> shadow_graph{};

    [[nodiscard]] bool structurally_valid() const noexcept {
        return shadow_created && structural_validation.valid();
    }
};

class ArchitectureShadowEvaluator {
public:
    explicit ArchitectureShadowEvaluator(ArchitectureLimits limits = {});

    ArchitectureShadowEvaluator(const ArchitectureShadowEvaluator&) = delete;
    ArchitectureShadowEvaluator& operator=(
        const ArchitectureShadowEvaluator&) = delete;
    ArchitectureShadowEvaluator(ArchitectureShadowEvaluator&&) = delete;
    ArchitectureShadowEvaluator& operator=(ArchitectureShadowEvaluator&&) =
        delete;

    [[nodiscard]] ArchitectureShadowResult evaluate(
        const ArchitectureGraphSnapshot& live_graph,
        const ArchitectureTransaction& transaction,
        const ArchitectureProposalAuthority& proposal_authority,
        const ArchitectureValidationAuthority& validation_authority,
        const ArchitectureShadowAuthority& shadow_authority,
        const ArchitectureEvaluationEvidence& evidence = {});

private:
    ArchitectureLimits limits_{};
    // This mutex protects only bounded duplicate-evidence history. Evaluation
    // invokes no callbacks and never locks ServiceManager or adaptive state.
    mutable std::mutex mutex_{};
    std::vector<std::string> processed_transaction_ids_{};
    std::vector<std::string> processed_proposal_ids_{};
};

[[nodiscard]] const char* to_string(
    ArchitectureAuthorityDecision value) noexcept;
[[nodiscard]] const char* to_string(
    ArchitectureProposalValidationCode value) noexcept;
[[nodiscard]] const char* to_string(ArchitectureImpactOutcome value) noexcept;
[[nodiscard]] const char* to_string(ArchitectureLifecycleImpact value) noexcept;
[[nodiscard]] const char* to_string(
    ArchitectureRollbackFeasibility value) noexcept;

}  // namespace prometheus::core

#endif  // SRC_CORE_ARCHITECTURE_SHADOW_HPP
