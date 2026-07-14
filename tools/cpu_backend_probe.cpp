#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "backend_registry.hpp"
#include "cpu/cpu_backend.hpp"

namespace pb = prometheus::backends;
namespace pc = prometheus::backends::cpu;

namespace {

constexpr double kBytesPerGiB = 1024.0 * 1024.0 * 1024.0;

void print_issue_line(
    const char* family,
    const char* code,
    bool fatal,
    const std::string& message,
    int os_cpu_id = -1) {
    std::cout << "  [" << family << "] " << code;
    if (os_cpu_id >= 0) {
        std::cout << " cpu=" << os_cpu_id;
    }
    std::cout << " fatal=" << (fatal ? "yes" : "no");
    if (!message.empty()) {
        std::cout << " message=\"" << message << "\"";
    }
    std::cout << '\n';
}

void print_pressure(
    const char* label,
    const pc::CpuPressureReading& reading) {
    std::cout << label << ": state=" << pc::to_string(reading.state);
    if (reading.some.available) {
        std::cout << ", some.avg10=" << reading.some.average_10_seconds
                  << ", some.avg60=" << reading.some.average_60_seconds
                  << ", some.avg300=" << reading.some.average_300_seconds;
    }
    if (reading.full.available) {
        std::cout << ", full.avg10=" << reading.full.average_10_seconds
                  << ", full.avg60=" << reading.full.average_60_seconds
                  << ", full.avg300=" << reading.full.average_300_seconds;
    }
    std::cout << '\n';
}

}  // namespace

int main() {
    std::cout << "Prometheus CPU backend probe\n";

    auto cpu_backend = std::make_shared<pc::CpuBackend>();
    pb::BackendRegistry registry{};

    const pb::BackendRegistrationResult registration =
        registry.register_backend(cpu_backend);
    std::cout << "Backend registration: "
              << pb::to_string(registration.code) << '\n';
    if (!registration.ok()) {
        std::cerr << registration.message << '\n';
        return EXIT_FAILURE;
    }

    const pb::ComputeBackendStatus initialization = cpu_backend->initialize();
    std::cout << "Initial backend state: "
              << pb::to_string(initialization.state) << '\n'
              << "Initial backend usable: "
              << (initialization.usable ? "yes" : "no") << '\n';
    if (!initialization.message.empty()) {
        std::cout << "Initial backend message: "
                  << initialization.message << '\n';
    }

    if (!initialization.usable) {
        cpu_backend->shutdown();
        return EXIT_FAILURE;
    }

    // The first /proc/stat sample is deliberately baseline-only. Refresh a
    // few times so ordinary Linux hosts can expose a measured delta while
    // still accepting environments whose counters do not advance.
    pb::ComputeBackendStatus refreshed = initialization;
    for (int attempt = 0; attempt < 5; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        refreshed = cpu_backend->refresh();
        const pc::CpuBackendInventorySnapshot sample =
            cpu_backend->inventory_snapshot();
        if (sample.health.aggregate_utilization.state ==
            pc::CpuTelemetryState::available) {
            break;
        }
    }

    const pb::ComputeBackendSnapshot generic = cpu_backend->snapshot();
    const pc::CpuBackendInventorySnapshot inventory =
        cpu_backend->inventory_snapshot();

    std::cout << "\nGeneric backend snapshot\n"
              << "  id: " << generic.backend_id << '\n'
              << "  kind: " << pb::to_string(generic.kind) << '\n'
              << "  state: " << pb::to_string(generic.state) << '\n'
              << "  usable: " << (generic.usable ? "yes" : "no") << '\n'
              << "  visible resources: "
              << generic.visible_resource_count << '\n'
              << "  advisory worker lanes: "
              << generic.usable_resource_count << '\n';
    if (!generic.message.empty()) {
        std::cout << "  message: " << generic.message << '\n';
    }

    const pc::CpuTopologySnapshot& topology = inventory.topology;
    const pc::CpuTopologySummary& topology_summary = topology.summary;
    std::cout << "\nTopology\n"
              << "  status: "
              << pc::to_string(inventory.topology_status.code) << '\n'
              << "  configured logical processors: "
              << topology_summary.configured_logical_processor_count << '\n'
              << "  online logical processors: "
              << topology_summary.online_logical_processor_count << '\n'
              << "  process-allowed logical processors: "
              << topology_summary.process_allowed_logical_processor_count
              << '\n'
              << "  process affinity known: "
              << (topology_summary.process_affinity_known ? "yes" : "no")
              << '\n'
              << "  physical cores: "
              << topology_summary.physical_core_count << '\n'
              << "  packages: " << topology_summary.package_count << '\n'
              << "  NUMA nodes: " << topology_summary.numa_node_count << '\n'
              << "  cache records: " << topology_summary.cache_count << '\n'
              << "  SMT present: "
              << (topology_summary.simultaneous_multithreading_present
                      ? "yes"
                      : "no")
              << '\n'
              << "  heterogeneous core types: "
              << (topology_summary.heterogeneous_core_types_present
                      ? "yes"
                      : "no")
              << '\n'
              << "  heterogeneous capacity hints: "
              << (topology_summary.heterogeneous_capacity_hints_present
                      ? "yes"
                      : "no")
              << '\n';

    for (const pc::CpuPackageTopology& package : topology.packages) {
        std::cout << "  package " << package.identity.node_relative_key
                  << ": logical="
                  << package.identity.logical_processor_ids.size()
                  << ", cores=" << package.cores.size()
                  << ", NUMA-memberships=" << package.numa_node_ids.size()
                  << '\n';
        if (!package.identity.signature.vendor_id.empty() ||
            !package.identity.signature.model_name.empty()) {
            std::cout << "    vendor="
                      << package.identity.signature.vendor_id
                      << ", model=\""
                      << package.identity.signature.model_name << "\"\n";
        }
    }

    for (const pc::CpuNumaNodeTopology& node : topology.numa_nodes) {
        std::cout << "  NUMA node " << node.numa_node_id
                  << ": logical=" << node.logical_processor_ids.size()
                  << ", synthetic="
                  << (node.synthetic_single_node ? "yes" : "no");
        if (node.total_memory_available) {
            std::cout << ", memory_gib=" << std::fixed
                      << std::setprecision(2)
                      << static_cast<double>(node.total_memory_bytes) /
                             kBytesPerGiB;
        } else {
            std::cout << ", memory=unknown";
        }
        std::cout << '\n';
    }

    const pc::CpuCapabilitySnapshot& capabilities = inventory.capabilities;
    std::cout << "\nCapabilities\n"
              << "  status: "
              << pc::to_string(inventory.capability_status.code) << '\n'
              << "  architecture: "
              << pc::to_string(capabilities.isa.architecture) << '\n'
              << "  source: "
              << pc::to_string(capabilities.isa.source) << '\n'
              << "  target logical processors: "
              << capabilities.target_logical_processor_count << '\n'
              << "  pointer width: "
              << capabilities.isa.pointer_width_bits << '\n'
              << "  little endian: "
              << (capabilities.isa.little_endian ? "yes" : "no") << '\n'
              << "  heterogeneous feature sets: "
              << (capabilities.isa.feature_sets_heterogeneous ? "yes" : "no")
              << '\n'
              << "  common SIMD level: "
              << pc::to_string(capabilities.isa.common_simd_level) << '\n'
              << "  common vector width bytes: "
              << capabilities.isa.common_vector_width_bytes << '\n'
              << "  FP32 common: "
              << pc::to_string(capabilities.isa.fp32.common) << '\n'
              << "  FP64 common: "
              << pc::to_string(capabilities.isa.fp64.common) << '\n'
              << "  lock-free atomic u64 common: "
              << pc::to_string(
                     capabilities.isa.lock_free_atomic_u64.common)
              << '\n';

    const pc::CpuHealthSnapshot& health = inventory.health;
    std::cout << "\nHealth\n"
              << "  status: "
              << pc::to_string(inventory.health_status.code) << '\n'
              << "  aggregate utilization state: "
              << pc::to_string(health.aggregate_utilization.state) << '\n';
    if (health.aggregate_utilization.state ==
        pc::CpuTelemetryState::available) {
        std::cout << "  aggregate busy percent: " << std::fixed
                  << std::setprecision(2)
                  << health.aggregate_utilization.busy_fraction * 100.0
                  << '\n';
    }

    std::cout << "  load average state: "
              << pc::to_string(health.load_average.state);
    if (health.load_average.state == pc::CpuTelemetryState::available) {
        std::cout << ", 1m=" << health.load_average.one_minute
                  << ", 5m=" << health.load_average.five_minutes
                  << ", 15m=" << health.load_average.fifteen_minutes;
    }
    std::cout << '\n';

    std::cout << "  memory state: "
              << pc::to_string(health.memory.state);
    if (health.memory.state == pc::CpuTelemetryState::available) {
        std::cout << ", total_gib=" << std::fixed << std::setprecision(2)
                  << static_cast<double>(health.memory.total_bytes) /
                         kBytesPerGiB
                  << ", available_gib="
                  << static_cast<double>(health.memory.available_bytes) /
                         kBytesPerGiB;
    }
    std::cout << '\n'
              << "  frequency records: " << health.frequencies.size()
              << '\n'
              << "  temperature provider: "
              << pc::to_string(health.temperature_provider_state) << '\n'
              << "  temperature records: " << health.temperatures.size()
              << '\n';
    print_pressure("  CPU pressure", health.cpu_pressure);
    print_pressure("  memory pressure", health.memory_pressure);

    const pc::CpuCapacitySnapshot& capacity = inventory.capacity;
    const pc::CpuCapacityPlan& plan = inventory.capacity_plan;
    std::cout << "\nCapacity\n"
              << "  status: "
              << pc::to_string(inventory.capacity_status.code) << '\n'
              << "  allowed physical cores: "
              << capacity.process_allowed_physical_core_count << '\n'
              << "  execution groups: "
              << capacity.execution_groups.size() << '\n'
              << "  heterogeneous execution groups: "
              << (capacity.heterogeneous_execution_groups_present
                      ? "yes"
                      : "no")
              << '\n'
              << "  plan status: " << pc::to_string(plan.status.code) << '\n'
              << "  candidate worker lanes: "
              << plan.candidate_worker_lane_count << '\n'
              << "  reserved physical cores: "
              << plan.reserved_physical_core_count << '\n'
              << "  reserved logical processors: "
              << plan.reserved_logical_processor_count << '\n'
              << "  advisory worker lanes: "
              << plan.advisory_worker_lane_count << '\n';

    for (const pc::CpuExecutionGroup& group :
         plan.worker_execution_groups) {
        std::cout << "  group " << group.group_key
                  << ": numa=" << group.numa_node_id
                  << ", cores=" << group.physical_core_keys.size()
                  << ", worker logical processors="
                  << group.logical_processor_ids.size()
                  << ", SIMD=" << pc::to_string(group.safe_simd_level)
                  << '\n';
    }

    const std::size_t issue_count = topology.issues.size() +
        capabilities.issues.size() + health.issues.size() +
        capacity.issues.size();
    std::cout << "\nIssues: " << issue_count << '\n';
    for (const pc::CpuTopologyIssue& issue : topology.issues) {
        print_issue_line(
            "topology",
            pc::to_string(issue.code),
            issue.fatal,
            issue.message,
            issue.os_cpu_id);
    }
    for (const pc::CpuCapabilityIssue& issue : capabilities.issues) {
        print_issue_line(
            "capability",
            pc::to_string(issue.code),
            issue.fatal,
            issue.message,
            issue.os_cpu_id);
    }
    for (const pc::CpuHealthIssue& issue : health.issues) {
        print_issue_line(
            "health",
            pc::to_string(issue.code),
            issue.fatal,
            issue.message,
            issue.os_cpu_id);
    }
    for (const pc::CpuCapacityIssue& issue : capacity.issues) {
        print_issue_line(
            "capacity",
            pc::to_string(issue.code),
            issue.fatal,
            issue.message);
    }

    const bool passed = refreshed.usable && generic.usable &&
        inventory.topology_status.completed() &&
        inventory.capability_status.completed() &&
        inventory.health_status.completed() &&
        inventory.capacity_status.completed() &&
        plan.status.usable() &&
        generic.visible_resource_count > 0 &&
        generic.usable_resource_count > 0 &&
        capabilities.target_logical_processor_count > 0;

    cpu_backend->shutdown();
    std::cout << "\nFinal backend state: "
              << pb::to_string(cpu_backend->status().state) << '\n'
              << (passed ? "Phase 7.1 CPU backend probe passed\n"
                         : "Phase 7.1 CPU backend probe failed\n");
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
