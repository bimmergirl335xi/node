#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "cuda_device_worker.hpp"

namespace pc = prometheus::backends::cuda;
using namespace std::chrono_literals;

namespace {

enum class TestAdapterBehavior {
    success,
    initialization_failure,
    validation_mismatch,
    launch_failure,
};

class TestAdapter final : public pc::CudaKernelLaunchAdapter {
public:
    TestAdapter(std::string kernel_id, TestAdapterBehavior behavior)
        : kernel_id_(std::move(kernel_id)), behavior_(behavior) {}

    [[nodiscard]] std::string_view kernel_id() const noexcept override {
        return kernel_id_;
    }

    [[nodiscard]] std::string_view adapter_name() const noexcept override {
        return "gpu_7_2a_test_adapter";
    }

    [[nodiscard]] bool initialized() const noexcept override {
        return initialized_;
    }

    [[nodiscard]] pc::CudaKernelAdapterResult initialize(
        const pc::CudaKernelAdapterDeviceContext& context) noexcept override {
        if (behavior_ == TestAdapterBehavior::initialization_failure) {
            return {
                pc::CudaKernelAdapterCode::initialization_failed,
                0,
                true,
                "Controlled adapter initialization failure",
            };
        }
        if (context.runtime_ordinal_snapshot < 0 || context.stream == nullptr ||
            context.persistent_device_key.empty()) {
            return {pc::CudaKernelAdapterCode::invalid_context};
        }
        stream_ = context.stream;
        initialized_ = true;
        return {};
    }

    [[nodiscard]] pc::CudaKernelAdapterResult launch(
        const pc::CudaJobDescriptor& job,
        cudaStream_t stream) noexcept override {
        if (!initialized_ || stream == nullptr || stream != stream_ ||
            job.kernel_id != kernel_id_) {
            return {
                pc::CudaKernelAdapterCode::invalid_context,
                0,
                false,
                "Test adapter launch context mismatch",
            };
        }
        if (behavior_ == TestAdapterBehavior::launch_failure) {
            return {
                pc::CudaKernelAdapterCode::launch_failed,
                0,
                false,
                std::string(2048, 'L'),
            };
        }
        launched_ = true;
        return {};
    }

    [[nodiscard]] pc::CudaKernelAdapterResult validate_completion()
        noexcept override {
        if (!launched_) {
            return {pc::CudaKernelAdapterCode::not_initialized};
        }
        launched_ = false;
        if (behavior_ == TestAdapterBehavior::validation_mismatch) {
            return {
                pc::CudaKernelAdapterCode::completion_validation_failed,
                0,
                false,
                "Controlled synthetic output mismatch",
            };
        }
        return {};
    }

    [[nodiscard]] pc::CudaKernelAdapterResult shutdown() noexcept override {
        stream_ = nullptr;
        initialized_ = false;
        launched_ = false;
        return {};
    }

private:
    std::string kernel_id_{};
    TestAdapterBehavior behavior_ = TestAdapterBehavior::success;
    cudaStream_t stream_ = nullptr;
    bool initialized_ = false;
    bool launched_ = false;
};

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

pc::CudaKernelDescriptor executable_descriptor(const std::string& id) {
    pc::CudaKernelDescriptor descriptor{};
    descriptor.kernel_id = id;
    descriptor.display_name = "GPU-7.2A controlled test adapter";
    descriptor.family = pc::CudaKernelFamily::activation;
    descriptor.scalar_types = {pc::CudaScalarType::fp32};
    descriptor.minimum_compute_capability = {6, 1};
    descriptor.cuda_12_release_support = pc::CudaSupportState::supported;
    descriptor.cuda_13_release_support = pc::CudaSupportState::supported;
    descriptor.binary_state = pc::CudaKernelBinaryState::available;
    descriptor.binding_state = pc::CudaKernelBindingState::adapter_available;
    descriptor.source_file =
        "tests/unit/backends/test_cuda_device_worker.cpp";
    descriptor.adapter_name = "gpu_7_2a_test_adapter";
    return descriptor;
}

pc::CudaJobDescriptor make_job(
    std::string job_id,
    const std::string& device_key,
    std::string kernel_id =
        std::string{pc::kCudaSyntheticExecutionKernelId}) {
    pc::CudaJobDescriptor job{};
    job.job_id = std::move(job_id);
    job.request_id = "gpu-7.2a-tests";
    job.target_device_key = device_key;
    job.kernel_id = std::move(kernel_id);
    return job;
}

pc::CudaDevicePool make_non_ready_pool() {
    pc::CudaDevicePool pool{};
    pc::CudaPoolDevice device{};
    device.discovered.identity.runtime_ordinal = 0;
    device.discovered.identity.uuid_available = true;
    device.discovered.identity.uuid =
        "GPU-00000000-0000-0000-0000-000000000001";
    device.discovered.identity.persistent_key =
        "cuda:GPU-00000000-0000-0000-0000-000000000001";
    device.discovered.capabilities_available = false;
    device.health_was_requested = true;
    device.health_available = true;
    device.health.status.code = pc::CudaHealthQueryCode::success;
    device.health.snapshot.runtime_binding =
        pc::CudaRuntimeBindingState::matched;
    pool.devices.push_back(std::move(device));
    return pool;
}

void test_adapter_registry() {
    pc::CudaKernelAdapterRegistryOptions options{};
    options.maximum_adapters = 2;
    options.maximum_identifier_bytes = 32;
    pc::CudaKernelAdapterRegistry registry{options};

    require(
        registry.register_adapter(nullptr).code ==
            pc::CudaKernelAdapterCode::invalid_adapter,
        "null adapter was not rejected");

    require(
        registry.register_adapter(std::make_unique<TestAdapter>(
            "test.adapter.b", TestAdapterBehavior::success)).ok(),
        "valid adapter B registration failed");
    require(
        registry.register_adapter(std::make_unique<TestAdapter>(
            "test.adapter.a", TestAdapterBehavior::success)).ok(),
        "valid adapter A registration failed");
    require(
        registry.register_adapter(std::make_unique<TestAdapter>(
            "test.adapter.a", TestAdapterBehavior::success)).code ==
            pc::CudaKernelAdapterCode::duplicate_kernel_id,
        "duplicate adapter ID was not rejected");
    require(
        registry.register_adapter(std::make_unique<TestAdapter>(
            "invalid adapter id", TestAdapterBehavior::success)).code ==
            pc::CudaKernelAdapterCode::invalid_kernel_id,
        "invalid adapter ID was not rejected");
    require(
        registry.register_adapter(std::make_unique<TestAdapter>(
            "test.adapter.c", TestAdapterBehavior::success)).code ==
            pc::CudaKernelAdapterCode::capacity_reached,
        "adapter capacity exhaustion was not explicit");
    require(registry.find("test.adapter.a") != nullptr,
            "exact adapter lookup failed");
    require(registry.find("test.adapter.missing") == nullptr,
            "missing adapter lookup did not remain missing");
    const auto inventory = registry.inventory();
    require(inventory.size() == 2 &&
                inventory[0].kernel_id == "test.adapter.a" &&
                inventory[1].kernel_id == "test.adapter.b",
            "adapter inventory was not deterministic");

    pc::CudaKernelAdapterRegistryOptions short_ids{};
    short_ids.maximum_identifier_bytes = 8;
    pc::CudaKernelAdapterRegistry bounded_registry{short_ids};
    require(
        bounded_registry.register_adapter(std::make_unique<TestAdapter>(
            std::string(64, 'a'), TestAdapterBehavior::success)).code ==
            pc::CudaKernelAdapterCode::invalid_kernel_id,
        "overlength authoritative adapter ID was not rejected");
}

void test_start_rejections() {
    pc::CudaJobQueue unknown_queue{};
    pc::CudaKernelRegistry unknown_metadata{};
    pc::CudaDeviceWorker unknown{
        "cuda:GPU-unknown", {}, unknown_queue, unknown_metadata};
    require(unknown.start().code == pc::CudaDeviceWorkerCode::unknown_device,
            "unknown device key did not reject worker start");
    auto snapshot = unknown.snapshot();
    require(!snapshot.stream_created && !snapshot.event_created &&
                snapshot.thread_joined,
            "unknown device start created resources or retained a thread");

    pc::CudaDevicePool non_ready = make_non_ready_pool();
    const std::string key = non_ready.devices.front().persistent_key();
    pc::CudaJobQueue non_ready_queue{};
    pc::CudaKernelRegistry non_ready_metadata{};
    pc::CudaDeviceWorker worker{
        key, non_ready, non_ready_queue, non_ready_metadata};
    require(
        worker.start().code ==
            pc::CudaDeviceWorkerCode::device_not_registration_ready,
        "non-registration-ready evidence did not reject worker start");
    snapshot = worker.snapshot();
    require(!snapshot.stream_created && !snapshot.event_created &&
                snapshot.thread_joined,
            "non-ready device start attempted CUDA execution resources");
}

void test_idle_start_stop(const pc::CudaDevicePool& pool) {
    pc::CudaJobQueue queue{};
    pc::CudaKernelRegistry metadata{};
    pc::CudaDeviceWorker worker{
        pool.devices.front().persistent_key(), pool, queue, metadata};
    require(worker.start().ok(), "idle worker start failed");
    require(worker.wait_until_idle(5s).ok(), "idle worker did not sleep");
    auto snapshot = worker.snapshot();
    require(snapshot.state == pc::CudaDeviceWorkerState::idle &&
                snapshot.stream_created && snapshot.event_created &&
                snapshot.waiting_for_work,
            "idle worker resources or state were incorrect");
    require(worker.stop().ok(), "idle worker stop failed");
    snapshot = worker.snapshot();
    require(snapshot.state == pc::CudaDeviceWorkerState::stopped &&
                snapshot.thread_joined && snapshot.stream_destroyed &&
                snapshot.event_destroyed &&
                snapshot.adapter_resources_destroyed,
            "idle worker cleanup was incomplete");
    const auto queue_snapshot = queue.snapshot();
    require(queue_snapshot.total_records == 0,
            "idle start/stop changed queue state");
}

void test_worker_fatal_start_boundaries(const pc::CudaDevicePool& pool) {
    const std::string key = pool.devices.front().persistent_key();

    pc::CudaDevicePool unusable = pool;
    unusable.devices.front().health.snapshot.runtime_binding =
        pc::CudaRuntimeBindingState::identity_mismatch;
    pc::CudaJobQueue unusable_queue{};
    pc::CudaKernelRegistry unusable_metadata{};
    pc::CudaDeviceWorker unusable_worker{
        key, unusable, unusable_queue, unusable_metadata};
    require(
        unusable_worker.start().code ==
            pc::CudaDeviceWorkerCode::runtime_binding_unusable,
        "unusable runtime binding did not reject worker start");
    require(unusable_worker.snapshot().thread_joined,
            "runtime-binding rejection retained a worker thread");

    pc::CudaDevicePool invalid_ordinal = pool;
    invalid_ordinal.devices.front().discovered.identity.runtime_ordinal =
        1000000;
    pc::CudaJobQueue ordinal_queue{};
    pc::CudaKernelRegistry ordinal_metadata{};
    pc::CudaDeviceWorker ordinal_worker{
        key, invalid_ordinal, ordinal_queue, ordinal_metadata};
    require(
        ordinal_worker.start().code ==
            pc::CudaDeviceWorkerCode::cuda_set_device_failure,
        "cudaSetDevice failure was not classified as worker-fatal");
    const auto ordinal_snapshot = ordinal_worker.snapshot();
    require(ordinal_snapshot.state == pc::CudaDeviceWorkerState::failed &&
                ordinal_snapshot.thread_joined &&
                !ordinal_snapshot.stream_created &&
                !ordinal_snapshot.event_created,
            "cudaSetDevice failure did not contain startup resources");

    pc::CudaJobQueue adapter_queue{};
    pc::CudaKernelRegistry adapter_metadata{};
    pc::CudaDeviceWorker adapter_worker{
        key, pool, adapter_queue, adapter_metadata};
    require(adapter_worker.register_adapter(std::make_unique<TestAdapter>(
                "test.initialization.failure.fp32",
                TestAdapterBehavior::initialization_failure)).ok(),
            "initialization-failure adapter registration failed");
    require(
        adapter_worker.start().code ==
            pc::CudaDeviceWorkerCode::adapter_initialization_failure,
        "adapter initialization failure was not worker-fatal");
    const auto adapter_snapshot = adapter_worker.snapshot();
    require(adapter_snapshot.state == pc::CudaDeviceWorkerState::failed &&
                adapter_snapshot.thread_joined &&
                adapter_snapshot.stream_destroyed &&
                adapter_snapshot.event_destroyed &&
                adapter_snapshot.adapter_resources_destroyed,
            "adapter initialization failure cleanup was incomplete");
}

void test_success_and_serial_jobs(const pc::CudaDevicePool& pool) {
    pc::CudaJobQueue queue{};
    pc::CudaKernelRegistry metadata{};
    require(metadata.register_kernel(
                pc::make_cuda_synthetic_execution_descriptor()).ok(),
            "synthetic execution metadata registration failed");

    const std::string key = pool.devices.front().persistent_key();
    pc::CudaDeviceWorker worker{key, pool, queue, metadata};
    require(worker.register_adapter(
                std::make_unique<pc::CudaSyntheticFp32Adapter>()).ok(),
            "synthetic adapter registration failed");
    require(worker.start().ok(), "synthetic worker start failed");

    constexpr int kJobCount = 32;
    std::vector<std::string> job_ids{};
    job_ids.reserve(kJobCount);
    for (int index = 0; index < kJobCount; ++index) {
        std::string id = "test-synthetic-" + std::to_string(index);
        pc::CudaDeviceWorkerResult submitted =
            worker.submit(make_job(id, key));
        require(submitted.ok() && submitted.has_job,
                "serial synthetic job submission failed");
        job_ids.push_back(std::move(id));
    }

    for (const std::string& id : job_ids) {
        const pc::CudaDeviceWorkerResult terminal =
            worker.wait_for_terminal(id, 10s);
        require(terminal.ok() && terminal.has_job &&
                    terminal.job.state == pc::CudaJobState::completed &&
                    terminal.job.claim_token != 0 &&
                    terminal.job.claimed_unix_ns != 0 &&
                    terminal.job.started_unix_ns != 0 &&
                    terminal.job.finished_unix_ns != 0,
                "synthetic job did not complete exactly through a claim");
    }

    const auto queue_snapshot = queue.snapshot();
    const auto worker_snapshot = worker.snapshot();
    require(queue_snapshot.completed == kJobCount &&
                queue_snapshot.failed == 0 &&
                worker_snapshot.claimed_job_count == kJobCount &&
                worker_snapshot.started_job_count == kJobCount &&
                worker_snapshot.completed_job_count == kJobCount &&
                worker_snapshot.failed_job_count == 0,
            "serial synthetic terminal counts were inconsistent");
    require(worker.stop().ok(), "serial synthetic worker stop failed");
}

void test_missing_adapter_and_issue_bound(const pc::CudaDevicePool& pool) {
    const std::string kernel_id = "test.actual.adapter.missing.fp32";
    pc::CudaKernelRegistry metadata{};
    require(metadata.register_kernel(executable_descriptor(kernel_id)).ok(),
            "missing-adapter metadata registration failed");
    pc::CudaJobQueue queue{};
    pc::CudaDeviceWorkerOptions options{};
    options.maximum_retained_issues = 2;
    const std::string key = pool.devices.front().persistent_key();
    pc::CudaDeviceWorker worker{key, pool, queue, metadata, options};
    require(worker.start().ok(), "missing-adapter worker start failed");

    for (int index = 0; index < 3; ++index) {
        const std::string id = "test-missing-adapter-" +
                               std::to_string(index);
        require(worker.submit(make_job(id, key, kernel_id)).ok(),
                "missing-adapter job submission failed");
        const auto terminal = worker.wait_for_terminal(id, 5s);
        require(terminal.ok() &&
                    terminal.job.state == pc::CudaJobState::failed,
                "missing actual adapter did not fail the job");
    }
    const auto snapshot = worker.snapshot();
    require(snapshot.completed_job_count == 0 &&
                snapshot.failed_job_count == 3 &&
                snapshot.retained_issues.size() == 2 &&
                snapshot.has_last_issue &&
                snapshot.last_issue.code ==
                    pc::CudaDeviceWorkerCode::adapter_not_found,
            "missing-adapter evidence or issue bound was incorrect");
    require(worker.stop().ok(), "missing-adapter worker stop failed");
}

void test_metadata_only_rejection(const pc::CudaDevicePool& pool) {
    pc::CudaKernelDescriptor descriptor =
        executable_descriptor("test.metadata.only.fp32");
    descriptor.binary_state = pc::CudaKernelBinaryState::unknown;
    descriptor.binding_state = pc::CudaKernelBindingState::metadata_only;
    pc::CudaKernelRegistry metadata{};
    require(metadata.register_kernel(descriptor).ok(),
            "metadata-only descriptor registration failed");
    pc::CudaJobQueue queue{};
    const std::string key = pool.devices.front().persistent_key();
    pc::CudaDeviceWorker worker{key, pool, queue, metadata};
    require(worker.start().ok(), "metadata-only worker start failed");
    const std::uint64_t generation =
        worker.snapshot().notification_generation;
    const auto submitted = worker.submit(
        make_job("test-metadata-only", key, descriptor.kernel_id));
    require(submitted.code == pc::CudaDeviceWorkerCode::queue_rejected &&
                submitted.queue_code == pc::CudaJobQueueCode::kernel_not_ready,
            "metadata-only job was not rejected by the existing queue");
    require(worker.snapshot().notification_generation == generation &&
                queue.snapshot().total_records == 0,
            "rejected metadata-only work incorrectly notified the worker");
    require(worker.stop().ok(), "metadata-only worker stop failed");
}

void test_controlled_adapter_failure(
    const pc::CudaDevicePool& pool,
    TestAdapterBehavior behavior,
    pc::CudaDeviceWorkerCode expected_issue,
    const std::string& suffix) {
    const std::string kernel_id = "test.controlled." + suffix + ".fp32";
    pc::CudaKernelRegistry metadata{};
    require(metadata.register_kernel(executable_descriptor(kernel_id)).ok(),
            "controlled adapter metadata registration failed");
    pc::CudaJobQueue queue{};
    pc::CudaDeviceWorkerOptions options{};
    options.maximum_issue_message_bytes = 64;
    const std::string key = pool.devices.front().persistent_key();
    pc::CudaDeviceWorker worker{key, pool, queue, metadata, options};
    require(worker.register_adapter(
                std::make_unique<TestAdapter>(kernel_id, behavior)).ok(),
            "controlled adapter registration failed");
    require(worker.start().ok(), "controlled adapter worker start failed");
    const std::string job_id = "test-controlled-" + suffix;
    require(worker.submit(make_job(job_id, key, kernel_id)).ok(),
            "controlled failure job submission failed");
    const auto terminal = worker.wait_for_terminal(job_id, 5s);
    require(terminal.ok() && terminal.job.state == pc::CudaJobState::failed,
            "controlled adapter failure falsely completed");
    const auto snapshot = worker.snapshot();
    require(snapshot.completed_job_count == 0 &&
                snapshot.failed_job_count == 1 && snapshot.has_last_issue &&
                snapshot.last_issue.code == expected_issue &&
                snapshot.last_issue.message.size() <= 64 &&
                snapshot.state == pc::CudaDeviceWorkerState::idle,
            "controlled adapter failure classification was incorrect");
    require(worker.stop().ok(), "controlled failure worker stop failed");
}

void test_stop_leaves_unclaimed_job_queued(const pc::CudaDevicePool& pool) {
    const std::string kernel_id = "test.stop.queued.fp32";
    pc::CudaKernelRegistry metadata{};
    require(metadata.register_kernel(executable_descriptor(kernel_id)).ok(),
            "stop-policy metadata registration failed");
    pc::CudaJobQueue queue{};
    const std::string key = pool.devices.front().persistent_key();
    pc::CudaDeviceWorker worker{key, pool, queue, metadata};
    require(worker.register_adapter(std::make_unique<TestAdapter>(
                kernel_id, TestAdapterBehavior::success)).ok(),
            "stop-policy adapter registration failed");
    require(worker.start().ok() && worker.wait_until_idle(5s).ok(),
            "stop-policy worker did not reach its wait state");

    require(queue.submit(
                make_job("test-left-queued", key, kernel_id),
                pool,
                metadata).ok(),
            "shared queue did not accept stop-policy job");
    require(worker.stop().ok(), "stop-policy worker stop failed");
    const auto record = queue.find("test-left-queued");
    require(record.ok() && record.job.state == pc::CudaJobState::queued,
            "stop silently claimed or cancelled queued shared work");
}

void test_restart(const pc::CudaDevicePool& pool) {
    pc::CudaKernelRegistry metadata{};
    require(metadata.register_kernel(
                pc::make_cuda_synthetic_execution_descriptor()).ok(),
            "restart metadata registration failed");
    pc::CudaJobQueue queue{};
    const std::string key = pool.devices.front().persistent_key();
    pc::CudaDeviceWorker worker{key, pool, queue, metadata};
    require(worker.register_adapter(
                std::make_unique<pc::CudaSyntheticFp32Adapter>()).ok(),
            "restart adapter registration failed");

    require(worker.start().ok(), "first restart-policy start failed");
    require(worker.submit(make_job("test-restart-first", key)).ok(),
            "first restart job submission failed");
    require(worker.wait_for_terminal("test-restart-first", 5s).ok(),
            "first restart job did not terminate");
    require(worker.stop().ok(), "first restart-policy stop failed");
    const auto first_stop = worker.snapshot();
    require(first_stop.stream_destroyed && first_stop.event_destroyed &&
                first_stop.adapter_resources_destroyed,
            "first restart cleanup was incomplete");

    require(worker.start().ok(), "stopped worker did not restart safely");
    const auto restarted = worker.snapshot();
    require(restarted.stream_created && restarted.event_created &&
                restarted.runtime_ordinal_snapshot ==
                    pool.devices.front().discovered.identity.runtime_ordinal &&
                !restarted.has_current_job,
            "restart did not re-resolve clean device-local resources");
    require(worker.submit(make_job("test-restart-second", key)).ok(),
            "second restart job submission failed");
    require(worker.wait_for_terminal("test-restart-second", 5s).ok(),
            "second restart job did not terminate");
    require(worker.stop().ok(), "second restart-policy stop failed");
    require(worker.snapshot().completed_job_count == 2,
            "restart cumulative counter policy was not preserved");
}

void test_wrong_device(const pc::CudaDevicePool& pool) {
    pc::CudaKernelRegistry metadata{};
    require(metadata.register_kernel(
                pc::make_cuda_synthetic_execution_descriptor()).ok(),
            "wrong-device metadata registration failed");
    pc::CudaJobQueue queue{};
    const std::string key = pool.devices.front().persistent_key();
    pc::CudaDeviceWorker worker{key, pool, queue, metadata};
    require(worker.start().ok(), "wrong-device worker start failed");
    const auto result = worker.submit(make_job(
        "test-wrong-device", key + "-different"));
    require(result.code == pc::CudaDeviceWorkerCode::wrong_device &&
                queue.snapshot().total_records == 0,
            "wrong-device job entered the execution path");
    require(worker.stop().ok(), "wrong-device worker stop failed");
}

void test_two_workers(const pc::CudaDevicePool& pool) {
    if (pool.devices.size() < 2) {
        std::cout << "GPU_WORKER_TWO_DEVICE status=skipped visible="
                  << pool.devices.size() << '\n';
        return;
    }

    pc::CudaKernelRegistry metadata{};
    require(metadata.register_kernel(
                pc::make_cuda_synthetic_execution_descriptor()).ok(),
            "two-device metadata registration failed");
    pc::CudaJobQueue queue{};
    const std::string key_a = pool.devices[0].persistent_key();
    const std::string key_b = pool.devices[1].persistent_key();
    require(key_a != key_b, "two devices did not retain distinct durable keys");
    pc::CudaDeviceWorker worker_a{key_a, pool, queue, metadata};
    pc::CudaDeviceWorker worker_b{key_b, pool, queue, metadata};
    require(worker_a.register_adapter(
                std::make_unique<pc::CudaSyntheticFp32Adapter>()).ok() &&
                worker_b.register_adapter(
                    std::make_unique<pc::CudaSyntheticFp32Adapter>()).ok(),
            "two-device adapter registration failed");
    require(worker_a.start().ok() && worker_b.start().ok(),
            "two-device workers did not start");
    require(worker_a.submit(make_job("test-device-a", key_a)).ok() &&
                worker_b.submit(make_job("test-device-b", key_b)).ok(),
            "two-device targeted submission failed");
    const auto terminal_a = worker_a.wait_for_terminal("test-device-a", 10s);
    const auto terminal_b = worker_b.wait_for_terminal("test-device-b", 10s);
    require(terminal_a.ok() && terminal_b.ok() &&
                terminal_a.job.descriptor.target_device_key == key_a &&
                terminal_b.job.descriptor.target_device_key == key_b &&
                terminal_a.job.state == pc::CudaJobState::completed &&
                terminal_b.job.state == pc::CudaJobState::completed &&
                worker_a.snapshot().completed_job_count == 1 &&
                worker_b.snapshot().completed_job_count == 1,
            "two-device execution crossed durable device identities");
    require(worker_a.stop().ok() && worker_b.stop().ok(),
            "two-device workers did not stop cleanly");
    std::cout << "GPU_WORKER_TWO_DEVICE status=passed device_a=" << key_a
              << " device_b=" << key_b << '\n';
}

}  // namespace

int main() {
    try {
        test_adapter_registry();
        test_start_rejections();

        pc::CudaDevicePoolBuildOptions options{};
        options.query_health = true;
        options.health_options.query_nvml = false;
        options.health_options.allow_cuda_memory_fallback = false;
        options.query_peer_access = false;
        const pc::CudaDevicePoolBuildResult discovery =
            pc::discover_cuda_device_pool(options);
        if (discovery.pool.devices.empty()) {
            std::cout << "GPU_WORKER_TEST status=skipped reason=no_devices\n";
            return EXIT_SUCCESS;
        }

        test_idle_start_stop(discovery.pool);
        test_worker_fatal_start_boundaries(discovery.pool);
        test_success_and_serial_jobs(discovery.pool);
        test_missing_adapter_and_issue_bound(discovery.pool);
        test_metadata_only_rejection(discovery.pool);
        test_controlled_adapter_failure(
            discovery.pool,
            TestAdapterBehavior::validation_mismatch,
            pc::CudaDeviceWorkerCode::output_validation_failure,
            "mismatch");
        test_controlled_adapter_failure(
            discovery.pool,
            TestAdapterBehavior::launch_failure,
            pc::CudaDeviceWorkerCode::launch_failure,
            "launch-failure");
        test_stop_leaves_unclaimed_job_queued(discovery.pool);
        test_restart(discovery.pool);
        test_wrong_device(discovery.pool);
        test_two_workers(discovery.pool);

        std::cout << "GPU_WORKER_TEST status=passed visible_devices="
                  << discovery.pool.devices.size() << '\n';
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "GPU_WORKER_TEST status=failed message="
                  << error.what() << '\n';
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "GPU_WORKER_TEST status=failed message=unknown\n";
        return EXIT_FAILURE;
    }
}
