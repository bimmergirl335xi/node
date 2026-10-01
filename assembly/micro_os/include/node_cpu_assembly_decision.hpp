#ifndef NODE_CPU_ASSEMBLY_DECISION_HPP
#define NODE_CPU_ASSEMBLY_DECISION_HPP

#include <cstddef>
#include <cstdint>

#include "cpu_capabilities.hpp"

namespace node::boot {

enum class ObservationCondition : std::uint8_t {
    observed = 0,
    partial,
    unavailable,
    unsupported,
    unknown,
    invalid,
    conflicting,
};

enum class ComponentRequirement : std::uint8_t {
    required = 0,
    optional,
    not_required,
    not_applicable,
    unsupported,
    unavailable,
    unknown,
};

enum class AssemblyEvaluationCode : std::uint8_t {
    compatible = 0,
    unsupported,
    unavailable,
    unknown,
    invalid,
    conflicting,
};

struct CpuAssemblyEvidence {
    ObservationCondition topology = ObservationCondition::unknown;
    ObservationCondition capability = ObservationCondition::unknown;
    prometheus::backends::cpu::CpuArchitectureFamily architecture =
        prometheus::backends::cpu::CpuArchitectureFamily::unknown;
    prometheus::backends::cpu::CpuSimdLevel common_simd =
        prometheus::backends::cpu::CpuSimdLevel::scalar;
    prometheus::backends::cpu::CpuSupportState lock_free_atomic_u64 =
        prometheus::backends::cpu::CpuSupportState::unknown;
    std::size_t pointer_width_bits = 0;
    std::size_t configured_logical_processors = 0;
    std::size_t online_logical_processors = 0;
    std::size_t process_allowed_logical_processors = 0;
    std::size_t packages = 0;
    std::size_t physical_cores = 0;
    std::size_t numa_nodes = 0;
    bool process_affinity_known = false;
};

struct CpuAssemblyDecision {
    AssemblyEvaluationCode evaluation = AssemblyEvaluationCode::unknown;
    ComponentRequirement cpu_runtime = ComponentRequirement::required;
    ComponentRequirement gpu_runtime = ComponentRequirement::not_required;
    prometheus::backends::cpu::CpuArchitectureFamily selected_architecture =
        prometheus::backends::cpu::CpuArchitectureFamily::unknown;
    prometheus::backends::cpu::CpuSimdLevel selected_simd =
        prometheus::backends::cpu::CpuSimdLevel::scalar;

    [[nodiscard]] bool compatible() const noexcept {
        return evaluation == AssemblyEvaluationCode::compatible;
    }
};

// BOOT-owned pure evaluation. It produces a bounded requirement decision only;
// it does not build, install, activate, register, or execute a runtime.
[[nodiscard]] CpuAssemblyDecision evaluate_cpu_assembly_requirement(
    const CpuAssemblyEvidence& evidence) noexcept;

[[nodiscard]] const char* to_string(ObservationCondition value) noexcept;
[[nodiscard]] const char* to_string(ComponentRequirement value) noexcept;
[[nodiscard]] const char* to_string(AssemblyEvaluationCode value) noexcept;

}  // namespace node::boot

#endif
