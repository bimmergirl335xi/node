#include <chrono>
#include <cstdlib>
#include <iostream>
#include <set>
#include <thread>

#include "cpu_capabilities.hpp"
#include "cpu_capacity.hpp"
#include "cpu_health.hpp"
#include "cpu_topology.hpp"

namespace cpu = prometheus::backends::cpu;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "CPU health/capacity test failed: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    const cpu::CpuTopologyQueryResult topology_result =
        cpu::query_cpu_topology();
    require(topology_result.status.completed(),
            "topology discovery must complete");

    const cpu::CpuCapabilityQueryResult capability_result =
        cpu::query_cpu_capabilities(topology_result.topology);
    require(capability_result.status.completed(),
            "capability discovery must complete");

    const cpu::CpuHealthQueryResult first_health =
        cpu::query_cpu_health(topology_result.topology);
    require(first_health.status.completed(),
            "first health sample must complete");
    require(first_health.health.sample_time_unix_nanoseconds > 0,
            "health sample must contain a wall-clock timestamp");
    require(first_health.health.sample_time_monotonic_nanoseconds > 0,
            "health sample must contain a monotonic timestamp");
    require(first_health.health.aggregate_utilization.counters.available,
            "aggregate CPU counters must be available on Linux");
    require(first_health.health.aggregate_utilization.state ==
                cpu::CpuTelemetryState::baseline_only,
            "first utilization sample must remain a baseline");
    require(first_health.health.memory.state ==
                cpu::CpuTelemetryState::available,
            "memory telemetry must be available from /proc/meminfo");
    require(first_health.health.memory.total_bytes > 0,
            "total memory must be non-zero");
    require(first_health.health.memory.available_bytes <=
                first_health.health.memory.total_bytes,
            "available memory cannot exceed total memory");

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    const cpu::CpuHealthQueryResult second_health =
        cpu::query_cpu_health(
            topology_result.topology, &first_health.health);
    require(second_health.status.completed(),
            "second health sample must complete");
    require(second_health.health.sample_time_monotonic_nanoseconds >
                first_health.health.sample_time_monotonic_nanoseconds,
            "health sample timestamps must advance");
    require(second_health.health.aggregate_utilization.state ==
                cpu::CpuTelemetryState::available ||
                second_health.health.aggregate_utilization.state ==
                    cpu::CpuTelemetryState::baseline_only,
            "second utilization sample must be available or explicitly remain baseline-only");

    const cpu::CpuCapacityQueryResult capacity_result =
        cpu::query_cpu_capacity(
            topology_result.topology,
            capability_result.capabilities,
            second_health.health);
    require(capacity_result.status.completed(),
            "capacity discovery must complete");
    require(capacity_result.capacity.online_logical_processor_count > 0,
            "online processor capacity must be non-zero");
    require(
        capacity_result.capacity.process_allowed_logical_processor_count > 0,
        "allowed processor capacity must be non-zero");
    require(
        capacity_result.capacity.process_allowed_logical_processor_count ==
            capacity_result.capacity
                .process_allowed_logical_processor_ids.size(),
        "allowed processor count must match IDs");
    require(
        capacity_result.capacity.process_allowed_physical_core_count > 0,
        "allowed physical core capacity must be non-zero");
    require(!capacity_result.capacity.execution_groups.empty(),
            "at least one execution group must exist");
    require(capacity_result.capacity.available_memory_state ==
                cpu::CpuTelemetryState::available,
            "capacity must retain current available-memory telemetry");

    const cpu::CpuCapacityPlan default_plan = cpu::plan_cpu_capacity(
        topology_result.topology, capacity_result.capacity);
    require(default_plan.status.usable(),
            "default capacity plan must be usable");
    require(default_plan.advisory_worker_lane_count > 0,
            "default plan must expose at least one worker lane");
    require(default_plan.worker_logical_processor_ids.size() ==
                default_plan.advisory_worker_lane_count,
            "worker lane count must match worker IDs");

    std::set<int> worker_ids(
        default_plan.worker_logical_processor_ids.begin(),
        default_plan.worker_logical_processor_ids.end());
    require(worker_ids.size() ==
                default_plan.worker_logical_processor_ids.size(),
            "worker processor IDs must be unique");

    if (capacity_result.capacity.process_allowed_physical_core_count > 1) {
        cpu::CpuCapacityPolicy reserved_policy{};
        reserved_policy.reserved_physical_cores = 1;
        const cpu::CpuCapacityPlan reserved_plan = cpu::plan_cpu_capacity(
            topology_result.topology,
            capacity_result.capacity,
            reserved_policy);
        require(reserved_plan.status.usable(),
                "one-core reservation must be usable when multiple cores exist");
        require(reserved_plan.reserved_physical_core_count == 1,
                "one physical core must be reserved");
        require(reserved_plan.advisory_worker_lane_count <
                    default_plan.advisory_worker_lane_count,
                "reserving a core must reduce worker lanes");
    }

    cpu::CpuCapacityPolicy impossible_policy{};
    impossible_policy.reserved_physical_cores =
        capacity_result.capacity.process_allowed_physical_core_count + 1;
    const cpu::CpuCapacityPlan impossible_plan = cpu::plan_cpu_capacity(
        topology_result.topology,
        capacity_result.capacity,
        impossible_policy);
    require(impossible_plan.status.code ==
                cpu::CpuCapacityPlanCode::insufficient_capacity,
            "impossible reservations must be rejected explicitly");

    return EXIT_SUCCESS;
}
