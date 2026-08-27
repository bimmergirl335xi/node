#ifndef SRC_BACKENDS_CUDA_CUDA_DEVICE_WORKER_HPP
#define SRC_BACKENDS_CUDA_CUDA_DEVICE_WORKER_HPP

#include <cuda_runtime_api.h>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "cuda_device_pool.hpp"
#include "cuda_kernel_adapter.hpp"

namespace prometheus::backends::cuda {

inline constexpr std::size_t kCudaWorkerIssueCountAbsoluteMaximum = 128;
inline constexpr std::size_t kCudaWorkerIssueMessageAbsoluteMaximum = 1024;
inline constexpr std::size_t kCudaWorkerDeviceKeyAbsoluteMaximum = 256;
inline constexpr std::size_t kCudaWorkerJobIdentifierAbsoluteMaximum = 1024;
inline constexpr std::int64_t kCudaWorkerWaitAbsoluteMaximumMs = 300000;

enum class CudaDeviceWorkerState : std::uint8_t {
    constructed = 0,
    starting,
    idle,
    executing,
    stopping,
    stopped,
    failed,
};

enum class CudaDeviceWorkerCode : std::uint8_t {
    success = 0,
    invalid_worker_configuration,
    invalid_state,
    unknown_device,
    device_not_registration_ready,
    runtime_binding_unusable,
    cuda_set_device_failure,
    stream_creation_failure,
    event_creation_failure,
    adapter_registration_failure,
    adapter_initialization_failure,
    adapter_not_found,
    metadata_not_executable,
    mark_running_failure,
    launch_failure,
    event_record_failure,
    completion_failure,
    output_validation_failure,
    queue_terminal_transition_failure,
    adapter_shutdown_failure,
    event_destroy_failure,
    stream_destroy_failure,
    wrong_device,
    not_accepting,
    queue_rejected,
    worker_stopped,
    timeout,
    resource_exhausted,
};

struct CudaDeviceWorkerIssue {
    CudaDeviceWorkerCode code = CudaDeviceWorkerCode::success;
    int native_error_code = 0;
    bool worker_fatal = false;
    std::string job_id{};
    std::string message{};
};

struct CudaDeviceWorkerResult {
    CudaDeviceWorkerCode code = CudaDeviceWorkerCode::success;
    CudaJobQueueCode queue_code = CudaJobQueueCode::success;
    int native_error_code = 0;
    std::string message{};
    bool has_job = false;
    CudaJobRecord job{};

    [[nodiscard]] bool ok() const noexcept {
        return code == CudaDeviceWorkerCode::success;
    }
};

struct CudaDeviceWorkerOptions {
    CudaKernelAdapterRegistryOptions adapter_registry{};
    std::size_t maximum_retained_issues = 32;
    std::size_t maximum_issue_message_bytes = 256;
    std::size_t maximum_device_key_bytes = 128;
    std::size_t maximum_job_identifier_bytes = 256;
};

struct CudaDeviceWorkerSnapshot {
    std::string persistent_device_key{};
    int runtime_ordinal_snapshot = -1;
    CudaDeviceWorkerState state = CudaDeviceWorkerState::constructed;
    bool accepting_submissions = false;
    bool stream_created = false;
    bool event_created = false;
    bool stream_destroyed = true;
    bool event_destroyed = true;
    bool adapter_resources_destroyed = true;
    bool thread_joined = true;
    bool waiting_for_work = false;
    std::size_t adapter_count = 0;
    std::uint64_t notification_generation = 0;
    std::uint64_t claimed_job_count = 0;
    std::uint64_t started_job_count = 0;
    std::uint64_t completed_job_count = 0;
    std::uint64_t failed_job_count = 0;
    bool has_current_job = false;
    std::string current_job_id{};
    bool has_last_issue = false;
    CudaDeviceWorkerIssue last_issue{};
    std::vector<CudaDeviceWorkerIssue> retained_issues{};
};

// One explicitly constructed worker owns one thread, one non-blocking stream,
// one timing-disabled event, and one device-local adapter registry. The pool is
// copied as immutable evidence. The queue and metadata registry are borrowed
// and must outlive the worker.
class CudaDeviceWorker {
public:
    CudaDeviceWorker(
        std::string persistent_device_key,
        CudaDevicePool device_pool_snapshot,
        CudaJobQueue& queue,
        const CudaKernelRegistry& metadata_registry,
        CudaDeviceWorkerOptions options = {});
    ~CudaDeviceWorker() noexcept;

    CudaDeviceWorker(const CudaDeviceWorker&) = delete;
    CudaDeviceWorker& operator=(const CudaDeviceWorker&) = delete;
    CudaDeviceWorker(CudaDeviceWorker&&) = delete;
    CudaDeviceWorker& operator=(CudaDeviceWorker&&) = delete;

    [[nodiscard]] CudaDeviceWorkerResult register_adapter(
        std::unique_ptr<CudaKernelLaunchAdapter> adapter) noexcept;
    [[nodiscard]] CudaDeviceWorkerResult start() noexcept;
    [[nodiscard]] CudaDeviceWorkerResult stop() noexcept;

    [[nodiscard]] CudaDeviceWorkerResult submit(
        CudaJobDescriptor descriptor) noexcept;

    [[nodiscard]] CudaDeviceWorkerResult wait_for_terminal(
        const std::string& job_id,
        std::chrono::milliseconds timeout) noexcept;

    [[nodiscard]] CudaDeviceWorkerResult wait_until_idle(
        std::chrono::milliseconds timeout) noexcept;

    [[nodiscard]] CudaDeviceWorkerSnapshot snapshot() const noexcept;

private:
    [[nodiscard]] bool options_valid() const noexcept;
    [[nodiscard]] CudaDeviceWorkerResult make_result(
        CudaDeviceWorkerCode code,
        std::string message,
        int native_error_code = 0,
        CudaJobQueueCode queue_code = CudaJobQueueCode::success) const noexcept;
    void thread_main(int runtime_ordinal_snapshot) noexcept;
    [[nodiscard]] bool execute_claimed_job(const CudaJobRecord& job) noexcept;
    [[nodiscard]] bool fail_running_job(
        const CudaJobRecord& job,
        CudaDeviceWorkerCode code,
        std::string message,
        int native_error_code,
        bool worker_fatal) noexcept;
    void publish_issue(
        CudaDeviceWorkerCode code,
        std::string job_id,
        std::string message,
        int native_error_code,
        bool worker_fatal) noexcept;
    void publish_start_failure(
        CudaDeviceWorkerCode code,
        std::string message,
        int native_error_code) noexcept;
    void cleanup_device_resources(bool worker_failed) noexcept;
    void notify_terminal() noexcept;
    void clear_current_job(bool worker_failed) noexcept;

    std::string persistent_device_key_{};
    CudaDevicePool device_pool_snapshot_{};
    CudaJobQueue& queue_;
    const CudaKernelRegistry& metadata_registry_;
    CudaDeviceWorkerOptions options_{};
    CudaKernelAdapterRegistry adapter_registry_{};

    mutable std::mutex mutex_{};
    std::condition_variable wake_condition_{};
    std::condition_variable start_condition_{};
    std::condition_variable terminal_condition_{};
    std::condition_variable state_condition_{};
    std::thread worker_thread_{};
    bool thread_joined_ = true;

    CudaDeviceWorkerState state_ = CudaDeviceWorkerState::constructed;
    bool accepting_submissions_ = false;
    bool stop_requested_ = false;
    bool notification_pending_ = false;
    bool waiting_for_work_ = false;
    bool start_completed_ = false;
    CudaDeviceWorkerResult start_result_{};
    int runtime_ordinal_snapshot_ = -1;
    cudaStream_t stream_ = nullptr;
    cudaEvent_t completion_event_ = nullptr;
    bool stream_created_ = false;
    bool event_created_ = false;
    bool stream_destroyed_ = true;
    bool event_destroyed_ = true;
    bool adapter_resources_destroyed_ = true;
    std::uint64_t notification_generation_ = 0;
    std::uint64_t claimed_job_count_ = 0;
    std::uint64_t started_job_count_ = 0;
    std::uint64_t completed_job_count_ = 0;
    std::uint64_t failed_job_count_ = 0;
    std::string current_job_id_{};
    std::vector<CudaDeviceWorkerIssue> issues_{};
};

[[nodiscard]] const char* to_string(CudaDeviceWorkerState value) noexcept;
[[nodiscard]] const char* to_string(CudaDeviceWorkerCode value) noexcept;

}  // namespace prometheus::backends::cuda

#endif  // SRC_BACKENDS_CUDA_CUDA_DEVICE_WORKER_HPP
