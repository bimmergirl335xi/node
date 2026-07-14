#ifndef SRC_BACKENDS_CPU_CPU_BACKEND_HPP
#define SRC_BACKENDS_CPU_CPU_BACKEND_HPP

#include <mutex>

#include "../compute_backend.hpp"
#include "cpu_capabilities.hpp"
#include "cpu_capacity.hpp"
#include "cpu_health.hpp"
#include "cpu_topology.hpp"

namespace prometheus::backends::cpu {

struct CpuBackendOptions {
    // Minimal Linux nodes may legitimately lack the topology interfaces used by
    // this release. Treat that as unavailable by default so other registered
    // backends may continue operating without making CPU discovery fatal.
    bool unsupported_platform_is_unavailable = true;

    // Advisory only. The backend reports the plan but does not create workers,
    // pin threads, reserve memory, or consume the listed processors.
    CpuCapacityPolicy capacity_policy{};
};

struct CpuBackendInventorySnapshot {
    CpuTopologyQueryStatus topology_status{};
    CpuCapabilityQueryStatus capability_status{};
    CpuHealthQueryStatus health_status{};
    CpuCapacityQueryStatus capacity_status{};

    CpuTopologySnapshot topology{};
    CpuCapabilitySnapshot capabilities{};
    CpuHealthSnapshot health{};
    CpuCapacitySnapshot capacity{};
    CpuCapacityPlan capacity_plan{};
};

// Phase 7.1 CPU backend facade. It exposes read-only CPU identity, topology,
// health, affinity-aware capacity, and ISA capability inventory. It does not
// create a worker pool, change affinity, execute reference operations, or
// become the cognitive master of the node.
class CpuBackend final : public ComputeBackend {
public:
    explicit CpuBackend(CpuBackendOptions options = {});

    [[nodiscard]] std::string_view backend_id() const noexcept override;
    [[nodiscard]] std::string_view display_name() const noexcept override;
    [[nodiscard]] ComputeBackendKind kind() const noexcept override;

    ComputeBackendStatus initialize() override;
    ComputeBackendStatus refresh() override;
    void shutdown() noexcept override;

    [[nodiscard]] ComputeBackendStatus status() const override;
    [[nodiscard]] ComputeBackendSnapshot snapshot() const override;

    [[nodiscard]] CpuBackendInventorySnapshot inventory_snapshot() const;

private:
    [[nodiscard]] ComputeBackendStatus rebuild_inventory();

    CpuBackendOptions options_{};
    // Serializes initialize, refresh, and shutdown so an older discovery
    // operation cannot publish state after a newer lifecycle transition.
    mutable std::mutex lifecycle_mutex_{};
    mutable std::mutex mutex_{};
    ComputeBackendStatus status_{};
    CpuBackendInventorySnapshot inventory_{};
};

}  // namespace prometheus::backends::cpu

#endif  // SRC_BACKENDS_CPU_CPU_BACKEND_HPP
