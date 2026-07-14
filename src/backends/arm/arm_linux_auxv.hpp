#ifndef SRC_BACKENDS_ARM_ARM_LINUX_AUXV_HPP
#define SRC_BACKENDS_ARM_ARM_LINUX_AUXV_HPP

#include <cstddef>
#include <cstdint>

#include "arm_capabilities.hpp"

namespace prometheus::backends::arm {

struct ArmLinuxAuxvObservation {
    ArmExecutionState execution_state = ArmExecutionState::unknown;

    bool provider_available = false;
    bool hwcap_observed = false;
    bool hwcap2_observed = false;

    std::uint64_t hwcap = 0;
    std::uint64_t hwcap2 = 0;

    int hwcap_error = 0;
    int hwcap2_error = 0;

    bool sve_vector_length_observed = false;
    std::size_t sve_vector_length_bytes = 0;
    int sve_vector_length_error = 0;
};

namespace linux_auxv_bits {

namespace aarch64 {
inline constexpr std::uint64_t fp = UINT64_C(1) << 0;
inline constexpr std::uint64_t asimd = UINT64_C(1) << 1;
inline constexpr std::uint64_t aes = UINT64_C(1) << 3;
inline constexpr std::uint64_t pmull = UINT64_C(1) << 4;
inline constexpr std::uint64_t sha1 = UINT64_C(1) << 5;
inline constexpr std::uint64_t sha2 = UINT64_C(1) << 6;
inline constexpr std::uint64_t crc32 = UINT64_C(1) << 7;
inline constexpr std::uint64_t atomics = UINT64_C(1) << 8;
inline constexpr std::uint64_t fphp = UINT64_C(1) << 9;
inline constexpr std::uint64_t asimdhp = UINT64_C(1) << 10;
inline constexpr std::uint64_t asimdrdm = UINT64_C(1) << 12;
inline constexpr std::uint64_t jscvt = UINT64_C(1) << 13;
inline constexpr std::uint64_t fcma = UINT64_C(1) << 14;
inline constexpr std::uint64_t sha3 = UINT64_C(1) << 17;
inline constexpr std::uint64_t sm3 = UINT64_C(1) << 18;
inline constexpr std::uint64_t sm4 = UINT64_C(1) << 19;
inline constexpr std::uint64_t asimddp = UINT64_C(1) << 20;
inline constexpr std::uint64_t sve = UINT64_C(1) << 22;
inline constexpr std::uint64_t flagm = UINT64_C(1) << 27;

namespace hwcap2 {
inline constexpr std::uint64_t sve2 = UINT64_C(1) << 1;
inline constexpr std::uint64_t sveaes = UINT64_C(1) << 2;
inline constexpr std::uint64_t svepmull = UINT64_C(1) << 3;
inline constexpr std::uint64_t svebitperm = UINT64_C(1) << 4;
inline constexpr std::uint64_t svesha3 = UINT64_C(1) << 5;
inline constexpr std::uint64_t svesm4 = UINT64_C(1) << 6;
inline constexpr std::uint64_t flagm2 = UINT64_C(1) << 7;
inline constexpr std::uint64_t i8mm = UINT64_C(1) << 13;
inline constexpr std::uint64_t bf16 = UINT64_C(1) << 14;
inline constexpr std::uint64_t svei8mm = UINT64_C(1) << 15;
inline constexpr std::uint64_t svebf16 = UINT64_C(1) << 16;
}  // namespace hwcap2
}  // namespace aarch64

namespace aarch32 {
inline constexpr std::uint64_t vfp = UINT64_C(1) << 6;
inline constexpr std::uint64_t neon = UINT64_C(1) << 12;

namespace hwcap2 {
inline constexpr std::uint64_t aes = UINT64_C(1) << 0;
inline constexpr std::uint64_t pmull = UINT64_C(1) << 1;
inline constexpr std::uint64_t sha1 = UINT64_C(1) << 2;
inline constexpr std::uint64_t sha2 = UINT64_C(1) << 3;
inline constexpr std::uint64_t crc32 = UINT64_C(1) << 4;
}  // namespace hwcap2
}  // namespace aarch32

}  // namespace linux_auxv_bits

[[nodiscard]] ArmLinuxAuxvObservation query_linux_arm_auxv_observation(
    ArmExecutionState execution_state) noexcept;

// Applies only fields proven by the supplied observation. Missing HWCAP or
// HWCAP2 observations leave the corresponding feature states unchanged.
[[nodiscard]] bool apply_linux_arm_auxv_observation(
    const ArmLinuxAuxvObservation& observation,
    std::size_t target_processor_count,
    ArmIsaCapabilities& capabilities) noexcept;

}  // namespace prometheus::backends::arm

#endif  // SRC_BACKENDS_ARM_ARM_LINUX_AUXV_HPP
