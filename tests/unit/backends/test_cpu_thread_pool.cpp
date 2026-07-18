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

namespace {

void test_bounded_shutdown_and_restart() {
    cpu::CpuThreadPoolOptions options{};
    options.worker_count = 1;
    options.queue_capacity = 2;
    cpu::CpuThreadPool pool{options};
    require(
        pool.start().state == cpu::CpuThreadPoolState::running,
        "bounded-shutdown pool must start");

    std::mutex gate_mutex;
    std::condition_variable gate_cv;
    bool task_started = false;
    bool release_task = false;
    const cpu::CpuTaskSubmissionResult blocker = pool.submit([&]() {
        {
            std::lock_guard<std::mutex> lock(gate_mutex);
            task_started = true;
        }
        gate_cv.notify_all();
        std::unique_lock<std::mutex> lock(gate_mutex);
        gate_cv.wait(lock, [&]() { return release_task; });
    });
    require(blocker.accepted(), "bounded-shutdown blocker must be accepted");
    {
        std::unique_lock<std::mutex> lock(gate_mutex);
        require(
            gate_cv.wait_for(
                lock,
                std::chrono::seconds(2),
                [&]() { return task_started; }),
            "bounded-shutdown blocker must start");
    }

    const auto request_started = std::chrono::steady_clock::now();
    require(
        pool.request_shutdown(
            cpu::CpuThreadPoolStopMode::cancel_pending) ==
            cpu::CpuThreadPoolShutdownResult::request_accepted,
        "non-blocking shutdown request must be accepted");
    require(
        std::chrono::steady_clock::now() - request_started <
            std::chrono::seconds(1),
        "shutdown request must return without waiting for running work");
    require(
        pool.submit([]() {}).code ==
            cpu::CpuTaskSubmissionCode::pool_stopping,
        "shutdown request must reject new work");

    const cpu::CpuThreadPoolSnapshot stopping = pool.snapshot();
    require(
        stopping.state == cpu::CpuThreadPoolState::stopping &&
            !stopping.accepting_work() &&
            stopping.live_worker_count == 1 &&
            stopping.active_task_count == 1 &&
            !stopping.fully_stopped(),
        "incomplete shutdown snapshot must remain truthful");
    require(
        pool.start().state == cpu::CpuThreadPoolState::stopping,
        "restart must be rejected while the old generation is stopping");
    require(
        pool.wait_for_shutdown(std::chrono::nanoseconds{-1}) ==
            cpu::CpuThreadPoolShutdownResult::invalid_timeout,
        "negative shutdown timeouts must be rejected");

    const auto wait_started = std::chrono::steady_clock::now();
    require(
        pool.wait_for_shutdown(std::chrono::milliseconds{10}) ==
            cpu::CpuThreadPoolShutdownResult::timed_out,
        "bounded shutdown wait must time out while work is blocked");
    require(
        std::chrono::steady_clock::now() - wait_started <
            std::chrono::seconds(1),
        "bounded shutdown wait must honor a reasonable upper tolerance");
    require(
        pool.snapshot().state == cpu::CpuThreadPoolState::stopping &&
            !pool.snapshot().fully_stopped(),
        "timeout must not publish a stopped state");

    {
        std::lock_guard<std::mutex> lock(gate_mutex);
        release_task = true;
    }
    gate_cv.notify_all();
    require(
        pool.wait_for_shutdown(std::chrono::seconds{2}) ==
            cpu::CpuThreadPoolShutdownResult::fully_stopped,
        "later shutdown wait must finalize exited workers");
    const cpu::CpuThreadPoolSnapshot stopped = pool.snapshot();
    require(
        stopped.fully_stopped() && stopped.live_worker_count == 0,
        "completed shutdown must report fully stopped");

    require(
        pool.start().state == cpu::CpuThreadPoolState::running,
        "fully finalized pool must restart");
    std::atomic<int> restarted_runs{0};
    const cpu::CpuTaskSubmissionResult restarted =
        pool.submit([&]() { ++restarted_runs; });
    require(restarted.accepted(), "restarted pool must accept work");
    require(
        restarted.handle.task_id() > blocker.handle.task_id(),
        "task identifiers must remain monotonic across restart");
    restarted.handle.wait();
    require(
        pool.drain().state == cpu::CpuThreadPoolState::stopped &&
            restarted_runs.load() == 1,
        "restarted generation must execute and drain");
    require(
        pool.snapshot().accepted_task_count == 2,
        "lifetime counters must remain cumulative across restart");
}

void test_drain_and_escalation() {
    cpu::CpuThreadPoolOptions drain_options{};
    drain_options.worker_count = 1;
    drain_options.queue_capacity = 4;
    cpu::CpuThreadPool drain_pool{drain_options};
    require(
        drain_pool.start().state == cpu::CpuThreadPoolState::running,
        "drain behavior pool must start");

    std::mutex drain_gate_mutex;
    std::condition_variable drain_gate_cv;
    bool drain_blocker_started = false;
    bool release_drain_blocker = false;
    std::atomic<int> drain_runs{0};
    const auto drain_blocker = drain_pool.submit([&]() {
        {
            std::lock_guard<std::mutex> lock(drain_gate_mutex);
            drain_blocker_started = true;
        }
        drain_gate_cv.notify_all();
        std::unique_lock<std::mutex> lock(drain_gate_mutex);
        drain_gate_cv.wait(
            lock, [&]() { return release_drain_blocker; });
        ++drain_runs;
    });
    require(drain_blocker.accepted(), "drain blocker must be accepted");
    {
        std::unique_lock<std::mutex> lock(drain_gate_mutex);
        require(
            drain_gate_cv.wait_for(
                lock,
                std::chrono::seconds(2),
                [&]() { return drain_blocker_started; }),
            "drain blocker must start");
    }

    std::vector<cpu::CpuTaskHandle> drain_handles;
    for (int index = 0; index < 3; ++index) {
        const auto submitted = drain_pool.submit([&]() { ++drain_runs; });
        require(submitted.accepted(), "queued drain task must be accepted");
        drain_handles.push_back(submitted.handle);
    }
    require(
        drain_pool.request_shutdown(cpu::CpuThreadPoolStopMode::drain) ==
            cpu::CpuThreadPoolShutdownResult::request_accepted,
        "drain request must be accepted");
    require(
        drain_pool.snapshot().queued_task_count == 3 &&
            drain_pool.snapshot().cancelled_task_count == 0,
        "drain request must preserve queued work");
    {
        std::lock_guard<std::mutex> lock(drain_gate_mutex);
        release_drain_blocker = true;
    }
    drain_gate_cv.notify_all();
    require(
        drain_pool.wait_for_shutdown(std::chrono::seconds{2}) ==
            cpu::CpuThreadPoolShutdownResult::fully_stopped,
        "drain request must complete after all accepted work");
    require(
        drain_runs.load() == 4 &&
            drain_pool.snapshot().completed_task_count == 4 &&
            drain_pool.snapshot().cancelled_task_count == 0,
        "drain must execute every accepted task exactly once");
    for (const auto& handle : drain_handles) {
        require(
            handle.result().state == cpu::CpuTaskState::completed,
            "drained queued task must complete");
    }

    cpu::CpuThreadPool escalation_pool{drain_options};
    require(
        escalation_pool.start().state == cpu::CpuThreadPoolState::running,
        "escalation pool must start");
    std::mutex escalation_gate_mutex;
    std::condition_variable escalation_gate_cv;
    bool escalation_blocker_started = false;
    bool release_escalation_blocker = false;
    const auto escalation_blocker = escalation_pool.submit([&]() {
        {
            std::lock_guard<std::mutex> lock(escalation_gate_mutex);
            escalation_blocker_started = true;
        }
        escalation_gate_cv.notify_all();
        std::unique_lock<std::mutex> lock(escalation_gate_mutex);
        escalation_gate_cv.wait(
            lock, [&]() { return release_escalation_blocker; });
    });
    require(
        escalation_blocker.accepted(),
        "escalation blocker must be accepted");
    {
        std::unique_lock<std::mutex> lock(escalation_gate_mutex);
        require(
            escalation_gate_cv.wait_for(
                lock,
                std::chrono::seconds(2),
                [&]() { return escalation_blocker_started; }),
            "escalation blocker must start");
    }

    std::vector<cpu::CpuTaskHandle> cancelled_handles;
    for (int index = 0; index < 4; ++index) {
        const auto submitted = escalation_pool.submit([]() {});
        require(
            submitted.accepted(),
            "queued escalation task must be accepted");
        cancelled_handles.push_back(submitted.handle);
    }
    require(
        escalation_pool.request_shutdown(
            cpu::CpuThreadPoolStopMode::drain) ==
            cpu::CpuThreadPoolShutdownResult::request_accepted,
        "initial drain request must be accepted");
    require(
        escalation_pool.request_shutdown(
            cpu::CpuThreadPoolStopMode::cancel_pending) ==
            cpu::CpuThreadPoolShutdownResult::escalated,
        "drain must escalate to cancel-pending");
    require(
        escalation_pool.request_shutdown(
            cpu::CpuThreadPoolStopMode::drain) ==
            cpu::CpuThreadPoolShutdownResult::already_requested,
        "weaker request must not reverse cancellation");

    std::vector<std::thread> requesters;
    for (int index = 0; index < 8; ++index) {
        requesters.emplace_back([&, index]() {
            const auto result = escalation_pool.request_shutdown(
                index % 2 == 0
                    ? cpu::CpuThreadPoolStopMode::drain
                    : cpu::CpuThreadPoolStopMode::cancel_pending);
            require(
                result ==
                    cpu::CpuThreadPoolShutdownResult::already_requested,
                "concurrent repeated request must be idempotent");
        });
    }
    for (auto& requester : requesters) {
        requester.join();
    }
    require(
        escalation_pool.snapshot().state ==
                cpu::CpuThreadPoolState::stopping &&
            escalation_pool.snapshot().cancelled_task_count == 4,
        "escalation must cancel queued work exactly once");
    for (const auto& handle : cancelled_handles) {
        require(
            handle.result().state == cpu::CpuTaskState::cancelled,
            "escalated queued task must be cancelled");
    }
    {
        std::lock_guard<std::mutex> lock(escalation_gate_mutex);
        release_escalation_blocker = true;
    }
    escalation_gate_cv.notify_all();

    std::vector<cpu::CpuThreadPoolShutdownResult> wait_results(4);
    std::vector<std::thread> waiters;
    for (std::size_t index = 0; index < wait_results.size(); ++index) {
        waiters.emplace_back([&, index]() {
            wait_results[index] = escalation_pool.wait_for_shutdown(
                std::chrono::seconds{2});
        });
    }
    for (auto& waiter : waiters) {
        waiter.join();
    }
    for (const auto result : wait_results) {
        require(
            result == cpu::CpuThreadPoolShutdownResult::fully_stopped,
            "concurrent shutdown waiters must all observe completion");
    }
}

}  // namespace

namespace {

void test_worker_origin_shutdown() {
    cpu::CpuThreadPoolOptions options{};
    options.worker_count = 1;
    options.queue_capacity = 2;
    cpu::CpuThreadPool pool{options};
    require(
        pool.start().state == cpu::CpuThreadPoolState::running,
        "worker-origin shutdown pool must start");

    std::atomic<cpu::CpuThreadPoolShutdownResult> request_result{
        cpu::CpuThreadPoolShutdownResult::failed_lifecycle};
    std::atomic<cpu::CpuThreadPoolShutdownResult> wait_result{
        cpu::CpuThreadPoolShutdownResult::failed_lifecycle};
    const auto task = pool.submit([&]() {
        request_result.store(pool.request_shutdown(
            cpu::CpuThreadPoolStopMode::drain));
        wait_result.store(pool.wait_for_shutdown(
            std::chrono::milliseconds{10}));
    });
    require(
        task.accepted(),
        "worker-origin shutdown task must be accepted");
    task.handle.wait();
    require(
        request_result.load() ==
            cpu::CpuThreadPoolShutdownResult::request_accepted,
        "worker may issue a non-blocking shutdown request");
    require(
        wait_result.load() ==
            cpu::CpuThreadPoolShutdownResult::called_from_worker,
        "worker-origin bounded wait must be rejected explicitly");
    require(
        pool.wait_for_shutdown(std::chrono::seconds{2}) ==
            cpu::CpuThreadPoolShutdownResult::fully_stopped,
        "external waiter must finalize worker-origin shutdown");
}

void test_two_worker_contention() {
    constexpr std::size_t task_count = 256;
    cpu::CpuThreadPoolOptions options{};
    options.worker_count = 2;
    options.queue_capacity = task_count;
    cpu::CpuThreadPool pool{options};
    require(
        pool.start().state == cpu::CpuThreadPoolState::running,
        "two-worker stress pool must start");

    std::vector<std::atomic<int>> executions(task_count);
    std::vector<cpu::CpuTaskHandle> handles;
    handles.reserve(task_count);
    for (std::size_t index = 0; index < task_count; ++index) {
        executions[index].store(0);
        const auto submitted = pool.submit([&, index]() {
            ++executions[index];
        });
        require(
            submitted.accepted(),
            "two-worker deterministic task must be accepted");
        handles.push_back(submitted.handle);
    }
    require(
        pool.request_shutdown(cpu::CpuThreadPoolStopMode::drain) ==
            cpu::CpuThreadPoolShutdownResult::request_accepted,
        "two-worker stress drain must be requested");
    require(
        pool.wait_for_shutdown(std::chrono::seconds{5}) ==
            cpu::CpuThreadPoolShutdownResult::fully_stopped,
        "two-worker stress drain must complete");
    for (std::size_t index = 0; index < task_count; ++index) {
        require(
            executions[index].load() == 1 &&
                handles[index].result().state ==
                    cpu::CpuTaskState::completed,
            "two-worker task must execute exactly once");
    }
    const auto snapshot = pool.snapshot();
    require(
        snapshot.accepted_task_count == task_count &&
            snapshot.started_task_count == task_count &&
            snapshot.completed_task_count == task_count &&
            snapshot.failed_task_count == 0 &&
            snapshot.cancelled_task_count == 0,
        "two-worker terminal accounting must be exact");
}

void test_four_worker_concurrent_producers() {
    constexpr std::size_t worker_count = 4;
    constexpr std::size_t producer_count = 4;
    constexpr std::size_t attempts_per_producer = 100;
    constexpr std::size_t attempted =
        producer_count * attempts_per_producer;
    constexpr std::size_t queue_capacity = 16;

    cpu::CpuThreadPoolOptions options{};
    options.worker_count = worker_count;
    options.queue_capacity = queue_capacity;
    cpu::CpuThreadPool pool{options};
    require(
        pool.start().state == cpu::CpuThreadPoolState::running,
        "four-worker producer pool must start");

    std::mutex producer_gate_mutex;
    std::condition_variable producer_gate_cv;
    std::size_t ready_producers = 0;
    bool release_producers = false;
    std::mutex task_gate_mutex;
    std::condition_variable task_gate_cv;
    bool release_tasks = false;

    std::vector<std::atomic<int>> executions(attempted);
    for (auto& execution : executions) {
        execution.store(0);
    }
    std::vector<std::vector<cpu::CpuTaskHandle>> producer_handles(
        producer_count);
    std::atomic<std::size_t> accepted{0};
    std::atomic<std::size_t> rejected{0};
    std::atomic<std::size_t> maximum_observed_queue{0};

    std::vector<std::thread> producers;
    for (std::size_t producer = 0;
         producer < producer_count;
         ++producer) {
        producers.emplace_back([&, producer]() {
            {
                std::unique_lock<std::mutex> lock(producer_gate_mutex);
                ++ready_producers;
                producer_gate_cv.notify_all();
                producer_gate_cv.wait(
                    lock, [&]() { return release_producers; });
            }
            for (std::size_t offset = 0;
                 offset < attempts_per_producer;
                 ++offset) {
                const std::size_t task_index =
                    producer * attempts_per_producer + offset;
                const auto submitted = pool.submit([&, task_index]() {
                    std::unique_lock<std::mutex> lock(task_gate_mutex);
                    task_gate_cv.wait(
                        lock, [&]() { return release_tasks; });
                    lock.unlock();
                    ++executions[task_index];
                });
                if (submitted.accepted()) {
                    ++accepted;
                    producer_handles[producer].push_back(
                        submitted.handle);
                } else {
                    ++rejected;
                }

                const std::size_t observed =
                    pool.snapshot().queued_task_count;
                std::size_t previous = maximum_observed_queue.load();
                while (observed > previous &&
                       !maximum_observed_queue.compare_exchange_weak(
                           previous, observed)) {
                }
            }
        });
    }
    {
        std::unique_lock<std::mutex> lock(producer_gate_mutex);
        require(
            producer_gate_cv.wait_for(
                lock,
                std::chrono::seconds{2},
                [&]() { return ready_producers == producer_count; }),
            "all producer threads must reach the synchronized start");
        release_producers = true;
    }
    producer_gate_cv.notify_all();
    for (auto& producer : producers) {
        producer.join();
    }

    require(
        accepted.load() + rejected.load() == attempted,
        "accepted plus rejected submissions must equal attempts");
    require(
        rejected.load() > 0,
        "bounded producer stress must exercise immediate backpressure");
    require(
        maximum_observed_queue.load() <= queue_capacity &&
            pool.snapshot().queued_task_count <= queue_capacity,
        "four-worker stress must preserve bounded queue capacity");
    require(
        pool.request_shutdown(cpu::CpuThreadPoolStopMode::drain) ==
            cpu::CpuThreadPoolShutdownResult::request_accepted,
        "four-worker stress drain must be requested");
    {
        std::lock_guard<std::mutex> lock(task_gate_mutex);
        release_tasks = true;
    }
    task_gate_cv.notify_all();
    require(
        pool.wait_for_shutdown(std::chrono::seconds{5}) ==
            cpu::CpuThreadPoolShutdownResult::fully_stopped,
        "four-worker stress shutdown must complete");

    std::size_t handle_count = 0;
    for (const auto& handles : producer_handles) {
        for (const auto& handle : handles) {
            ++handle_count;
            require(
                handle.result().state == cpu::CpuTaskState::completed,
                "accepted producer task must reach one terminal state");
        }
    }
    std::size_t execution_count = 0;
    for (const auto& execution : executions) {
        require(
            execution.load() <= 1,
            "producer task must never execute more than once");
        execution_count += static_cast<std::size_t>(execution.load());
    }
    const auto snapshot = pool.snapshot();
    require(
        handle_count == accepted.load() &&
            execution_count == accepted.load() &&
            snapshot.accepted_task_count == accepted.load() &&
            snapshot.rejected_task_count == rejected.load() &&
            snapshot.started_task_count == accepted.load() &&
            snapshot.completed_task_count == accepted.load() &&
            snapshot.failed_task_count == 0 &&
            snapshot.cancelled_task_count == 0,
        "concurrent producer accounting must be exact");
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

    test_bounded_shutdown_and_restart();
    test_drain_and_escalation();
    test_worker_origin_shutdown();
    test_two_worker_contention();
    test_four_worker_concurrent_producers();

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
