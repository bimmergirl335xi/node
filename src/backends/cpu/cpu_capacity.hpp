#ifndef SRC_BACKENDS_CPU_CPU_CAPACITY_HPP
#define SRC_BACKENDS_CPU_CPU_CAPACITY_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "cpu_capabilities.hpp"
#include "cpu_health.hpp"
#include "cpu_topology.hpp"

namespace prometheus::backends::cpu {

enum class CpuCapacityIssueCode : std::uint8_t {
    topology_unusable = 0,
    no_allowed_processors,
    physical_core_mapping_incomplete,
    numa_mapping_incomplete,
    total_memory_unknown,
    available_memory_unknown,
};

struct CpuCapacityIssue {
    CpuCapacityIssueCode code = CpuCapacityIssueCode::topology_unusable;
    bool fatal = false;
    std::string message{};
};

struct CpuExecutionGroup {
    std::string group_key{};
    int numa_node_id = -1;
    int core_type = -1;
    std::uint64_t capacity_hint = 0;
    bool capacity_hint_available = false;
    CpuSimdLevel safe_simd_level = CpuSimdLevel::scalar;

    std::vector<std::string> physical_core_keys{};
    std::vector<int> logical_processor_ids{};
};

enum class CpuCapacityQueryCode : std::uint8_t {
    success = 0,
    partial_success,
    query_failed,
};

struct CpuCapacityQueryStatus {
    CpuCapacityQueryCode code = CpuCapacityQueryCode::success;
    std::string message{};

    [[nodiscard]] bool completed() const noexcept {
        return code == CpuCapacityQueryCode::success ||
               code == CpuCapacityQueryCode::partial_success;
    }
};

struct CpuCapacitySnapshot {
    std::size_t configured_logical_processor_count = 0;
    std::size_t online_logical_processor_count = 0;
    std::size_t process_allowed_logical_processor_count = 0;
    std::size_t physical_core_count = 0;
    std::size_t process_allowed_physical_core_count = 0;
    std::size_t package_count = 0;
    std::size_t numa_node_count = 0;

    bool process_affinity_known = false;
    bool simultaneous_multithreading_present = false;
    bool heterogeneous_execution_groups_present = false;

    CpuSimdLevel common_simd_level = CpuSimdLevel::scalar;
    std::size_t common_vector_width_bytes = 0;

    CpuTelemetryState total_memory_state = CpuTelemetryState::unknown;
    CpuTelemetryState available_memory_state = CpuTelemetryState::unknown;
    std::uint64_t total_memory_bytes = 0;
    std::uint64_t available_memory_bytes = 0;

    std::vector<int> process_allowed_logical_processor_ids{};
    std::vector<CpuExecutionGroup> execution_groups{};
    std::vector<CpuCapacityIssue> issues{};
};

struct CpuCapacityQueryResult {
    CpuCapacityQueryStatus status{};
    CpuCapacitySnapshot capacity{};
};

struct CpuCapacityPolicy {
    std::size_t reserved_physical_cores = 0;
    std::size_t reserved_logical_processors = 0;
    bool preserve_one_physical_core_per_numa_node = false;
    bool count_smt_siblings_as_worker_lanes = false;
    std::size_t minimum_worker_lanes = 1;
};

enum class CpuCapacityPlanCode : std::uint8_t {
    success = 0,
    insufficient_capacity,
    invalid_policy,
};

struct CpuCapacityPlanStatus {
    CpuCapacityPlanCode code = CpuCapacityPlanCode::success;
    std::string message{};

    [[nodiscard]] bool usable() const noexcept {
        return code == CpuCapacityPlanCode::success;
    }
};

struct CpuCapacityPlan {
    CpuCapacityPlanStatus status{};
    CpuCapacityPolicy policy{};

    std::size_t candidate_worker_lane_count = 0;
    std::size_t reserved_physical_core_count = 0;
    std::size_t reserved_logical_processor_count = 0;
    std::size_t advisory_worker_lane_count = 0;

    std::vector<std::string> reserved_physical_core_keys{};
    std::vector<int> reserved_logical_processor_ids{};
    std::vector<int> worker_logical_processor_ids{};
    std::vector<CpuExecutionGroup> worker_execution_groups{};
};

[[nodiscard]] CpuCapacityQueryResult query_cpu_capacity(
    const CpuTopologySnapshot& topology,
    const CpuCapabilitySnapshot& capabilities,
    const CpuHealthSnapshot& health);

// Advisory only: does not start workers, change affinity, reserve memory, or
// bind any cognitive workload to a CPU.
[[nodiscard]] CpuCapacityPlan plan_cpu_capacity(
    const CpuTopologySnapshot& topology,
    const CpuCapacitySnapshot& capacity,
    const CpuCapacityPolicy& policy = {});

[[nodiscard]] const char* to_string(CpuCapacityIssueCode value) noexcept;
[[nodiscard]] const char* to_string(CpuCapacityQueryCode value) noexcept;
[[nodiscard]] const char* to_string(CpuCapacityPlanCode value) noexcept;

}  // namespace prometheus::backends::cpu

#endif  // SRC_BACKENDS_CPU_CPU_CAPACITY_HPP
