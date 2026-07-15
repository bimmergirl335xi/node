#include "cpu_thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

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
    const cpu::CpuTaskSubmissionResult replacement = pool.submit(
        [&]() { ++executed; },
        cpu::CpuTaskPriority::high);
    require(
        replacement.accepted(),
        "cancelled queued work must release capacity on submission");

    {
        std::lock_guard<std::mutex> lock(gate_mutex);
        release_blocker = true;
    }
    gate_cv.notify_all();

    blocker.handle.wait();
    queued.handle.wait();
    cancellable.handle.wait();
    replacement.handle.wait();

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
    require(
        replacement.handle.result().state ==
            cpu::CpuTaskState::completed,
        "replacement task must complete");
    require(executed.load() == 3,
            "cancelled task must not execute and replacement must run");

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
    require(executed.load() == 4,
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
    require(snapshot.accepted_task_count == 7,
            "accepted task count must be exact");
    require(snapshot.rejected_task_count == 3,
            "rejected task count must include invalid and backpressured submissions");
    require(snapshot.completed_task_count == 5,
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

    require(
        pool.start().state == cpu::CpuThreadPoolState::running,
        "a stopped pool must support an explicit restart");
    const cpu::CpuTaskSubmissionResult after_restart =
        pool.submit([&]() { ++executed; });
    require(after_restart.accepted(),
            "restarted pool must accept work");
    after_restart.handle.wait();
    require(after_restart.handle.result().succeeded(),
            "restarted pool work must complete");
    require(
        pool.drain().state == cpu::CpuThreadPoolState::stopped,
        "restarted pool must drain cleanly");

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

    cpu::CpuThreadPoolOptions fairness_options{};
    fairness_options.worker_count = 1;
    fairness_options.queue_capacity = 16;
    cpu::CpuThreadPool fairness_pool{fairness_options};
    require(
        fairness_pool.start().state == cpu::CpuThreadPoolState::running,
        "fairness pool must start");

    std::mutex fairness_gate_mutex;
    std::condition_variable fairness_gate_cv;
    bool fairness_blocker_started = false;
    bool release_fairness_blocker = false;
    const cpu::CpuTaskSubmissionResult fairness_blocker =
        fairness_pool.submit(
            [&]() {
                {
                    std::lock_guard<std::mutex> lock(fairness_gate_mutex);
                    fairness_blocker_started = true;
                }
                fairness_gate_cv.notify_all();
                std::unique_lock<std::mutex> lock(fairness_gate_mutex);
                fairness_gate_cv.wait(
                    lock,
                    [&]() { return release_fairness_blocker; });
            },
            cpu::CpuTaskPriority::critical);
    require(fairness_blocker.accepted(),
            "fairness blocker must be accepted");
    {
        std::unique_lock<std::mutex> lock(fairness_gate_mutex);
        require(
            fairness_gate_cv.wait_for(
                lock,
                std::chrono::seconds(2),
                [&]() { return fairness_blocker_started; }),
            "fairness blocker must begin");
    }

    std::atomic<int> fairness_order{0};
    std::atomic<int> low_task_order{0};
    const cpu::CpuTaskSubmissionResult low_task = fairness_pool.submit(
        [&]() { low_task_order.store(++fairness_order); },
        cpu::CpuTaskPriority::low);
    require(low_task.accepted(), "low-priority task must be accepted");

    std::vector<cpu::CpuTaskHandle> critical_handles;
    for (int index = 0; index < 12; ++index) {
        const cpu::CpuTaskSubmissionResult critical = fairness_pool.submit(
            [&]() { ++fairness_order; },
            cpu::CpuTaskPriority::critical);
        require(critical.accepted(),
                "critical fairness task must be accepted");
        critical_handles.push_back(critical.handle);
    }

    {
        std::lock_guard<std::mutex> lock(fairness_gate_mutex);
        release_fairness_blocker = true;
    }
    fairness_gate_cv.notify_all();
    low_task.handle.wait();
    for (const cpu::CpuTaskHandle& handle : critical_handles) {
        handle.wait();
    }
    require(
        low_task_order.load() > 0 && low_task_order.load() <= 8,
        "weighted priorities must bound low-priority starvation");
    require(
        fairness_pool.drain().state == cpu::CpuThreadPoolState::stopped,
        "fairness pool must drain");

    cpu::CpuThreadPoolOptions destruction_options{};
    destruction_options.worker_count = 1;
    destruction_options.queue_capacity = 1;
    auto worker_owned_pool =
        std::make_unique<cpu::CpuThreadPool>(destruction_options);
    require(
        worker_owned_pool->start().state ==
            cpu::CpuThreadPoolState::running,
        "worker-destruction pool must start");

    std::mutex destruction_gate_mutex;
    std::condition_variable destruction_gate_cv;
    bool release_destruction = false;
    const cpu::CpuTaskSubmissionResult destroy_from_worker =
        worker_owned_pool->submit([&]() {
            std::unique_lock<std::mutex> lock(destruction_gate_mutex);
            destruction_gate_cv.wait(
                lock,
                [&]() { return release_destruction; });
            lock.unlock();
            worker_owned_pool.reset();
        });
    require(destroy_from_worker.accepted(),
            "worker-origin destruction task must be accepted");
    {
        std::lock_guard<std::mutex> lock(destruction_gate_mutex);
        release_destruction = true;
    }
    destruction_gate_cv.notify_all();
    require(
        destroy_from_worker.handle.wait_for(std::chrono::seconds(2)),
        "worker-origin destruction must not terminate or deadlock");
    require(worker_owned_pool == nullptr,
            "worker-origin destruction must release the pool object");

    return EXIT_SUCCESS;
}
