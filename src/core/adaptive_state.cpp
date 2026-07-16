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

static_assert(std::is_nothrow_move_assignable_v<AdaptiveValue>);
static_assert(std::is_nothrow_move_assignable_v<std::string>);

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

[[nodiscard]] const char* transaction_message(
    AdaptiveTransactionCode code) noexcept {
    switch (code) {
        case AdaptiveTransactionCode::applied:
            return "Adaptive-state transaction applied atomically";
        case AdaptiveTransactionCode::rejected:
            return "Adaptive-state transaction validation failed";
        case AdaptiveTransactionCode::version_conflict:
            return "Expected registry revision is stale";
        case AdaptiveTransactionCode::authority_denied:
            return "Transaction application authority is denied";
        case AdaptiveTransactionCode::duplicate:
            return "Transaction or proposal identity is duplicated";
        case AdaptiveTransactionCode::invalid_transaction:
            return "Adaptive-state transaction metadata is invalid";
        case AdaptiveTransactionCode::resource_exhausted:
            return "Adaptive-state transaction exhausted a configured resource";
        case AdaptiveTransactionCode::unknown:
            return "Adaptive-state transaction outcome is unknown";
    }
    return "Adaptive-state transaction outcome is unknown";
}

[[nodiscard]] const char* checkpoint_message(
    AdaptiveCheckpointCode code) noexcept {
    switch (code) {
        case AdaptiveCheckpointCode::created:
            return "In-memory checkpoint created";
        case AdaptiveCheckpointCode::invalid_request:
            return "Checkpoint request metadata is invalid";
        case AdaptiveCheckpointCode::authority_denied:
            return "Checkpoint validation authority is denied";
        case AdaptiveCheckpointCode::duplicate:
            return "Checkpoint identity is already present";
        case AdaptiveCheckpointCode::capacity_exhausted:
            return "Checkpoint count capacity is exhausted";
        case AdaptiveCheckpointCode::resource_exhausted:
            return "Checkpoint byte capacity or allocation is exhausted";
        case AdaptiveCheckpointCode::no_eligible_variables:
            return "No rollback-eligible checkpoint variables are registered";
        case AdaptiveCheckpointCode::unknown:
            return "Checkpoint outcome is unknown";
    }
    return "Checkpoint outcome is unknown";
}

[[nodiscard]] const char* rollback_message(
    AdaptiveRollbackCode code) noexcept {
    switch (code) {
        case AdaptiveRollbackCode::restored:
            return "Checkpoint values restored atomically in memory";
        case AdaptiveRollbackCode::invalid_request:
            return "Rollback request metadata is invalid";
        case AdaptiveRollbackCode::authority_denied:
            return "Rollback authority is denied";
        case AdaptiveRollbackCode::duplicate:
            return "Rollback identity has already been processed";
        case AdaptiveRollbackCode::checkpoint_not_found:
            return "Checkpoint identity is not present";
        case AdaptiveRollbackCode::incompatible_checkpoint:
            return "Checkpoint is incompatible with current descriptors";
        case AdaptiveRollbackCode::version_conflict:
            return "Expected rollback registry revision is stale";
        case AdaptiveRollbackCode::resource_exhausted:
            return "Rollback exhausted a configured resource";
        case AdaptiveRollbackCode::unknown:
            return "Rollback outcome is unknown";
    }
    return "Rollback outcome is unknown";
}

[[nodiscard]] std::size_t adaptive_value_bytes(
    const AdaptiveValue& value) noexcept {
    return std::visit(
        [](const auto& typed_value) -> std::size_t {
            using Value = std::decay_t<decltype(typed_value)>;
            if constexpr (std::is_same_v<Value, std::string>) {
                return typed_value.size();
            } else if constexpr (
                std::is_same_v<Value, std::vector<std::int64_t>> ||
                std::is_same_v<Value, std::vector<std::uint64_t>> ||
                std::is_same_v<Value, std::vector<double>>) {
                return typed_value.size() * sizeof(typename Value::value_type);
            }
            return sizeof(Value);
        },
        value);
}

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
    const AdaptiveProposalAuthority& proposal_authority,
    const AdaptiveValidationAuthority& authority) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        return validate_mutation_locked(
                   proposal, proposal_authority, authority)
            .validation;
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
    const AdaptiveProposalAuthority& proposal_authority,
    const AdaptiveValidationAuthority& authority) const {
    PreparedMutation prepared{};
    AdaptiveMutationValidationResult& result = prepared.validation;
    result.proposal_id = proposal.proposal_id;
    result.target_variable_id = proposal.target_variable_id;
    result.expected_variable_version = proposal.expected_variable_version;
    result.proposal_authority_identity =
        proposal_authority.authority_identity;
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
        !valid_identity(proposal.proposal_authority_reference) ||
        !valid_identity(proposal_authority.authority_identity) ||
        !valid_identity(authority.authority_identity) ||
        (!proposal.correlation_id.empty() &&
         !valid_identity(proposal.correlation_id)) ||
        proposal.validation_evidence_reference.size() >
            limits_.maximum_metadata_length ||
        proposal_authority.evidence_reference.size() >
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
    if (proposal_authority.decision !=
            AdaptiveAuthorityDecision::authorized ||
        authority.decision != AdaptiveAuthorityDecision::authorized ||
        proposal.proposal_authority_reference !=
            proposal_authority.evidence_reference) {
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

AdaptiveTransactionResult AdaptiveStateRegistry::apply_transaction(
    const AdaptiveStateTransaction& transaction,
    const AdaptiveProposalAuthority& proposal_authority,
    const AdaptiveValidationAuthority& validation_authority,
    const AdaptiveApplicationAuthority& application_authority) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        AdaptiveTransactionResult result{};
        result.transaction_id = transaction.transaction_id;
        result.registry_revision_before = registry_revision_;
        result.registry_revision_after = registry_revision_;
        result.application_authority_identity =
            application_authority.authority_identity;
        const auto reject = [&result](AdaptiveTransactionCode code) {
            result.code = code;
            result.message = transaction_message(code);
        };

        const auto valid_identity = [this](const std::string& value) {
            return is_valid_adaptive_identifier(
                value, limits_.maximum_identifier_length);
        };
        if (!limits_valid_ ||
            transaction.proposals.size() >
                limits_.maximum_proposals_per_transaction) {
            reject(AdaptiveTransactionCode::resource_exhausted);
            return result;
        }
        if (!valid_identity(transaction.transaction_id) ||
            !valid_identity(transaction.coordinator_identity) ||
            !valid_identity(application_authority.authority_identity) ||
            application_authority.evidence_reference.size() >
                limits_.maximum_metadata_length ||
            transaction.proposals.empty()) {
            reject(AdaptiveTransactionCode::invalid_transaction);
            return result;
        }
        if (application_authority.decision !=
            AdaptiveAuthorityDecision::authorized) {
            reject(AdaptiveTransactionCode::authority_denied);
            return result;
        }
        if (std::find(
                processed_transaction_ids_.begin(),
                processed_transaction_ids_.end(),
                transaction.transaction_id) !=
            processed_transaction_ids_.end()) {
            reject(AdaptiveTransactionCode::duplicate);
            return result;
        }
        if (transaction.expected_registry_revision.has_value() &&
            *transaction.expected_registry_revision != registry_revision_) {
            reject(AdaptiveTransactionCode::version_conflict);
            return result;
        }
        if (registry_revision_ == std::numeric_limits<std::uint64_t>::max()) {
            reject(AdaptiveTransactionCode::resource_exhausted);
            return result;
        }

        for (std::size_t left = 0; left < transaction.proposals.size();
             ++left) {
            for (std::size_t right = left + 1;
                 right < transaction.proposals.size();
                 ++right) {
                if (transaction.proposals[left].proposal_id ==
                        transaction.proposals[right].proposal_id ||
                    transaction.proposals[left].target_variable_id ==
                        transaction.proposals[right].target_variable_id) {
                    reject(AdaptiveTransactionCode::duplicate);
                    return result;
                }
            }
        }

        std::vector<PreparedMutation> prepared{};
        prepared.reserve(transaction.proposals.size());
        result.proposal_results.reserve(transaction.proposals.size());
        bool all_valid = true;
        for (const AdaptiveMutationProposal& proposal :
             transaction.proposals) {
            prepared.push_back(
                validate_mutation_locked(
                    proposal,
                    proposal_authority,
                    validation_authority));
            result.proposal_results.push_back(prepared.back().validation);
            all_valid = all_valid && prepared.back().validation.accepted();
        }

        AdaptiveTransactionSummary summary{};
        summary.transaction_id = transaction.transaction_id;
        summary.proposal_count = transaction.proposals.size();
        summary.registry_revision_before = registry_revision_;
        summary.registry_revision_after = registry_revision_;
        summary.coordinator_identity = transaction.coordinator_identity;

        std::vector<std::string> next_transaction_ids =
            processed_transaction_ids_;
        std::vector<std::string> next_proposal_ids = processed_proposal_ids_;
        std::vector<AdaptiveTransactionSummary> next_history =
            transaction_history_;
        append_bounded(
            next_transaction_ids,
            transaction.transaction_id,
            limits_.maximum_history_entries);
        for (const AdaptiveMutationProposal& proposal :
             transaction.proposals) {
            append_bounded(
                next_proposal_ids,
                proposal.proposal_id,
                limits_.maximum_history_entries);
        }

        if (!all_valid) {
            reject(AdaptiveTransactionCode::rejected);
            summary.code = result.code;
            append_bounded(
                next_history,
                std::move(summary),
                limits_.maximum_history_entries);
            processed_transaction_ids_.swap(next_transaction_ids);
            processed_proposal_ids_.swap(next_proposal_ids);
            transaction_history_.swap(next_history);
            return result;
        }

        struct CommitRecord {
            std::size_t variable_index = 0;
            AdaptiveValue value{false};
            std::string mutation_id{};
            std::string source_identity{};
            std::string validation_evidence{};
        };
        std::vector<CommitRecord> commits{};
        commits.reserve(prepared.size());
        result.changed_variables.reserve(prepared.size());
        for (std::size_t index = 0; index < prepared.size(); ++index) {
            const AdaptiveMutationProposal& proposal =
                transaction.proposals[index];
            const auto variable = std::find_if(
                variables_.begin(),
                variables_.end(),
                [&proposal](const VariableRecord& record) {
                    return record.descriptor.variable_id ==
                           proposal.target_variable_id;
                });
            if (variable == variables_.end()) {
                reject(AdaptiveTransactionCode::rejected);
                return result;
            }
            const std::size_t variable_index =
                static_cast<std::size_t>(
                    std::distance(variables_.begin(), variable));
            CommitRecord commit{};
            commit.variable_index = variable_index;
            commit.value = std::move(prepared[index].resulting_value);
            commit.mutation_id = proposal.proposal_id;
            commit.source_identity = proposal.source_identity;
            commit.validation_evidence =
                prepared[index].validation.evidence_reference;
            commits.push_back(std::move(commit));
            result.changed_variables.push_back(
                {proposal.target_variable_id,
                 variable->version,
                 variable->version + 1});
        }

        const std::uint64_t new_revision = registry_revision_ + 1;
        reject(AdaptiveTransactionCode::applied);
        result.registry_revision_after = new_revision;
        result.application_occurred = true;
        summary.code = result.code;
        summary.registry_revision_after = new_revision;
        summary.application_occurred = true;
        append_bounded(
            next_history,
            std::move(summary),
            limits_.maximum_history_entries);

        for (CommitRecord& commit : commits) {
            VariableRecord& variable = variables_[commit.variable_index];
            variable.value = std::move(commit.value);
            ++variable.version;
            variable.last_mutation_id = std::move(commit.mutation_id);
            variable.last_source_identity =
                std::move(commit.source_identity);
            variable.last_update_category =
                AdaptiveUpdateCategory::mutation;
            variable.last_validation_evidence =
                std::move(commit.validation_evidence);
            variable.condition = AdaptiveVariableCondition::ready;
            variable.last_changed_registry_revision = new_revision;
        }
        registry_revision_ = new_revision;
        processed_transaction_ids_.swap(next_transaction_ids);
        processed_proposal_ids_.swap(next_proposal_ids);
        transaction_history_.swap(next_history);
        return result;
    } catch (const std::bad_alloc&) {
        AdaptiveTransactionResult result{};
        result.code = AdaptiveTransactionCode::resource_exhausted;
        result.message = transaction_message(result.code);
        return result;
    } catch (...) {
        AdaptiveTransactionResult result{};
        result.code = AdaptiveTransactionCode::rejected;
        result.message = "Adaptive-state transaction failed safely";
        return result;
    }
}

AdaptiveCheckpointResult AdaptiveStateRegistry::create_checkpoint(
    const AdaptiveCheckpointRequest& request,
    const AdaptiveValidationAuthority& authority) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        AdaptiveCheckpointResult result{};
        result.summary.checkpoint_id = request.checkpoint_id;
        result.summary.creation_source = request.creation_source;
        result.summary.reason = request.reason;
        result.summary.registry_revision = registry_revision_;
        const auto reject = [&result](AdaptiveCheckpointCode code) {
            result.code = code;
            result.message = checkpoint_message(code);
            result.summary.validation_status =
                AdaptiveCheckpointValidationStatus::rejected;
        };
        const auto valid_identity = [this](const std::string& value) {
            return is_valid_adaptive_identifier(
                value, limits_.maximum_identifier_length);
        };
        if (!limits_valid_) {
            reject(AdaptiveCheckpointCode::resource_exhausted);
            return result;
        }
        if (!valid_identity(request.checkpoint_id) ||
            !valid_identity(request.creation_source) ||
            !valid_identity(authority.authority_identity) ||
            request.reason == AdaptiveCheckpointReason::unknown ||
            request.metadata_reference.size() >
                limits_.maximum_metadata_length ||
            authority.evidence_reference.size() >
                limits_.maximum_metadata_length) {
            reject(AdaptiveCheckpointCode::invalid_request);
            return result;
        }
        if (authority.decision != AdaptiveAuthorityDecision::authorized) {
            reject(AdaptiveCheckpointCode::authority_denied);
            return result;
        }
        if (std::find_if(
                checkpoints_.begin(),
                checkpoints_.end(),
                [&request](const CheckpointRecord& checkpoint) {
                    return checkpoint.summary.checkpoint_id ==
                           request.checkpoint_id;
                }) != checkpoints_.end()) {
            reject(AdaptiveCheckpointCode::duplicate);
            return result;
        }
        if (checkpoints_.size() >= limits_.maximum_checkpoints) {
            reject(AdaptiveCheckpointCode::capacity_exhausted);
            return result;
        }

        CheckpointRecord checkpoint{};
        checkpoint.summary = result.summary;
        checkpoint.metadata_reference = request.metadata_reference;
        checkpoint.variables.reserve(variables_.size());
        std::size_t estimated_bytes =
            sizeof(CheckpointRecord) + request.checkpoint_id.size() +
            request.creation_source.size() + request.metadata_reference.size();
        for (const VariableRecord& variable : variables_) {
            if (!variable.descriptor.rollback_eligible ||
                variable.descriptor.persistence !=
                    AdaptivePersistenceClass::checkpoint_eligible) {
                continue;
            }
            const std::size_t variable_bytes =
                sizeof(CheckpointVariable) +
                variable.descriptor.variable_id.size() +
                adaptive_value_bytes(variable.value);
            if (estimated_bytes >
                std::numeric_limits<std::size_t>::max() - variable_bytes) {
                reject(AdaptiveCheckpointCode::resource_exhausted);
                return result;
            }
            estimated_bytes += variable_bytes;
            checkpoint.variables.push_back(
                {variable.descriptor.variable_id,
                 variable.value,
                 variable.version,
                 variable.descriptor.descriptor_version,
                 variable.descriptor.schema_version});
        }
        if (checkpoint.variables.empty()) {
            reject(AdaptiveCheckpointCode::no_eligible_variables);
            return result;
        }
        std::sort(
            checkpoint.variables.begin(),
            checkpoint.variables.end(),
            [](const CheckpointVariable& left,
               const CheckpointVariable& right) {
                return left.variable_id < right.variable_id;
            });
        if (estimated_bytes > limits_.maximum_checkpoint_bytes ||
            checkpoint_bytes_ >
                limits_.maximum_checkpoint_bytes - estimated_bytes) {
            reject(AdaptiveCheckpointCode::resource_exhausted);
            return result;
        }

        checkpoint.summary.variable_count = checkpoint.variables.size();
        checkpoint.summary.estimated_bytes = estimated_bytes;
        checkpoint.summary.validation_status =
            AdaptiveCheckpointValidationStatus::validated;
        result.summary = checkpoint.summary;
        result.code = AdaptiveCheckpointCode::created;
        result.message = checkpoint_message(result.code);
        checkpoints_.push_back(std::move(checkpoint));
        checkpoint_bytes_ += estimated_bytes;
        return result;
    } catch (const std::bad_alloc&) {
        AdaptiveCheckpointResult result{};
        result.code = AdaptiveCheckpointCode::resource_exhausted;
        result.message = checkpoint_message(result.code);
        return result;
    } catch (...) {
        AdaptiveCheckpointResult result{};
        result.code = AdaptiveCheckpointCode::invalid_request;
        result.message = "Checkpoint creation failed safely";
        return result;
    }
}

AdaptiveRollbackResult AdaptiveStateRegistry::rollback(
    const AdaptiveRollbackRequest& request,
    const AdaptiveRollbackAuthority& authority) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        AdaptiveRollbackResult result{};
        result.rollback_id = request.rollback_id;
        result.checkpoint_id = request.checkpoint_id;
        result.registry_revision_before = registry_revision_;
        result.registry_revision_after = registry_revision_;
        const auto reject = [&result](AdaptiveRollbackCode code) {
            result.code = code;
            result.message = rollback_message(code);
        };
        const auto valid_identity = [this](const std::string& value) {
            return is_valid_adaptive_identifier(
                value, limits_.maximum_identifier_length);
        };
        if (!limits_valid_) {
            reject(AdaptiveRollbackCode::resource_exhausted);
            return result;
        }
        if (!valid_identity(request.rollback_id) ||
            !valid_identity(request.checkpoint_id) ||
            !valid_identity(request.source_identity) ||
            !valid_identity(authority.authority_identity) ||
            request.reason_reference.size() >
                limits_.maximum_metadata_length ||
            authority.evidence_reference.size() >
                limits_.maximum_metadata_length) {
            reject(AdaptiveRollbackCode::invalid_request);
            return result;
        }
        if (authority.decision != AdaptiveAuthorityDecision::authorized) {
            reject(AdaptiveRollbackCode::authority_denied);
            return result;
        }
        if (std::find(
                processed_rollback_ids_.begin(),
                processed_rollback_ids_.end(),
                request.rollback_id) != processed_rollback_ids_.end()) {
            reject(AdaptiveRollbackCode::duplicate);
            return result;
        }
        if (request.expected_registry_revision.has_value() &&
            *request.expected_registry_revision != registry_revision_) {
            reject(AdaptiveRollbackCode::version_conflict);
            return result;
        }
        const auto checkpoint = std::find_if(
            checkpoints_.begin(),
            checkpoints_.end(),
            [&request](const CheckpointRecord& record) {
                return record.summary.checkpoint_id == request.checkpoint_id;
            });
        if (checkpoint == checkpoints_.end()) {
            reject(AdaptiveRollbackCode::checkpoint_not_found);
            return result;
        }
        if ((request.expected_checkpoint_registry_revision.has_value() &&
             *request.expected_checkpoint_registry_revision !=
                 checkpoint->summary.registry_revision) ||
            checkpoint->summary.validation_status !=
                AdaptiveCheckpointValidationStatus::validated) {
            reject(AdaptiveRollbackCode::incompatible_checkpoint);
            return result;
        }
        if (registry_revision_ == std::numeric_limits<std::uint64_t>::max()) {
            reject(AdaptiveRollbackCode::resource_exhausted);
            return result;
        }

        struct RestoreRecord {
            std::size_t variable_index = 0;
            AdaptiveValue value{false};
            std::string mutation_id{};
            std::string source_identity{};
            std::string validation_evidence{};
        };
        std::vector<RestoreRecord> restores{};
        restores.reserve(checkpoint->variables.size());
        result.changed_variables.reserve(checkpoint->variables.size());
        for (const CheckpointVariable& checkpoint_variable :
             checkpoint->variables) {
            const auto variable = std::find_if(
                variables_.begin(),
                variables_.end(),
                [&checkpoint_variable](const VariableRecord& record) {
                    return record.descriptor.variable_id ==
                           checkpoint_variable.variable_id;
                });
            if (variable == variables_.end() ||
                !variable->descriptor.rollback_eligible ||
                variable->descriptor.persistence !=
                    AdaptivePersistenceClass::checkpoint_eligible ||
                variable->descriptor.descriptor_version !=
                    checkpoint_variable.descriptor_version ||
                variable->descriptor.schema_version !=
                    checkpoint_variable.schema_version ||
                !adaptive_value_in_bounds(
                    checkpoint_variable.value,
                    variable->descriptor.hard_bounds) ||
                (variable->descriptor.soft_bounds.enabled &&
                 !adaptive_value_in_bounds(
                     checkpoint_variable.value,
                     variable->descriptor.soft_bounds))) {
                reject(AdaptiveRollbackCode::incompatible_checkpoint);
                return result;
            }
            if (variable->version ==
                std::numeric_limits<std::uint64_t>::max()) {
                reject(AdaptiveRollbackCode::resource_exhausted);
                return result;
            }
            const std::size_t variable_index =
                static_cast<std::size_t>(
                    std::distance(variables_.begin(), variable));
            RestoreRecord restore{};
            restore.variable_index = variable_index;
            restore.value = checkpoint_variable.value;
            restore.mutation_id = request.rollback_id;
            restore.source_identity = request.source_identity;
            restore.validation_evidence = request.checkpoint_id;
            restores.push_back(std::move(restore));
            result.changed_variables.push_back(
                {checkpoint_variable.variable_id,
                 variable->version,
                 variable->version + 1});
        }

        std::vector<std::string> next_rollback_ids =
            processed_rollback_ids_;
        std::vector<AdaptiveRollbackSummary> next_history =
            rollback_history_;
        append_bounded(
            next_rollback_ids,
            request.rollback_id,
            limits_.maximum_history_entries);
        AdaptiveRollbackSummary summary{};
        summary.rollback_id = request.rollback_id;
        summary.checkpoint_id = request.checkpoint_id;
        summary.code = AdaptiveRollbackCode::restored;
        summary.variable_count = restores.size();
        summary.registry_revision_before = registry_revision_;
        summary.registry_revision_after = registry_revision_ + 1;
        summary.restoration_occurred = true;
        summary.source_identity = request.source_identity;
        append_bounded(
            next_history,
            std::move(summary),
            limits_.maximum_history_entries);

        const std::uint64_t new_revision = registry_revision_ + 1;
        result.code = AdaptiveRollbackCode::restored;
        result.message = rollback_message(result.code);
        result.registry_revision_after = new_revision;
        result.restoration_occurred = true;
        for (RestoreRecord& restore : restores) {
            VariableRecord& variable = variables_[restore.variable_index];
            variable.value = std::move(restore.value);
            ++variable.version;
            variable.last_mutation_id = std::move(restore.mutation_id);
            variable.last_source_identity =
                std::move(restore.source_identity);
            variable.last_update_category = AdaptiveUpdateCategory::rollback;
            variable.last_validation_evidence =
                std::move(restore.validation_evidence);
            variable.condition = AdaptiveVariableCondition::ready;
            variable.last_changed_registry_revision = new_revision;
        }
        registry_revision_ = new_revision;
        processed_rollback_ids_.swap(next_rollback_ids);
        rollback_history_.swap(next_history);
        return result;
    } catch (const std::bad_alloc&) {
        AdaptiveRollbackResult result{};
        result.code = AdaptiveRollbackCode::resource_exhausted;
        result.message = rollback_message(result.code);
        return result;
    } catch (...) {
        AdaptiveRollbackResult result{};
        result.code = AdaptiveRollbackCode::invalid_request;
        result.message = "Rollback failed safely";
        return result;
    }
}

std::vector<AdaptiveCheckpointSummary>
AdaptiveStateRegistry::checkpoint_summaries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AdaptiveCheckpointSummary> result{};
    result.reserve(checkpoints_.size());
    for (const CheckpointRecord& checkpoint : checkpoints_) {
        result.push_back(checkpoint.summary);
    }
    std::sort(
        result.begin(),
        result.end(),
        [](const AdaptiveCheckpointSummary& left,
           const AdaptiveCheckpointSummary& right) {
            return left.checkpoint_id < right.checkpoint_id;
        });
    return result;
}

std::vector<AdaptiveTransactionSummary>
AdaptiveStateRegistry::recent_transaction_history() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return transaction_history_;
}

std::vector<AdaptiveRollbackSummary>
AdaptiveStateRegistry::recent_rollback_history() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return rollback_history_;
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

const char* to_string(AdaptiveTransactionCode value) noexcept {
    switch (value) {
        case AdaptiveTransactionCode::applied: return "applied";
        case AdaptiveTransactionCode::rejected: return "rejected";
        case AdaptiveTransactionCode::version_conflict:
            return "version_conflict";
        case AdaptiveTransactionCode::authority_denied:
            return "authority_denied";
        case AdaptiveTransactionCode::duplicate: return "duplicate";
        case AdaptiveTransactionCode::invalid_transaction:
            return "invalid_transaction";
        case AdaptiveTransactionCode::resource_exhausted:
            return "resource_exhausted";
        case AdaptiveTransactionCode::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(AdaptiveCheckpointReason value) noexcept {
    switch (value) {
        case AdaptiveCheckpointReason::operator_requested:
            return "operator_requested";
        case AdaptiveCheckpointReason::before_controlled_change:
            return "before_controlled_change";
        case AdaptiveCheckpointReason::recovery_baseline:
            return "recovery_baseline";
        case AdaptiveCheckpointReason::test_evidence:
            return "test_evidence";
        case AdaptiveCheckpointReason::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(AdaptiveCheckpointCode value) noexcept {
    switch (value) {
        case AdaptiveCheckpointCode::created: return "created";
        case AdaptiveCheckpointCode::invalid_request:
            return "invalid_request";
        case AdaptiveCheckpointCode::authority_denied:
            return "authority_denied";
        case AdaptiveCheckpointCode::duplicate: return "duplicate";
        case AdaptiveCheckpointCode::capacity_exhausted:
            return "capacity_exhausted";
        case AdaptiveCheckpointCode::resource_exhausted:
            return "resource_exhausted";
        case AdaptiveCheckpointCode::no_eligible_variables:
            return "no_eligible_variables";
        case AdaptiveCheckpointCode::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(AdaptiveRollbackCode value) noexcept {
    switch (value) {
        case AdaptiveRollbackCode::restored: return "restored";
        case AdaptiveRollbackCode::invalid_request:
            return "invalid_request";
        case AdaptiveRollbackCode::authority_denied:
            return "authority_denied";
        case AdaptiveRollbackCode::duplicate: return "duplicate";
        case AdaptiveRollbackCode::checkpoint_not_found:
            return "checkpoint_not_found";
        case AdaptiveRollbackCode::incompatible_checkpoint:
            return "incompatible_checkpoint";
        case AdaptiveRollbackCode::version_conflict:
            return "version_conflict";
        case AdaptiveRollbackCode::resource_exhausted:
            return "resource_exhausted";
        case AdaptiveRollbackCode::unknown: return "unknown";
    }
    return "unknown";
}

}  // namespace prometheus::core
