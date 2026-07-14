#include "arm_linux_auxv.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace arm = prometheus::backends::arm;
namespace cpu = prometheus::backends::cpu;

namespace {

[[nodiscard]] bool expect(
    bool condition,
    const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }

    return true;
}

[[nodiscard]] bool test_aarch64_auxv_decode() {
    arm::ArmLinuxAuxvObservation observation{};
    observation.execution_state = arm::ArmExecutionState::aarch64;
    observation.provider_available = true;
    observation.hwcap_observed = true;
    observation.hwcap2_observed = true;

    observation.hwcap =
        arm::linux_auxv_bits::aarch64::fp |
        arm::linux_auxv_bits::aarch64::asimd |
        arm::linux_auxv_bits::aarch64::fphp |
        arm::linux_auxv_bits::aarch64::asimdhp |
        arm::linux_auxv_bits::aarch64::asimddp |
        arm::linux_auxv_bits::aarch64::atomics |
        arm::linux_auxv_bits::aarch64::crc32 |
        arm::linux_auxv_bits::aarch64::sve;

    observation.hwcap2 =
        arm::linux_auxv_bits::aarch64::hwcap2::sve2 |
        arm::linux_auxv_bits::aarch64::hwcap2::i8mm |
        arm::linux_auxv_bits::aarch64::hwcap2::bf16 |
        arm::linux_auxv_bits::aarch64::hwcap2::svei8mm |
        arm::linux_auxv_bits::aarch64::hwcap2::svebf16;

    observation.sve_vector_length_observed = true;
    observation.sve_vector_length_bytes = 32;

    arm::ArmIsaCapabilities capabilities{};
    bool passed = true;

    passed &= expect(
        arm::apply_linux_arm_auxv_observation(
            observation,
            8,
            capabilities),
        "AArch64 auxiliary-vector observation should apply");

    passed &= expect(
        capabilities.advanced_simd.safe_for_all_targets(),
        "AArch64 ASIMD should decode as safe for all targets");

    passed &= expect(
        capabilities.fp16_scalar.safe_for_all_targets(),
        "AArch64 scalar FP16 should decode");

    passed &= expect(
        capabilities.fp16_vector.safe_for_all_targets(),
        "AArch64 vector FP16 should decode");

    passed &= expect(
        capabilities.dot_product.safe_for_all_targets(),
        "AArch64 dot product should decode");

    passed &= expect(
        capabilities.i8mm.safe_for_all_targets(),
        "AArch64 I8MM should decode");

    passed &= expect(
        capabilities.bfloat16.safe_for_all_targets(),
        "AArch64 BF16 should decode");

    passed &= expect(
        capabilities.sve.safe_for_all_targets(),
        "AArch64 SVE should decode");

    passed &= expect(
        capabilities.sve2.safe_for_all_targets(),
        "AArch64 SVE2 should decode");

    passed &= expect(
        capabilities.sve_vector_length_observed &&
            capabilities.sve_vector_length_bytes == 32,
        "observed SVE vector length should be preserved");

    return passed;
}

[[nodiscard]] bool test_missing_hwcap2_remains_unknown() {
    arm::ArmLinuxAuxvObservation observation{};
    observation.execution_state = arm::ArmExecutionState::aarch64;
    observation.provider_available = true;
    observation.hwcap_observed = true;
    observation.hwcap =
        arm::linux_auxv_bits::aarch64::fp |
        arm::linux_auxv_bits::aarch64::asimd;

    arm::ArmIsaCapabilities capabilities{};
    bool passed = true;

    passed &= expect(
        arm::apply_linux_arm_auxv_observation(
            observation,
            2,
            capabilities),
        "HWCAP-only AArch64 observation should apply");

    passed &= expect(
        capabilities.i8mm.common == cpu::CpuSupportState::unknown,
        "missing HWCAP2 must keep I8MM unknown");

    passed &= expect(
        capabilities.sve2.common == cpu::CpuSupportState::unknown,
        "missing HWCAP2 must keep SVE2 unknown");

    return passed;
}

[[nodiscard]] bool test_aarch32_auxv_decode() {
    arm::ArmLinuxAuxvObservation observation{};
    observation.execution_state = arm::ArmExecutionState::aarch32;
    observation.provider_available = true;
    observation.hwcap_observed = true;
    observation.hwcap2_observed = true;

    observation.hwcap =
        arm::linux_auxv_bits::aarch32::vfp |
        arm::linux_auxv_bits::aarch32::neon;

    observation.hwcap2 =
        arm::linux_auxv_bits::aarch32::hwcap2::aes |
        arm::linux_auxv_bits::aarch32::hwcap2::crc32;

    arm::ArmIsaCapabilities capabilities{};
    bool passed = true;

    passed &= expect(
        arm::apply_linux_arm_auxv_observation(
            observation,
            4,
            capabilities),
        "AArch32 auxiliary-vector observation should apply");

    passed &= expect(
        capabilities.floating_point.safe_for_all_targets(),
        "AArch32 VFP should decode");

    passed &= expect(
        capabilities.advanced_simd.safe_for_all_targets(),
        "AArch32 NEON should decode");

    passed &= expect(
        capabilities.aes.safe_for_all_targets(),
        "AArch32 AES should decode");

    passed &= expect(
        capabilities.crc32.safe_for_all_targets(),
        "AArch32 CRC32 should decode");

    passed &= expect(
        capabilities.sha2.common == cpu::CpuSupportState::unsupported,
        "observed absent AArch32 SHA2 should be unsupported");

    return passed;
}

[[nodiscard]] bool test_unavailable_provider_does_not_mutate() {
    arm::ArmLinuxAuxvObservation observation{};
    observation.execution_state = arm::ArmExecutionState::aarch64;

    arm::ArmIsaCapabilities capabilities{};
    bool passed = true;

    passed &= expect(
        !arm::apply_linux_arm_auxv_observation(
            observation,
            4,
            capabilities),
        "unavailable provider should not apply");

    passed &= expect(
        capabilities.advanced_simd.common ==
            cpu::CpuSupportState::unknown,
        "unavailable provider must not invent unsupported features");

    return passed;
}

}  // namespace

int main() {
    bool passed = true;

    passed &= test_aarch64_auxv_decode();
    passed &= test_missing_hwcap2_remains_unknown();
    passed &= test_aarch32_auxv_decode();
    passed &= test_unavailable_provider_does_not_mutate();

    if (!passed) {
        return EXIT_FAILURE;
    }

    std::cout << "ARM Linux auxiliary-vector tests passed\n";
    return EXIT_SUCCESS;
}
