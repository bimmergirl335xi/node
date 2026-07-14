#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include "cuda_backend.hpp"
#include "cuda_job_queue.hpp"
#include "cuda_memory_pool.hpp"

namespace pc = prometheus::backends::cuda;

namespace {

void print_bytes(std::uint64_t bytes) {
    const double mib =
        static_cast<double>(bytes) / (1024.0 * 1024.0);
    std::cout << bytes << " bytes (" << std::fixed
              << std::setprecision(2) << mib << " MiB)";
}

}  // namespace

int main() {
    std::cout << "Prometheus CUDA memory-pool and job-queue probe\n";

    pc::CudaBackend backend{};
    const auto backend_status = backend.initialize();
    std::cout << "Backend: "
              << prometheus::backends::to_string(backend_status.state)
              << "\n";
    if (!backend_status.usable) {
        return EXIT_FAILURE;
    }

    const pc::CudaDevicePool pool = backend.pool_snapshot();
    if (pool.devices.empty()) {
        return EXIT_FAILURE;
    }

    const std::string key = pool.devices.front().persistent_key();
    std::cout << "Selected stable device key: " << key << "\n";

    pc::CudaMemoryPoolOptions options{};
    options.maximum_cached_block_bytes =
        64ULL * 1024ULL * 1024ULL;
    options.maximum_cached_bytes_per_device =
        128ULL * 1024ULL * 1024ULL;

    pc::CudaMemoryPool memory_pool{options};
    const auto init = memory_pool.initialize(pool);
    std::cout << "Memory pool initialize: "
              << pc::to_string(init.code) << "\n";
    if (!init.ok()) {
        return EXIT_FAILURE;
    }

    auto first = memory_pool.allocate(
        key,
        16ULL * 1024ULL * 1024ULL,
        "probe-specialist-state");
    std::cout << "First allocation: "
              << pc::to_string(first.status.code) << ", ";
    print_bytes(first.allocation.capacity_bytes);
    std::cout << ", reused="
              << (first.allocation.reused_cached_block ? "yes" : "no")
              << "\n";
    if (!first.status.ok()) {
        return EXIT_FAILURE;
    }

    if (!memory_pool.release(first.allocation.lease_id).ok()) {
        return EXIT_FAILURE;
    }

    auto second = memory_pool.allocate(
        key,
        8ULL * 1024ULL * 1024ULL,
        "probe-specialist-state-reuse");
    std::cout << "Second allocation: "
              << pc::to_string(second.status.code) << ", ";
    print_bytes(second.allocation.capacity_bytes);
    std::cout << ", reused="
              << (second.allocation.reused_cached_block ? "yes" : "no")
              << "\n";
    if (!second.status.ok()) {
        return EXIT_FAILURE;
    }

    pc::CudaKernelDescriptor descriptor{};
    descriptor.kernel_id = "probe.runtime_resources.fp32";
    descriptor.family = pc::CudaKernelFamily::activation;
    descriptor.scalar_types = {pc::CudaScalarType::fp32};
    descriptor.minimum_compute_capability = {6, 1};
    descriptor.cuda_12_release_support =
        pc::CudaSupportState::supported;
    descriptor.cuda_13_release_support =
        pc::CudaSupportState::supported;
    descriptor.binary_state =
        pc::CudaKernelBinaryState::available;
    descriptor.binding_state =
        pc::CudaKernelBindingState::adapter_available;

    if (!backend.kernel_registry().register_kernel(descriptor).ok()) {
        return EXIT_FAILURE;
    }

    pc::CudaJobQueue queue{};

    for (int index = 0; index < 3; ++index) {
        pc::CudaJobDescriptor job{};
        job.job_id = "probe-job-" + std::to_string(index);
        job.request_id = "probe-specialists";
        job.instance_index = static_cast<std::size_t>(index);
        job.target_device_key = key;
        job.kernel_id = descriptor.kernel_id;
        job.priority = index == 2
                           ? pc::CudaJobPriority::urgent
                           : pc::CudaJobPriority::normal;
        job.memory_lease_ids = {second.allocation.lease_id};

        const auto submit = queue.submit(
            std::move(job),
            pool,
            backend.kernel_registry());
        std::cout << "Submit " << index << ": "
                  << pc::to_string(submit.code) << "\n";
        if (!submit.ok()) {
            return EXIT_FAILURE;
        }
    }

    const auto claim = queue.claim_next(key);
    std::cout << "First claimed job: "
              << (claim.has_job
                      ? claim.job.descriptor.job_id
                      : std::string{"none"})
              << ", priority="
              << (claim.has_job
                      ? pc::to_string(
                            claim.job.descriptor.priority)
                      : "unknown")
              << "\n";
    if (!claim.ok() ||
        claim.job.descriptor.job_id != "probe-job-2") {
        return EXIT_FAILURE;
    }

    if (!queue.mark_running(
             claim.job.descriptor.job_id,
             claim.job.claim_token).ok() ||
        !queue.complete(
             claim.job.descriptor.job_id,
             claim.job.claim_token,
             "probe completion").ok()) {
        return EXIT_FAILURE;
    }

    const auto queue_snapshot = queue.snapshot();
    std::cout << "Queue snapshot: queued="
              << queue_snapshot.queued
              << ", running=" << queue_snapshot.running
              << ", completed=" << queue_snapshot.completed
              << "\n";

    if (!memory_pool.release(second.allocation.lease_id).ok() ||
        !memory_pool.trim_all().ok() ||
        !memory_pool.shutdown().ok()) {
        return EXIT_FAILURE;
    }

    backend.shutdown();
    std::cout << "Phase 6 CUDA runtime-resource probe passed\n";
    return EXIT_SUCCESS;
}
