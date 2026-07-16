#include <cstdlib>
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

}  // namespace

int main() {
    return test_empty_and_registration() &&
                   test_rejections_and_capacity() &&
                   test_ordering_shape_and_redaction()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
