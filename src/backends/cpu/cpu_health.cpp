#include "cpu_health.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

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

[[nodiscard]] std::string lower_copy(std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return value;
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

[[nodiscard]] std::optional<std::int64_t> parse_int64(
    std::string_view input) {
    const std::string trimmed = trim_copy(input);
    if (trimmed.empty()) {
        return std::nullopt;
    }

    std::int64_t value = 0;
    const char* begin = trimmed.data();
    const char* end = begin + trimmed.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] std::optional<std::uint64_t> read_uint64_file(
    const fs::path& path) {
    const std::optional<std::string> text = read_text_file(path);
    return text.has_value() ? parse_uint64(*text) : std::nullopt;
}

void add_issue(
    CpuHealthSnapshot& snapshot,
    CpuHealthIssueCode code,
    int os_cpu_id,
    bool fatal,
    std::string message) {
    snapshot.issues.push_back(
        CpuHealthIssue{code, os_cpu_id, fatal, std::move(message)});
}

[[nodiscard]] std::uint64_t unix_time_nanoseconds() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

[[nodiscard]] std::uint64_t monotonic_time_nanoseconds() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

struct ProcStatCounters {
    std::uint64_t total_ticks = 0;
    std::uint64_t idle_ticks = 0;
};

[[nodiscard]] std::optional<std::map<int, ProcStatCounters>> read_proc_stat(
    ProcStatCounters& aggregate) {
    std::ifstream stream("/proc/stat");
    if (!stream) {
        return std::nullopt;
    }

    std::map<int, ProcStatCounters> per_cpu;
    bool aggregate_found = false;
    std::string line;
    while (std::getline(stream, line)) {
        if (line.rfind("cpu", 0) != 0) {
            break;
        }

        std::istringstream parser(line);
        std::string label;
        parser >> label;

        std::uint64_t user = 0;
        std::uint64_t nice = 0;
        std::uint64_t system = 0;
        std::uint64_t idle = 0;
        std::uint64_t iowait = 0;
        std::uint64_t irq = 0;
        std::uint64_t softirq = 0;
        std::uint64_t steal = 0;
        if (!(parser >> user >> nice >> system >> idle >> iowait >> irq >>
              softirq >> steal)) {
            return std::nullopt;
        }

        ProcStatCounters counters{};
        counters.total_ticks =
            user + nice + system + idle + iowait + irq + softirq + steal;
        counters.idle_ticks = idle + iowait;

        if (label == "cpu") {
            aggregate = counters;
            aggregate_found = true;
            continue;
        }

        const std::string_view id_text(label.data() + 3, label.size() - 3);
        int cpu_id = -1;
        const auto parse_result = std::from_chars(
            id_text.data(), id_text.data() + id_text.size(), cpu_id);
        if (parse_result.ec == std::errc{} &&
            parse_result.ptr == id_text.data() + id_text.size() &&
            cpu_id >= 0) {
            per_cpu[cpu_id] = counters;
        }
    }

    return aggregate_found
               ? std::optional<std::map<int, ProcStatCounters>>(
                     std::move(per_cpu))
               : std::nullopt;
}

[[nodiscard]] const CpuUtilizationReading* find_previous_utilization(
    const CpuHealthSnapshot* previous,
    int os_cpu_id) {
    if (previous == nullptr) {
        return nullptr;
    }
    if (os_cpu_id < 0) {
        return &previous->aggregate_utilization;
    }

    const auto iterator = std::find_if(
        previous->logical_processor_utilization.begin(),
        previous->logical_processor_utilization.end(),
        [os_cpu_id](const CpuUtilizationReading& reading) {
            return reading.os_cpu_id == os_cpu_id;
        });
    return iterator == previous->logical_processor_utilization.end()
               ? nullptr
               : &*iterator;
}

[[nodiscard]] CpuUtilizationReading make_utilization_reading(
    int os_cpu_id,
    const ProcStatCounters& current,
    const CpuHealthSnapshot* previous) {
    CpuUtilizationReading reading{};
    reading.os_cpu_id = os_cpu_id;
    reading.counters.total_ticks = current.total_ticks;
    reading.counters.idle_ticks = current.idle_ticks;
    reading.counters.available = true;
    reading.state = CpuTelemetryState::baseline_only;

    const CpuUtilizationReading* prior =
        find_previous_utilization(previous, os_cpu_id);
    if (prior == nullptr || !prior->counters.available ||
        current.total_ticks <= prior->counters.total_ticks ||
        current.idle_ticks < prior->counters.idle_ticks) {
        return reading;
    }

    const std::uint64_t total_delta =
        current.total_ticks - prior->counters.total_ticks;
    const std::uint64_t idle_delta =
        current.idle_ticks - prior->counters.idle_ticks;
    if (total_delta == 0 || idle_delta > total_delta) {
        return reading;
    }

    reading.busy_fraction = static_cast<double>(total_delta - idle_delta) /
                            static_cast<double>(total_delta);
    reading.state = CpuTelemetryState::available;
    return reading;
}

void sample_utilization(
    CpuHealthSnapshot& snapshot,
    const CpuTopologySnapshot& topology,
    const CpuHealthSnapshot* previous) {
    ProcStatCounters aggregate{};
    const auto counters = read_proc_stat(aggregate);
    if (!counters.has_value()) {
        snapshot.aggregate_utilization.state =
            CpuTelemetryState::provider_unavailable;
        add_issue(
            snapshot,
            CpuHealthIssueCode::proc_stat_unavailable,
            -1,
            false,
            "unable to read CPU counters from /proc/stat");
        return;
    }

    snapshot.aggregate_utilization =
        make_utilization_reading(-1, aggregate, previous);

    snapshot.logical_processor_utilization.reserve(
        topology.logical_processors.size());
    for (const CpuLogicalProcessorTopology& processor :
         topology.logical_processors) {
        const int cpu_id = processor.identity.os_cpu_id;
        const auto iterator = counters->find(cpu_id);
        if (iterator == counters->end()) {
            CpuUtilizationReading missing{};
            missing.os_cpu_id = cpu_id;
            missing.state = CpuTelemetryState::query_failed;
            snapshot.logical_processor_utilization.push_back(missing);
            add_issue(
                snapshot,
                CpuHealthIssueCode::proc_stat_malformed,
                cpu_id,
                false,
                "logical processor is absent from /proc/stat");
            continue;
        }

        snapshot.logical_processor_utilization.push_back(
            make_utilization_reading(cpu_id, iterator->second, previous));
    }
}

[[nodiscard]] CpuFrequencyReading sample_frequency(int cpu_id) {
    CpuFrequencyReading reading{};
    reading.os_cpu_id = cpu_id;

    const fs::path root =
        fs::path("/sys/devices/system/cpu") /
        ("cpu" + std::to_string(cpu_id)) / "cpufreq";

    std::optional<std::uint64_t> current =
        read_uint64_file(root / "scaling_cur_freq");
    if (!current.has_value()) {
        current = read_uint64_file(root / "cpuinfo_cur_freq");
    }
    std::optional<std::uint64_t> maximum =
        read_uint64_file(root / "scaling_max_freq");
    if (!maximum.has_value()) {
        maximum = read_uint64_file(root / "cpuinfo_max_freq");
    }

    if (current.has_value()) {
        reading.current_state = CpuTelemetryState::available;
        reading.current_khz = *current;
    } else {
        reading.current_state = CpuTelemetryState::provider_unavailable;
    }

    if (maximum.has_value()) {
        reading.maximum_state = CpuTelemetryState::available;
        reading.maximum_khz = *maximum;
    } else {
        reading.maximum_state = CpuTelemetryState::provider_unavailable;
    }

    return reading;
}

void sample_frequencies(
    CpuHealthSnapshot& snapshot,
    const CpuTopologySnapshot& topology) {
    snapshot.frequencies.reserve(topology.logical_processors.size());
    for (const CpuLogicalProcessorTopology& processor :
         topology.logical_processors) {
        snapshot.frequencies.push_back(
            sample_frequency(processor.identity.os_cpu_id));
    }
}

void sample_load_average(CpuHealthSnapshot& snapshot) {
    std::ifstream stream("/proc/loadavg");
    if (!stream) {
        snapshot.load_average.state =
            CpuTelemetryState::provider_unavailable;
        add_issue(
            snapshot,
            CpuHealthIssueCode::load_average_unavailable,
            -1,
            false,
            "unable to read /proc/loadavg");
        return;
    }

    std::string entities;
    if (!(stream >> snapshot.load_average.one_minute >>
          snapshot.load_average.five_minutes >>
          snapshot.load_average.fifteen_minutes >> entities >>
          snapshot.load_average.latest_pid)) {
        snapshot.load_average.state = CpuTelemetryState::query_failed;
        add_issue(
            snapshot,
            CpuHealthIssueCode::load_average_unavailable,
            -1,
            false,
            "unable to parse /proc/loadavg");
        return;
    }

    const std::size_t slash = entities.find('/');
    if (slash == std::string::npos) {
        snapshot.load_average.state = CpuTelemetryState::query_failed;
        return;
    }

    const auto runnable = parse_uint64(entities.substr(0, slash));
    const auto total = parse_uint64(entities.substr(slash + 1));
    if (!runnable.has_value() || !total.has_value()) {
        snapshot.load_average.state = CpuTelemetryState::query_failed;
        return;
    }

    snapshot.load_average.runnable_entities = *runnable;
    snapshot.load_average.total_entities = *total;
    snapshot.load_average.state = CpuTelemetryState::available;
}

[[nodiscard]] std::optional<std::uint64_t> kib_to_bytes(
    std::uint64_t kib) {
    if (kib > std::numeric_limits<std::uint64_t>::max() / 1024ULL) {
        return std::nullopt;
    }
    return kib * 1024ULL;
}

void sample_memory(CpuHealthSnapshot& snapshot) {
    std::ifstream stream("/proc/meminfo");
    if (!stream) {
        snapshot.memory.state = CpuTelemetryState::provider_unavailable;
        add_issue(
            snapshot,
            CpuHealthIssueCode::memory_info_unavailable,
            -1,
            false,
            "unable to read /proc/meminfo");
        return;
    }

    std::map<std::string, std::uint64_t> values;
    std::string line;
    while (std::getline(stream, line)) {
        const std::size_t colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        const std::string key = trim_copy(
            std::string_view(line).substr(0, colon));
        std::istringstream parser(line.substr(colon + 1));
        std::uint64_t value = 0;
        if (parser >> value) {
            values[key] = value;
        }
    }

    const auto total = values.find("MemTotal");
    const auto available = values.find("MemAvailable");
    if (total == values.end() || available == values.end()) {
        snapshot.memory.state = CpuTelemetryState::query_failed;
        add_issue(
            snapshot,
            CpuHealthIssueCode::memory_info_unavailable,
            -1,
            false,
            "MemTotal or MemAvailable is missing from /proc/meminfo");
        return;
    }

    const auto total_bytes = kib_to_bytes(total->second);
    const auto available_bytes = kib_to_bytes(available->second);
    if (!total_bytes.has_value() || !available_bytes.has_value()) {
        snapshot.memory.state = CpuTelemetryState::query_failed;
        return;
    }

    snapshot.memory.total_bytes = *total_bytes;
    snapshot.memory.available_bytes = *available_bytes;

    const auto swap_total = values.find("SwapTotal");
    const auto swap_free = values.find("SwapFree");
    if (swap_total != values.end()) {
        snapshot.memory.swap_total_bytes =
            kib_to_bytes(swap_total->second).value_or(0);
    }
    if (swap_free != values.end()) {
        snapshot.memory.swap_free_bytes =
            kib_to_bytes(swap_free->second).value_or(0);
    }

    snapshot.memory.state = CpuTelemetryState::available;
}

[[nodiscard]] bool parse_pressure_line(
    std::string_view line,
    CpuPressureWindow& window) {
    std::istringstream parser{std::string(line)};
    std::string category;
    parser >> category;
    if (category != "some" && category != "full") {
        return false;
    }

    std::string token;
    bool avg10 = false;
    bool avg60 = false;
    bool avg300 = false;
    bool total = false;
    while (parser >> token) {
        const std::size_t equals = token.find('=');
        if (equals == std::string::npos) {
            continue;
        }
        const std::string name = token.substr(0, equals);
        const std::string value_text = token.substr(equals + 1);
        try {
            if (name == "avg10") {
                window.average_10_seconds = std::stod(value_text);
                avg10 = true;
            } else if (name == "avg60") {
                window.average_60_seconds = std::stod(value_text);
                avg60 = true;
            } else if (name == "avg300") {
                window.average_300_seconds = std::stod(value_text);
                avg300 = true;
            } else if (name == "total") {
                const auto parsed = parse_uint64(value_text);
                if (parsed.has_value()) {
                    window.total_stall_microseconds = *parsed;
                    total = true;
                }
            }
        } catch (...) {
            return false;
        }
    }

    window.available = avg10 && avg60 && avg300 && total;
    return window.available;
}

[[nodiscard]] CpuPressureReading sample_pressure(
    const fs::path& path,
    CpuHealthSnapshot& snapshot) {
    CpuPressureReading reading{};
    std::ifstream stream(path);
    if (!stream) {
        reading.state = CpuTelemetryState::provider_unavailable;
        return reading;
    }

    bool parsed_any = false;
    std::string line;
    while (std::getline(stream, line)) {
        if (line.rfind("some", 0) == 0) {
            parsed_any = parse_pressure_line(line, reading.some) || parsed_any;
        } else if (line.rfind("full", 0) == 0) {
            parsed_any = parse_pressure_line(line, reading.full) || parsed_any;
        }
    }

    if (!parsed_any) {
        reading.state = CpuTelemetryState::query_failed;
        add_issue(
            snapshot,
            CpuHealthIssueCode::pressure_query_failed,
            -1,
            false,
            "unable to parse " + path.string());
    } else {
        reading.state = CpuTelemetryState::available;
    }
    return reading;
}

[[nodiscard]] bool is_cpu_temperature_provider(std::string_view name) {
    const std::string lowered = lower_copy(std::string(name));
    return lowered.find("cpu") != std::string::npos ||
           lowered.find("package") != std::string::npos ||
           lowered.find("coretemp") != std::string::npos ||
           lowered.find("k10temp") != std::string::npos ||
           lowered.find("zenpower") != std::string::npos ||
           lowered.find("x86_pkg") != std::string::npos;
}

[[nodiscard]] CpuTemperatureAssociation temperature_association(
    std::string_view label) {
    const std::string lowered = lower_copy(std::string(label));
    if (lowered.find("package") != std::string::npos ||
        lowered.find("x86_pkg") != std::string::npos) {
        return CpuTemperatureAssociation::package;
    }
    if (is_cpu_temperature_provider(lowered)) {
        return CpuTemperatureAssociation::system_cpu;
    }
    return CpuTemperatureAssociation::unknown;
}

void sample_thermal_zones(CpuHealthSnapshot& snapshot) {
    const fs::path root("/sys/class/thermal");
    std::error_code error;
    if (!fs::is_directory(root, error)) {
        return;
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(root, error)) {
        if (error) {
            add_issue(
                snapshot,
                CpuHealthIssueCode::thermal_query_failed,
                -1,
                false,
                "unable to enumerate /sys/class/thermal");
            return;
        }

        const std::string filename = entry.path().filename().string();
        if (filename.rfind("thermal_zone", 0) != 0) {
            continue;
        }

        const auto type = read_text_file(entry.path() / "type");
        const auto temperature = read_text_file(entry.path() / "temp");
        if (!type.has_value() || !temperature.has_value() ||
            !is_cpu_temperature_provider(*type)) {
            continue;
        }

        CpuTemperatureReading reading{};
        reading.source_path = entry.path().string();
        reading.label = *type;
        reading.association = temperature_association(*type);
        const auto parsed = parse_int64(*temperature);
        if (parsed.has_value()) {
            reading.state = CpuTelemetryState::available;
            reading.millidegrees_celsius = *parsed;
        } else {
            reading.state = CpuTelemetryState::query_failed;
        }
        snapshot.temperatures.push_back(std::move(reading));
    }
}

void sample_hwmon_temperatures(CpuHealthSnapshot& snapshot) {
    const fs::path root("/sys/class/hwmon");
    std::error_code error;
    if (!fs::is_directory(root, error)) {
        return;
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(root, error)) {
        if (error) {
            add_issue(
                snapshot,
                CpuHealthIssueCode::thermal_query_failed,
                -1,
                false,
                "unable to enumerate /sys/class/hwmon");
            return;
        }

        const auto provider = read_text_file(entry.path() / "name");
        if (!provider.has_value() ||
            !is_cpu_temperature_provider(*provider)) {
            continue;
        }

        for (const fs::directory_entry& sensor :
             fs::directory_iterator(entry.path(), error)) {
            if (error) {
                break;
            }
            const std::string filename = sensor.path().filename().string();
            if (filename.rfind("temp", 0) != 0 ||
                filename.size() <= 10 ||
                filename.substr(filename.size() - 6) != "_input") {
                continue;
            }

            const std::string prefix = filename.substr(0, filename.size() - 6);
            const auto value = read_text_file(sensor.path());
            if (!value.has_value()) {
                continue;
            }

            CpuTemperatureReading reading{};
            reading.source_path = sensor.path().string();
            const auto label = read_text_file(entry.path() / (prefix + "_label"));
            reading.label = label.value_or(*provider + ":" + prefix);
            reading.association = temperature_association(reading.label);
            if (reading.association == CpuTemperatureAssociation::unknown) {
                reading.association = temperature_association(*provider);
            }

            const auto parsed = parse_int64(*value);
            if (parsed.has_value()) {
                reading.state = CpuTelemetryState::available;
                reading.millidegrees_celsius = *parsed;
            } else {
                reading.state = CpuTelemetryState::query_failed;
            }
            snapshot.temperatures.push_back(std::move(reading));
        }
    }
}

void sample_temperatures(CpuHealthSnapshot& snapshot) {
    sample_thermal_zones(snapshot);
    sample_hwmon_temperatures(snapshot);

    if (snapshot.temperatures.empty()) {
        snapshot.temperature_provider_state =
            CpuTelemetryState::provider_unavailable;
        return;
    }

    const bool any_available = std::any_of(
        snapshot.temperatures.begin(),
        snapshot.temperatures.end(),
        [](const CpuTemperatureReading& reading) {
            return reading.state == CpuTelemetryState::available;
        });
    snapshot.temperature_provider_state =
        any_available ? CpuTelemetryState::available
                      : CpuTelemetryState::query_failed;
}

}  // namespace

CpuHealthQueryResult query_cpu_health(
    const CpuTopologySnapshot& topology,
    const CpuHealthSnapshot* previous) {
    CpuHealthQueryResult result{};

#if !defined(__linux__)
    static_cast<void>(topology);
    static_cast<void>(previous);
    result.status.code = CpuHealthQueryCode::unsupported_platform;
    result.status.message =
        "Phase 7.1 CPU health sampling currently supports Linux only";
    return result;
#else
    CpuHealthSnapshot& snapshot = result.health;
    snapshot.sample_time_unix_nanoseconds = unix_time_nanoseconds();
    snapshot.sample_time_monotonic_nanoseconds =
        monotonic_time_nanoseconds();

    sample_utilization(snapshot, topology, previous);
    sample_frequencies(snapshot, topology);
    sample_load_average(snapshot);
    sample_memory(snapshot);
    snapshot.cpu_pressure =
        sample_pressure("/proc/pressure/cpu", snapshot);
    snapshot.memory_pressure =
        sample_pressure("/proc/pressure/memory", snapshot);
    sample_temperatures(snapshot);

    const bool fatal_issue = std::any_of(
        snapshot.issues.begin(), snapshot.issues.end(),
        [](const CpuHealthIssue& issue) { return issue.fatal; });
    const bool core_sample_available =
        snapshot.aggregate_utilization.counters.available ||
        snapshot.memory.state == CpuTelemetryState::available ||
        snapshot.load_average.state == CpuTelemetryState::available;

    if (fatal_issue || !core_sample_available) {
        result.status.code = CpuHealthQueryCode::query_failed;
        result.status.message = "CPU health sampling produced no core telemetry";
    } else if (!snapshot.issues.empty()) {
        result.status.code = CpuHealthQueryCode::partial_success;
        result.status.message =
            "CPU health sampling completed with partial information";
    } else {
        result.status.code = CpuHealthQueryCode::success;
    }
    return result;
#endif
}

const char* to_string(CpuTelemetryState value) noexcept {
    switch (value) {
        case CpuTelemetryState::unknown:
            return "unknown";
        case CpuTelemetryState::available:
            return "available";
        case CpuTelemetryState::baseline_only:
            return "baseline_only";
        case CpuTelemetryState::unsupported:
            return "unsupported";
        case CpuTelemetryState::provider_unavailable:
            return "provider_unavailable";
        case CpuTelemetryState::query_failed:
            return "query_failed";
    }
    return "unknown";
}

const char* to_string(CpuTemperatureAssociation value) noexcept {
    switch (value) {
        case CpuTemperatureAssociation::unknown:
            return "unknown";
        case CpuTemperatureAssociation::system_cpu:
            return "system_cpu";
        case CpuTemperatureAssociation::package:
            return "package";
    }
    return "unknown";
}

const char* to_string(CpuHealthIssueCode value) noexcept {
    switch (value) {
        case CpuHealthIssueCode::proc_stat_unavailable:
            return "proc_stat_unavailable";
        case CpuHealthIssueCode::proc_stat_malformed:
            return "proc_stat_malformed";
        case CpuHealthIssueCode::load_average_unavailable:
            return "load_average_unavailable";
        case CpuHealthIssueCode::memory_info_unavailable:
            return "memory_info_unavailable";
        case CpuHealthIssueCode::frequency_query_failed:
            return "frequency_query_failed";
        case CpuHealthIssueCode::thermal_query_failed:
            return "thermal_query_failed";
        case CpuHealthIssueCode::pressure_query_failed:
            return "pressure_query_failed";
    }
    return "unknown";
}

const char* to_string(CpuHealthQueryCode value) noexcept {
    switch (value) {
        case CpuHealthQueryCode::success:
            return "success";
        case CpuHealthQueryCode::partial_success:
            return "partial_success";
        case CpuHealthQueryCode::unsupported_platform:
            return "unsupported_platform";
        case CpuHealthQueryCode::query_failed:
            return "query_failed";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cpu
