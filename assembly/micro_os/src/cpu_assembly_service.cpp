#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <string>

#include "cpu_capabilities.hpp"
#include "cpu_topology.hpp"
#include "node_cpu_assembly_decision.hpp"

namespace cpu = prometheus::backends::cpu;
namespace boot = node::boot;

namespace {

int event_output_fd = STDOUT_FILENO;

void write_all(const char* data, std::size_t size) noexcept {
    while (size > 0) {
        const ssize_t count = write(event_output_fd, data, size);
        if (count > 0) {
            data += count;
            size -= static_cast<std::size_t>(count);
        } else if (count < 0 && errno == EINTR) {
            continue;
        } else {
            return;
        }
    }
}

template <typename... Arguments>
void emit(const char* format, Arguments... arguments) noexcept {
    std::array<char, 1024> line{};
    const int length = std::snprintf(
        line.data(), line.size(), format, arguments...);
    if (length > 0 && static_cast<std::size_t>(length) < line.size())
        write_all(line.data(), static_cast<std::size_t>(length));
}

bool valid_node_id(const std::string& value) {
    if (value.empty() || value.size() > 32 || value.front() == '-' ||
        value.back() == '-') return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '-';
    });
}

std::string node_identity() {
    FILE* input = std::fopen("/sys/class/dmi/id/product_serial", "r");
    if (!input) return "unmanaged-node";
    std::array<char, 64> value{};
    const bool read = std::fgets(value.data(), static_cast<int>(value.size()), input);
    const int closed = std::fclose(input);
    if (!read || closed != 0) return "unmanaged-node";
    value[std::strcspn(value.data(), "\r\n")] = '\0';
    const std::string result{value.data()};
    return valid_node_id(result) ? result : "unmanaged-node";
}

std::string bounded_token(std::string value) {
    if (value.size() > 64) value.resize(64);
    for (char& ch : value) {
        const unsigned char byte = static_cast<unsigned char>(ch);
        if (!((byte >= 'a' && byte <= 'z') ||
              (byte >= 'A' && byte <= 'Z') ||
              (byte >= '0' && byte <= '9') || ch == '.' || ch == '_' ||
              ch == '-')) ch = '_';
    }
    return value.empty() ? "unknown" : value;
}

boot::ObservationCondition topology_condition(cpu::CpuTopologyQueryCode code) {
    switch (code) {
        case cpu::CpuTopologyQueryCode::success:
            return boot::ObservationCondition::observed;
        case cpu::CpuTopologyQueryCode::partial_success:
            return boot::ObservationCondition::partial;
        case cpu::CpuTopologyQueryCode::unsupported_platform:
            return boot::ObservationCondition::unsupported;
        case cpu::CpuTopologyQueryCode::query_failed:
            return boot::ObservationCondition::unavailable;
    }
    return boot::ObservationCondition::invalid;
}

boot::ObservationCondition capability_condition(cpu::CpuCapabilityQueryCode code) {
    switch (code) {
        case cpu::CpuCapabilityQueryCode::success:
            return boot::ObservationCondition::observed;
        case cpu::CpuCapabilityQueryCode::partial_success:
            return boot::ObservationCondition::partial;
        case cpu::CpuCapabilityQueryCode::unsupported_platform:
            return boot::ObservationCondition::unsupported;
        case cpu::CpuCapabilityQueryCode::query_failed:
            return boot::ObservationCondition::unavailable;
    }
    return boot::ObservationCondition::invalid;
}

boot::CpuAssemblyEvidence normalize(
    const cpu::CpuTopologyQueryResult& topology,
    const cpu::CpuCapabilityQueryResult& capability) {
    boot::CpuAssemblyEvidence evidence{};
    evidence.topology = topology_condition(topology.status.code);
    evidence.capability = capability_condition(capability.status.code);
    evidence.architecture = capability.capabilities.isa.architecture;
    evidence.common_simd = capability.capabilities.isa.common_simd_level;
    evidence.lock_free_atomic_u64 =
        capability.capabilities.isa.lock_free_atomic_u64.common;
    evidence.pointer_width_bits = capability.capabilities.isa.pointer_width_bits;
    const auto& summary = topology.topology.summary;
    evidence.configured_logical_processors =
        summary.configured_logical_processor_count;
    evidence.online_logical_processors = summary.online_logical_processor_count;
    evidence.process_allowed_logical_processors =
        summary.process_allowed_logical_processor_count;
    evidence.packages = summary.package_count;
    evidence.physical_cores = summary.physical_core_count;
    evidence.numa_nodes = summary.numa_node_count;
    evidence.process_affinity_known = summary.process_affinity_known;
    return evidence;
}

int run() {
    const std::string node = node_identity();
    emit("{\"record\":\"cpu_discovery_started\",\"subject\":\"%s\","
         "\"outcome\":\"started\",\"provider\":\"prometheus.cpu.foundation\","
         "\"mechanism\":\"linux.sysfs.proc.cpuid\","
         "\"scope\":\"p01.first_boot.cpu\",\"attempt\":\"cpu-observation-1\","
         "\"detail\":\"fresh_guest_observation\"}\n",
         node.c_str());
    const cpu::CpuTopologyQueryResult topology = cpu::query_cpu_topology();
    cpu::CpuCapabilityQueryResult capability{};
    if (topology.status.completed())
        capability = cpu::query_cpu_capabilities(topology.topology);
    else
        capability.status.code = cpu::CpuCapabilityQueryCode::query_failed;
    const boot::CpuAssemblyEvidence evidence = normalize(topology, capability);
    std::string vendor = "unknown";
    if (!topology.topology.packages.empty())
        vendor = bounded_token(
            topology.topology.packages.front().identity.signature.vendor_id);
    const auto& summary = topology.topology.summary;
    emit("{\"record\":\"cpu_topology_observed\",\"subject\":\"%s\","
         "\"outcome\":\"%s\",\"configured\":%zu,\"online\":%zu,"
         "\"process_allowed\":%zu,\"packages\":%zu,\"physical_cores\":%zu,"
         "\"numa_nodes\":%zu,\"affinity\":\"%s\",\"smt\":\"%s\","
         "\"issues\":%zu,\"provider\":\"prometheus.cpu.foundation\","
         "\"scope\":\"p01.first_boot.cpu\",\"attempt\":\"cpu-observation-1\","
         "\"detail\":\"read_only_linux_topology\"}\n",
         node.c_str(), boot::to_string(evidence.topology),
         summary.configured_logical_processor_count,
         summary.online_logical_processor_count,
         summary.process_allowed_logical_processor_count, summary.package_count,
         summary.physical_core_count, summary.numa_node_count,
         summary.process_affinity_known ? "known" : "unknown",
         summary.simultaneous_multithreading_present ? "present" : "not_observed",
         topology.topology.issues.size());
    const auto& isa = capability.capabilities.isa;
    emit("{\"record\":\"cpu_capability_observed\",\"subject\":\"%s\","
         "\"outcome\":\"%s\",\"architecture\":\"%s\",\"vendor\":\"%s\","
         "\"pointer_bits\":%zu,\"common_simd\":\"%s\",\"avx2\":\"%s\","
         "\"avx512f\":\"%s\",\"atomic_u64\":\"%s\",\"source\":\"%s\","
         "\"issues\":%zu,\"provider\":\"prometheus.cpu.foundation\","
         "\"scope\":\"p01.first_boot.cpu\",\"attempt\":\"cpu-observation-1\","
         "\"detail\":\"normalized_cpu_capability\"}\n",
         node.c_str(), boot::to_string(evidence.capability),
         cpu::to_string(isa.architecture), vendor.c_str(), isa.pointer_width_bits,
         cpu::to_string(isa.common_simd_level), cpu::to_string(isa.avx2.common),
         cpu::to_string(isa.avx512f.common),
         cpu::to_string(isa.lock_free_atomic_u64.common),
         cpu::to_string(isa.source), capability.capabilities.issues.size());
    const boot::CpuAssemblyDecision decision =
        boot::evaluate_cpu_assembly_requirement(evidence);
    emit("{\"record\":\"cpu_profile_evaluation\",\"subject\":\"%s\","
         "\"outcome\":\"%s\",\"architecture\":\"%s\",\"profile\":\"%s\","
         "\"component_set\":\"p01.cpu-only.first-boot\","
         "\"component_set_revision\":1,"
         "\"detail\":\"capability_driven_no_vendor_dispatch\"}\n",
         node.c_str(), boot::to_string(decision.evaluation),
         cpu::to_string(decision.selected_architecture),
         cpu::to_string(decision.selected_simd));
    emit("{\"record\":\"component_requirement\",\"subject\":\"%s\","
         "\"component\":\"cpu.runtime\",\"outcome\":\"%s\","
         "\"evaluation\":\"%s\","
         "\"component_set\":\"p01.cpu-only.first-boot\","
         "\"component_set_revision\":1,\"detail\":\"required_not_assembled\"}\n",
         node.c_str(), boot::to_string(decision.cpu_runtime),
         boot::to_string(decision.evaluation));
    emit("{\"record\":\"component_requirement\",\"subject\":\"%s\","
         "\"component\":\"gpu.runtime\",\"outcome\":\"%s\","
         "\"evaluation\":\"profile_not_requested\","
         "\"component_set\":\"p01.cpu-only.first-boot\","
         "\"component_set_revision\":1,"
         "\"detail\":\"not_required_no_hardware_absence_claim\"}\n",
         node.c_str(), boot::to_string(decision.gpu_runtime));
    emit("{\"record\":\"assembly_requirement_result\",\"subject\":\"%s\","
         "\"outcome\":\"%s\",\"runtime_compiled\":false,"
         "\"runtime_activated\":false,"
         "\"component_set\":\"p01.cpu-only.first-boot\","
         "\"component_set_revision\":1,"
         "\"detail\":\"boot_owned_requirement_plan_only\"}\n",
         node.c_str(), boot::to_string(decision.evaluation));
    return decision.compatible() ? 0 : 30;
}

}  // namespace

int main() {
    const int serial = open(
        "/dev/ttyS0", O_WRONLY | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (serial >= 0) event_output_fd = serial;
    try {
        return run();
    } catch (...) {
        emit("{\"record\":\"assembly_requirement_result\","
             "\"subject\":\"unmanaged-node\",\"outcome\":\"unavailable\","
             "\"runtime_compiled\":false,\"runtime_activated\":false,"
             "\"detail\":\"exception_contained_at_service_boundary\"}\n");
        return 31;
    }
}
