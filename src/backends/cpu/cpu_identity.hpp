#ifndef SRC_BACKENDS_CPU_CPU_IDENTITY_HPP
#define SRC_BACKENDS_CPU_CPU_IDENTITY_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace prometheus::backends::cpu {

// CPU identities are node-relative. A globally durable Prometheus identity is
// formed later by combining the node's stable identity with these topology
// keys. Linux logical processor numbers are runtime observations only.
enum class CpuIdentityStability : std::uint8_t {
    unknown = 0,
    runtime_only,
    node_topology_stable,
    firmware_backed,
};

struct CpuProcessorSignature {
    std::string vendor_id{};
    std::string model_name{};

    int family = -1;
    int model = -1;
    int stepping = -1;

    // ARM and other non-x86 fields are retained as strings because their
    // canonical formats differ between architectures and kernels.
    std::string implementer{};
    std::string architecture{};
    std::string part{};
    std::string revision{};
};

struct CpuLogicalProcessorIdentity {
    // Temporary Linux CPU number. Affinity, cpusets, hotplug, or firmware may
    // alter which numbers are visible between process launches.
    int os_cpu_id = -1;

    int package_id = -1;
    int die_id = -1;
    int core_id = -1;
    int thread_index_within_core = -1;

    std::string node_relative_key{};
    CpuIdentityStability stability = CpuIdentityStability::unknown;

    [[nodiscard]] bool topology_complete() const noexcept {
        return os_cpu_id >= 0 && package_id >= 0 && core_id >= 0 &&
               thread_index_within_core >= 0;
    }
};

struct CpuCoreIdentity {
    int package_id = -1;
    int die_id = -1;
    int core_id = -1;

    std::string node_relative_key{};
    CpuIdentityStability stability = CpuIdentityStability::unknown;

    std::vector<int> logical_processor_ids{};

    [[nodiscard]] bool topology_complete() const noexcept {
        return package_id >= 0 && core_id >= 0;
    }
};

struct CpuPackageIdentity {
    int package_id = -1;

    std::string node_relative_key{};
    CpuIdentityStability stability = CpuIdentityStability::unknown;
    CpuProcessorSignature signature{};

    std::vector<int> logical_processor_ids{};

    [[nodiscard]] bool topology_complete() const noexcept {
        return package_id >= 0;
    }
};

[[nodiscard]] std::string make_cpu_package_key(int package_id);

[[nodiscard]] std::string make_cpu_core_key(
    int package_id,
    int die_id,
    int core_id);

[[nodiscard]] std::string make_cpu_thread_key(
    int package_id,
    int die_id,
    int core_id,
    int thread_index_within_core);

[[nodiscard]] const char* to_string(CpuIdentityStability value) noexcept;

}  // namespace prometheus::backends::cpu

#endif  // SRC_BACKENDS_CPU_CPU_IDENTITY_HPP
