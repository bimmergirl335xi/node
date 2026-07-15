#include "simd_dispatch.hpp"

#include <cstddef>
#include <cstdint>

namespace prometheus::backends::cpu {
namespace {

enum class SimdFamily : std::uint8_t {
    scalar = 0,
    x86,
    arm,
};

[[nodiscard]] constexpr SimdFamily simd_family(
    CpuSimdLevel level) noexcept {
    switch (level) {
        case CpuSimdLevel::scalar:
            return SimdFamily::scalar;
        case CpuSimdLevel::sse2:
        case CpuSimdLevel::sse4_1:
        case CpuSimdLevel::avx:
        case CpuSimdLevel::avx2:
        case CpuSimdLevel::avx512f:
            return SimdFamily::x86;
        case CpuSimdLevel::neon:
        case CpuSimdLevel::sve:
        case CpuSimdLevel::sve2:
            return SimdFamily::arm;
    }
    return SimdFamily::scalar;
}

[[nodiscard]] constexpr bool valid_simd_level(
    CpuSimdLevel level) noexcept {
    switch (level) {
        case CpuSimdLevel::scalar:
        case CpuSimdLevel::sse2:
        case CpuSimdLevel::sse4_1:
        case CpuSimdLevel::avx:
        case CpuSimdLevel::avx2:
        case CpuSimdLevel::avx512f:
        case CpuSimdLevel::neon:
        case CpuSimdLevel::sve:
        case CpuSimdLevel::sve2:
            return true;
    }
    return false;
}

[[nodiscard]] constexpr unsigned simd_rank(
    CpuSimdLevel level) noexcept {
    switch (level) {
        case CpuSimdLevel::scalar:
            return 0;
        case CpuSimdLevel::sse2:
        case CpuSimdLevel::neon:
            return 1;
        case CpuSimdLevel::sse4_1:
        case CpuSimdLevel::sve:
            return 2;
        case CpuSimdLevel::avx:
        case CpuSimdLevel::sve2:
            return 3;
        case CpuSimdLevel::avx2:
            return 4;
        case CpuSimdLevel::avx512f:
            return 5;
    }
    return 0;
}

[[nodiscard]] constexpr bool families_compatible(
    CpuSimdLevel lhs,
    CpuSimdLevel rhs) noexcept {
    const SimdFamily lhs_family = simd_family(lhs);
    const SimdFamily rhs_family = simd_family(rhs);
    return lhs_family == SimdFamily::scalar ||
           rhs_family == SimdFamily::scalar ||
           lhs_family == rhs_family;
}

[[nodiscard]] constexpr bool level_matches_architecture(
    CpuSimdLevel level,
    CpuArchitectureFamily architecture) noexcept {
    switch (level) {
        case CpuSimdLevel::scalar:
            return true;
        case CpuSimdLevel::sse2:
        case CpuSimdLevel::sse4_1:
        case CpuSimdLevel::avx:
        case CpuSimdLevel::avx2:
        case CpuSimdLevel::avx512f:
            return architecture == CpuArchitectureFamily::x86_64;
        case CpuSimdLevel::neon:
            return architecture == CpuArchitectureFamily::aarch64 ||
                architecture == CpuArchitectureFamily::armv7;
        case CpuSimdLevel::sve:
        case CpuSimdLevel::sve2:
            return architecture == CpuArchitectureFamily::aarch64;
    }
    return false;
}

[[nodiscard]] constexpr bool valid_sve_width(
    std::size_t width_bytes) noexcept {
    constexpr std::size_t minimum_width_bytes = 16;
    constexpr std::size_t maximum_width_bytes = 256;
    constexpr std::size_t width_step_bytes = 16;

    return width_bytes >= minimum_width_bytes &&
           width_bytes <= maximum_width_bytes &&
           width_bytes % width_step_bytes == 0;
}

}  // namespace

CpuSimdLevel select_simd_level(
    CpuSimdLevel available,
    CpuSimdLevel maximum) noexcept {
    if (!valid_simd_level(available) ||
        !valid_simd_level(maximum) ||
        !families_compatible(available, maximum)) {
        return CpuSimdLevel::scalar;
    }

    return simd_rank(available) <= simd_rank(maximum)
               ? available
               : maximum;
}

CpuSimdLevel select_simd_level(
    const CpuIsaCapabilities& capabilities,
    CpuSimdLevel maximum) noexcept {
    if (!level_matches_architecture(
            capabilities.common_simd_level,
            capabilities.architecture) ||
        !level_matches_architecture(maximum, capabilities.architecture)) {
        return CpuSimdLevel::scalar;
    }

    return select_simd_level(capabilities.common_simd_level, maximum);
}

std::size_t simd_vector_width_bytes(
    CpuSimdLevel level,
    std::size_t scalable_width_bytes) noexcept {
    switch (level) {
        case CpuSimdLevel::scalar:
            return 0;
        case CpuSimdLevel::sse2:
        case CpuSimdLevel::sse4_1:
        case CpuSimdLevel::neon:
            return 16;
        case CpuSimdLevel::avx:
        case CpuSimdLevel::avx2:
            return 32;
        case CpuSimdLevel::avx512f:
            return 64;
        case CpuSimdLevel::sve:
        case CpuSimdLevel::sve2:
            return valid_sve_width(scalable_width_bytes)
                       ? scalable_width_bytes
                       : 0;
    }
    return 0;
}

}  // namespace prometheus::backends::cpu
