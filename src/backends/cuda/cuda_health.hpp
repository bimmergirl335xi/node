#ifndef SRC_BACKENDS_CUDA_CUDA_HEALTH_HPP
#define SRC_BACKENDS_CUDA_CUDA_HEALTH_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "cuda_device.hpp"

namespace prometheus::backends::cuda {

// Live telemetry is intentionally separate from immutable device identity and
// capabilities.  Unsupported and unavailable are distinct so missing telemetry
// is never silently interpreted as a healthy value.
enum class CudaHealthMetricState : std::uint8_t {
    unknown = 0,
    available,
    unsupported,
    permission_denied,
    provider_unavailable,
    query_failed,
};

enum class CudaHealthSource : std::uint8_t {
    none = 0,
    cuda_runtime,
    nvml,
};

template <typename T>
struct CudaHealthMetric {
    CudaHealthMetricState state = CudaHealthMetricState::unknown;
    CudaHealthSource source = CudaHealthSource::none;
    T value{};
    int native_error_code = 0;
    std::string message{};

    [[nodiscard]] bool available() const noexcept {
        return state == CudaHealthMetricState::available;
    }
};

enum class CudaRuntimeBindingState : std::uint8_t {
    unknown = 0,
    matched,
    identity_mismatch,
    ordinal_unavailable,
    query_failed,
};

enum class CudaNvmlProviderState : std::uint8_t {
    not_requested = 0,
    ready,
    library_unavailable,
    initialization_failed,
    device_not_found,
    device_lost,
};

enum class CudaHealthIssueCode : std::uint8_t {
    invalid_stable_identity = 0,
    runtime_identity_query_failed,
    runtime_identity_mismatch,
    nvml_library_unavailable,
    nvml_initialization_failed,
    nvml_device_lookup_failed,
    nvml_metric_query_failed,
    cuda_memory_query_failed,
    current_device_query_failed,
    current_device_restore_failed,
};

struct CudaHealthIssue {
    CudaHealthIssueCode code =
        CudaHealthIssueCode::invalid_stable_identity;
    int native_error_code = 0;
    bool execution_binding_fatal = false;
    std::string message{};
};

// A point-in-time observation.  This structure does not authorize scheduling,
// disable a device, or make thermal/power policy decisions.
struct CudaDeviceHealthSnapshot {
    std::uint64_t sampled_at_unix_ns = 0;

    std::string persistent_key{};
    std::string uuid{};
    int requested_runtime_ordinal = -1;

    CudaRuntimeBindingState runtime_binding =
        CudaRuntimeBindingState::unknown;
    CudaNvmlProviderState nvml_provider =
        CudaNvmlProviderState::not_requested;

    CudaHealthMetric<std::uint64_t> memory_total_bytes{};
    CudaHealthMetric<std::uint64_t> memory_free_bytes{};
    CudaHealthMetric<std::uint64_t> memory_used_bytes{};

    CudaHealthMetric<std::uint32_t> gpu_temperature_c{};
    CudaHealthMetric<std::uint32_t> power_draw_mw{};
    CudaHealthMetric<std::uint32_t> gpu_utilization_percent{};
    CudaHealthMetric<std::uint32_t> memory_utilization_percent{};

    CudaHealthMetric<bool> ecc_currently_enabled{};
    CudaHealthMetric<bool> ecc_pending_enabled{};
    CudaHealthMetric<std::uint64_t> corrected_ecc_volatile{};
    CudaHealthMetric<std::uint64_t> uncorrected_ecc_volatile{};
    CudaHealthMetric<std::uint64_t> corrected_ecc_aggregate{};
    CudaHealthMetric<std::uint64_t> uncorrected_ecc_aggregate{};
    CudaHealthMetric<bool> retired_pages_pending{};

    // Raw NVML clock-event/throttle-reason mask.  Interpretation and policy
    // belong in diagnostics, not in this low-level backend query.
    CudaHealthMetric<std::uint64_t> clock_event_reasons{};

    std::vector<CudaHealthIssue> issues{};
};

struct CudaHealthQueryOptions {
    // NVML is loaded dynamically, so the CUDA backend still builds and runs
    // when the management library is absent.
    bool query_nvml = true;

    // cudaMemGetInfo requires selecting the CUDA runtime device and may cause
    // primary-context initialization.  It is used only when NVML memory data
    // is unavailable, only after UUID/ordinal revalidation, and the previous
    // current device is restored before return.
    bool allow_cuda_memory_fallback = true;
};

enum class CudaHealthQueryCode : std::uint8_t {
    success = 0,
    partial_success,
    invalid_identity,
    identity_mismatch,
    query_failed,
};

struct CudaHealthQueryStatus {
    CudaHealthQueryCode code = CudaHealthQueryCode::success;
    std::string message{};

    [[nodiscard]] bool completed() const noexcept {
        return code == CudaHealthQueryCode::success ||
               code == CudaHealthQueryCode::partial_success ||
               code == CudaHealthQueryCode::identity_mismatch;
    }

    [[nodiscard]] bool runtime_binding_usable() const noexcept {
        return code == CudaHealthQueryCode::success ||
               code == CudaHealthQueryCode::partial_success;
    }
};

struct CudaHealthQueryResult {
    CudaHealthQueryStatus status{};
    CudaDeviceHealthSnapshot snapshot{};
};

// Revalidates the temporary runtime ordinal against the durable UUID before
// any ordinal-based fallback query.  NVML telemetry is resolved by UUID.
[[nodiscard]] CudaHealthQueryResult query_cuda_device_health(
    const CudaDeviceIdentity& identity,
    const CudaHealthQueryOptions& options = {});

[[nodiscard]] const char* to_string(CudaHealthMetricState value) noexcept;
[[nodiscard]] const char* to_string(CudaHealthSource value) noexcept;
[[nodiscard]] const char* to_string(CudaRuntimeBindingState value) noexcept;
[[nodiscard]] const char* to_string(CudaNvmlProviderState value) noexcept;
[[nodiscard]] const char* to_string(CudaHealthIssueCode value) noexcept;
[[nodiscard]] const char* to_string(CudaHealthQueryCode value) noexcept;

}  // namespace prometheus::backends::cuda

#endif  // SRC_BACKENDS_CUDA_CUDA_HEALTH_HPP
