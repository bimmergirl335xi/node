#ifndef SRC_BACKENDS_CPU_SIMD_DISPATCH_HPP
#define SRC_BACKENDS_CPU_SIMD_DISPATCH_HPP

#include "cpu_capabilities.hpp"

#include <cstddef>

namespace prometheus::backends::cpu {

[[nodiscard]] CpuSimdLevel select_simd_level(
    CpuSimdLevel available, CpuSimdLevel maximum) noexcept;

[[nodiscard]] CpuSimdLevel select_simd_level(
    const CpuIsaCapabilities& capabilities,
    CpuSimdLevel maximum) noexcept;

[[nodiscard]] std::size_t simd_vector_width_bytes(
    CpuSimdLevel level, std::size_t scalable_width_bytes = 0) noexcept;

}  // namespace prometheus::backends::cpu

#endif  // SRC_BACKENDS_CPU_SIMD_DISPATCH_HPP
