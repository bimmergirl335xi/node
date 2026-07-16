#include <cstdlib>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/architecture_graph.hpp"

namespace pc = prometheus::core;

namespace {

[[nodiscard]] pc::ArchitectureComponentDescriptor component(
    std::string id,
    pc::ArchitectureComponentType type =
        pc::ArchitectureComponentType::service,
    bool enabled = true) {
    pc::ArchitectureComponentDescriptor descriptor{};
    descriptor.component_id = std::move(id);
    descriptor.display_name = "Logical component";
    descriptor.component_type = type;
    descriptor.enabled = enabled;
    descriptor.weight = 0.5;
    return descriptor;
}

[[nodiscard]] pc::ArchitectureDependencyRequirement dependency(
    std::string id,
    pc::ArchitectureComponentType type =
        pc::ArchitectureComponentType::service) {
    return {std::move(id), type};
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
    descriptor.weight = 0.75;
    return descriptor;
}

[[nodiscard]] pc::ArchitectureProposal proposal(
    std::string id,
    std::string target,
    pc::ArchitectureProposalOperation operation,
    pc::ArchitectureProposalPayload payload = pc::ArchitectureNoPayload{}) {
    pc::ArchitectureProposal result{};
    result.proposal_id = std::move(id);
    result.source_identity = "planner.native";
    result.source_category =
        pc::ArchitectureProposalSourceCategory::automatic_internal;
    result.proposal_authority_reference = "evidence.proposal";
    result.expected_graph_revision = 1;
    result.target_identity = std::move(target);
    result.operation = operation;
    result.payload = std::move(payload);
    result.correlation_id = "correlation.one";
    result.evidence_reference = "evidence.one";
    result.explanation = "Bounded logical architecture proposal";
    return result;
}

[[nodiscard]] bool has_issue(
    const pc::ArchitectureGraphValidationResult& result,
    pc::ArchitectureGraphIssueCode code) {
    for (const auto& issue : result.issues) {
        if (issue.code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool test_empty_valid_and_deterministic_graph() {
    static_assert(!std::is_copy_constructible_v<pc::ArchitectureGraph>);
    static_assert(!std::is_move_constructible_v<pc::ArchitectureGraph>);

    pc::ArchitectureGraph empty{};
    if (!empty.validate().valid() || empty.revision() != 0 ||
        empty.component_count() != 0 || empty.edge_count() != 0) {
        return false;
    }

    pc::ArchitectureGraph graph{};
    auto dependent = component("component.zeta");
    dependent.dependencies.push_back(dependency("component.alpha"));
    if (!graph.register_component(dependent).ok() ||
        !graph.register_component(component("component.alpha")).ok() ||
        !graph.register_edge(
                  edge("edge.flow", "component.alpha", "component.zeta"))
             .ok()) {
        return false;
    }
    const auto snapshot = graph.snapshot();
    const auto validation = graph.validate();
    return snapshot.graph_revision == 3 && snapshot.schema_version == 1 &&
           snapshot.components.size() == 2 && snapshot.edges.size() == 1 &&
           snapshot.components[0].component_id == "component.alpha" &&
           snapshot.components[1].component_id == "component.zeta" &&
           validation.valid() && validation.dependency_order.size() == 2 &&
           validation.dependency_order[0] == "component.alpha" &&
           validation.dependency_order[1] == "component.zeta";
}

[[nodiscard]] bool test_registration_rejections_and_bounds() {
    pc::ArchitectureLimits limits{};
    limits.maximum_components = 1;
    pc::ArchitectureGraph graph{limits};
    auto invalid = component("bad id");
    auto invalid_weight = component("component.weight");
    invalid_weight.weight = 2.0;
    if (graph.register_component(invalid).code !=
            pc::ArchitectureRegistrationCode::invalid_identifier ||
        graph.register_component(invalid_weight).code !=
            pc::ArchitectureRegistrationCode::invalid_descriptor ||
        !graph.register_component(component("component.one")).ok() ||
        graph.register_component(component("component.one")).code !=
            pc::ArchitectureRegistrationCode::duplicate_identifier ||
        graph.register_component(component("component.two")).code !=
            pc::ArchitectureRegistrationCode::capacity_exhausted) {
        return false;
    }

    pc::ArchitectureLimits invalid_limits{};
    invalid_limits.maximum_components = 0;
    pc::ArchitectureGraph invalid_graph{invalid_limits};
    return invalid_graph.register_component(component("blocked")).code ==
           pc::ArchitectureRegistrationCode::invalid_configuration;
}

[[nodiscard]] bool test_missing_self_cycle_and_type_validation() {
    pc::ArchitectureGraph missing{};
    auto missing_component = component("component.missing.user");
    missing_component.dependencies.push_back(
        dependency("component.absent"));
    if (!missing.register_component(missing_component).ok() ||
        !has_issue(
            missing.validate(),
            pc::ArchitectureGraphIssueCode::missing_dependency)) {
        return false;
    }

    pc::ArchitectureGraph self{};
    auto self_component = component("component.self");
    self_component.dependencies.push_back(dependency("component.self"));
    if (!self.register_component(self_component).ok() ||
        !has_issue(
            self.validate(), pc::ArchitectureGraphIssueCode::self_dependency)) {
        return false;
    }

    pc::ArchitectureGraph cycle{};
    auto cycle_a = component("component.cycle.a");
    auto cycle_b = component("component.cycle.b");
    cycle_a.dependencies.push_back(dependency("component.cycle.b"));
    cycle_b.dependencies.push_back(dependency("component.cycle.a"));
    if (!cycle.register_component(cycle_a).ok() ||
        !cycle.register_component(cycle_b).ok() ||
        !has_issue(
            cycle.validate(), pc::ArchitectureGraphIssueCode::dependency_cycle)) {
        return false;
    }

    pc::ArchitectureGraph mismatch{};
    auto adapter = component(
        "component.adapter",
        pc::ArchitectureComponentType::backend_adapter);
    adapter.dependencies.push_back(dependency(
        "component.service", pc::ArchitectureComponentType::compute_stage));
    if (!mismatch.register_component(adapter).ok() ||
        !mismatch.register_component(component("component.service")).ok() ||
        !has_issue(
            mismatch.validate(),
            pc::ArchitectureGraphIssueCode::dependency_type_mismatch)) {
        return false;
    }

    pc::ArchitectureGraph disabled{};
    auto enabled = component("component.enabled");
    enabled.dependencies.push_back(dependency("component.disabled"));
    if (!disabled.register_component(enabled).ok() ||
        !disabled
             .register_component(component(
                 "component.disabled",
                 pc::ArchitectureComponentType::service,
                 false))
             .ok() ||
        !has_issue(
            disabled.validate(),
            pc::ArchitectureGraphIssueCode::disabled_required_dependency)) {
        return false;
    }
    return true;
}

[[nodiscard]] bool test_edge_and_proposal_shape_validation() {
    pc::ArchitectureGraph graph{};
    if (!graph.register_component(component("component.a")).ok() ||
        !graph.register_component(component("component.b")).ok() ||
        !graph.register_edge(edge("edge.valid", "component.a", "component.b"))
             .ok() ||
        graph.register_edge(edge("edge.missing", "component.a", "absent"))
                .code != pc::ArchitectureRegistrationCode::invalid_descriptor) {
        return false;
    }

    const pc::ArchitectureLimits limits{};
    const auto add = proposal(
        "proposal.add",
        "component.new",
        pc::ArchitectureProposalOperation::add_component,
        pc::ArchitectureAddComponentPayload{component("component.new")});
    const auto weight = proposal(
        "proposal.weight",
        "component.a",
        pc::ArchitectureProposalOperation::adjust_weight,
        pc::ArchitectureAdjustWeightPayload{
            pc::ArchitectureWeightTarget::component, 0.25});
    auto invalid_weight = weight;
    invalid_weight.proposal_id = "proposal.weight.invalid";
    std::get<pc::ArchitectureAdjustWeightPayload>(invalid_weight.payload)
        .weight = -1.0;
    const auto mismatched = proposal(
        "proposal.mismatch",
        "component.a",
        pc::ArchitectureProposalOperation::remove_component,
        pc::ArchitectureAddComponentPayload{component("component.a")});
    auto invalid_identity = add;
    invalid_identity.proposal_id = "bad id";
    return pc::validate_architecture_proposal_shape(add, limits).valid() &&
           pc::validate_architecture_proposal_shape(weight, limits).valid() &&
           pc::validate_architecture_proposal_shape(invalid_weight, limits)
                   .code == pc::ArchitectureProposalShapeCode::invalid_metadata &&
           pc::validate_architecture_proposal_shape(mismatched, limits).code ==
               pc::ArchitectureProposalShapeCode::payload_mismatch &&
           pc::validate_architecture_proposal_shape(invalid_identity, limits)
                   .code ==
               pc::ArchitectureProposalShapeCode::invalid_identity;
}

}  // namespace

int main() {
    return test_empty_valid_and_deterministic_graph() &&
                   test_registration_rejections_and_bounds() &&
                   test_missing_self_cycle_and_type_validation() &&
                   test_edge_and_proposal_shape_validation()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
