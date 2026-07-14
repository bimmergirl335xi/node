#include <cstdlib>
#include <string>

#include "cuda_backend.hpp"
#include "cuda_job_queue.hpp"
#include "cuda_memory_pool.hpp"

namespace pc = prometheus::backends::cuda;

int main() {
    pc::CudaBackend backend{};
    if (!backend.initialize().usable) {
        return EXIT_FAILURE;
    }

    const pc::CudaDevicePool device_pool = backend.pool_snapshot();
    if (device_pool.devices.empty()) {
        return EXIT_FAILURE;
    }

    const std::string device_key =
        device_pool.devices.front().persistent_key();

    pc::CudaMemoryPoolOptions memory_options{};
    memory_options.maximum_cached_block_bytes =
        16ULL * 1024ULL * 1024ULL;
    memory_options.maximum_cached_bytes_per_device =
        32ULL * 1024ULL * 1024ULL;

    pc::CudaMemoryPool memory_pool{memory_options};
    if (!memory_pool.initialize(device_pool).ok()) {
        return EXIT_FAILURE;
    }

    const pc::CudaMemoryAllocationResult first =
        memory_pool.allocate(
            device_key,
            4ULL * 1024ULL * 1024ULL,
            "test-first");
    if (!first.status.ok() || !first.allocation.valid()) {
        return EXIT_FAILURE;
    }

    if (!memory_pool.release(first.allocation.lease_id).ok()) {
        return EXIT_FAILURE;
    }

    const pc::CudaMemoryAllocationResult second =
        memory_pool.allocate(
            device_key,
            2ULL * 1024ULL * 1024ULL,
            "test-second");
    if (!second.status.ok() || !second.allocation.valid() ||
        !second.allocation.reused_cached_block ||
        second.allocation.device_pointer !=
            first.allocation.device_pointer) {
        return EXIT_FAILURE;
    }

    pc::CudaKernelDescriptor descriptor{};
    descriptor.kernel_id = "test.runtime_resources.fp32";
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

    pc::CudaJobDescriptor low{};
    low.job_id = "test-low";
    low.target_device_key = device_key;
    low.kernel_id = descriptor.kernel_id;
    low.priority = pc::CudaJobPriority::low;
    low.memory_lease_ids = {second.allocation.lease_id};

    pc::CudaJobDescriptor urgent{};
    urgent.job_id = "test-urgent";
    urgent.target_device_key = device_key;
    urgent.kernel_id = descriptor.kernel_id;
    urgent.priority = pc::CudaJobPriority::urgent;
    urgent.memory_lease_ids = {second.allocation.lease_id};

    if (!queue.submit(
             std::move(low),
             device_pool,
             backend.kernel_registry()).ok() ||
        !queue.submit(
             std::move(urgent),
             device_pool,
             backend.kernel_registry()).ok()) {
        return EXIT_FAILURE;
    }

    const pc::CudaJobQueueResult claimed =
        queue.claim_next(device_key);
    if (!claimed.ok() || !claimed.has_job ||
        claimed.job.descriptor.job_id != "test-urgent") {
        return EXIT_FAILURE;
    }

    if (!queue.mark_running(
             claimed.job.descriptor.job_id,
             claimed.job.claim_token).ok()) {
        return EXIT_FAILURE;
    }

    if (!queue.complete(
             claimed.job.descriptor.job_id,
             claimed.job.claim_token,
             "synthetic completion").ok()) {
        return EXIT_FAILURE;
    }

    const pc::CudaJobQueueResult low_claim =
        queue.claim_next(device_key);
    if (!low_claim.ok() ||
        low_claim.job.descriptor.job_id != "test-low") {
        return EXIT_FAILURE;
    }

    if (!queue.cancel(
             low_claim.job.descriptor.job_id,
             low_claim.job.claim_token,
             "synthetic cancellation").ok()) {
        return EXIT_FAILURE;
    }

    const pc::CudaJobQueueSnapshot queue_snapshot =
        queue.snapshot();
    if (queue_snapshot.completed != 1 ||
        queue_snapshot.cancelled != 1 ||
        queue_snapshot.queued != 0 ||
        queue_snapshot.running != 0) {
        return EXIT_FAILURE;
    }

    if (!memory_pool.release(second.allocation.lease_id).ok()) {
        return EXIT_FAILURE;
    }
    if (!memory_pool.trim_all().ok()) {
        return EXIT_FAILURE;
    }

    const pc::CudaMemoryPoolSnapshot memory_snapshot =
        memory_pool.snapshot();
    if (memory_snapshot.in_use_block_count != 0 ||
        memory_snapshot.cached_block_count != 0 ||
        memory_snapshot.release_failed_block_count != 0) {
        return EXIT_FAILURE;
    }

    if (!memory_pool.shutdown().ok()) {
        return EXIT_FAILURE;
    }

    backend.shutdown();
    return EXIT_SUCCESS;
}
