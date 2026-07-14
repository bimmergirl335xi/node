#include "cpu_capabilities.hpp"
#include "cpu_identity.hpp"
#include "cpu_topology.hpp"

#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

namespace cpu = prometheus::backends::cpu;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "CPU foundation test failed: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    require(
        cpu::make_cpu_package_key(2) == "cpu:package:2",
        "package key must be deterministic");
    require(
        cpu::make_cpu_core_key(2, -1, 7) ==
            "cpu:package:2:die:unknown:core:7",
        "core key must retain unknown die state");
    require(
        cpu::make_cpu_thread_key(2, -1, 7, 1) ==
            "cpu:package:2:die:unknown:core:7:thread:1",
        "thread key must be deterministic");

    const cpu::CpuTopologyQueryResult topology_result =
        cpu::query_cpu_topology();
    require(topology_result.status.completed(),
            "topology discovery must complete on Linux");

    const cpu::CpuTopologySnapshot& topology = topology_result.topology;
    require(!topology.logical_processors.empty(),
            "at least one logical processor must be discovered");
    require(!topology.cores.empty(),
            "at least one physical-core record must be discovered");
    require(!topology.packages.empty(),
            "at least one package record must be discovered");
    require(!topology.numa_nodes.empty(),
            "at least one real or synthetic NUMA node must exist");

    std::set<int> logical_ids;
    std::set<std::string> logical_keys;
    for (const cpu::CpuLogicalProcessorTopology& processor :
         topology.logical_processors) {
        require(processor.identity.os_cpu_id >= 0,
                "logical processor ID must be non-negative");
        require(logical_ids.insert(processor.identity.os_cpu_id).second,
                "logical processor IDs must be unique");
        require(!processor.identity.node_relative_key.empty(),
                "logical processor key must not be empty");
        require(logical_keys.insert(
                    processor.identity.node_relative_key).second,
                "logical processor keys must be unique");
        require(!processor.thread_sibling_ids.empty(),
                "thread sibling set must not be empty");
        require(!processor.package_sibling_ids.empty(),
                "package sibling set must not be empty");
    }

    require(
        topology.summary.online_logical_processor_count ==
            topology.logical_processors.size(),
        "online summary must match discovered logical processors");
    require(
        topology.summary.physical_core_count == topology.cores.size(),
        "core summary must match core records");
    require(
        topology.summary.package_count == topology.packages.size(),
        "package summary must match package records");
    require(
        topology.summary.numa_node_count == topology.numa_nodes.size(),
        "NUMA summary must match NUMA records");

    if (topology.summary.process_affinity_known) {
        require(
            topology.summary.process_allowed_logical_processor_count > 0,
            "known affinity must allow at least one processor");
        require(
            topology.summary.process_allowed_logical_processor_count <=
                topology.summary.online_logical_processor_count,
            "allowed processor count cannot exceed online count");
    }

    const cpu::CpuCapabilityQueryResult capability_result =
        cpu::query_cpu_capabilities(topology);
    require(capability_result.status.completed(),
            "capability discovery must complete on Linux");

    const cpu::CpuCapabilitySnapshot& capabilities =
        capability_result.capabilities;
    require(capabilities.target_logical_processor_count > 0,
            "capability query must target processors");
    require(
        capabilities.target_logical_processor_count ==
            capabilities.target_logical_processor_ids.size(),
        "target processor count must match target IDs");
    require(capabilities.isa.pointer_width_bits == sizeof(void*) * 8,
            "pointer width must match the compiled process");
    require(
        capabilities.isa.lock_free_atomic_u64.common !=
            cpu::CpuSupportState::unknown,
        "compile-time uint64 atomic state must be explicit");

    return EXIT_SUCCESS;
}
