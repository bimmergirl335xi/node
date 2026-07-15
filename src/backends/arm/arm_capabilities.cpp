#include "arm_capabilities.hpp"

#include <utility>

#include "arm_linux_auxv.hpp"

namespace prometheus::backends::arm {
namespace {

void add_issue(
    ArmCapabilitySnapshot& snapshot,
    ArmCapabilityIssueCode code,
    int os_cpu_id,
    bool fatal,
    std::string message) {
    ArmCapabilityIssue issue{};
    issue.code = code;
    issue.os_cpu_id = os_cpu_id;
    issue.fatal = fatal;
    issue.message = std::move(message);
    snapshot.issues.push_back(std::move(issue));
}

[[nodiscard]] ArmExecutionState execution_state_from_cpu_architecture(
    cpu::CpuArchitectureFamily architecture) noexcept {
    switch (architecture) {
        case cpu::CpuArchitectureFamily::aarch64:
            return ArmExecutionState::aarch64;
        case cpu::CpuArchitectureFamily::armv7:
            return ArmExecutionState::aarch32;
        case cpu::CpuArchitectureFamily::x86_64:
        case cpu::CpuArchitectureFamily::riscv64:
        case cpu::CpuArchitectureFamily::ppc64le:
            return ArmExecutionState::not_arm;
        case cpu::CpuArchitectureFamily::unknown:
            return ArmExecutionState::unknown;
    }

    return ArmExecutionState::unknown;
}

[[nodiscard]] ArmExecutionProfile select_common_profile(
    const ArmIsaCapabilities& isa) noexcept {
    if (isa.execution_state == ArmExecutionState::not_arm) {
        return ArmExecutionProfile::not_arm;
    }

    if (isa.execution_state == ArmExecutionState::unknown) {
        return ArmExecutionProfile::unknown;
    }

    if (isa.execution_state == ArmExecutionState::aarch64) {
        if (isa.sve2.safe_for_all_targets()) {
            return ArmExecutionProfile::sve2;
        }

        if (isa.sve.safe_for_all_targets()) {
            return ArmExecutionProfile::sve;
        }
    }

    if (isa.i8mm.safe_for_all_targets()) {
        return ArmExecutionProfile::advanced_simd_i8mm;
    }

    if (isa.dot_product.safe_for_all_targets()) {
        return ArmExecutionProfile::advanced_simd_dotprod;
    }

    if (isa.advanced_simd.safe_for_all_targets() &&
        isa.fp16_vector.safe_for_all_targets()) {
        return ArmExecutionProfile::advanced_simd_fp16;
    }

    if (isa.advanced_simd.safe_for_all_targets()) {
        return ArmExecutionProfile::advanced_simd;
    }

    return ArmExecutionProfile::scalar;
}

void copy_generic_cpu_coverage(
    const cpu::CpuIsaCapabilities& cpu_isa,
    ArmIsaCapabilities& arm_isa) {
    arm_isa.feature_sets_heterogeneous =
        cpu_isa.feature_sets_heterogeneous;

    arm_isa.floating_point = cpu_isa.fp32;
    arm_isa.advanced_simd = cpu_isa.neon;
    arm_isa.fp16_scalar = cpu_isa.fp16;
    arm_isa.fp16_vector = cpu_isa.fp16;
    arm_isa.bfloat16 = cpu_isa.bfloat16;

    arm_isa.atomics = cpu_isa.arm_atomics;
    arm_isa.crc32 = cpu_isa.crc32;

    arm_isa.aes = cpu_isa.aes;
    arm_isa.sha1 = cpu_isa.sha1;
    arm_isa.sha2 = cpu_isa.sha2;

    arm_isa.sve = cpu_isa.sve;
    arm_isa.sve2 = cpu_isa.sve2;
}

}  // namespace

ArmExecutionState compile_time_arm_execution_state() noexcept {
#if defined(__aarch64__) || defined(_M_ARM64)
    return ArmExecutionState::aarch64;
#elif defined(__arm__) || defined(_M_ARM)
    return ArmExecutionState::aarch32;
#elif defined(__x86_64__) || defined(_M_X64) || \
      defined(__i386__) || defined(_M_IX86) || \
      defined(__riscv) || defined(__powerpc__) || \
      defined(__powerpc64__)
    return ArmExecutionState::not_arm;
#else
    return ArmExecutionState::unknown;
#endif
}

ArmCapabilityQueryResult query_arm_capabilities(
    const cpu::CpuCapabilitySnapshot& cpu_snapshot) {
    ArmCapabilityQueryResult result{};
    ArmCapabilitySnapshot& snapshot = result.capabilities;
    ArmIsaCapabilities& isa = snapshot.isa;

    snapshot.target_logical_processor_count =
        cpu_snapshot.target_logical_processor_count;
    snapshot.target_logical_processor_ids =
        cpu_snapshot.target_logical_processor_ids;

    isa.execution_state =
        execution_state_from_cpu_architecture(cpu_snapshot.isa.architecture);
    isa.source = ArmCapabilitySource::cpu_snapshot;

    if (isa.execution_state == ArmExecutionState::not_arm) {
        isa.common_profile = ArmExecutionProfile::not_arm;

        add_issue(
            snapshot,
            ArmCapabilityIssueCode::not_arm_architecture,
            -1,
            false,
            "generic CPU capability snapshot describes a non-ARM processor");

        result.status.code =
            ArmCapabilityQueryCode::unsupported_architecture;
        result.status.message =
            "ARM capability enrichment is not applicable to this CPU";
        return result;
    }

    if (isa.execution_state == ArmExecutionState::unknown) {
        add_issue(
            snapshot,
            ArmCapabilityIssueCode::execution_state_unknown,
            -1,
            true,
            "generic CPU architecture does not establish AArch32 or AArch64");

        result.status.code = ArmCapabilityQueryCode::query_failed;
        result.status.message =
            "ARM execution state could not be established";
        return result;
    }

    if (snapshot.target_logical_processor_count == 0 ||
        snapshot.target_logical_processor_ids.empty()) {
        add_issue(
            snapshot,
            ArmCapabilityIssueCode::cpu_snapshot_incomplete,
            -1,
            true,
            "generic CPU snapshot contains no target logical processors");

        result.status.code = ArmCapabilityQueryCode::query_failed;
        result.status.message =
            "ARM capability enrichment requires CPU target processors";
        return result;
    }

    copy_generic_cpu_coverage(cpu_snapshot.isa, isa);

    const ArmLinuxAuxvObservation auxv =
        query_linux_arm_auxv_observation(isa.execution_state);
    const bool auxv_applied = apply_linux_arm_auxv_observation(
        auxv,
        snapshot.target_logical_processor_count,
        isa);

    if (auxv_applied) {
        isa.source = ArmCapabilitySource::combined;
    } else if (!auxv.provider_available) {
        add_issue(
            snapshot,
            ArmCapabilityIssueCode::auxiliary_vector_unavailable,
            -1,
            false,
            "Linux ARM auxiliary-vector provider is unavailable on this build");
    } else {
        add_issue(
            snapshot,
            ArmCapabilityIssueCode::auxiliary_vector_query_failed,
            -1,
            false,
            "Linux ARM auxiliary-vector provider returned no usable capability record");
    }

    isa.common_profile = select_common_profile(isa);

    if (isa.sve.any == cpu::CpuSupportState::supported &&
        !isa.sve_vector_length_observed) {
        add_issue(
            snapshot,
            ArmCapabilityIssueCode::sve_vector_length_unavailable,
            -1,
            false,
            "SVE is reported but its runtime vector length is unavailable");
    }

    result.status.code = ArmCapabilityQueryCode::partial_success;
    result.status.message = auxv_applied
        ? "ARM capabilities combine the generic CPU snapshot with Linux auxiliary-vector observations"
        : "ARM capabilities were conservatively derived from the generic CPU snapshot";
    return result;
}

const char* to_string(ArmExecutionState value) noexcept {
    switch (value) {
        case ArmExecutionState::unknown:
            return "unknown";
        case ArmExecutionState::not_arm:
            return "not_arm";
        case ArmExecutionState::aarch32:
            return "aarch32";
        case ArmExecutionState::aarch64:
            return "aarch64";
    }

    return "unknown";
}

const char* to_string(ArmCapabilitySource value) noexcept {
    switch (value) {
        case ArmCapabilitySource::none:
            return "none";
        case ArmCapabilitySource::compile_time:
            return "compile_time";
        case ArmCapabilitySource::cpu_snapshot:
            return "cpu_snapshot";
        case ArmCapabilitySource::linux_auxv:
            return "linux_auxv";
        case ArmCapabilitySource::proc_cpuinfo:
            return "proc_cpuinfo";
        case ArmCapabilitySource::system_register:
            return "system_register";
        case ArmCapabilitySource::combined:
            return "combined";
    }

    return "unknown";
}

const char* to_string(ArmExecutionProfile value) noexcept {
    switch (value) {
        case ArmExecutionProfile::unknown:
            return "unknown";
        case ArmExecutionProfile::not_arm:
            return "not_arm";
        case ArmExecutionProfile::scalar:
            return "scalar";
        case ArmExecutionProfile::advanced_simd:
            return "advanced_simd";
        case ArmExecutionProfile::advanced_simd_fp16:
            return "advanced_simd_fp16";
        case ArmExecutionProfile::advanced_simd_dotprod:
            return "advanced_simd_dotprod";
        case ArmExecutionProfile::advanced_simd_i8mm:
            return "advanced_simd_i8mm";
        case ArmExecutionProfile::sve:
            return "sve";
        case ArmExecutionProfile::sve2:
            return "sve2";
    }

    return "unknown";
}

const char* to_string(ArmCapabilityIssueCode value) noexcept {
    switch (value) {
        case ArmCapabilityIssueCode::not_arm_architecture:
            return "not_arm_architecture";
        case ArmCapabilityIssueCode::cpu_snapshot_incomplete:
            return "cpu_snapshot_incomplete";
        case ArmCapabilityIssueCode::execution_state_unknown:
            return "execution_state_unknown";
        case ArmCapabilityIssueCode::identity_unavailable:
            return "identity_unavailable";
        case ArmCapabilityIssueCode::auxiliary_vector_unavailable:
            return "auxiliary_vector_unavailable";
        case ArmCapabilityIssueCode::auxiliary_vector_query_failed:
            return "auxiliary_vector_query_failed";
        case ArmCapabilityIssueCode::processor_record_missing:
            return "processor_record_missing";
        case ArmCapabilityIssueCode::feature_record_missing:
            return "feature_record_missing";
        case ArmCapabilityIssueCode::sve_vector_length_unavailable:
            return "sve_vector_length_unavailable";
    }

    return "unknown";
}

const char* to_string(ArmCapabilityQueryCode value) noexcept {
    switch (value) {
        case ArmCapabilityQueryCode::success:
            return "success";
        case ArmCapabilityQueryCode::partial_success:
            return "partial_success";
        case ArmCapabilityQueryCode::unsupported_architecture:
            return "unsupported_architecture";
        case ArmCapabilityQueryCode::unsupported_platform:
            return "unsupported_platform";
        case ArmCapabilityQueryCode::query_failed:
            return "query_failed";
    }

    return "unknown";
}

}  // namespace prometheus::backends::arm
