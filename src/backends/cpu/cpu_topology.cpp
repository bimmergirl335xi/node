#include "cpu_topology.hpp"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(__linux__)
#include <sched.h>
#include <unistd.h>
#endif

namespace prometheus::backends::cpu {
namespace {

namespace fs = std::filesystem;

[[nodiscard]] std::string trim_copy(std::string_view input) {
    std::size_t begin = 0;
    while (begin < input.size() &&
           std::isspace(static_cast<unsigned char>(input[begin])) != 0) {
        ++begin;
    }

    std::size_t end = input.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(input[end - 1])) != 0) {
        --end;
    }

    return std::string(input.substr(begin, end - begin));
}

[[nodiscard]] std::optional<std::string> read_text_file(
    const fs::path& path) {
    std::ifstream stream(path);
    if (!stream) {
        return std::nullopt;
    }

    std::ostringstream contents;
    contents << stream.rdbuf();
    if (!stream.good() && !stream.eof()) {
        return std::nullopt;
    }
    return trim_copy(contents.str());
}

[[nodiscard]] std::optional<int> parse_int(std::string_view input) {
    const std::string trimmed = trim_copy(input);
    if (trimmed.empty()) {
        return std::nullopt;
    }

    int value = 0;
    const char* begin = trimmed.data();
    const char* end = begin + trimmed.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] std::optional<std::uint64_t> parse_uint64(
    std::string_view input) {
    const std::string trimmed = trim_copy(input);
    if (trimmed.empty()) {
        return std::nullopt;
    }

    std::uint64_t value = 0;
    const char* begin = trimmed.data();
    const char* end = begin + trimmed.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] std::optional<std::vector<int>> parse_cpu_list(
    std::string_view input) {
    std::set<int> values;
    std::size_t cursor = 0;

    while (cursor < input.size()) {
        const std::size_t comma = input.find(',', cursor);
        const std::size_t end =
            comma == std::string_view::npos ? input.size() : comma;
        const std::string token = trim_copy(input.substr(cursor, end - cursor));
        if (token.empty()) {
            return std::nullopt;
        }

        const std::size_t dash = token.find('-');
        if (dash == std::string::npos) {
            const std::optional<int> value = parse_int(token);
            if (!value.has_value() || *value < 0) {
                return std::nullopt;
            }
            values.insert(*value);
        } else {
            const std::optional<int> first =
                parse_int(std::string_view(token).substr(0, dash));
            const std::optional<int> last =
                parse_int(std::string_view(token).substr(dash + 1));
            if (!first.has_value() || !last.has_value() ||
                *first < 0 || *last < *first) {
                return std::nullopt;
            }

            for (int value = *first; value <= *last; ++value) {
                values.insert(value);
                if (value == std::numeric_limits<int>::max()) {
                    break;
                }
            }
        }

        if (comma == std::string_view::npos) {
            break;
        }
        cursor = comma + 1;
    }

    return std::vector<int>(values.begin(), values.end());
}

[[nodiscard]] std::string canonical_cpu_list(
    const std::vector<int>& input) {
    if (input.empty()) {
        return "none";
    }

    std::vector<int> values = input;
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());

    std::ostringstream stream;
    for (std::size_t index = 0; index < values.size();) {
        const int first = values[index];
        int last = first;
        while (index + 1 < values.size() &&
               values[index + 1] == last + 1) {
            ++index;
            last = values[index];
        }

        if (stream.tellp() > 0) {
            stream << ',';
        }
        stream << first;
        if (last != first) {
            stream << '-' << last;
        }
        ++index;
    }
    return stream.str();
}

[[nodiscard]] std::optional<std::uint64_t> parse_size_bytes(
    std::string_view input) {
    const std::string trimmed = trim_copy(input);
    if (trimmed.empty()) {
        return std::nullopt;
    }

    std::size_t numeric_end = 0;
    while (numeric_end < trimmed.size() &&
           std::isdigit(static_cast<unsigned char>(trimmed[numeric_end])) != 0) {
        ++numeric_end;
    }
    if (numeric_end == 0) {
        return std::nullopt;
    }

    const std::optional<std::uint64_t> magnitude =
        parse_uint64(std::string_view(trimmed).substr(0, numeric_end));
    if (!magnitude.has_value()) {
        return std::nullopt;
    }

    std::uint64_t multiplier = 1;
    if (numeric_end < trimmed.size()) {
        const char suffix = static_cast<char>(
            std::toupper(static_cast<unsigned char>(trimmed[numeric_end])));
        if (suffix == 'K') {
            multiplier = 1024ULL;
        } else if (suffix == 'M') {
            multiplier = 1024ULL * 1024ULL;
        } else if (suffix == 'G') {
            multiplier = 1024ULL * 1024ULL * 1024ULL;
        } else if (suffix != 'B') {
            return std::nullopt;
        }
    }

    if (*magnitude >
        std::numeric_limits<std::uint64_t>::max() / multiplier) {
        return std::nullopt;
    }
    return *magnitude * multiplier;
}

void add_issue(
    CpuTopologySnapshot& topology,
    CpuTopologyIssueCode code,
    int os_cpu_id,
    bool fatal,
    std::string message) {
    CpuTopologyIssue issue{};
    issue.code = code;
    issue.os_cpu_id = os_cpu_id;
    issue.fatal = fatal;
    issue.message = std::move(message);
    topology.issues.push_back(std::move(issue));
}

[[nodiscard]] std::optional<int> read_int_file(const fs::path& path) {
    const std::optional<std::string> text = read_text_file(path);
    return text.has_value() ? parse_int(*text) : std::nullopt;
}

[[nodiscard]] std::optional<std::uint64_t> read_uint64_file(
    const fs::path& path) {
    const std::optional<std::string> text = read_text_file(path);
    return text.has_value() ? parse_uint64(*text) : std::nullopt;
}

[[nodiscard]] std::optional<std::vector<int>> read_cpu_list_file(
    const fs::path& path) {
    const std::optional<std::string> text = read_text_file(path);
    return text.has_value() ? parse_cpu_list(*text) : std::nullopt;
}

struct ProcCpuTopologyRecord {
    CpuProcessorSignature signature{};
    int package_id = -1;
    int core_id = -1;
};

[[nodiscard]] std::unordered_map<int, ProcCpuTopologyRecord>
read_processor_topology_records(CpuTopologySnapshot& topology) {
    std::unordered_map<int, ProcCpuTopologyRecord> records;
    std::ifstream stream("/proc/cpuinfo");
    if (!stream) {
        add_issue(
            topology,
            CpuTopologyIssueCode::proc_cpuinfo_unavailable,
            -1,
            false,
            "unable to read /proc/cpuinfo processor topology records");
        return records;
    }

    int current_processor = -1;
    ProcCpuTopologyRecord current{};

    const auto commit = [&]() {
        if (current_processor >= 0) {
            records[current_processor] = current;
        }
    };

    std::string line;
    while (std::getline(stream, line)) {
        if (trim_copy(line).empty()) {
            commit();
            current_processor = -1;
            current = {};
            continue;
        }

        const std::size_t separator = line.find(':');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = trim_copy(
            std::string_view(line).substr(0, separator));
        const std::string value = trim_copy(
            std::string_view(line).substr(separator + 1));

        if (key == "processor") {
            const std::optional<int> parsed = parse_int(value);
            if (parsed.has_value()) {
                current_processor = *parsed;
            }
        } else if (key == "physical id") {
            current.package_id = parse_int(value).value_or(-1);
        } else if (key == "core id") {
            current.core_id = parse_int(value).value_or(-1);
        } else if (key == "vendor_id") {
            current.signature.vendor_id = value;
        } else if (key == "model name" || key == "Processor" ||
                   key == "Hardware") {
            if (current.signature.model_name.empty()) {
                current.signature.model_name = value;
            }
        } else if (key == "cpu family") {
            current.signature.family = parse_int(value).value_or(-1);
        } else if (key == "model") {
            current.signature.model = parse_int(value).value_or(-1);
        } else if (key == "stepping") {
            current.signature.stepping = parse_int(value).value_or(-1);
        } else if (key == "CPU implementer") {
            current.signature.implementer = value;
        } else if (key == "CPU architecture") {
            current.signature.architecture = value;
        } else if (key == "CPU part") {
            current.signature.part = value;
        } else if (key == "CPU revision") {
            current.signature.revision = value;
        }
    }
    commit();
    return records;
}

[[nodiscard]] std::optional<std::set<int>> query_process_affinity() {
#if defined(__linux__)
    cpu_set_t affinity;
    CPU_ZERO(&affinity);
    if (sched_getaffinity(0, sizeof(affinity), &affinity) != 0) {
        return std::nullopt;
    }

    std::set<int> allowed;
    for (int cpu_id = 0; cpu_id < CPU_SETSIZE; ++cpu_id) {
        if (CPU_ISSET(cpu_id, &affinity) != 0) {
            allowed.insert(cpu_id);
        }
    }
    return allowed;
#else
    return std::nullopt;
#endif
}

[[nodiscard]] int find_thread_index(
    int os_cpu_id,
    const std::vector<int>& siblings) {
    const auto iterator =
        std::find(siblings.begin(), siblings.end(), os_cpu_id);
    if (iterator == siblings.end()) {
        return -1;
    }
    return static_cast<int>(std::distance(siblings.begin(), iterator));
}

[[nodiscard]] int parse_prefixed_numeric_name(
    std::string_view name,
    std::string_view prefix) {
    if (name.size() <= prefix.size() ||
        name.substr(0, prefix.size()) != prefix) {
        return -1;
    }
    return parse_int(name.substr(prefix.size())).value_or(-1);
}

[[nodiscard]] CpuCacheType parse_cache_type(std::string_view value) {
    const std::string trimmed = trim_copy(value);
    if (trimmed == "Data") {
        return CpuCacheType::data;
    }
    if (trimmed == "Instruction") {
        return CpuCacheType::instruction;
    }
    if (trimmed == "Unified") {
        return CpuCacheType::unified;
    }
    return CpuCacheType::unknown;
}

[[nodiscard]] std::uint64_t query_total_system_memory_bytes() {
#if defined(__linux__)
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long page_size = sysconf(_SC_PAGESIZE);
    if (pages <= 0 || page_size <= 0) {
        return 0;
    }

    const std::uint64_t unsigned_pages = static_cast<std::uint64_t>(pages);
    const std::uint64_t unsigned_page_size =
        static_cast<std::uint64_t>(page_size);
    if (unsigned_pages >
        std::numeric_limits<std::uint64_t>::max() / unsigned_page_size) {
        return 0;
    }
    return unsigned_pages * unsigned_page_size;
#else
    return 0;
#endif
}

void discover_numa_nodes(
    CpuTopologySnapshot& topology,
    const std::vector<int>& online_cpu_ids,
    std::unordered_map<int, int>& cpu_to_numa_node) {
    const fs::path node_root("/sys/devices/system/node");
    std::error_code error;
    bool found_real_node = false;

    if (fs::is_directory(node_root, error)) {
        for (const fs::directory_entry& entry :
             fs::directory_iterator(node_root, error)) {
            if (error) {
                break;
            }

            const int node_id = parse_prefixed_numeric_name(
                entry.path().filename().string(), "node");
            if (node_id < 0 || !entry.is_directory(error)) {
                continue;
            }

            const std::optional<std::vector<int>> cpus =
                read_cpu_list_file(entry.path() / "cpulist");
            if (!cpus.has_value()) {
                add_issue(
                    topology,
                    CpuTopologyIssueCode::numa_node_unavailable,
                    -1,
                    false,
                    "unable to read CPU membership for NUMA node " +
                        std::to_string(node_id));
                continue;
            }

            CpuNumaNodeTopology node{};
            node.numa_node_id = node_id;
            node.logical_processor_ids = *cpus;

            std::ifstream meminfo(entry.path() / "meminfo");
            std::string line;
            bool memory_found = false;
            while (std::getline(meminfo, line)) {
                const std::size_t marker = line.find("MemTotal:");
                if (marker == std::string::npos) {
                    continue;
                }

                std::istringstream parser(line.substr(marker + 9));
                std::uint64_t kib = 0;
                std::string unit;
                if (parser >> kib >> unit) {
                    if (kib <=
                        std::numeric_limits<std::uint64_t>::max() / 1024ULL) {
                        node.total_memory_bytes = kib * 1024ULL;
                        node.total_memory_available = true;
                        memory_found = true;
                    }
                }
                break;
            }

            if (!memory_found) {
                add_issue(
                    topology,
                    CpuTopologyIssueCode::numa_memory_unavailable,
                    -1,
                    false,
                    "unable to read total memory for NUMA node " +
                        std::to_string(node_id));
            }

            for (const int cpu_id : node.logical_processor_ids) {
                cpu_to_numa_node[cpu_id] = node_id;
            }

            topology.numa_nodes.push_back(std::move(node));
            found_real_node = true;
        }
    }

    if (found_real_node) {
        std::sort(
            topology.numa_nodes.begin(),
            topology.numa_nodes.end(),
            [](const CpuNumaNodeTopology& left,
               const CpuNumaNodeTopology& right) {
                return left.numa_node_id < right.numa_node_id;
            });
        return;
    }

    CpuNumaNodeTopology synthetic{};
    synthetic.numa_node_id = 0;
    synthetic.synthetic_single_node = true;
    synthetic.logical_processor_ids = online_cpu_ids;
    synthetic.total_memory_bytes = query_total_system_memory_bytes();
    synthetic.total_memory_available = synthetic.total_memory_bytes > 0;

    for (const int cpu_id : online_cpu_ids) {
        cpu_to_numa_node[cpu_id] = 0;
    }
    topology.numa_nodes.push_back(std::move(synthetic));
}

void discover_caches(
    CpuTopologySnapshot& topology,
    const std::vector<int>& online_cpu_ids) {
    std::map<std::string, CpuCacheInfo> unique_caches;

    for (const int cpu_id : online_cpu_ids) {
        const fs::path cache_root =
            fs::path("/sys/devices/system/cpu") /
            ("cpu" + std::to_string(cpu_id)) / "cache";

        std::error_code error;
        if (!fs::is_directory(cache_root, error)) {
            continue;
        }

        for (const fs::directory_entry& entry :
             fs::directory_iterator(cache_root, error)) {
            if (error) {
                add_issue(
                    topology,
                    CpuTopologyIssueCode::cache_query_failed,
                    cpu_id,
                    false,
                    "unable to enumerate cache descriptors");
                break;
            }

            if (parse_prefixed_numeric_name(
                    entry.path().filename().string(), "index") < 0) {
                continue;
            }

            const std::optional<int> level =
                read_int_file(entry.path() / "level");
            const std::optional<std::string> type_text =
                read_text_file(entry.path() / "type");
            const std::optional<std::string> size_text =
                read_text_file(entry.path() / "size");
            const std::optional<int> line_size =
                read_int_file(entry.path() / "coherency_line_size");
            const std::optional<std::vector<int>> shared =
                read_cpu_list_file(entry.path() / "shared_cpu_list");

            if (!level.has_value() || !type_text.has_value() ||
                !size_text.has_value() || !shared.has_value()) {
                add_issue(
                    topology,
                    CpuTopologyIssueCode::cache_query_failed,
                    cpu_id,
                    false,
                    "incomplete cache descriptor under " +
                        entry.path().string());
                continue;
            }

            const std::optional<std::uint64_t> size_bytes =
                parse_size_bytes(*size_text);
            if (!size_bytes.has_value()) {
                add_issue(
                    topology,
                    CpuTopologyIssueCode::malformed_topology_value,
                    cpu_id,
                    false,
                    "unable to parse cache size " + *size_text);
                continue;
            }

            CpuCacheInfo cache{};
            cache.level = *level;
            cache.type = parse_cache_type(*type_text);
            cache.size_bytes = *size_bytes;
            cache.coherency_line_size_bytes =
                line_size.has_value() && *line_size >= 0
                    ? static_cast<std::uint32_t>(*line_size)
                    : 0;
            cache.shared_logical_processor_ids = *shared;

            std::ostringstream key;
            key << "cpu:cache:level:" << cache.level
                << ":type:" << to_string(cache.type)
                << ":shared:"
                << canonical_cpu_list(cache.shared_logical_processor_ids);
            cache.node_relative_key = key.str();

            unique_caches.emplace(cache.node_relative_key, std::move(cache));
        }
    }

    topology.caches.reserve(unique_caches.size());
    for (auto& [key, cache] : unique_caches) {
        static_cast<void>(key);
        topology.caches.push_back(std::move(cache));
    }
}

[[nodiscard]] std::vector<int> fallback_online_cpu_ids() {
    std::vector<int> cpu_ids;
#if defined(__linux__)
    const long count = sysconf(_SC_NPROCESSORS_ONLN);
    if (count > 0 && count <= std::numeric_limits<int>::max()) {
        cpu_ids.reserve(static_cast<std::size_t>(count));
        for (int cpu_id = 0; cpu_id < static_cast<int>(count); ++cpu_id) {
            cpu_ids.push_back(cpu_id);
        }
    }
#endif
    return cpu_ids;
}

[[nodiscard]] std::size_t query_configured_processor_count(
    std::size_t fallback) {
#if defined(__linux__)
    const long count = sysconf(_SC_NPROCESSORS_CONF);
    if (count > 0) {
        return static_cast<std::size_t>(count);
    }
#endif
    return fallback;
}

}  // namespace

CpuTopologyQueryResult query_cpu_topology() {
    CpuTopologyQueryResult result{};

#if !defined(__linux__)
    result.status.code = CpuTopologyQueryCode::unsupported_platform;
    result.status.message =
        "Phase 7.1 CPU topology discovery currently supports Linux only";
    return result;
#else
    CpuTopologySnapshot& topology = result.topology;

    std::vector<int> online_cpu_ids;
    const std::optional<std::vector<int>> online_from_sysfs =
        read_cpu_list_file("/sys/devices/system/cpu/online");
    if (online_from_sysfs.has_value()) {
        online_cpu_ids = *online_from_sysfs;
    } else {
        online_cpu_ids = fallback_online_cpu_ids();
        add_issue(
            topology,
            CpuTopologyIssueCode::online_cpu_list_unavailable,
            -1,
            online_cpu_ids.empty(),
            "unable to read /sys/devices/system/cpu/online; used sysconf fallback");
    }

    if (online_cpu_ids.empty()) {
        add_issue(
            topology,
            CpuTopologyIssueCode::no_processors_discovered,
            -1,
            true,
            "no online logical processors were discovered");
        result.status.code = CpuTopologyQueryCode::query_failed;
        result.status.message = "CPU topology discovery found no processors";
        return result;
    }

    topology.summary.configured_logical_processor_count =
        query_configured_processor_count(online_cpu_ids.size());
    topology.summary.online_logical_processor_count = online_cpu_ids.size();

    const std::optional<std::set<int>> allowed_cpu_ids =
        query_process_affinity();
    topology.summary.process_affinity_known = allowed_cpu_ids.has_value();
    if (!allowed_cpu_ids.has_value()) {
        add_issue(
            topology,
            CpuTopologyIssueCode::process_affinity_unavailable,
            -1,
            false,
            "sched_getaffinity failed; process-allowed CPUs remain unknown");
    }

    std::unordered_map<int, int> cpu_to_numa_node;
    discover_numa_nodes(topology, online_cpu_ids, cpu_to_numa_node);

    const std::unordered_map<int, ProcCpuTopologyRecord> proc_records =
        read_processor_topology_records(topology);

    std::set<int> observed_cpu_ids;
    std::set<int> core_types;
    std::set<std::uint64_t> capacity_hints;

    topology.logical_processors.reserve(online_cpu_ids.size());
    for (const int cpu_id : online_cpu_ids) {
        if (!observed_cpu_ids.insert(cpu_id).second) {
            add_issue(
                topology,
                CpuTopologyIssueCode::duplicate_logical_processor,
                cpu_id,
                true,
                "logical processor appeared more than once in online set");
            continue;
        }

        const fs::path cpu_root =
            fs::path("/sys/devices/system/cpu") /
            ("cpu" + std::to_string(cpu_id));
        const fs::path topology_root = cpu_root / "topology";

        CpuLogicalProcessorTopology processor{};
        processor.identity.os_cpu_id = cpu_id;
        processor.online = CpuMembershipState::yes;
        if (allowed_cpu_ids.has_value()) {
            processor.process_allowed =
                allowed_cpu_ids->count(cpu_id) != 0
                    ? CpuMembershipState::yes
                    : CpuMembershipState::no;
        }

        const auto proc_record_iterator = proc_records.find(cpu_id);

        const std::optional<int> package_id =
            read_int_file(topology_root / "physical_package_id");
        if (package_id.has_value()) {
            processor.identity.package_id = *package_id;
        } else if (proc_record_iterator != proc_records.end() &&
                   proc_record_iterator->second.package_id >= 0) {
            processor.identity.package_id =
                proc_record_iterator->second.package_id;
        } else {
            add_issue(
                topology,
                CpuTopologyIssueCode::package_id_unavailable,
                cpu_id,
                false,
                "physical package ID is unavailable from sysfs and /proc/cpuinfo");
        }

        const std::optional<int> die_id =
            read_int_file(topology_root / "die_id");
        if (die_id.has_value()) {
            processor.identity.die_id = *die_id;
        }

        const std::optional<int> core_id =
            read_int_file(topology_root / "core_id");
        if (core_id.has_value()) {
            processor.identity.core_id = *core_id;
        } else if (proc_record_iterator != proc_records.end() &&
                   proc_record_iterator->second.core_id >= 0) {
            processor.identity.core_id =
                proc_record_iterator->second.core_id;
        } else {
            add_issue(
                topology,
                CpuTopologyIssueCode::core_id_unavailable,
                cpu_id,
                false,
                "physical core ID is unavailable from sysfs and /proc/cpuinfo");
        }

        const std::optional<std::vector<int>> thread_siblings =
            read_cpu_list_file(topology_root / "thread_siblings_list");
        if (thread_siblings.has_value()) {
            processor.thread_sibling_ids = *thread_siblings;
        }

        const std::optional<std::vector<int>> package_siblings =
            read_cpu_list_file(topology_root / "core_siblings_list");
        if (package_siblings.has_value()) {
            processor.package_sibling_ids = *package_siblings;
        }

        const auto numa_iterator = cpu_to_numa_node.find(cpu_id);
        if (numa_iterator != cpu_to_numa_node.end()) {
            processor.numa_node_id = numa_iterator->second;
        } else {
            add_issue(
                topology,
                CpuTopologyIssueCode::numa_node_unavailable,
                cpu_id,
                false,
                "NUMA membership is unavailable for logical processor");
        }

        const std::optional<int> core_type =
            read_int_file(topology_root / "core_type");
        if (core_type.has_value()) {
            processor.core_type = *core_type;
            core_types.insert(*core_type);
        }

        std::optional<std::uint64_t> capacity =
            read_uint64_file(cpu_root / "cpu_capacity");
        if (!capacity.has_value()) {
            capacity = read_uint64_file(topology_root / "cpu_capacity");
        }
        if (capacity.has_value()) {
            processor.capacity_hint = *capacity;
            processor.capacity_hint_available = true;
            capacity_hints.insert(*capacity);
        }

        if (processor.process_allowed == CpuMembershipState::yes) {
            ++topology.summary.process_allowed_logical_processor_count;
        }

        topology.logical_processors.push_back(std::move(processor));
    }

    std::map<std::pair<int, int>, std::vector<int>> derived_core_siblings;
    std::map<int, std::vector<int>> derived_package_siblings;

    for (const CpuLogicalProcessorTopology& processor :
         topology.logical_processors) {
        if (processor.identity.package_id >= 0) {
            derived_package_siblings[processor.identity.package_id].push_back(
                processor.identity.os_cpu_id);
        }
        if (processor.identity.package_id >= 0 &&
            processor.identity.core_id >= 0) {
            derived_core_siblings[
                {processor.identity.package_id, processor.identity.core_id}]
                .push_back(processor.identity.os_cpu_id);
        }
    }

    for (auto& [key, ids] : derived_core_siblings) {
        static_cast<void>(key);
        std::sort(ids.begin(), ids.end());
        ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    }
    for (auto& [key, ids] : derived_package_siblings) {
        static_cast<void>(key);
        std::sort(ids.begin(), ids.end());
        ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    }

    for (CpuLogicalProcessorTopology& processor :
         topology.logical_processors) {
        if (processor.thread_sibling_ids.empty() &&
            processor.identity.package_id >= 0 &&
            processor.identity.core_id >= 0) {
            const auto iterator = derived_core_siblings.find(
                {processor.identity.package_id, processor.identity.core_id});
            if (iterator != derived_core_siblings.end()) {
                processor.thread_sibling_ids = iterator->second;
            }
        }
        if (processor.thread_sibling_ids.empty()) {
            processor.thread_sibling_ids = {processor.identity.os_cpu_id};
            add_issue(
                topology,
                CpuTopologyIssueCode::sibling_list_unavailable,
                processor.identity.os_cpu_id,
                false,
                "thread sibling relationship is unavailable");
        }

        if (processor.package_sibling_ids.empty() &&
            processor.identity.package_id >= 0) {
            const auto iterator = derived_package_siblings.find(
                processor.identity.package_id);
            if (iterator != derived_package_siblings.end()) {
                processor.package_sibling_ids = iterator->second;
            }
        }
        if (processor.package_sibling_ids.empty()) {
            processor.package_sibling_ids = {processor.identity.os_cpu_id};
            add_issue(
                topology,
                CpuTopologyIssueCode::sibling_list_unavailable,
                processor.identity.os_cpu_id,
                false,
                "package sibling relationship is unavailable");
        }

        processor.identity.thread_index_within_core = find_thread_index(
            processor.identity.os_cpu_id,
            processor.thread_sibling_ids);
        if (processor.thread_sibling_ids.size() > 1) {
            topology.summary.simultaneous_multithreading_present = true;
        }

        if (processor.identity.topology_complete()) {
            processor.identity.node_relative_key = make_cpu_thread_key(
                processor.identity.package_id,
                processor.identity.die_id,
                processor.identity.core_id,
                processor.identity.thread_index_within_core);
            processor.identity.stability =
                CpuIdentityStability::node_topology_stable;
        } else {
            processor.identity.node_relative_key =
                "cpu:logical:" +
                std::to_string(processor.identity.os_cpu_id);
            processor.identity.stability = CpuIdentityStability::runtime_only;
        }
    }

    topology.summary.heterogeneous_core_types_present = core_types.size() > 1;
    topology.summary.heterogeneous_capacity_hints_present =
        capacity_hints.size() > 1;

    struct CoreAccumulator {
        CpuCoreTopology core{};
    };
    std::map<std::string, CoreAccumulator> core_map;

    struct PackageAccumulator {
        CpuPackageTopology package{};
    };
    std::map<std::string, PackageAccumulator> package_map;

    for (const CpuLogicalProcessorTopology& processor :
         topology.logical_processors) {
        std::string package_key;
        if (processor.identity.package_id >= 0) {
            package_key = make_cpu_package_key(processor.identity.package_id);
        } else {
            package_key = "cpu:package:unknown:logical:" +
                          std::to_string(processor.identity.os_cpu_id);
        }

        PackageAccumulator& package_accumulator = package_map[package_key];
        CpuPackageIdentity& package_identity =
            package_accumulator.package.identity;
        if (package_identity.node_relative_key.empty()) {
            package_identity.package_id = processor.identity.package_id;
            package_identity.node_relative_key = package_key;
            package_identity.stability =
                processor.identity.package_id >= 0
                    ? CpuIdentityStability::node_topology_stable
                    : CpuIdentityStability::runtime_only;

            const auto proc_record = proc_records.find(
                processor.identity.os_cpu_id);
            if (proc_record != proc_records.end()) {
                package_identity.signature = proc_record->second.signature;
            }
        }
        package_identity.logical_processor_ids.push_back(
            processor.identity.os_cpu_id);
        if (processor.numa_node_id >= 0) {
            package_accumulator.package.numa_node_ids.push_back(
                processor.numa_node_id);
        }

        std::string core_key;
        if (processor.identity.package_id >= 0 &&
            processor.identity.core_id >= 0) {
            core_key = make_cpu_core_key(
                processor.identity.package_id,
                processor.identity.die_id,
                processor.identity.core_id);
        } else {
            core_key = "cpu:core:unknown:logical:" +
                       std::to_string(processor.identity.os_cpu_id);
        }

        CoreAccumulator& core_accumulator = core_map[core_key];
        CpuCoreTopology& core = core_accumulator.core;
        if (core.identity.node_relative_key.empty()) {
            core.identity.package_id = processor.identity.package_id;
            core.identity.die_id = processor.identity.die_id;
            core.identity.core_id = processor.identity.core_id;
            core.identity.node_relative_key = core_key;
            core.identity.stability =
                processor.identity.package_id >= 0 &&
                        processor.identity.core_id >= 0
                    ? CpuIdentityStability::node_topology_stable
                    : CpuIdentityStability::runtime_only;
            core.numa_node_id = processor.numa_node_id;
            core.core_type = processor.core_type;
            core.capacity_hint = processor.capacity_hint;
            core.capacity_hint_available = processor.capacity_hint_available;
        }
        core.identity.logical_processor_ids.push_back(
            processor.identity.os_cpu_id);
    }

    topology.cores.reserve(core_map.size());
    for (auto& [core_key, accumulator] : core_map) {
        static_cast<void>(core_key);
        std::sort(
            accumulator.core.identity.logical_processor_ids.begin(),
            accumulator.core.identity.logical_processor_ids.end());
        topology.cores.push_back(std::move(accumulator.core));
    }

    for (CpuCoreTopology& core : topology.cores) {
        const std::string package_key =
            core.identity.package_id >= 0
                ? make_cpu_package_key(core.identity.package_id)
                : "cpu:package:unknown:logical:" +
                      std::to_string(
                          core.identity.logical_processor_ids.empty()
                              ? -1
                              : core.identity.logical_processor_ids.front());
        const auto package_iterator = package_map.find(package_key);
        if (package_iterator != package_map.end()) {
            package_iterator->second.package.cores.push_back(core.identity);
        }
    }

    topology.packages.reserve(package_map.size());
    for (auto& [package_key, accumulator] : package_map) {
        static_cast<void>(package_key);
        auto& processor_ids =
            accumulator.package.identity.logical_processor_ids;
        std::sort(processor_ids.begin(), processor_ids.end());

        auto& numa_ids = accumulator.package.numa_node_ids;
        std::sort(numa_ids.begin(), numa_ids.end());
        numa_ids.erase(std::unique(numa_ids.begin(), numa_ids.end()),
                       numa_ids.end());

        std::sort(
            accumulator.package.cores.begin(),
            accumulator.package.cores.end(),
            [](const CpuCoreIdentity& left, const CpuCoreIdentity& right) {
                return left.node_relative_key < right.node_relative_key;
            });

        topology.packages.push_back(std::move(accumulator.package));
    }

    discover_caches(topology, online_cpu_ids);

    topology.summary.package_count = topology.packages.size();
    topology.summary.physical_core_count = topology.cores.size();
    topology.summary.numa_node_count = topology.numa_nodes.size();
    topology.summary.cache_count = topology.caches.size();

    const bool fatal_issue = std::any_of(
        topology.issues.begin(),
        topology.issues.end(),
        [](const CpuTopologyIssue& issue) { return issue.fatal; });

    if (fatal_issue) {
        result.status.code = CpuTopologyQueryCode::query_failed;
        result.status.message =
            "CPU topology discovery completed with a fatal issue";
    } else if (!topology.issues.empty()) {
        result.status.code = CpuTopologyQueryCode::partial_success;
        result.status.message =
            "CPU topology discovery completed with partial information";
    } else {
        result.status.code = CpuTopologyQueryCode::success;
    }

    return result;
#endif
}

const char* to_string(CpuMembershipState value) noexcept {
    switch (value) {
        case CpuMembershipState::unknown:
            return "unknown";
        case CpuMembershipState::no:
            return "no";
        case CpuMembershipState::yes:
            return "yes";
    }
    return "unknown";
}

const char* to_string(CpuCacheType value) noexcept {
    switch (value) {
        case CpuCacheType::unknown:
            return "unknown";
        case CpuCacheType::data:
            return "data";
        case CpuCacheType::instruction:
            return "instruction";
        case CpuCacheType::unified:
            return "unified";
    }
    return "unknown";
}

const char* to_string(CpuTopologyIssueCode value) noexcept {
    switch (value) {
        case CpuTopologyIssueCode::online_cpu_list_unavailable:
            return "online_cpu_list_unavailable";
        case CpuTopologyIssueCode::process_affinity_unavailable:
            return "process_affinity_unavailable";
        case CpuTopologyIssueCode::package_id_unavailable:
            return "package_id_unavailable";
        case CpuTopologyIssueCode::die_id_unavailable:
            return "die_id_unavailable";
        case CpuTopologyIssueCode::core_id_unavailable:
            return "core_id_unavailable";
        case CpuTopologyIssueCode::sibling_list_unavailable:
            return "sibling_list_unavailable";
        case CpuTopologyIssueCode::numa_node_unavailable:
            return "numa_node_unavailable";
        case CpuTopologyIssueCode::numa_memory_unavailable:
            return "numa_memory_unavailable";
        case CpuTopologyIssueCode::cache_query_failed:
            return "cache_query_failed";
        case CpuTopologyIssueCode::proc_cpuinfo_unavailable:
            return "proc_cpuinfo_unavailable";
        case CpuTopologyIssueCode::duplicate_logical_processor:
            return "duplicate_logical_processor";
        case CpuTopologyIssueCode::malformed_topology_value:
            return "malformed_topology_value";
        case CpuTopologyIssueCode::no_processors_discovered:
            return "no_processors_discovered";
    }
    return "unknown";
}

const char* to_string(CpuTopologyQueryCode value) noexcept {
    switch (value) {
        case CpuTopologyQueryCode::success:
            return "success";
        case CpuTopologyQueryCode::partial_success:
            return "partial_success";
        case CpuTopologyQueryCode::unsupported_platform:
            return "unsupported_platform";
        case CpuTopologyQueryCode::query_failed:
            return "query_failed";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cpu
