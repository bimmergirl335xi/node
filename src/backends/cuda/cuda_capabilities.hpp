#ifndef SRC_BACKENDS_CUDA_CUDA_CAPABILITIES_HPP
#define SRC_BACKENDS_CUDA_CUDA_CAPABILITIES_HPP

#include <array>
#include <cstddef>
#include <string>

#include "cuda_profiles.hpp"

namespace prometheus::backends::cuda {

struct CudaSoftwareVersions {
    CudaVersion compile_time_toolkit{};
    CudaVersion runtime{};
    CudaVersion driver{};

    CudaReleaseFamily compile_time_release_family =
        CudaReleaseFamily::unknown;
    CudaReleaseFamily runtime_release_family = CudaReleaseFamily::unknown;

    bool runtime_version_available = false;
    bool driver_version_available = false;
    CudaSupportState driver_supports_runtime = CudaSupportState::unknown;
};

struct CudaCompatibilityReport {
    CudaSupportState prometheus_project = CudaSupportState::unknown;
    CudaSupportState compile_time_release_family = CudaSupportState::unknown;
    CudaSupportState runtime_release_family = CudaSupportState::unknown;

    // These remain unknown until build metadata and the Phase 6 kernel registry
    // can prove that a usable image and implementation are present.
    CudaSupportState binary_architecture_image = CudaSupportState::unknown;
    CudaSupportState registered_kernel_coverage = CudaSupportState::unknown;
};

// Immutable or effectively immutable properties reported by the CUDA runtime.
// Live memory pressure, temperature, power, utilization, and fault telemetry do
// not belong here; they will be represented by a refreshable health snapshot.
struct CudaDeviceCapabilities {
    int runtime_ordinal = -1;

    CudaArchitectureProfile architecture{};
    CudaCompatibilityReport compatibility{};
    CudaSoftwareVersions software{};

    std::size_t total_global_memory_bytes = 0;
    std::size_t total_constant_memory_bytes = 0;
    std::size_t shared_memory_per_block_bytes = 0;
    std::size_t shared_memory_per_multiprocessor_bytes = 0;
    std::size_t shared_memory_per_block_optin_bytes = 0;

    int multiprocessor_count = 0;
    int warp_size = 0;
    int registers_per_block = 0;
    int registers_per_multiprocessor = 0;

    int max_threads_per_block = 0;
    int max_threads_per_multiprocessor = 0;
    int max_blocks_per_multiprocessor = 0;
    std::array<int, 3> max_block_dimensions{};
    std::array<int, 3> max_grid_dimensions{};

    int core_clock_khz = 0;
    int memory_clock_khz = 0;
    int memory_bus_width_bits = 0;
    int l2_cache_bytes = 0;
    int async_engine_count = 0;

    CudaSupportState concurrent_kernels = CudaSupportState::unknown;
    CudaSupportState copy_compute_overlap = CudaSupportState::unknown;
    CudaSupportState unified_addressing = CudaSupportState::unknown;
    CudaSupportState managed_memory = CudaSupportState::unknown;
    CudaSupportState concurrent_managed_access = CudaSupportState::unknown;
    CudaSupportState pageable_memory_access = CudaSupportState::unknown;
    CudaSupportState direct_managed_memory_access_from_host =
        CudaSupportState::unknown;
    CudaSupportState host_native_atomic_operations =
        CudaSupportState::unknown;
    CudaSupportState cooperative_launch = CudaSupportState::unknown;
    CudaSupportState stream_priorities = CudaSupportState::unknown;
    CudaSupportState memory_pools = CudaSupportState::unknown;
    CudaSupportState compute_preemption = CudaSupportState::unknown;
    CudaSupportState can_map_host_memory = CudaSupportState::unknown;
    CudaSupportState integrated_gpu = CudaSupportState::unknown;
    CudaSupportState kernel_execution_timeout = CudaSupportState::unknown;
    CudaSupportState ecc_enabled = CudaSupportState::unknown;
};

enum class CudaCapabilityQueryCode {
    success = 0,
    invalid_device_ordinal,
    runtime_query_failed,
    device_property_query_failed,
};

struct CudaCapabilityQueryStatus {
    CudaCapabilityQueryCode code = CudaCapabilityQueryCode::success;
    int native_error_code = 0;
    std::string message{};

    [[nodiscard]] bool ok() const noexcept {
        return code == CudaCapabilityQueryCode::success;
    }
};

struct CudaCapabilityQueryResult {
    CudaCapabilityQueryStatus status{};
    CudaDeviceCapabilities capabilities{};
};

// Does not select a permanent device and does not alter Prometheus task
// placement.  It queries the requested temporary runtime ordinal only.
[[nodiscard]] CudaCapabilityQueryResult query_cuda_device_capabilities(
    int runtime_ordinal);

[[nodiscard]] CudaSoftwareVersions query_cuda_software_versions() noexcept;

}  // namespace prometheus::backends::cuda

#endif  // SRC_BACKENDS_CUDA_CUDA_CAPABILITIES_HPP
