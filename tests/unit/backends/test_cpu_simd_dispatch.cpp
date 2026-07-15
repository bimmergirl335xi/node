#include <cstdlib>
#include <iostream>

#include "simd_dispatch.hpp"

namespace cpu = prometheus::backends::cpu;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "CPU SIMD dispatch test failed: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    require(
        cpu::select_simd_level(
            cpu::CpuSimdLevel::avx512f,
            cpu::CpuSimdLevel::avx2) == cpu::CpuSimdLevel::avx2,
        "x86 selection must honor the compiled maximum");
    require(
        cpu::select_simd_level(
            cpu::CpuSimdLevel::sse2,
            cpu::CpuSimdLevel::avx2) == cpu::CpuSimdLevel::sse2,
        "x86 selection cannot exceed available capability");
    require(
        cpu::select_simd_level(
            cpu::CpuSimdLevel::sve2,
            cpu::CpuSimdLevel::sve) == cpu::CpuSimdLevel::sve,
        "ARM selection must honor the compiled maximum");
    require(
        cpu::select_simd_level(
            cpu::CpuSimdLevel::neon,
            cpu::CpuSimdLevel::sve2) == cpu::CpuSimdLevel::neon,
        "ARM selection cannot exceed available capability");
    require(
        cpu::select_simd_level(
            cpu::CpuSimdLevel::avx2,
            cpu::CpuSimdLevel::neon) == cpu::CpuSimdLevel::scalar,
        "cross-family dispatch must fall back to scalar");
    require(
        cpu::select_simd_level(
            cpu::CpuSimdLevel::sve2,
            cpu::CpuSimdLevel::avx512f) == cpu::CpuSimdLevel::scalar,
        "reverse cross-family dispatch must fall back to scalar");
    require(
        cpu::select_simd_level(
            cpu::CpuSimdLevel::avx2,
            cpu::CpuSimdLevel::scalar) == cpu::CpuSimdLevel::scalar,
        "scalar maximum must force scalar dispatch");
    require(
        cpu::select_simd_level(
            cpu::CpuSimdLevel::scalar,
            cpu::CpuSimdLevel::avx512f) == cpu::CpuSimdLevel::scalar,
        "scalar availability must remain scalar");

    cpu::CpuIsaCapabilities x86{};
    x86.architecture = cpu::CpuArchitectureFamily::x86_64;
    x86.common_simd_level = cpu::CpuSimdLevel::avx2;
    require(
        cpu::select_simd_level(x86, cpu::CpuSimdLevel::avx) ==
            cpu::CpuSimdLevel::avx,
        "capability selection must clamp a valid x86 snapshot");
    require(
        cpu::select_simd_level(x86, cpu::CpuSimdLevel::sve2) ==
            cpu::CpuSimdLevel::scalar,
        "x86 snapshots must reject ARM maximums");

    cpu::CpuIsaCapabilities arm{};
    arm.architecture = cpu::CpuArchitectureFamily::aarch64;
    arm.common_simd_level = cpu::CpuSimdLevel::sve2;
    require(
        cpu::select_simd_level(arm, cpu::CpuSimdLevel::sve) ==
            cpu::CpuSimdLevel::sve,
        "capability selection must clamp a valid ARM snapshot");
    require(
        cpu::select_simd_level(arm, cpu::CpuSimdLevel::avx2) ==
            cpu::CpuSimdLevel::scalar,
        "ARM snapshots must reject x86 maximums");

    cpu::CpuIsaCapabilities armv7{};
    armv7.architecture = cpu::CpuArchitectureFamily::armv7;
    armv7.common_simd_level = cpu::CpuSimdLevel::neon;
    require(
        cpu::select_simd_level(armv7, cpu::CpuSimdLevel::neon) ==
            cpu::CpuSimdLevel::neon,
        "ARMv7 snapshots may select NEON");
    armv7.common_simd_level = cpu::CpuSimdLevel::sve;
    require(
        cpu::select_simd_level(armv7, cpu::CpuSimdLevel::sve) ==
            cpu::CpuSimdLevel::scalar,
        "ARMv7 snapshots must reject SVE");
    armv7.common_simd_level = cpu::CpuSimdLevel::sve2;
    require(
        cpu::select_simd_level(armv7, cpu::CpuSimdLevel::sve2) ==
            cpu::CpuSimdLevel::scalar,
        "ARMv7 snapshots must reject SVE2");

    cpu::CpuIsaCapabilities malformed{};
    malformed.architecture = cpu::CpuArchitectureFamily::aarch64;
    malformed.common_simd_level = cpu::CpuSimdLevel::avx2;
    require(
        cpu::select_simd_level(malformed, cpu::CpuSimdLevel::avx2) ==
            cpu::CpuSimdLevel::scalar,
        "architecture-inconsistent snapshots must fall back to scalar");
    require(
        cpu::select_simd_level(
            static_cast<cpu::CpuSimdLevel>(255),
            cpu::CpuSimdLevel::avx2) == cpu::CpuSimdLevel::scalar,
        "unknown SIMD levels must fall back to scalar");

    require(
        cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::scalar) == 0,
        "scalar dispatch must have zero vector width");
    require(
        cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::sse2) == 16 &&
            cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::sse4_1) == 16 &&
            cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::neon) == 16,
        "128-bit SIMD levels must report 16 bytes");
    require(
        cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::avx) == 32 &&
            cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::avx2) == 32,
        "256-bit SIMD levels must report 32 bytes");
    require(
        cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::avx512f) == 64,
        "AVX-512 must report 64 bytes");
    require(
        cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::sve) == 0 &&
            cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::sve2) == 0,
        "unknown scalable widths must remain zero");
    require(
        cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::sve, 16) == 16 &&
            cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::sve2, 64) == 64 &&
            cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::sve2, 256) == 256,
        "valid SVE widths must be preserved");
    require(
        cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::sve, 15) == 0 &&
            cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::sve, 24) == 0 &&
            cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::sve2, 272) == 0,
        "invalid SVE widths must be rejected");
    require(
        cpu::simd_vector_width_bytes(cpu::CpuSimdLevel::avx2, 64) == 32,
        "fixed-width levels must ignore scalable width input");

    return EXIT_SUCCESS;
}
