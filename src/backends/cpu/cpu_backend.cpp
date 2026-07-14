#include "cpu_backend.hpp"

#include <utility>

namespace prometheus::backends::cpu {
namespace {

[[nodiscard]] std::size_t usable_processor_count(
    const CpuTopologySnapshot& topology) noexcept {
    if (topology.summary.process_affinity_known) {
        return topology.summary.process_allowed_logical_processor_count;
    }
    return topology.summary.online_logical_processor_count;
}

[[nodiscard]] ComputeBackendStatus map_inventory_status(
    const CpuTopologyQueryResult& topology_result,
    const CpuCapabilityQueryResult& capability_result,
    const CpuCapacityQueryResult& capacity_result,
    const CpuCapacityPlan& capacity_plan,
    bool unsupported_platform_is_unavailable) {
    ComputeBackendStatus status{};

    switch (topology_result.status.code) {
        case CpuTopologyQueryCode::success:
        case CpuTopologyQueryCode::partial_success:
            break;

        case CpuTopologyQueryCode::unsupported_platform:
            status.state = unsupported_platform_is_unavailable
                               ? ComputeBackendState::unavailable
                               : ComputeBackendState::failed;
            status.usable = false;
            status.message = topology_result.status.message.empty()
                                 ? "CPU topology discovery is unsupported"
                                 : topology_result.status.message;
            return status;

        case CpuTopologyQueryCode::query_failed:
            status.state = ComputeBackendState::failed;
            status.usable = false;
            status.message = topology_result.status.message.empty()
                                 ? "CPU topology discovery failed"
                                 : topology_result.status.message;
            return status;
    }

    const std::size_t usable_count =
        usable_processor_count(topology_result.topology);
    if (usable_count == 0) {
        status.state = ComputeBackendState::unavailable;
        status.usable = false;
        status.message =
            "CPU topology was discovered, but no processor is available to "
            "this process";
        return status;
    }

    if (!capacity_result.status.completed()) {
        status.state = ComputeBackendState::degraded;
        status.usable = true;
        status.message = capacity_result.status.message.empty()
                             ? "CPU inventory is usable, but capacity "
                               "reporting failed"
                             : capacity_result.status.message;
        return status;
    }

    if (!capacity_plan.status.usable()) {
        status.state = ComputeBackendState::degraded;
        status.usable = capacity_plan.advisory_worker_lane_count > 0;
        status.message = capacity_plan.status.message.empty()
                             ? "CPU inventory is usable, but the advisory "
                               "capacity policy is unsatisfied"
                             : capacity_plan.status.message;
        return status;
    }

    status.usable = true;

    const bool topology_complete =
        topology_result.status.code == CpuTopologyQueryCode::success;
    const bool capabilities_complete =
        capability_result.status.code == CpuCapabilityQueryCode::success;
    const bool capacity_complete =
        capacity_result.status.code == CpuCapacityQueryCode::success;

    if (topology_complete && capabilities_complete && capacity_complete) {
        status.state = ComputeBackendState::ready;
        status.message = "CPU backend inventory is ready";
        return status;
    }

    status.state = ComputeBackendState::degraded;

    if (topology_result.status.code ==
        CpuTopologyQueryCode::partial_success) {
        status.message = topology_result.status.message.empty()
                             ? "CPU topology inventory is incomplete"
                             : topology_result.status.message;
        return status;
    }

    if (capability_result.status.code != CpuCapabilityQueryCode::success) {
        status.message = capability_result.status.message.empty()
                             ? "CPU ISA capability inventory is incomplete"
                             : capability_result.status.message;
        return status;
    }

    status.message = capacity_result.status.message.empty()
                         ? "CPU capacity inventory is incomplete"
                         : capacity_result.status.message;
    return status;
}

}  // namespace

CpuBackend::CpuBackend(CpuBackendOptions options)
    : options_(std::move(options)) {}

std::string_view CpuBackend::backend_id() const noexcept {
    return "cpu";
}

std::string_view CpuBackend::display_name() const noexcept {
    return "Prometheus CPU Backend";
}

ComputeBackendKind CpuBackend::kind() const noexcept {
    return ComputeBackendKind::cpu;
}

ComputeBackendStatus CpuBackend::initialize() {
    return rebuild_inventory();
}

ComputeBackendStatus CpuBackend::refresh() {
    return rebuild_inventory();
}

void CpuBackend::shutdown() noexcept {
    std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
    std::lock_guard<std::mutex> lock(mutex_);
    inventory_ = {};
    status_.state = ComputeBackendState::stopped;
    status_.usable = false;
    status_.message = "CPU backend inventory was released";
}

ComputeBackendStatus CpuBackend::status() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

ComputeBackendSnapshot CpuBackend::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);

    ComputeBackendSnapshot result{};
    result.backend_id = std::string{backend_id()};
    result.display_name = std::string{display_name()};
    result.kind = kind();
    result.state = status_.state;
    result.usable = status_.usable;
    result.visible_resource_count =
        inventory_.topology.summary.online_logical_processor_count;
    result.usable_resource_count =
        inventory_.capacity_plan.advisory_worker_lane_count;
    result.message = status_.message;
    return result;
}

CpuBackendInventorySnapshot CpuBackend::inventory_snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return inventory_;
}

ComputeBackendStatus CpuBackend::rebuild_inventory() {
    std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);

    CpuHealthSnapshot previous_health{};
    bool previous_health_available = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        previous_health = inventory_.health;
        previous_health_available =
            previous_health.sample_time_monotonic_nanoseconds != 0;
        status_.state = ComputeBackendState::initializing;
        status_.usable = false;
        status_.message = "Discovering CPU resources";
    }

    CpuTopologyQueryResult topology_result = query_cpu_topology();

    CpuCapabilityQueryResult capability_result{};
    CpuHealthQueryResult health_result{};
    CpuCapacityQueryResult capacity_result{};
    CpuCapacityPlan capacity_plan{};

    if (topology_result.status.completed()) {
        capability_result = query_cpu_capabilities(topology_result.topology);
        health_result = query_cpu_health(
            topology_result.topology,
            previous_health_available ? &previous_health : nullptr);
        capacity_result = query_cpu_capacity(
            topology_result.topology,
            capability_result.capabilities,
            health_result.health);
        if (capacity_result.status.completed()) {
            capacity_plan = plan_cpu_capacity(
                topology_result.topology,
                capacity_result.capacity,
                options_.capacity_policy);
        } else {
            capacity_plan.status.code =
                CpuCapacityPlanCode::insufficient_capacity;
            capacity_plan.status.message =
                "capacity planning was not attempted because capacity "
                "discovery did not complete";
        }
    } else {
        capability_result.status.code = CpuCapabilityQueryCode::query_failed;
        capability_result.status.message =
            "CPU capability query was not attempted because topology "
            "discovery did not complete";
        health_result.status.code = CpuHealthQueryCode::query_failed;
        health_result.status.message =
            "CPU health query was not attempted because topology discovery "
            "did not complete";
        capacity_result.status.code = CpuCapacityQueryCode::query_failed;
        capacity_result.status.message =
            "CPU capacity query was not attempted because topology discovery "
            "did not complete";
        capacity_plan.status.code = CpuCapacityPlanCode::insufficient_capacity;
        capacity_plan.status.message =
            "CPU capacity planning was not attempted";
    }

    ComputeBackendStatus mapped = map_inventory_status(
        topology_result,
        capability_result,
        capacity_result,
        capacity_plan,
        options_.unsupported_platform_is_unavailable);

    CpuBackendInventorySnapshot inventory{};
    inventory.topology_status = topology_result.status;
    inventory.capability_status = capability_result.status;
    inventory.health_status = health_result.status;
    inventory.capacity_status = capacity_result.status;
    inventory.topology = std::move(topology_result.topology);
    inventory.capabilities = std::move(capability_result.capabilities);
    inventory.health = std::move(health_result.health);
    inventory.capacity = std::move(capacity_result.capacity);
    inventory.capacity_plan = std::move(capacity_plan);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        inventory_ = std::move(inventory);
        status_ = mapped;
    }

    return mapped;
}

}  // namespace prometheus::backends::cpu
