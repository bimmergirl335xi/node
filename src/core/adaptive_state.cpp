#include "adaptive_state.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

namespace prometheus::core {
namespace {

inline constexpr std::size_t kAbsoluteMaximumVariables = 65536;
inline constexpr std::size_t kAbsoluteMaximumIdentifierLength = 256;
inline constexpr std::size_t kAbsoluteMaximumDisplayNameLength = 1024;
inline constexpr std::size_t kAbsoluteMaximumDescriptionLength = 4096;
inline constexpr std::size_t kAbsoluteMaximumDimensions = 8;
inline constexpr std::size_t kAbsoluteMaximumVectorElements = 65536;
inline constexpr std::size_t kAbsoluteMaximumSymbolicLength = 4096;
inline constexpr std::size_t kAbsoluteMaximumProposals = 1024;
inline constexpr std::size_t kAbsoluteMaximumCheckpoints = 256;
inline constexpr std::size_t kAbsoluteMaximumCheckpointBytes =
    256U * 1024U * 1024U;
inline constexpr std::size_t kAbsoluteMaximumHistoryEntries = 4096;
inline constexpr std::size_t kAbsoluteMaximumMessageLength = 4096;
inline constexpr std::size_t kAbsoluteMaximumMetadataLength = 4096;

[[nodiscard]] bool valid_limits(const AdaptiveStateLimits& limits) noexcept {
    return limits.maximum_variables > 0 &&
           limits.maximum_variables <= kAbsoluteMaximumVariables &&
           limits.maximum_identifier_length > 0 &&
           limits.maximum_identifier_length <=
               kAbsoluteMaximumIdentifierLength &&
           limits.maximum_display_name_length > 0 &&
           limits.maximum_display_name_length <=
               kAbsoluteMaximumDisplayNameLength &&
           limits.maximum_description_length > 0 &&
           limits.maximum_description_length <=
               kAbsoluteMaximumDescriptionLength &&
           limits.maximum_dimensions > 0 &&
           limits.maximum_dimensions <= kAbsoluteMaximumDimensions &&
           limits.maximum_vector_elements > 0 &&
           limits.maximum_vector_elements <=
               kAbsoluteMaximumVectorElements &&
           limits.maximum_symbolic_value_length > 0 &&
           limits.maximum_symbolic_value_length <=
               kAbsoluteMaximumSymbolicLength &&
           limits.maximum_proposals_per_transaction > 0 &&
           limits.maximum_proposals_per_transaction <=
               kAbsoluteMaximumProposals &&
           limits.maximum_checkpoints > 0 &&
           limits.maximum_checkpoints <= kAbsoluteMaximumCheckpoints &&
           limits.maximum_checkpoint_bytes > 0 &&
           limits.maximum_checkpoint_bytes <=
               kAbsoluteMaximumCheckpointBytes &&
           limits.maximum_history_entries > 0 &&
           limits.maximum_history_entries <=
               kAbsoluteMaximumHistoryEntries &&
           limits.maximum_message_length >= 128 &&
           limits.maximum_message_length <= kAbsoluteMaximumMessageLength &&
           limits.maximum_metadata_length >= 128 &&
           limits.maximum_metadata_length <= kAbsoluteMaximumMetadataLength;
}

[[nodiscard]] bool is_vector_type(AdaptiveValueType type) noexcept {
    return type == AdaptiveValueType::signed_integer_vector ||
           type == AdaptiveValueType::unsigned_integer_vector ||
           type == AdaptiveValueType::floating_point_vector;
}

[[nodiscard]] bool is_numeric_type(AdaptiveValueType type) noexcept {
    return type == AdaptiveValueType::signed_integer ||
           type == AdaptiveValueType::unsigned_integer ||
           type == AdaptiveValueType::floating_point || is_vector_type(type);
}

[[nodiscard]] std::size_t value_element_count(
    const AdaptiveValue& value) noexcept {
    return std::visit(
        [](const auto& typed_value) -> std::size_t {
            using Value = std::decay_t<decltype(typed_value)>;
            if constexpr (std::is_same_v<Value, std::vector<std::int64_t>> ||
                          std::is_same_v<Value, std::vector<std::uint64_t>> ||
                          std::is_same_v<Value, std::vector<double>>) {
                return typed_value.size();
            }
            return 1;
        },
        value);
}

[[nodiscard]] bool valid_shape(
    const AdaptiveVariableDescriptor& descriptor,
    const AdaptiveStateLimits& limits) noexcept {
    if (!is_vector_type(descriptor.value_type)) {
        return descriptor.shape.empty();
    }
    if (descriptor.shape.empty() ||
        descriptor.shape.size() > limits.maximum_dimensions) {
        return false;
    }
    std::size_t product = 1;
    for (const std::size_t dimension : descriptor.shape) {
        if (dimension == 0 ||
            product > limits.maximum_vector_elements / dimension) {
            return false;
        }
        product *= dimension;
    }
    return product == value_element_count(descriptor.default_value) &&
           product <= limits.maximum_vector_elements;
}

[[nodiscard]] bool bounds_type_matches(
    AdaptiveValueType type,
    const AdaptiveNumericBound& bound) noexcept {
    if (type == AdaptiveValueType::signed_integer ||
        type == AdaptiveValueType::signed_integer_vector) {
        return std::holds_alternative<std::int64_t>(bound);
    }
    if (type == AdaptiveValueType::unsigned_integer ||
        type == AdaptiveValueType::unsigned_integer_vector) {
        return std::holds_alternative<std::uint64_t>(bound);
    }
    if (type == AdaptiveValueType::floating_point ||
        type == AdaptiveValueType::floating_point_vector) {
        return std::holds_alternative<double>(bound);
    }
    return false;
}

template <typename Value>
[[nodiscard]] bool ordered_bounds(const AdaptiveNumericBounds& bounds) {
    const Value minimum = std::get<Value>(bounds.minimum);
    const Value maximum = std::get<Value>(bounds.maximum);
    if constexpr (std::is_same_v<Value, double>) {
        return std::isfinite(minimum) && std::isfinite(maximum) &&
               minimum <= maximum;
    }
    return minimum <= maximum;
}

[[nodiscard]] bool valid_bounds(
    AdaptiveValueType type,
    const AdaptiveNumericBounds& bounds) {
    if (!bounds.enabled) {
        return true;
    }
    if (!bounds_type_matches(type, bounds.minimum) ||
        !bounds_type_matches(type, bounds.maximum)) {
        return false;
    }
    if (std::holds_alternative<std::int64_t>(bounds.minimum)) {
        return ordered_bounds<std::int64_t>(bounds);
    }
    if (std::holds_alternative<std::uint64_t>(bounds.minimum)) {
        return ordered_bounds<std::uint64_t>(bounds);
    }
    return ordered_bounds<double>(bounds);
}

template <typename Value>
[[nodiscard]] bool value_in_bounds(
    Value value,
    const AdaptiveNumericBounds& bounds) noexcept {
    if constexpr (std::is_same_v<Value, double>) {
        if (!std::isfinite(value)) {
            return false;
        }
    }
    if (!bounds.enabled) {
        return true;
    }
    return value >= std::get<Value>(bounds.minimum) &&
           value <= std::get<Value>(bounds.maximum);
}

[[nodiscard]] bool adaptive_value_in_bounds(
    const AdaptiveValue& value,
    const AdaptiveNumericBounds& bounds) {
    return std::visit(
        [&bounds](const auto& typed_value) {
            using Value = std::decay_t<decltype(typed_value)>;
            if constexpr (std::is_same_v<Value, std::int64_t> ||
                          std::is_same_v<Value, std::uint64_t> ||
                          std::is_same_v<Value, double>) {
                return value_in_bounds(typed_value, bounds);
            } else if constexpr (
                std::is_same_v<Value, std::vector<std::int64_t>> ||
                std::is_same_v<Value, std::vector<std::uint64_t>> ||
                std::is_same_v<Value, std::vector<double>>) {
                return std::all_of(
                    typed_value.begin(),
                    typed_value.end(),
                    [&bounds](const auto element) {
                        return value_in_bounds(element, bounds);
                    });
            }
            return true;
        },
        value);
}

[[nodiscard]] bool numeric_bound_in_bounds(
    const AdaptiveNumericBound& value,
    const AdaptiveNumericBounds& bounds) {
    return std::visit(
        [&bounds](const auto typed_value) {
            return value_in_bounds(typed_value, bounds);
        },
        value);
}

[[nodiscard]] bool valid_descriptor(
    const AdaptiveVariableDescriptor& descriptor,
    const AdaptiveStateLimits& limits) {
    if (!is_valid_adaptive_identifier(
            descriptor.variable_id, limits.maximum_identifier_length) ||
        descriptor.display_name.empty() ||
        descriptor.display_name.size() > limits.maximum_display_name_length ||
        descriptor.description.size() > limits.maximum_description_length ||
        adaptive_value_type(descriptor.default_value) != descriptor.value_type ||
        !valid_shape(descriptor, limits) || descriptor.descriptor_version == 0 ||
        descriptor.schema_version == 0) {
        return false;
    }
    if (descriptor.value_type == AdaptiveValueType::symbolic &&
        std::get<std::string>(descriptor.default_value).size() >
            limits.maximum_symbolic_value_length) {
        return false;
    }
    if (is_numeric_type(descriptor.value_type)) {
        if (!descriptor.hard_bounds.enabled ||
            !valid_bounds(descriptor.value_type, descriptor.hard_bounds) ||
            !valid_bounds(descriptor.value_type, descriptor.soft_bounds) ||
            !adaptive_value_in_bounds(
                descriptor.default_value, descriptor.hard_bounds)) {
            return false;
        }
        if (descriptor.soft_bounds.enabled) {
            if (!adaptive_value_in_bounds(
                    descriptor.default_value, descriptor.soft_bounds) ||
                !numeric_bound_in_bounds(
                    descriptor.soft_bounds.minimum,
                    descriptor.hard_bounds) ||
                !numeric_bound_in_bounds(
                    descriptor.soft_bounds.maximum,
                    descriptor.hard_bounds)) {
                return false;
            }
        }
    } else if (descriptor.hard_bounds.enabled || descriptor.soft_bounds.enabled) {
        return false;
    }
    if (descriptor.maximum_mutation_magnitude.has_value() &&
        (!std::isfinite(*descriptor.maximum_mutation_magnitude) ||
         *descriptor.maximum_mutation_magnitude < 0.0)) {
        return false;
    }
    return true;
}

[[nodiscard]] bool should_include_value(
    AdaptiveSensitivity sensitivity,
    AdaptiveSnapshotMode mode) noexcept {
    if (mode == AdaptiveSnapshotMode::include_all_values) {
        return true;
    }
    if (mode == AdaptiveSnapshotMode::metadata_only) {
        return false;
    }
    return sensitivity == AdaptiveSensitivity::public_value ||
           sensitivity == AdaptiveSensitivity::internal;
}

[[nodiscard]] AdaptiveValue redacted_value_for(
    const AdaptiveValue& value) {
    return std::visit(
        [](const auto& typed_value) -> AdaptiveValue {
            using Value = std::decay_t<decltype(typed_value)>;
            if constexpr (std::is_same_v<Value, bool>) {
                return false;
            } else if constexpr (std::is_same_v<Value, std::int64_t>) {
                return std::int64_t{0};
            } else if constexpr (std::is_same_v<Value, std::uint64_t>) {
                return std::uint64_t{0};
            } else if constexpr (std::is_same_v<Value, double>) {
                return 0.0;
            } else if constexpr (std::is_same_v<Value, std::string>) {
                return std::string{};
            } else {
                return Value(typed_value.size(), typename Value::value_type{});
            }
        },
        value);
}

template <typename Value>
[[nodiscard]] bool checked_add(
    Value left,
    Value right,
    Value& result) noexcept {
    if constexpr (std::is_same_v<Value, std::int64_t>) {
        if ((right > 0 &&
             left > std::numeric_limits<Value>::max() - right) ||
            (right < 0 &&
             left < std::numeric_limits<Value>::min() - right)) {
            return false;
        }
    } else if constexpr (std::is_same_v<Value, std::uint64_t>) {
        if (left > std::numeric_limits<Value>::max() - right) {
            return false;
        }
    }
    result = left + right;
    if constexpr (std::is_same_v<Value, double>) {
        return std::isfinite(left) && std::isfinite(right) &&
               std::isfinite(result);
    }
    return true;
}

template <typename Value>
[[nodiscard]] bool checked_multiply(
    Value left,
    Value right,
    Value& result) noexcept {
    if constexpr (std::is_same_v<Value, std::int64_t>) {
        if (left == 0 || right == 0) {
            result = 0;
            return true;
        }
        if ((left == -1 &&
             right == std::numeric_limits<Value>::min()) ||
            (right == -1 &&
             left == std::numeric_limits<Value>::min())) {
            return false;
        }
        if ((left > 0 && right > 0 &&
             left > std::numeric_limits<Value>::max() / right) ||
            (left > 0 && right < 0 &&
             right < std::numeric_limits<Value>::min() / left) ||
            (left < 0 && right > 0 &&
             left < std::numeric_limits<Value>::min() / right) ||
            (left < 0 && right < 0 &&
             left < std::numeric_limits<Value>::max() / right)) {
            return false;
        }
    } else if constexpr (std::is_same_v<Value, std::uint64_t>) {
        if (right != 0 &&
            left > std::numeric_limits<Value>::max() / right) {
            return false;
        }
    }
    result = left * right;
    if constexpr (std::is_same_v<Value, double>) {
        return std::isfinite(left) && std::isfinite(right) &&
               std::isfinite(result);
    }
    return true;
}

template <typename Value>
[[nodiscard]] bool transform_scalar(
    Value current,
    Value operand,
    AdaptiveMutationOperation operation,
    Value& result) noexcept {
    switch (operation) {
        case AdaptiveMutationOperation::replace:
            result = operand;
            if constexpr (std::is_same_v<Value, double>) {
                return std::isfinite(result);
            }
            return true;
        case AdaptiveMutationOperation::add:
        case AdaptiveMutationOperation::bounded_adjust:
            return checked_add(current, operand, result);
        case AdaptiveMutationOperation::scale:
            return checked_multiply(current, operand, result);
        case AdaptiveMutationOperation::unknown:
            return false;
    }
    return false;
}

[[nodiscard]] bool value_within_resource_limits(
    const AdaptiveValue& value,
    const AdaptiveStateLimits& limits) noexcept {
    return std::visit(
        [&limits](const auto& typed_value) {
            using Value = std::decay_t<decltype(typed_value)>;
            if constexpr (std::is_same_v<Value, std::string>) {
                return typed_value.size() <=
                       limits.maximum_symbolic_value_length;
            } else if constexpr (
                std::is_same_v<Value, std::vector<std::int64_t>> ||
                std::is_same_v<Value, std::vector<std::uint64_t>> ||
                std::is_same_v<Value, std::vector<double>>) {
                return typed_value.size() <= limits.maximum_vector_elements;
            }
            return true;
        },
        value);
}

[[nodiscard]] AdaptiveMutationValidationCode compute_resulting_value(
    const AdaptiveVariableDescriptor& descriptor,
    const AdaptiveValue& current,
    const AdaptiveMutationProposal& proposal,
    AdaptiveValue& result) {
    if (proposal.operation == AdaptiveMutationOperation::unknown) {
        return AdaptiveMutationValidationCode::invalid_proposal;
    }
    if (proposal.operation == AdaptiveMutationOperation::replace) {
        if (adaptive_value_type(proposal.operand) != descriptor.value_type) {
            return AdaptiveMutationValidationCode::type_mismatch;
        }
        if (is_vector_type(descriptor.value_type) &&
            value_element_count(proposal.operand) !=
                value_element_count(current)) {
            return AdaptiveMutationValidationCode::shape_mismatch;
        }
        result = proposal.operand;
        if (!adaptive_value_in_bounds(result, descriptor.hard_bounds)) {
            return AdaptiveMutationValidationCode::out_of_bounds;
        }
        return AdaptiveMutationValidationCode::accepted;
    }

    if (!is_numeric_type(descriptor.value_type)) {
        return AdaptiveMutationValidationCode::type_mismatch;
    }

    const bool vector_target = is_vector_type(descriptor.value_type);
    if (!vector_target) {
        if (adaptive_value_type(proposal.operand) != descriptor.value_type) {
            return AdaptiveMutationValidationCode::type_mismatch;
        }
        return std::visit(
            [&proposal, &result](const auto& typed_current)
                -> AdaptiveMutationValidationCode {
                using Value = std::decay_t<decltype(typed_current)>;
                if constexpr (std::is_same_v<Value, std::int64_t> ||
                              std::is_same_v<Value, std::uint64_t> ||
                              std::is_same_v<Value, double>) {
                    Value transformed{};
                    if (!transform_scalar(
                            typed_current,
                            std::get<Value>(proposal.operand),
                            proposal.operation,
                            transformed)) {
                        return AdaptiveMutationValidationCode::arithmetic_overflow;
                    }
                    result = transformed;
                    return AdaptiveMutationValidationCode::accepted;
                }
                return AdaptiveMutationValidationCode::type_mismatch;
            },
            current);
    }

    if (proposal.operation == AdaptiveMutationOperation::scale) {
        const AdaptiveValueType scalar_type =
            descriptor.value_type == AdaptiveValueType::signed_integer_vector
                ? AdaptiveValueType::signed_integer
                : descriptor.value_type ==
                          AdaptiveValueType::unsigned_integer_vector
                      ? AdaptiveValueType::unsigned_integer
                      : AdaptiveValueType::floating_point;
        if (adaptive_value_type(proposal.operand) != scalar_type) {
            return AdaptiveMutationValidationCode::type_mismatch;
        }
    } else {
        if (adaptive_value_type(proposal.operand) != descriptor.value_type) {
            return AdaptiveMutationValidationCode::type_mismatch;
        }
        if (value_element_count(proposal.operand) !=
            value_element_count(current)) {
            return AdaptiveMutationValidationCode::shape_mismatch;
        }
    }

    return std::visit(
        [&proposal, &result](const auto& typed_current)
            -> AdaptiveMutationValidationCode {
            using Vector = std::decay_t<decltype(typed_current)>;
            if constexpr (
                std::is_same_v<Vector, std::vector<std::int64_t>> ||
                std::is_same_v<Vector, std::vector<std::uint64_t>> ||
                std::is_same_v<Vector, std::vector<double>>) {
                using Value = typename Vector::value_type;
                Vector transformed(typed_current.size());
                for (std::size_t index = 0; index < typed_current.size();
                     ++index) {
                    const Value operand =
                        proposal.operation == AdaptiveMutationOperation::scale
                            ? std::get<Value>(proposal.operand)
                            : std::get<Vector>(proposal.operand)[index];
                    if (!transform_scalar(
                            typed_current[index],
                            operand,
                            proposal.operation,
                            transformed[index])) {
                        return AdaptiveMutationValidationCode::arithmetic_overflow;
                    }
                }
                result = std::move(transformed);
                return AdaptiveMutationValidationCode::accepted;
            }
            return AdaptiveMutationValidationCode::type_mismatch;
        },
        current);
}

[[nodiscard]] double maximum_value_difference(
    const AdaptiveValue& left,
    const AdaptiveValue& right) {
    return std::visit(
        [&right](const auto& typed_left) -> double {
            using Value = std::decay_t<decltype(typed_left)>;
            if constexpr (std::is_same_v<Value, bool> ||
                          std::is_same_v<Value, std::string>) {
                return typed_left == std::get<Value>(right) ? 0.0 : 1.0;
            } else if constexpr (std::is_same_v<Value, std::int64_t> ||
                                 std::is_same_v<Value, std::uint64_t> ||
                                 std::is_same_v<Value, double>) {
                return static_cast<double>(std::fabs(
                    static_cast<long double>(typed_left) -
                    static_cast<long double>(std::get<Value>(right))));
            } else {
                const Value& typed_right = std::get<Value>(right);
                long double maximum = 0.0L;
                for (std::size_t index = 0; index < typed_left.size(); ++index) {
                    maximum = std::max(
                        maximum,
                        std::fabs(
                            static_cast<long double>(typed_left[index]) -
                            static_cast<long double>(typed_right[index])));
                }
                return static_cast<double>(maximum);
            }
        },
        left);
}

[[nodiscard]] const char* validation_message(
    AdaptiveMutationValidationCode code) noexcept {
    switch (code) {
        case AdaptiveMutationValidationCode::accepted:
            return "Mutation proposal is valid";
        case AdaptiveMutationValidationCode::not_found:
            return "Target variable is not registered";
        case AdaptiveMutationValidationCode::version_conflict:
            return "Expected variable version is stale";
        case AdaptiveMutationValidationCode::type_mismatch:
            return "Mutation operand type is incompatible";
        case AdaptiveMutationValidationCode::shape_mismatch:
            return "Mutation operand shape is incompatible";
        case AdaptiveMutationValidationCode::out_of_bounds:
            return "Mutation result violates hard bounds";
        case AdaptiveMutationValidationCode::soft_bound_violation:
            return "Mutation result violates soft bounds";
        case AdaptiveMutationValidationCode::magnitude_exceeded:
            return "Mutation exceeds the maximum per-element magnitude";
        case AdaptiveMutationValidationCode::arithmetic_overflow:
            return "Mutation arithmetic is non-finite or would overflow";
        case AdaptiveMutationValidationCode::authority_denied:
            return "Mutation authority or source permission is denied";
        case AdaptiveMutationValidationCode::disabled:
            return "Target variable is disabled";
        case AdaptiveMutationValidationCode::rate_limited:
            return "Mutation violates the minimum revision interval";
        case AdaptiveMutationValidationCode::resource_exhausted:
            return "Mutation validation exhausted a configured resource";
        case AdaptiveMutationValidationCode::duplicate:
            return "Mutation identity has already been processed";
        case AdaptiveMutationValidationCode::invalid_proposal:
            return "Mutation proposal metadata is invalid";
        case AdaptiveMutationValidationCode::incompatible_rollback:
            return "Target variable is not rollback eligible";
        case AdaptiveMutationValidationCode::deferred:
            return "Mutation validation was deferred";
        case AdaptiveMutationValidationCode::rejected:
            return "Mutation proposal was rejected";
        case AdaptiveMutationValidationCode::unknown:
            return "Mutation validation outcome is unknown";
    }
    return "Mutation validation outcome is unknown";
}

}  // namespace

AdaptiveStateRegistry::AdaptiveStateRegistry(AdaptiveStateLimits limits)
    : limits_(limits), limits_valid_(valid_limits(limits)) {}

AdaptiveRegistrationResult AdaptiveStateRegistry::register_variable(
    AdaptiveVariableDescriptor descriptor) {
    try {
        if (!limits_valid_) {
            return {AdaptiveRegistrationCode::invalid_configuration,
                    descriptor.variable_id,
                    "Adaptive-state limits are invalid"};
        }
        if (!is_valid_adaptive_identifier(
                descriptor.variable_id, limits_.maximum_identifier_length)) {
            return {AdaptiveRegistrationCode::invalid_identifier,
                    descriptor.variable_id,
                    "Adaptive variable identifier is invalid"};
        }
        if (!valid_descriptor(descriptor, limits_)) {
            return {AdaptiveRegistrationCode::invalid_descriptor,
                    descriptor.variable_id,
                    "Adaptive variable descriptor is invalid"};
        }

        std::lock_guard<std::mutex> lock(mutex_);
        const auto duplicate = std::find_if(
            variables_.begin(),
            variables_.end(),
            [&descriptor](const VariableRecord& record) {
                return record.descriptor.variable_id == descriptor.variable_id;
            });
        if (duplicate != variables_.end()) {
            return {AdaptiveRegistrationCode::duplicate_identifier,
                    descriptor.variable_id,
                    "Adaptive variable identifier is already registered"};
        }
        if (variables_.size() >= limits_.maximum_variables) {
            return {AdaptiveRegistrationCode::capacity_exhausted,
                    descriptor.variable_id,
                    "Adaptive-state registry capacity is exhausted"};
        }
        if (registry_revision_ == std::numeric_limits<std::uint64_t>::max()) {
            return {AdaptiveRegistrationCode::capacity_exhausted,
                    descriptor.variable_id,
                    "Adaptive-state registry revision is exhausted"};
        }

        ++registry_revision_;
        VariableRecord record{};
        record.value = descriptor.default_value;
        record.condition = descriptor.enabled
                               ? AdaptiveVariableCondition::ready
                               : AdaptiveVariableCondition::disabled;
        record.last_changed_registry_revision = registry_revision_;
        record.descriptor = std::move(descriptor);
        const std::string variable_id = record.descriptor.variable_id;
        variables_.push_back(std::move(record));
        return {AdaptiveRegistrationCode::success, variable_id, {}};
    } catch (const std::bad_alloc&) {
        return {AdaptiveRegistrationCode::resource_exhausted,
                descriptor.variable_id,
                "Adaptive variable registration exhausted memory"};
    } catch (...) {
        return {AdaptiveRegistrationCode::invalid_descriptor,
                descriptor.variable_id,
                "Adaptive variable registration failed safely"};
    }
}

AdaptiveMutationValidationResult AdaptiveStateRegistry::validate_mutation(
    const AdaptiveMutationProposal& proposal,
    const AdaptiveValidationAuthority& authority) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        return validate_mutation_locked(proposal, authority).validation;
    } catch (const std::bad_alloc&) {
        AdaptiveMutationValidationResult result{};
        result.code = AdaptiveMutationValidationCode::resource_exhausted;
        result.message = validation_message(result.code);
        return result;
    } catch (...) {
        AdaptiveMutationValidationResult result{};
        result.code = AdaptiveMutationValidationCode::rejected;
        result.message = "Mutation validation failed safely";
        return result;
    }
}

AdaptiveStateRegistry::PreparedMutation
AdaptiveStateRegistry::validate_mutation_locked(
    const AdaptiveMutationProposal& proposal,
    const AdaptiveValidationAuthority& authority) const {
    PreparedMutation prepared{};
    AdaptiveMutationValidationResult& result = prepared.validation;
    result.proposal_id = proposal.proposal_id;
    result.target_variable_id = proposal.target_variable_id;
    result.expected_variable_version = proposal.expected_variable_version;
    result.validation_authority_identity = authority.authority_identity;
    result.evidence_reference = !authority.evidence_reference.empty()
                                    ? authority.evidence_reference
                                    : proposal.validation_evidence_reference;

    const auto reject = [&result](AdaptiveMutationValidationCode code) {
        result.code = code;
        result.message = validation_message(code);
    };

    if (!limits_valid_) {
        reject(AdaptiveMutationValidationCode::resource_exhausted);
        return prepared;
    }
    const auto valid_identity = [this](const std::string& value) {
        return is_valid_adaptive_identifier(
            value, limits_.maximum_identifier_length);
    };
    if (!valid_identity(proposal.proposal_id) ||
        !valid_identity(proposal.target_variable_id) ||
        !valid_identity(proposal.source_identity) ||
        !valid_identity(proposal.proposal_authority.authority_identity) ||
        !valid_identity(authority.authority_identity) ||
        (!proposal.correlation_id.empty() &&
         !valid_identity(proposal.correlation_id)) ||
        proposal.validation_evidence_reference.size() >
            limits_.maximum_metadata_length ||
        proposal.proposal_authority.evidence_reference.size() >
            limits_.maximum_metadata_length ||
        authority.evidence_reference.size() >
            limits_.maximum_metadata_length ||
        proposal.expected_variable_version == 0 ||
        proposal.operation == AdaptiveMutationOperation::unknown) {
        reject(AdaptiveMutationValidationCode::invalid_proposal);
        return prepared;
    }
    if (proposal.source_category ==
        AdaptiveMutationSourceCategory::unknown) {
        reject(AdaptiveMutationValidationCode::authority_denied);
        return prepared;
    }
    if (proposal.proposal_authority.decision !=
            AdaptiveAuthorityDecision::authorized ||
        authority.decision != AdaptiveAuthorityDecision::authorized) {
        reject(AdaptiveMutationValidationCode::authority_denied);
        return prepared;
    }
    if (!value_within_resource_limits(proposal.operand, limits_)) {
        reject(AdaptiveMutationValidationCode::resource_exhausted);
        return prepared;
    }
    if (std::find(
            processed_proposal_ids_.begin(),
            processed_proposal_ids_.end(),
            proposal.proposal_id) != processed_proposal_ids_.end()) {
        reject(AdaptiveMutationValidationCode::duplicate);
        return prepared;
    }

    const auto iterator = std::find_if(
        variables_.begin(),
        variables_.end(),
        [&proposal](const VariableRecord& record) {
            return record.descriptor.variable_id ==
                   proposal.target_variable_id;
        });
    if (iterator == variables_.end()) {
        reject(AdaptiveMutationValidationCode::not_found);
        return prepared;
    }
    const VariableRecord& record = *iterator;
    result.current_variable_version = record.version;
    if (!record.descriptor.enabled ||
        record.condition == AdaptiveVariableCondition::disabled) {
        reject(AdaptiveMutationValidationCode::disabled);
        return prepared;
    }

    const bool automatic = proposal.source_category ==
                           AdaptiveMutationSourceCategory::automatic_internal;
    if ((automatic && !record.descriptor.allow_automatic_mutation) ||
        (!automatic && !record.descriptor.allow_external_mutation)) {
        reject(AdaptiveMutationValidationCode::authority_denied);
        return prepared;
    }
    if (proposal.expected_variable_version != record.version) {
        reject(AdaptiveMutationValidationCode::version_conflict);
        return prepared;
    }
    if (proposal.require_rollback_eligibility &&
        !record.descriptor.rollback_eligible) {
        reject(AdaptiveMutationValidationCode::incompatible_rollback);
        return prepared;
    }
    if (record.descriptor.provenance_requirement ==
            AdaptiveProvenanceRequirement::source_and_correlation &&
        proposal.correlation_id.empty()) {
        reject(AdaptiveMutationValidationCode::invalid_proposal);
        return prepared;
    }
    if (record.descriptor.provenance_requirement ==
            AdaptiveProvenanceRequirement::validation_evidence &&
        result.evidence_reference.empty()) {
        reject(AdaptiveMutationValidationCode::invalid_proposal);
        return prepared;
    }
    if (record.descriptor.minimum_revision_interval > 0 &&
        (registry_revision_ < record.last_changed_registry_revision ||
         registry_revision_ - record.last_changed_registry_revision <
             record.descriptor.minimum_revision_interval)) {
        reject(AdaptiveMutationValidationCode::rate_limited);
        return prepared;
    }

    AdaptiveMutationValidationCode computation = compute_resulting_value(
        record.descriptor, record.value, proposal, prepared.resulting_value);
    if (computation != AdaptiveMutationValidationCode::accepted) {
        reject(computation);
        return prepared;
    }
    if (!adaptive_value_in_bounds(
            prepared.resulting_value, record.descriptor.hard_bounds)) {
        reject(AdaptiveMutationValidationCode::out_of_bounds);
        return prepared;
    }
    if (record.descriptor.soft_bounds.enabled &&
        !adaptive_value_in_bounds(
            prepared.resulting_value, record.descriptor.soft_bounds)) {
        reject(AdaptiveMutationValidationCode::soft_bound_violation);
        return prepared;
    }
    if (record.descriptor.maximum_mutation_magnitude.has_value() &&
        maximum_value_difference(
            record.value, prepared.resulting_value) >
            *record.descriptor.maximum_mutation_magnitude) {
        reject(AdaptiveMutationValidationCode::magnitude_exceeded);
        return prepared;
    }
    if (record.version == std::numeric_limits<std::uint64_t>::max()) {
        reject(AdaptiveMutationValidationCode::resource_exhausted);
        return prepared;
    }

    result.resulting_variable_version = record.version + 1;
    reject(AdaptiveMutationValidationCode::accepted);
    return prepared;
}

std::optional<AdaptiveVariableSnapshot> AdaptiveStateRegistry::find(
    const std::string& variable_id,
    AdaptiveSnapshotMode mode) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iterator = std::find_if(
        variables_.begin(),
        variables_.end(),
        [&variable_id](const VariableRecord& record) {
            return record.descriptor.variable_id == variable_id;
        });
    if (iterator == variables_.end()) {
        return std::nullopt;
    }
    return make_snapshot_locked(*iterator, mode);
}

AdaptiveRegistrySnapshot AdaptiveStateRegistry::snapshot(
    AdaptiveSnapshotMode mode) const {
    std::lock_guard<std::mutex> lock(mutex_);
    AdaptiveRegistrySnapshot result{};
    result.registry_revision = registry_revision_;
    result.variables.reserve(variables_.size());
    for (const VariableRecord& record : variables_) {
        result.variables.push_back(make_snapshot_locked(record, mode));
    }
    std::sort(
        result.variables.begin(),
        result.variables.end(),
        [](const AdaptiveVariableSnapshot& left,
           const AdaptiveVariableSnapshot& right) {
            return left.descriptor.variable_id < right.descriptor.variable_id;
        });
    return result;
}

AdaptiveRegistryStatus AdaptiveStateRegistry::status() const {
    std::lock_guard<std::mutex> lock(mutex_);
    AdaptiveRegistryStatus result{};
    result.condition = limits_valid_ ? AdaptiveRegistryCondition::ready
                                     : AdaptiveRegistryCondition::invalid_configuration;
    result.limits = limits_;
    result.registered_variables = variables_.size();
    result.registry_revision = registry_revision_;
    result.message = limits_valid_ ? "Adaptive-state registry is ready"
                                   : "Adaptive-state limits are invalid";
    return result;
}

std::size_t AdaptiveStateRegistry::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return variables_.size();
}

std::uint64_t AdaptiveStateRegistry::revision() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return registry_revision_;
}

AdaptiveVariableSnapshot AdaptiveStateRegistry::make_snapshot_locked(
    const VariableRecord& record,
    AdaptiveSnapshotMode mode) const {
    AdaptiveVariableSnapshot result{};
    result.descriptor = record.descriptor;
    result.value_redacted = !should_include_value(
        record.descriptor.sensitivity, mode);
    if (result.value_redacted) {
        result.descriptor.default_value =
            redacted_value_for(record.descriptor.default_value);
    } else {
        result.value = record.value;
    }
    result.version = record.version;
    result.descriptor_version = record.descriptor.descriptor_version;
    result.last_mutation_id = record.last_mutation_id;
    result.last_source_identity = record.last_source_identity;
    result.last_update_category = record.last_update_category;
    result.last_validation_evidence = record.last_validation_evidence;
    result.condition = record.condition;
    result.last_changed_registry_revision =
        record.last_changed_registry_revision;
    return result;
}

bool is_valid_adaptive_identifier(
    const std::string& value,
    std::size_t maximum_length) noexcept {
    if (value.empty() || value.size() > maximum_length ||
        !std::isalnum(static_cast<unsigned char>(value.front()))) {
        return false;
    }
    return std::all_of(
        value.begin(), value.end(), [](const char character) {
            const unsigned char byte =
                static_cast<unsigned char>(character);
            return std::isalnum(byte) || character == '.' ||
                   character == '_' || character == '-';
        });
}

AdaptiveValueType adaptive_value_type(const AdaptiveValue& value) noexcept {
    return static_cast<AdaptiveValueType>(value.index());
}

const char* to_string(AdaptiveValueType value) noexcept {
    switch (value) {
        case AdaptiveValueType::boolean: return "boolean";
        case AdaptiveValueType::signed_integer: return "signed_integer";
        case AdaptiveValueType::unsigned_integer: return "unsigned_integer";
        case AdaptiveValueType::floating_point: return "floating_point";
        case AdaptiveValueType::symbolic: return "symbolic";
        case AdaptiveValueType::signed_integer_vector: return "signed_integer_vector";
        case AdaptiveValueType::unsigned_integer_vector: return "unsigned_integer_vector";
        case AdaptiveValueType::floating_point_vector: return "floating_point_vector";
    }
    return "unknown";
}

const char* to_string(AdaptivePersistenceClass value) noexcept {
    switch (value) {
        case AdaptivePersistenceClass::ephemeral: return "ephemeral";
        case AdaptivePersistenceClass::checkpoint_eligible: return "checkpoint_eligible";
        case AdaptivePersistenceClass::memory_proposal_eligible: return "memory_proposal_eligible";
        case AdaptivePersistenceClass::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(AdaptiveSensitivity value) noexcept {
    switch (value) {
        case AdaptiveSensitivity::public_value: return "public";
        case AdaptiveSensitivity::internal: return "internal";
        case AdaptiveSensitivity::restricted: return "restricted";
        case AdaptiveSensitivity::secret: return "secret";
        case AdaptiveSensitivity::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(AdaptiveVariableCondition value) noexcept {
    switch (value) {
        case AdaptiveVariableCondition::unknown: return "unknown";
        case AdaptiveVariableCondition::ready: return "ready";
        case AdaptiveVariableCondition::disabled: return "disabled";
        case AdaptiveVariableCondition::degraded: return "degraded";
        case AdaptiveVariableCondition::invalid: return "invalid";
        case AdaptiveVariableCondition::unavailable: return "unavailable";
    }
    return "unknown";
}

const char* to_string(AdaptiveUpdateCategory value) noexcept {
    switch (value) {
        case AdaptiveUpdateCategory::unknown: return "unknown";
        case AdaptiveUpdateCategory::initial_registration: return "initial_registration";
        case AdaptiveUpdateCategory::mutation: return "mutation";
        case AdaptiveUpdateCategory::rollback: return "rollback";
    }
    return "unknown";
}

const char* to_string(AdaptiveRegistryCondition value) noexcept {
    switch (value) {
        case AdaptiveRegistryCondition::ready: return "ready";
        case AdaptiveRegistryCondition::invalid_configuration: return "invalid_configuration";
        case AdaptiveRegistryCondition::degraded: return "degraded";
        case AdaptiveRegistryCondition::unavailable: return "unavailable";
    }
    return "unknown";
}

const char* to_string(AdaptiveRegistrationCode value) noexcept {
    switch (value) {
        case AdaptiveRegistrationCode::success: return "success";
        case AdaptiveRegistrationCode::invalid_configuration: return "invalid_configuration";
        case AdaptiveRegistrationCode::invalid_identifier: return "invalid_identifier";
        case AdaptiveRegistrationCode::invalid_descriptor: return "invalid_descriptor";
        case AdaptiveRegistrationCode::duplicate_identifier: return "duplicate_identifier";
        case AdaptiveRegistrationCode::capacity_exhausted: return "capacity_exhausted";
        case AdaptiveRegistrationCode::resource_exhausted: return "resource_exhausted";
    }
    return "unknown";
}

const char* to_string(AdaptiveMutationSourceCategory value) noexcept {
    switch (value) {
        case AdaptiveMutationSourceCategory::automatic_internal:
            return "automatic_internal";
        case AdaptiveMutationSourceCategory::externally_authorized:
            return "externally_authorized";
        case AdaptiveMutationSourceCategory::operator_directed:
            return "operator_directed";
        case AdaptiveMutationSourceCategory::recovery: return "recovery";
        case AdaptiveMutationSourceCategory::system_policy:
            return "system_policy";
        case AdaptiveMutationSourceCategory::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(AdaptiveMutationOperation value) noexcept {
    switch (value) {
        case AdaptiveMutationOperation::replace: return "replace";
        case AdaptiveMutationOperation::add: return "add";
        case AdaptiveMutationOperation::scale: return "scale";
        case AdaptiveMutationOperation::bounded_adjust:
            return "bounded_adjust";
        case AdaptiveMutationOperation::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(AdaptiveAuthorityDecision value) noexcept {
    switch (value) {
        case AdaptiveAuthorityDecision::unknown: return "unknown";
        case AdaptiveAuthorityDecision::authorized: return "authorized";
        case AdaptiveAuthorityDecision::denied: return "denied";
    }
    return "unknown";
}

const char* to_string(AdaptiveMutationValidationCode value) noexcept {
    switch (value) {
        case AdaptiveMutationValidationCode::accepted: return "accepted";
        case AdaptiveMutationValidationCode::rejected: return "rejected";
        case AdaptiveMutationValidationCode::deferred: return "deferred";
        case AdaptiveMutationValidationCode::not_found: return "not_found";
        case AdaptiveMutationValidationCode::version_conflict:
            return "version_conflict";
        case AdaptiveMutationValidationCode::type_mismatch:
            return "type_mismatch";
        case AdaptiveMutationValidationCode::shape_mismatch:
            return "shape_mismatch";
        case AdaptiveMutationValidationCode::out_of_bounds:
            return "out_of_bounds";
        case AdaptiveMutationValidationCode::soft_bound_violation:
            return "soft_bound_violation";
        case AdaptiveMutationValidationCode::magnitude_exceeded:
            return "magnitude_exceeded";
        case AdaptiveMutationValidationCode::arithmetic_overflow:
            return "arithmetic_overflow";
        case AdaptiveMutationValidationCode::authority_denied:
            return "authority_denied";
        case AdaptiveMutationValidationCode::disabled: return "disabled";
        case AdaptiveMutationValidationCode::rate_limited:
            return "rate_limited";
        case AdaptiveMutationValidationCode::resource_exhausted:
            return "resource_exhausted";
        case AdaptiveMutationValidationCode::duplicate: return "duplicate";
        case AdaptiveMutationValidationCode::invalid_proposal:
            return "invalid_proposal";
        case AdaptiveMutationValidationCode::incompatible_rollback:
            return "incompatible_rollback";
        case AdaptiveMutationValidationCode::unknown: return "unknown";
    }
    return "unknown";
}

}  // namespace prometheus::core
