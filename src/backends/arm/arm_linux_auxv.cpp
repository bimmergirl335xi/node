#include "arm_linux_auxv.hpp"

#include <cerrno>

#if defined(__linux__) && (defined(__aarch64__) || defined(__arm__))
#include <sys/auxv.h>
#include <sys/prctl.h>
#endif

namespace prometheus::backends::arm {
namespace {

[[nodiscard]] cpu::CpuFeatureCoverage observed_coverage(
    bool supported,
    std::size_t target_processor_count) noexcept {
    cpu::CpuFeatureCoverage coverage{};
    coverage.common = supported
        ? cpu::CpuSupportState::supported
        : cpu::CpuSupportState::unsupported;
    coverage.any = coverage.common;
    coverage.target_processor_count = target_processor_count;
    coverage.observed_processor_count = target_processor_count;
    coverage.supported_processor_count = supported
        ? target_processor_count
        : 0;
    return coverage;
}

[[nodiscard]] bool has_bit(
    std::uint64_t value,
    std::uint64_t bit) noexcept {
    return (value & bit) != 0;
}

#if defined(__linux__) && (defined(__aarch64__) || defined(__arm__))
[[nodiscard]] bool read_auxv_value(
    unsigned long key,
    std::uint64_t& value,
    int& error) noexcept {
    errno = 0;
    const unsigned long raw = ::getauxval(key);
    error = errno;

    if (raw == 0 && error != 0) {
        value = 0;
        return false;
    }

    value = static_cast<std::uint64_t>(raw);
    return true;
}
#endif

}  // namespace

ArmLinuxAuxvObservation query_linux_arm_auxv_observation(
    ArmExecutionState execution_state) noexcept {
    ArmLinuxAuxvObservation observation{};
    observation.execution_state = execution_state;

#if defined(__linux__) && defined(__aarch64__)
    if (execution_state != ArmExecutionState::aarch64) {
        return observation;
    }

    observation.provider_available = true;
    observation.hwcap_observed = read_auxv_value(
        16UL,
        observation.hwcap,
        observation.hwcap_error);
    observation.hwcap2_observed = read_auxv_value(
        26UL,
        observation.hwcap2,
        observation.hwcap2_error);

    if (observation.hwcap_observed &&
        has_bit(observation.hwcap, linux_auxv_bits::aarch64::sve)) {
        constexpr int kPrSveGetVl = 51;
        constexpr unsigned long kPrSveVlLengthMask = 0xffffUL;

        errno = 0;
        const int raw_vector_length =
            ::prctl(kPrSveGetVl, 0UL, 0UL, 0UL, 0UL);
        observation.sve_vector_length_error = errno;

        if (raw_vector_length >= 0) {
            const std::size_t vector_length = static_cast<std::size_t>(
                static_cast<unsigned long>(raw_vector_length) &
                kPrSveVlLengthMask);

            if (vector_length > 0) {
                observation.sve_vector_length_observed = true;
                observation.sve_vector_length_bytes = vector_length;
            }
        }
    }
#elif defined(__linux__) && defined(__arm__)
    if (execution_state != ArmExecutionState::aarch32) {
        return observation;
    }

    observation.provider_available = true;
    observation.hwcap_observed = read_auxv_value(
        16UL,
        observation.hwcap,
        observation.hwcap_error);
    observation.hwcap2_observed = read_auxv_value(
        26UL,
        observation.hwcap2,
        observation.hwcap2_error);
#else
    (void)execution_state;
#endif

    return observation;
}

bool apply_linux_arm_auxv_observation(
    const ArmLinuxAuxvObservation& observation,
    std::size_t target_processor_count,
    ArmIsaCapabilities& capabilities) noexcept {
    if (!observation.provider_available || target_processor_count == 0) {
        return false;
    }

    bool applied = false;
    capabilities.execution_state = observation.execution_state;

    if (observation.execution_state == ArmExecutionState::aarch64) {
        if (observation.hwcap_observed) {
            using namespace linux_auxv_bits::aarch64;

            capabilities.floating_point = observed_coverage(
                has_bit(observation.hwcap, fp), target_processor_count);
            capabilities.advanced_simd = observed_coverage(
                has_bit(observation.hwcap, asimd), target_processor_count);
            capabilities.fp16_scalar = observed_coverage(
                has_bit(observation.hwcap, fphp), target_processor_count);
            capabilities.fp16_vector = observed_coverage(
                has_bit(observation.hwcap, asimdhp), target_processor_count);
            capabilities.dot_product = observed_coverage(
                has_bit(observation.hwcap, asimddp), target_processor_count);
            capabilities.rdm = observed_coverage(
                has_bit(observation.hwcap, asimdrdm), target_processor_count);
            capabilities.fcma = observed_coverage(
                has_bit(observation.hwcap, fcma), target_processor_count);
            capabilities.jscvt = observed_coverage(
                has_bit(observation.hwcap, jscvt), target_processor_count);
            capabilities.atomics = observed_coverage(
                has_bit(observation.hwcap, atomics), target_processor_count);
            capabilities.crc32 = observed_coverage(
                has_bit(observation.hwcap, crc32), target_processor_count);
            capabilities.flagm = observed_coverage(
                has_bit(observation.hwcap, flagm), target_processor_count);
            capabilities.aes = observed_coverage(
                has_bit(observation.hwcap, aes), target_processor_count);
            capabilities.pmull = observed_coverage(
                has_bit(observation.hwcap, pmull), target_processor_count);
            capabilities.sha1 = observed_coverage(
                has_bit(observation.hwcap, sha1), target_processor_count);
            capabilities.sha2 = observed_coverage(
                has_bit(observation.hwcap, sha2), target_processor_count);
            capabilities.sha3 = observed_coverage(
                has_bit(observation.hwcap, sha3), target_processor_count);
            capabilities.sm3 = observed_coverage(
                has_bit(observation.hwcap, sm3), target_processor_count);
            capabilities.sm4 = observed_coverage(
                has_bit(observation.hwcap, sm4), target_processor_count);
            capabilities.sve = observed_coverage(
                has_bit(observation.hwcap, sve), target_processor_count);
            applied = true;
        }

        if (observation.hwcap2_observed) {
            namespace bits = linux_auxv_bits::aarch64::hwcap2;

            capabilities.sve2 = observed_coverage(
                has_bit(observation.hwcap2, bits::sve2),
                target_processor_count);
            capabilities.sve_aes = observed_coverage(
                has_bit(observation.hwcap2, bits::sveaes),
                target_processor_count);
            capabilities.sve_pmull = observed_coverage(
                has_bit(observation.hwcap2, bits::svepmull),
                target_processor_count);
            capabilities.sve_bitperm = observed_coverage(
                has_bit(observation.hwcap2, bits::svebitperm),
                target_processor_count);
            capabilities.sve_sha3 = observed_coverage(
                has_bit(observation.hwcap2, bits::svesha3),
                target_processor_count);
            capabilities.sve_sm4 = observed_coverage(
                has_bit(observation.hwcap2, bits::svesm4),
                target_processor_count);
            capabilities.flagm2 = observed_coverage(
                has_bit(observation.hwcap2, bits::flagm2),
                target_processor_count);
            capabilities.i8mm = observed_coverage(
                has_bit(observation.hwcap2, bits::i8mm),
                target_processor_count);
            capabilities.bfloat16 = observed_coverage(
                has_bit(observation.hwcap2, bits::bf16),
                target_processor_count);
            capabilities.sve_i8mm = observed_coverage(
                has_bit(observation.hwcap2, bits::svei8mm),
                target_processor_count);
            capabilities.sve_bf16 = observed_coverage(
                has_bit(observation.hwcap2, bits::svebf16),
                target_processor_count);
            applied = true;
        }
    } else if (observation.execution_state == ArmExecutionState::aarch32) {
        if (observation.hwcap_observed) {
            capabilities.floating_point = observed_coverage(
                has_bit(observation.hwcap, linux_auxv_bits::aarch32::vfp),
                target_processor_count);
            capabilities.advanced_simd = observed_coverage(
                has_bit(observation.hwcap, linux_auxv_bits::aarch32::neon),
                target_processor_count);
            applied = true;
        }

        if (observation.hwcap2_observed) {
            namespace bits = linux_auxv_bits::aarch32::hwcap2;

            capabilities.aes = observed_coverage(
                has_bit(observation.hwcap2, bits::aes),
                target_processor_count);
            capabilities.pmull = observed_coverage(
                has_bit(observation.hwcap2, bits::pmull),
                target_processor_count);
            capabilities.sha1 = observed_coverage(
                has_bit(observation.hwcap2, bits::sha1),
                target_processor_count);
            capabilities.sha2 = observed_coverage(
                has_bit(observation.hwcap2, bits::sha2),
                target_processor_count);
            capabilities.crc32 = observed_coverage(
                has_bit(observation.hwcap2, bits::crc32),
                target_processor_count);
            applied = true;
        }
    }

    if (applied) {
        capabilities.source = ArmCapabilitySource::linux_auxv;
    }

    if (observation.sve_vector_length_observed &&
        observation.sve_vector_length_bytes > 0) {
        capabilities.sve_vector_length_observed = true;
        capabilities.sve_vector_length_bytes =
            observation.sve_vector_length_bytes;
    }

    return applied;
}

}  // namespace prometheus::backends::arm
