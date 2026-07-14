#ifndef SRC_BACKENDS_CUDA_CUDA_JOB_QUEUE_HPP
#define SRC_BACKENDS_CUDA_CUDA_JOB_QUEUE_HPP

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "cuda_device_pool.hpp"
#include "cuda_kernel_registry.hpp"

namespace prometheus::backends::cuda {

enum class CudaJobPriority : std::uint8_t {
    background = 0,
    low,
    normal,
    high,
    urgent,
};

enum class CudaJobState : std::uint8_t {
    queued = 0,
    claimed,
    running,
    completed,
    failed,
    cancelled,
};

struct CudaJobDescriptor {
    std::string job_id{};
    std::string request_id{};
    std::size_t instance_index = 0;

    // Jobs are submitted after placement, so the target is a durable UUID-based
    // device key rather than a temporary CUDA ordinal.
    std::string target_device_key{};
    std::string kernel_id{};

    CudaJobPriority priority = CudaJobPriority::normal;

    // Leases are descriptive here. The queue does not own or free memory.
    std::vector<std::uint64_t> memory_lease_ids{};

    // Opaque correlation value for a future service/message layer.
    std::string correlation_tag{};
};

struct CudaJobRecord {
    CudaJobDescriptor descriptor{};
    CudaJobState state = CudaJobState::queued;
    std::uint64_t sequence = 0;
    std::uint64_t submitted_unix_ns = 0;
    std::uint64_t claim_token = 0;
    std::uint64_t claimed_unix_ns = 0;
    std::uint64_t started_unix_ns = 0;
    std::uint64_t finished_unix_ns = 0;
    std::string terminal_message{};
};

struct CudaJobQueueOptions {
    std::size_t maximum_active_jobs = 4096;
    std::size_t maximum_retained_terminal_jobs = 1024;
};

enum class CudaJobQueueCode : std::uint8_t {
    success = 0,
    no_job_available,
    invalid_descriptor,
    duplicate_job_id,
    capacity_reached,
    unknown_device,
    device_not_ready,
    kernel_not_ready,
    job_not_found,
    invalid_state,
    claim_token_mismatch,
};

struct CudaJobQueueResult {
    CudaJobQueueCode code = CudaJobQueueCode::success;
    std::string message{};
    bool has_job = false;
    CudaJobRecord job{};

    [[nodiscard]] bool ok() const noexcept {
        return code == CudaJobQueueCode::success;
    }
};

struct CudaJobQueueSnapshot {
    std::size_t total_records = 0;
    std::size_t queued = 0;
    std::size_t claimed = 0;
    std::size_t running = 0;
    std::size_t completed = 0;
    std::size_t failed = 0;
    std::size_t cancelled = 0;
};

// Claim-based queue foundation.
//
// The queue creates no worker thread, stream, event, or kernel launch. A later
// execution worker may claim a job, resolve a typed adapter, launch it on an
// explicitly managed stream, and report completion through the claim token.
class CudaJobQueue {
public:
    explicit CudaJobQueue(CudaJobQueueOptions options = {});

    CudaJobQueue(const CudaJobQueue&) = delete;
    CudaJobQueue& operator=(const CudaJobQueue&) = delete;
    CudaJobQueue(CudaJobQueue&&) = delete;
    CudaJobQueue& operator=(CudaJobQueue&&) = delete;

    [[nodiscard]] CudaJobQueueResult submit(
        CudaJobDescriptor descriptor,
        const CudaDevicePool& device_pool,
        const CudaKernelRegistry& kernel_registry);

    [[nodiscard]] CudaJobQueueResult claim_next(
        const std::string& persistent_key);

    [[nodiscard]] CudaJobQueueResult mark_running(
        const std::string& job_id,
        std::uint64_t claim_token);

    [[nodiscard]] CudaJobQueueResult complete(
        const std::string& job_id,
        std::uint64_t claim_token,
        std::string message = {});

    [[nodiscard]] CudaJobQueueResult fail(
        const std::string& job_id,
        std::uint64_t claim_token,
        std::string message);

    [[nodiscard]] CudaJobQueueResult cancel(
        const std::string& job_id,
        std::uint64_t claim_token = 0,
        std::string message = {});

    [[nodiscard]] CudaJobQueueResult requeue_claim(
        const std::string& job_id,
        std::uint64_t claim_token,
        std::string message = {});

    [[nodiscard]] CudaJobQueueResult find(
        const std::string& job_id) const;

    [[nodiscard]] CudaJobQueueSnapshot snapshot() const;

    std::size_t purge_terminal();
    void clear();

private:
    [[nodiscard]] std::size_t active_job_count_locked() const noexcept;
    [[nodiscard]] std::vector<CudaJobRecord>::iterator find_locked(
        const std::string& job_id);
    [[nodiscard]] std::vector<CudaJobRecord>::const_iterator find_locked(
        const std::string& job_id) const;
    void enforce_terminal_retention_locked();

    CudaJobQueueOptions options_{};
    mutable std::mutex mutex_{};
    std::uint64_t next_sequence_ = 1;
    std::uint64_t next_claim_token_ = 1;
    std::vector<CudaJobRecord> records_{};
};

[[nodiscard]] const char* to_string(CudaJobPriority value) noexcept;
[[nodiscard]] const char* to_string(CudaJobState value) noexcept;
[[nodiscard]] const char* to_string(CudaJobQueueCode value) noexcept;

}  // namespace prometheus::backends::cuda

#endif  // SRC_BACKENDS_CUDA_CUDA_JOB_QUEUE_HPP
