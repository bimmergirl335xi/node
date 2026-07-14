#ifndef SRC_BACKENDS_CPU_CPU_TOPOLOGY_HPP
#define SRC_BACKENDS_CPU_CPU_TOPOLOGY_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "cpu_identity.hpp"

namespace prometheus::backends::cpu {

// Tri-state membership prevents an unavailable affinity or hotplug query from
// being silently interpreted as either allowed or disallowed.
enum class CpuMembershipState : std::uint8_t {
    unknown = 0,
    no,
    yes,
};

enum class CpuCacheType : std::uint8_t {
    unknown = 0,
    data,
    instruction,
    unified,
};

enum class CpuTopologyIssueCode : std::uint8_t {
    online_cpu_list_unavailable = 0,
    process_affinity_unavailable,
    package_id_unavailable,
    die_id_unavailable,
    core_id_unavailable,
    sibling_list_unavailable,
    numa_node_unavailable,
    numa_memory_unavailable,
    cache_query_failed,
    proc_cpuinfo_unavailable,
    duplicate_logical_processor,
    malformed_topology_value,
    no_processors_discovered,
};

struct CpuTopologyIssue {
    CpuTopologyIssueCode code =
        CpuTopologyIssueCode::online_cpu_list_unavailable;
    int os_cpu_id = -1;
    bool fatal = false;
    std::string message{};
};

struct CpuCacheInfo {
    std::string node_relative_key{};
    int level = -1;
    CpuCacheType type = CpuCacheType::unknown;

    std::uint64_t size_bytes = 0;
    std::uint32_t coherency_line_size_bytes = 0;

    std::vector<int> shared_logical_processor_ids{};
};

struct CpuLogicalProcessorTopology {
    CpuLogicalProcessorIdentity identity{};

    CpuMembershipState online = CpuMembershipState::unknown;
    CpuMembershipState process_allowed = CpuMembershipState::unknown;

    int numa_node_id = -1;

    // Raw Linux topology hints. Their interpretation is architecture-specific,
    // so Phase 7.1 exposes them without making scheduling policy decisions.
    int core_type = -1;
    std::uint64_t capacity_hint = 0;
    bool capacity_hint_available = false;

    std::vector<int> thread_sibling_ids{};
    std::vector<int> package_sibling_ids{};
};

struct CpuCoreTopology {
    CpuCoreIdentity identity{};
    int numa_node_id = -1;

    int core_type = -1;
    std::uint64_t capacity_hint = 0;
    bool capacity_hint_available = false;
};

struct CpuPackageTopology {
    CpuPackageIdentity identity{};
    std::vector<int> numa_node_ids{};
    std::vector<CpuCoreIdentity> cores{};
};

struct CpuNumaNodeTopology {
    int numa_node_id = -1;
    bool synthetic_single_node = false;

    std::vector<int> logical_processor_ids{};

    std::uint64_t total_memory_bytes = 0;
    bool total_memory_available = false;
};

struct CpuTopologySummary {
    std::size_t configured_logical_processor_count = 0;
    std::size_t online_logical_processor_count = 0;
    std::size_t process_allowed_logical_processor_count = 0;

    std::size_t package_count = 0;
    std::size_t physical_core_count = 0;
    std::size_t numa_node_count = 0;
    std::size_t cache_count = 0;

    bool process_affinity_known = false;
    bool simultaneous_multithreading_present = false;
    bool heterogeneous_core_types_present = false;
    bool heterogeneous_capacity_hints_present = false;
};

enum class CpuTopologyQueryCode : std::uint8_t {
    success = 0,
    partial_success,
    unsupported_platform,
    query_failed,
};

struct CpuTopologyQueryStatus {
    CpuTopologyQueryCode code = CpuTopologyQueryCode::success;
    std::string message{};

    [[nodiscard]] bool completed() const noexcept {
        return code == CpuTopologyQueryCode::success ||
               code == CpuTopologyQueryCode::partial_success;
    }
};

struct CpuTopologySnapshot {
    CpuTopologySummary summary{};

    std::vector<CpuLogicalProcessorTopology> logical_processors{};
    std::vector<CpuCoreTopology> cores{};
    std::vector<CpuPackageTopology> packages{};
    std::vector<CpuNumaNodeTopology> numa_nodes{};
    std::vector<CpuCacheInfo> caches{};

    std::vector<CpuTopologyIssue> issues{};
};

struct CpuTopologyQueryResult {
    CpuTopologyQueryStatus status{};
    CpuTopologySnapshot topology{};
};

// Linux discovery is read-only. It does not pin threads, change affinity,
// create worker pools, allocate production memory, or select a cognitive role.
[[nodiscard]] CpuTopologyQueryResult query_cpu_topology();

[[nodiscard]] const char* to_string(CpuMembershipState value) noexcept;
[[nodiscard]] const char* to_string(CpuCacheType value) noexcept;
[[nodiscard]] const char* to_string(CpuTopologyIssueCode value) noexcept;
[[nodiscard]] const char* to_string(CpuTopologyQueryCode value) noexcept;

}  // namespace prometheus::backends::cpu

#endif  // SRC_BACKENDS_CPU_CPU_TOPOLOGY_HPP
