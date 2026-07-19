#ifndef SRC_BACKENDS_CPU_CPU_THREAD_POOL_HPP
#define SRC_BACKENDS_CPU_CPU_THREAD_POOL_HPP

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace prometheus::backends::cpu {

using CpuTaskId = std::uint64_t;
using CpuTaskFunction = std::function<void()>;

enum class CpuTaskPriority : std::uint8_t {
    low = 0,
    normal,
    high,
    critical,
};

enum class CpuTaskState : std::uint8_t {
    invalid = 0,
    queued,
    running,
    completed,
    failed,
    cancelled,
};

enum class CpuTaskSubmissionCode : std::uint8_t {
    accepted = 0,
    invalid_task,
    pool_not_running,
    queue_full,
    pool_stopping,
    resource_exhausted,
};

enum class CpuThreadPoolState : std::uint8_t {
    constructed = 0,
    starting,
    running,
    draining,
    stopping,
    stopped,
    failed,
};

enum class CpuThreadPoolStopMode : std::uint8_t {
    drain = 0,
    cancel_pending,
};

enum class CpuThreadPoolShutdownResult : std::uint8_t {
    request_accepted = 0,
    already_requested,
    escalated,
    fully_stopped,
    timed_out,
    called_from_worker,
    invalid_timeout,
    failed_lifecycle,
};

struct CpuTaskResult {
    CpuTaskId task_id = 0;
    CpuTaskState state = CpuTaskState::invalid;
    std::string error_message{};

    [[nodiscard]] bool terminal() const noexcept {
        return state == CpuTaskState::completed ||
               state == CpuTaskState::failed ||
               state == CpuTaskState::cancelled;
    }

    [[nodiscard]] bool succeeded() const noexcept {
        return state == CpuTaskState::completed;
    }
};

namespace detail {
struct CpuTaskControl;
}  // namespace detail

class CpuTaskHandle {
public:
    CpuTaskHandle() = default;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] CpuTaskId task_id() const noexcept;
    [[nodiscard]] CpuTaskState state() const;
    [[nodiscard]] bool cancellation_requested() const;

    // Cancellation is cooperative only before execution starts. Running tasks
    // are never forcefully interrupted.
    [[nodiscard]] bool request_cancel() const;

    void wait() const;
    [[nodiscard]] bool wait_for(std::chrono::nanoseconds timeout) const;
    [[nodiscard]] CpuTaskResult result() const;

private:
    explicit CpuTaskHandle(
        std::shared_ptr<detail::CpuTaskControl> control);

    std::shared_ptr<detail::CpuTaskControl> control_{};

    friend class CpuThreadPool;
};

struct CpuTaskSubmissionResult {
    CpuTaskSubmissionCode code = CpuTaskSubmissionCode::pool_not_running;
    CpuTaskHandle handle{};
    std::string message{};

    [[nodiscard]] bool accepted() const noexcept {
        return code == CpuTaskSubmissionCode::accepted;
    }
};

struct CpuThreadPoolOptions {
    std::size_t worker_count = 1;
    std::size_t queue_capacity = 256;

    // Phase 7.2.1 records execution-group metadata but does not alter thread
    // affinity. Binding policy belongs to a later Phase 7.2 slice.
    std::string execution_group_key{};
    std::vector<int> advisory_logical_processor_ids{};
};

struct CpuThreadPoolStatus {
    CpuThreadPoolState state = CpuThreadPoolState::constructed;
    std::string message{};

    [[nodiscard]] bool accepting_work() const noexcept {
        return state == CpuThreadPoolState::running;
    }
};

struct CpuThreadPoolSnapshot {
    CpuThreadPoolState state = CpuThreadPoolState::constructed;
    std::string execution_group_key{};
    std::vector<int> advisory_logical_processor_ids{};

    std::size_t configured_worker_count = 0;
    std::size_t live_worker_count = 0;
    std::size_t queue_capacity = 0;
    std::size_t queued_task_count = 0;
    std::size_t active_task_count = 0;

    std::uint64_t accepted_task_count = 0;
    std::uint64_t rejected_task_count = 0;
    std::uint64_t started_task_count = 0;
    std::uint64_t completed_task_count = 0;
    std::uint64_t failed_task_count = 0;
    std::uint64_t cancelled_task_count = 0;

    std::string message{};

    [[nodiscard]] bool accepting_work() const noexcept {
        return state == CpuThreadPoolState::running;
    }

    [[nodiscard]] bool quiescent() const noexcept {
        return queued_task_count == 0 &&
               active_task_count == 0;
    }

    [[nodiscard]] bool fully_stopped() const noexcept {
        return state == CpuThreadPoolState::stopped &&
               live_worker_count == 0 &&
               quiescent();
    }
};

// Phase 7.2.1 provides one bounded worker pool for one logical execution
// group. It does not pin threads, steal work between NUMA groups, dispatch
// kernels, or encode neural-current semantics.
class CpuThreadPool {
public:
    explicit CpuThreadPool(CpuThreadPoolOptions options = {});
    ~CpuThreadPool();

    CpuThreadPool(const CpuThreadPool&) = delete;
    CpuThreadPool& operator=(const CpuThreadPool&) = delete;
    CpuThreadPool(CpuThreadPool&&) = delete;
    CpuThreadPool& operator=(CpuThreadPool&&) = delete;

    // A stopped pool may be started again. Structured counters remain
    // cumulative across restarts.
    [[nodiscard]] CpuThreadPoolStatus start();

    [[nodiscard]] CpuTaskSubmissionResult submit(
        CpuTaskFunction task,
        CpuTaskPriority priority = CpuTaskPriority::normal);

    // Stops new task acceptance and publishes the selected shutdown policy
    // without waiting for running work or joining worker threads.
    [[nodiscard]] CpuThreadPoolShutdownResult request_shutdown(
        CpuThreadPoolStopMode mode);

    // Observes and finalizes a requested shutdown within a finite interval.
    // A zero timeout is an immediate observation. Negative timeouts are
    // rejected. Worker tasks cannot wait for their own pool.
    [[nodiscard]] CpuThreadPoolShutdownResult wait_for_shutdown(
        std::chrono::nanoseconds timeout);

    // Stops accepting new work, completes queued/running work, joins workers,
    // and leaves the pool stopped.
    [[nodiscard]] CpuThreadPoolStatus drain();

    // Running tasks are allowed to finish. cancel_pending additionally marks
    // queued tasks cancelled before workers are joined.
    [[nodiscard]] CpuThreadPoolStatus stop(
        CpuThreadPoolStopMode mode =
            CpuThreadPoolStopMode::cancel_pending);

    [[nodiscard]] CpuThreadPoolStatus status() const;
    [[nodiscard]] CpuThreadPoolSnapshot snapshot() const;

private:
    struct Impl;
    // Workers retain the implementation while exiting so destruction from a
    // worker task cannot invalidate state still used by that worker.
    std::shared_ptr<Impl> impl_;
};

[[nodiscard]] const char* to_string(CpuTaskPriority value) noexcept;
[[nodiscard]] const char* to_string(CpuTaskState value) noexcept;
[[nodiscard]] const char* to_string(CpuTaskSubmissionCode value) noexcept;
[[nodiscard]] const char* to_string(CpuThreadPoolState value) noexcept;
[[nodiscard]] const char* to_string(CpuThreadPoolStopMode value) noexcept;
[[nodiscard]] const char* to_string(
    CpuThreadPoolShutdownResult value) noexcept;

}  // namespace prometheus::backends::cpu

#endif  // SRC_BACKENDS_CPU_CPU_THREAD_POOL_HPP
