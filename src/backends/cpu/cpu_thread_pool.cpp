#include "cpu_thread_pool.hpp"

#include <algorithm>
#include <array>
#include <condition_variable>
#include <deque>
#include <exception>
#include <limits>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace prometheus::backends::cpu {

namespace detail {

struct CpuTaskControl {
    mutable std::mutex mutex{};
    std::condition_variable completion_cv{};
    CpuTaskId task_id = 0;
    CpuTaskState state = CpuTaskState::queued;
    bool cancel_requested = false;
    std::string error_message{};
};

}  // namespace detail

namespace {

[[nodiscard]] bool is_terminal(CpuTaskState state) noexcept {
    return state == CpuTaskState::completed ||
           state == CpuTaskState::failed ||
           state == CpuTaskState::cancelled;
}

[[nodiscard]] std::size_t priority_index(CpuTaskPriority priority) noexcept {
    return static_cast<std::size_t>(priority);
}

void finish_task(
    const std::shared_ptr<detail::CpuTaskControl>& control,
    CpuTaskState state,
    std::string error = {}) {
    {
        std::lock_guard<std::mutex> lock(control->mutex);
        control->state = state;
        control->error_message = std::move(error);
    }
    control->completion_cv.notify_all();
}

}  // namespace

CpuTaskHandle::CpuTaskHandle(
    std::shared_ptr<detail::CpuTaskControl> control)
    : control_(std::move(control)) {}

bool CpuTaskHandle::valid() const noexcept {
    return static_cast<bool>(control_);
}

CpuTaskId CpuTaskHandle::task_id() const noexcept {
    return control_ ? control_->task_id : 0;
}

CpuTaskState CpuTaskHandle::state() const {
    if (!control_) {
        return CpuTaskState::invalid;
    }
    std::lock_guard<std::mutex> lock(control_->mutex);
    return control_->state;
}

bool CpuTaskHandle::cancellation_requested() const {
    if (!control_) {
        return false;
    }
    std::lock_guard<std::mutex> lock(control_->mutex);
    return control_->cancel_requested;
}

bool CpuTaskHandle::request_cancel() const {
    if (!control_) {
        return false;
    }
    std::lock_guard<std::mutex> lock(control_->mutex);
    if (control_->state != CpuTaskState::queued) {
        return false;
    }
    control_->cancel_requested = true;
    return true;
}

void CpuTaskHandle::wait() const {
    if (!control_) {
        return;
    }
    std::unique_lock<std::mutex> lock(control_->mutex);
    control_->completion_cv.wait(
        lock, [this]() { return is_terminal(control_->state); });
}

bool CpuTaskHandle::wait_for(std::chrono::nanoseconds timeout) const {
    if (!control_) {
        return false;
    }
    std::unique_lock<std::mutex> lock(control_->mutex);
    return control_->completion_cv.wait_for(
        lock,
        timeout,
        [this]() { return is_terminal(control_->state); });
}

CpuTaskResult CpuTaskHandle::result() const {
    CpuTaskResult result{};
    if (!control_) {
        return result;
    }
    std::lock_guard<std::mutex> lock(control_->mutex);
    result.task_id = control_->task_id;
    result.state = control_->state;
    result.error_message = control_->error_message;
    return result;
}

struct CpuThreadPool::Impl {
    struct QueuedTask {
        CpuTaskId id = 0;
        CpuTaskFunction function{};
        std::shared_ptr<detail::CpuTaskControl> control{};
    };

    explicit Impl(CpuThreadPoolOptions supplied_options)
        : options(std::move(supplied_options)) {}

    CpuThreadPoolOptions options{};
    std::mutex lifecycle_mutex{};
    mutable std::mutex mutex{};
    std::condition_variable work_cv{};
    std::condition_variable idle_cv{};
    CpuThreadPoolStatus status{};
    std::array<std::deque<QueuedTask>, 4> queues{};
    std::vector<std::thread> workers{};
    std::vector<std::thread::id> worker_ids{};

    CpuTaskId next_task_id = 1;
    std::size_t queued_count = 0;
    std::size_t active_count = 0;
    std::size_t live_workers = 0;
    std::uint64_t accepted = 0;
    std::uint64_t rejected = 0;
    std::uint64_t started = 0;
    std::uint64_t completed = 0;
    std::uint64_t failed = 0;
    std::uint64_t cancelled = 0;
    bool accepting = false;
    bool stop_requested = false;

    [[nodiscard]] bool queues_empty() const noexcept {
        return queued_count == 0;
    }

    [[nodiscard]] QueuedTask pop_next_locked() {
        for (std::size_t index = queues.size(); index > 0; --index) {
            auto& queue = queues[index - 1];
            if (!queue.empty()) {
                QueuedTask task = std::move(queue.front());
                queue.pop_front();
                --queued_count;
                return task;
            }
        }
        return {};
    }

    [[nodiscard]] bool called_from_worker() const {
        const std::thread::id caller = std::this_thread::get_id();
        std::lock_guard<std::mutex> lock(mutex);
        return std::find(worker_ids.begin(), worker_ids.end(), caller) !=
               worker_ids.end();
    }

    void cancel_queued_locked() {
        for (auto& queue : queues) {
            while (!queue.empty()) {
                QueuedTask task = std::move(queue.front());
                queue.pop_front();
                finish_task(task.control, CpuTaskState::cancelled);
                ++cancelled;
            }
        }
        queued_count = 0;
        idle_cv.notify_all();
    }

    void worker_loop() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            ++live_workers;
            worker_ids.push_back(std::this_thread::get_id());
        }

        while (true) {
            QueuedTask task{};
            {
                std::unique_lock<std::mutex> lock(mutex);
                work_cv.wait(
                    lock,
                    [this]() { return stop_requested || !queues_empty(); });
                if (stop_requested && queues_empty()) {
                    break;
                }
                task = pop_next_locked();
                ++active_count;
            }

            bool skip = false;
            {
                std::lock_guard<std::mutex> task_lock(task.control->mutex);
                if (task.control->cancel_requested) {
                    task.control->state = CpuTaskState::cancelled;
                    skip = true;
                } else {
                    task.control->state = CpuTaskState::running;
                }
            }

            if (skip) {
                task.control->completion_cv.notify_all();
                std::lock_guard<std::mutex> lock(mutex);
                --active_count;
                ++cancelled;
                if (queues_empty() && active_count == 0) {
                    idle_cv.notify_all();
                }
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(mutex);
                ++started;
            }

            CpuTaskState final_state = CpuTaskState::completed;
            std::string error{};
            try {
                task.function();
            } catch (const std::exception& exception) {
                final_state = CpuTaskState::failed;
                error = exception.what();
            } catch (...) {
                final_state = CpuTaskState::failed;
                error = "unknown CPU task exception";
            }
            finish_task(task.control, final_state, std::move(error));

            {
                std::lock_guard<std::mutex> lock(mutex);
                --active_count;
                if (final_state == CpuTaskState::completed) {
                    ++completed;
                } else {
                    ++failed;
                }
                if (queues_empty() && active_count == 0) {
                    idle_cv.notify_all();
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(mutex);
            const std::thread::id id = std::this_thread::get_id();
            worker_ids.erase(
                std::remove(worker_ids.begin(), worker_ids.end(), id),
                worker_ids.end());
            --live_workers;
            idle_cv.notify_all();
        }
    }

    void join_workers() {
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        workers.clear();
    }
};

CpuThreadPool::CpuThreadPool(CpuThreadPoolOptions options)
    : impl_(std::make_unique<Impl>(std::move(options))) {}

CpuThreadPool::~CpuThreadPool() {
    static_cast<void>(stop(CpuThreadPoolStopMode::cancel_pending));
}

CpuThreadPoolStatus CpuThreadPool::start() {
    std::lock_guard<std::mutex> lifecycle_lock(impl_->lifecycle_mutex);
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (impl_->status.state == CpuThreadPoolState::running) {
            return impl_->status;
        }
        if (impl_->status.state == CpuThreadPoolState::starting ||
            impl_->status.state == CpuThreadPoolState::draining ||
            impl_->status.state == CpuThreadPoolState::stopping) {
            impl_->status.message = "CPU thread pool lifecycle is busy";
            return impl_->status;
        }
        if (impl_->options.worker_count == 0 ||
            impl_->options.queue_capacity == 0) {
            impl_->status.state = CpuThreadPoolState::failed;
            impl_->status.message =
                "CPU thread pool requires workers and queue capacity";
            return impl_->status;
        }
        impl_->stop_requested = false;
        impl_->accepting = false;
        impl_->status.state = CpuThreadPoolState::starting;
        impl_->status.message = "Starting CPU thread pool";
    }

    try {
        impl_->workers.reserve(impl_->options.worker_count);
        for (std::size_t index = 0;
             index < impl_->options.worker_count;
             ++index) {
            impl_->workers.emplace_back([this]() { impl_->worker_loop(); });
        }
    } catch (const std::exception& exception) {
        {
            std::lock_guard<std::mutex> lock(impl_->mutex);
            impl_->stop_requested = true;
            impl_->status.state = CpuThreadPoolState::failed;
            impl_->status.message = exception.what();
        }
        impl_->work_cv.notify_all();
        impl_->join_workers();
        return status();
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(impl_->mutex);
            impl_->stop_requested = true;
            impl_->status.state = CpuThreadPoolState::failed;
            impl_->status.message = "unknown worker creation failure";
        }
        impl_->work_cv.notify_all();
        impl_->join_workers();
        return status();
    }

    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->accepting = true;
    impl_->status.state = CpuThreadPoolState::running;
    impl_->status.message = "CPU thread pool is accepting work";
    return impl_->status;
}

CpuTaskSubmissionResult CpuThreadPool::submit(
    CpuTaskFunction task,
    CpuTaskPriority priority) {
    CpuTaskSubmissionResult result{};
    const std::size_t selected = priority_index(priority);
    if (!task || selected >= impl_->queues.size()) {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        ++impl_->rejected;
        result.code = CpuTaskSubmissionCode::invalid_task;
        result.message = !task ? "CPU task callable is empty"
                               : "CPU task priority is invalid";
        return result;
    }

    auto control = std::make_shared<detail::CpuTaskControl>();
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (!impl_->accepting ||
            impl_->status.state != CpuThreadPoolState::running) {
            ++impl_->rejected;
            const bool stopping =
                impl_->status.state == CpuThreadPoolState::draining ||
                impl_->status.state == CpuThreadPoolState::stopping;
            result.code = stopping
                              ? CpuTaskSubmissionCode::pool_stopping
                              : CpuTaskSubmissionCode::pool_not_running;
            result.message = stopping
                                 ? "CPU thread pool is no longer accepting work"
                                 : "CPU thread pool is not running";
            return result;
        }
        if (impl_->queued_count >= impl_->options.queue_capacity) {
            ++impl_->rejected;
            result.code = CpuTaskSubmissionCode::queue_full;
            result.message = "CPU thread pool queue is full";
            return result;
        }
        if (impl_->next_task_id ==
            std::numeric_limits<CpuTaskId>::max()) {
            ++impl_->rejected;
            result.code = CpuTaskSubmissionCode::invalid_task;
            result.message = "CPU task identifier space is exhausted";
            return result;
        }

        const CpuTaskId id = impl_->next_task_id++;
        control->task_id = id;
        impl_->queues[selected].push_back(
            Impl::QueuedTask{id, std::move(task), control});
        ++impl_->queued_count;
        ++impl_->accepted;
        result.code = CpuTaskSubmissionCode::accepted;
        result.handle = CpuTaskHandle{control};
        result.message = "CPU task was accepted";
    }
    impl_->work_cv.notify_one();
    return result;
}

CpuThreadPoolStatus CpuThreadPool::drain() {
    if (impl_->called_from_worker()) {
        CpuThreadPoolStatus current = status();
        current.message =
            "CPU thread pool cannot be drained from one of its workers";
        return current;
    }

    std::lock_guard<std::mutex> lifecycle_lock(impl_->lifecycle_mutex);
    {
        std::unique_lock<std::mutex> lock(impl_->mutex);
        if (impl_->status.state == CpuThreadPoolState::constructed ||
            impl_->status.state == CpuThreadPoolState::stopped) {
            impl_->accepting = false;
            impl_->status.state = CpuThreadPoolState::stopped;
            impl_->status.message = "CPU thread pool is stopped";
            return impl_->status;
        }
        if (impl_->status.state == CpuThreadPoolState::failed) {
            return impl_->status;
        }
        impl_->accepting = false;
        impl_->status.state = CpuThreadPoolState::draining;
        impl_->status.message = "Draining queued CPU work";
        impl_->idle_cv.wait(
            lock,
            [this]() {
                return impl_->queues_empty() && impl_->active_count == 0;
            });
        impl_->stop_requested = true;
    }
    impl_->work_cv.notify_all();
    impl_->join_workers();

    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->status.state = CpuThreadPoolState::stopped;
    impl_->status.message = "CPU thread pool drained and stopped";
    return impl_->status;
}

CpuThreadPoolStatus CpuThreadPool::stop(CpuThreadPoolStopMode mode) {
    if (impl_->called_from_worker()) {
        CpuThreadPoolStatus current = status();
        current.message =
            "CPU thread pool cannot be stopped from one of its workers";
        return current;
    }

    std::lock_guard<std::mutex> lifecycle_lock(impl_->lifecycle_mutex);
    {
        std::unique_lock<std::mutex> lock(impl_->mutex);
        if (impl_->status.state == CpuThreadPoolState::constructed ||
            impl_->status.state == CpuThreadPoolState::stopped) {
            impl_->accepting = false;
            impl_->status.state = CpuThreadPoolState::stopped;
            impl_->status.message = "CPU thread pool is stopped";
            return impl_->status;
        }
        impl_->accepting = false;
        impl_->status.state =
            mode == CpuThreadPoolStopMode::drain
                ? CpuThreadPoolState::draining
                : CpuThreadPoolState::stopping;
        impl_->status.message =
            mode == CpuThreadPoolStopMode::drain
                ? "Draining queued CPU work"
                : "Cancelling queued CPU work";

        if (mode == CpuThreadPoolStopMode::cancel_pending) {
            impl_->cancel_queued_locked();
        } else {
            impl_->idle_cv.wait(
                lock,
                [this]() {
                    return impl_->queues_empty() &&
                           impl_->active_count == 0;
                });
        }
        impl_->stop_requested = true;
    }
    impl_->work_cv.notify_all();
    impl_->join_workers();

    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->status.state = CpuThreadPoolState::stopped;
    impl_->status.message =
        mode == CpuThreadPoolStopMode::drain
            ? "CPU thread pool drained and stopped"
            : "CPU thread pool stopped";
    return impl_->status;
}

CpuThreadPoolStatus CpuThreadPool::status() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->status;
}

CpuThreadPoolSnapshot CpuThreadPool::snapshot() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    CpuThreadPoolSnapshot result{};
    result.state = impl_->status.state;
    result.execution_group_key = impl_->options.execution_group_key;
    result.advisory_logical_processor_ids =
        impl_->options.advisory_logical_processor_ids;
    result.configured_worker_count = impl_->options.worker_count;
    result.live_worker_count = impl_->live_workers;
    result.queue_capacity = impl_->options.queue_capacity;
    result.queued_task_count = impl_->queued_count;
    result.active_task_count = impl_->active_count;
    result.accepted_task_count = impl_->accepted;
    result.rejected_task_count = impl_->rejected;
    result.started_task_count = impl_->started;
    result.completed_task_count = impl_->completed;
    result.failed_task_count = impl_->failed;
    result.cancelled_task_count = impl_->cancelled;
    result.message = impl_->status.message;
    return result;
}

const char* to_string(CpuTaskPriority value) noexcept {
    switch (value) {
        case CpuTaskPriority::low: return "low";
        case CpuTaskPriority::normal: return "normal";
        case CpuTaskPriority::high: return "high";
        case CpuTaskPriority::critical: return "critical";
    }
    return "unknown";
}

const char* to_string(CpuTaskState value) noexcept {
    switch (value) {
        case CpuTaskState::invalid: return "invalid";
        case CpuTaskState::queued: return "queued";
        case CpuTaskState::running: return "running";
        case CpuTaskState::completed: return "completed";
        case CpuTaskState::failed: return "failed";
        case CpuTaskState::cancelled: return "cancelled";
    }
    return "unknown";
}

const char* to_string(CpuTaskSubmissionCode value) noexcept {
    switch (value) {
        case CpuTaskSubmissionCode::accepted: return "accepted";
        case CpuTaskSubmissionCode::invalid_task: return "invalid_task";
        case CpuTaskSubmissionCode::pool_not_running:
            return "pool_not_running";
        case CpuTaskSubmissionCode::queue_full: return "queue_full";
        case CpuTaskSubmissionCode::pool_stopping: return "pool_stopping";
    }
    return "unknown";
}

const char* to_string(CpuThreadPoolState value) noexcept {
    switch (value) {
        case CpuThreadPoolState::constructed: return "constructed";
        case CpuThreadPoolState::starting: return "starting";
        case CpuThreadPoolState::running: return "running";
        case CpuThreadPoolState::draining: return "draining";
        case CpuThreadPoolState::stopping: return "stopping";
        case CpuThreadPoolState::stopped: return "stopped";
        case CpuThreadPoolState::failed: return "failed";
    }
    return "unknown";
}

const char* to_string(CpuThreadPoolStopMode value) noexcept {
    switch (value) {
        case CpuThreadPoolStopMode::drain: return "drain";
        case CpuThreadPoolStopMode::cancel_pending:
            return "cancel_pending";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cpu
