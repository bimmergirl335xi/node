#ifndef SRC_BACKENDS_ARM_ARM_CAPABILITIES_HPP
#define SRC_BACKENDS_ARM_ARM_CAPABILITIES_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "../cpu/cpu_capabilities.hpp"

namespace prometheus::backends::arm {

// This provider enriches the generic CpuBackend capability snapshot.
// It is not an independently registered compute backend and does not represent
// a second copy of the physical CPU.

enum class ArmExecutionState : std::uint8_t {
    unknown = 0,
    not_arm,
    aarch32,
    aarch64,
};

enum class ArmCapabilitySource : std::uint8_t {
    none = 0,
    compile_time,
    cpu_snapshot,
    linux_auxv,
    proc_cpuinfo,
    system_register,
    combined,
};

// Capability-derived execution profile.
//
// These levels describe hardware and operating-system exposure only. They do
// not prove that Prometheus has a typed executable adapter for an operation.
enum class ArmExecutionProfile : std::uint8_t {
    unknown = 0,
    not_arm,
    scalar,
    advanced_simd,
    advanced_simd_fp16,
    advanced_simd_dotprod,
    advanced_simd_i8mm,
    sve,
    sve2,
};

// ARM implementer/part information is observational metadata. It must not be
// used as the sole basis for instruction dispatch.
struct ArmProcessorIdentity {
    bool observed = false;

    std::uint32_t implementer = 0;
    std::uint32_t architecture = 0;
    std::uint32_t variant = 0;
    std::uint32_t part = 0;
    std::uint32_t revision = 0;

    std::string implementer_text{};
    std::string part_text{};
};

// ARM-specific feature vocabulary supplements CpuIsaCapabilities.
//
// CpuFeatureCoverage is reused so common/any support retains exactly the same
// meaning as the generic CPU capability layer.
struct ArmIsaCapabilities {
    ArmExecutionState execution_state = ArmExecutionState::unknown;
    ArmCapabilitySource source = ArmCapabilitySource::none;
    ArmExecutionProfile common_profile = ArmExecutionProfile::unknown;

    bool feature_sets_heterogeneous = false;

    // Baseline floating-point and Advanced SIMD.
    cpu::CpuFeatureCoverage floating_point{};
    cpu::CpuFeatureCoverage advanced_simd{};
    cpu::CpuFeatureCoverage fp16_scalar{};
    cpu::CpuFeatureCoverage fp16_vector{};
    cpu::CpuFeatureCoverage bfloat16{};

    // Arithmetic and matrix-oriented extensions.
    cpu::CpuFeatureCoverage dot_product{};
    cpu::CpuFeatureCoverage i8mm{};
    cpu::CpuFeatureCoverage rdm{};
    cpu::CpuFeatureCoverage fcma{};
    cpu::CpuFeatureCoverage jscvt{};

    // Synchronization and general-purpose extensions.
    cpu::CpuFeatureCoverage atomics{};
    cpu::CpuFeatureCoverage crc32{};
    cpu::CpuFeatureCoverage flagm{};
    cpu::CpuFeatureCoverage flagm2{};

    // Cryptographic instruction exposure.
    cpu::CpuFeatureCoverage aes{};
    cpu::CpuFeatureCoverage pmull{};
    cpu::CpuFeatureCoverage sha1{};
    cpu::CpuFeatureCoverage sha2{};
    cpu::CpuFeatureCoverage sha3{};
    cpu::CpuFeatureCoverage sm3{};
    cpu::CpuFeatureCoverage sm4{};

    // Scalable Vector Extension families.
    cpu::CpuFeatureCoverage sve{};
    cpu::CpuFeatureCoverage sve2{};
    cpu::CpuFeatureCoverage sve_aes{};
    cpu::CpuFeatureCoverage sve_pmull{};
    cpu::CpuFeatureCoverage sve_bitperm{};
    cpu::CpuFeatureCoverage sve_sha3{};
    cpu::CpuFeatureCoverage sve_sm4{};
    cpu::CpuFeatureCoverage sve_i8mm{};
    cpu::CpuFeatureCoverage sve_bf16{};

    // Zero means unknown, unsupported, unavailable, or not applicable.
    // SVE remains vector-length agnostic even when this observation exists.
    std::size_t sve_vector_length_bytes = 0;
    bool sve_vector_length_observed = false;
};

enum class ArmCapabilityIssueCode : std::uint8_t {
    not_arm_architecture = 0,
    cpu_snapshot_incomplete,
    execution_state_unknown,
    identity_unavailable,
    auxiliary_vector_unavailable,
    auxiliary_vector_query_failed,
    processor_record_missing,
    feature_record_missing,
    sve_vector_length_unavailable,
};

struct ArmCapabilityIssue {
    ArmCapabilityIssueCode code =
        ArmCapabilityIssueCode::cpu_snapshot_incomplete;

    int os_cpu_id = -1;
    bool fatal = false;
    std::string message{};
};

enum class ArmCapabilityQueryCode : std::uint8_t {
    success = 0,
    partial_success,
    unsupported_architecture,
    unsupported_platform,
    query_failed,
};

struct ArmCapabilityQueryStatus {
    ArmCapabilityQueryCode code = ArmCapabilityQueryCode::success;
    std::string message{};

    [[nodiscard]] bool completed() const noexcept {
        return code == ArmCapabilityQueryCode::success ||
               code == ArmCapabilityQueryCode::partial_success;
    }
};

struct ArmCapabilitySnapshot {
    ArmIsaCapabilities isa{};

    // One entry may eventually exist per processor identity group. A1 begins
    // conservatively with whatever identity information the operating system
    // exposes without assuming a homogeneous ARM machine.
    std::vector<ArmProcessorIdentity> processor_identities{};

    std::size_t target_logical_processor_count = 0;
    std::vector<int> target_logical_processor_ids{};

    std::vector<ArmCapabilityIssue> issues{};
};

struct ArmCapabilityQueryResult {
    ArmCapabilityQueryStatus status{};
    ArmCapabilitySnapshot capabilities{};
};

// Enriches an already-discovered generic CPU capability snapshot.
//
// This function does not:
// - register a second backend;
// - change CPU affinity;
// - start workers;
// - execute SIMD instructions;
// - inspect cameras, GPUs, or accelerators;
// - redirect robot behavior.
[[nodiscard]] ArmCapabilityQueryResult query_arm_capabilities(
    const cpu::CpuCapabilitySnapshot& cpu_snapshot);

[[nodiscard]] ArmExecutionState compile_time_arm_execution_state() noexcept;

[[nodiscard]] const char* to_string(ArmExecutionState value) noexcept;
[[nodiscard]] const char* to_string(ArmCapabilitySource value) noexcept;
[[nodiscard]] const char* to_string(ArmExecutionProfile value) noexcept;
[[nodiscard]] const char* to_string(ArmCapabilityIssueCode value) noexcept;
[[nodiscard]] const char* to_string(ArmCapabilityQueryCode value) noexcept;

}  // namespace prometheus::backends::arm

#endif  // SRC_BACKENDS_ARM_ARM_CAPABILITIES_HPP
