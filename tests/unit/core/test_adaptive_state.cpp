#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/adaptive_state.hpp"

namespace pc = prometheus::core;

namespace {

[[nodiscard]] pc::AdaptiveVariableDescriptor integer_descriptor(
    std::string id,
    std::int64_t initial = 10) {
    pc::AdaptiveVariableDescriptor descriptor{};
    descriptor.variable_id = std::move(id);
    descriptor.display_name = "Integer variable";
    descriptor.description = "Bounded signed runtime state";
    descriptor.value_type = pc::AdaptiveValueType::signed_integer;
    descriptor.default_value = initial;
    descriptor.hard_bounds.enabled = true;
    descriptor.hard_bounds.minimum = std::int64_t{-100};
    descriptor.hard_bounds.maximum = std::int64_t{100};
    descriptor.soft_bounds.enabled = true;
    descriptor.soft_bounds.minimum = std::int64_t{-50};
    descriptor.soft_bounds.maximum = std::int64_t{50};
    descriptor.allow_automatic_mutation = true;
    descriptor.allow_external_mutation = false;
    descriptor.maximum_mutation_magnitude = 10.0;
    descriptor.persistence = pc::AdaptivePersistenceClass::checkpoint_eligible;
    descriptor.sensitivity = pc::AdaptiveSensitivity::internal;
    return descriptor;
}

[[nodiscard]] pc::AdaptiveVariableDescriptor vector_descriptor(
    std::string id) {
    pc::AdaptiveVariableDescriptor descriptor{};
    descriptor.variable_id = std::move(id);
    descriptor.display_name = "Vector variable";
    descriptor.value_type = pc::AdaptiveValueType::floating_point_vector;
    descriptor.shape = {2, 2};
    descriptor.default_value = std::vector<double>{1.0, 2.0, 3.0, 4.0};
    descriptor.hard_bounds.enabled = true;
    descriptor.hard_bounds.minimum = -10.0;
    descriptor.hard_bounds.maximum = 10.0;
    descriptor.allow_external_mutation = true;
    return descriptor;
}

[[nodiscard]] pc::AdaptiveValidationAuthority validation_authority() {
    pc::AdaptiveValidationAuthority authority{};
    authority.decision = pc::AdaptiveAuthorityDecision::authorized;
    authority.authority_identity = "runtime.validator";
    authority.evidence_reference = "evidence.validated";
    return authority;
}

[[nodiscard]] pc::AdaptiveMutationProposal proposal(
    std::string id,
    std::string target,
    pc::AdaptiveValue operand,
    pc::AdaptiveMutationOperation operation =
        pc::AdaptiveMutationOperation::replace,
    pc::AdaptiveMutationSourceCategory category =
        pc::AdaptiveMutationSourceCategory::automatic_internal) {
    pc::AdaptiveMutationProposal result{};
    result.proposal_id = std::move(id);
    result.target_variable_id = std::move(target);
    result.source_identity = "adaptive.controller";
    result.source_category = category;
    result.proposal_authority.decision =
        pc::AdaptiveAuthorityDecision::authorized;
    result.proposal_authority.authority_identity = "runtime.policy";
    result.correlation_id = "correlation.one";
    result.expected_variable_version = 1;
    result.operation = operation;
    result.operand = std::move(operand);
    return result;
}

[[nodiscard]] bool test_empty_and_registration() {
    static_assert(!std::is_copy_constructible_v<pc::AdaptiveStateRegistry>);
    static_assert(!std::is_move_constructible_v<pc::AdaptiveStateRegistry>);

    pc::AdaptiveStateRegistry registry{};
    if (!registry.status().usable() || registry.size() != 0 ||
        registry.revision() != 0 || !registry.snapshot().variables.empty() ||
        registry.find("missing").has_value()) {
        return false;
    }

    pc::AdaptiveVariableDescriptor descriptor = integer_descriptor("state.zeta");
    const pc::AdaptiveVariableDescriptor original = descriptor;
    if (!registry.register_variable(descriptor).ok() || registry.size() != 1 ||
        registry.revision() != 1) {
        return false;
    }
    descriptor.hard_bounds.maximum = std::int64_t{1000};
    const auto snapshot = registry.find("state.zeta");
    return snapshot.has_value() && snapshot->value.has_value() &&
           std::get<std::int64_t>(*snapshot->value) == 10 &&
           std::get<std::int64_t>(snapshot->descriptor.hard_bounds.maximum) ==
               std::get<std::int64_t>(original.hard_bounds.maximum) &&
           snapshot->version == 1 &&
           snapshot->last_update_category ==
               pc::AdaptiveUpdateCategory::initial_registration;
}

[[nodiscard]] bool test_rejections_and_capacity() {
    pc::AdaptiveStateLimits limits{};
    limits.maximum_variables = 1;
    pc::AdaptiveStateRegistry registry{limits};

    pc::AdaptiveVariableDescriptor invalid_id = integer_descriptor("bad id");
    if (registry.register_variable(invalid_id).code !=
        pc::AdaptiveRegistrationCode::invalid_identifier) {
        return false;
    }
    pc::AdaptiveVariableDescriptor invalid_descriptor =
        integer_descriptor("invalid.bounds");
    invalid_descriptor.hard_bounds.minimum = std::int64_t{100};
    invalid_descriptor.hard_bounds.maximum = std::int64_t{-100};
    if (registry.register_variable(invalid_descriptor).code !=
        pc::AdaptiveRegistrationCode::invalid_descriptor) {
        return false;
    }
    pc::AdaptiveVariableDescriptor bad_shape = vector_descriptor("bad.shape");
    bad_shape.shape = {3};
    if (registry.register_variable(bad_shape).code !=
        pc::AdaptiveRegistrationCode::invalid_descriptor) {
        return false;
    }

    const auto first = integer_descriptor("valid.one");
    if (!registry.register_variable(first).ok() ||
        registry.register_variable(first).code !=
            pc::AdaptiveRegistrationCode::duplicate_identifier ||
        registry.register_variable(integer_descriptor("valid.two")).code !=
            pc::AdaptiveRegistrationCode::capacity_exhausted) {
        return false;
    }

    pc::AdaptiveStateLimits invalid_limits{};
    invalid_limits.maximum_variables = 0;
    pc::AdaptiveStateRegistry invalid_registry{invalid_limits};
    return !invalid_registry.status().usable() &&
           invalid_registry.register_variable(integer_descriptor("blocked")).code ==
               pc::AdaptiveRegistrationCode::invalid_configuration;
}

[[nodiscard]] bool test_ordering_shape_and_redaction() {
    pc::AdaptiveStateRegistry registry{};
    pc::AdaptiveVariableDescriptor restricted = integer_descriptor("state.b");
    restricted.sensitivity = pc::AdaptiveSensitivity::restricted;
    if (!registry.register_variable(restricted).ok() ||
        !registry.register_variable(integer_descriptor("state.a")).ok() ||
        !registry.register_variable(vector_descriptor("state.c")).ok()) {
        return false;
    }

    const pc::AdaptiveRegistrySnapshot ordinary = registry.snapshot();
    if (ordinary.variables.size() != 3 ||
        ordinary.variables[0].descriptor.variable_id != "state.a" ||
        ordinary.variables[1].descriptor.variable_id != "state.b" ||
        ordinary.variables[2].descriptor.variable_id != "state.c" ||
        ordinary.variables[1].value.has_value() ||
        !ordinary.variables[1].value_redacted ||
        std::get<std::int64_t>(
            ordinary.variables[1].descriptor.default_value) != 0 ||
        !ordinary.variables[2].value.has_value()) {
        return false;
    }
    const auto explicit_restricted = registry.find(
        "state.b", pc::AdaptiveSnapshotMode::include_all_values);
    const auto metadata = registry.snapshot(
        pc::AdaptiveSnapshotMode::metadata_only);
    return explicit_restricted.has_value() &&
           explicit_restricted->value.has_value() &&
           metadata.variables[0].value_redacted &&
           !metadata.variables[0].value.has_value();
}

[[nodiscard]] bool test_scalar_validation() {
    pc::AdaptiveStateRegistry registry{};

    pc::AdaptiveVariableDescriptor boolean{};
    boolean.variable_id = "scalar.boolean";
    boolean.display_name = "Boolean";
    boolean.value_type = pc::AdaptiveValueType::boolean;
    boolean.default_value = false;
    boolean.allow_external_mutation = true;

    pc::AdaptiveVariableDescriptor integer =
        integer_descriptor("scalar.integer");
    pc::AdaptiveVariableDescriptor floating{};
    floating.variable_id = "scalar.float";
    floating.display_name = "Floating point";
    floating.value_type = pc::AdaptiveValueType::floating_point;
    floating.default_value = 1.5;
    floating.hard_bounds = {true, -100.0, 100.0};
    floating.allow_automatic_mutation = true;

    pc::AdaptiveVariableDescriptor disabled =
        integer_descriptor("scalar.disabled");
    disabled.enabled = false;

    pc::AdaptiveVariableDescriptor overflow =
        integer_descriptor("scalar.overflow");
    overflow.default_value = std::numeric_limits<std::int64_t>::max();
    overflow.hard_bounds.minimum =
        std::numeric_limits<std::int64_t>::min();
    overflow.hard_bounds.maximum =
        std::numeric_limits<std::int64_t>::max();
    overflow.soft_bounds.enabled = false;
    overflow.maximum_mutation_magnitude.reset();

    if (!registry.register_variable(boolean).ok() ||
        !registry.register_variable(integer).ok() ||
        !registry.register_variable(floating).ok() ||
        !registry.register_variable(disabled).ok() ||
        !registry.register_variable(overflow).ok()) {
        return false;
    }

    const auto validator = validation_authority();
    const auto boolean_result = registry.validate_mutation(
        proposal(
            "proposal.boolean",
            "scalar.boolean",
            true,
            pc::AdaptiveMutationOperation::replace,
            pc::AdaptiveMutationSourceCategory::externally_authorized),
        validator);
    const auto integer_result = registry.validate_mutation(
        proposal(
            "proposal.integer",
            "scalar.integer",
            std::int64_t{5},
            pc::AdaptiveMutationOperation::add),
        validator);
    const auto overflow_result = registry.validate_mutation(
        proposal(
            "proposal.overflow",
            "scalar.overflow",
            std::int64_t{1},
            pc::AdaptiveMutationOperation::add),
        validator);
    const auto float_result = registry.validate_mutation(
        proposal("proposal.float", "scalar.float", 2.5), validator);
    const auto nan_result = registry.validate_mutation(
        proposal(
            "proposal.nan",
            "scalar.float",
            std::numeric_limits<double>::quiet_NaN()),
        validator);
    const auto infinity_result = registry.validate_mutation(
        proposal(
            "proposal.infinity",
            "scalar.float",
            std::numeric_limits<double>::infinity()),
        validator);
    const auto hard_result = registry.validate_mutation(
        proposal(
            "proposal.hard",
            "scalar.integer",
            std::int64_t{100},
            pc::AdaptiveMutationOperation::add),
        validator);
    const auto soft_result = registry.validate_mutation(
        proposal(
            "proposal.soft",
            "scalar.integer",
            std::int64_t{45},
            pc::AdaptiveMutationOperation::add),
        validator);
    const auto magnitude_result = registry.validate_mutation(
        proposal(
            "proposal.magnitude",
            "scalar.integer",
            std::int64_t{11},
            pc::AdaptiveMutationOperation::add),
        validator);
    const auto disabled_result = registry.validate_mutation(
        proposal(
            "proposal.disabled",
            "scalar.disabled",
            std::int64_t{11}),
        validator);

    const auto unchanged = registry.find("scalar.integer");
    return boolean_result.accepted() && integer_result.accepted() &&
           overflow_result.code ==
               pc::AdaptiveMutationValidationCode::arithmetic_overflow &&
           float_result.accepted() &&
           nan_result.code == pc::AdaptiveMutationValidationCode::out_of_bounds &&
           infinity_result.code ==
               pc::AdaptiveMutationValidationCode::out_of_bounds &&
           hard_result.code == pc::AdaptiveMutationValidationCode::out_of_bounds &&
           soft_result.code ==
               pc::AdaptiveMutationValidationCode::soft_bound_violation &&
           magnitude_result.code ==
               pc::AdaptiveMutationValidationCode::magnitude_exceeded &&
           disabled_result.code == pc::AdaptiveMutationValidationCode::disabled &&
           unchanged.has_value() && unchanged->version == 1 &&
           std::get<std::int64_t>(*unchanged->value) == 10;
}

[[nodiscard]] bool test_vector_validation() {
    pc::AdaptiveStateLimits limits{};
    limits.maximum_vector_elements = 4;
    pc::AdaptiveStateRegistry registry{limits};
    if (!registry.register_variable(vector_descriptor("vector.float")).ok()) {
        return false;
    }
    const auto validator = validation_authority();
    const auto valid = registry.validate_mutation(
        proposal(
            "proposal.vector.valid",
            "vector.float",
            std::vector<double>{2.0, 3.0, 4.0, 5.0},
            pc::AdaptiveMutationOperation::replace,
            pc::AdaptiveMutationSourceCategory::externally_authorized),
        validator);
    const auto shape = registry.validate_mutation(
        proposal(
            "proposal.vector.shape",
            "vector.float",
            std::vector<double>{1.0, 2.0},
            pc::AdaptiveMutationOperation::replace,
            pc::AdaptiveMutationSourceCategory::externally_authorized),
        validator);
    const auto count = registry.validate_mutation(
        proposal(
            "proposal.vector.count",
            "vector.float",
            std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0},
            pc::AdaptiveMutationOperation::replace,
            pc::AdaptiveMutationSourceCategory::externally_authorized),
        validator);
    const auto range = registry.validate_mutation(
        proposal(
            "proposal.vector.range",
            "vector.float",
            std::vector<double>{1.0, 2.0, 3.0, 11.0},
            pc::AdaptiveMutationOperation::replace,
            pc::AdaptiveMutationSourceCategory::externally_authorized),
        validator);
    const auto invalid = registry.validate_mutation(
        proposal(
            "proposal.vector.invalid",
            "vector.float",
            std::numeric_limits<double>::infinity(),
            pc::AdaptiveMutationOperation::scale,
            pc::AdaptiveMutationSourceCategory::externally_authorized),
        validator);
    return valid.accepted() &&
           shape.code == pc::AdaptiveMutationValidationCode::shape_mismatch &&
           count.code == pc::AdaptiveMutationValidationCode::resource_exhausted &&
           range.code == pc::AdaptiveMutationValidationCode::out_of_bounds &&
           invalid.code ==
               pc::AdaptiveMutationValidationCode::arithmetic_overflow;
}

[[nodiscard]] bool test_authority_and_version_validation() {
    pc::AdaptiveStateRegistry registry{};
    pc::AdaptiveVariableDescriptor automatic =
        integer_descriptor("authority.automatic");
    pc::AdaptiveVariableDescriptor external =
        integer_descriptor("authority.external");
    external.allow_automatic_mutation = false;
    external.allow_external_mutation = true;
    if (!registry.register_variable(automatic).ok() ||
        !registry.register_variable(external).ok()) {
        return false;
    }

    auto denied_proposal = proposal(
        "proposal.denied",
        "authority.automatic",
        std::int64_t{11});
    denied_proposal.proposal_authority.decision =
        pc::AdaptiveAuthorityDecision::denied;
    auto unknown_source = proposal(
        "proposal.unknown",
        "authority.automatic",
        std::int64_t{11});
    unknown_source.source_category =
        pc::AdaptiveMutationSourceCategory::unknown;
    auto stale = proposal(
        "proposal.stale",
        "authority.automatic",
        std::int64_t{11});
    stale.expected_variable_version = 2;
    auto validator = validation_authority();
    auto denied_validator = validator;
    denied_validator.decision = pc::AdaptiveAuthorityDecision::denied;

    const auto automatic_allowed = registry.validate_mutation(
        proposal(
            "proposal.automatic.allowed",
            "authority.automatic",
            std::int64_t{11}),
        validator);
    const auto automatic_denied = registry.validate_mutation(
        proposal(
            "proposal.automatic.denied",
            "authority.external",
            std::int64_t{11}),
        validator);
    const auto external_allowed = registry.validate_mutation(
        proposal(
            "proposal.external.allowed",
            "authority.external",
            std::int64_t{11},
            pc::AdaptiveMutationOperation::replace,
            pc::AdaptiveMutationSourceCategory::operator_directed),
        validator);
    const auto external_denied = registry.validate_mutation(
        proposal(
            "proposal.external.denied",
            "authority.automatic",
            std::int64_t{11},
            pc::AdaptiveMutationOperation::replace,
            pc::AdaptiveMutationSourceCategory::externally_authorized),
        validator);
    const auto proposal_denied =
        registry.validate_mutation(denied_proposal, validator);
    const auto validation_denied = registry.validate_mutation(
        proposal(
            "proposal.validator.denied",
            "authority.automatic",
            std::int64_t{11}),
        denied_validator);
    const auto unknown = registry.validate_mutation(unknown_source, validator);
    const auto conflict = registry.validate_mutation(stale, validator);

    return automatic_allowed.accepted() &&
           automatic_denied.code ==
               pc::AdaptiveMutationValidationCode::authority_denied &&
           external_allowed.accepted() &&
           external_denied.code ==
               pc::AdaptiveMutationValidationCode::authority_denied &&
           proposal_denied.code ==
               pc::AdaptiveMutationValidationCode::authority_denied &&
           validation_denied.code ==
               pc::AdaptiveMutationValidationCode::authority_denied &&
           unknown.code == pc::AdaptiveMutationValidationCode::authority_denied &&
           conflict.code ==
               pc::AdaptiveMutationValidationCode::version_conflict;
}

}  // namespace

int main() {
    return test_empty_and_registration() &&
                   test_rejections_and_capacity() &&
                   test_ordering_shape_and_redaction() &&
                   test_scalar_validation() && test_vector_validation() &&
                   test_authority_and_version_validation()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
