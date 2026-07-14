#include "cuda_health.hpp"

#include <cuda_runtime_api.h>
#include <dlfcn.h>

#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>

namespace prometheus::backends::cuda {
namespace {

using NvmlReturn = int;
using NvmlDeviceHandle = void*;

struct NvmlMemory {
    unsigned long long total;
    unsigned long long free;
    unsigned long long used;
};

struct NvmlUtilization {
    unsigned int gpu;
    unsigned int memory;
};

inline constexpr NvmlReturn kNvmlSuccess = 0;
inline constexpr NvmlReturn kNvmlErrorNotSupported = 3;
inline constexpr NvmlReturn kNvmlErrorNoPermission = 4;
inline constexpr NvmlReturn kNvmlErrorNotFound = 6;
inline constexpr NvmlReturn kNvmlErrorGpuIsLost = 15;

inline constexpr unsigned int kNvmlTemperatureGpu = 0;
inline constexpr unsigned int kNvmlFeatureDisabled = 0;
inline constexpr unsigned int kNvmlFeatureEnabled = 1;
inline constexpr unsigned int kNvmlMemoryErrorCorrected = 0;
inline constexpr unsigned int kNvmlMemoryErrorUncorrected = 1;
inline constexpr unsigned int kNvmlVolatileEcc = 0;
inline constexpr unsigned int kNvmlAggregateEcc = 1;

using NvmlInitFn = NvmlReturn (*)();
using NvmlShutdownFn = NvmlReturn (*)();
using NvmlErrorStringFn = const char* (*)(NvmlReturn);
using NvmlDeviceGetHandleByUuidFn =
    NvmlReturn (*)(const char*, NvmlDeviceHandle*);
using NvmlDeviceGetMemoryInfoFn =
    NvmlReturn (*)(NvmlDeviceHandle, NvmlMemory*);
using NvmlDeviceGetTemperatureFn =
    NvmlReturn (*)(NvmlDeviceHandle, unsigned int, unsigned int*);
using NvmlDeviceGetPowerUsageFn =
    NvmlReturn (*)(NvmlDeviceHandle, unsigned int*);
using NvmlDeviceGetUtilizationRatesFn =
    NvmlReturn (*)(NvmlDeviceHandle, NvmlUtilization*);
using NvmlDeviceGetEccModeFn = NvmlReturn (*)(
    NvmlDeviceHandle,
    unsigned int*,
    unsigned int*);
using NvmlDeviceGetTotalEccErrorsFn = NvmlReturn (*)(
    NvmlDeviceHandle,
    unsigned int,
    unsigned int,
    unsigned long long*);
using NvmlDeviceGetRetiredPagesPendingStatusFn =
    NvmlReturn (*)(NvmlDeviceHandle, unsigned int*);
using NvmlDeviceGetCurrentClocksThrottleReasonsFn =
    NvmlReturn (*)(NvmlDeviceHandle, unsigned long long*);

template <typename FunctionType>
[[nodiscard]] FunctionType load_function(
    void* library,
    const char* symbol_name) noexcept {
    static_assert(std::is_pointer<FunctionType>::value,
                  "FunctionType must be a function pointer");

    FunctionType function = nullptr;
    void* symbol = dlsym(library, symbol_name);
    static_assert(sizeof(function) == sizeof(symbol),
                  "Function and object pointers must have equal size here");
    std::memcpy(&function, &symbol, sizeof(function));
    return function;
}

class NvmlDynamicApi {
public:
    NvmlDynamicApi() {
        library_ = dlopen("libnvidia-ml.so.1", RTLD_NOW | RTLD_LOCAL);
        if (library_ == nullptr) {
            library_ = dlopen("libnvidia-ml.so", RTLD_NOW | RTLD_LOCAL);
        }
        if (library_ == nullptr) {
            const char* error = dlerror();
            load_error_ = error == nullptr
                              ? "NVML shared library was not found"
                              : error;
            return;
        }

        init_ = load_function<NvmlInitFn>(library_, "nvmlInit_v2");
        if (init_ == nullptr) {
            init_ = load_function<NvmlInitFn>(library_, "nvmlInit");
        }
        shutdown_ =
            load_function<NvmlShutdownFn>(library_, "nvmlShutdown");
        error_string_ =
            load_function<NvmlErrorStringFn>(library_, "nvmlErrorString");
        get_handle_by_uuid_ = load_function<NvmlDeviceGetHandleByUuidFn>(
            library_,
            "nvmlDeviceGetHandleByUUID");

        get_memory_info_ = load_function<NvmlDeviceGetMemoryInfoFn>(
            library_,
            "nvmlDeviceGetMemoryInfo");
        get_temperature_ = load_function<NvmlDeviceGetTemperatureFn>(
            library_,
            "nvmlDeviceGetTemperature");
        get_power_usage_ = load_function<NvmlDeviceGetPowerUsageFn>(
            library_,
            "nvmlDeviceGetPowerUsage");
        get_utilization_rates_ =
            load_function<NvmlDeviceGetUtilizationRatesFn>(
                library_,
                "nvmlDeviceGetUtilizationRates");
        get_ecc_mode_ = load_function<NvmlDeviceGetEccModeFn>(
            library_,
            "nvmlDeviceGetEccMode");
        get_total_ecc_errors_ =
            load_function<NvmlDeviceGetTotalEccErrorsFn>(
                library_,
                "nvmlDeviceGetTotalEccErrors");
        get_retired_pages_pending_status_ =
            load_function<NvmlDeviceGetRetiredPagesPendingStatusFn>(
                library_,
                "nvmlDeviceGetRetiredPagesPendingStatus");
        get_current_clock_event_reasons_ =
            load_function<NvmlDeviceGetCurrentClocksThrottleReasonsFn>(
                library_,
                "nvmlDeviceGetCurrentClocksEventReasons");
        if (get_current_clock_event_reasons_ == nullptr) {
            get_current_clock_event_reasons_ =
                load_function<NvmlDeviceGetCurrentClocksThrottleReasonsFn>(
                    library_,
                    "nvmlDeviceGetCurrentClocksThrottleReasons");
        }

        if (init_ == nullptr || get_handle_by_uuid_ == nullptr) {
            load_error_ =
                "NVML is missing required initialization or UUID lookup symbols";
            return;
        }

        init_status_ = init_();
        initialized_ = init_status_ == kNvmlSuccess;
        if (!initialized_) {
            load_error_ = make_error_message(init_status_, "NVML initialization");
        }
    }

    ~NvmlDynamicApi() {
        if (initialized_ && shutdown_ != nullptr) {
            (void)shutdown_();
        }
        if (library_ != nullptr) {
            dlclose(library_);
        }
    }

    NvmlDynamicApi(const NvmlDynamicApi&) = delete;
    NvmlDynamicApi& operator=(const NvmlDynamicApi&) = delete;

    [[nodiscard]] bool library_loaded() const noexcept {
        return library_ != nullptr;
    }

    [[nodiscard]] bool initialized() const noexcept {
        return initialized_;
    }

    [[nodiscard]] NvmlReturn init_status() const noexcept {
        return init_status_;
    }

    [[nodiscard]] const std::string& load_error() const noexcept {
        return load_error_;
    }

    [[nodiscard]] std::string make_error_message(
        NvmlReturn status,
        const char* operation) const {
        std::string message = operation;
        message += " failed";
        if (error_string_ != nullptr) {
            const char* text = error_string_(status);
            if (text != nullptr) {
                message += ": ";
                message += text;
            }
        } else {
            message += " with NVML status ";
            message += std::to_string(status);
        }
        return message;
    }

    NvmlDeviceGetHandleByUuidFn get_handle_by_uuid_ = nullptr;
    NvmlDeviceGetMemoryInfoFn get_memory_info_ = nullptr;
    NvmlDeviceGetTemperatureFn get_temperature_ = nullptr;
    NvmlDeviceGetPowerUsageFn get_power_usage_ = nullptr;
    NvmlDeviceGetUtilizationRatesFn get_utilization_rates_ = nullptr;
    NvmlDeviceGetEccModeFn get_ecc_mode_ = nullptr;
    NvmlDeviceGetTotalEccErrorsFn get_total_ecc_errors_ = nullptr;
    NvmlDeviceGetRetiredPagesPendingStatusFn
        get_retired_pages_pending_status_ = nullptr;
    NvmlDeviceGetCurrentClocksThrottleReasonsFn
        get_current_clock_event_reasons_ = nullptr;

private:
    void* library_ = nullptr;
    NvmlInitFn init_ = nullptr;
    NvmlShutdownFn shutdown_ = nullptr;
    NvmlErrorStringFn error_string_ = nullptr;
    NvmlReturn init_status_ = 0;
    bool initialized_ = false;
    std::string load_error_{};
};

[[nodiscard]] NvmlDynamicApi& nvml_api() {
    static NvmlDynamicApi api{};
    return api;
}

[[nodiscard]] std::uint64_t current_unix_time_ns() noexcept {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

[[nodiscard]] std::string make_cuda_error_message(
    cudaError_t error,
    const char* operation) {
    std::string message = operation;
    message += " failed";

    const char* error_name = cudaGetErrorName(error);
    const char* error_text = cudaGetErrorString(error);
    if (error_name != nullptr) {
        message += " (";
        message += error_name;
        message += ')';
    }
    if (error_text != nullptr) {
        message += ": ";
        message += error_text;
    }
    return message;
}

template <typename T>
void set_available(
    CudaHealthMetric<T>& metric,
    T value,
    CudaHealthSource source) {
    metric.state = CudaHealthMetricState::available;
    metric.source = source;
    metric.value = value;
    metric.native_error_code = 0;
    metric.message.clear();
}

template <typename T>
void set_metric_failure(
    CudaHealthMetric<T>& metric,
    CudaHealthMetricState state,
    CudaHealthSource source,
    int native_error_code,
    std::string message) {
    metric.state = state;
    metric.source = source;
    metric.native_error_code = native_error_code;
    metric.message = std::move(message);
}

[[nodiscard]] CudaHealthMetricState metric_state_for_nvml_error(
    NvmlReturn status) noexcept {
    if (status == kNvmlErrorNotSupported) {
        return CudaHealthMetricState::unsupported;
    }
    if (status == kNvmlErrorNoPermission) {
        return CudaHealthMetricState::permission_denied;
    }
    if (status == kNvmlErrorNotFound) {
        return CudaHealthMetricState::provider_unavailable;
    }
    return CudaHealthMetricState::query_failed;
}

void append_issue(
    CudaDeviceHealthSnapshot& snapshot,
    CudaHealthIssueCode code,
    int native_error_code,
    bool execution_binding_fatal,
    std::string message) {
    CudaHealthIssue issue{};
    issue.code = code;
    issue.native_error_code = native_error_code;
    issue.execution_binding_fatal = execution_binding_fatal;
    issue.message = std::move(message);
    snapshot.issues.push_back(std::move(issue));
}


template <typename T>
void set_nvml_symbol_unavailable(
    CudaHealthMetric<T>& metric,
    const char* symbol_name) {
    std::string message = "NVML function symbol unavailable: ";
    message += symbol_name;
    set_metric_failure(
        metric,
        CudaHealthMetricState::unsupported,
        CudaHealthSource::nvml,
        0,
        std::move(message));
}

template <typename T>
void set_nvml_result(
    CudaDeviceHealthSnapshot& snapshot,
    CudaHealthMetric<T>& metric,
    NvmlReturn status,
    T value,
    const char* operation,
    const NvmlDynamicApi& api) {
    if (status == kNvmlSuccess) {
        set_available(metric, value, CudaHealthSource::nvml);
        return;
    }

    const CudaHealthMetricState state = metric_state_for_nvml_error(status);
    const std::string message = api.make_error_message(status, operation);
    set_metric_failure(
        metric,
        state,
        CudaHealthSource::nvml,
        status,
        message);

    if (state != CudaHealthMetricState::unsupported) {
        append_issue(
            snapshot,
            CudaHealthIssueCode::nvml_metric_query_failed,
            status,
            false,
            message);
    }
}

void mark_nvml_unavailable_metrics(
    CudaDeviceHealthSnapshot& snapshot,
    const std::string& message) {
    auto mark = [&message](auto& metric) {
        set_metric_failure(
            metric,
            CudaHealthMetricState::provider_unavailable,
            CudaHealthSource::nvml,
            0,
            message);
    };

    mark(snapshot.gpu_temperature_c);
    mark(snapshot.power_draw_mw);
    mark(snapshot.gpu_utilization_percent);
    mark(snapshot.memory_utilization_percent);
    mark(snapshot.ecc_currently_enabled);
    mark(snapshot.ecc_pending_enabled);
    mark(snapshot.corrected_ecc_volatile);
    mark(snapshot.uncorrected_ecc_volatile);
    mark(snapshot.corrected_ecc_aggregate);
    mark(snapshot.uncorrected_ecc_aggregate);
    mark(snapshot.retired_pages_pending);
    mark(snapshot.clock_event_reasons);
}

void query_nvml_metrics(
    const CudaDeviceIdentity& identity,
    CudaDeviceHealthSnapshot& snapshot) {
    NvmlDynamicApi& api = nvml_api();
    if (!api.library_loaded()) {
        snapshot.nvml_provider =
            CudaNvmlProviderState::library_unavailable;
        append_issue(
            snapshot,
            CudaHealthIssueCode::nvml_library_unavailable,
            0,
            false,
            api.load_error());
        mark_nvml_unavailable_metrics(snapshot, api.load_error());
        return;
    }

    if (!api.initialized()) {
        snapshot.nvml_provider =
            CudaNvmlProviderState::initialization_failed;
        append_issue(
            snapshot,
            CudaHealthIssueCode::nvml_initialization_failed,
            api.init_status(),
            false,
            api.load_error());
        mark_nvml_unavailable_metrics(snapshot, api.load_error());
        return;
    }

    NvmlDeviceHandle handle = nullptr;
    const NvmlReturn handle_status =
        api.get_handle_by_uuid_(identity.uuid.c_str(), &handle);
    if (handle_status != kNvmlSuccess) {
        snapshot.nvml_provider =
            handle_status == kNvmlErrorGpuIsLost
                ? CudaNvmlProviderState::device_lost
                : CudaNvmlProviderState::device_not_found;
        const std::string message = api.make_error_message(
            handle_status,
            "nvmlDeviceGetHandleByUUID");
        append_issue(
            snapshot,
            CudaHealthIssueCode::nvml_device_lookup_failed,
            handle_status,
            handle_status == kNvmlErrorGpuIsLost,
            message);
        mark_nvml_unavailable_metrics(snapshot, message);
        return;
    }

    snapshot.nvml_provider = CudaNvmlProviderState::ready;

    if (api.get_memory_info_ != nullptr) {
        NvmlMemory memory{};
        const NvmlReturn status = api.get_memory_info_(handle, &memory);
        set_nvml_result(
            snapshot,
            snapshot.memory_total_bytes,
            status,
            static_cast<std::uint64_t>(memory.total),
            "nvmlDeviceGetMemoryInfo(total)",
            api);
        set_nvml_result(
            snapshot,
            snapshot.memory_free_bytes,
            status,
            static_cast<std::uint64_t>(memory.free),
            "nvmlDeviceGetMemoryInfo(free)",
            api);
        set_nvml_result(
            snapshot,
            snapshot.memory_used_bytes,
            status,
            static_cast<std::uint64_t>(memory.used),
            "nvmlDeviceGetMemoryInfo(used)",
            api);
    } else {
        set_nvml_symbol_unavailable(
            snapshot.memory_total_bytes,
            "nvmlDeviceGetMemoryInfo");
        set_nvml_symbol_unavailable(
            snapshot.memory_free_bytes,
            "nvmlDeviceGetMemoryInfo");
        set_nvml_symbol_unavailable(
            snapshot.memory_used_bytes,
            "nvmlDeviceGetMemoryInfo");
    }

    if (api.get_temperature_ != nullptr) {
        unsigned int temperature = 0;
        const NvmlReturn status = api.get_temperature_(
            handle,
            kNvmlTemperatureGpu,
            &temperature);
        set_nvml_result(
            snapshot,
            snapshot.gpu_temperature_c,
            status,
            static_cast<std::uint32_t>(temperature),
            "nvmlDeviceGetTemperature",
            api);
    } else {
        set_nvml_symbol_unavailable(
            snapshot.gpu_temperature_c,
            "nvmlDeviceGetTemperature");
    }

    if (api.get_power_usage_ != nullptr) {
        unsigned int power_mw = 0;
        const NvmlReturn status =
            api.get_power_usage_(handle, &power_mw);
        set_nvml_result(
            snapshot,
            snapshot.power_draw_mw,
            status,
            static_cast<std::uint32_t>(power_mw),
            "nvmlDeviceGetPowerUsage",
            api);
    } else {
        set_nvml_symbol_unavailable(
            snapshot.power_draw_mw,
            "nvmlDeviceGetPowerUsage");
    }

    if (api.get_utilization_rates_ != nullptr) {
        NvmlUtilization utilization{};
        const NvmlReturn status =
            api.get_utilization_rates_(handle, &utilization);
        set_nvml_result(
            snapshot,
            snapshot.gpu_utilization_percent,
            status,
            static_cast<std::uint32_t>(utilization.gpu),
            "nvmlDeviceGetUtilizationRates(gpu)",
            api);
        set_nvml_result(
            snapshot,
            snapshot.memory_utilization_percent,
            status,
            static_cast<std::uint32_t>(utilization.memory),
            "nvmlDeviceGetUtilizationRates(memory)",
            api);
    } else {
        set_nvml_symbol_unavailable(
            snapshot.gpu_utilization_percent,
            "nvmlDeviceGetUtilizationRates");
        set_nvml_symbol_unavailable(
            snapshot.memory_utilization_percent,
            "nvmlDeviceGetUtilizationRates");
    }

    if (api.get_ecc_mode_ != nullptr) {
        unsigned int current = kNvmlFeatureDisabled;
        unsigned int pending = kNvmlFeatureDisabled;
        const NvmlReturn status =
            api.get_ecc_mode_(handle, &current, &pending);
        set_nvml_result(
            snapshot,
            snapshot.ecc_currently_enabled,
            status,
            current == kNvmlFeatureEnabled,
            "nvmlDeviceGetEccMode(current)",
            api);
        set_nvml_result(
            snapshot,
            snapshot.ecc_pending_enabled,
            status,
            pending == kNvmlFeatureEnabled,
            "nvmlDeviceGetEccMode(pending)",
            api);
    } else {
        set_nvml_symbol_unavailable(
            snapshot.ecc_currently_enabled,
            "nvmlDeviceGetEccMode");
        set_nvml_symbol_unavailable(
            snapshot.ecc_pending_enabled,
            "nvmlDeviceGetEccMode");
    }

    if (api.get_total_ecc_errors_ != nullptr) {
        const auto query_ecc = [&](CudaHealthMetric<std::uint64_t>& metric,
                                   unsigned int error_type,
                                   unsigned int counter_type,
                                   const char* operation) {
            unsigned long long count = 0;
            const NvmlReturn status = api.get_total_ecc_errors_(
                handle,
                error_type,
                counter_type,
                &count);
            set_nvml_result(
                snapshot,
                metric,
                status,
                static_cast<std::uint64_t>(count),
                operation,
                api);
        };

        query_ecc(
            snapshot.corrected_ecc_volatile,
            kNvmlMemoryErrorCorrected,
            kNvmlVolatileEcc,
            "nvmlDeviceGetTotalEccErrors(corrected, volatile)");
        query_ecc(
            snapshot.uncorrected_ecc_volatile,
            kNvmlMemoryErrorUncorrected,
            kNvmlVolatileEcc,
            "nvmlDeviceGetTotalEccErrors(uncorrected, volatile)");
        query_ecc(
            snapshot.corrected_ecc_aggregate,
            kNvmlMemoryErrorCorrected,
            kNvmlAggregateEcc,
            "nvmlDeviceGetTotalEccErrors(corrected, aggregate)");
        query_ecc(
            snapshot.uncorrected_ecc_aggregate,
            kNvmlMemoryErrorUncorrected,
            kNvmlAggregateEcc,
            "nvmlDeviceGetTotalEccErrors(uncorrected, aggregate)");
    } else {
        set_nvml_symbol_unavailable(
            snapshot.corrected_ecc_volatile,
            "nvmlDeviceGetTotalEccErrors");
        set_nvml_symbol_unavailable(
            snapshot.uncorrected_ecc_volatile,
            "nvmlDeviceGetTotalEccErrors");
        set_nvml_symbol_unavailable(
            snapshot.corrected_ecc_aggregate,
            "nvmlDeviceGetTotalEccErrors");
        set_nvml_symbol_unavailable(
            snapshot.uncorrected_ecc_aggregate,
            "nvmlDeviceGetTotalEccErrors");
    }

    if (api.get_retired_pages_pending_status_ != nullptr) {
        unsigned int pending = kNvmlFeatureDisabled;
        const NvmlReturn status =
            api.get_retired_pages_pending_status_(handle, &pending);
        set_nvml_result(
            snapshot,
            snapshot.retired_pages_pending,
            status,
            pending == kNvmlFeatureEnabled,
            "nvmlDeviceGetRetiredPagesPendingStatus",
            api);
    } else {
        set_nvml_symbol_unavailable(
            snapshot.retired_pages_pending,
            "nvmlDeviceGetRetiredPagesPendingStatus");
    }

    if (api.get_current_clock_event_reasons_ != nullptr) {
        unsigned long long reasons = 0;
        const NvmlReturn status =
            api.get_current_clock_event_reasons_(handle, &reasons);
        set_nvml_result(
            snapshot,
            snapshot.clock_event_reasons,
            status,
            static_cast<std::uint64_t>(reasons),
            "nvmlDeviceGetCurrentClocksEventReasons",
            api);
    } else {
        set_nvml_symbol_unavailable(
            snapshot.clock_event_reasons,
            "nvmlDeviceGetCurrentClocksEventReasons");
    }
}

[[nodiscard]] bool memory_metrics_available(
    const CudaDeviceHealthSnapshot& snapshot) noexcept {
    return snapshot.memory_total_bytes.available() &&
           snapshot.memory_free_bytes.available() &&
           snapshot.memory_used_bytes.available();
}

void query_cuda_memory_fallback(
    const CudaDeviceIdentity& identity,
    CudaDeviceHealthSnapshot& snapshot) {
    if (snapshot.runtime_binding != CudaRuntimeBindingState::matched) {
        return;
    }

    int previous_device = -1;
    const cudaError_t current_status = cudaGetDevice(&previous_device);
    if (current_status != cudaSuccess) {
        append_issue(
            snapshot,
            CudaHealthIssueCode::current_device_query_failed,
            static_cast<int>(current_status),
            false,
            make_cuda_error_message(current_status, "cudaGetDevice"));
        return;
    }

    const bool switch_required =
        previous_device != identity.runtime_ordinal;
    if (switch_required) {
        const cudaError_t set_status =
            cudaSetDevice(identity.runtime_ordinal);
        if (set_status != cudaSuccess) {
            const std::string message = make_cuda_error_message(
                set_status,
                "cudaSetDevice for memory health query");
            append_issue(
                snapshot,
                CudaHealthIssueCode::cuda_memory_query_failed,
                static_cast<int>(set_status),
                false,
                message);
            set_metric_failure(
                snapshot.memory_total_bytes,
                CudaHealthMetricState::query_failed,
                CudaHealthSource::cuda_runtime,
                static_cast<int>(set_status),
                message);
            set_metric_failure(
                snapshot.memory_free_bytes,
                CudaHealthMetricState::query_failed,
                CudaHealthSource::cuda_runtime,
                static_cast<int>(set_status),
                message);
            set_metric_failure(
                snapshot.memory_used_bytes,
                CudaHealthMetricState::query_failed,
                CudaHealthSource::cuda_runtime,
                static_cast<int>(set_status),
                message);
            return;
        }
    }

    std::size_t free_bytes = 0;
    std::size_t total_bytes = 0;
    const cudaError_t memory_status =
        cudaMemGetInfo(&free_bytes, &total_bytes);

    if (memory_status == cudaSuccess) {
        set_available(
            snapshot.memory_total_bytes,
            static_cast<std::uint64_t>(total_bytes),
            CudaHealthSource::cuda_runtime);
        set_available(
            snapshot.memory_free_bytes,
            static_cast<std::uint64_t>(free_bytes),
            CudaHealthSource::cuda_runtime);
        set_available(
            snapshot.memory_used_bytes,
            static_cast<std::uint64_t>(total_bytes - free_bytes),
            CudaHealthSource::cuda_runtime);
    } else {
        const std::string message = make_cuda_error_message(
            memory_status,
            "cudaMemGetInfo");
        append_issue(
            snapshot,
            CudaHealthIssueCode::cuda_memory_query_failed,
            static_cast<int>(memory_status),
            false,
            message);
        set_metric_failure(
            snapshot.memory_total_bytes,
            CudaHealthMetricState::query_failed,
            CudaHealthSource::cuda_runtime,
            static_cast<int>(memory_status),
            message);
        set_metric_failure(
            snapshot.memory_free_bytes,
            CudaHealthMetricState::query_failed,
            CudaHealthSource::cuda_runtime,
            static_cast<int>(memory_status),
            message);
        set_metric_failure(
            snapshot.memory_used_bytes,
            CudaHealthMetricState::query_failed,
            CudaHealthSource::cuda_runtime,
            static_cast<int>(memory_status),
            message);
    }

    if (switch_required) {
        const cudaError_t restore_status = cudaSetDevice(previous_device);
        if (restore_status != cudaSuccess) {
            append_issue(
                snapshot,
                CudaHealthIssueCode::current_device_restore_failed,
                static_cast<int>(restore_status),
                true,
                make_cuda_error_message(
                    restore_status,
                    "cudaSetDevice while restoring the previous device"));
        }
    }
}

[[nodiscard]] CudaHealthQueryStatus finalize_status(
    const CudaDeviceHealthSnapshot& snapshot) {
    CudaHealthQueryStatus status{};

    if (snapshot.runtime_binding ==
        CudaRuntimeBindingState::identity_mismatch) {
        status.code = CudaHealthQueryCode::identity_mismatch;
        status.message =
            "The stored CUDA runtime ordinal no longer resolves to the stored UUID";
        return status;
    }

    if (snapshot.runtime_binding ==
            CudaRuntimeBindingState::ordinal_unavailable ||
        snapshot.runtime_binding ==
            CudaRuntimeBindingState::query_failed) {
        if (snapshot.nvml_provider == CudaNvmlProviderState::ready) {
            status.code = CudaHealthQueryCode::partial_success;
            status.message =
                "Stable-UUID telemetry is available, but the CUDA runtime binding is not usable";
        } else {
            status.code = CudaHealthQueryCode::query_failed;
            status.message =
                "Neither a usable CUDA runtime binding nor NVML telemetry is available";
        }
        return status;
    }

    if (snapshot.nvml_provider == CudaNvmlProviderState::device_lost) {
        status.code = CudaHealthQueryCode::query_failed;
        status.message = "NVML reports that the GPU is lost";
        return status;
    }

    const bool serious_issue = [&snapshot]() {
        for (const CudaHealthIssue& issue : snapshot.issues) {
            if (issue.execution_binding_fatal ||
                issue.code ==
                    CudaHealthIssueCode::nvml_metric_query_failed ||
                issue.code ==
                    CudaHealthIssueCode::cuda_memory_query_failed ||
                issue.code ==
                    CudaHealthIssueCode::current_device_restore_failed) {
                return true;
            }
        }
        return false;
    }();

    if (serious_issue) {
        status.code = CudaHealthQueryCode::partial_success;
        status.message =
            "CUDA health sampling completed with one or more query failures";
    }
    return status;
}

}  // namespace

CudaHealthQueryResult query_cuda_device_health(
    const CudaDeviceIdentity& identity,
    const CudaHealthQueryOptions& options) {
    CudaHealthQueryResult result{};
    CudaDeviceHealthSnapshot& snapshot = result.snapshot;

    snapshot.sampled_at_unix_ns = current_unix_time_ns();
    snapshot.persistent_key = identity.persistent_key;
    snapshot.uuid = identity.uuid;
    snapshot.requested_runtime_ordinal = identity.runtime_ordinal;

    if (!identity.has_stable_identity()) {
        result.status.code = CudaHealthQueryCode::invalid_identity;
        result.status.message =
            "CUDA health queries require a durable UUID identity";
        append_issue(
            snapshot,
            CudaHealthIssueCode::invalid_stable_identity,
            0,
            true,
            result.status.message);
        return result;
    }

    if (identity.runtime_ordinal < 0) {
        snapshot.runtime_binding =
            CudaRuntimeBindingState::ordinal_unavailable;
        append_issue(
            snapshot,
            CudaHealthIssueCode::runtime_identity_query_failed,
            0,
            true,
            "The discovered device no longer has a usable CUDA runtime ordinal");
    } else {
        const CudaIdentityQueryResult current_identity =
            query_cuda_device_identity(identity.runtime_ordinal);
        if (!current_identity.status.completed()) {
            snapshot.runtime_binding =
                current_identity.status.code ==
                        CudaIdentityQueryCode::invalid_device_ordinal
                    ? CudaRuntimeBindingState::ordinal_unavailable
                    : CudaRuntimeBindingState::query_failed;
            append_issue(
                snapshot,
                CudaHealthIssueCode::runtime_identity_query_failed,
                current_identity.status.native_error_code,
                true,
                current_identity.status.message.empty()
                    ? "CUDA runtime identity revalidation failed"
                    : current_identity.status.message);
        } else if (!current_identity.identity.has_stable_identity() ||
                   current_identity.identity.uuid != identity.uuid) {
            snapshot.runtime_binding =
                CudaRuntimeBindingState::identity_mismatch;
            std::string message =
                "CUDA ordinal identity changed: expected ";
            message += identity.uuid;
            message += ", observed ";
            message += current_identity.identity.uuid.empty()
                           ? "<unavailable>"
                           : current_identity.identity.uuid;
            append_issue(
                snapshot,
                CudaHealthIssueCode::runtime_identity_mismatch,
                current_identity.status.native_error_code,
                true,
                std::move(message));
        } else {
            snapshot.runtime_binding = CudaRuntimeBindingState::matched;
        }
    }

    if (options.query_nvml) {
        query_nvml_metrics(identity, snapshot);
    } else {
        snapshot.nvml_provider = CudaNvmlProviderState::not_requested;
    }

    if (options.allow_cuda_memory_fallback &&
        !memory_metrics_available(snapshot)) {
        query_cuda_memory_fallback(identity, snapshot);
    }

    result.status = finalize_status(snapshot);
    return result;
}

const char* to_string(CudaHealthMetricState value) noexcept {
    switch (value) {
        case CudaHealthMetricState::unknown:
            return "unknown";
        case CudaHealthMetricState::available:
            return "available";
        case CudaHealthMetricState::unsupported:
            return "unsupported";
        case CudaHealthMetricState::permission_denied:
            return "permission_denied";
        case CudaHealthMetricState::provider_unavailable:
            return "provider_unavailable";
        case CudaHealthMetricState::query_failed:
            return "query_failed";
    }
    return "unknown";
}

const char* to_string(CudaHealthSource value) noexcept {
    switch (value) {
        case CudaHealthSource::none:
            return "none";
        case CudaHealthSource::cuda_runtime:
            return "cuda_runtime";
        case CudaHealthSource::nvml:
            return "nvml";
    }
    return "unknown";
}

const char* to_string(CudaRuntimeBindingState value) noexcept {
    switch (value) {
        case CudaRuntimeBindingState::unknown:
            return "unknown";
        case CudaRuntimeBindingState::matched:
            return "matched";
        case CudaRuntimeBindingState::identity_mismatch:
            return "identity_mismatch";
        case CudaRuntimeBindingState::ordinal_unavailable:
            return "ordinal_unavailable";
        case CudaRuntimeBindingState::query_failed:
            return "query_failed";
    }
    return "unknown";
}

const char* to_string(CudaNvmlProviderState value) noexcept {
    switch (value) {
        case CudaNvmlProviderState::not_requested:
            return "not_requested";
        case CudaNvmlProviderState::ready:
            return "ready";
        case CudaNvmlProviderState::library_unavailable:
            return "library_unavailable";
        case CudaNvmlProviderState::initialization_failed:
            return "initialization_failed";
        case CudaNvmlProviderState::device_not_found:
            return "device_not_found";
        case CudaNvmlProviderState::device_lost:
            return "device_lost";
    }
    return "unknown";
}

const char* to_string(CudaHealthIssueCode value) noexcept {
    switch (value) {
        case CudaHealthIssueCode::invalid_stable_identity:
            return "invalid_stable_identity";
        case CudaHealthIssueCode::runtime_identity_query_failed:
            return "runtime_identity_query_failed";
        case CudaHealthIssueCode::runtime_identity_mismatch:
            return "runtime_identity_mismatch";
        case CudaHealthIssueCode::nvml_library_unavailable:
            return "nvml_library_unavailable";
        case CudaHealthIssueCode::nvml_initialization_failed:
            return "nvml_initialization_failed";
        case CudaHealthIssueCode::nvml_device_lookup_failed:
            return "nvml_device_lookup_failed";
        case CudaHealthIssueCode::nvml_metric_query_failed:
            return "nvml_metric_query_failed";
        case CudaHealthIssueCode::cuda_memory_query_failed:
            return "cuda_memory_query_failed";
        case CudaHealthIssueCode::current_device_query_failed:
            return "current_device_query_failed";
        case CudaHealthIssueCode::current_device_restore_failed:
            return "current_device_restore_failed";
    }
    return "unknown";
}

const char* to_string(CudaHealthQueryCode value) noexcept {
    switch (value) {
        case CudaHealthQueryCode::success:
            return "success";
        case CudaHealthQueryCode::partial_success:
            return "partial_success";
        case CudaHealthQueryCode::invalid_identity:
            return "invalid_identity";
        case CudaHealthQueryCode::identity_mismatch:
            return "identity_mismatch";
        case CudaHealthQueryCode::query_failed:
            return "query_failed";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cuda
