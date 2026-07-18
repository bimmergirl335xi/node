#ifndef SRC_BACKENDS_CUDA_CUDA_KERNEL_ADAPTER_HPP
#define SRC_BACKENDS_CUDA_CUDA_KERNEL_ADAPTER_HPP

#include <cuda_runtime_api.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "cuda_job_queue.hpp"

namespace prometheus::backends::cuda {

inline constexpr std::size_t kCudaAdapterIdentifierAbsoluteMaximum = 256;
inline constexpr std::size_t kCudaAdapterCountAbsoluteMaximum = 64;
inline constexpr std::size_t kCudaAdapterMessageAbsoluteMaximum = 1024;
inline constexpr std::size_t kCudaSyntheticProbeElementCount = 8;
inline constexpr std::size_t kCudaSyntheticProbeElementAbsoluteMaximum = 64;
inline constexpr std::string_view kCudaSyntheticExecutionKernelId =
    "probe.synthetic.execute.fp32";

enum class CudaKernelAdapterCode : std::uint8_t {
    success = 0,
    invalid_adapter,
    invalid_kernel_id,
    duplicate_kernel_id,
    capacity_reached,
    registry_active,
    not_found,
    invalid_context,
    not_initialized,
    already_initialized,
    initialization_failed,
    launch_failed,
    completion_validation_failed,
    shutdown_failed,
    resource_exhausted,
};

struct CudaKernelAdapterResult {
    CudaKernelAdapterCode code = CudaKernelAdapterCode::success;
    int native_error_code = 0;
    bool worker_fatal = false;
    std::string message{};

    [[nodiscard]] bool ok() const noexcept {
        return code == CudaKernelAdapterCode::success;
    }
};

struct CudaKernelAdapterDeviceContext {
    std::string persistent_device_key{};
    int runtime_ordinal_snapshot = -1;
    cudaStream_t stream = nullptr;
};

// One adapter instance belongs to one device worker. Implementations may own
// device-local resources, but must use only the supplied worker stream for job
// operations. Every method is an exception-containment boundary.
class CudaKernelLaunchAdapter {
public:
    virtual ~CudaKernelLaunchAdapter() = default;

    [[nodiscard]] virtual std::string_view kernel_id() const noexcept = 0;
    [[nodiscard]] virtual std::string_view adapter_name() const noexcept = 0;
    [[nodiscard]] virtual bool initialized() const noexcept = 0;

    [[nodiscard]] virtual CudaKernelAdapterResult initialize(
        const CudaKernelAdapterDeviceContext& context) noexcept = 0;
    [[nodiscard]] virtual CudaKernelAdapterResult launch(
        const CudaJobDescriptor& job,
        cudaStream_t stream) noexcept = 0;
    [[nodiscard]] virtual CudaKernelAdapterResult validate_completion()
        noexcept = 0;
    [[nodiscard]] virtual CudaKernelAdapterResult shutdown() noexcept = 0;
};

struct CudaKernelAdapterRegistryOptions {
    std::size_t maximum_adapters = 16;
    std::size_t maximum_identifier_bytes = 128;
    std::size_t maximum_message_bytes = 256;
};

struct CudaKernelAdapterInventoryItem {
    std::string kernel_id{};
    std::string adapter_name{};
    bool initialized = false;
};

// This registry owns actual typed adapters. It is intentionally distinct from
// CudaKernelRegistry, which owns metadata only. Registration is closed while
// device-local resources are initialized.
class CudaKernelAdapterRegistry {
public:
    explicit CudaKernelAdapterRegistry(
        CudaKernelAdapterRegistryOptions options = {});

    CudaKernelAdapterRegistry(const CudaKernelAdapterRegistry&) = delete;
    CudaKernelAdapterRegistry& operator=(const CudaKernelAdapterRegistry&) =
        delete;
    CudaKernelAdapterRegistry(CudaKernelAdapterRegistry&&) = delete;
    CudaKernelAdapterRegistry& operator=(CudaKernelAdapterRegistry&&) = delete;

    [[nodiscard]] CudaKernelAdapterResult register_adapter(
        std::unique_ptr<CudaKernelLaunchAdapter> adapter) noexcept;

    [[nodiscard]] CudaKernelLaunchAdapter* find(
        std::string_view kernel_id) noexcept;
    [[nodiscard]] const CudaKernelLaunchAdapter* find(
        std::string_view kernel_id) const noexcept;

    [[nodiscard]] std::vector<CudaKernelAdapterInventoryItem> inventory()
        const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool initialized() const noexcept;

    [[nodiscard]] CudaKernelAdapterResult initialize_all(
        const CudaKernelAdapterDeviceContext& context) noexcept;
    [[nodiscard]] CudaKernelAdapterResult shutdown_all() noexcept;

private:
    [[nodiscard]] CudaKernelAdapterResult bounded_result(
        CudaKernelAdapterCode code,
        std::string message,
        int native_error_code = 0,
        bool worker_fatal = false) const noexcept;

    CudaKernelAdapterRegistryOptions options_{};
    bool options_valid_ = false;
    bool initialized_ = false;
    std::vector<std::unique_ptr<CudaKernelLaunchAdapter>> adapters_{};
};

// Fixed eight-element ReLU adapter used only to prove the governed execution
// path. It is not a general job-memory or payload mechanism.
class CudaSyntheticFp32Adapter final : public CudaKernelLaunchAdapter {
public:
    CudaSyntheticFp32Adapter() = default;
    ~CudaSyntheticFp32Adapter() override = default;

    [[nodiscard]] std::string_view kernel_id() const noexcept override;
    [[nodiscard]] std::string_view adapter_name() const noexcept override;
    [[nodiscard]] bool initialized() const noexcept override;

    [[nodiscard]] CudaKernelAdapterResult initialize(
        const CudaKernelAdapterDeviceContext& context) noexcept override;
    [[nodiscard]] CudaKernelAdapterResult launch(
        const CudaJobDescriptor& job,
        cudaStream_t stream) noexcept override;
    [[nodiscard]] CudaKernelAdapterResult validate_completion()
        noexcept override;
    [[nodiscard]] CudaKernelAdapterResult shutdown() noexcept override;

private:
    [[nodiscard]] CudaKernelAdapterResult cuda_failure(
        CudaKernelAdapterCode code,
        cudaError_t error,
        const char* operation,
        bool worker_fatal) const noexcept;

    int runtime_ordinal_snapshot_ = -1;
    cudaStream_t stream_ = nullptr;
    float* device_input_ = nullptr;
    float* device_output_ = nullptr;
    float host_input_[kCudaSyntheticProbeElementCount]{};
    float host_output_[kCudaSyntheticProbeElementCount]{};
    bool initialized_ = false;
    bool launch_pending_validation_ = false;
};

[[nodiscard]] CudaKernelDescriptor make_cuda_synthetic_execution_descriptor()
    noexcept;

[[nodiscard]] const char* to_string(CudaKernelAdapterCode value) noexcept;

}  // namespace prometheus::backends::cuda

#endif  // SRC_BACKENDS_CUDA_CUDA_KERNEL_ADAPTER_HPP
