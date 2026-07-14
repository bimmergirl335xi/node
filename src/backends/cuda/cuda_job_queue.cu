#include "cuda_job_queue.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

namespace prometheus::backends::cuda {
namespace {

[[nodiscard]] std::uint64_t unix_time_ns() noexcept {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

[[nodiscard]] bool is_terminal(CudaJobState state) noexcept {
    return state == CudaJobState::completed ||
           state == CudaJobState::failed ||
           state == CudaJobState::cancelled;
}

[[nodiscard]] bool higher_priority(
    const CudaJobRecord& left,
    const CudaJobRecord& right) noexcept {
    const auto left_priority =
        static_cast<std::uint8_t>(left.descriptor.priority);
    const auto right_priority =
        static_cast<std::uint8_t>(right.descriptor.priority);

    if (left_priority != right_priority) {
        return left_priority > right_priority;
    }
    return left.sequence < right.sequence;
}

}  // namespace

CudaJobQueue::CudaJobQueue(CudaJobQueueOptions options)
    : options_(std::move(options)) {}

CudaJobQueueResult CudaJobQueue::submit(
    CudaJobDescriptor descriptor,
    const CudaDevicePool& device_pool,
    const CudaKernelRegistry& kernel_registry) {
    if (descriptor.job_id.empty() ||
        descriptor.target_device_key.empty() ||
        descriptor.kernel_id.empty()) {
        return {
            CudaJobQueueCode::invalid_descriptor,
            "Job ID, target device key, and kernel ID are required",
        };
    }

    const CudaPoolDevice* device =
        device_pool.find_device(descriptor.target_device_key);
    if (device == nullptr) {
        return {
            CudaJobQueueCode::unknown_device,
            "Target CUDA device is not present in the current pool",
        };
    }

    if (!device->registration_ready() ||
        !device->runtime_binding_usable()) {
        return {
            CudaJobQueueCode::device_not_ready,
            "Target CUDA device is not execution-ready",
        };
    }

    const CudaKernelCoverageReport coverage =
        kernel_registry.evaluate_device_coverage(
            device->discovered.capabilities,
            {descriptor.kernel_id});
    if (coverage.state != CudaKernelCoverageState::complete) {
        return {
            CudaJobQueueCode::kernel_not_ready,
            "Kernel metadata, binary image, or typed adapter is incomplete",
        };
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (find_locked(descriptor.job_id) != records_.end()) {
        return {
            CudaJobQueueCode::duplicate_job_id,
            "CUDA job ID is already present",
        };
    }

    if (active_job_count_locked() >= options_.maximum_active_jobs) {
        return {
            CudaJobQueueCode::capacity_reached,
            "CUDA job queue active-job capacity was reached",
        };
    }

    CudaJobRecord record{};
    record.descriptor = std::move(descriptor);
    record.state = CudaJobState::queued;
    record.sequence = next_sequence_++;
    record.submitted_unix_ns = unix_time_ns();
    records_.push_back(record);

    CudaJobQueueResult result{};
    result.code = CudaJobQueueCode::success;
    result.message = "CUDA job queued";
    result.has_job = true;
    result.job = std::move(record);
    return result;
}

CudaJobQueueResult CudaJobQueue::claim_next(
    const std::string& persistent_key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto selected = records_.end();
    for (auto iterator = records_.begin();
         iterator != records_.end();
         ++iterator) {
        if (iterator->state != CudaJobState::queued ||
            iterator->descriptor.target_device_key != persistent_key) {
            continue;
        }

        if (selected == records_.end() ||
            higher_priority(*iterator, *selected)) {
            selected = iterator;
        }
    }

    if (selected == records_.end()) {
        return {
            CudaJobQueueCode::no_job_available,
            "No queued CUDA job is available for this device",
        };
    }

    selected->state = CudaJobState::claimed;
    selected->claim_token = next_claim_token_++;
    selected->claimed_unix_ns = unix_time_ns();

    CudaJobQueueResult result{};
    result.code = CudaJobQueueCode::success;
    result.message = "CUDA job claimed";
    result.has_job = true;
    result.job = *selected;
    return result;
}

CudaJobQueueResult CudaJobQueue::mark_running(
    const std::string& job_id,
    std::uint64_t claim_token) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto iterator = find_locked(job_id);
    if (iterator == records_.end()) {
        return {
            CudaJobQueueCode::job_not_found,
            "CUDA job was not found",
        };
    }
    if (iterator->state != CudaJobState::claimed) {
        return {
            CudaJobQueueCode::invalid_state,
            "Only a claimed CUDA job may enter the running state",
        };
    }
    if (iterator->claim_token != claim_token || claim_token == 0) {
        return {
            CudaJobQueueCode::claim_token_mismatch,
            "CUDA job claim token does not match",
        };
    }

    iterator->state = CudaJobState::running;
    iterator->started_unix_ns = unix_time_ns();

    return {
        CudaJobQueueCode::success,
        "CUDA job marked running",
        true,
        *iterator,
    };
}

CudaJobQueueResult CudaJobQueue::complete(
    const std::string& job_id,
    std::uint64_t claim_token,
    std::string message) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto iterator = find_locked(job_id);
    if (iterator == records_.end()) {
        return {
            CudaJobQueueCode::job_not_found,
            "CUDA job was not found",
        };
    }
    if (iterator->state != CudaJobState::running) {
        return {
            CudaJobQueueCode::invalid_state,
            "Only a running CUDA job may complete",
        };
    }
    if (iterator->claim_token != claim_token || claim_token == 0) {
        return {
            CudaJobQueueCode::claim_token_mismatch,
            "CUDA job claim token does not match",
        };
    }

    iterator->state = CudaJobState::completed;
    iterator->finished_unix_ns = unix_time_ns();
    iterator->terminal_message = std::move(message);
    CudaJobRecord completed = *iterator;
    enforce_terminal_retention_locked();

    return {
        CudaJobQueueCode::success,
        "CUDA job completed",
        true,
        std::move(completed),
    };
}

CudaJobQueueResult CudaJobQueue::fail(
    const std::string& job_id,
    std::uint64_t claim_token,
    std::string message) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto iterator = find_locked(job_id);
    if (iterator == records_.end()) {
        return {
            CudaJobQueueCode::job_not_found,
            "CUDA job was not found",
        };
    }
    if (iterator->state != CudaJobState::running) {
        return {
            CudaJobQueueCode::invalid_state,
            "Only a running CUDA job may fail",
        };
    }
    if (iterator->claim_token != claim_token || claim_token == 0) {
        return {
            CudaJobQueueCode::claim_token_mismatch,
            "CUDA job claim token does not match",
        };
    }

    iterator->state = CudaJobState::failed;
    iterator->finished_unix_ns = unix_time_ns();
    iterator->terminal_message = std::move(message);
    CudaJobRecord failed = *iterator;
    enforce_terminal_retention_locked();

    return {
        CudaJobQueueCode::success,
        "CUDA job failed",
        true,
        std::move(failed),
    };
}

CudaJobQueueResult CudaJobQueue::cancel(
    const std::string& job_id,
    std::uint64_t claim_token,
    std::string message) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto iterator = find_locked(job_id);
    if (iterator == records_.end()) {
        return {
            CudaJobQueueCode::job_not_found,
            "CUDA job was not found",
        };
    }

    if (iterator->state == CudaJobState::running) {
        return {
            CudaJobQueueCode::invalid_state,
            "A running CUDA kernel cannot be represented as safely cancelled",
        };
    }

    if (iterator->state != CudaJobState::queued &&
        iterator->state != CudaJobState::claimed) {
        return {
            CudaJobQueueCode::invalid_state,
            "Only queued or claimed CUDA jobs may be cancelled",
        };
    }

    if (iterator->state == CudaJobState::claimed &&
        (claim_token == 0 || iterator->claim_token != claim_token)) {
        return {
            CudaJobQueueCode::claim_token_mismatch,
            "CUDA job claim token does not match",
        };
    }

    iterator->state = CudaJobState::cancelled;
    iterator->finished_unix_ns = unix_time_ns();
    iterator->terminal_message = std::move(message);
    CudaJobRecord cancelled = *iterator;
    enforce_terminal_retention_locked();

    return {
        CudaJobQueueCode::success,
        "CUDA job cancelled",
        true,
        std::move(cancelled),
    };
}

CudaJobQueueResult CudaJobQueue::requeue_claim(
    const std::string& job_id,
    std::uint64_t claim_token,
    std::string message) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto iterator = find_locked(job_id);
    if (iterator == records_.end()) {
        return {
            CudaJobQueueCode::job_not_found,
            "CUDA job was not found",
        };
    }
    if (iterator->state != CudaJobState::claimed) {
        return {
            CudaJobQueueCode::invalid_state,
            "Only a claimed CUDA job may be requeued",
        };
    }
    if (iterator->claim_token != claim_token || claim_token == 0) {
        return {
            CudaJobQueueCode::claim_token_mismatch,
            "CUDA job claim token does not match",
        };
    }

    iterator->state = CudaJobState::queued;
    iterator->claim_token = 0;
    iterator->claimed_unix_ns = 0;
    iterator->terminal_message = std::move(message);

    return {
        CudaJobQueueCode::success,
        "CUDA job requeued",
        true,
        *iterator,
    };
}

CudaJobQueueResult CudaJobQueue::find(
    const std::string& job_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto iterator = find_locked(job_id);
    if (iterator == records_.end()) {
        return {
            CudaJobQueueCode::job_not_found,
            "CUDA job was not found",
        };
    }

    return {
        CudaJobQueueCode::success,
        "CUDA job found",
        true,
        *iterator,
    };
}

CudaJobQueueSnapshot CudaJobQueue::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);

    CudaJobQueueSnapshot result{};
    result.total_records = records_.size();

    for (const CudaJobRecord& record : records_) {
        switch (record.state) {
            case CudaJobState::queued:
                ++result.queued;
                break;
            case CudaJobState::claimed:
                ++result.claimed;
                break;
            case CudaJobState::running:
                ++result.running;
                break;
            case CudaJobState::completed:
                ++result.completed;
                break;
            case CudaJobState::failed:
                ++result.failed;
                break;
            case CudaJobState::cancelled:
                ++result.cancelled;
                break;
        }
    }

    return result;
}

std::size_t CudaJobQueue::purge_terminal() {
    std::lock_guard<std::mutex> lock(mutex_);

    const std::size_t before = records_.size();
    records_.erase(
        std::remove_if(
            records_.begin(),
            records_.end(),
            [](const CudaJobRecord& record) {
                return is_terminal(record.state);
            }),
        records_.end());
    return before - records_.size();
}

void CudaJobQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.clear();
}

std::size_t CudaJobQueue::active_job_count_locked() const noexcept {
    return static_cast<std::size_t>(
        std::count_if(
            records_.begin(),
            records_.end(),
            [](const CudaJobRecord& record) {
                return !is_terminal(record.state);
            }));
}

std::vector<CudaJobRecord>::iterator CudaJobQueue::find_locked(
    const std::string& job_id) {
    return std::find_if(
        records_.begin(),
        records_.end(),
        [&job_id](const CudaJobRecord& record) {
            return record.descriptor.job_id == job_id;
        });
}

std::vector<CudaJobRecord>::const_iterator CudaJobQueue::find_locked(
    const std::string& job_id) const {
    return std::find_if(
        records_.begin(),
        records_.end(),
        [&job_id](const CudaJobRecord& record) {
            return record.descriptor.job_id == job_id;
        });
}

void CudaJobQueue::enforce_terminal_retention_locked() {
    std::size_t terminal_count = static_cast<std::size_t>(
        std::count_if(
            records_.begin(),
            records_.end(),
            [](const CudaJobRecord& record) {
                return is_terminal(record.state);
            }));

    while (terminal_count >
           options_.maximum_retained_terminal_jobs) {
        auto oldest = records_.end();

        for (auto iterator = records_.begin();
             iterator != records_.end();
             ++iterator) {
            if (!is_terminal(iterator->state)) {
                continue;
            }
            if (oldest == records_.end() ||
                iterator->finished_unix_ns <
                    oldest->finished_unix_ns) {
                oldest = iterator;
            }
        }

        if (oldest == records_.end()) {
            break;
        }

        records_.erase(oldest);
        --terminal_count;
    }
}

const char* to_string(CudaJobPriority value) noexcept {
    switch (value) {
        case CudaJobPriority::background:
            return "background";
        case CudaJobPriority::low:
            return "low";
        case CudaJobPriority::normal:
            return "normal";
        case CudaJobPriority::high:
            return "high";
        case CudaJobPriority::urgent:
            return "urgent";
    }
    return "unknown";
}

const char* to_string(CudaJobState value) noexcept {
    switch (value) {
        case CudaJobState::queued:
            return "queued";
        case CudaJobState::claimed:
            return "claimed";
        case CudaJobState::running:
            return "running";
        case CudaJobState::completed:
            return "completed";
        case CudaJobState::failed:
            return "failed";
        case CudaJobState::cancelled:
            return "cancelled";
    }
    return "unknown";
}

const char* to_string(CudaJobQueueCode value) noexcept {
    switch (value) {
        case CudaJobQueueCode::success:
            return "success";
        case CudaJobQueueCode::no_job_available:
            return "no_job_available";
        case CudaJobQueueCode::invalid_descriptor:
            return "invalid_descriptor";
        case CudaJobQueueCode::duplicate_job_id:
            return "duplicate_job_id";
        case CudaJobQueueCode::capacity_reached:
            return "capacity_reached";
        case CudaJobQueueCode::unknown_device:
            return "unknown_device";
        case CudaJobQueueCode::device_not_ready:
            return "device_not_ready";
        case CudaJobQueueCode::kernel_not_ready:
            return "kernel_not_ready";
        case CudaJobQueueCode::job_not_found:
            return "job_not_found";
        case CudaJobQueueCode::invalid_state:
            return "invalid_state";
        case CudaJobQueueCode::claim_token_mismatch:
            return "claim_token_mismatch";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cuda
