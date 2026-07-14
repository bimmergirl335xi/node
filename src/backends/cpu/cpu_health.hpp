#ifndef SRC_BACKENDS_CPU_CPU_HEALTH_HPP
#define SRC_BACKENDS_CPU_CPU_HEALTH_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "cpu_topology.hpp"

namespace prometheus::backends::cpu {

enum class CpuTelemetryState : std::uint8_t {
    unknown = 0,
    available,
    baseline_only,
    unsupported,
    provider_unavailable,
    query_failed,
};

struct CpuCounterSnapshot {
    std::uint64_t total_ticks = 0;
    std::uint64_t idle_ticks = 0;
    bool available = false;
};

struct CpuUtilizationReading {
    int os_cpu_id = -1;
    CpuTelemetryState state = CpuTelemetryState::unknown;
    double busy_fraction = 0.0;
    CpuCounterSnapshot counters{};
};

struct CpuFrequencyReading {
    int os_cpu_id = -1;
    CpuTelemetryState current_state = CpuTelemetryState::unknown;
    CpuTelemetryState maximum_state = CpuTelemetryState::unknown;
    std::uint64_t current_khz = 0;
    std::uint64_t maximum_khz = 0;
};

enum class CpuTemperatureAssociation : std::uint8_t {
    unknown = 0,
    system_cpu,
    package,
};

struct CpuTemperatureReading {
    std::string source_path{};
    std::string label{};
    CpuTemperatureAssociation association =
        CpuTemperatureAssociation::unknown;
    int package_id = -1;
    CpuTelemetryState state = CpuTelemetryState::unknown;
    std::int64_t millidegrees_celsius = 0;
};

struct CpuLoadAverageReading {
    CpuTelemetryState state = CpuTelemetryState::unknown;
    double one_minute = 0.0;
    double five_minutes = 0.0;
    double fifteen_minutes = 0.0;
    std::uint64_t runnable_entities = 0;
    std::uint64_t total_entities = 0;
    std::uint64_t latest_pid = 0;
};

struct CpuMemoryHealthReading {
    CpuTelemetryState state = CpuTelemetryState::unknown;
    std::uint64_t total_bytes = 0;
    std::uint64_t available_bytes = 0;
    std::uint64_t swap_total_bytes = 0;
    std::uint64_t swap_free_bytes = 0;
};

struct CpuPressureWindow {
    bool available = false;
    double average_10_seconds = 0.0;
    double average_60_seconds = 0.0;
    double average_300_seconds = 0.0;
    std::uint64_t total_stall_microseconds = 0;
};

struct CpuPressureReading {
    CpuTelemetryState state = CpuTelemetryState::unknown;
    CpuPressureWindow some{};
    CpuPressureWindow full{};
};

enum class CpuHealthIssueCode : std::uint8_t {
    proc_stat_unavailable = 0,
    proc_stat_malformed,
    load_average_unavailable,
    memory_info_unavailable,
    frequency_query_failed,
    thermal_query_failed,
    pressure_query_failed,
};

struct CpuHealthIssue {
    CpuHealthIssueCode code = CpuHealthIssueCode::proc_stat_unavailable;
    int os_cpu_id = -1;
    bool fatal = false;
    std::string message{};
};

enum class CpuHealthQueryCode : std::uint8_t {
    success = 0,
    partial_success,
    unsupported_platform,
    query_failed,
};

struct CpuHealthQueryStatus {
    CpuHealthQueryCode code = CpuHealthQueryCode::success;
    std::string message{};

    [[nodiscard]] bool completed() const noexcept {
        return code == CpuHealthQueryCode::success ||
               code == CpuHealthQueryCode::partial_success;
    }
};

struct CpuHealthSnapshot {
    std::uint64_t sample_time_unix_nanoseconds = 0;
    std::uint64_t sample_time_monotonic_nanoseconds = 0;

    CpuUtilizationReading aggregate_utilization{};
    std::vector<CpuUtilizationReading> logical_processor_utilization{};
    std::vector<CpuFrequencyReading> frequencies{};

    CpuTelemetryState temperature_provider_state = CpuTelemetryState::unknown;
    std::vector<CpuTemperatureReading> temperatures{};

    CpuLoadAverageReading load_average{};
    CpuMemoryHealthReading memory{};
    CpuPressureReading cpu_pressure{};
    CpuPressureReading memory_pressure{};

    std::vector<CpuHealthIssue> issues{};
};

struct CpuHealthQueryResult {
    CpuHealthQueryStatus status{};
    CpuHealthSnapshot health{};
};

// Read-only health sampling. Supplying the previous snapshot permits a
// utilization delta; the first sample deliberately reports baseline_only.
[[nodiscard]] CpuHealthQueryResult query_cpu_health(
    const CpuTopologySnapshot& topology,
    const CpuHealthSnapshot* previous = nullptr);

[[nodiscard]] const char* to_string(CpuTelemetryState value) noexcept;
[[nodiscard]] const char* to_string(
    CpuTemperatureAssociation value) noexcept;
[[nodiscard]] const char* to_string(CpuHealthIssueCode value) noexcept;
[[nodiscard]] const char* to_string(CpuHealthQueryCode value) noexcept;

}  // namespace prometheus::backends::cpu

#endif  // SRC_BACKENDS_CPU_CPU_HEALTH_HPP
