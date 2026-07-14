#include "cpu_thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace cpu = prometheus::backends::cpu;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "CPU thread-pool test failed: "
                  << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    cpu::CpuThreadPoolOptions options{};
    options.worker_count = 1;
    options.queue_capacity = 2;
    options.execution_group_key = "test:single-group";
    options.advisory_logical_processor_ids = {0};

    cpu::CpuThreadPool pool{options};

    require(
        pool.status().state == cpu::CpuThreadPoolState::constructed,
        "pool must begin constructed");

    const cpu::CpuTaskSubmissionResult before_start =
        pool.submit([]() {});
    require(
        before_start.code ==
            cpu::CpuTaskSubmissionCode::pool_not_running,
        "submission before start must be rejected");

    const cpu::CpuThreadPoolStatus started = pool.start();
    require(
        started.state == cpu::CpuThreadPoolState::running,
        "pool must enter running state");

    const cpu::CpuTaskSubmissionResult empty_task =
        pool.submit(cpu::CpuTaskFunction{});
    require(
        empty_task.code == cpu::CpuTaskSubmissionCode::invalid_task,
        "empty task must be rejected");

    std::mutex gate_mutex;
    std::condition_variable gate_cv;
    bool blocker_started = false;
    bool release_blocker = false;
    std::atomic<int> executed{0};

    const cpu::CpuTaskSubmissionResult blocker = pool.submit(
        [&]() {
            {
                std::lock_guard<std::mutex> lock(gate_mutex);
                blocker_started = true;
            }
            gate_cv.notify_all();

            std::unique_lock<std::mutex> lock(gate_mutex);
            gate_cv.wait(lock, [&]() { return release_blocker; });
            ++executed;
        },
        cpu::CpuTaskPriority::critical);
    require(blocker.accepted(), "blocking task must be accepted");

    {
        std::unique_lock<std::mutex> lock(gate_mutex);
        require(
            gate_cv.wait_for(
                lock,
                std::chrono::seconds(2),
                [&]() { return blocker_started; }),
            "blocking task must begin");
    }

    const cpu::CpuTaskSubmissionResult cancellable = pool.submit(
        [&]() { ++executed; },
        cpu::CpuTaskPriority::low);
    const cpu::CpuTaskSubmissionResult queued = pool.submit(
        [&]() { ++executed; },
        cpu::CpuTaskPriority::normal);
    const cpu::CpuTaskSubmissionResult overflow = pool.submit(
        [&]() { ++executed; },
        cpu::CpuTaskPriority::high);

    require(cancellable.accepted(), "cancellable task must be accepted");
    require(queued.accepted(), "second queued task must be accepted");
    require(
        overflow.code == cpu::CpuTaskSubmissionCode::queue_full,
        "bounded queue must report backpressure");
    require(
        cancellable.handle.request_cancel(),
        "queued task cancellation request must succeed");

    {
        std::lock_guard<std::mutex> lock(gate_mutex);
        release_blocker = true;
    }
    gate_cv.notify_all();

    blocker.handle.wait();
    queued.handle.wait();
    cancellable.handle.wait();

    require(
        blocker.handle.result().state == cpu::CpuTaskState::completed,
        "blocking task must complete");
    require(
        queued.handle.result().state == cpu::CpuTaskState::completed,
        "queued task must complete");
    require(
        cancellable.handle.result().state ==
            cpu::CpuTaskState::cancelled,
        "cancel-requested queued task must be cancelled");
    require(executed.load() == 2,
            "cancelled task must not execute");

    const cpu::CpuTaskSubmissionResult throwing = pool.submit(
        []() { throw std::runtime_error("expected failure"); });
    require(throwing.accepted(), "throwing task must be accepted");
    throwing.handle.wait();
    const cpu::CpuTaskResult failed_result = throwing.handle.result();
    require(
        failed_result.state == cpu::CpuTaskState::failed,
        "task exception must be contained");
    require(
        failed_result.error_message == "expected failure",
        "standard exception message must be retained");

    const cpu::CpuTaskSubmissionResult recovery = pool.submit(
        [&]() { ++executed; });
    require(
        recovery.accepted(),
        "worker must remain usable after a task exception");
    recovery.handle.wait();
    require(
        recovery.handle.result().succeeded(),
        "post-exception task must complete");
    require(executed.load() == 3,
            "post-exception task must execute");

    std::atomic<bool> worker_stop_rejected{false};
    const cpu::CpuTaskSubmissionResult worker_stop = pool.submit(
        [&]() {
            const cpu::CpuThreadPoolStatus self_stop =
                pool.stop(
                    cpu::CpuThreadPoolStopMode::cancel_pending);
            worker_stop_rejected.store(
                self_stop.state == cpu::CpuThreadPoolState::running);
        });
    require(worker_stop.accepted(),
            "worker lifecycle-guard task must be accepted");
    worker_stop.handle.wait();
    require(worker_stop.handle.result().succeeded(),
            "worker lifecycle-guard task must complete");
    require(worker_stop_rejected.load(),
            "worker must not be allowed to stop its own pool");

    const cpu::CpuThreadPoolStatus drained = pool.drain();
    require(
        drained.state == cpu::CpuThreadPoolState::stopped,
        "drain must stop the pool");

    const cpu::CpuThreadPoolSnapshot snapshot = pool.snapshot();
    require(snapshot.configured_worker_count == 1,
            "configured worker count must be retained");
    require(snapshot.live_worker_count == 0,
            "drained pool must have no live workers");
    require(snapshot.queue_capacity == 2,
            "queue capacity must be retained");
    require(snapshot.queued_task_count == 0,
            "drained pool queue must be empty");
    require(snapshot.active_task_count == 0,
            "drained pool must have no active tasks");
    require(snapshot.accepted_task_count == 6,
            "accepted task count must be exact");
    require(snapshot.rejected_task_count == 3,
            "rejected task count must include invalid and backpressured submissions");
    require(snapshot.completed_task_count == 4,
            "completed task count must be exact");
    require(snapshot.failed_task_count == 1,
            "failed task count must be exact");
    require(snapshot.cancelled_task_count == 1,
            "cancelled task count must be exact");

    const cpu::CpuTaskSubmissionResult after_stop =
        pool.submit([]() {});
    require(
        after_stop.code ==
            cpu::CpuTaskSubmissionCode::pool_not_running,
        "stopped pool must reject new work");

    cpu::CpuThreadPoolOptions stop_options{};
    stop_options.worker_count = 1;
    stop_options.queue_capacity = 1;
    cpu::CpuThreadPool stop_pool{stop_options};
    require(
        stop_pool.start().state == cpu::CpuThreadPoolState::running,
        "second pool must start");

    std::mutex stop_gate_mutex;
    std::condition_variable stop_gate_cv;
    bool stop_blocker_started = false;
    bool release_stop_blocker = false;

    const cpu::CpuTaskSubmissionResult stop_blocker =
        stop_pool.submit([&]() {
            {
                std::lock_guard<std::mutex> lock(stop_gate_mutex);
                stop_blocker_started = true;
            }
            stop_gate_cv.notify_all();

            std::unique_lock<std::mutex> lock(stop_gate_mutex);
            stop_gate_cv.wait(
                lock,
                [&]() { return release_stop_blocker; });
        });
    require(stop_blocker.accepted(),
            "stop blocker must be accepted");

    {
        std::unique_lock<std::mutex> lock(stop_gate_mutex);
        require(
            stop_gate_cv.wait_for(
                lock,
                std::chrono::seconds(2),
                [&]() { return stop_blocker_started; }),
            "stop blocker must begin");
    }

    const cpu::CpuTaskSubmissionResult stop_queued =
        stop_pool.submit([]() {});
    require(stop_queued.accepted(),
            "queued stop task must be accepted");

    std::thread stopper([&]() {
        static_cast<void>(
            stop_pool.stop(
                cpu::CpuThreadPoolStopMode::cancel_pending));
    });

    require(
        stop_queued.handle.wait_for(std::chrono::seconds(2)),
        "cancel-pending stop must complete queued handle");
    require(
        stop_queued.handle.result().state ==
            cpu::CpuTaskState::cancelled,
        "cancel-pending stop must cancel queued work");

    {
        std::lock_guard<std::mutex> lock(stop_gate_mutex);
        release_stop_blocker = true;
    }
    stop_gate_cv.notify_all();
    stopper.join();

    require(
        stop_pool.status().state ==
            cpu::CpuThreadPoolState::stopped,
        "cancel-pending stop must stop pool");

    return EXIT_SUCCESS;
}
