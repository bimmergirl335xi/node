#include "cpu_thread_pool.hpp"

#include <algorithm>
#include <array>
#include <condition_variable>
#include <deque>
#include <exception>
#include <limits>
#include <new>
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

[[nodiscard]] bool valid_stop_mode(
    CpuThreadPoolStopMode mode) noexcept {
    switch (mode) {
        case CpuThreadPoolStopMode::drain:
        case CpuThreadPoolStopMode::cancel_pending:
            return true;
    }
    return false;
}

void assign_message_noexcept(
    std::string& destination,
    const char* message) noexcept {
    try {
        destination = message;
    } catch (const std::bad_alloc&) {
        destination.clear();
    }
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
    std::condition_variable shutdown_cv{};
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
    bool shutdown_requested = false;
    CpuThreadPoolStopMode shutdown_mode = CpuThreadPoolStopMode::drain;
    bool finalization_in_progress = false;
    std::size_t priority_schedule_cursor = 0;

    [[nodiscard]] bool queues_empty() const noexcept {
        return queued_count == 0;
    }

    [[nodiscard]] QueuedTask pop_next_locked() {
        // Weighted service retains priority preference while bounding the
        // number of selections for which any non-empty queue can be skipped.
        static constexpr std::array<std::size_t, 15> schedule{
            3, 3, 3, 3, 3, 3, 3, 3,
            2, 2, 2, 2,
            1, 1,
            0,
        };

        for (std::size_t offset = 0; offset < schedule.size(); ++offset) {
            const std::size_t cursor =
                (priority_schedule_cursor + offset) % schedule.size();
            auto& queue = queues[schedule[cursor]];
            if (!queue.empty()) {
                QueuedTask task = std::move(queue.front());
                queue.pop_front();
                --queued_count;
                priority_schedule_cursor = (cursor + 1) % schedule.size();
                return task;
            }
        }
        return {};
    }

    void purge_cancelled_queued_locked() {
        for (auto& queue : queues) {
            auto iterator = queue.begin();
            while (iterator != queue.end()) {
                bool remove = false;
                {
                    std::lock_guard<std::mutex> task_lock(
                        iterator->control->mutex);
                    if (iterator->control->state == CpuTaskState::queued &&
                        iterator->control->cancel_requested) {
                        iterator->control->state = CpuTaskState::cancelled;
                        remove = true;
                    }
                }

                if (!remove) {
                    ++iterator;
                    continue;
                }

                iterator->control->completion_cv.notify_all();
                iterator = queue.erase(iterator);
                --queued_count;
                ++cancelled;
            }
        }
        if (queues_empty() && active_count == 0) {
            idle_cv.notify_all();
        }
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
            shutdown_cv.notify_all();
        }
    }

    void join_workers() {
        const std::thread::id caller = std::this_thread::get_id();
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                if (worker.get_id() == caller) {
                    worker.detach();
                } else {
                    worker.join();
                }
            }
        }
        workers.clear();
    }
};

CpuThreadPool::CpuThreadPool(CpuThreadPoolOptions options)
    : impl_(std::make_shared<Impl>(std::move(options))) {}

CpuThreadPool::~CpuThreadPool() {
    const std::shared_ptr<Impl> implementation = impl_;
    if (!implementation->called_from_worker()) {
        static_cast<void>(stop(CpuThreadPoolStopMode::cancel_pending));
        return;
    }

    std::lock_guard<std::mutex> lifecycle_lock(
        implementation->lifecycle_mutex);
    {
        std::lock_guard<std::mutex> lock(implementation->mutex);
        implementation->accepting = false;
        implementation->shutdown_requested = true;
        implementation->shutdown_mode =
            CpuThreadPoolStopMode::cancel_pending;
        implementation->status.state = CpuThreadPoolState::stopping;
        implementation->status.message =
            "CPU thread pool is stopping during worker-origin destruction";
        implementation->cancel_queued_locked();
        implementation->stop_requested = true;
    }
    implementation->work_cv.notify_all();
    implementation->join_workers();
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
        if (!impl_->workers.empty() || impl_->live_workers != 0 ||
            impl_->finalization_in_progress) {
            impl_->status.message =
                "CPU thread pool worker generation is not finalized";
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
        impl_->shutdown_requested = false;
        impl_->shutdown_mode = CpuThreadPoolStopMode::drain;
        impl_->finalization_in_progress = false;
        impl_->accepting = false;
        impl_->priority_schedule_cursor = 0;
        impl_->status.state = CpuThreadPoolState::starting;
        impl_->status.message = "Starting CPU thread pool";
    }

    try {
        impl_->workers.reserve(impl_->options.worker_count);
        {
            std::lock_guard<std::mutex> lock(impl_->mutex);
            impl_->worker_ids.reserve(impl_->options.worker_count);
        }
        for (std::size_t index = 0;
             index < impl_->options.worker_count;
             ++index) {
            const std::shared_ptr<Impl> implementation = impl_;
            {
                std::lock_guard<std::mutex> lock(impl_->mutex);
                ++impl_->live_workers;
            }
            try {
                impl_->workers.emplace_back(
                    [implementation]() { implementation->worker_loop(); });
            } catch (...) {
                {
                    std::lock_guard<std::mutex> lock(impl_->mutex);
                    --impl_->live_workers;
                }
                impl_->shutdown_cv.notify_all();
                throw;
            }
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
        assign_message_noexcept(
            result.message,
            !task ? "CPU task callable is empty"
                  : "CPU task priority is invalid");
        return result;
    }

    std::shared_ptr<detail::CpuTaskControl> control{};
    try {
        control = std::make_shared<detail::CpuTaskControl>();
    } catch (const std::bad_alloc&) {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        ++impl_->rejected;
        result.code = CpuTaskSubmissionCode::resource_exhausted;
        assign_message_noexcept(
            result.message,
            "CPU task control allocation failed");
        return result;
    }
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
            assign_message_noexcept(
                result.message,
                stopping
                    ? "CPU thread pool is no longer accepting work"
                    : "CPU thread pool is not running");
            return result;
        }
        impl_->purge_cancelled_queued_locked();
        if (impl_->queued_count >= impl_->options.queue_capacity) {
            ++impl_->rejected;
            result.code = CpuTaskSubmissionCode::queue_full;
            assign_message_noexcept(
                result.message,
                "CPU thread pool queue is full");
            return result;
        }
        if (impl_->next_task_id ==
            std::numeric_limits<CpuTaskId>::max()) {
            ++impl_->rejected;
            result.code = CpuTaskSubmissionCode::invalid_task;
            assign_message_noexcept(
                result.message,
                "CPU task identifier space is exhausted");
            return result;
        }

        const CpuTaskId id = impl_->next_task_id;
        control->task_id = id;
        try {
            impl_->queues[selected].push_back(
                Impl::QueuedTask{id, std::move(task), control});
        } catch (const std::bad_alloc&) {
            ++impl_->rejected;
            result.code = CpuTaskSubmissionCode::resource_exhausted;
            assign_message_noexcept(
                result.message,
                "CPU task queue allocation failed");
            return result;
        }
        ++impl_->next_task_id;
        ++impl_->queued_count;
        ++impl_->accepted;
        result.code = CpuTaskSubmissionCode::accepted;
        result.handle = CpuTaskHandle{control};
        assign_message_noexcept(result.message, "CPU task was accepted");
    }
    impl_->work_cv.notify_one();
    return result;
}

CpuThreadPoolShutdownResult CpuThreadPool::request_shutdown(
    CpuThreadPoolStopMode mode) {
    if (!valid_stop_mode(mode)) {
        return CpuThreadPoolShutdownResult::failed_lifecycle;
    }

    CpuThreadPoolShutdownResult result =
        CpuThreadPoolShutdownResult::request_accepted;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (impl_->status.state == CpuThreadPoolState::failed ||
            impl_->status.state == CpuThreadPoolState::starting) {
            return CpuThreadPoolShutdownResult::failed_lifecycle;
        }
        if (impl_->status.state == CpuThreadPoolState::stopped) {
            impl_->accepting = false;
            impl_->stop_requested = true;
            impl_->status.message = "CPU thread pool is stopped";
            return CpuThreadPoolShutdownResult::fully_stopped;
        }
        if (impl_->status.state == CpuThreadPoolState::constructed) {
            impl_->accepting = false;
            impl_->stop_requested = true;
            impl_->shutdown_requested = true;
            impl_->shutdown_mode = mode;
            impl_->status.state = CpuThreadPoolState::stopped;
            impl_->status.message = "CPU thread pool is stopped";
            return CpuThreadPoolShutdownResult::fully_stopped;
        }

        if (impl_->shutdown_requested) {
            if (impl_->shutdown_mode == CpuThreadPoolStopMode::drain &&
                mode == CpuThreadPoolStopMode::cancel_pending) {
                impl_->shutdown_mode = mode;
                impl_->status.state = CpuThreadPoolState::stopping;
                impl_->status.message = "Cancelling queued CPU work";
                impl_->cancel_queued_locked();
                result = CpuThreadPoolShutdownResult::escalated;
            } else {
                result = CpuThreadPoolShutdownResult::already_requested;
            }
        } else {
            impl_->shutdown_requested = true;
            impl_->shutdown_mode = mode;
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
            }
        }

        impl_->accepting = false;
        impl_->stop_requested = true;
    }
    impl_->work_cv.notify_all();
    return result;
}

CpuThreadPoolShutdownResult CpuThreadPool::wait_for_shutdown(
    std::chrono::nanoseconds timeout) {
    if (impl_->called_from_worker()) {
        return CpuThreadPoolShutdownResult::called_from_worker;
    }
    if (timeout < std::chrono::nanoseconds::zero()) {
        return CpuThreadPoolShutdownResult::invalid_timeout;
    }

    using Clock = std::chrono::steady_clock;
    const Clock::time_point now = Clock::now();
    const Clock::duration remaining = Clock::time_point::max() - now;
    const std::chrono::duration<long double> requested_span{timeout};
    const std::chrono::duration<long double> remaining_span{remaining};
    const Clock::time_point deadline =
        requested_span >= remaining_span
            ? Clock::time_point::max()
            : now + std::chrono::duration_cast<Clock::duration>(timeout);

    std::unique_lock<std::mutex> lock(impl_->mutex);
    if (impl_->status.state == CpuThreadPoolState::failed) {
        return CpuThreadPoolShutdownResult::failed_lifecycle;
    }
    if (impl_->status.state == CpuThreadPoolState::stopped &&
        impl_->workers.empty() && !impl_->finalization_in_progress) {
        return CpuThreadPoolShutdownResult::fully_stopped;
    }
    if (!impl_->shutdown_requested) {
        return CpuThreadPoolShutdownResult::failed_lifecycle;
    }

    while (impl_->live_workers != 0 ||
           impl_->finalization_in_progress) {
        const bool ready = impl_->shutdown_cv.wait_until(
            lock,
            deadline,
            [this]() {
                return impl_->status.state ==
                           CpuThreadPoolState::stopped ||
                       (impl_->live_workers == 0 &&
                        !impl_->finalization_in_progress);
            });
        if (!ready) {
            return CpuThreadPoolShutdownResult::timed_out;
        }
        if (impl_->status.state == CpuThreadPoolState::stopped) {
            return CpuThreadPoolShutdownResult::fully_stopped;
        }
    }

    impl_->finalization_in_progress = true;
    lock.unlock();
    impl_->join_workers();
    lock.lock();

    impl_->finalization_in_progress = false;
    if (impl_->live_workers != 0 || !impl_->workers.empty()) {
        impl_->status.state = CpuThreadPoolState::failed;
        impl_->status.message =
            "CPU thread pool worker finalization failed";
        lock.unlock();
        impl_->shutdown_cv.notify_all();
        return CpuThreadPoolShutdownResult::failed_lifecycle;
    }

    impl_->status.state = CpuThreadPoolState::stopped;
    impl_->status.message =
        impl_->shutdown_mode == CpuThreadPoolStopMode::drain
            ? "CPU thread pool drained and stopped"
            : "CPU thread pool stopped";
    lock.unlock();
    impl_->shutdown_cv.notify_all();
    return CpuThreadPoolShutdownResult::fully_stopped;
}

CpuThreadPoolStatus CpuThreadPool::drain() {
    if (impl_->called_from_worker()) {
        CpuThreadPoolStatus current = status();
        current.message =
            "CPU thread pool cannot be drained from one of its workers";
        return current;
    }

    const CpuThreadPoolShutdownResult requested =
        request_shutdown(CpuThreadPoolStopMode::drain);
    if (requested != CpuThreadPoolShutdownResult::failed_lifecycle &&
        requested != CpuThreadPoolShutdownResult::fully_stopped) {
        static_cast<void>(wait_for_shutdown(
            std::chrono::nanoseconds::max()));
    }
    return status();
}

CpuThreadPoolStatus CpuThreadPool::stop(CpuThreadPoolStopMode mode) {
    if (impl_->called_from_worker()) {
        CpuThreadPoolStatus current = status();
        current.message =
            "CPU thread pool cannot be stopped from one of its workers";
        return current;
    }

    const CpuThreadPoolShutdownResult requested = request_shutdown(mode);
    if (requested != CpuThreadPoolShutdownResult::failed_lifecycle &&
        requested != CpuThreadPoolShutdownResult::fully_stopped) {
        static_cast<void>(wait_for_shutdown(
            std::chrono::nanoseconds::max()));
    }
    return status();
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
        case CpuTaskSubmissionCode::resource_exhausted:
            return "resource_exhausted";
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

const char* to_string(CpuThreadPoolShutdownResult value) noexcept {
    switch (value) {
        case CpuThreadPoolShutdownResult::request_accepted:
            return "request_accepted";
        case CpuThreadPoolShutdownResult::already_requested:
            return "already_requested";
        case CpuThreadPoolShutdownResult::escalated:
            return "escalated";
        case CpuThreadPoolShutdownResult::fully_stopped:
            return "fully_stopped";
        case CpuThreadPoolShutdownResult::timed_out:
            return "timed_out";
        case CpuThreadPoolShutdownResult::called_from_worker:
            return "called_from_worker";
        case CpuThreadPoolShutdownResult::invalid_timeout:
            return "invalid_timeout";
        case CpuThreadPoolShutdownResult::failed_lifecycle:
            return "failed_lifecycle";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cpu
