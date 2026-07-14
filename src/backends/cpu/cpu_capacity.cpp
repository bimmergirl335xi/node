#include "cpu_capacity.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

namespace prometheus::backends::cpu {
namespace {

void add_issue(
    CpuCapacitySnapshot& snapshot,
    CpuCapacityIssueCode code,
    bool fatal,
    std::string message) {
    snapshot.issues.push_back(
        CpuCapacityIssue{code, fatal, std::move(message)});
}

[[nodiscard]] std::vector<int> allowed_processor_ids(
    const CpuTopologySnapshot& topology) {
    std::vector<int> result;
    const bool affinity_known = topology.summary.process_affinity_known;

    for (const CpuLogicalProcessorTopology& processor :
         topology.logical_processors) {
        if (affinity_known) {
            if (processor.process_allowed == CpuMembershipState::yes) {
                result.push_back(processor.identity.os_cpu_id);
            }
        } else if (processor.online == CpuMembershipState::yes) {
            result.push_back(processor.identity.os_cpu_id);
        }
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

[[nodiscard]] bool contains_id(
    const std::set<int>& ids,
    const std::vector<int>& candidates) {
    return std::any_of(
        candidates.begin(), candidates.end(),
        [&ids](int cpu_id) { return ids.count(cpu_id) != 0; });
}

struct GroupKey {
    int numa_node_id = -1;
    int core_type = -1;
    std::uint64_t capacity_hint = 0;
    bool capacity_hint_available = false;

    [[nodiscard]] bool operator<(const GroupKey& other) const noexcept {
        return std::tie(
                   numa_node_id,
                   core_type,
                   capacity_hint_available,
                   capacity_hint) <
               std::tie(
                   other.numa_node_id,
                   other.core_type,
                   other.capacity_hint_available,
                   other.capacity_hint);
    }
};

[[nodiscard]] std::string execution_group_key(const GroupKey& key) {
    std::ostringstream stream;
    stream << "cpu:execution-group:numa:";
    if (key.numa_node_id >= 0) {
        stream << key.numa_node_id;
    } else {
        stream << "unknown";
    }
    stream << ":core-type:";
    if (key.core_type >= 0) {
        stream << key.core_type;
    } else {
        stream << "unknown";
    }
    stream << ":capacity:";
    if (key.capacity_hint_available) {
        stream << key.capacity_hint;
    } else {
        stream << "unknown";
    }
    return stream.str();
}

[[nodiscard]] std::map<int, const CpuLogicalProcessorTopology*>
logical_processor_lookup(const CpuTopologySnapshot& topology) {
    std::map<int, const CpuLogicalProcessorTopology*> lookup;
    for (const CpuLogicalProcessorTopology& processor :
         topology.logical_processors) {
        lookup.emplace(processor.identity.os_cpu_id, &processor);
    }
    return lookup;
}

void populate_execution_groups(
    CpuCapacitySnapshot& snapshot,
    const CpuTopologySnapshot& topology,
    const CpuCapabilitySnapshot& capabilities,
    const std::set<int>& allowed_ids) {
    std::map<GroupKey, CpuExecutionGroup> groups;

    for (const CpuCoreTopology& core : topology.cores) {
        std::vector<int> core_allowed_ids;
        for (const int cpu_id : core.identity.logical_processor_ids) {
            if (allowed_ids.count(cpu_id) != 0) {
                core_allowed_ids.push_back(cpu_id);
            }
        }
        if (core_allowed_ids.empty()) {
            continue;
        }

        GroupKey key{};
        key.numa_node_id = core.numa_node_id;
        key.core_type = core.core_type;
        key.capacity_hint = core.capacity_hint;
        key.capacity_hint_available = core.capacity_hint_available;

        CpuExecutionGroup& group = groups[key];
        if (group.group_key.empty()) {
            group.group_key = execution_group_key(key);
            group.numa_node_id = key.numa_node_id;
            group.core_type = key.core_type;
            group.capacity_hint = key.capacity_hint;
            group.capacity_hint_available = key.capacity_hint_available;
            group.safe_simd_level = capabilities.isa.common_simd_level;
        }

        group.physical_core_keys.push_back(core.identity.node_relative_key);
        group.logical_processor_ids.insert(
            group.logical_processor_ids.end(),
            core_allowed_ids.begin(), core_allowed_ids.end());
    }

    // In incomplete topology cases, retain usable logical processors rather
    // than dropping them. Each unmapped processor forms a conservative group.
    std::set<int> grouped_ids;
    for (const auto& [key, group] : groups) {
        static_cast<void>(key);
        grouped_ids.insert(
            group.logical_processor_ids.begin(),
            group.logical_processor_ids.end());
    }

    const auto lookup = logical_processor_lookup(topology);
    for (const int cpu_id : allowed_ids) {
        if (grouped_ids.count(cpu_id) != 0) {
            continue;
        }

        GroupKey key{};
        const auto iterator = lookup.find(cpu_id);
        if (iterator != lookup.end()) {
            key.numa_node_id = iterator->second->numa_node_id;
            key.core_type = iterator->second->core_type;
            key.capacity_hint = iterator->second->capacity_hint;
            key.capacity_hint_available =
                iterator->second->capacity_hint_available;
        }

        CpuExecutionGroup& group = groups[key];
        if (group.group_key.empty()) {
            group.group_key = execution_group_key(key);
            group.numa_node_id = key.numa_node_id;
            group.core_type = key.core_type;
            group.capacity_hint = key.capacity_hint;
            group.capacity_hint_available = key.capacity_hint_available;
            group.safe_simd_level = capabilities.isa.common_simd_level;
        }
        group.logical_processor_ids.push_back(cpu_id);
    }

    snapshot.execution_groups.reserve(groups.size());
    for (auto& [key, group] : groups) {
        static_cast<void>(key);
        std::sort(
            group.physical_core_keys.begin(),
            group.physical_core_keys.end());
        group.physical_core_keys.erase(
            std::unique(
                group.physical_core_keys.begin(),
                group.physical_core_keys.end()),
            group.physical_core_keys.end());
        std::sort(
            group.logical_processor_ids.begin(),
            group.logical_processor_ids.end());
        group.logical_processor_ids.erase(
            std::unique(
                group.logical_processor_ids.begin(),
                group.logical_processor_ids.end()),
            group.logical_processor_ids.end());
        snapshot.execution_groups.push_back(std::move(group));
    }

    snapshot.heterogeneous_execution_groups_present =
        snapshot.execution_groups.size() > 1;
}

struct CoreCandidate {
    std::string key{};
    int numa_node_id = -1;
    std::vector<int> allowed_logical_processor_ids{};
};

[[nodiscard]] std::vector<CoreCandidate> collect_core_candidates(
    const CpuTopologySnapshot& topology,
    const std::set<int>& allowed_ids) {
    std::vector<CoreCandidate> result;
    std::set<int> mapped_ids;

    for (const CpuCoreTopology& core : topology.cores) {
        CoreCandidate candidate{};
        candidate.key = core.identity.node_relative_key;
        candidate.numa_node_id = core.numa_node_id;
        for (const int cpu_id : core.identity.logical_processor_ids) {
            if (allowed_ids.count(cpu_id) != 0) {
                candidate.allowed_logical_processor_ids.push_back(cpu_id);
                mapped_ids.insert(cpu_id);
            }
        }
        if (!candidate.allowed_logical_processor_ids.empty()) {
            std::sort(
                candidate.allowed_logical_processor_ids.begin(),
                candidate.allowed_logical_processor_ids.end());
            result.push_back(std::move(candidate));
        }
    }

    const auto lookup = logical_processor_lookup(topology);
    for (const int cpu_id : allowed_ids) {
        if (mapped_ids.count(cpu_id) != 0) {
            continue;
        }
        CoreCandidate fallback{};
        fallback.key = "cpu:core:fallback:logical:" + std::to_string(cpu_id);
        const auto iterator = lookup.find(cpu_id);
        if (iterator != lookup.end()) {
            fallback.numa_node_id = iterator->second->numa_node_id;
        }
        fallback.allowed_logical_processor_ids.push_back(cpu_id);
        result.push_back(std::move(fallback));
    }

    std::sort(
        result.begin(), result.end(),
        [](const CoreCandidate& left, const CoreCandidate& right) {
            return left.key < right.key;
        });
    return result;
}

void reserve_core(
    const CoreCandidate& core,
    std::set<std::string>& reserved_core_keys,
    std::set<int>& reserved_cpu_ids) {
    reserved_core_keys.insert(core.key);
    reserved_cpu_ids.insert(
        core.allowed_logical_processor_ids.begin(),
        core.allowed_logical_processor_ids.end());
}

[[nodiscard]] std::vector<CpuExecutionGroup> filter_worker_groups(
    const std::vector<CpuExecutionGroup>& groups,
    const std::set<int>& worker_ids) {
    std::vector<CpuExecutionGroup> result;
    for (const CpuExecutionGroup& source : groups) {
        CpuExecutionGroup filtered = source;
        filtered.logical_processor_ids.clear();
        std::copy_if(
            source.logical_processor_ids.begin(),
            source.logical_processor_ids.end(),
            std::back_inserter(filtered.logical_processor_ids),
            [&worker_ids](int cpu_id) {
                return worker_ids.count(cpu_id) != 0;
            });
        if (!filtered.logical_processor_ids.empty()) {
            result.push_back(std::move(filtered));
        }
    }
    return result;
}

}  // namespace

CpuCapacityQueryResult query_cpu_capacity(
    const CpuTopologySnapshot& topology,
    const CpuCapabilitySnapshot& capabilities,
    const CpuHealthSnapshot& health) {
    CpuCapacityQueryResult result{};
    CpuCapacitySnapshot& snapshot = result.capacity;

    snapshot.configured_logical_processor_count =
        topology.summary.configured_logical_processor_count;
    snapshot.online_logical_processor_count =
        topology.summary.online_logical_processor_count;
    snapshot.physical_core_count = topology.summary.physical_core_count;
    snapshot.package_count = topology.summary.package_count;
    snapshot.numa_node_count = topology.summary.numa_node_count;
    snapshot.process_affinity_known = topology.summary.process_affinity_known;
    snapshot.simultaneous_multithreading_present =
        topology.summary.simultaneous_multithreading_present;
    snapshot.common_simd_level = capabilities.isa.common_simd_level;
    snapshot.common_vector_width_bytes =
        capabilities.isa.common_vector_width_bytes;

    snapshot.process_allowed_logical_processor_ids =
        allowed_processor_ids(topology);
    snapshot.process_allowed_logical_processor_count =
        snapshot.process_allowed_logical_processor_ids.size();

    if (snapshot.process_allowed_logical_processor_ids.empty()) {
        add_issue(
            snapshot,
            CpuCapacityIssueCode::no_allowed_processors,
            true,
            "no process-allowed or online logical processors are available");
        result.status.code = CpuCapacityQueryCode::query_failed;
        result.status.message = "CPU capacity has no usable processors";
        return result;
    }

    const std::set<int> allowed_ids(
        snapshot.process_allowed_logical_processor_ids.begin(),
        snapshot.process_allowed_logical_processor_ids.end());

    for (const CpuCoreTopology& core : topology.cores) {
        if (contains_id(allowed_ids, core.identity.logical_processor_ids)) {
            ++snapshot.process_allowed_physical_core_count;
        }
    }
    if (snapshot.process_allowed_physical_core_count == 0) {
        snapshot.process_allowed_physical_core_count = allowed_ids.size();
        add_issue(
            snapshot,
            CpuCapacityIssueCode::physical_core_mapping_incomplete,
            false,
            "physical core mapping was unavailable; logical processors were "
            "used as conservative core units");
    }

    bool all_numa_memory_available = !topology.numa_nodes.empty();
    std::uint64_t total_memory = 0;
    for (const CpuNumaNodeTopology& node : topology.numa_nodes) {
        if (!node.total_memory_available) {
            all_numa_memory_available = false;
            continue;
        }
        if (node.total_memory_bytes >
            std::numeric_limits<std::uint64_t>::max() - total_memory) {
            all_numa_memory_available = false;
            continue;
        }
        total_memory += node.total_memory_bytes;
    }

    if (health.memory.state == CpuTelemetryState::available) {
        snapshot.total_memory_state = CpuTelemetryState::available;
        snapshot.total_memory_bytes = health.memory.total_bytes;
        snapshot.available_memory_state = CpuTelemetryState::available;
        snapshot.available_memory_bytes = health.memory.available_bytes;
    } else {
        if (all_numa_memory_available) {
            snapshot.total_memory_state = CpuTelemetryState::available;
            snapshot.total_memory_bytes = total_memory;
        } else {
            snapshot.total_memory_state = CpuTelemetryState::unknown;
            add_issue(
                snapshot,
                CpuCapacityIssueCode::total_memory_unknown,
                false,
                "total CPU-visible memory could not be proven");
        }
        snapshot.available_memory_state = health.memory.state;
        add_issue(
            snapshot,
            CpuCapacityIssueCode::available_memory_unknown,
            false,
            "currently available memory could not be proven");
    }

    populate_execution_groups(
        snapshot, topology, capabilities, allowed_ids);
    if (snapshot.execution_groups.empty()) {
        add_issue(
            snapshot,
            CpuCapacityIssueCode::topology_unusable,
            true,
            "no CPU execution groups could be formed");
    }

    const bool fatal_issue = std::any_of(
        snapshot.issues.begin(), snapshot.issues.end(),
        [](const CpuCapacityIssue& issue) { return issue.fatal; });
    if (fatal_issue) {
        result.status.code = CpuCapacityQueryCode::query_failed;
        result.status.message = "CPU capacity discovery failed";
    } else if (!snapshot.issues.empty()) {
        result.status.code = CpuCapacityQueryCode::partial_success;
        result.status.message =
            "CPU capacity discovery completed with partial information";
    } else {
        result.status.code = CpuCapacityQueryCode::success;
    }
    return result;
}

CpuCapacityPlan plan_cpu_capacity(
    const CpuTopologySnapshot& topology,
    const CpuCapacitySnapshot& capacity,
    const CpuCapacityPolicy& policy) {
    CpuCapacityPlan plan{};
    plan.policy = policy;

    if (capacity.process_allowed_logical_processor_ids.empty()) {
        plan.status.code = CpuCapacityPlanCode::insufficient_capacity;
        plan.status.message = "no CPU processors are available for planning";
        return plan;
    }

    const std::set<int> allowed_ids(
        capacity.process_allowed_logical_processor_ids.begin(),
        capacity.process_allowed_logical_processor_ids.end());
    const std::vector<CoreCandidate> cores =
        collect_core_candidates(topology, allowed_ids);
    if (cores.empty()) {
        plan.status.code = CpuCapacityPlanCode::insufficient_capacity;
        plan.status.message = "no CPU core candidates are available";
        return plan;
    }

    std::set<std::string> reserved_core_keys;
    std::set<int> reserved_cpu_ids;

    if (policy.preserve_one_physical_core_per_numa_node) {
        std::set<int> reserved_numa_nodes;
        for (const CoreCandidate& core : cores) {
            if (reserved_numa_nodes.insert(core.numa_node_id).second) {
                reserve_core(core, reserved_core_keys, reserved_cpu_ids);
            }
        }
    }

    std::size_t explicit_core_reservations = 0;
    for (const CoreCandidate& core : cores) {
        if (explicit_core_reservations >= policy.reserved_physical_cores) {
            break;
        }
        if (reserved_core_keys.count(core.key) != 0) {
            continue;
        }
        reserve_core(core, reserved_core_keys, reserved_cpu_ids);
        ++explicit_core_reservations;
    }

    std::vector<int> candidate_worker_ids;
    for (const CoreCandidate& core : cores) {
        if (reserved_core_keys.count(core.key) != 0) {
            continue;
        }
        if (policy.count_smt_siblings_as_worker_lanes) {
            candidate_worker_ids.insert(
                candidate_worker_ids.end(),
                core.allowed_logical_processor_ids.begin(),
                core.allowed_logical_processor_ids.end());
        } else {
            candidate_worker_ids.push_back(
                core.allowed_logical_processor_ids.front());
        }
    }

    std::sort(candidate_worker_ids.begin(), candidate_worker_ids.end());
    candidate_worker_ids.erase(
        std::unique(candidate_worker_ids.begin(), candidate_worker_ids.end()),
        candidate_worker_ids.end());
    plan.candidate_worker_lane_count = candidate_worker_ids.size();

    const std::size_t logical_reservations = std::min(
        policy.reserved_logical_processors,
        candidate_worker_ids.size());
    for (std::size_t index = 0; index < logical_reservations; ++index) {
        reserved_cpu_ids.insert(candidate_worker_ids[index]);
    }
    candidate_worker_ids.erase(
        candidate_worker_ids.begin(),
        candidate_worker_ids.begin() +
            static_cast<std::ptrdiff_t>(logical_reservations));

    plan.reserved_physical_core_keys.assign(
        reserved_core_keys.begin(), reserved_core_keys.end());
    plan.reserved_logical_processor_ids.assign(
        reserved_cpu_ids.begin(), reserved_cpu_ids.end());
    plan.worker_logical_processor_ids = candidate_worker_ids;
    plan.reserved_physical_core_count = reserved_core_keys.size();
    plan.reserved_logical_processor_count = reserved_cpu_ids.size();
    plan.advisory_worker_lane_count = candidate_worker_ids.size();

    const std::set<int> worker_ids(
        candidate_worker_ids.begin(), candidate_worker_ids.end());
    plan.worker_execution_groups =
        filter_worker_groups(capacity.execution_groups, worker_ids);

    const bool physical_request_satisfied =
        explicit_core_reservations == policy.reserved_physical_cores;
    const bool logical_request_satisfied =
        logical_reservations == policy.reserved_logical_processors;
    const bool minimum_satisfied =
        plan.advisory_worker_lane_count >= policy.minimum_worker_lanes;

    if (!physical_request_satisfied || !logical_request_satisfied ||
        !minimum_satisfied) {
        plan.status.code = CpuCapacityPlanCode::insufficient_capacity;
        plan.status.message =
            "CPU capacity policy cannot be fully satisfied";
    } else {
        plan.status.code = CpuCapacityPlanCode::success;
        plan.status.message = "CPU capacity plan is advisory and usable";
    }
    return plan;
}

const char* to_string(CpuCapacityIssueCode value) noexcept {
    switch (value) {
        case CpuCapacityIssueCode::topology_unusable:
            return "topology_unusable";
        case CpuCapacityIssueCode::no_allowed_processors:
            return "no_allowed_processors";
        case CpuCapacityIssueCode::physical_core_mapping_incomplete:
            return "physical_core_mapping_incomplete";
        case CpuCapacityIssueCode::numa_mapping_incomplete:
            return "numa_mapping_incomplete";
        case CpuCapacityIssueCode::total_memory_unknown:
            return "total_memory_unknown";
        case CpuCapacityIssueCode::available_memory_unknown:
            return "available_memory_unknown";
    }
    return "unknown";
}

const char* to_string(CpuCapacityQueryCode value) noexcept {
    switch (value) {
        case CpuCapacityQueryCode::success:
            return "success";
        case CpuCapacityQueryCode::partial_success:
            return "partial_success";
        case CpuCapacityQueryCode::query_failed:
            return "query_failed";
    }
    return "unknown";
}

const char* to_string(CpuCapacityPlanCode value) noexcept {
    switch (value) {
        case CpuCapacityPlanCode::success:
            return "success";
        case CpuCapacityPlanCode::insufficient_capacity:
            return "insufficient_capacity";
        case CpuCapacityPlanCode::invalid_policy:
            return "invalid_policy";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cpu
