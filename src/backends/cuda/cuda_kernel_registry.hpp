#ifndef SRC_BACKENDS_CUDA_CUDA_KERNEL_REGISTRY_HPP
#define SRC_BACKENDS_CUDA_CUDA_KERNEL_REGISTRY_HPP

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "cuda_capabilities.hpp"

namespace prometheus::backends::cuda {

enum class CudaKernelFamily : std::uint8_t {
    unknown = 0,
    activation,
    reduction,
    dense,
    normalization,
    temporal,
    learning,
    optimizer,
    candidate_validation,
};

enum class CudaScalarType : std::uint8_t {
    unknown = 0,
    fp16,
    fp32,
    fp64,
    int8,
    int16,
    int32,
    int64,
    uint8,
    uint16,
    uint32,
    uint64,
    boolean,
};

// The registry can describe a kernel before a typed launch adapter exists.
// Metadata-only entries are not executable and are never reported as complete.
enum class CudaKernelBindingState : std::uint8_t {
    metadata_only = 0,
    adapter_available,
    disabled,
};

// Build metadata must explicitly state image availability. Compute capability
// alone cannot prove that this executable contains a compatible cubin or PTX.
enum class CudaKernelBinaryState : std::uint8_t {
    unknown = 0,
    unavailable,
    available,
};

struct CudaKernelDescriptor {
    std::string kernel_id{};
    std::string display_name{};
    CudaKernelFamily family = CudaKernelFamily::unknown;
    std::vector<CudaScalarType> scalar_types{};

    CudaComputeCapability minimum_compute_capability{};
    // Invalid means no declared upper bound.
    CudaComputeCapability maximum_compute_capability{};

    CudaSupportState cuda_12_release_support = CudaSupportState::unknown;
    CudaSupportState cuda_13_release_support = CudaSupportState::unknown;
    CudaKernelBinaryState binary_state = CudaKernelBinaryState::unknown;
    CudaKernelBindingState binding_state =
        CudaKernelBindingState::metadata_only;

    // Provenance only; neither field is treated as proof of implementation.
    std::string source_file{};
    std::string adapter_name{};
};

enum class CudaKernelRegistrationCode : std::uint8_t {
    success = 0,
    invalid_descriptor,
    duplicate_kernel_id,
    not_found,
};

struct CudaKernelRegistrationResult {
    CudaKernelRegistrationCode code = CudaKernelRegistrationCode::success;
    std::string message{};

    [[nodiscard]] bool ok() const noexcept {
        return code == CudaKernelRegistrationCode::success;
    }
};

enum class CudaKernelCompatibilityState : std::uint8_t {
    unknown = 0,
    unsupported,
    supported,
};

struct CudaKernelCoverageItem {
    std::string kernel_id{};
    bool descriptor_found = false;
    CudaKernelCompatibilityState compatibility =
        CudaKernelCompatibilityState::unknown;
    CudaKernelBinaryState binary_state = CudaKernelBinaryState::unknown;
    CudaKernelBindingState binding_state =
        CudaKernelBindingState::metadata_only;
    std::string message{};
};

enum class CudaKernelCoverageState : std::uint8_t {
    unknown = 0,
    unavailable,
    partial,
    complete,
};

struct CudaKernelCoverageReport {
    CudaKernelCoverageState state = CudaKernelCoverageState::unknown;
    std::size_t required_count = 0;
    std::size_t supported_count = 0;
    std::size_t unresolved_count = 0;
    std::size_t unavailable_count = 0;
    std::vector<CudaKernelCoverageItem> items{};
};

class CudaKernelRegistry {
public:
    [[nodiscard]] CudaKernelRegistrationResult register_kernel(
        CudaKernelDescriptor descriptor);

    [[nodiscard]] CudaKernelRegistrationResult remove_kernel(
        const std::string& kernel_id);

    [[nodiscard]] bool contains(const std::string& kernel_id) const;
    [[nodiscard]] std::vector<CudaKernelDescriptor> descriptors() const;
    [[nodiscard]] std::size_t size() const;

    // Evaluates metadata only. No kernel is launched and no CUDA device is
    // selected. An empty requirement list intentionally returns unknown.
    [[nodiscard]] CudaKernelCoverageReport evaluate_device_coverage(
        const CudaDeviceCapabilities& capabilities,
        const std::vector<std::string>& required_kernel_ids) const;

private:
    mutable std::mutex mutex_{};
    std::vector<CudaKernelDescriptor> descriptors_{};
};

[[nodiscard]] const char* to_string(CudaKernelFamily value) noexcept;
[[nodiscard]] const char* to_string(CudaScalarType value) noexcept;
[[nodiscard]] const char* to_string(CudaKernelBindingState value) noexcept;
[[nodiscard]] const char* to_string(CudaKernelBinaryState value) noexcept;
[[nodiscard]] const char* to_string(CudaKernelRegistrationCode value) noexcept;
[[nodiscard]] const char* to_string(CudaKernelCompatibilityState value) noexcept;
[[nodiscard]] const char* to_string(CudaKernelCoverageState value) noexcept;

}  // namespace prometheus::backends::cuda

#endif  // SRC_BACKENDS_CUDA_CUDA_KERNEL_REGISTRY_HPP
