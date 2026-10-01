#include "node_cpu_assembly_decision.hpp"

#include "simd_dispatch.hpp"

namespace cpu = prometheus::backends::cpu;

namespace node::boot {
namespace {

AssemblyEvaluationCode condition_failure(ObservationCondition value) noexcept {
    switch (value) {
        case ObservationCondition::observed:
        case ObservationCondition::partial:
            return AssemblyEvaluationCode::compatible;
        case ObservationCondition::unavailable:
            return AssemblyEvaluationCode::unavailable;
        case ObservationCondition::unsupported:
            return AssemblyEvaluationCode::unsupported;
        case ObservationCondition::unknown:
            return AssemblyEvaluationCode::unknown;
        case ObservationCondition::invalid:
            return AssemblyEvaluationCode::invalid;
        case ObservationCondition::conflicting:
            return AssemblyEvaluationCode::conflicting;
    }
    return AssemblyEvaluationCode::invalid;
}

cpu::CpuSimdLevel maximum_profile(cpu::CpuArchitectureFamily architecture) noexcept {
    switch (architecture) {
        case cpu::CpuArchitectureFamily::x86_64:
            return cpu::CpuSimdLevel::avx512f;
        case cpu::CpuArchitectureFamily::aarch64:
            return cpu::CpuSimdLevel::sve2;
        case cpu::CpuArchitectureFamily::armv7:
            return cpu::CpuSimdLevel::neon;
        case cpu::CpuArchitectureFamily::unknown:
        case cpu::CpuArchitectureFamily::riscv64:
        case cpu::CpuArchitectureFamily::ppc64le:
            return cpu::CpuSimdLevel::scalar;
    }
    return cpu::CpuSimdLevel::scalar;
}

}  // namespace

CpuAssemblyDecision evaluate_cpu_assembly_requirement(
    const CpuAssemblyEvidence& evidence) noexcept {
    CpuAssemblyDecision result{};
    const auto topology_failure = condition_failure(evidence.topology);
    if (topology_failure != AssemblyEvaluationCode::compatible) {
        result.evaluation = topology_failure;
        return result;
    }
    const auto capability_failure = condition_failure(evidence.capability);
    if (capability_failure != AssemblyEvaluationCode::compatible) {
        result.evaluation = capability_failure;
        return result;
    }
    if (evidence.configured_logical_processors == 0 ||
        evidence.online_logical_processors == 0 || evidence.packages == 0 ||
        evidence.physical_cores == 0 || evidence.numa_nodes == 0 ||
        (evidence.process_affinity_known &&
         evidence.process_allowed_logical_processors == 0)) {
        result.evaluation = AssemblyEvaluationCode::unavailable;
        return result;
    }
    if (evidence.online_logical_processors > evidence.configured_logical_processors ||
        evidence.process_allowed_logical_processors >
            evidence.online_logical_processors ||
        (evidence.pointer_width_bits != 32 && evidence.pointer_width_bits != 64)) {
        result.evaluation = AssemblyEvaluationCode::invalid;
        return result;
    }
    if (evidence.architecture == cpu::CpuArchitectureFamily::unknown) {
        result.evaluation = AssemblyEvaluationCode::unknown;
        return result;
    }
    if (evidence.architecture == cpu::CpuArchitectureFamily::riscv64 ||
        evidence.architecture == cpu::CpuArchitectureFamily::ppc64le) {
        result.evaluation = AssemblyEvaluationCode::unsupported;
        return result;
    }
    if (evidence.lock_free_atomic_u64 == cpu::CpuSupportState::unknown) {
        result.evaluation = AssemblyEvaluationCode::unknown;
        return result;
    }
    if (evidence.lock_free_atomic_u64 == cpu::CpuSupportState::unsupported) {
        result.evaluation = AssemblyEvaluationCode::unsupported;
        return result;
    }
    cpu::CpuIsaCapabilities capabilities{};
    capabilities.architecture = evidence.architecture;
    capabilities.common_simd_level = evidence.common_simd;
    const auto selected = cpu::select_simd_level(
        capabilities, maximum_profile(evidence.architecture));
    if (evidence.common_simd != cpu::CpuSimdLevel::scalar &&
        selected == cpu::CpuSimdLevel::scalar) {
        result.evaluation = AssemblyEvaluationCode::invalid;
        return result;
    }
    result.evaluation = AssemblyEvaluationCode::compatible;
    result.selected_architecture = evidence.architecture;
    result.selected_simd = selected;
    return result;
}

const char* to_string(ObservationCondition value) noexcept {
    switch (value) {
        case ObservationCondition::observed: return "observed";
        case ObservationCondition::partial: return "partial";
        case ObservationCondition::unavailable: return "unavailable";
        case ObservationCondition::unsupported: return "unsupported";
        case ObservationCondition::unknown: return "unknown";
        case ObservationCondition::invalid: return "invalid";
        case ObservationCondition::conflicting: return "conflicting";
    }
    return "invalid";
}

const char* to_string(ComponentRequirement value) noexcept {
    switch (value) {
        case ComponentRequirement::required: return "required";
        case ComponentRequirement::optional: return "optional";
        case ComponentRequirement::not_required: return "not_required";
        case ComponentRequirement::not_applicable: return "not_applicable";
        case ComponentRequirement::unsupported: return "unsupported";
        case ComponentRequirement::unavailable: return "unavailable";
        case ComponentRequirement::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(AssemblyEvaluationCode value) noexcept {
    switch (value) {
        case AssemblyEvaluationCode::compatible: return "compatible";
        case AssemblyEvaluationCode::unsupported: return "unsupported";
        case AssemblyEvaluationCode::unavailable: return "unavailable";
        case AssemblyEvaluationCode::unknown: return "unknown";
        case AssemblyEvaluationCode::invalid: return "invalid";
        case AssemblyEvaluationCode::conflicting: return "conflicting";
    }
    return "invalid";
}

}  // namespace node::boot
