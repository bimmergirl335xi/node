#include "architecture_graph.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

namespace prometheus::core {
namespace {

inline constexpr std::size_t kAbsoluteMaximumComponents = 4096;
inline constexpr std::size_t kAbsoluteMaximumEdges = 16384;
inline constexpr std::size_t kAbsoluteMaximumDependencies = 256;
inline constexpr std::size_t kAbsoluteMaximumAdapters = 256;
inline constexpr std::size_t kAbsoluteMaximumContractReferences = 128;
inline constexpr std::size_t kAbsoluteMaximumBackendKinds = 32;
inline constexpr std::size_t kAbsoluteMaximumProposals = 1024;
inline constexpr std::size_t kAbsoluteMaximumEvidence = 4096;
inline constexpr std::size_t kAbsoluteMaximumHistory = 4096;
inline constexpr std::size_t kAbsoluteMaximumIdentifierLength = 256;
inline constexpr std::size_t kAbsoluteMaximumDisplayNameLength = 1024;
inline constexpr std::size_t kAbsoluteMaximumMetadataLength = 4096;

[[nodiscard]] bool valid_limits(const ArchitectureLimits& limits) noexcept {
    return limits.maximum_components > 0 &&
           limits.maximum_components <= kAbsoluteMaximumComponents &&
           limits.maximum_edges > 0 &&
           limits.maximum_edges <= kAbsoluteMaximumEdges &&
           limits.maximum_dependencies_per_component > 0 &&
           limits.maximum_dependencies_per_component <=
               kAbsoluteMaximumDependencies &&
           limits.maximum_adapters_per_component > 0 &&
           limits.maximum_adapters_per_component <= kAbsoluteMaximumAdapters &&
           limits.maximum_contract_references_per_component > 0 &&
           limits.maximum_contract_references_per_component <=
               kAbsoluteMaximumContractReferences &&
           limits.maximum_backend_kinds_per_component > 0 &&
           limits.maximum_backend_kinds_per_component <=
               kAbsoluteMaximumBackendKinds &&
           limits.maximum_proposals_per_transaction > 0 &&
           limits.maximum_proposals_per_transaction <=
               kAbsoluteMaximumProposals &&
           limits.maximum_evidence_entries > 0 &&
           limits.maximum_evidence_entries <= kAbsoluteMaximumEvidence &&
           limits.maximum_history_entries > 0 &&
           limits.maximum_history_entries <= kAbsoluteMaximumHistory &&
           limits.maximum_identifier_length > 0 &&
           limits.maximum_identifier_length <=
               kAbsoluteMaximumIdentifierLength &&
           limits.maximum_display_name_length > 0 &&
           limits.maximum_display_name_length <=
               kAbsoluteMaximumDisplayNameLength &&
           limits.maximum_metadata_length >= 128 &&
           limits.maximum_metadata_length <= kAbsoluteMaximumMetadataLength;
}

[[nodiscard]] bool valid_identifier_list(
    const std::vector<std::string>& values,
    std::size_t maximum_entries,
    const ArchitectureLimits& limits) noexcept {
    if (values.size() > maximum_entries) {
        return false;
    }
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (!is_valid_architecture_identifier(
                values[index], limits.maximum_identifier_length)) {
            return false;
        }
        for (std::size_t other = index + 1; other < values.size(); ++other) {
            if (values[index] == values[other]) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] bool valid_resource_requirement(
    const ArchitectureResourceRequirement& requirement,
    const ArchitectureLimits& limits) noexcept {
    if (requirement.allowed_backend_kinds.size() >
            limits.maximum_backend_kinds_per_component ||
        !valid_identifier_list(
            requirement.operation_adapters,
            limits.maximum_adapters_per_component,
            limits) ||
        requirement.required_memory_bytes >
            std::numeric_limits<std::uint64_t>::max() -
                requirement.reservation_headroom_bytes) {
        return false;
    }
    for (std::size_t index = 0;
         index < requirement.allowed_backend_kinds.size();
         ++index) {
        for (std::size_t other = index + 1;
             other < requirement.allowed_backend_kinds.size();
             ++other) {
            if (requirement.allowed_backend_kinds[index] ==
                requirement.allowed_backend_kinds[other]) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] bool valid_component_descriptor(
    const ArchitectureComponentDescriptor& descriptor,
    const ArchitectureLimits& limits) noexcept {
    if (!is_valid_architecture_identifier(
            descriptor.component_id, limits.maximum_identifier_length) ||
        descriptor.display_name.empty() ||
        descriptor.display_name.size() > limits.maximum_display_name_length ||
        descriptor.dependencies.size() >
            limits.maximum_dependencies_per_component ||
        !std::isfinite(descriptor.weight) || descriptor.weight < 0.0 ||
        descriptor.weight > 1.0 || descriptor.schema_version == 0 ||
        !valid_resource_requirement(descriptor.resource_requirement, limits) ||
        !valid_identifier_list(
            descriptor.memory_contract_references,
            limits.maximum_contract_references_per_component,
            limits) ||
        !valid_identifier_list(
            descriptor.acs_contract_references,
            limits.maximum_contract_references_per_component,
            limits)) {
        return false;
    }
    for (const ArchitectureDependencyRequirement& dependency :
         descriptor.dependencies) {
        if (!is_valid_architecture_identifier(
                dependency.component_id, limits.maximum_identifier_length)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool valid_edge_descriptor(
    const ArchitectureEdgeDescriptor& descriptor,
    const ArchitectureLimits& limits) noexcept {
    return is_valid_architecture_identifier(
               descriptor.edge_id, limits.maximum_identifier_length) &&
           is_valid_architecture_identifier(
               descriptor.source_component_id,
               limits.maximum_identifier_length) &&
           is_valid_architecture_identifier(
               descriptor.target_component_id,
               limits.maximum_identifier_length) &&
           descriptor.source_component_id != descriptor.target_component_id &&
           descriptor.edge_type != ArchitectureEdgeType::unknown &&
           std::isfinite(descriptor.weight) && descriptor.weight >= 0.0 &&
           descriptor.weight <= 1.0 && descriptor.schema_version != 0;
}

template <typename Value>
[[nodiscard]] const Value* find_by_id(
    const std::vector<Value>& values,
    const std::string& identity) noexcept;

template <>
const ArchitectureComponentDescriptor* find_by_id(
    const std::vector<ArchitectureComponentDescriptor>& values,
    const std::string& identity) noexcept {
    const auto found = std::find_if(
        values.begin(), values.end(), [&identity](const auto& value) {
            return value.component_id == identity;
        });
    return found == values.end() ? nullptr : &*found;
}

template <>
const ArchitectureEdgeDescriptor* find_by_id(
    const std::vector<ArchitectureEdgeDescriptor>& values,
    const std::string& identity) noexcept {
    const auto found = std::find_if(
        values.begin(), values.end(), [&identity](const auto& value) {
            return value.edge_id == identity;
        });
    return found == values.end() ? nullptr : &*found;
}

void add_issue(
    ArchitectureGraphValidationResult& result,
    const ArchitectureLimits& limits,
    ArchitectureGraphIssueCode code,
    std::string subject,
    std::string related,
    std::string message) {
    if (result.issues.size() < limits.maximum_evidence_entries) {
        result.issues.push_back(
            {code,
             std::move(subject),
             std::move(related),
             std::move(message)});
    }
}

[[nodiscard]] bool no_payload(
    const ArchitectureProposalPayload& payload) noexcept {
    return std::holds_alternative<ArchitectureNoPayload>(payload);
}

[[nodiscard]] const char* registration_message(
    ArchitectureRegistrationCode code) noexcept {
    switch (code) {
        case ArchitectureRegistrationCode::success:
            return "Architecture descriptor registered";
        case ArchitectureRegistrationCode::invalid_configuration:
            return "Architecture limits or schema version are invalid";
        case ArchitectureRegistrationCode::invalid_identifier:
            return "Architecture identity is invalid";
        case ArchitectureRegistrationCode::invalid_descriptor:
            return "Architecture descriptor is invalid";
        case ArchitectureRegistrationCode::duplicate_identifier:
            return "Architecture identity is already registered";
        case ArchitectureRegistrationCode::capacity_exhausted:
            return "Architecture graph capacity is exhausted";
        case ArchitectureRegistrationCode::resource_exhausted:
            return "Architecture registration exhausted memory";
    }
    return "Architecture registration failed";
}

}  // namespace

ArchitectureGraph::ArchitectureGraph(
    ArchitectureLimits limits,
    std::uint64_t schema_version)
    : limits_(limits),
      limits_valid_(valid_limits(limits) && schema_version != 0),
      schema_version_(schema_version) {}

ArchitectureRegistrationResult ArchitectureGraph::register_component(
    ArchitectureComponentDescriptor descriptor) {
    try {
        if (!limits_valid_) {
            return {ArchitectureRegistrationCode::invalid_configuration,
                    descriptor.component_id,
                    registration_message(
                        ArchitectureRegistrationCode::invalid_configuration)};
        }
        if (!is_valid_architecture_identifier(
                descriptor.component_id, limits_.maximum_identifier_length)) {
            return {ArchitectureRegistrationCode::invalid_identifier,
                    descriptor.component_id,
                    registration_message(
                        ArchitectureRegistrationCode::invalid_identifier)};
        }
        if (!valid_component_descriptor(descriptor, limits_)) {
            return {ArchitectureRegistrationCode::invalid_descriptor,
                    descriptor.component_id,
                    registration_message(
                        ArchitectureRegistrationCode::invalid_descriptor)};
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if (find_by_id(components_, descriptor.component_id) != nullptr) {
            return {ArchitectureRegistrationCode::duplicate_identifier,
                    descriptor.component_id,
                    registration_message(
                        ArchitectureRegistrationCode::duplicate_identifier)};
        }
        if (components_.size() >= limits_.maximum_components ||
            graph_revision_ == std::numeric_limits<std::uint64_t>::max()) {
            return {ArchitectureRegistrationCode::capacity_exhausted,
                    descriptor.component_id,
                    registration_message(
                        ArchitectureRegistrationCode::capacity_exhausted)};
        }
        const std::string identity = descriptor.component_id;
        components_.push_back(std::move(descriptor));
        ++graph_revision_;
        return {ArchitectureRegistrationCode::success,
                identity,
                registration_message(ArchitectureRegistrationCode::success)};
    } catch (const std::bad_alloc&) {
        return {ArchitectureRegistrationCode::resource_exhausted,
                {},
                registration_message(
                    ArchitectureRegistrationCode::resource_exhausted)};
    } catch (...) {
        return {ArchitectureRegistrationCode::invalid_descriptor,
                {},
                "Architecture component registration failed safely"};
    }
}

ArchitectureRegistrationResult ArchitectureGraph::register_edge(
    ArchitectureEdgeDescriptor descriptor) {
    try {
        if (!limits_valid_) {
            return {ArchitectureRegistrationCode::invalid_configuration,
                    descriptor.edge_id,
                    registration_message(
                        ArchitectureRegistrationCode::invalid_configuration)};
        }
        if (!is_valid_architecture_identifier(
                descriptor.edge_id, limits_.maximum_identifier_length)) {
            return {ArchitectureRegistrationCode::invalid_identifier,
                    descriptor.edge_id,
                    registration_message(
                        ArchitectureRegistrationCode::invalid_identifier)};
        }
        if (!valid_edge_descriptor(descriptor, limits_)) {
            return {ArchitectureRegistrationCode::invalid_descriptor,
                    descriptor.edge_id,
                    registration_message(
                        ArchitectureRegistrationCode::invalid_descriptor)};
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if (find_by_id(edges_, descriptor.edge_id) != nullptr) {
            return {ArchitectureRegistrationCode::duplicate_identifier,
                    descriptor.edge_id,
                    registration_message(
                        ArchitectureRegistrationCode::duplicate_identifier)};
        }
        if (find_by_id(components_, descriptor.source_component_id) == nullptr ||
            find_by_id(components_, descriptor.target_component_id) == nullptr) {
            return {ArchitectureRegistrationCode::invalid_descriptor,
                    descriptor.edge_id,
                    "Architecture edge endpoint is not registered"};
        }
        if (edges_.size() >= limits_.maximum_edges ||
            graph_revision_ == std::numeric_limits<std::uint64_t>::max()) {
            return {ArchitectureRegistrationCode::capacity_exhausted,
                    descriptor.edge_id,
                    registration_message(
                        ArchitectureRegistrationCode::capacity_exhausted)};
        }
        const std::string identity = descriptor.edge_id;
        edges_.push_back(std::move(descriptor));
        ++graph_revision_;
        return {ArchitectureRegistrationCode::success,
                identity,
                registration_message(ArchitectureRegistrationCode::success)};
    } catch (const std::bad_alloc&) {
        return {ArchitectureRegistrationCode::resource_exhausted,
                {},
                registration_message(
                    ArchitectureRegistrationCode::resource_exhausted)};
    } catch (...) {
        return {ArchitectureRegistrationCode::invalid_descriptor,
                {},
                "Architecture edge registration failed safely"};
    }
}

ArchitectureGraphSnapshot ArchitectureGraph::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    ArchitectureGraphSnapshot result{};
    result.graph_revision = graph_revision_;
    result.schema_version = schema_version_;
    result.components = components_;
    result.edges = edges_;
    for (ArchitectureComponentDescriptor& component : result.components) {
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
        result.components.begin(),
        result.components.end(),
        [](const auto& left, const auto& right) {
            return left.component_id < right.component_id;
        });
    std::sort(
        result.edges.begin(),
        result.edges.end(),
        [](const auto& left, const auto& right) {
            return left.edge_id < right.edge_id;
        });
    return result;
}

ArchitectureGraphValidationResult ArchitectureGraph::validate() const {
    try {
        return validate_architecture_graph(snapshot(), limits_);
    } catch (const std::bad_alloc&) {
        ArchitectureGraphValidationResult result{};
        result.code = ArchitectureGraphValidationCode::resource_exhausted;
        return result;
    }
}

std::uint64_t ArchitectureGraph::revision() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return graph_revision_;
}

std::size_t ArchitectureGraph::component_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return components_.size();
}

std::size_t ArchitectureGraph::edge_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return edges_.size();
}

const ArchitectureLimits& ArchitectureGraph::limits() const noexcept {
    return limits_;
}

bool is_valid_architecture_identifier(
    const std::string& value,
    std::size_t maximum_length) noexcept {
    if (value.empty() || value.size() > maximum_length ||
        !std::isalnum(static_cast<unsigned char>(value.front()))) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](const char character) {
        const unsigned char byte = static_cast<unsigned char>(character);
        return std::isalnum(byte) || character == '.' || character == '_' ||
               character == '-';
    });
}

ArchitectureGraphValidationResult validate_architecture_graph(
    const ArchitectureGraphSnapshot& graph,
    const ArchitectureLimits& limits) {
    ArchitectureGraphValidationResult result{};
    if (!valid_limits(limits) || graph.schema_version == 0 ||
        graph.components.size() > limits.maximum_components ||
        graph.edges.size() > limits.maximum_edges) {
        result.code = ArchitectureGraphValidationCode::resource_exhausted;
        return result;
    }

    bool invalid = false;
    bool unknown = false;
    for (std::size_t index = 0; index < graph.components.size(); ++index) {
        const ArchitectureComponentDescriptor& component =
            graph.components[index];
        if (!valid_component_descriptor(component, limits)) {
            invalid = true;
            add_issue(result,
                      limits,
                      ArchitectureGraphIssueCode::invalid_component,
                      component.component_id,
                      {},
                      "Component descriptor is invalid");
        }
        if (component.component_type == ArchitectureComponentType::unknown) {
            unknown = true;
        }
        for (std::size_t other = index + 1;
             other < graph.components.size();
             ++other) {
            if (component.component_id ==
                graph.components[other].component_id) {
                invalid = true;
                add_issue(result,
                          limits,
                          ArchitectureGraphIssueCode::duplicate_component,
                          component.component_id,
                          {},
                          "Component identity is duplicated");
            }
        }
    }

    std::vector<std::size_t> indegree(graph.components.size(), 0);
    std::vector<std::vector<std::size_t>> dependents(graph.components.size());
    for (std::size_t index = 0; index < graph.components.size(); ++index) {
        const ArchitectureComponentDescriptor& component =
            graph.components[index];
        for (std::size_t dependency_index = 0;
             dependency_index < component.dependencies.size();
             ++dependency_index) {
            const ArchitectureDependencyRequirement& dependency =
                component.dependencies[dependency_index];
            if (dependency.component_id == component.component_id) {
                invalid = true;
                add_issue(result,
                          limits,
                          ArchitectureGraphIssueCode::self_dependency,
                          component.component_id,
                          dependency.component_id,
                          "Component cannot depend on itself");
                continue;
            }
            for (std::size_t other = dependency_index + 1;
                 other < component.dependencies.size();
                 ++other) {
                if (dependency.component_id ==
                    component.dependencies[other].component_id) {
                    invalid = true;
                    add_issue(result,
                              limits,
                              ArchitectureGraphIssueCode::invalid_component,
                              component.component_id,
                              dependency.component_id,
                              "Dependency identity is duplicated");
                }
            }
            const auto dependency_component = std::find_if(
                graph.components.begin(),
                graph.components.end(),
                [&dependency](const auto& candidate) {
                    return candidate.component_id == dependency.component_id;
                });
            if (dependency_component == graph.components.end()) {
                invalid = true;
                add_issue(result,
                          limits,
                          ArchitectureGraphIssueCode::missing_dependency,
                          component.component_id,
                          dependency.component_id,
                          "Required dependency is absent");
                continue;
            }
            const std::size_t resolved_index =
                static_cast<std::size_t>(std::distance(
                    graph.components.begin(), dependency_component));
            ++indegree[index];
            dependents[resolved_index].push_back(index);
            if (dependency.required_type == ArchitectureComponentType::unknown ||
                dependency_component->component_type ==
                    ArchitectureComponentType::unknown) {
                unknown = true;
                add_issue(result,
                          limits,
                          ArchitectureGraphIssueCode::dependency_type_unknown,
                          component.component_id,
                          dependency.component_id,
                          "Dependency type compatibility is unknown");
            } else if (dependency.required_type !=
                       dependency_component->component_type) {
                invalid = true;
                add_issue(result,
                          limits,
                          ArchitectureGraphIssueCode::dependency_type_mismatch,
                          component.component_id,
                          dependency.component_id,
                          "Dependency component type is incompatible");
            }
            if (component.enabled && !dependency_component->enabled) {
                invalid = true;
                add_issue(result,
                          limits,
                          ArchitectureGraphIssueCode::disabled_required_dependency,
                          component.component_id,
                          dependency.component_id,
                          "Enabled component requires a disabled dependency");
            }
        }
    }

    std::vector<bool> emitted(graph.components.size(), false);
    for (std::size_t emitted_count = 0;
         emitted_count < graph.components.size();
         ++emitted_count) {
        std::optional<std::size_t> selected{};
        for (std::size_t index = 0; index < graph.components.size(); ++index) {
            if (!emitted[index] && indegree[index] == 0 &&
                (!selected.has_value() ||
                 graph.components[index].component_id <
                     graph.components[*selected].component_id)) {
                selected = index;
            }
        }
        if (!selected.has_value()) {
            invalid = true;
            add_issue(result,
                      limits,
                      ArchitectureGraphIssueCode::dependency_cycle,
                      {},
                      {},
                      "Dependency graph contains a cycle");
            break;
        }
        emitted[*selected] = true;
        result.dependency_order.push_back(
            graph.components[*selected].component_id);
        for (const std::size_t dependent : dependents[*selected]) {
            if (indegree[dependent] > 0) {
                --indegree[dependent];
            }
        }
    }

    for (std::size_t index = 0; index < graph.edges.size(); ++index) {
        const ArchitectureEdgeDescriptor& edge = graph.edges[index];
        if (!valid_edge_descriptor(edge, limits)) {
            invalid = true;
            add_issue(result,
                      limits,
                      ArchitectureGraphIssueCode::invalid_edge,
                      edge.edge_id,
                      {},
                      "Edge descriptor is invalid");
        }
        if (find_by_id(graph.components, edge.source_component_id) == nullptr ||
            find_by_id(graph.components, edge.target_component_id) == nullptr) {
            invalid = true;
            add_issue(result,
                      limits,
                      ArchitectureGraphIssueCode::missing_edge_endpoint,
                      edge.edge_id,
                      {},
                      "Edge endpoint is absent");
        }
        for (std::size_t other = index + 1; other < graph.edges.size(); ++other) {
            const ArchitectureEdgeDescriptor& candidate = graph.edges[other];
            if (edge.edge_id == candidate.edge_id) {
                invalid = true;
                add_issue(result,
                          limits,
                          ArchitectureGraphIssueCode::duplicate_edge,
                          edge.edge_id,
                          {},
                          "Edge identity is duplicated");
            } else if (edge.source_component_id ==
                           candidate.source_component_id &&
                       edge.target_component_id ==
                           candidate.target_component_id &&
                       edge.edge_type == candidate.edge_type) {
                invalid = true;
                add_issue(result,
                          limits,
                          ArchitectureGraphIssueCode::duplicate_semantic_edge,
                          edge.edge_id,
                          candidate.edge_id,
                          "Equivalent logical edge is duplicated");
            }
        }
    }

    result.code = invalid ? ArchitectureGraphValidationCode::invalid
                          : unknown ? ArchitectureGraphValidationCode::unknown
                                    : ArchitectureGraphValidationCode::valid;
    return result;
}

ArchitectureProposalShapeResult validate_architecture_proposal_shape(
    const ArchitectureProposal& proposal,
    const ArchitectureLimits& limits) {
    if (!valid_limits(limits)) {
        return {ArchitectureProposalShapeCode::resource_exhausted,
                "Architecture limits are invalid"};
    }
    if (!is_valid_architecture_identifier(
            proposal.proposal_id, limits.maximum_identifier_length) ||
        !is_valid_architecture_identifier(
            proposal.source_identity, limits.maximum_identifier_length) ||
        !is_valid_architecture_identifier(
            proposal.proposal_authority_reference,
            limits.maximum_identifier_length) ||
        !is_valid_architecture_identifier(
            proposal.target_identity, limits.maximum_identifier_length) ||
        (!proposal.correlation_id.empty() &&
         !is_valid_architecture_identifier(
             proposal.correlation_id, limits.maximum_identifier_length)) ||
        (!proposal.evidence_reference.empty() &&
         !is_valid_architecture_identifier(
             proposal.evidence_reference, limits.maximum_identifier_length))) {
        return {ArchitectureProposalShapeCode::invalid_identity,
                "Proposal identities are invalid"};
    }
    if (proposal.source_category == ArchitectureProposalSourceCategory::unknown ||
        proposal.explanation.size() > limits.maximum_metadata_length) {
        return {ArchitectureProposalShapeCode::invalid_metadata,
                "Proposal source or explanatory metadata is invalid"};
    }

    bool matches = false;
    bool payload_valid = true;
    switch (proposal.operation) {
        case ArchitectureProposalOperation::add_component: {
            const auto* payload =
                std::get_if<ArchitectureAddComponentPayload>(&proposal.payload);
            matches = payload != nullptr;
            payload_valid = payload != nullptr &&
                            payload->component.component_id ==
                                proposal.target_identity &&
                            valid_component_descriptor(
                                payload->component, limits);
            break;
        }
        case ArchitectureProposalOperation::remove_component:
        case ArchitectureProposalOperation::enable_component:
        case ArchitectureProposalOperation::disable_component:
        case ArchitectureProposalOperation::remove_edge:
            matches = no_payload(proposal.payload);
            break;
        case ArchitectureProposalOperation::replace_dependency: {
            const auto* payload =
                std::get_if<ArchitectureReplaceDependencyPayload>(
                    &proposal.payload);
            matches = payload != nullptr;
            payload_valid = payload != nullptr &&
                            is_valid_architecture_identifier(
                                payload->old_dependency_id,
                                limits.maximum_identifier_length) &&
                            is_valid_architecture_identifier(
                                payload->new_dependency.component_id,
                                limits.maximum_identifier_length);
            break;
        }
        case ArchitectureProposalOperation::add_edge: {
            const auto* payload =
                std::get_if<ArchitectureAddEdgePayload>(&proposal.payload);
            matches = payload != nullptr;
            payload_valid = payload != nullptr &&
                            payload->edge.edge_id == proposal.target_identity &&
                            valid_edge_descriptor(payload->edge, limits);
            break;
        }
        case ArchitectureProposalOperation::adjust_weight: {
            const auto* payload =
                std::get_if<ArchitectureAdjustWeightPayload>(&proposal.payload);
            matches = payload != nullptr;
            payload_valid = payload != nullptr &&
                            payload->target != ArchitectureWeightTarget::unknown &&
                            std::isfinite(payload->weight) &&
                            payload->weight >= 0.0 && payload->weight <= 1.0;
            break;
        }
        case ArchitectureProposalOperation::replace_resource_requirement: {
            const auto* payload =
                std::get_if<ArchitectureReplaceResourcePayload>(
                    &proposal.payload);
            matches = payload != nullptr;
            payload_valid = payload != nullptr &&
                            valid_resource_requirement(
                                payload->requirement, limits);
            break;
        }
        case ArchitectureProposalOperation::unknown:
            return {ArchitectureProposalShapeCode::invalid_operation,
                    "Architecture proposal operation is unknown"};
    }
    if (!matches) {
        return {ArchitectureProposalShapeCode::payload_mismatch,
                "Proposal payload does not match its operation"};
    }
    if (!payload_valid) {
        return {ArchitectureProposalShapeCode::invalid_metadata,
                "Proposal payload metadata is invalid"};
    }
    return {ArchitectureProposalShapeCode::valid,
            "Architecture proposal shape is valid"};
}

const char* to_string(ArchitectureComponentType value) noexcept {
    switch (value) {
        case ArchitectureComponentType::service: return "service";
        case ArchitectureComponentType::backend_adapter: return "backend_adapter";
        case ArchitectureComponentType::adaptive_state: return "adaptive_state";
        case ArchitectureComponentType::memory_interface: return "memory_interface";
        case ArchitectureComponentType::communication_interface:
            return "communication_interface";
        case ArchitectureComponentType::compute_stage: return "compute_stage";
        case ArchitectureComponentType::control_stage: return "control_stage";
        case ArchitectureComponentType::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(ArchitectureEdgeType value) noexcept {
    switch (value) {
        case ArchitectureEdgeType::data_flow: return "data_flow";
        case ArchitectureEdgeType::control_flow: return "control_flow";
        case ArchitectureEdgeType::evidence_flow: return "evidence_flow";
        case ArchitectureEdgeType::logical_affinity: return "logical_affinity";
        case ArchitectureEdgeType::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(ArchitectureRegistrationCode value) noexcept {
    switch (value) {
        case ArchitectureRegistrationCode::success: return "success";
        case ArchitectureRegistrationCode::invalid_configuration:
            return "invalid_configuration";
        case ArchitectureRegistrationCode::invalid_identifier:
            return "invalid_identifier";
        case ArchitectureRegistrationCode::invalid_descriptor:
            return "invalid_descriptor";
        case ArchitectureRegistrationCode::duplicate_identifier:
            return "duplicate_identifier";
        case ArchitectureRegistrationCode::capacity_exhausted:
            return "capacity_exhausted";
        case ArchitectureRegistrationCode::resource_exhausted:
            return "resource_exhausted";
    }
    return "unknown";
}

const char* to_string(ArchitectureGraphValidationCode value) noexcept {
    switch (value) {
        case ArchitectureGraphValidationCode::valid: return "valid";
        case ArchitectureGraphValidationCode::invalid: return "invalid";
        case ArchitectureGraphValidationCode::unknown: return "unknown";
        case ArchitectureGraphValidationCode::resource_exhausted:
            return "resource_exhausted";
    }
    return "unknown";
}

const char* to_string(ArchitectureGraphIssueCode value) noexcept {
    switch (value) {
        case ArchitectureGraphIssueCode::none: return "none";
        case ArchitectureGraphIssueCode::invalid_component: return "invalid_component";
        case ArchitectureGraphIssueCode::invalid_edge: return "invalid_edge";
        case ArchitectureGraphIssueCode::duplicate_component: return "duplicate_component";
        case ArchitectureGraphIssueCode::duplicate_edge: return "duplicate_edge";
        case ArchitectureGraphIssueCode::missing_dependency: return "missing_dependency";
        case ArchitectureGraphIssueCode::self_dependency: return "self_dependency";
        case ArchitectureGraphIssueCode::dependency_cycle: return "dependency_cycle";
        case ArchitectureGraphIssueCode::dependency_type_mismatch:
            return "dependency_type_mismatch";
        case ArchitectureGraphIssueCode::dependency_type_unknown:
            return "dependency_type_unknown";
        case ArchitectureGraphIssueCode::disabled_required_dependency:
            return "disabled_required_dependency";
        case ArchitectureGraphIssueCode::missing_edge_endpoint:
            return "missing_edge_endpoint";
        case ArchitectureGraphIssueCode::duplicate_semantic_edge:
            return "duplicate_semantic_edge";
        case ArchitectureGraphIssueCode::capacity_exceeded: return "capacity_exceeded";
    }
    return "unknown";
}

const char* to_string(ArchitectureProposalSourceCategory value) noexcept {
    switch (value) {
        case ArchitectureProposalSourceCategory::automatic_internal:
            return "automatic_internal";
        case ArchitectureProposalSourceCategory::externally_authorized:
            return "externally_authorized";
        case ArchitectureProposalSourceCategory::operator_directed:
            return "operator_directed";
        case ArchitectureProposalSourceCategory::recovery: return "recovery";
        case ArchitectureProposalSourceCategory::system_policy:
            return "system_policy";
        case ArchitectureProposalSourceCategory::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(ArchitectureProposalOperation value) noexcept {
    switch (value) {
        case ArchitectureProposalOperation::add_component: return "add_component";
        case ArchitectureProposalOperation::remove_component:
            return "remove_component";
        case ArchitectureProposalOperation::enable_component:
            return "enable_component";
        case ArchitectureProposalOperation::disable_component:
            return "disable_component";
        case ArchitectureProposalOperation::replace_dependency:
            return "replace_dependency";
        case ArchitectureProposalOperation::add_edge: return "add_edge";
        case ArchitectureProposalOperation::remove_edge: return "remove_edge";
        case ArchitectureProposalOperation::adjust_weight: return "adjust_weight";
        case ArchitectureProposalOperation::replace_resource_requirement:
            return "replace_resource_requirement";
        case ArchitectureProposalOperation::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(ArchitectureProposalShapeCode value) noexcept {
    switch (value) {
        case ArchitectureProposalShapeCode::valid: return "valid";
        case ArchitectureProposalShapeCode::invalid_identity:
            return "invalid_identity";
        case ArchitectureProposalShapeCode::invalid_metadata:
            return "invalid_metadata";
        case ArchitectureProposalShapeCode::invalid_operation:
            return "invalid_operation";
        case ArchitectureProposalShapeCode::payload_mismatch:
            return "payload_mismatch";
        case ArchitectureProposalShapeCode::resource_exhausted:
            return "resource_exhausted";
    }
    return "unknown";
}

}  // namespace prometheus::core
