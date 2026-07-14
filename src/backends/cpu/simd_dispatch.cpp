#include "simd_dispatch.hpp"

#include "cpu_capabilities.hpp"

#include <cstddef>

namespace prometheus::backends::cpu {
namespace {

[[nodiscard]] constexpr unsigned rank(CpuSimdLevel level) noexcept {
    switch (level) {
        case CpuSimdLevel::scalar:  return 0;
        case CpuSimdLevel::sse2:    return 1;
        case CpuSimdLevel::sse4_1:  return 2;
        case CpuSimdLevel::avx:     return 3;
        case CpuSimdLevel::avx2:    return 4;
        case CpuSimdLevel::avx512f: return 5;
        case CpuSimdLevel::neon:    return 1;
        case CpuSimdLevel::sve:     return 2;
        case CpuSimdLevel::sve2:    return 3;
    }
    return 0;
}

[[nodiscard]] constexpr bool same_family(
    CpuSimdLevel lhs, CpuSimdLevel rhs) noexcept {
    const bool lhs_arm = lhs >= CpuSimdLevel::neon;
    const bool rhs_arm = rhs >= CpuSimdLevel::neon;
    return lhs == CpuSimdLevel::scalar || rhs == CpuSimdLevel::scalar ||
           lhs_arm == rhs_arm;
}

}  // namespace

CpuSimdLevel select_simd_level(
    CpuSimdLevel available, CpuSimdLevel maximum) noexcept {
    if (!same_family(available, maximum)) {
        return CpuSimdLevel::scalar;
    }
    return rank(available) <= rank(maximum) ? available : maximum;
}

CpuSimdLevel select_simd_level(
    const CpuIsaCapabilities& capabilities,
    CpuSimdLevel maximum) noexcept {
    return select_simd_level(capabilities.common_simd_level, maximum);
}

std::size_t simd_vector_width_bytes(
    CpuSimdLevel level, std::size_t scalable_width_bytes) noexcept {
    switch (level) {
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
            return scalable_width_bytes;
        case CpuSimdLevel::scalar:
            return 0;
    }
    return 0;
}

}  // namespace prometheus::backends::cpu
