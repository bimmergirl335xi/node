#include "cuda_device_worker.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace prometheus::backends::cuda {
namespace {

[[nodiscard]] std::string bounded_text(
    std::string value,
    std::size_t maximum_bytes) {
    if (value.size() > maximum_bytes) {
        value.resize(maximum_bytes);
    }
    return value;
}

[[nodiscard]] std::string cuda_error_message(
    cudaError_t error,
    const char* operation,
    std::size_t maximum_bytes) {
    std::string message = operation;
    message += " failed";
    const char* name = cudaGetErrorName(error);
    const char* description = cudaGetErrorString(error);
    if (name != nullptr) {
        message += " (";
        message += name;
        message += ')';
    }
    if (description != nullptr) {
        message += ": ";
        message += description;
    }
    return bounded_text(std::move(message), maximum_bytes);
}

[[nodiscard]] bool terminal_state(CudaJobState state) noexcept {
    return state == CudaJobState::completed ||
           state == CudaJobState::failed ||
           state == CudaJobState::cancelled;
}

void increment_saturated(std::uint64_t& value) noexcept {
    if (value != std::numeric_limits<std::uint64_t>::max()) {
        ++value;
    }
}

}  // namespace

CudaDeviceWorker::CudaDeviceWorker(
    std::string persistent_device_key,
    CudaDevicePool device_pool_snapshot,
    CudaJobQueue& queue,
    const CudaKernelRegistry& metadata_registry,
    CudaDeviceWorkerOptions options)
    : persistent_device_key_(std::move(persistent_device_key)),
      device_pool_snapshot_(std::move(device_pool_snapshot)),
      queue_(queue),
      metadata_registry_(metadata_registry),
      options_(std::move(options)),
      adapter_registry_(options_.adapter_registry) {}

CudaDeviceWorker::~CudaDeviceWorker() noexcept {
    (void)stop();
}

CudaDeviceWorkerResult CudaDeviceWorker::register_adapter(
    std::unique_ptr<CudaKernelLaunchAdapter> adapter) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ != CudaDeviceWorkerState::constructed &&
            state_ != CudaDeviceWorkerState::stopped) {
            return make_result(
                CudaDeviceWorkerCode::invalid_state,
                "CUDA adapters may be registered only while the worker is inactive");
        }
        const CudaKernelAdapterResult result =
            adapter_registry_.register_adapter(std::move(adapter));
        if (!result.ok()) {
            return make_result(
                CudaDeviceWorkerCode::adapter_registration_failure,
                result.message,
                result.native_error_code);
        }
        return {};
    } catch (...) {
        return {CudaDeviceWorkerCode::resource_exhausted};
    }
}

CudaDeviceWorkerResult CudaDeviceWorker::start() noexcept {
    try {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!options_valid()) {
            return make_result(
                CudaDeviceWorkerCode::invalid_worker_configuration,
                "CUDA device worker bounds or durable device key are invalid");
        }
        if (state_ != CudaDeviceWorkerState::constructed &&
            state_ != CudaDeviceWorkerState::stopped) {
            return make_result(
                CudaDeviceWorkerCode::invalid_state,
                "CUDA device worker is already active or failed");
        }
        if (worker_thread_.joinable()) {
            return make_result(
                CudaDeviceWorkerCode::invalid_state,
                "CUDA device worker retained a joinable prior thread");
        }

        const CudaPoolDevice* device =
            device_pool_snapshot_.find_device(persistent_device_key_);
        if (device == nullptr) {
            return make_result(
                CudaDeviceWorkerCode::unknown_device,
                "Durable CUDA device key is absent from the supplied pool snapshot");
        }
        if (!device->registration_ready()) {
            return make_result(
                CudaDeviceWorkerCode::device_not_registration_ready,
                "CUDA device is not registration-ready");
        }
        if (!device->runtime_binding_usable()) {
            return make_result(
                CudaDeviceWorkerCode::runtime_binding_unusable,
                "CUDA device runtime binding is not usable");
        }
        const int ordinal = device->discovered.identity.runtime_ordinal;
        if (ordinal < 0) {
            return make_result(
                CudaDeviceWorkerCode::runtime_binding_unusable,
                "CUDA device pool did not retain a usable runtime ordinal snapshot");
        }

        state_ = CudaDeviceWorkerState::starting;
        accepting_submissions_ = false;
        stop_requested_ = false;
        notification_pending_ = false;
        waiting_for_work_ = false;
        start_completed_ = false;
        start_result_ = {};
        runtime_ordinal_snapshot_ = ordinal;
        current_job_id_.clear();
        stream_created_ = false;
        event_created_ = false;
        stream_destroyed_ = true;
        event_destroyed_ = true;
        adapter_resources_destroyed_ = true;

        try {
            worker_thread_ = std::thread(
                &CudaDeviceWorker::thread_main,
                this,
                ordinal);
            thread_joined_ = false;
        } catch (...) {
            state_ = CudaDeviceWorkerState::failed;
            runtime_ordinal_snapshot_ = -1;
            thread_joined_ = true;
            return {CudaDeviceWorkerCode::resource_exhausted};
        }

        start_condition_.wait(lock, [this] { return start_completed_; });
        const CudaDeviceWorkerResult result = start_result_;
        const bool must_join = !result.ok() && worker_thread_.joinable();
        lock.unlock();

        if (must_join) {
            worker_thread_.join();
            std::lock_guard<std::mutex> joined_lock(mutex_);
            thread_joined_ = true;
        }
        return result;
    } catch (...) {
        return {CudaDeviceWorkerCode::resource_exhausted};
    }
}

CudaDeviceWorkerResult CudaDeviceWorker::stop() noexcept {
    try {
        bool join_thread = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (state_ == CudaDeviceWorkerState::constructed) {
                state_ = CudaDeviceWorkerState::stopped;
                accepting_submissions_ = false;
                thread_joined_ = true;
                return {};
            }
            if (state_ == CudaDeviceWorkerState::starting) {
                return make_result(
                    CudaDeviceWorkerCode::invalid_state,
                    "CUDA device worker start is still in progress");
            }
            if (state_ == CudaDeviceWorkerState::stopped &&
                !worker_thread_.joinable()) {
                thread_joined_ = true;
                return {};
            }

            accepting_submissions_ = false;
            stop_requested_ = true;
            notification_pending_ = true;
            if (state_ != CudaDeviceWorkerState::failed) {
                state_ = CudaDeviceWorkerState::stopping;
            }
            if (notification_generation_ !=
                std::numeric_limits<std::uint64_t>::max()) {
                ++notification_generation_;
            }
            join_thread = worker_thread_.joinable();
        }
        wake_condition_.notify_all();

        if (join_thread) {
            worker_thread_.join();
            std::lock_guard<std::mutex> lock(mutex_);
            thread_joined_ = true;
        }

        const CudaDeviceWorkerSnapshot final_snapshot = snapshot();
        if (final_snapshot.state == CudaDeviceWorkerState::failed) {
            if (final_snapshot.has_last_issue) {
                return make_result(
                    final_snapshot.last_issue.code,
                    final_snapshot.last_issue.message,
                    final_snapshot.last_issue.native_error_code);
            }
            return make_result(
                CudaDeviceWorkerCode::worker_stopped,
                "CUDA device worker stopped after a fatal failure");
        }
        return {};
    } catch (...) {
        return {CudaDeviceWorkerCode::resource_exhausted};
    }
}

CudaDeviceWorkerResult CudaDeviceWorker::submit(
    CudaJobDescriptor descriptor) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        if (descriptor.target_device_key != persistent_device_key_) {
            return make_result(
                CudaDeviceWorkerCode::wrong_device,
                "CUDA job targets a different durable device key");
        }
        if (descriptor.job_id.size() >
                options_.maximum_job_identifier_bytes ||
            descriptor.job_id.size() >
                kCudaWorkerJobIdentifierAbsoluteMaximum) {
            return make_result(
                CudaDeviceWorkerCode::queue_rejected,
                "CUDA job ID exceeds the worker snapshot bound");
        }
        if (!accepting_submissions_ || stop_requested_) {
            return make_result(
                CudaDeviceWorkerCode::not_accepting,
                "CUDA device worker is not accepting submissions");
        }

        CudaJobQueueResult queued = queue_.submit(
            std::move(descriptor),
            device_pool_snapshot_,
            metadata_registry_);
        if (!queued.ok()) {
            CudaDeviceWorkerResult result = make_result(
                CudaDeviceWorkerCode::queue_rejected,
                queued.message,
                0,
                queued.code);
            result.has_job = queued.has_job;
            result.job = std::move(queued.job);
            return result;
        }

        notification_pending_ = true;
        if (notification_generation_ !=
            std::numeric_limits<std::uint64_t>::max()) {
            ++notification_generation_;
        }
        CudaDeviceWorkerResult result{};
        result.has_job = true;
        result.job = std::move(queued.job);
        wake_condition_.notify_one();
        return result;
    } catch (...) {
        return {CudaDeviceWorkerCode::resource_exhausted};
    }
}

CudaDeviceWorkerResult CudaDeviceWorker::wait_for_terminal(
    const std::string& job_id,
    std::chrono::milliseconds timeout) noexcept {
    try {
        if (job_id.empty() ||
            job_id.size() > options_.maximum_job_identifier_bytes ||
            timeout.count() < 0 ||
            timeout.count() > kCudaWorkerWaitAbsoluteMaximumMs) {
            return make_result(
                CudaDeviceWorkerCode::invalid_worker_configuration,
                "CUDA terminal wait arguments are invalid");
        }

        std::unique_lock<std::mutex> lock(mutex_);
        CudaJobQueueResult found{};
        const auto terminal_or_unavailable = [&] {
            found = queue_.find(job_id);
            return (found.ok() && found.has_job &&
                    terminal_state(found.job.state)) ||
                   (state_ == CudaDeviceWorkerState::failed &&
                    current_job_id_ != job_id) ||
                   (state_ == CudaDeviceWorkerState::stopped &&
                    current_job_id_ != job_id);
        };

        if (!terminal_condition_.wait_for(
                lock, timeout, terminal_or_unavailable)) {
            return make_result(
                CudaDeviceWorkerCode::timeout,
                "Timed out waiting for CUDA job terminal state");
        }
        found = queue_.find(job_id);
        if (!found.ok() || !found.has_job ||
            !terminal_state(found.job.state)) {
            return make_result(
                CudaDeviceWorkerCode::worker_stopped,
                "CUDA worker stopped before the requested job became terminal",
                0,
                found.code);
        }

        CudaDeviceWorkerResult result{};
        result.has_job = true;
        result.job = std::move(found.job);
        return result;
    } catch (...) {
        return {CudaDeviceWorkerCode::resource_exhausted};
    }
}

CudaDeviceWorkerResult CudaDeviceWorker::wait_until_idle(
    std::chrono::milliseconds timeout) noexcept {
    try {
        if (timeout.count() < 0 ||
            timeout.count() > kCudaWorkerWaitAbsoluteMaximumMs) {
            return make_result(
                CudaDeviceWorkerCode::invalid_worker_configuration,
                "CUDA idle wait timeout is invalid");
        }
        std::unique_lock<std::mutex> lock(mutex_);
        const bool ready = state_condition_.wait_for(lock, timeout, [this] {
            return (state_ == CudaDeviceWorkerState::idle &&
                    waiting_for_work_) ||
                   state_ == CudaDeviceWorkerState::failed ||
                   state_ == CudaDeviceWorkerState::stopped;
        });
        if (!ready) {
            return make_result(
                CudaDeviceWorkerCode::timeout,
                "Timed out waiting for CUDA worker idle state");
        }
        if (state_ != CudaDeviceWorkerState::idle || !waiting_for_work_) {
            return make_result(
                CudaDeviceWorkerCode::worker_stopped,
                "CUDA worker stopped or failed before reaching idle wait");
        }
        return {};
    } catch (...) {
        return {CudaDeviceWorkerCode::resource_exhausted};
    }
}

CudaDeviceWorkerSnapshot CudaDeviceWorker::snapshot() const noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        CudaDeviceWorkerSnapshot result{};
        result.persistent_device_key = persistent_device_key_;
        result.runtime_ordinal_snapshot = runtime_ordinal_snapshot_;
        result.state = state_;
        result.accepting_submissions = accepting_submissions_;
        result.stream_created = stream_created_;
        result.event_created = event_created_;
        result.stream_destroyed = stream_destroyed_;
        result.event_destroyed = event_destroyed_;
        result.adapter_resources_destroyed = adapter_resources_destroyed_;
        result.thread_joined = thread_joined_;
        result.waiting_for_work = waiting_for_work_;
        result.adapter_count = adapter_registry_.size();
        result.notification_generation = notification_generation_;
        result.claimed_job_count = claimed_job_count_;
        result.started_job_count = started_job_count_;
        result.completed_job_count = completed_job_count_;
        result.failed_job_count = failed_job_count_;
        result.has_current_job = !current_job_id_.empty();
        result.current_job_id = current_job_id_;
        result.retained_issues = issues_;
        if (!issues_.empty()) {
            result.has_last_issue = true;
            result.last_issue = issues_.back();
        }
        return result;
    } catch (...) {
        return {};
    }
}

bool CudaDeviceWorker::options_valid() const noexcept {
    return !persistent_device_key_.empty() &&
           persistent_device_key_.size() <= options_.maximum_device_key_bytes &&
           options_.maximum_device_key_bytes > 0 &&
           options_.maximum_device_key_bytes <=
               kCudaWorkerDeviceKeyAbsoluteMaximum &&
           options_.maximum_retained_issues > 0 &&
           options_.maximum_retained_issues <=
               kCudaWorkerIssueCountAbsoluteMaximum &&
           options_.maximum_issue_message_bytes > 0 &&
           options_.maximum_issue_message_bytes <=
               kCudaWorkerIssueMessageAbsoluteMaximum &&
           options_.maximum_job_identifier_bytes > 0 &&
           options_.maximum_job_identifier_bytes <=
               kCudaWorkerJobIdentifierAbsoluteMaximum;
}

CudaDeviceWorkerResult CudaDeviceWorker::make_result(
    CudaDeviceWorkerCode code,
    std::string message,
    int native_error_code,
    CudaJobQueueCode queue_code) const noexcept {
    try {
        return {
            code,
            queue_code,
            native_error_code,
            bounded_text(
                std::move(message),
                options_.maximum_issue_message_bytes),
        };
    } catch (...) {
        return {code, queue_code, native_error_code};
    }
}

void CudaDeviceWorker::thread_main(int runtime_ordinal_snapshot) noexcept {
    try {
        cudaError_t status = cudaSetDevice(runtime_ordinal_snapshot);
        if (status != cudaSuccess) {
            cleanup_device_resources(true);
            publish_start_failure(
                CudaDeviceWorkerCode::cuda_set_device_failure,
                cuda_error_message(
                    status,
                    "cudaSetDevice",
                    options_.maximum_issue_message_bytes),
                static_cast<int>(status));
            return;
        }

        status = cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking);
        if (status != cudaSuccess) {
            cleanup_device_resources(true);
            publish_start_failure(
                CudaDeviceWorkerCode::stream_creation_failure,
                cuda_error_message(
                    status,
                    "cudaStreamCreateWithFlags",
                    options_.maximum_issue_message_bytes),
                static_cast<int>(status));
            return;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stream_created_ = true;
            stream_destroyed_ = false;
        }

        status = cudaEventCreateWithFlags(
            &completion_event_, cudaEventDisableTiming);
        if (status != cudaSuccess) {
            cleanup_device_resources(true);
            publish_start_failure(
                CudaDeviceWorkerCode::event_creation_failure,
                cuda_error_message(
                    status,
                    "cudaEventCreateWithFlags",
                    options_.maximum_issue_message_bytes),
                static_cast<int>(status));
            return;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            event_created_ = true;
            event_destroyed_ = false;
        }

        CudaKernelAdapterDeviceContext context{};
        context.persistent_device_key = persistent_device_key_;
        context.runtime_ordinal_snapshot = runtime_ordinal_snapshot;
        context.stream = stream_;
        const CudaKernelAdapterResult adapter_initialization =
            adapter_registry_.initialize_all(context);
        if (!adapter_initialization.ok()) {
            cleanup_device_resources(true);
            publish_start_failure(
                CudaDeviceWorkerCode::adapter_initialization_failure,
                adapter_initialization.message,
                adapter_initialization.native_error_code);
            return;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            adapter_resources_destroyed_ = false;
            state_ = CudaDeviceWorkerState::idle;
            accepting_submissions_ = true;
            start_result_ = {};
            start_completed_ = true;
        }
        start_condition_.notify_all();

        bool worker_failed = false;
        while (true) {
            CudaJobQueueResult claim{};
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (stop_requested_) {
                    break;
                }

                state_ = CudaDeviceWorkerState::idle;
                notification_pending_ = false;
                claim = queue_.claim_next(persistent_device_key_);
                if (claim.code == CudaJobQueueCode::no_job_available) {
                    waiting_for_work_ = true;
                    state_condition_.notify_all();
                    wake_condition_.wait(lock, [this] {
                        return stop_requested_ || notification_pending_;
                    });
                    waiting_for_work_ = false;
                    continue;
                }
                if (!claim.ok() || !claim.has_job) {
                    worker_failed = true;
                    accepting_submissions_ = false;
                    stop_requested_ = true;
                } else if (
                    claim.job.descriptor.job_id.size() >
                    options_.maximum_job_identifier_bytes) {
                    worker_failed = true;
                    accepting_submissions_ = false;
                    stop_requested_ = true;
                } else {
                    state_ = CudaDeviceWorkerState::executing;
                    current_job_id_ = claim.job.descriptor.job_id;
                    increment_saturated(claimed_job_count_);
                }
            }

            if (worker_failed) {
                if (claim.ok() && claim.has_job &&
                    claim.job.claim_token != 0) {
                    (void)queue_.requeue_claim(
                        claim.job.descriptor.job_id,
                        claim.job.claim_token,
                        "Worker rejected an overlength claimed job ID");
                }
                publish_issue(
                    CudaDeviceWorkerCode::invalid_worker_configuration,
                    {},
                    "CUDA worker received an invalid claim from the shared queue",
                    0,
                    true);
                break;
            }

            const bool job_fatal = execute_claimed_job(claim.job);
            clear_current_job(job_fatal);
            if (job_fatal) {
                worker_failed = true;
                break;
            }
        }

        cleanup_device_resources(worker_failed);
    } catch (...) {
        publish_issue(
            CudaDeviceWorkerCode::resource_exhausted,
            {},
            "Unhandled exception was contained in the CUDA worker thread",
            0,
            true);
        cleanup_device_resources(true);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!start_completed_) {
                start_result_ = {
                    CudaDeviceWorkerCode::resource_exhausted};
                start_completed_ = true;
            }
        }
        start_condition_.notify_all();
    }
}

bool CudaDeviceWorker::execute_claimed_job(
    const CudaJobRecord& job) noexcept {
    try {
        const CudaJobDescriptor& descriptor = job.descriptor;
        if (descriptor.target_device_key != persistent_device_key_ ||
            job.claim_token == 0 || descriptor.kernel_id.empty()) {
            (void)queue_.requeue_claim(
                descriptor.job_id,
                job.claim_token,
                "CUDA worker rejected an invalid claim");
            publish_issue(
                CudaDeviceWorkerCode::invalid_worker_configuration,
                descriptor.job_id,
                "CUDA worker claim invariants were violated",
                0,
                true);
            return true;
        }

        CudaDeviceWorkerCode readiness_code =
            CudaDeviceWorkerCode::success;
        std::string readiness_message{};
        CudaKernelLaunchAdapter* adapter = nullptr;

        const CudaPoolDevice* device =
            device_pool_snapshot_.find_device(persistent_device_key_);
        if (device == nullptr || !device->discovered.capabilities_available) {
            readiness_code = CudaDeviceWorkerCode::metadata_not_executable;
            readiness_message =
                "CUDA device capabilities are unavailable for metadata evaluation";
        } else {
            const CudaKernelCoverageReport coverage =
                metadata_registry_.evaluate_device_coverage(
                    device->discovered.capabilities,
                    {descriptor.kernel_id});
            if (coverage.state != CudaKernelCoverageState::complete) {
                readiness_code =
                    CudaDeviceWorkerCode::metadata_not_executable;
                readiness_message =
                    "CUDA kernel metadata is not executable for this device";
            }
        }

        if (readiness_code == CudaDeviceWorkerCode::success) {
            adapter = adapter_registry_.find(descriptor.kernel_id);
            if (adapter == nullptr || !adapter->initialized()) {
                readiness_code = CudaDeviceWorkerCode::adapter_not_found;
                readiness_message =
                    "No initialized device-local adapter matches the exact kernel ID";
            }
        }

        const CudaJobQueueResult running = queue_.mark_running(
            descriptor.job_id,
            job.claim_token);
        if (!running.ok()) {
            (void)queue_.requeue_claim(
                descriptor.job_id,
                job.claim_token,
                "CUDA worker could not mark the claim running");
            publish_issue(
                CudaDeviceWorkerCode::mark_running_failure,
                descriptor.job_id,
                running.message,
                0,
                true);
            return true;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            increment_saturated(started_job_count_);
        }

        if (readiness_code != CudaDeviceWorkerCode::success) {
            return fail_running_job(
                job,
                readiness_code,
                std::move(readiness_message),
                0,
                false);
        }

        const CudaKernelAdapterResult launched =
            adapter->launch(descriptor, stream_);
        if (!launched.ok()) {
            return fail_running_job(
                job,
                CudaDeviceWorkerCode::launch_failure,
                launched.message,
                launched.native_error_code,
                launched.worker_fatal);
        }

        cudaError_t status = cudaEventRecord(completion_event_, stream_);
        if (status != cudaSuccess) {
            return fail_running_job(
                job,
                CudaDeviceWorkerCode::event_record_failure,
                cuda_error_message(
                    status,
                    "cudaEventRecord",
                    options_.maximum_issue_message_bytes),
                static_cast<int>(status),
                true);
        }

        status = cudaEventSynchronize(completion_event_);
        if (status != cudaSuccess) {
            return fail_running_job(
                job,
                CudaDeviceWorkerCode::completion_failure,
                cuda_error_message(
                    status,
                    "cudaEventSynchronize",
                    options_.maximum_issue_message_bytes),
                static_cast<int>(status),
                true);
        }

        const CudaKernelAdapterResult validation =
            adapter->validate_completion();
        if (!validation.ok()) {
            return fail_running_job(
                job,
                CudaDeviceWorkerCode::output_validation_failure,
                validation.message,
                validation.native_error_code,
                validation.worker_fatal);
        }

        const CudaJobQueueResult completed = queue_.complete(
            descriptor.job_id,
            job.claim_token,
            "CUDA event completed and adapter output validated");
        if (!completed.ok()) {
            publish_issue(
                CudaDeviceWorkerCode::queue_terminal_transition_failure,
                descriptor.job_id,
                completed.message,
                0,
                true);
            notify_terminal();
            return true;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            increment_saturated(completed_job_count_);
        }
        notify_terminal();
        return false;
    } catch (...) {
        return fail_running_job(
            job,
            CudaDeviceWorkerCode::resource_exhausted,
            "Exception was contained while executing a CUDA job",
            0,
            true);
    }
}

bool CudaDeviceWorker::fail_running_job(
    const CudaJobRecord& job,
    CudaDeviceWorkerCode code,
    std::string message,
    int native_error_code,
    bool worker_fatal) noexcept {
    try {
        message = bounded_text(
            std::move(message),
            options_.maximum_issue_message_bytes);
        const CudaJobQueueResult failed = queue_.fail(
            job.descriptor.job_id,
            job.claim_token,
            message);
        if (!failed.ok()) {
            publish_issue(
                CudaDeviceWorkerCode::queue_terminal_transition_failure,
                job.descriptor.job_id,
                failed.message,
                0,
                true);
            notify_terminal();
            return true;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            increment_saturated(failed_job_count_);
        }
        publish_issue(
            code,
            job.descriptor.job_id,
            std::move(message),
            native_error_code,
            worker_fatal);
        notify_terminal();
        return worker_fatal;
    } catch (...) {
        publish_issue(
            CudaDeviceWorkerCode::resource_exhausted,
            {},
            "Exception was contained while failing a CUDA job",
            0,
            true);
        notify_terminal();
        return true;
    }
}

void CudaDeviceWorker::publish_issue(
    CudaDeviceWorkerCode code,
    std::string job_id,
    std::string message,
    int native_error_code,
    bool worker_fatal) noexcept {
    try {
        if (job_id.size() > options_.maximum_job_identifier_bytes) {
            job_id.clear();
        }
        message = bounded_text(
            std::move(message),
            options_.maximum_issue_message_bytes);
        std::lock_guard<std::mutex> lock(mutex_);
        if (issues_.size() >= options_.maximum_retained_issues) {
            issues_.erase(issues_.begin());
        }
        issues_.push_back(
            {code,
             native_error_code,
             worker_fatal,
             std::move(job_id),
             std::move(message)});
        if (worker_fatal) {
            accepting_submissions_ = false;
            stop_requested_ = true;
            notification_pending_ = true;
        }
    } catch (...) {
        std::lock_guard<std::mutex> lock(mutex_);
        accepting_submissions_ = false;
        stop_requested_ = true;
        notification_pending_ = true;
        state_ = CudaDeviceWorkerState::failed;
    }
}

void CudaDeviceWorker::publish_start_failure(
    CudaDeviceWorkerCode code,
    std::string message,
    int native_error_code) noexcept {
    publish_issue(
        code, {}, message, native_error_code, true);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = CudaDeviceWorkerState::failed;
        accepting_submissions_ = false;
        start_result_ = make_result(code, std::move(message), native_error_code);
        start_completed_ = true;
    }
    start_condition_.notify_all();
    terminal_condition_.notify_all();
}

void CudaDeviceWorker::cleanup_device_resources(bool worker_failed) noexcept {
    const CudaKernelAdapterResult adapter_shutdown =
        adapter_registry_.shutdown_all();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        adapter_resources_destroyed_ = adapter_shutdown.ok();
    }
    if (!adapter_shutdown.ok()) {
        worker_failed = true;
        publish_issue(
            CudaDeviceWorkerCode::adapter_shutdown_failure,
            {},
            adapter_shutdown.message,
            adapter_shutdown.native_error_code,
            true);
    }

    if (completion_event_ != nullptr) {
        const cudaError_t status = cudaEventDestroy(completion_event_);
        completion_event_ = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            event_created_ = false;
            event_destroyed_ = status == cudaSuccess;
        }
        if (status != cudaSuccess) {
            worker_failed = true;
            publish_issue(
                CudaDeviceWorkerCode::event_destroy_failure,
                {},
                cuda_error_message(
                    status,
                    "cudaEventDestroy",
                    options_.maximum_issue_message_bytes),
                static_cast<int>(status),
                true);
        }
    }

    if (stream_ != nullptr) {
        const cudaError_t status = cudaStreamDestroy(stream_);
        stream_ = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stream_created_ = false;
            stream_destroyed_ = status == cudaSuccess;
        }
        if (status != cudaSuccess) {
            worker_failed = true;
            publish_issue(
                CudaDeviceWorkerCode::stream_destroy_failure,
                {},
                cuda_error_message(
                    status,
                    "cudaStreamDestroy",
                    options_.maximum_issue_message_bytes),
                static_cast<int>(status),
                true);
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        accepting_submissions_ = false;
        current_job_id_.clear();
        state_ = worker_failed ? CudaDeviceWorkerState::failed
                               : CudaDeviceWorkerState::stopped;
        waiting_for_work_ = false;
    }
    terminal_condition_.notify_all();
    wake_condition_.notify_all();
    state_condition_.notify_all();
}

void CudaDeviceWorker::notify_terminal() noexcept {
    terminal_condition_.notify_all();
}

void CudaDeviceWorker::clear_current_job(bool worker_failed) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    current_job_id_.clear();
    if (worker_failed) {
        accepting_submissions_ = false;
        stop_requested_ = true;
        notification_pending_ = true;
        state_ = CudaDeviceWorkerState::failed;
    } else if (stop_requested_) {
        state_ = CudaDeviceWorkerState::stopping;
    } else {
        state_ = CudaDeviceWorkerState::idle;
    }
    state_condition_.notify_all();
}

const char* to_string(CudaDeviceWorkerState value) noexcept {
    switch (value) {
        case CudaDeviceWorkerState::constructed:
            return "constructed";
        case CudaDeviceWorkerState::starting:
            return "starting";
        case CudaDeviceWorkerState::idle:
            return "idle";
        case CudaDeviceWorkerState::executing:
            return "executing";
        case CudaDeviceWorkerState::stopping:
            return "stopping";
        case CudaDeviceWorkerState::stopped:
            return "stopped";
        case CudaDeviceWorkerState::failed:
            return "failed";
    }
    return "unknown";
}

const char* to_string(CudaDeviceWorkerCode value) noexcept {
    switch (value) {
        case CudaDeviceWorkerCode::success:
            return "success";
        case CudaDeviceWorkerCode::invalid_worker_configuration:
            return "invalid_worker_configuration";
        case CudaDeviceWorkerCode::invalid_state:
            return "invalid_state";
        case CudaDeviceWorkerCode::unknown_device:
            return "unknown_device";
        case CudaDeviceWorkerCode::device_not_registration_ready:
            return "device_not_registration_ready";
        case CudaDeviceWorkerCode::runtime_binding_unusable:
            return "runtime_binding_unusable";
        case CudaDeviceWorkerCode::cuda_set_device_failure:
            return "cuda_set_device_failure";
        case CudaDeviceWorkerCode::stream_creation_failure:
            return "stream_creation_failure";
        case CudaDeviceWorkerCode::event_creation_failure:
            return "event_creation_failure";
        case CudaDeviceWorkerCode::adapter_registration_failure:
            return "adapter_registration_failure";
        case CudaDeviceWorkerCode::adapter_initialization_failure:
            return "adapter_initialization_failure";
        case CudaDeviceWorkerCode::adapter_not_found:
            return "adapter_not_found";
        case CudaDeviceWorkerCode::metadata_not_executable:
            return "metadata_not_executable";
        case CudaDeviceWorkerCode::mark_running_failure:
            return "mark_running_failure";
        case CudaDeviceWorkerCode::launch_failure:
            return "launch_failure";
        case CudaDeviceWorkerCode::event_record_failure:
            return "event_record_failure";
        case CudaDeviceWorkerCode::completion_failure:
            return "completion_failure";
        case CudaDeviceWorkerCode::output_validation_failure:
            return "output_validation_failure";
        case CudaDeviceWorkerCode::queue_terminal_transition_failure:
            return "queue_terminal_transition_failure";
        case CudaDeviceWorkerCode::adapter_shutdown_failure:
            return "adapter_shutdown_failure";
        case CudaDeviceWorkerCode::event_destroy_failure:
            return "event_destroy_failure";
        case CudaDeviceWorkerCode::stream_destroy_failure:
            return "stream_destroy_failure";
        case CudaDeviceWorkerCode::wrong_device:
            return "wrong_device";
        case CudaDeviceWorkerCode::not_accepting:
            return "not_accepting";
        case CudaDeviceWorkerCode::queue_rejected:
            return "queue_rejected";
        case CudaDeviceWorkerCode::worker_stopped:
            return "worker_stopped";
        case CudaDeviceWorkerCode::timeout:
            return "timeout";
        case CudaDeviceWorkerCode::resource_exhausted:
            return "resource_exhausted";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cuda
