#include "cuda_backend.hpp"

#include <utility>

namespace prometheus::backends::cuda {
namespace {

[[nodiscard]] ComputeBackendStatus map_pool_status(
    const CudaDevicePoolBuildResult& result,
    bool no_devices_is_unavailable) {
    ComputeBackendStatus status{};

    switch (result.status.code) {
        case CudaDevicePoolBuildCode::success:
            status.state = result.pool.summary.runtime_binding_usable_count > 0
                               ? ComputeBackendState::ready
                               : ComputeBackendState::degraded;
            status.usable =
                result.pool.summary.runtime_binding_usable_count > 0;
            status.message = result.status.message.empty()
                                 ? "CUDA backend inventory is ready"
                                 : result.status.message;
            return status;

        case CudaDevicePoolBuildCode::partial_success:
            status.state = ComputeBackendState::degraded;
            status.usable =
                result.pool.summary.runtime_binding_usable_count > 0;
            status.message = result.status.message;
            return status;

        case CudaDevicePoolBuildCode::no_devices:
            status.state = no_devices_is_unavailable
                               ? ComputeBackendState::unavailable
                               : ComputeBackendState::failed;
            status.usable = false;
            status.message = result.status.message;
            return status;

        case CudaDevicePoolBuildCode::discovery_failed:
            status.state = ComputeBackendState::failed;
            status.usable = false;
            status.message = result.status.message;
            return status;
    }

    status.state = ComputeBackendState::failed;
    status.message = "Unrecognized CUDA device-pool result";
    return status;
}

}  // namespace

CudaBackend::CudaBackend(CudaBackendOptions options)
    : options_(std::move(options)) {}

std::string_view CudaBackend::backend_id() const noexcept {
    return "cuda";
}

std::string_view CudaBackend::display_name() const noexcept {
    return "Prometheus CUDA Backend";
}

ComputeBackendKind CudaBackend::kind() const noexcept {
    return ComputeBackendKind::cuda;
}

ComputeBackendStatus CudaBackend::initialize() {
    return rebuild_pool();
}

ComputeBackendStatus CudaBackend::refresh() {
    return rebuild_pool();
}

void CudaBackend::shutdown() noexcept {
    std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
    std::lock_guard<std::mutex> lock(mutex_);
    pool_ = {};
    status_.state = ComputeBackendState::stopped;
    status_.usable = false;
    status_.message = "CUDA backend inventory was released";
}

ComputeBackendStatus CudaBackend::status() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

ComputeBackendSnapshot CudaBackend::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    ComputeBackendSnapshot result{};
    result.backend_id = std::string{backend_id()};
    result.display_name = std::string{display_name()};
    result.kind = kind();
    result.state = status_.state;
    result.usable = status_.usable;
    result.visible_resource_count = pool_.summary.visible_device_count;
    result.usable_resource_count =
        pool_.summary.runtime_binding_usable_count;
    result.message = status_.message;
    return result;
}

CudaDevicePool CudaBackend::pool_snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pool_;
}

CudaTaskPlacementPlan CudaBackend::plan_task_instances(
    const CudaTaskInstanceRequest& request,
    const std::vector<CudaCapacityCommitment>& commitments) const {
    CudaDevicePool pool_copy{};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_copy = pool_;
    }
    return plan_cuda_task_instances(pool_copy, request, commitments);
}

CudaDeviceExecutionReport CudaBackend::evaluate_device_execution(
    const std::string& persistent_key,
    const std::vector<std::string>& required_kernel_ids) const {
    CudaDeviceExecutionReport report{};
    report.persistent_key = persistent_key;

    CudaDeviceCapabilities capabilities{};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const CudaPoolDevice* device = pool_.find_device(persistent_key);
        if (device == nullptr) {
            return report;
        }

        report.device_found = true;
        report.registration_ready = device->registration_ready();
        report.runtime_binding_usable = device->runtime_binding_usable();
        capabilities = device->discovered.capabilities;
    }

    report.kernel_coverage = kernel_registry_.evaluate_device_coverage(
        capabilities,
        required_kernel_ids);
    return report;
}

CudaKernelRegistry& CudaBackend::kernel_registry() noexcept {
    return kernel_registry_;
}

const CudaKernelRegistry& CudaBackend::kernel_registry() const noexcept {
    return kernel_registry_;
}

ComputeBackendStatus CudaBackend::rebuild_pool() {
    std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        status_.state = ComputeBackendState::initializing;
        status_.usable = false;
        status_.message = "Discovering CUDA resources";
    }

    CudaDevicePoolBuildResult result =
        discover_cuda_device_pool(options_.pool_options);
    ComputeBackendStatus mapped = map_pool_status(
        result,
        options_.no_devices_is_unavailable);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_ = std::move(result.pool);
        status_ = mapped;
    }
    return mapped;
}

}  // namespace prometheus::backends::cuda
