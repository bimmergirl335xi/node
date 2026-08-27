#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cuda_device_worker.hpp"

namespace pc = prometheus::backends::cuda;
using namespace std::chrono_literals;

namespace {

struct ProbeJob {
    std::size_t worker_index = 0;
    std::string job_id{};
    pc::CudaJobRecord terminal{};
};

void stop_workers(
    std::vector<std::unique_ptr<pc::CudaDeviceWorker>>& workers) noexcept {
    for (auto& worker : workers) {
        (void)worker->stop();
    }
}

}  // namespace

int main() {
    pc::CudaDevicePoolBuildOptions pool_options{};
    pool_options.query_health = true;
    pool_options.health_options.query_nvml = false;
    pool_options.health_options.allow_cuda_memory_fallback = false;
    pool_options.query_peer_access = false;
    const pc::CudaDevicePoolBuildResult discovery =
        pc::discover_cuda_device_pool(pool_options);

    if (discovery.pool.devices.empty()) {
        std::cout << "CUDA_EXECUTION status=no_devices devices=0 submitted=0 "
                     "completed=0 failed=0\n"
                  << "CUDA_RESOURCES workers=0 stream_destroyed=true "
                     "event_destroyed=true adapters_destroyed=true "
                     "worker_joined=true\n";
        return EXIT_SUCCESS;
    }

    pc::CudaKernelRegistry metadata{};
    const pc::CudaKernelRegistrationResult metadata_registration =
        metadata.register_kernel(
            pc::make_cuda_synthetic_execution_descriptor());
    if (!metadata_registration.ok()) {
        std::cerr << "CUDA_EXECUTION status=failed reason=metadata_registration\n";
        return EXIT_FAILURE;
    }

    pc::CudaJobQueue queue{};
    std::vector<std::unique_ptr<pc::CudaDeviceWorker>> workers{};
    for (const pc::CudaPoolDevice& device : discovery.pool.devices) {
        if (!device.registration_ready() ||
            !device.runtime_binding_usable()) {
            continue;
        }

        auto worker = std::make_unique<pc::CudaDeviceWorker>(
            device.persistent_key(), discovery.pool, queue, metadata);
        if (!worker->register_adapter(
                std::make_unique<pc::CudaSyntheticFp32Adapter>()).ok()) {
            stop_workers(workers);
            std::cerr << "CUDA_EXECUTION status=failed "
                         "reason=adapter_registration\n";
            return EXIT_FAILURE;
        }
        const pc::CudaDeviceWorkerResult started = worker->start();
        if (!started.ok()) {
            stop_workers(workers);
            std::cerr << "CUDA_EXECUTION status=failed reason=worker_start "
                      << "code=" << pc::to_string(started.code) << '\n';
            return EXIT_FAILURE;
        }
        workers.push_back(std::move(worker));
    }

    if (workers.empty()) {
        std::cout << "CUDA_EXECUTION status=unavailable devices=0 submitted=0 "
                     "completed=0 failed=0 reason=no_usable_binding\n";
        return EXIT_SUCCESS;
    }

    constexpr std::size_t kJobsPerWorker = 8;
    std::vector<ProbeJob> jobs{};
    jobs.reserve(workers.size() * kJobsPerWorker);
    for (std::size_t worker_index = 0;
         worker_index < workers.size();
         ++worker_index) {
        const pc::CudaDeviceWorkerSnapshot worker_snapshot =
            workers[worker_index]->snapshot();
        for (std::size_t job_index = 0;
             job_index < kJobsPerWorker;
             ++job_index) {
            ProbeJob tracked{};
            tracked.worker_index = worker_index;
            tracked.job_id = "probe-worker-" +
                             std::to_string(worker_index) + '-' +
                             std::to_string(job_index);
            pc::CudaJobDescriptor job{};
            job.job_id = tracked.job_id;
            job.request_id = "gpu-7.2a-execution-probe";
            job.instance_index = job_index;
            job.target_device_key =
                worker_snapshot.persistent_device_key;
            job.kernel_id = pc::kCudaSyntheticExecutionKernelId;
            if (!workers[worker_index]->submit(std::move(job)).ok()) {
                stop_workers(workers);
                std::cerr << "CUDA_EXECUTION status=failed "
                             "reason=job_submission\n";
                return EXIT_FAILURE;
            }
            jobs.push_back(std::move(tracked));
        }
    }

    std::size_t completed = 0;
    std::size_t failed = 0;
    for (ProbeJob& job : jobs) {
        const pc::CudaDeviceWorkerResult terminal =
            workers[job.worker_index]->wait_for_terminal(job.job_id, 15s);
        if (!terminal.ok() || !terminal.has_job) {
            stop_workers(workers);
            std::cerr << "CUDA_EXECUTION status=failed "
                         "reason=terminal_wait job_id="
                      << job.job_id << '\n';
            return EXIT_FAILURE;
        }
        job.terminal = terminal.job;
        if (terminal.job.state == pc::CudaJobState::completed) {
            ++completed;
        } else {
            ++failed;
        }
    }

    stop_workers(workers);
    bool all_streams_destroyed = true;
    bool all_events_destroyed = true;
    bool all_adapters_destroyed = true;
    bool all_threads_joined = true;
    std::vector<pc::CudaDeviceWorkerSnapshot> final_snapshots{};
    final_snapshots.reserve(workers.size());
    for (const auto& worker : workers) {
        pc::CudaDeviceWorkerSnapshot snapshot = worker->snapshot();
        all_streams_destroyed =
            all_streams_destroyed && snapshot.stream_destroyed;
        all_events_destroyed =
            all_events_destroyed && snapshot.event_destroyed;
        all_adapters_destroyed =
            all_adapters_destroyed && snapshot.adapter_resources_destroyed;
        all_threads_joined = all_threads_joined && snapshot.thread_joined;
        final_snapshots.push_back(std::move(snapshot));
    }

    const bool success = failed == 0 && completed == jobs.size() &&
                         all_streams_destroyed && all_events_destroyed &&
                         all_adapters_destroyed && all_threads_joined;
    std::cout << "CUDA_EXECUTION status="
              << (success ? "success" : "failed")
              << " devices=" << workers.size()
              << " submitted=" << jobs.size()
              << " completed=" << completed
              << " failed=" << failed << '\n';

    for (const pc::CudaDeviceWorkerSnapshot& snapshot : final_snapshots) {
        std::cout << "CUDA_WORKER device=" << snapshot.persistent_device_key
                  << " ordinal_snapshot=" << snapshot.runtime_ordinal_snapshot
                  << " state=" << pc::to_string(snapshot.state)
                  << " adapters=" << snapshot.adapter_count
                  << " claimed=" << snapshot.claimed_job_count
                  << " started=" << snapshot.started_job_count
                  << " completed=" << snapshot.completed_job_count
                  << " failed=" << snapshot.failed_job_count << '\n';
    }

    for (const ProbeJob& job : jobs) {
        std::cout << "CUDA_JOB job_id=" << job.job_id
                  << " device=" << job.terminal.descriptor.target_device_key
                  << " kernel=" << job.terminal.descriptor.kernel_id
                  << " state=" << pc::to_string(job.terminal.state)
                  << " claim_token=" << job.terminal.claim_token << '\n';
    }

    std::cout << "CUDA_RESOURCES workers=" << workers.size()
              << " stream_destroyed="
              << (all_streams_destroyed ? "true" : "false")
              << " event_destroyed="
              << (all_events_destroyed ? "true" : "false")
              << " adapters_destroyed="
              << (all_adapters_destroyed ? "true" : "false")
              << " worker_joined="
              << (all_threads_joined ? "true" : "false") << '\n';
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
