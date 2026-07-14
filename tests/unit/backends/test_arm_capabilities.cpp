#include "arm_capabilities.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace arm = prometheus::backends::arm;
namespace cpu = prometheus::backends::cpu;

namespace {

[[nodiscard]] cpu::CpuFeatureCoverage make_coverage(
    std::size_t target_count,
    cpu::CpuSupportState common,
    cpu::CpuSupportState any) {
    cpu::CpuFeatureCoverage coverage{};
    coverage.common = common;
    coverage.any = any;
    coverage.target_processor_count = target_count;
    coverage.observed_processor_count = target_count;

    if (common == cpu::CpuSupportState::supported) {
        coverage.supported_processor_count = target_count;
    } else if (any == cpu::CpuSupportState::supported && target_count > 0) {
        coverage.supported_processor_count = 1;
    }

    return coverage;
}

[[nodiscard]] cpu::CpuCapabilitySnapshot make_snapshot(
    cpu::CpuArchitectureFamily architecture,
    std::size_t processor_count) {
    cpu::CpuCapabilitySnapshot snapshot{};
    snapshot.isa.architecture = architecture;
    snapshot.isa.source = cpu::CpuCapabilitySource::proc_cpuinfo;

    for (std::size_t index = 0; index < processor_count; ++index) {
        snapshot.target_logical_processor_ids.push_back(
            static_cast<int>(index));
    }

    snapshot.target_logical_processor_count =
        snapshot.target_logical_processor_ids.size();

    snapshot.isa.fp32 = make_coverage(
        processor_count,
        cpu::CpuSupportState::supported,
        cpu::CpuSupportState::supported);

    snapshot.isa.fp64 = make_coverage(
        processor_count,
        cpu::CpuSupportState::supported,
        cpu::CpuSupportState::supported);

    snapshot.isa.neon = make_coverage(
        processor_count,
        cpu::CpuSupportState::unsupported,
        cpu::CpuSupportState::unsupported);

    snapshot.isa.fp16 = make_coverage(
        processor_count,
        cpu::CpuSupportState::unsupported,
        cpu::CpuSupportState::unsupported);

    snapshot.isa.bfloat16 = make_coverage(
        processor_count,
        cpu::CpuSupportState::unsupported,
        cpu::CpuSupportState::unsupported);

    snapshot.isa.arm_atomics = make_coverage(
        processor_count,
        cpu::CpuSupportState::unsupported,
        cpu::CpuSupportState::unsupported);

    snapshot.isa.crc32 = make_coverage(
        processor_count,
        cpu::CpuSupportState::unsupported,
        cpu::CpuSupportState::unsupported);

    snapshot.isa.aes = make_coverage(
        processor_count,
        cpu::CpuSupportState::unsupported,
        cpu::CpuSupportState::unsupported);

    snapshot.isa.sha1 = make_coverage(
        processor_count,
        cpu::CpuSupportState::unsupported,
        cpu::CpuSupportState::unsupported);

    snapshot.isa.sha2 = make_coverage(
        processor_count,
        cpu::CpuSupportState::unsupported,
        cpu::CpuSupportState::unsupported);

    snapshot.isa.sve = make_coverage(
        processor_count,
        cpu::CpuSupportState::unsupported,
        cpu::CpuSupportState::unsupported);

    snapshot.isa.sve2 = make_coverage(
        processor_count,
        cpu::CpuSupportState::unsupported,
        cpu::CpuSupportState::unsupported);

    return snapshot;
}

[[nodiscard]] bool expect(
    bool condition,
    const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }

    return true;
}

[[nodiscard]] bool test_non_arm_snapshot_is_rejected() {
    cpu::CpuCapabilitySnapshot snapshot =
        make_snapshot(cpu::CpuArchitectureFamily::x86_64, 4);

    const arm::ArmCapabilityQueryResult result =
        arm::query_arm_capabilities(snapshot);

    bool passed = true;

    passed &= expect(
        result.status.code ==
            arm::ArmCapabilityQueryCode::unsupported_architecture,
        "x86-64 snapshot should be reported as unsupported architecture");

    passed &= expect(
        result.capabilities.isa.execution_state ==
            arm::ArmExecutionState::not_arm,
        "x86-64 snapshot should resolve to not_arm");

    passed &= expect(
        result.capabilities.isa.common_profile ==
            arm::ArmExecutionProfile::not_arm,
        "x86-64 snapshot should use the not_arm profile");

    passed &= expect(
        !result.capabilities.issues.empty(),
        "non-ARM query should retain an explanatory issue");

    return passed;
}

[[nodiscard]] bool test_aarch64_scalar_profile() {
    cpu::CpuCapabilitySnapshot snapshot =
        make_snapshot(cpu::CpuArchitectureFamily::aarch64, 8);

    const arm::ArmCapabilityQueryResult result =
        arm::query_arm_capabilities(snapshot);

    bool passed = true;

    passed &= expect(
        result.status.code ==
            arm::ArmCapabilityQueryCode::partial_success,
        "AArch64 snapshot should complete conservatively");

    passed &= expect(
        result.status.completed(),
        "partial AArch64 query should count as completed");

    passed &= expect(
        result.capabilities.isa.execution_state ==
            arm::ArmExecutionState::aarch64,
        "AArch64 snapshot should preserve execution state");

    passed &= expect(
        result.capabilities.isa.common_profile ==
            arm::ArmExecutionProfile::scalar,
        "AArch64 without proven vector features should remain scalar");

    passed &= expect(
        result.capabilities.target_logical_processor_count == 8,
        "target processor count should be preserved");

    return passed;
}

[[nodiscard]] bool test_neon_fp16_profile() {
    cpu::CpuCapabilitySnapshot snapshot =
        make_snapshot(cpu::CpuArchitectureFamily::aarch64, 4);

    snapshot.isa.neon = make_coverage(
        4,
        cpu::CpuSupportState::supported,
        cpu::CpuSupportState::supported);

    snapshot.isa.fp16 = make_coverage(
        4,
        cpu::CpuSupportState::supported,
        cpu::CpuSupportState::supported);

    const arm::ArmCapabilityQueryResult result =
        arm::query_arm_capabilities(snapshot);

    bool passed = true;

    passed &= expect(
        result.capabilities.isa.advanced_simd.safe_for_all_targets(),
        "generic NEON support should map to Advanced SIMD");

    passed &= expect(
        result.capabilities.isa.fp16_vector.safe_for_all_targets(),
        "generic FP16 support should initially map to vector FP16");

    passed &= expect(
        result.capabilities.isa.common_profile ==
            arm::ArmExecutionProfile::advanced_simd_fp16,
        "NEON plus FP16 should select the Advanced SIMD FP16 profile");

    return passed;
}

[[nodiscard]] bool test_sve_profile_precedence() {
    cpu::CpuCapabilitySnapshot snapshot =
        make_snapshot(cpu::CpuArchitectureFamily::aarch64, 16);

    snapshot.isa.neon = make_coverage(
        16,
        cpu::CpuSupportState::supported,
        cpu::CpuSupportState::supported);

    snapshot.isa.fp16 = make_coverage(
        16,
        cpu::CpuSupportState::supported,
        cpu::CpuSupportState::supported);

    snapshot.isa.sve = make_coverage(
        16,
        cpu::CpuSupportState::supported,
        cpu::CpuSupportState::supported);

    const arm::ArmCapabilityQueryResult sve_result =
        arm::query_arm_capabilities(snapshot);

    bool passed = true;

    passed &= expect(
        sve_result.capabilities.isa.common_profile ==
            arm::ArmExecutionProfile::sve,
        "SVE should take precedence over fixed-width Advanced SIMD profiles");

    snapshot.isa.sve2 = make_coverage(
        16,
        cpu::CpuSupportState::supported,
        cpu::CpuSupportState::supported);

    const arm::ArmCapabilityQueryResult sve2_result =
        arm::query_arm_capabilities(snapshot);

    passed &= expect(
        sve2_result.capabilities.isa.common_profile ==
            arm::ArmExecutionProfile::sve2,
        "SVE2 should take precedence over SVE");

    passed &= expect(
        !sve2_result.capabilities.isa.sve_vector_length_observed,
        "SVE vector length must remain unobserved until queried");

    passed &= expect(
        sve2_result.capabilities.isa.sve_vector_length_bytes == 0,
        "unknown SVE vector length must remain zero");

    return passed;
}

[[nodiscard]] bool test_aarch32_execution_state() {
    cpu::CpuCapabilitySnapshot snapshot =
        make_snapshot(cpu::CpuArchitectureFamily::armv7, 2);

    snapshot.isa.neon = make_coverage(
        2,
        cpu::CpuSupportState::supported,
        cpu::CpuSupportState::supported);

    const arm::ArmCapabilityQueryResult result =
        arm::query_arm_capabilities(snapshot);

    bool passed = true;

    passed &= expect(
        result.capabilities.isa.execution_state ==
            arm::ArmExecutionState::aarch32,
        "ARMv7 should map to AArch32");

    passed &= expect(
        result.capabilities.isa.common_profile ==
            arm::ArmExecutionProfile::advanced_simd,
        "ARMv7 NEON should select the Advanced SIMD profile");

    return passed;
}

[[nodiscard]] bool test_empty_target_set_fails() {
    cpu::CpuCapabilitySnapshot snapshot =
        make_snapshot(cpu::CpuArchitectureFamily::aarch64, 0);

    const arm::ArmCapabilityQueryResult result =
        arm::query_arm_capabilities(snapshot);

    bool passed = true;

    passed &= expect(
        result.status.code ==
            arm::ArmCapabilityQueryCode::query_failed,
        "ARM snapshot without target processors should fail");

    passed &= expect(
        !result.status.completed(),
        "failed ARM query should not count as completed");

    bool found_fatal_issue = false;
    for (const arm::ArmCapabilityIssue& issue :
         result.capabilities.issues) {
        if (issue.fatal) {
            found_fatal_issue = true;
            break;
        }
    }

    passed &= expect(
        found_fatal_issue,
        "empty target-set failure should retain a fatal issue");

    return passed;
}

}  // namespace

int main() {
    bool passed = true;

    passed &= test_non_arm_snapshot_is_rejected();
    passed &= test_aarch64_scalar_profile();
    passed &= test_neon_fp16_profile();
    passed &= test_sve_profile_precedence();
    passed &= test_aarch32_execution_state();
    passed &= test_empty_target_set_fails();

    if (!passed) {
        return EXIT_FAILURE;
    }

    std::cout << "ARM capability foundation tests passed\n";
    return EXIT_SUCCESS;
}