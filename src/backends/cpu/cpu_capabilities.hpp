#ifndef SRC_BACKENDS_CPU_CPU_CAPABILITIES_HPP
#define SRC_BACKENDS_CPU_CPU_CAPABILITIES_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "cpu_topology.hpp"

namespace prometheus::backends::cpu {

enum class CpuArchitectureFamily : std::uint8_t {
    unknown = 0,
    x86_64,
    aarch64,
    armv7,
    riscv64,
    ppc64le,
};

enum class CpuSupportState : std::uint8_t {
    unknown = 0,
    unsupported,
    supported,
};

enum class CpuSimdLevel : std::uint8_t {
    scalar = 0,
    sse2,
    sse4_1,
    avx,
    avx2,
    avx512f,
    neon,
    sve,
    sve2,
};

enum class CpuCapabilitySource : std::uint8_t {
    none = 0,
    compile_time,
    proc_cpuinfo,
    linux_auxv,
    sysfs,
    combined,
};

// "common" is the safe execution state across every target logical processor.
// "any" reports whether at least one target processor exposes the feature.
// Missing observations remain unknown instead of becoming unsupported.
struct CpuFeatureCoverage {
    CpuSupportState common = CpuSupportState::unknown;
    CpuSupportState any = CpuSupportState::unknown;

    std::size_t target_processor_count = 0;
    std::size_t observed_processor_count = 0;
    std::size_t supported_processor_count = 0;

    [[nodiscard]] bool safe_for_all_targets() const noexcept {
        return common == CpuSupportState::supported;
    }
};

struct CpuIsaCapabilities {
    CpuArchitectureFamily architecture = CpuArchitectureFamily::unknown;
    CpuCapabilitySource source = CpuCapabilitySource::none;

    std::size_t pointer_width_bits = 0;
    bool little_endian = true;
    bool feature_sets_heterogeneous = false;

    CpuSimdLevel common_simd_level = CpuSimdLevel::scalar;
    std::size_t common_vector_width_bytes = 0;

    CpuFeatureCoverage fp32{};
    CpuFeatureCoverage fp64{};
    CpuFeatureCoverage fp16{};
    CpuFeatureCoverage bfloat16{};

    CpuFeatureCoverage sse2{};
    CpuFeatureCoverage sse4_1{};
    CpuFeatureCoverage avx{};
    CpuFeatureCoverage avx2{};
    CpuFeatureCoverage avx512f{};
    CpuFeatureCoverage fma{};
    CpuFeatureCoverage bmi1{};
    CpuFeatureCoverage bmi2{};
    CpuFeatureCoverage popcnt{};

    CpuFeatureCoverage neon{};
    CpuFeatureCoverage sve{};
    CpuFeatureCoverage sve2{};
    CpuFeatureCoverage arm_atomics{};
    CpuFeatureCoverage crc32{};

    CpuFeatureCoverage aes{};
    CpuFeatureCoverage sha1{};
    CpuFeatureCoverage sha2{};

    CpuFeatureCoverage lock_free_atomic_u64{};
};

enum class CpuCapabilityIssueCode : std::uint8_t {
    proc_cpuinfo_unavailable = 0,
    processor_record_missing,
    feature_record_missing,
    architecture_unknown,
    topology_unusable,
};

struct CpuCapabilityIssue {
    CpuCapabilityIssueCode code =
        CpuCapabilityIssueCode::proc_cpuinfo_unavailable;
    int os_cpu_id = -1;
    bool fatal = false;
    std::string message{};
};

enum class CpuCapabilityQueryCode : std::uint8_t {
    success = 0,
    partial_success,
    unsupported_platform,
    query_failed,
};

struct CpuCapabilityQueryStatus {
    CpuCapabilityQueryCode code = CpuCapabilityQueryCode::success;
    std::string message{};

    [[nodiscard]] bool completed() const noexcept {
        return code == CpuCapabilityQueryCode::success ||
               code == CpuCapabilityQueryCode::partial_success;
    }
};

struct CpuCapabilitySnapshot {
    CpuIsaCapabilities isa{};

    std::size_t target_logical_processor_count = 0;
    std::vector<int> target_logical_processor_ids{};

    std::vector<CpuCapabilityIssue> issues{};
};

struct CpuCapabilityQueryResult {
    CpuCapabilityQueryStatus status{};
    CpuCapabilitySnapshot capabilities{};
};

// Uses process-allowed logical processors when affinity is known. If affinity
// is unavailable, it conservatively targets every online processor in the
// supplied topology snapshot.
[[nodiscard]] CpuCapabilityQueryResult query_cpu_capabilities(
    const CpuTopologySnapshot& topology);

[[nodiscard]] CpuArchitectureFamily compile_time_cpu_architecture() noexcept;

[[nodiscard]] const char* to_string(CpuArchitectureFamily value) noexcept;
[[nodiscard]] const char* to_string(CpuSupportState value) noexcept;
[[nodiscard]] const char* to_string(CpuSimdLevel value) noexcept;
[[nodiscard]] const char* to_string(CpuCapabilitySource value) noexcept;
[[nodiscard]] const char* to_string(CpuCapabilityIssueCode value) noexcept;
[[nodiscard]] const char* to_string(CpuCapabilityQueryCode value) noexcept;

}  // namespace prometheus::backends::cpu

#endif  // SRC_BACKENDS_CPU_CPU_CAPABILITIES_HPP
