#include "cpu_identity.hpp"

#include <sstream>

namespace prometheus::backends::cpu {
namespace {

void append_component(
    std::ostringstream& stream,
    const char* label,
    int value) {
    stream << ':' << label << ':';
    if (value >= 0) {
        stream << value;
    } else {
        stream << "unknown";
    }
}

}  // namespace

std::string make_cpu_package_key(int package_id) {
    std::ostringstream stream;
    stream << "cpu";
    append_component(stream, "package", package_id);
    return stream.str();
}

std::string make_cpu_core_key(
    int package_id,
    int die_id,
    int core_id) {
    std::ostringstream stream;
    stream << make_cpu_package_key(package_id);
    append_component(stream, "die", die_id);
    append_component(stream, "core", core_id);
    return stream.str();
}

std::string make_cpu_thread_key(
    int package_id,
    int die_id,
    int core_id,
    int thread_index_within_core) {
    std::ostringstream stream;
    stream << make_cpu_core_key(package_id, die_id, core_id);
    append_component(stream, "thread", thread_index_within_core);
    return stream.str();
}

const char* to_string(CpuIdentityStability value) noexcept {
    switch (value) {
        case CpuIdentityStability::unknown:
            return "unknown";
        case CpuIdentityStability::runtime_only:
            return "runtime_only";
        case CpuIdentityStability::node_topology_stable:
            return "node_topology_stable";
        case CpuIdentityStability::firmware_backed:
            return "firmware_backed";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cpu
