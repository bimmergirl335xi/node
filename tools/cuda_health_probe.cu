#include "cuda_health.hpp"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

namespace cuda_backend = prometheus::backends::cuda;

namespace {

template <typename T>
void print_metric(
    const char* label,
    const cuda_backend::CudaHealthMetric<T>& metric,
    const char* suffix = "") {
    std::cout << "  " << label << ": ";
    if (metric.available()) {
        std::cout << metric.value << suffix;
    } else {
        std::cout << cuda_backend::to_string(metric.state);
        if (!metric.message.empty()) {
            std::cout << " (" << metric.message << ')';
        }
    }
    std::cout << " [source="
              << cuda_backend::to_string(metric.source) << "]\n";
}

void print_bool_metric(
    const char* label,
    const cuda_backend::CudaHealthMetric<bool>& metric) {
    std::cout << "  " << label << ": ";
    if (metric.available()) {
        std::cout << (metric.value ? "yes" : "no");
    } else {
        std::cout << cuda_backend::to_string(metric.state);
        if (!metric.message.empty()) {
            std::cout << " (" << metric.message << ')';
        }
    }
    std::cout << " [source="
              << cuda_backend::to_string(metric.source) << "]\n";
}

void print_bytes_metric(
    const char* label,
    const cuda_backend::CudaHealthMetric<std::uint64_t>& metric) {
    std::cout << "  " << label << ": ";
    if (metric.available()) {
        const double gib = static_cast<double>(metric.value) /
                           (1024.0 * 1024.0 * 1024.0);
        std::cout << metric.value << " bytes ("
                  << std::fixed << std::setprecision(2) << gib
                  << " GiB)" << std::defaultfloat;
    } else {
        std::cout << cuda_backend::to_string(metric.state);
        if (!metric.message.empty()) {
            std::cout << " (" << metric.message << ')';
        }
    }
    std::cout << " [source="
              << cuda_backend::to_string(metric.source) << "]\n";
}

}  // namespace

int main() {
    const cuda_backend::CudaDeviceDiscoveryResult discovery =
        cuda_backend::discover_cuda_devices();

    std::cout << "Prometheus CUDA health probe\n";
    std::cout << "Discovery status: "
              << cuda_backend::to_string(discovery.status.code) << '\n';
    std::cout << "Visible devices: " << discovery.devices.size()
              << "\n\n";

    bool fatal_failure = false;
    bool partial_result = false;

    for (const cuda_backend::CudaDiscoveredDevice& device :
         discovery.devices) {
        const cuda_backend::CudaHealthQueryResult health =
            cuda_backend::query_cuda_device_health(device.identity);
        const auto& snapshot = health.snapshot;

        std::cout << "Device " << device.identity.persistent_key << '\n';
        std::cout << "  query: "
                  << cuda_backend::to_string(health.status.code) << '\n';
        if (!health.status.message.empty()) {
            std::cout << "  status message: "
                      << health.status.message << '\n';
        }
        std::cout << "  runtime ordinal: "
                  << snapshot.requested_runtime_ordinal << '\n';
        std::cout << "  runtime binding: "
                  << cuda_backend::to_string(snapshot.runtime_binding)
                  << '\n';
        std::cout << "  NVML provider: "
                  << cuda_backend::to_string(snapshot.nvml_provider)
                  << '\n';
        std::cout << "  sampled at Unix ns: "
                  << snapshot.sampled_at_unix_ns << '\n';

        print_bytes_metric("memory total", snapshot.memory_total_bytes);
        print_bytes_metric("memory free", snapshot.memory_free_bytes);
        print_bytes_metric("memory used", snapshot.memory_used_bytes);
        print_metric("GPU temperature", snapshot.gpu_temperature_c, " C");
        print_metric("power draw", snapshot.power_draw_mw, " mW");
        print_metric(
            "GPU utilization",
            snapshot.gpu_utilization_percent,
            "%");
        print_metric(
            "memory utilization",
            snapshot.memory_utilization_percent,
            "%");
        print_bool_metric(
            "ECC currently enabled",
            snapshot.ecc_currently_enabled);
        print_bool_metric(
            "ECC pending enabled",
            snapshot.ecc_pending_enabled);
        print_metric(
            "corrected ECC volatile",
            snapshot.corrected_ecc_volatile);
        print_metric(
            "uncorrected ECC volatile",
            snapshot.uncorrected_ecc_volatile);
        print_metric(
            "corrected ECC aggregate",
            snapshot.corrected_ecc_aggregate);
        print_metric(
            "uncorrected ECC aggregate",
            snapshot.uncorrected_ecc_aggregate);
        print_bool_metric(
            "retired pages pending",
            snapshot.retired_pages_pending);

        std::cout << "  clock event reasons: ";
        if (snapshot.clock_event_reasons.available()) {
            std::cout << "0x" << std::hex
                      << snapshot.clock_event_reasons.value
                      << std::dec;
        } else {
            std::cout << cuda_backend::to_string(
                snapshot.clock_event_reasons.state);
        }
        std::cout << " [source="
                  << cuda_backend::to_string(
                         snapshot.clock_event_reasons.source)
                  << "]\n";

        if (snapshot.issues.empty()) {
            std::cout << "  issues: none\n";
        } else {
            std::cout << "  issues:\n";
            for (const cuda_backend::CudaHealthIssue& issue :
                 snapshot.issues) {
                std::cout << "    - "
                          << cuda_backend::to_string(issue.code)
                          << " [binding-fatal="
                          << (issue.execution_binding_fatal ? "yes" : "no")
                          << "]: " << issue.message << '\n';
            }
        }
        std::cout << '\n';

        if (!health.status.completed()) {
            fatal_failure = true;
        } else if (health.status.code !=
                   cuda_backend::CudaHealthQueryCode::success) {
            partial_result = true;
        }
    }

    if (fatal_failure) {
        return 1;
    }
    if (partial_result || !discovery.status.complete()) {
        return 2;
    }
    return 0;
}
