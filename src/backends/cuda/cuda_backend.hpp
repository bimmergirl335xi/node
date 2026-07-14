#ifndef SRC_BACKENDS_CUDA_CUDA_BACKEND_HPP
#define SRC_BACKENDS_CUDA_CUDA_BACKEND_HPP

#include <mutex>
#include <string>
#include <vector>

#include "../compute_backend.hpp"
#include "cuda_device_pool.hpp"
#include "cuda_kernel_registry.hpp"

namespace prometheus::backends::cuda {

struct CudaBackendOptions {
    CudaDevicePoolBuildOptions pool_options{};

    // A node without CUDA is still allowed to run other backends. This option
    // controls whether no visible GPU is represented as unavailable or failed.
    bool no_devices_is_unavailable = true;
};

struct CudaDeviceExecutionReport {
    std::string persistent_key{};
    bool device_found = false;
    bool registration_ready = false;
    bool runtime_binding_usable = false;
    CudaKernelCoverageReport kernel_coverage{};

    [[nodiscard]] bool execution_ready() const noexcept {
        return device_found && registration_ready && runtime_binding_usable &&
               kernel_coverage.state == CudaKernelCoverageState::complete;
    }
};

// Phase 6 backend facade. It exposes discovery, health, capacity planning, and
// registry coverage. It does not own the robot loop, launch kernels, or select a
// permanent master GPU.
class CudaBackend final : public ComputeBackend {
public:
    explicit CudaBackend(CudaBackendOptions options = {});

    [[nodiscard]] std::string_view backend_id() const noexcept override;
    [[nodiscard]] std::string_view display_name() const noexcept override;
    [[nodiscard]] ComputeBackendKind kind() const noexcept override;

    ComputeBackendStatus initialize() override;
    ComputeBackendStatus refresh() override;
    void shutdown() noexcept override;

    [[nodiscard]] ComputeBackendStatus status() const override;
    [[nodiscard]] ComputeBackendSnapshot snapshot() const override;

    [[nodiscard]] CudaDevicePool pool_snapshot() const;

    [[nodiscard]] CudaTaskPlacementPlan plan_task_instances(
        const CudaTaskInstanceRequest& request,
        const std::vector<CudaCapacityCommitment>& commitments = {}) const;

    [[nodiscard]] CudaDeviceExecutionReport evaluate_device_execution(
        const std::string& persistent_key,
        const std::vector<std::string>& required_kernel_ids) const;

    [[nodiscard]] CudaKernelRegistry& kernel_registry() noexcept;
    [[nodiscard]] const CudaKernelRegistry& kernel_registry() const noexcept;

private:
    [[nodiscard]] ComputeBackendStatus rebuild_pool();

    CudaBackendOptions options_{};
    // Serializes initialize, refresh, and shutdown so an older discovery
    // operation cannot publish state after a newer lifecycle transition.
    mutable std::mutex lifecycle_mutex_{};
    mutable std::mutex mutex_{};
    ComputeBackendStatus status_{};
    CudaDevicePool pool_{};
    CudaKernelRegistry kernel_registry_{};
};

}  // namespace prometheus::backends::cuda

#endif  // SRC_BACKENDS_CUDA_CUDA_BACKEND_HPP
