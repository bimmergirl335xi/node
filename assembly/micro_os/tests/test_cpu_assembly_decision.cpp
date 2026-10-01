#include "node_cpu_assembly_decision.hpp"

#include <cstdlib>
#include <iostream>

namespace boot = node::boot;
namespace cpu = prometheus::backends::cpu;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "CPU assembly decision test failed: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

boot::CpuAssemblyEvidence valid_evidence() {
    boot::CpuAssemblyEvidence evidence{};
    evidence.topology = boot::ObservationCondition::observed;
    evidence.capability = boot::ObservationCondition::observed;
    evidence.architecture = cpu::CpuArchitectureFamily::x86_64;
    evidence.common_simd = cpu::CpuSimdLevel::avx2;
    evidence.lock_free_atomic_u64 = cpu::CpuSupportState::supported;
    evidence.pointer_width_bits = 64;
    evidence.configured_logical_processors = 2;
    evidence.online_logical_processors = 2;
    evidence.process_allowed_logical_processors = 2;
    evidence.packages = 1;
    evidence.physical_cores = 2;
    evidence.numa_nodes = 1;
    evidence.process_affinity_known = true;
    return evidence;
}

}  // namespace

int main() {
    auto evidence = valid_evidence();
    auto decision = boot::evaluate_cpu_assembly_requirement(evidence);
    require(decision.compatible(), "valid CPU evidence must be compatible");
    require(decision.cpu_runtime == boot::ComponentRequirement::required,
            "the P01 CPU runtime requirement must remain required");
    require(decision.gpu_runtime == boot::ComponentRequirement::not_required,
            "the P01 GPU runtime requirement must remain not required");
    require(decision.selected_simd == cpu::CpuSimdLevel::avx2,
            "profile selection must use normalized capability evidence");

    evidence = valid_evidence();
    evidence.topology = boot::ObservationCondition::unavailable;
    require(boot::evaluate_cpu_assembly_requirement(evidence).evaluation ==
                boot::AssemblyEvaluationCode::unavailable,
            "unavailable observation must remain unavailable");

    evidence = valid_evidence();
    evidence.physical_cores = 0;
    require(boot::evaluate_cpu_assembly_requirement(evidence).evaluation ==
                boot::AssemblyEvaluationCode::unavailable,
            "incomplete required topology must not fabricate a profile");

    evidence = valid_evidence();
    evidence.common_simd = cpu::CpuSimdLevel::scalar;
    decision = boot::evaluate_cpu_assembly_requirement(evidence);
    require(decision.compatible() &&
                decision.selected_simd == cpu::CpuSimdLevel::scalar,
            "unknown optional vector capability must permit scalar selection");

    evidence = valid_evidence();
    evidence.architecture = cpu::CpuArchitectureFamily::riscv64;
    require(boot::evaluate_cpu_assembly_requirement(evidence).evaluation ==
                boot::AssemblyEvaluationCode::unsupported,
            "unsupported architecture must remain unsupported");

    evidence = valid_evidence();
    evidence.online_logical_processors = 3;
    require(boot::evaluate_cpu_assembly_requirement(evidence).evaluation ==
                boot::AssemblyEvaluationCode::invalid,
            "corrupt topology counts must be invalid");

    evidence = valid_evidence();
    evidence.lock_free_atomic_u64 = cpu::CpuSupportState::unknown;
    require(boot::evaluate_cpu_assembly_requirement(evidence).evaluation ==
                boot::AssemblyEvaluationCode::unknown,
            "unknown required atomic capability must remain unknown");

    evidence = valid_evidence();
    evidence.lock_free_atomic_u64 = cpu::CpuSupportState::unsupported;
    require(boot::evaluate_cpu_assembly_requirement(evidence).evaluation ==
                boot::AssemblyEvaluationCode::unsupported,
            "missing required atomic capability must be unsupported");

    evidence = valid_evidence();
    evidence.topology = boot::ObservationCondition::conflicting;
    require(boot::evaluate_cpu_assembly_requirement(evidence).evaluation ==
                boot::AssemblyEvaluationCode::conflicting,
            "conflicting evidence must remain conflicting");
    return EXIT_SUCCESS;
}
