#ifndef SRC_BACKENDS_CPU_SIMD_DISPATCH_HPP
#define SRC_BACKENDS_CPU_SIMD_DISPATCH_HPP

#include "cpu_capabilities.hpp"

#include <cstddef>

namespace prometheus::backends::cpu {

// Selects the highest dispatch level that is both available and no greater
// than the caller's maximum compiled adapter. Cross-architecture requests
// conservatively fall back to scalar.
[[nodiscard]] CpuSimdLevel select_simd_level(
    CpuSimdLevel available, CpuSimdLevel maximum) noexcept;

// Applies the same selection while also validating that the capability
// snapshot's common SIMD level and requested maximum belong to the snapshot's
// architecture family. Inconsistent or unknown non-scalar combinations fall
// back to scalar.
[[nodiscard]] CpuSimdLevel select_simd_level(
    const CpuIsaCapabilities& capabilities,
    CpuSimdLevel maximum) noexcept;

// Returns the execution width for a selected SIMD level. Fixed-width levels
// ignore scalable_width_bytes. SVE/SVE2 require a valid runtime width in the
// architectural 16..256 byte range, in 16-byte increments; otherwise zero is
// returned so callers cannot mistake an unknown width for a usable one.
[[nodiscard]] std::size_t simd_vector_width_bytes(
    CpuSimdLevel level,
    std::size_t scalable_width_bytes = 0) noexcept;

}  // namespace prometheus::backends::cpu

#endif  // SRC_BACKENDS_CPU_SIMD_DISPATCH_HPP
