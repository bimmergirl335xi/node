#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <atomic>
#include <future>
#include <limits>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/adaptive_state.hpp"
#include "backends/backend_registry.hpp"

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

[[nodiscard]] pc::AdaptiveProposalAuthority proposal_authority() {
    pc::AdaptiveProposalAuthority authority{};
    authority.decision = pc::AdaptiveAuthorityDecision::authorized;
    authority.authority_identity = "runtime.proposal.policy";
    authority.evidence_reference = "evidence.proposal";
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
    result.proposal_authority_reference = "evidence.proposal";
    result.correlation_id = "correlation.one";
    result.expected_variable_version = 1;
    result.operation = operation;
    result.operand = std::move(operand);
    return result;
}

[[nodiscard]] pc::AdaptiveApplicationAuthority application_authority() {
    pc::AdaptiveApplicationAuthority authority{};
    authority.decision = pc::AdaptiveAuthorityDecision::authorized;
    authority.authority_identity = "runtime.applicator";
    authority.evidence_reference = "evidence.application";
    return authority;
}

[[nodiscard]] pc::AdaptiveRollbackAuthority rollback_authority() {
    pc::AdaptiveRollbackAuthority authority{};
    authority.decision = pc::AdaptiveAuthorityDecision::authorized;
    authority.authority_identity = "runtime.rollback";
    authority.evidence_reference = "evidence.rollback";
    return authority;
}

[[nodiscard]] pc::AdaptiveStateTransaction transaction(
    std::string id,
    std::vector<pc::AdaptiveMutationProposal> proposals) {
    pc::AdaptiveStateTransaction result{};
    result.transaction_id = std::move(id);
    result.coordinator_identity = "runtime.coordinator";
    result.proposals = std::move(proposals);
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

    const auto proposer = proposal_authority();
    const auto validator = validation_authority();
    const auto boolean_result = registry.validate_mutation(
        proposal(
            "proposal.boolean",
            "scalar.boolean",
            true,
            pc::AdaptiveMutationOperation::replace,
            pc::AdaptiveMutationSourceCategory::externally_authorized),
        proposer, validator);
    const auto integer_result = registry.validate_mutation(
        proposal(
            "proposal.integer",
            "scalar.integer",
            std::int64_t{5},
            pc::AdaptiveMutationOperation::add),
        proposer, validator);
    const auto overflow_result = registry.validate_mutation(
        proposal(
            "proposal.overflow",
            "scalar.overflow",
            std::int64_t{1},
            pc::AdaptiveMutationOperation::add),
        proposer, validator);
    const auto float_result = registry.validate_mutation(
        proposal("proposal.float", "scalar.float", 2.5), proposer, validator);
    const auto nan_result = registry.validate_mutation(
        proposal(
            "proposal.nan",
            "scalar.float",
            std::numeric_limits<double>::quiet_NaN()),
        proposer, validator);
    const auto infinity_result = registry.validate_mutation(
        proposal(
            "proposal.infinity",
            "scalar.float",
            std::numeric_limits<double>::infinity()),
        proposer, validator);
    const auto hard_result = registry.validate_mutation(
        proposal(
            "proposal.hard",
            "scalar.integer",
            std::int64_t{100},
            pc::AdaptiveMutationOperation::add),
        proposer, validator);
    const auto soft_result = registry.validate_mutation(
        proposal(
            "proposal.soft",
            "scalar.integer",
            std::int64_t{45},
            pc::AdaptiveMutationOperation::add),
        proposer, validator);
    const auto magnitude_result = registry.validate_mutation(
        proposal(
            "proposal.magnitude",
            "scalar.integer",
            std::int64_t{11},
            pc::AdaptiveMutationOperation::add),
        proposer, validator);
    const auto disabled_result = registry.validate_mutation(
        proposal(
            "proposal.disabled",
            "scalar.disabled",
            std::int64_t{11}),
        proposer, validator);

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
    const auto proposer = proposal_authority();
    const auto validator = validation_authority();
    const auto valid = registry.validate_mutation(
        proposal(
            "proposal.vector.valid",
            "vector.float",
            std::vector<double>{2.0, 3.0, 4.0, 5.0},
            pc::AdaptiveMutationOperation::replace,
            pc::AdaptiveMutationSourceCategory::externally_authorized),
        proposer, validator);
    const auto shape = registry.validate_mutation(
        proposal(
            "proposal.vector.shape",
            "vector.float",
            std::vector<double>{1.0, 2.0},
            pc::AdaptiveMutationOperation::replace,
            pc::AdaptiveMutationSourceCategory::externally_authorized),
        proposer, validator);
    const auto count = registry.validate_mutation(
        proposal(
            "proposal.vector.count",
            "vector.float",
            std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0},
            pc::AdaptiveMutationOperation::replace,
            pc::AdaptiveMutationSourceCategory::externally_authorized),
        proposer, validator);
    const auto range = registry.validate_mutation(
        proposal(
            "proposal.vector.range",
            "vector.float",
            std::vector<double>{1.0, 2.0, 3.0, 11.0},
            pc::AdaptiveMutationOperation::replace,
            pc::AdaptiveMutationSourceCategory::externally_authorized),
        proposer, validator);
    const auto invalid = registry.validate_mutation(
        proposal(
            "proposal.vector.invalid",
            "vector.float",
            std::numeric_limits<double>::infinity(),
            pc::AdaptiveMutationOperation::scale,
            pc::AdaptiveMutationSourceCategory::externally_authorized),
        proposer, validator);
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

    const auto denied_proposal = proposal(
        "proposal.denied",
        "authority.automatic",
        std::int64_t{11});
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
    auto proposer = proposal_authority();
    auto denied_proposer = proposer;
    denied_proposer.decision = pc::AdaptiveAuthorityDecision::denied;
    auto validator = validation_authority();
    auto denied_validator = validator;
    denied_validator.decision = pc::AdaptiveAuthorityDecision::denied;

    const auto automatic_allowed = registry.validate_mutation(
        proposal(
            "proposal.automatic.allowed",
            "authority.automatic",
            std::int64_t{11}),
        proposer, validator);
    const auto automatic_denied = registry.validate_mutation(
        proposal(
            "proposal.automatic.denied",
            "authority.external",
            std::int64_t{11}),
        proposer, validator);
    const auto external_allowed = registry.validate_mutation(
        proposal(
            "proposal.external.allowed",
            "authority.external",
            std::int64_t{11},
            pc::AdaptiveMutationOperation::replace,
            pc::AdaptiveMutationSourceCategory::operator_directed),
        proposer, validator);
    const auto external_denied = registry.validate_mutation(
        proposal(
            "proposal.external.denied",
            "authority.automatic",
            std::int64_t{11},
            pc::AdaptiveMutationOperation::replace,
            pc::AdaptiveMutationSourceCategory::externally_authorized),
        proposer, validator);
    const auto proposal_denied =
        registry.validate_mutation(
            denied_proposal, denied_proposer, validator);
    const auto validation_denied = registry.validate_mutation(
        proposal(
            "proposal.validator.denied",
            "authority.automatic",
            std::int64_t{11}),
        proposer, denied_validator);
    const auto unknown = registry.validate_mutation(
        unknown_source, proposer, validator);
    const auto conflict = registry.validate_mutation(
        stale, proposer, validator);

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

[[nodiscard]] bool test_transactions_and_atomicity() {
    pc::AdaptiveStateRegistry registry{};
    if (!registry.register_variable(integer_descriptor("transaction.a")).ok() ||
        !registry.register_variable(integer_descriptor("transaction.b")).ok()) {
        return false;
    }
    const auto proposer = proposal_authority();
    const auto validator = validation_authority();
    const auto applicator = application_authority();

    auto denied_applicator = applicator;
    denied_applicator.decision = pc::AdaptiveAuthorityDecision::denied;
    const auto denied_application = registry.apply_transaction(
        transaction(
            "transaction.application.denied",
            {proposal(
                "proposal.application.denied",
                "transaction.a",
                std::int64_t{11})}),
        proposer,
        validator,
        denied_applicator);
    if (denied_application.code !=
            pc::AdaptiveTransactionCode::authority_denied ||
        registry.revision() != 2) {
        return false;
    }

    auto input_proposal = proposal(
        "proposal.single",
        "transaction.a",
        std::int64_t{5},
        pc::AdaptiveMutationOperation::add);
    const auto input_copy = input_proposal;
    auto single = transaction(
        "transaction.single", {input_proposal});
    single.expected_registry_revision = registry.revision();
    const auto single_result =
        registry.apply_transaction(single, proposer, validator, applicator);
    const auto after_single = registry.find("transaction.a");
    if (!single_result.applied() || single_result.changed_variables.size() != 1 ||
        single_result.registry_revision_before != 2 ||
        single_result.registry_revision_after != 3 ||
        !after_single.has_value() || after_single->version != 2 ||
        std::get<std::int64_t>(*after_single->value) != 15 ||
        std::get<std::int64_t>(input_proposal.operand) !=
            std::get<std::int64_t>(input_copy.operand)) {
        return false;
    }

    auto invalid_a = proposal(
        "proposal.atomic.a",
        "transaction.a",
        std::int64_t{5},
        pc::AdaptiveMutationOperation::add);
    invalid_a.expected_variable_version = 2;
    auto invalid_b = proposal(
        "proposal.atomic.b",
        "transaction.b",
        std::int64_t{100},
        pc::AdaptiveMutationOperation::add);
    const auto rejected = registry.apply_transaction(
        transaction("transaction.atomic.rejected", {invalid_a, invalid_b}),
        proposer,
        validator,
        applicator);
    const auto unchanged_a = registry.find("transaction.a");
    const auto unchanged_b = registry.find("transaction.b");
    if (rejected.code != pc::AdaptiveTransactionCode::rejected ||
        rejected.application_occurred ||
        rejected.proposal_results.size() != 2 ||
        registry.revision() != 3 || !unchanged_a.has_value() ||
        !unchanged_b.has_value() || unchanged_a->version != 2 ||
        unchanged_b->version != 1 ||
        std::get<std::int64_t>(*unchanged_a->value) != 15 ||
        std::get<std::int64_t>(*unchanged_b->value) != 10) {
        return false;
    }

    auto multi_a = proposal(
        "proposal.multi.a",
        "transaction.a",
        std::int64_t{20});
    multi_a.expected_variable_version = 2;
    auto multi_b = proposal(
        "proposal.multi.b",
        "transaction.b",
        std::int64_t{20});
    auto multi = transaction("transaction.multi", {multi_b, multi_a});
    multi.expected_registry_revision = 3;
    const auto multi_result =
        registry.apply_transaction(multi, proposer, validator, applicator);
    const auto snapshot = registry.snapshot();
    if (!multi_result.applied() || multi_result.changed_variables.size() != 2 ||
        multi_result.changed_variables[0].variable_id != "transaction.b" ||
        multi_result.changed_variables[1].variable_id != "transaction.a" ||
        snapshot.registry_revision != 4 || snapshot.variables.size() != 2 ||
        snapshot.variables[0].version != 3 ||
        snapshot.variables[1].version != 2 ||
        std::get<std::int64_t>(*snapshot.variables[0].value) != 20 ||
        std::get<std::int64_t>(*snapshot.variables[1].value) != 20) {
        return false;
    }

    auto duplicate_target_a = proposal(
        "proposal.duplicate.target.a",
        "transaction.a",
        std::int64_t{21});
    duplicate_target_a.expected_variable_version = 3;
    auto duplicate_target_b = duplicate_target_a;
    duplicate_target_b.proposal_id = "proposal.duplicate.target.b";
    const auto duplicate_target = registry.apply_transaction(
        transaction(
            "transaction.duplicate.target",
            {duplicate_target_a, duplicate_target_b}),
        proposer,
        validator,
        applicator);
    const auto duplicate_transaction =
        registry.apply_transaction(multi, proposer, validator, applicator);
    auto repeated_proposal = proposal(
        "proposal.multi.a", "transaction.a", std::int64_t{21});
    repeated_proposal.expected_variable_version = 3;
    const auto duplicate_proposal = registry.apply_transaction(
        transaction("transaction.duplicate.proposal", {repeated_proposal}),
        proposer,
        validator,
        applicator);
    auto stale = transaction(
        "transaction.stale",
        {proposal(
            "proposal.stale.transaction",
            "transaction.b",
            std::int64_t{21})});
    stale.proposals[0].expected_variable_version = 2;
    stale.expected_registry_revision = 3;
    const auto stale_result =
        registry.apply_transaction(stale, proposer, validator, applicator);

    return duplicate_target.code == pc::AdaptiveTransactionCode::duplicate &&
           duplicate_transaction.code == pc::AdaptiveTransactionCode::duplicate &&
           duplicate_proposal.code == pc::AdaptiveTransactionCode::rejected &&
           duplicate_proposal.proposal_results.size() == 1 &&
           duplicate_proposal.proposal_results[0].code ==
               pc::AdaptiveMutationValidationCode::duplicate &&
           stale_result.code == pc::AdaptiveTransactionCode::version_conflict &&
           registry.revision() == 4 &&
           registry.recent_transaction_history().size() == 4;
}

[[nodiscard]] bool test_checkpoint_and_rollback() {
    pc::AdaptiveStateLimits limits{};
    limits.maximum_checkpoints = 1;
    pc::AdaptiveStateRegistry registry{limits};
    if (!registry.register_variable(integer_descriptor("rollback.value")).ok()) {
        return false;
    }

    pc::AdaptiveCheckpointRequest checkpoint{};
    checkpoint.checkpoint_id = "checkpoint.initial";
    checkpoint.creation_source = "operator.one";
    checkpoint.reason = pc::AdaptiveCheckpointReason::before_controlled_change;
    checkpoint.metadata_reference = "evidence.checkpoint";
    const auto created =
        registry.create_checkpoint(checkpoint, validation_authority());
    const auto duplicate =
        registry.create_checkpoint(checkpoint, validation_authority());
    auto second_checkpoint = checkpoint;
    second_checkpoint.checkpoint_id = "checkpoint.second";
    const auto capacity = registry.create_checkpoint(
        second_checkpoint, validation_authority());
    if (!created.created() || created.summary.variable_count != 1 ||
        duplicate.code != pc::AdaptiveCheckpointCode::duplicate ||
        capacity.code != pc::AdaptiveCheckpointCode::capacity_exhausted ||
        registry.checkpoint_summaries().size() != 1) {
        return false;
    }

    pc::AdaptiveStateLimits byte_limits{};
    byte_limits.maximum_checkpoint_bytes = 1;
    pc::AdaptiveStateRegistry byte_limited_registry{byte_limits};
    if (!byte_limited_registry
             .register_variable(integer_descriptor("rollback.byte.limit"))
             .ok() ||
        byte_limited_registry
                .create_checkpoint(checkpoint, validation_authority())
                .code != pc::AdaptiveCheckpointCode::resource_exhausted) {
        return false;
    }

    const auto mutation = registry.apply_transaction(
        transaction(
            "transaction.before.rollback",
            {proposal(
                "proposal.before.rollback",
                "rollback.value",
                std::int64_t{20})}),
        proposal_authority(),
        validation_authority(),
        application_authority());
    if (!mutation.applied()) {
        return false;
    }

    pc::AdaptiveRollbackRequest request{};
    request.rollback_id = "rollback.first";
    request.checkpoint_id = "checkpoint.initial";
    request.source_identity = "operator.one";
    request.reason_reference = "recovery.request";
    request.expected_checkpoint_registry_revision = 1;
    request.expected_registry_revision = 2;

    auto denied_authority = rollback_authority();
    denied_authority.decision = pc::AdaptiveAuthorityDecision::denied;
    const auto denied = registry.rollback(request, denied_authority);
    auto missing_request = request;
    missing_request.rollback_id = "rollback.missing";
    missing_request.checkpoint_id = "checkpoint.missing";
    const auto missing = registry.rollback(missing_request, rollback_authority());
    auto incompatible_request = request;
    incompatible_request.rollback_id = "rollback.incompatible";
    incompatible_request.expected_checkpoint_registry_revision = 99;
    const auto incompatible =
        registry.rollback(incompatible_request, rollback_authority());
    const auto restored = registry.rollback(request, rollback_authority());
    const auto restored_value = registry.find("rollback.value");
    if (denied.code != pc::AdaptiveRollbackCode::authority_denied ||
        missing.code != pc::AdaptiveRollbackCode::checkpoint_not_found ||
        incompatible.code !=
            pc::AdaptiveRollbackCode::incompatible_checkpoint ||
        !restored.restored() || restored.registry_revision_before != 2 ||
        restored.registry_revision_after != 3 ||
        !restored_value.has_value() || restored_value->version != 3 ||
        std::get<std::int64_t>(*restored_value->value) != 10 ||
        restored_value->last_update_category !=
            pc::AdaptiveUpdateCategory::rollback) {
        return false;
    }

    auto repeated_request = request;
    repeated_request.rollback_id = "rollback.second";
    repeated_request.expected_registry_revision = 3;
    const auto repeated =
        registry.rollback(repeated_request, rollback_authority());
    const auto repeated_value = registry.find("rollback.value");
    const auto duplicate_rollback = registry.rollback(
        repeated_request, rollback_authority());
    return repeated.restored() && repeated_value.has_value() &&
           repeated_value->version == 4 && registry.revision() == 4 &&
           duplicate_rollback.code == pc::AdaptiveRollbackCode::duplicate &&
           registry.recent_rollback_history().size() == 2;
}

[[nodiscard]] bool test_bounded_history_and_separate_registries() {
    pc::AdaptiveStateLimits limits{};
    limits.maximum_history_entries = 2;
    pc::AdaptiveStateRegistry registry{limits};
    prometheus::backends::BackendRegistry backends{};
    if (!registry.register_variable(integer_descriptor("history.value")).ok() ||
        backends.size() != 0) {
        return false;
    }
    for (std::uint64_t index = 0; index < 3; ++index) {
        auto update = proposal(
            "proposal.history." + std::to_string(index),
            "history.value",
            std::int64_t{11 + static_cast<std::int64_t>(index)});
        update.expected_variable_version = index + 1;
        if (!registry.apply_transaction(
                transaction(
                    "transaction.history." + std::to_string(index),
                    {update}),
                proposal_authority(),
                validation_authority(),
                application_authority())
                 .applied()) {
            return false;
        }
    }
    const auto value = registry.find("history.value");
    return registry.recent_transaction_history().size() == 2 &&
           value.has_value() && value->version == 4 &&
           backends.size() == 0;
}

[[nodiscard]] bool test_concurrent_observation_and_validation() {
    pc::AdaptiveStateRegistry registry{};
    if (!registry.register_variable(integer_descriptor("concurrent.value")).ok()) {
        return false;
    }
    std::promise<void> start_promise{};
    std::shared_future<void> start = start_promise.get_future().share();
    std::vector<std::future<bool>> workers{};
    for (std::size_t worker = 0; worker < 4; ++worker) {
        workers.push_back(std::async(
            std::launch::async,
            [&registry, start, worker]() {
                start.wait();
                for (std::size_t iteration = 0; iteration < 100; ++iteration) {
                    const auto snapshot = registry.snapshot();
                    auto candidate = proposal(
                        "proposal.concurrent." + std::to_string(worker) + "." +
                            std::to_string(iteration),
                        "concurrent.value",
                        std::int64_t{11});
                    if (snapshot.variables.size() != 1 ||
                        !registry.validate_mutation(
                                     candidate,
                                     proposal_authority(),
                                     validation_authority())
                             .accepted()) {
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
    return registry.revision() == 1;
}

[[nodiscard]] bool test_serial_commits_and_no_torn_state() {
    pc::AdaptiveStateRegistry registry{};
    auto left_descriptor = integer_descriptor("atomic.left", 0);
    auto right_descriptor = integer_descriptor("atomic.right", 0);
    left_descriptor.soft_bounds.minimum = std::int64_t{-100};
    left_descriptor.soft_bounds.maximum = std::int64_t{100};
    right_descriptor.soft_bounds.minimum = std::int64_t{-100};
    right_descriptor.soft_bounds.maximum = std::int64_t{100};
    if (!registry.register_variable(left_descriptor).ok() ||
        !registry.register_variable(right_descriptor).ok()) {
        return false;
    }

    std::promise<void> start_promise{};
    std::shared_future<void> start = start_promise.get_future().share();
    std::atomic<bool> writer_done{false};
    std::atomic<bool> torn{false};
    auto writer = std::async(std::launch::async, [&]() {
        start.wait();
        for (std::uint64_t iteration = 1; iteration <= 20; ++iteration) {
            auto left = proposal(
                "proposal.atomic.left." + std::to_string(iteration),
                "atomic.left",
                static_cast<std::int64_t>(iteration));
            auto right = proposal(
                "proposal.atomic.right." + std::to_string(iteration),
                "atomic.right",
                static_cast<std::int64_t>(iteration));
            left.expected_variable_version = iteration;
            right.expected_variable_version = iteration;
            if (!registry.apply_transaction(
                    transaction(
                        "transaction.atomic." + std::to_string(iteration),
                        {left, right}),
                    proposal_authority(),
                    validation_authority(),
                    application_authority())
                     .applied()) {
                writer_done.store(true);
                return false;
            }
        }
        writer_done.store(true);
        return true;
    });
    auto reader = std::async(std::launch::async, [&]() {
        start.wait();
        std::size_t observations = 0;
        while (!writer_done.load() || observations < 100) {
            const auto snapshot = registry.snapshot();
            if (snapshot.variables.size() != 2 ||
                std::get<std::int64_t>(*snapshot.variables[0].value) !=
                    std::get<std::int64_t>(*snapshot.variables[1].value)) {
                torn.store(true);
                return false;
            }
            ++observations;
        }
        return true;
    });
    start_promise.set_value();
    const bool writer_ok = writer.get();
    const bool reader_ok = reader.get();
    const auto snapshot = registry.snapshot();
    return writer_ok && reader_ok && !torn.load() &&
           snapshot.registry_revision == 22 &&
           snapshot.variables[0].version == 21 &&
           snapshot.variables[1].version == 21;
}

[[nodiscard]] bool test_concurrent_commits_are_serialized() {
    pc::AdaptiveStateRegistry registry{};
    if (!registry.register_variable(integer_descriptor("serial.left")).ok() ||
        !registry.register_variable(integer_descriptor("serial.right")).ok()) {
        return false;
    }
    std::promise<void> start_promise{};
    std::shared_future<void> start = start_promise.get_future().share();
    auto commit = [&registry, start](const std::string& suffix) {
        start.wait();
        return registry.apply_transaction(
            transaction(
                "transaction.serial." + suffix,
                {proposal(
                    "proposal.serial." + suffix,
                    "serial." + suffix,
                    std::int64_t{12})}),
            proposal_authority(),
            validation_authority(),
            application_authority());
    };
    auto left = std::async(std::launch::async, commit, "left");
    auto right = std::async(std::launch::async, commit, "right");
    start_promise.set_value();
    const auto left_result = left.get();
    const auto right_result = right.get();
    const auto snapshot = registry.snapshot();
    return left_result.applied() && right_result.applied() &&
           left_result.registry_revision_after !=
               right_result.registry_revision_after &&
           snapshot.registry_revision == 4 &&
           snapshot.variables[0].version == 2 &&
           snapshot.variables[1].version == 2;
}

}  // namespace

int main() {
    return test_empty_and_registration() &&
                   test_rejections_and_capacity() &&
                   test_ordering_shape_and_redaction() &&
                   test_scalar_validation() && test_vector_validation() &&
                   test_authority_and_version_validation() &&
                   test_transactions_and_atomicity() &&
                   test_checkpoint_and_rollback() &&
                   test_bounded_history_and_separate_registries() &&
                   test_concurrent_observation_and_validation() &&
                   test_serial_commits_and_no_torn_state() &&
                   test_concurrent_commits_are_serialized()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
