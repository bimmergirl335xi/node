#ifndef SRC_BACKENDS_CUDA_CUDA_DEVICE_HPP
#define SRC_BACKENDS_CUDA_CUDA_DEVICE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "cuda_capabilities.hpp"

namespace prometheus::backends::cuda {

inline constexpr std::size_t kCudaUuidByteCount = 16;

// Parsed PCI location for diagnostics and topology reporting.  PCI location is
// useful for describing where a card is installed, but it is not the durable
// Prometheus identity because a card may be moved to another slot.
struct CudaPciAddress {
    std::uint32_t domain = 0;
    std::uint32_t bus = 0;
    std::uint32_t device = 0;
    std::uint32_t function = 0;
    bool domain_was_present = false;
    bool valid = false;
};

// Identity reported for one device visible to the current CUDA process.
// runtime_ordinal is intentionally temporary: CUDA_VISIBLE_DEVICES and system
// enumeration order can change it between process launches.
struct CudaDeviceIdentity {
    int runtime_ordinal = -1;
    std::string name{};

    std::array<std::uint8_t, kCudaUuidByteCount> uuid_bytes{};
    bool uuid_available = false;
    std::string uuid{};            // Canonical form: GPU-xxxxxxxx-....
    std::string persistent_key{};  // Canonical form: cuda:GPU-xxxxxxxx-....

    bool pci_bus_id_available = false;
    std::string pci_bus_id{};
    CudaPciAddress pci_address{};

    [[nodiscard]] bool has_stable_identity() const noexcept {
        return uuid_available && !uuid.empty() && !persistent_key.empty();
    }
};

enum class CudaDeviceIssueCode : std::uint8_t {
    uuid_unavailable = 0,
    pci_bus_id_query_failed,
    pci_bus_id_parse_failed,
    capability_query_failed,
    duplicate_stable_identity,
};

struct CudaDeviceIssue {
    CudaDeviceIssueCode code = CudaDeviceIssueCode::uuid_unavailable;
    int native_error_code = 0;
    bool fatal = false;
    std::string message{};
};

enum class CudaIdentityQueryCode : std::uint8_t {
    success = 0,
    partial_success,
    invalid_device_ordinal,
    runtime_query_failed,
    device_property_query_failed,
};

struct CudaIdentityQueryStatus {
    CudaIdentityQueryCode code = CudaIdentityQueryCode::success;
    int native_error_code = 0;
    std::string message{};

    [[nodiscard]] bool completed() const noexcept {
        return code == CudaIdentityQueryCode::success ||
               code == CudaIdentityQueryCode::partial_success;
    }

    [[nodiscard]] bool complete() const noexcept {
        return code == CudaIdentityQueryCode::success;
    }
};

struct CudaIdentityQueryResult {
    CudaIdentityQueryStatus status{};
    CudaDeviceIdentity identity{};
    std::vector<CudaDeviceIssue> issues{};
};

// A discovery record is retained even when one field cannot be queried.  This
// permits diagnostics to report partial failures instead of silently dropping
// a visible device from the node inventory.
struct CudaDiscoveredDevice {
    CudaDeviceIdentity identity{};
    CudaIdentityQueryStatus identity_status{};

    CudaDeviceCapabilities capabilities{};
    bool capabilities_available = false;

    std::vector<CudaDeviceIssue> issues{};

    [[nodiscard]] bool has_stable_identity() const noexcept {
        return identity.has_stable_identity();
    }

    [[nodiscard]] bool has_fatal_issue() const noexcept;

    // Registration readiness is deliberately narrower than execution
    // readiness.  Kernel-image and registry coverage are decided later.
    [[nodiscard]] bool ready_for_backend_registration() const noexcept {
        return has_stable_identity() && capabilities_available &&
               !has_fatal_issue();
    }

    [[nodiscard]] bool complete() const noexcept {
        return identity_status.complete() && capabilities_available &&
               issues.empty();
    }
};

enum class CudaDiscoveryCode : std::uint8_t {
    success = 0,
    no_devices,
    partial_success,
    runtime_query_failed,
};

struct CudaDiscoveryStatus {
    CudaDiscoveryCode code = CudaDiscoveryCode::success;
    int native_error_code = 0;
    std::string message{};

    [[nodiscard]] bool completed() const noexcept {
        return code == CudaDiscoveryCode::success ||
               code == CudaDiscoveryCode::no_devices ||
               code == CudaDiscoveryCode::partial_success;
    }

    [[nodiscard]] bool complete() const noexcept {
        return code == CudaDiscoveryCode::success ||
               code == CudaDiscoveryCode::no_devices;
    }
};

struct CudaDeviceDiscoveryResult {
    CudaDiscoveryStatus status{};
    int reported_device_count = 0;
    std::size_t stable_identity_count = 0;
    std::size_t capability_count = 0;
    std::size_t registration_ready_count = 0;
    std::vector<CudaDiscoveredDevice> devices{};
};

[[nodiscard]] std::string format_cuda_uuid(
    const std::array<std::uint8_t, kCudaUuidByteCount>& bytes);

[[nodiscard]] std::string make_cuda_persistent_key(
    const std::string& canonical_uuid);

[[nodiscard]] CudaPciAddress parse_cuda_pci_bus_id(
    const std::string& pci_bus_id) noexcept;

// Queries identity for one temporary runtime ordinal.  It does not call
// cudaSetDevice, allocate memory, create streams, or assign cognitive work.
[[nodiscard]] CudaIdentityQueryResult query_cuda_device_identity(
    int runtime_ordinal);

// Enumerates every device visible to this process, preserving a record for
// each runtime ordinal even when one device reports only partial information.
[[nodiscard]] CudaDeviceDiscoveryResult discover_cuda_devices();

[[nodiscard]] const char* to_string(CudaDeviceIssueCode value) noexcept;
[[nodiscard]] const char* to_string(CudaIdentityQueryCode value) noexcept;
[[nodiscard]] const char* to_string(CudaDiscoveryCode value) noexcept;

}  // namespace prometheus::backends::cuda

#endif  // SRC_BACKENDS_CUDA_CUDA_DEVICE_HPP
