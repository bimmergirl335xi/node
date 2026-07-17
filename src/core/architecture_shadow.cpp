#include "architecture_shadow.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <utility>

namespace prometheus::core {
namespace {

template <typename Value>
void append_bounded(
    std::vector<Value>& values,
    Value value,
    std::size_t maximum_entries) {
    if (values.size() >= maximum_entries) {
        values.erase(values.begin());
    }
    values.push_back(std::move(value));
}

void append_unique(
    std::vector<std::string>& values,
    const std::string& value,
    const ArchitectureLimits& limits,
    bool& exhausted) {
    if (std::find(values.begin(), values.end(), value) != values.end()) {
        return;
    }
    if (values.size() >= limits.maximum_evidence_entries) {
        exhausted = true;
        return;
    }
    values.push_back(value);
}

template <typename Value>
[[nodiscard]] Value* find_by_id(
    std::vector<Value>& values,
    const std::string& identity);

template <>
ArchitectureComponentDescriptor* find_by_id(
    std::vector<ArchitectureComponentDescriptor>& values,
    const std::string& identity) {
    const auto found = std::find_if(
        values.begin(), values.end(), [&identity](const auto& value) {
            return value.component_id == identity;
        });
    return found == values.end() ? nullptr : &*found;
}

template <>
ArchitectureEdgeDescriptor* find_by_id(
    std::vector<ArchitectureEdgeDescriptor>& values,
    const std::string& identity) {
    const auto found = std::find_if(
        values.begin(), values.end(), [&identity](const auto& value) {
            return value.edge_id == identity;
        });
    return found == values.end() ? nullptr : &*found;
}

template <typename Value>
[[nodiscard]] const Value* find_by_id(
    const std::vector<Value>& values,
    const std::string& identity);

template <>
const ArchitectureComponentDescriptor* find_by_id(
    const std::vector<ArchitectureComponentDescriptor>& values,
    const std::string& identity) {
    const auto found = std::find_if(
        values.begin(), values.end(), [&identity](const auto& value) {
            return value.component_id == identity;
        });
    return found == values.end() ? nullptr : &*found;
}

template <>
const ArchitectureEdgeDescriptor* find_by_id(
    const std::vector<ArchitectureEdgeDescriptor>& values,
    const std::string& identity) {
    const auto found = std::find_if(
        values.begin(), values.end(), [&identity](const auto& value) {
            return value.edge_id == identity;
        });
    return found == values.end() ? nullptr : &*found;
}

[[nodiscard]] bool resource_equal(
    const ArchitectureResourceRequirement& left,
    const ArchitectureResourceRequirement& right) noexcept {
    return left.allowed_backend_kinds == right.allowed_backend_kinds &&
           left.tenancy == right.tenancy &&
           left.required_memory_bytes == right.required_memory_bytes &&
           left.reservation_headroom_bytes ==
               right.reservation_headroom_bytes &&
           left.requested_execution_lanes == right.requested_execution_lanes &&
           left.operation_adapters == right.operation_adapters;
}

[[nodiscard]] bool dependencies_equal(
    const std::vector<ArchitectureDependencyRequirement>& left,
    const std::vector<ArchitectureDependencyRequirement>& right) noexcept {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (left[index].component_id != right[index].component_id ||
            left[index].required_type != right[index].required_type) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool has_resource_requirement(
    const ArchitectureResourceRequirement& requirement) noexcept {
    return !requirement.allowed_backend_kinds.empty() ||
           requirement.tenancy != ResourceTenancy::unknown ||
           requirement.required_memory_bytes != 0 ||
           requirement.reservation_headroom_bytes != 0 ||
           requirement.requested_execution_lanes != 0 ||
           !requirement.operation_adapters.empty();
}

[[nodiscard]] ArchitectureProposalValidationResult validation_result(
    const ArchitectureProposal& proposal,
    ArchitectureProposalValidationCode code,
    std::string message) {
    return {proposal.proposal_id,
            proposal.target_identity,
            code,
            std::move(message)};
}

[[nodiscard]] ArchitectureProposalValidationResult apply_proposal(
    ArchitectureGraphSnapshot& shadow,
    const ArchitectureProposal& proposal) {
    switch (proposal.operation) {
        case ArchitectureProposalOperation::add_component: {
            if (find_by_id(shadow.components, proposal.target_identity) !=
                nullptr) {
                return validation_result(
                    proposal,
                    ArchitectureProposalValidationCode::duplicate,
                    "Component identity already exists");
            }
            shadow.components.push_back(
                std::get<ArchitectureAddComponentPayload>(proposal.payload)
                    .component);
            break;
        }
        case ArchitectureProposalOperation::remove_component: {
            const auto found = std::find_if(
                shadow.components.begin(),
                shadow.components.end(),
                [&proposal](const auto& component) {
                    return component.component_id == proposal.target_identity;
                });
            if (found == shadow.components.end()) {
                return validation_result(
                    proposal,
                    ArchitectureProposalValidationCode::not_found,
                    "Component identity is absent");
            }
            shadow.components.erase(found);
            break;
        }
        case ArchitectureProposalOperation::enable_component:
        case ArchitectureProposalOperation::disable_component: {
            ArchitectureComponentDescriptor* component =
                find_by_id(shadow.components, proposal.target_identity);
            if (component == nullptr) {
                return validation_result(
                    proposal,
                    ArchitectureProposalValidationCode::not_found,
                    "Component identity is absent");
            }
            component->enabled = proposal.operation ==
                                 ArchitectureProposalOperation::enable_component;
            break;
        }
        case ArchitectureProposalOperation::replace_dependency: {
            ArchitectureComponentDescriptor* component =
                find_by_id(shadow.components, proposal.target_identity);
            if (component == nullptr) {
                return validation_result(
                    proposal,
                    ArchitectureProposalValidationCode::not_found,
                    "Dependent component is absent");
            }
            const auto& payload =
                std::get<ArchitectureReplaceDependencyPayload>(
                    proposal.payload);
            const auto dependency = std::find_if(
                component->dependencies.begin(),
                component->dependencies.end(),
                [&payload](const auto& candidate) {
                    return candidate.component_id ==
                           payload.old_dependency_id;
                });
            if (dependency == component->dependencies.end()) {
                return validation_result(
                    proposal,
                    ArchitectureProposalValidationCode::not_found,
                    "Dependency selected for replacement is absent");
            }
            *dependency = payload.new_dependency;
            break;
        }
        case ArchitectureProposalOperation::add_edge: {
            if (find_by_id(shadow.edges, proposal.target_identity) != nullptr) {
                return validation_result(
                    proposal,
                    ArchitectureProposalValidationCode::duplicate,
                    "Edge identity already exists");
            }
            shadow.edges.push_back(
                std::get<ArchitectureAddEdgePayload>(proposal.payload).edge);
            break;
        }
        case ArchitectureProposalOperation::remove_edge: {
            const auto found = std::find_if(
                shadow.edges.begin(),
                shadow.edges.end(),
                [&proposal](const auto& edge) {
                    return edge.edge_id == proposal.target_identity;
                });
            if (found == shadow.edges.end()) {
                return validation_result(
                    proposal,
                    ArchitectureProposalValidationCode::not_found,
                    "Edge identity is absent");
            }
            shadow.edges.erase(found);
            break;
        }
        case ArchitectureProposalOperation::adjust_weight: {
            const auto& payload =
                std::get<ArchitectureAdjustWeightPayload>(proposal.payload);
            if (payload.target == ArchitectureWeightTarget::component) {
                ArchitectureComponentDescriptor* component =
                    find_by_id(shadow.components, proposal.target_identity);
                if (component == nullptr) {
                    return validation_result(
                        proposal,
                        ArchitectureProposalValidationCode::not_found,
                        "Weighted component is absent");
                }
                component->weight = payload.weight;
            } else {
                ArchitectureEdgeDescriptor* edge =
                    find_by_id(shadow.edges, proposal.target_identity);
                if (edge == nullptr) {
                    return validation_result(
                        proposal,
                        ArchitectureProposalValidationCode::not_found,
                        "Weighted edge is absent");
                }
                edge->weight = payload.weight;
            }
            break;
        }
        case ArchitectureProposalOperation::replace_resource_requirement: {
            ArchitectureComponentDescriptor* component =
                find_by_id(shadow.components, proposal.target_identity);
            if (component == nullptr) {
                return validation_result(
                    proposal,
                    ArchitectureProposalValidationCode::not_found,
                    "Resource-bearing component is absent");
            }
            component->resource_requirement =
                std::get<ArchitectureReplaceResourcePayload>(proposal.payload)
                    .requirement;
            break;
        }
        case ArchitectureProposalOperation::unknown:
            return validation_result(
                proposal,
                ArchitectureProposalValidationCode::invalid,
                "Architecture operation is unknown");
    }
    return validation_result(
        proposal,
        ArchitectureProposalValidationCode::valid,
        "Proposal applied to isolated shadow graph");
}

[[nodiscard]] ArchitectureImpactOutcome outcome_for_admission(
    const ExecutionAdmissionResult& admission) noexcept {
    for (const AdmissionIssue& issue : admission.issues) {
        if (issue.code == AdmissionIssueCode::operation_adapter_unavailable) {
            return ArchitectureImpactOutcome::adapter_unsatisfied;
        }
    }
    switch (admission.outcome) {
        case AdmissionOutcome::accepted:
            return ArchitectureImpactOutcome::valid;
        case AdmissionOutcome::deferred:
            return ArchitectureImpactOutcome::deferred;
        case AdmissionOutcome::rejected:
        case AdmissionOutcome::policy_conflict:
            return ArchitectureImpactOutcome::policy_conflict;
        case AdmissionOutcome::unsupported:
        case AdmissionOutcome::unavailable:
            return ArchitectureImpactOutcome::unsupported;
        case AdmissionOutcome::resource_exhausted:
            return ArchitectureImpactOutcome::resource_unsatisfied;
        case AdmissionOutcome::unknown:
            return ArchitectureImpactOutcome::unknown;
    }
    return ArchitectureImpactOutcome::unknown;
}

[[nodiscard]] ArchitectureImpactOutcome aggregate_unsatisfied_outcomes(
    const std::vector<ArchitectureImpactOutcome>& outcomes) noexcept {
    const auto contains = [&outcomes](ArchitectureImpactOutcome value) {
        return std::find(outcomes.begin(), outcomes.end(), value) !=
               outcomes.end();
    };
    if (contains(ArchitectureImpactOutcome::unknown)) {
        return ArchitectureImpactOutcome::unknown;
    }
    if (contains(ArchitectureImpactOutcome::deferred)) {
        return ArchitectureImpactOutcome::deferred;
    }
    if (contains(ArchitectureImpactOutcome::adapter_unsatisfied)) {
        return ArchitectureImpactOutcome::adapter_unsatisfied;
    }
    if (contains(ArchitectureImpactOutcome::resource_unsatisfied)) {
        return ArchitectureImpactOutcome::resource_unsatisfied;
    }
    if (contains(ArchitectureImpactOutcome::policy_conflict)) {
        return ArchitectureImpactOutcome::policy_conflict;
    }
    return ArchitectureImpactOutcome::unsupported;
}

[[nodiscard]] ArchitectureImpactAnalysis analyze_impact(
    const ArchitectureGraphSnapshot& live,
    const ArchitectureGraphSnapshot& shadow,
    const ArchitectureEvaluationEvidence& evidence,
    const ArchitectureLimits& limits,
    const std::string& transaction_id) {
    ArchitectureImpactAnalysis impact{};
    impact.outcome = ArchitectureImpactOutcome::valid;
    impact.rollback_feasibility =
        ArchitectureRollbackFeasibility::shadow_discard_feasible;
    impact.lifecycle_impact =
        ArchitectureLifecycleImpact::restart_not_likely;
    bool exhausted = false;

    for (const auto& component : shadow.components) {
        const auto* before = find_by_id(live.components, component.component_id);
        if (before == nullptr) {
            append_unique(
                impact.introduced_components,
                component.component_id,
                limits,
                exhausted);
            append_unique(
                impact.affected_components,
                component.component_id,
                limits,
                exhausted);
        } else {
            if (before->enabled != component.enabled ||
                before->weight != component.weight ||
                !dependencies_equal(
                    before->dependencies, component.dependencies) ||
                !resource_equal(
                    before->resource_requirement,
                    component.resource_requirement)) {
                append_unique(
                    impact.affected_components,
                    component.component_id,
                    limits,
                    exhausted);
            }
            if (!dependencies_equal(
                    before->dependencies, component.dependencies)) {
                append_unique(
                    impact.affected_dependencies,
                    component.component_id,
                    limits,
                    exhausted);
            }
            if (!resource_equal(
                    before->resource_requirement,
                    component.resource_requirement)) {
                if (impact.tenancy_changes.size() <
                    limits.maximum_evidence_entries) {
                    impact.tenancy_changes.push_back(
                        {component.component_id,
                         before->resource_requirement.tenancy,
                         component.resource_requirement.tenancy});
                    impact.memory_changes.push_back(
                        {component.component_id,
                         before->resource_requirement.required_memory_bytes,
                         component.resource_requirement.required_memory_bytes});
                } else {
                    exhausted = true;
                }
            }
        }
        if (component.component_type ==
                ArchitectureComponentType::adaptive_state &&
            std::find(
                impact.affected_components.begin(),
                impact.affected_components.end(),
                component.component_id) != impact.affected_components.end()) {
            append_unique(
                impact.adaptive_state_impacts,
                component.component_id,
                limits,
                exhausted);
        }
        for (const auto kind :
             component.resource_requirement.allowed_backend_kinds) {
            if (std::find(
                    impact.required_backend_kinds.begin(),
                    impact.required_backend_kinds.end(),
                    kind) == impact.required_backend_kinds.end()) {
                if (impact.required_backend_kinds.size() <
                    limits.maximum_evidence_entries) {
                    impact.required_backend_kinds.push_back(kind);
                } else {
                    exhausted = true;
                }
            }
        }
        for (const std::string& adapter :
             component.resource_requirement.operation_adapters) {
            if (impact.adapter_requirements.size() <
                limits.maximum_evidence_entries) {
                impact.adapter_requirements.push_back(
                    {component.component_id, adapter});
            } else {
                exhausted = true;
            }
        }

        const auto& requirement = component.resource_requirement;
        if (!component.enabled || !has_resource_requirement(requirement)) {
            continue;
        }
        if (evidence.resources.empty()) {
            append_unique(
                impact.unresolved_assumptions,
                "resource.evidence.absent." + component.component_id,
                limits,
                exhausted);
            if (impact.outcome == ArchitectureImpactOutcome::valid) {
                impact.outcome = ArchitectureImpactOutcome::unknown;
            }
            continue;
        }

        ExecutionPolicyRequest request{};
        request.identity.workload_id = component.component_id;
        request.identity.request_id = transaction_id;
        request.tenancy = requirement.tenancy;
        request.concurrency.requested_execution_lanes =
            requirement.requested_execution_lanes;
        request.placement.allowed_backend_kinds =
            requirement.allowed_backend_kinds;
        request.placement.required_operation_adapters =
            requirement.operation_adapters;
        request.required_memory_bytes = requirement.required_memory_bytes;
        request.reservation_headroom_bytes =
            requirement.reservation_headroom_bytes;

        bool satisfied = false;
        std::vector<ArchitectureImpactOutcome> outcomes{};
        outcomes.reserve(evidence.resources.size());
        for (const ExecutionPolicyResource& resource : evidence.resources) {
            const ArchitectureImpactOutcome outcome = outcome_for_admission(
                evaluate_execution_policy(request, resource));
            outcomes.push_back(outcome);
            satisfied = satisfied || outcome == ArchitectureImpactOutcome::valid;
        }
        if (!satisfied) {
            const ArchitectureImpactOutcome component_outcome =
                aggregate_unsatisfied_outcomes(outcomes);
            if (impact.outcome == ArchitectureImpactOutcome::valid ||
                component_outcome == ArchitectureImpactOutcome::unknown ||
                component_outcome == ArchitectureImpactOutcome::deferred) {
                impact.outcome = component_outcome;
            }
            append_unique(
                impact.unresolved_assumptions,
                "resource.requirement.unsatisfied." + component.component_id,
                limits,
                exhausted);
        }
    }

    for (const auto& component : live.components) {
        if (find_by_id(shadow.components, component.component_id) == nullptr) {
            append_unique(
                impact.removed_components,
                component.component_id,
                limits,
                exhausted);
            append_unique(
                impact.affected_components,
                component.component_id,
                limits,
                exhausted);
        }
    }
    for (const auto& edge : shadow.edges) {
        if (find_by_id(live.edges, edge.edge_id) == nullptr) {
            append_unique(
                impact.introduced_edges, edge.edge_id, limits, exhausted);
            append_unique(
                impact.affected_components,
                edge.source_component_id,
                limits,
                exhausted);
            append_unique(
                impact.affected_components,
                edge.target_component_id,
                limits,
                exhausted);
            append_unique(
                impact.affected_dependencies,
                edge.edge_id,
                limits,
                exhausted);
        }
    }
    for (const auto& edge : live.edges) {
        if (find_by_id(shadow.edges, edge.edge_id) == nullptr) {
            append_unique(
                impact.removed_edges, edge.edge_id, limits, exhausted);
            append_unique(
                impact.affected_components,
                edge.source_component_id,
                limits,
                exhausted);
            append_unique(
                impact.affected_components,
                edge.target_component_id,
                limits,
                exhausted);
            append_unique(
                impact.affected_dependencies,
                edge.edge_id,
                limits,
                exhausted);
        }
    }

    for (const std::string& identity : impact.affected_components) {
        const auto* after = find_by_id(shadow.components, identity);
        const auto* before = find_by_id(live.components, identity);
        const ArchitectureComponentType type =
            after != nullptr ? after->component_type
                             : before != nullptr
                                   ? before->component_type
                                   : ArchitectureComponentType::unknown;
        if (type == ArchitectureComponentType::service ||
            type == ArchitectureComponentType::control_stage) {
            impact.lifecycle_impact =
                ArchitectureLifecycleImpact::restart_likely;
            break;
        }
        if (type == ArchitectureComponentType::unknown) {
            impact.lifecycle_impact = ArchitectureLifecycleImpact::unknown;
        }
    }
    if (exhausted) {
        impact.outcome = ArchitectureImpactOutcome::resource_exhausted;
    }
    return impact;
}

[[nodiscard]] const char* shadow_message(
    ArchitectureImpactOutcome outcome) noexcept {
    switch (outcome) {
        case ArchitectureImpactOutcome::valid:
            return "Shadow architecture is structurally valid and supported by supplied evidence";
        case ArchitectureImpactOutcome::invalid:
            return "Shadow architecture is structurally invalid";
        case ArchitectureImpactOutcome::deferred:
            return "Shadow impact requires deferred evidence or readiness";
        case ArchitectureImpactOutcome::unsupported:
            return "Shadow requirement is unsupported by supplied evidence";
        case ArchitectureImpactOutcome::unknown:
            return "Shadow impact remains unknown from supplied evidence";
        case ArchitectureImpactOutcome::policy_conflict:
            return "Shadow requirement conflicts with supplied resource policy";
        case ArchitectureImpactOutcome::resource_unsatisfied:
            return "Shadow resource requirement is unsatisfied";
        case ArchitectureImpactOutcome::adapter_unsatisfied:
            return "Shadow adapter requirement is unsatisfied";
        case ArchitectureImpactOutcome::authority_denied:
            return "Architecture proposal, validation, or shadow authority is denied";
        case ArchitectureImpactOutcome::version_conflict:
            return "Expected graph revision is stale";
        case ArchitectureImpactOutcome::resource_exhausted:
            return "Architecture shadow evaluation exhausted a configured resource";
    }
    return "Architecture shadow outcome is unknown";
}

}  // namespace

ArchitectureShadowEvaluator::ArchitectureShadowEvaluator(
    ArchitectureLimits limits)
    : limits_(limits) {}

ArchitectureShadowResult ArchitectureShadowEvaluator::evaluate(
    const ArchitectureGraphSnapshot& live_graph,
    const ArchitectureTransaction& transaction,
    const ArchitectureProposalAuthority& proposal_authority,
    const ArchitectureValidationAuthority& validation_authority,
    const ArchitectureShadowAuthority& shadow_authority,
    const ArchitectureEvaluationEvidence& evidence) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        ArchitectureShadowResult result{};
        result.transaction_id = transaction.transaction_id;
        result.live_graph_revision = live_graph.graph_revision;
        result.shadow_graph_revision = live_graph.graph_revision;
        result.live_state_modified = false;
        const auto finish = [&result](ArchitectureImpactOutcome outcome) {
            result.outcome = outcome;
            result.message = shadow_message(outcome);
        };
        const ArchitectureGraphValidationResult limits_check =
            validate_architecture_graph(
                ArchitectureGraphSnapshot{}, limits_);
        if (limits_check.code ==
                ArchitectureGraphValidationCode::resource_exhausted ||
            transaction.proposals.size() >
                limits_.maximum_proposals_per_transaction ||
            evidence.resources.size() > limits_.maximum_evidence_entries) {
            finish(ArchitectureImpactOutcome::resource_exhausted);
            return result;
        }
        const auto valid_identity = [this](const std::string& value) {
            return is_valid_architecture_identifier(
                value, limits_.maximum_identifier_length);
        };
        if (!valid_identity(transaction.transaction_id) ||
            !valid_identity(transaction.coordinator_identity) ||
            !valid_identity(proposal_authority.authority_identity) ||
            !valid_identity(validation_authority.authority_identity) ||
            !valid_identity(shadow_authority.authority_identity) ||
            proposal_authority.evidence_reference.size() >
                limits_.maximum_metadata_length ||
            validation_authority.evidence_reference.size() >
                limits_.maximum_metadata_length ||
            shadow_authority.evidence_reference.size() >
                limits_.maximum_metadata_length ||
            transaction.proposals.empty()) {
            finish(ArchitectureImpactOutcome::invalid);
            return result;
        }
        if (proposal_authority.decision !=
                ArchitectureAuthorityDecision::authorized ||
            validation_authority.decision !=
                ArchitectureAuthorityDecision::authorized ||
            shadow_authority.decision !=
                ArchitectureAuthorityDecision::authorized) {
            finish(ArchitectureImpactOutcome::authority_denied);
            return result;
        }
        if (transaction.expected_graph_revision != live_graph.graph_revision) {
            finish(ArchitectureImpactOutcome::version_conflict);
            return result;
        }
        if (std::find(
                processed_transaction_ids_.begin(),
                processed_transaction_ids_.end(),
                transaction.transaction_id) !=
            processed_transaction_ids_.end()) {
            finish(ArchitectureImpactOutcome::invalid);
            return result;
        }

        result.structural_validation =
            validate_architecture_graph(live_graph, limits_);
        if (!result.structural_validation.valid()) {
            finish(result.structural_validation.code ==
                           ArchitectureGraphValidationCode::resource_exhausted
                       ? ArchitectureImpactOutcome::resource_exhausted
                       : result.structural_validation.code ==
                                 ArchitectureGraphValidationCode::unknown
                             ? ArchitectureImpactOutcome::unknown
                             : ArchitectureImpactOutcome::invalid);
            return result;
        }

        for (std::size_t left = 0; left < transaction.proposals.size(); ++left) {
            const ArchitectureProposal& proposal = transaction.proposals[left];
            const auto shape =
                validate_architecture_proposal_shape(proposal, limits_);
            if (!shape.valid()) {
                result.proposal_results.push_back(validation_result(
                    proposal,
                    shape.code == ArchitectureProposalShapeCode::resource_exhausted
                        ? ArchitectureProposalValidationCode::resource_exhausted
                        : ArchitectureProposalValidationCode::invalid,
                    shape.message));
                finish(shape.code == ArchitectureProposalShapeCode::resource_exhausted
                           ? ArchitectureImpactOutcome::resource_exhausted
                           : ArchitectureImpactOutcome::invalid);
                return result;
            }
            if (proposal.expected_graph_revision != live_graph.graph_revision) {
                result.proposal_results.push_back(validation_result(
                    proposal,
                    ArchitectureProposalValidationCode::version_conflict,
                    "Proposal expected graph revision is stale"));
                finish(ArchitectureImpactOutcome::version_conflict);
                return result;
            }
            if (proposal.proposal_authority_reference !=
                proposal_authority.evidence_reference) {
                result.proposal_results.push_back(validation_result(
                    proposal,
                    ArchitectureProposalValidationCode::authority_denied,
                    "Proposal authority reference is not validated"));
                finish(ArchitectureImpactOutcome::authority_denied);
                return result;
            }
            if (std::find(
                    processed_proposal_ids_.begin(),
                    processed_proposal_ids_.end(),
                    proposal.proposal_id) != processed_proposal_ids_.end()) {
                result.proposal_results.push_back(validation_result(
                    proposal,
                    ArchitectureProposalValidationCode::duplicate,
                    "Proposal identity was already evaluated"));
                finish(ArchitectureImpactOutcome::invalid);
                return result;
            }
            for (std::size_t right = left + 1;
                 right < transaction.proposals.size();
                 ++right) {
                if (proposal.proposal_id ==
                        transaction.proposals[right].proposal_id ||
                    proposal.target_identity ==
                        transaction.proposals[right].target_identity) {
                    result.proposal_results.push_back(validation_result(
                        proposal,
                        ArchitectureProposalValidationCode::duplicate,
                        "Transaction contains duplicate proposal or target identity"));
                    finish(ArchitectureImpactOutcome::invalid);
                    return result;
                }
            }
        }

        std::vector<std::string> next_transaction_ids =
            processed_transaction_ids_;
        std::vector<std::string> next_proposal_ids = processed_proposal_ids_;
        append_bounded(
            next_transaction_ids,
            transaction.transaction_id,
            limits_.maximum_history_entries);
        for (const ArchitectureProposal& proposal : transaction.proposals) {
            append_bounded(
                next_proposal_ids,
                proposal.proposal_id,
                limits_.maximum_history_entries);
        }

        ArchitectureGraphSnapshot shadow = live_graph;
        result.proposal_results.reserve(transaction.proposals.size());
        for (const ArchitectureProposal& proposal : transaction.proposals) {
            ArchitectureProposalValidationResult proposal_result =
                apply_proposal(shadow, proposal);
            result.proposal_results.push_back(proposal_result);
            if (!proposal_result.valid()) {
                processed_transaction_ids_.swap(next_transaction_ids);
                processed_proposal_ids_.swap(next_proposal_ids);
                finish(ArchitectureImpactOutcome::invalid);
                return result;
            }
        }
        if (live_graph.graph_revision ==
            std::numeric_limits<std::uint64_t>::max()) {
            finish(ArchitectureImpactOutcome::resource_exhausted);
            return result;
        }
        shadow.graph_revision = live_graph.graph_revision + 1;
        for (ArchitectureComponentDescriptor& component : shadow.components) {
            std::sort(
                component.dependencies.begin(),
                component.dependencies.end(),
                [](const auto& left, const auto& right) {
                    return left.component_id < right.component_id;
                });
            std::sort(
                component.resource_requirement.operation_adapters.begin(),
                component.resource_requirement.operation_adapters.end());
            std::sort(
                component.resource_requirement.allowed_backend_kinds.begin(),
                component.resource_requirement.allowed_backend_kinds.end());
            std::sort(
                component.memory_contract_references.begin(),
                component.memory_contract_references.end());
            std::sort(
                component.acs_contract_references.begin(),
                component.acs_contract_references.end());
        }
        std::sort(
            shadow.components.begin(),
            shadow.components.end(),
            [](const auto& left, const auto& right) {
                return left.component_id < right.component_id;
            });
        std::sort(
            shadow.edges.begin(),
            shadow.edges.end(),
            [](const auto& left, const auto& right) {
                return left.edge_id < right.edge_id;
            });
        result.structural_validation =
            validate_architecture_graph(shadow, limits_);
        if (!result.structural_validation.valid()) {
            for (const ArchitectureGraphIssue& issue :
                 result.structural_validation.issues) {
                if (result.impact.unresolved_references.size() <
                    limits_.maximum_evidence_entries) {
                    result.impact.unresolved_references.push_back(
                        issue.subject_id + ":" + issue.related_id);
                }
            }
            processed_transaction_ids_.swap(next_transaction_ids);
            processed_proposal_ids_.swap(next_proposal_ids);
            finish(result.structural_validation.code ==
                           ArchitectureGraphValidationCode::unknown
                       ? ArchitectureImpactOutcome::unknown
                       : ArchitectureImpactOutcome::invalid);
            return result;
        }

        result.impact = analyze_impact(
            live_graph, shadow, evidence, limits_, transaction.transaction_id);
        result.shadow_graph_revision = shadow.graph_revision;
        result.shadow_created = true;
        result.shadow_graph = std::move(shadow);
        processed_transaction_ids_.swap(next_transaction_ids);
        processed_proposal_ids_.swap(next_proposal_ids);
        finish(result.impact.outcome);
        return result;
    } catch (const std::bad_alloc&) {
        ArchitectureShadowResult result{};
        result.outcome = ArchitectureImpactOutcome::resource_exhausted;
        result.message = shadow_message(result.outcome);
        return result;
    } catch (...) {
        ArchitectureShadowResult result{};
        result.outcome = ArchitectureImpactOutcome::invalid;
        result.message = "Architecture shadow evaluation failed safely";
        return result;
    }
}

const char* to_string(ArchitectureAuthorityDecision value) noexcept {
    switch (value) {
        case ArchitectureAuthorityDecision::unknown: return "unknown";
        case ArchitectureAuthorityDecision::authorized: return "authorized";
        case ArchitectureAuthorityDecision::denied: return "denied";
    }
    return "unknown";
}

const char* to_string(ArchitectureProposalValidationCode value) noexcept {
    switch (value) {
        case ArchitectureProposalValidationCode::valid: return "valid";
        case ArchitectureProposalValidationCode::invalid: return "invalid";
        case ArchitectureProposalValidationCode::not_found: return "not_found";
        case ArchitectureProposalValidationCode::duplicate: return "duplicate";
        case ArchitectureProposalValidationCode::authority_denied:
            return "authority_denied";
        case ArchitectureProposalValidationCode::version_conflict:
            return "version_conflict";
        case ArchitectureProposalValidationCode::type_mismatch:
            return "type_mismatch";
        case ArchitectureProposalValidationCode::orphaning: return "orphaning";
        case ArchitectureProposalValidationCode::resource_exhausted:
            return "resource_exhausted";
        case ArchitectureProposalValidationCode::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(ArchitectureImpactOutcome value) noexcept {
    switch (value) {
        case ArchitectureImpactOutcome::valid: return "valid";
        case ArchitectureImpactOutcome::invalid: return "invalid";
        case ArchitectureImpactOutcome::deferred: return "deferred";
        case ArchitectureImpactOutcome::unsupported: return "unsupported";
        case ArchitectureImpactOutcome::unknown: return "unknown";
        case ArchitectureImpactOutcome::policy_conflict: return "policy_conflict";
        case ArchitectureImpactOutcome::resource_unsatisfied:
            return "resource_unsatisfied";
        case ArchitectureImpactOutcome::adapter_unsatisfied:
            return "adapter_unsatisfied";
        case ArchitectureImpactOutcome::authority_denied:
            return "authority_denied";
        case ArchitectureImpactOutcome::version_conflict:
            return "version_conflict";
        case ArchitectureImpactOutcome::resource_exhausted:
            return "resource_exhausted";
    }
    return "unknown";
}

const char* to_string(ArchitectureLifecycleImpact value) noexcept {
    switch (value) {
        case ArchitectureLifecycleImpact::restart_not_likely:
            return "restart_not_likely";
        case ArchitectureLifecycleImpact::restart_likely:
            return "restart_likely";
        case ArchitectureLifecycleImpact::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(ArchitectureRollbackFeasibility value) noexcept {
    switch (value) {
        case ArchitectureRollbackFeasibility::shadow_discard_feasible:
            return "shadow_discard_feasible";
        case ArchitectureRollbackFeasibility::live_rollback_infeasible:
            return "live_rollback_infeasible";
        case ArchitectureRollbackFeasibility::unknown: return "unknown";
    }
    return "unknown";
}

}  // namespace prometheus::core
