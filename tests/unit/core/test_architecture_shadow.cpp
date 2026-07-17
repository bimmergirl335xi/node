#include <cstdlib>
#include <cstdint>
#include <future>
#include <string>
#include <utility>
#include <vector>

#include "core/architecture_shadow.hpp"

namespace pc = prometheus::core;

namespace {

[[nodiscard]] pc::ArchitectureComponentDescriptor component(
    std::string id,
    pc::ArchitectureComponentType type) {
    pc::ArchitectureComponentDescriptor descriptor{};
    descriptor.component_id = std::move(id);
    descriptor.display_name = "Shadow component";
    descriptor.component_type = type;
    descriptor.weight = 0.5;
    return descriptor;
}

[[nodiscard]] pc::ArchitectureEdgeDescriptor edge(
    std::string id,
    std::string source,
    std::string target) {
    pc::ArchitectureEdgeDescriptor descriptor{};
    descriptor.edge_id = std::move(id);
    descriptor.source_component_id = std::move(source);
    descriptor.target_component_id = std::move(target);
    descriptor.edge_type = pc::ArchitectureEdgeType::data_flow;
    return descriptor;
}

[[nodiscard]] pc::ArchitectureGraphSnapshot live_graph() {
    pc::ArchitectureGraph graph{};
    auto service = component(
        "component.service", pc::ArchitectureComponentType::service);
    service.dependencies.push_back(
        {"component.adapter",
         pc::ArchitectureComponentType::backend_adapter});
    (void)graph.register_component(service);
    (void)graph.register_component(component(
        "component.adapter",
        pc::ArchitectureComponentType::backend_adapter));
    (void)graph.register_component(component(
        "component.alternative",
        pc::ArchitectureComponentType::backend_adapter));
    (void)graph.register_edge(edge(
        "edge.adapter.service",
        "component.adapter",
        "component.service"));
    return graph.snapshot();
}

[[nodiscard]] pc::ArchitectureProposal proposal(
    std::string id,
    std::string target,
    pc::ArchitectureProposalOperation operation,
    pc::ArchitectureProposalPayload payload,
    std::uint64_t revision) {
    pc::ArchitectureProposal result{};
    result.proposal_id = std::move(id);
    result.source_identity = "planner.native";
    result.source_category =
        pc::ArchitectureProposalSourceCategory::automatic_internal;
    result.proposal_authority_reference = "evidence.proposal";
    result.expected_graph_revision = revision;
    result.target_identity = std::move(target);
    result.operation = operation;
    result.payload = std::move(payload);
    result.correlation_id = "correlation.shadow";
    result.evidence_reference = "evidence.shadow";
    result.explanation = "Isolated shadow proposal";
    return result;
}

[[nodiscard]] pc::ArchitectureTransaction transaction(
    std::string id,
    std::uint64_t revision,
    std::vector<pc::ArchitectureProposal> proposals) {
    return {std::move(id),
            "runtime.shadow.coordinator",
            revision,
            std::move(proposals)};
}

[[nodiscard]] pc::ArchitectureProposalAuthority proposal_authority() {
    return {pc::ArchitectureAuthorityDecision::authorized,
            "runtime.proposal.authority",
            "evidence.proposal"};
}

[[nodiscard]] pc::ArchitectureValidationAuthority validation_authority() {
    return {pc::ArchitectureAuthorityDecision::authorized,
            "runtime.validation.authority",
            "evidence.validation"};
}

[[nodiscard]] pc::ArchitectureShadowAuthority shadow_authority() {
    return {pc::ArchitectureAuthorityDecision::authorized,
            "runtime.shadow.authority",
            "evidence.shadow"};
}

[[nodiscard]] const pc::ArchitectureComponentDescriptor* find_component(
    const pc::ArchitectureGraphSnapshot& graph,
    const std::string& id) {
    for (const auto& candidate : graph.components) {
        if (candidate.component_id == id) {
            return &candidate;
        }
    }
    return nullptr;
}

[[nodiscard]] pc::ExecutionPolicyResource resource_evidence(
    pc::ResourceTenancy tenancy,
    bool include_adapter = true,
    std::uint64_t available_memory = 4096) {
    pc::ExecutionPolicyResource resource{};
    resource.resource_id = "resource.cpu.zero";
    resource.backend_id = "backend.cpu";
    resource.backend_kind = prometheus::backends::ComputeBackendKind::cpu;
    resource.operation_adapters = include_adapter
                                      ? std::vector<std::string>{"adapter.math"}
                                      : std::vector<std::string>{};
    resource.operational_state = pc::ResourceOperationalState::ready;
    resource.tenancy = tenancy;
    resource.concurrency.concurrent_workload_support =
        pc::PolicySupport::supported;
    resource.concurrency.concurrent_lane_support = pc::PolicySupport::supported;
    resource.concurrency.maximum_concurrent_workloads = 4;
    resource.concurrency.maximum_execution_lanes = 8;
    resource.memory_capacity_known = true;
    resource.total_memory_bytes = 8192;
    resource.available_memory_bytes = available_memory;
    return resource;
}

[[nodiscard]] bool snapshots_equal(
    const pc::ArchitectureGraphSnapshot& left,
    const pc::ArchitectureGraphSnapshot& right) {
    if (left.graph_revision != right.graph_revision ||
        left.schema_version != right.schema_version ||
        left.components.size() != right.components.size() ||
        left.edges.size() != right.edges.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.components.size(); ++index) {
        if (left.components[index].component_id !=
                right.components[index].component_id ||
            left.components[index].weight != right.components[index].weight ||
            left.components[index].enabled != right.components[index].enabled) {
            return false;
        }
    }
    for (std::size_t index = 0; index < left.edges.size(); ++index) {
        if (left.edges[index].edge_id != right.edges[index].edge_id ||
            left.edges[index].weight != right.edges[index].weight) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool test_dependency_replacement_and_live_protection() {
    const auto live = live_graph();
    pc::ArchitectureShadowEvaluator evaluator{};
    const auto replace = proposal(
        "proposal.replace.dependency",
        "component.service",
        pc::ArchitectureProposalOperation::replace_dependency,
        pc::ArchitectureReplaceDependencyPayload{
            "component.adapter",
            {"component.alternative",
             pc::ArchitectureComponentType::backend_adapter}},
        live.graph_revision);
    const auto result = evaluator.evaluate(
        live,
        transaction(
            "transaction.replace.dependency",
            live.graph_revision,
            {replace}),
        proposal_authority(),
        validation_authority(),
        shadow_authority());
    if (!result.structurally_valid() || result.live_state_modified ||
        !result.shadow_graph.has_value() ||
        result.shadow_graph_revision != live.graph_revision + 1 ||
        result.impact.lifecycle_impact !=
            pc::ArchitectureLifecycleImpact::restart_likely ||
        result.impact.rollback_feasibility !=
            pc::ArchitectureRollbackFeasibility::shadow_discard_feasible) {
        return false;
    }
    const auto* service =
        find_component(*result.shadow_graph, "component.service");
    const auto* live_service = find_component(live, "component.service");
    return service != nullptr && live_service != nullptr &&
           service->dependencies[0].component_id == "component.alternative" &&
           live_service->dependencies[0].component_id == "component.adapter";
}

[[nodiscard]] bool test_atomic_multi_operation_and_orphan_prevention() {
    const auto live = live_graph();
    pc::ArchitectureShadowEvaluator evaluator{};
    const auto new_component = component(
        "component.compute", pc::ArchitectureComponentType::compute_stage);
    const auto add_component = proposal(
        "proposal.add.component",
        "component.compute",
        pc::ArchitectureProposalOperation::add_component,
        pc::ArchitectureAddComponentPayload{new_component},
        live.graph_revision);
    const auto add_edge = proposal(
        "proposal.add.edge",
        "edge.compute.service",
        pc::ArchitectureProposalOperation::add_edge,
        pc::ArchitectureAddEdgePayload{edge(
            "edge.compute.service",
            "component.compute",
            "component.service")},
        live.graph_revision);
    const auto atomic = evaluator.evaluate(
        live,
        transaction(
            "transaction.atomic.shadow",
            live.graph_revision,
            {add_component, add_edge}),
        proposal_authority(),
        validation_authority(),
        shadow_authority());
    if (!atomic.structurally_valid() ||
        atomic.impact.introduced_components.size() != 1 ||
        atomic.impact.introduced_edges.size() != 1) {
        return false;
    }

    const auto remove = proposal(
        "proposal.remove.adapter",
        "component.adapter",
        pc::ArchitectureProposalOperation::remove_component,
        pc::ArchitectureNoPayload{},
        live.graph_revision);
    const auto invalid = evaluator.evaluate(
        live,
        transaction(
            "transaction.orphan.rejected",
            live.graph_revision,
            {remove}),
        proposal_authority(),
        validation_authority(),
        shadow_authority());
    return invalid.outcome == pc::ArchitectureImpactOutcome::invalid &&
           !invalid.shadow_created && !invalid.shadow_graph.has_value() &&
           snapshots_equal(live, live);
}

[[nodiscard]] bool test_weight_enable_version_duplicate_and_authority() {
    const auto live = live_graph();
    pc::ArchitectureShadowEvaluator evaluator{};
    const auto weight = proposal(
        "proposal.weight.valid",
        "component.adapter",
        pc::ArchitectureProposalOperation::adjust_weight,
        pc::ArchitectureAdjustWeightPayload{
            pc::ArchitectureWeightTarget::component, 0.25},
        live.graph_revision);
    const auto valid_transaction = transaction(
        "transaction.weight.valid", live.graph_revision, {weight});
    const auto valid = evaluator.evaluate(
        live,
        valid_transaction,
        proposal_authority(),
        validation_authority(),
        shadow_authority());
    const auto duplicate_transaction = evaluator.evaluate(
        live,
        valid_transaction,
        proposal_authority(),
        validation_authority(),
        shadow_authority());

    auto stale_transaction = transaction(
        "transaction.stale",
        live.graph_revision,
        {proposal(
            "proposal.stale",
            "component.adapter",
            pc::ArchitectureProposalOperation::adjust_weight,
            pc::ArchitectureAdjustWeightPayload{
                pc::ArchitectureWeightTarget::component, 0.3},
            live.graph_revision)});
    stale_transaction.expected_graph_revision = live.graph_revision - 1;
    const auto stale = evaluator.evaluate(
        live,
        stale_transaction,
        proposal_authority(),
        validation_authority(),
        shadow_authority());

    auto denied_authority = shadow_authority();
    denied_authority.decision = pc::ArchitectureAuthorityDecision::denied;
    const auto denied = evaluator.evaluate(
        live,
        transaction(
            "transaction.denied",
            live.graph_revision,
            {proposal(
                "proposal.denied",
                "component.adapter",
                pc::ArchitectureProposalOperation::adjust_weight,
                pc::ArchitectureAdjustWeightPayload{
                    pc::ArchitectureWeightTarget::component, 0.4},
                live.graph_revision)}),
        proposal_authority(),
        validation_authority(),
        denied_authority);

    const auto disable = evaluator.evaluate(
        live,
        transaction(
            "transaction.disable.dependency",
            live.graph_revision,
            {proposal(
                "proposal.disable.dependency",
                "component.adapter",
                pc::ArchitectureProposalOperation::disable_component,
                pc::ArchitectureNoPayload{},
                live.graph_revision)}),
        proposal_authority(),
        validation_authority(),
        shadow_authority());

    auto duplicate_proposal_a = weight;
    duplicate_proposal_a.proposal_id = "proposal.duplicate.same";
    auto duplicate_proposal_b = duplicate_proposal_a;
    duplicate_proposal_b.target_identity = "component.alternative";
    const auto duplicate_proposals = evaluator.evaluate(
        live,
        transaction(
            "transaction.duplicate.proposals",
            live.graph_revision,
            {duplicate_proposal_a, duplicate_proposal_b}),
        proposal_authority(),
        validation_authority(),
        shadow_authority());

    return valid.structurally_valid() &&
           duplicate_transaction.outcome ==
               pc::ArchitectureImpactOutcome::invalid &&
           stale.outcome == pc::ArchitectureImpactOutcome::version_conflict &&
           denied.outcome == pc::ArchitectureImpactOutcome::authority_denied &&
           disable.outcome == pc::ArchitectureImpactOutcome::invalid &&
           duplicate_proposals.outcome ==
               pc::ArchitectureImpactOutcome::invalid;
}

[[nodiscard]] pc::ArchitectureGraphSnapshot resource_graph(
    pc::ResourceTenancy tenancy) {
    pc::ArchitectureGraph graph{};
    auto compute = component(
        "component.resource", pc::ArchitectureComponentType::compute_stage);
    compute.resource_requirement.allowed_backend_kinds = {
        prometheus::backends::ComputeBackendKind::cpu};
    compute.resource_requirement.tenancy = tenancy;
    compute.resource_requirement.required_memory_bytes = 1024;
    compute.resource_requirement.requested_execution_lanes = 1;
    compute.resource_requirement.operation_adapters = {"adapter.math"};
    (void)graph.register_component(compute);
    return graph.snapshot();
}

[[nodiscard]] pc::ArchitectureTransaction resource_transaction(
    const pc::ArchitectureGraphSnapshot& live,
    std::string suffix) {
    return transaction(
        "transaction.resource." + suffix,
        live.graph_revision,
        {proposal(
            "proposal.resource." + suffix,
            "component.resource",
            pc::ArchitectureProposalOperation::adjust_weight,
            pc::ArchitectureAdjustWeightPayload{
                pc::ArchitectureWeightTarget::component, 0.75},
            live.graph_revision)});
}

[[nodiscard]] bool test_resource_policy_impacts() {
    const auto exclusive_live = resource_graph(pc::ResourceTenancy::exclusive);
    pc::ArchitectureShadowEvaluator evaluator{};
    const auto valid = evaluator.evaluate(
        exclusive_live,
        resource_transaction(exclusive_live, "valid"),
        proposal_authority(),
        validation_authority(),
        shadow_authority(),
        {{resource_evidence(pc::ResourceTenancy::exclusive)}});
    const auto adapter = evaluator.evaluate(
        exclusive_live,
        resource_transaction(exclusive_live, "adapter"),
        proposal_authority(),
        validation_authority(),
        shadow_authority(),
        {{resource_evidence(pc::ResourceTenancy::exclusive, false)}});
    const auto memory = evaluator.evaluate(
        exclusive_live,
        resource_transaction(exclusive_live, "memory"),
        proposal_authority(),
        validation_authority(),
        shadow_authority(),
        {{resource_evidence(pc::ResourceTenancy::exclusive, true, 512)}});
    const auto conflict = evaluator.evaluate(
        exclusive_live,
        resource_transaction(exclusive_live, "conflict"),
        proposal_authority(),
        validation_authority(),
        shadow_authority(),
        {{resource_evidence(pc::ResourceTenancy::shareable)}});
    const auto unknown = evaluator.evaluate(
        exclusive_live,
        resource_transaction(exclusive_live, "unknown"),
        proposal_authority(),
        validation_authority(),
        shadow_authority());

    pc::ArchitectureResourceRequirement shared_requirement =
        find_component(exclusive_live, "component.resource")
            ->resource_requirement;
    shared_requirement.tenancy = pc::ResourceTenancy::shareable;
    const auto tenancy = evaluator.evaluate(
        exclusive_live,
        transaction(
            "transaction.resource.tenancy",
            exclusive_live.graph_revision,
            {proposal(
                "proposal.resource.tenancy",
                "component.resource",
                pc::ArchitectureProposalOperation::replace_resource_requirement,
                pc::ArchitectureReplaceResourcePayload{shared_requirement},
                exclusive_live.graph_revision)}),
        proposal_authority(),
        validation_authority(),
        shadow_authority(),
        {{resource_evidence(pc::ResourceTenancy::shareable)}});

    return valid.outcome == pc::ArchitectureImpactOutcome::valid &&
           adapter.outcome ==
               pc::ArchitectureImpactOutcome::adapter_unsatisfied &&
           memory.outcome ==
               pc::ArchitectureImpactOutcome::resource_unsatisfied &&
           conflict.outcome == pc::ArchitectureImpactOutcome::policy_conflict &&
           unknown.outcome == pc::ArchitectureImpactOutcome::unknown &&
           !unknown.impact.unresolved_assumptions.empty() &&
           tenancy.structurally_valid() &&
           tenancy.impact.tenancy_changes.size() == 1;
}

[[nodiscard]] bool test_bounds_and_concurrent_shadow_evaluation() {
    const auto live = live_graph();
    pc::ArchitectureLimits limits{};
    limits.maximum_proposals_per_transaction = 1;
    pc::ArchitectureShadowEvaluator bounded{limits};
    auto first = proposal(
        "proposal.bound.first",
        "component.adapter",
        pc::ArchitectureProposalOperation::adjust_weight,
        pc::ArchitectureAdjustWeightPayload{
            pc::ArchitectureWeightTarget::component, 0.2},
        live.graph_revision);
    auto second = proposal(
        "proposal.bound.second",
        "component.alternative",
        pc::ArchitectureProposalOperation::adjust_weight,
        pc::ArchitectureAdjustWeightPayload{
            pc::ArchitectureWeightTarget::component, 0.3},
        live.graph_revision);
    if (bounded
            .evaluate(
                live,
                transaction(
                    "transaction.bound", live.graph_revision, {first, second}),
                proposal_authority(),
                validation_authority(),
                shadow_authority())
            .outcome != pc::ArchitectureImpactOutcome::resource_exhausted) {
        return false;
    }

    pc::ArchitectureLimits evidence_limits{};
    evidence_limits.maximum_evidence_entries = 1;
    pc::ArchitectureShadowEvaluator evidence_bounded{evidence_limits};
    const auto evidence_result = evidence_bounded.evaluate(
        live,
        transaction(
            "transaction.evidence.bound",
            live.graph_revision,
            {proposal(
                "proposal.evidence.bound",
                "edge.alternative.service",
                pc::ArchitectureProposalOperation::add_edge,
                pc::ArchitectureAddEdgePayload{edge(
                    "edge.alternative.service",
                    "component.alternative",
                    "component.service")},
                live.graph_revision)}),
        proposal_authority(),
        validation_authority(),
        shadow_authority());
    if (evidence_result.outcome !=
            pc::ArchitectureImpactOutcome::resource_exhausted ||
        !evidence_result.shadow_created) {
        return false;
    }

    pc::ArchitectureShadowEvaluator concurrent{};
    std::promise<void> start_promise{};
    std::shared_future<void> start = start_promise.get_future().share();
    std::vector<std::future<bool>> workers{};
    for (std::size_t worker = 0; worker < 4; ++worker) {
        workers.push_back(std::async(
            std::launch::async,
            [&concurrent, &live, start, worker]() {
                start.wait();
                for (std::size_t iteration = 0; iteration < 20; ++iteration) {
                    const std::string suffix = std::to_string(worker) + "." +
                                               std::to_string(iteration);
                    const auto result = concurrent.evaluate(
                        live,
                        transaction(
                            "transaction.concurrent." + suffix,
                            live.graph_revision,
                            {proposal(
                                "proposal.concurrent." + suffix,
                                "component.adapter",
                                pc::ArchitectureProposalOperation::adjust_weight,
                                pc::ArchitectureAdjustWeightPayload{
                                    pc::ArchitectureWeightTarget::component,
                                    0.4},
                                live.graph_revision)}),
                        proposal_authority(),
                        validation_authority(),
                        shadow_authority());
                    if (!result.structurally_valid() ||
                        result.live_state_modified) {
                        return false;
                    }
                }
                return true;
            }));
    }
    start_promise.set_value();
    for (auto& worker : workers) {
        if (!worker.get()) {
            return false;
        }
    }
    return snapshots_equal(live, live);
}

}  // namespace

int main() {
    return test_dependency_replacement_and_live_protection() &&
                   test_atomic_multi_operation_and_orphan_prevention() &&
                   test_weight_enable_version_duplicate_and_authority() &&
                   test_resource_policy_impacts() &&
                   test_bounds_and_concurrent_shadow_evaluation()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
