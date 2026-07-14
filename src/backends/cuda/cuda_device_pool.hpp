#ifndef SRC_BACKENDS_CUDA_CUDA_DEVICE_POOL_HPP
#define SRC_BACKENDS_CUDA_CUDA_DEVICE_POOL_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "cuda_health.hpp"

namespace prometheus::backends::cuda {

// Peer capability is descriptive only. Discovery never enables peer access,
// creates a context, or implies that a cognitive task must span devices.
enum class CudaPeerAccessState : std::uint8_t {
    unknown = 0,
    same_device,
    unsupported,
    supported,
    query_failed,
};

struct CudaPeerAccessRecord {
    std::string source_persistent_key{};
    std::string target_persistent_key{};
    int source_runtime_ordinal = -1;
    int target_runtime_ordinal = -1;
    CudaPeerAccessState state = CudaPeerAccessState::unknown;
    int native_error_code = 0;
    std::string message{};
};

struct CudaPoolDevice {
    CudaDiscoveredDevice discovered{};

    bool health_was_requested = false;
    bool health_available = false;
    CudaHealthQueryResult health{};

    [[nodiscard]] const std::string& persistent_key() const noexcept {
        return discovered.identity.persistent_key;
    }

    [[nodiscard]] bool registration_ready() const noexcept {
        return discovered.ready_for_backend_registration();
    }

    [[nodiscard]] bool runtime_binding_usable() const noexcept {
        return health_available && health.status.runtime_binding_usable() &&
               health.snapshot.runtime_binding ==
                   CudaRuntimeBindingState::matched;
    }
};

struct CudaDevicePoolSummary {
    std::size_t visible_device_count = 0;
    std::size_t stable_identity_count = 0;
    std::size_t registration_ready_count = 0;
    std::size_t health_completed_count = 0;
    std::size_t runtime_binding_usable_count = 0;
    std::size_t peer_link_count = 0;
    std::size_t peer_supported_link_count = 0;

    std::uint64_t total_global_memory_bytes = 0;
    std::uint64_t observed_free_memory_bytes = 0;
    std::size_t observed_free_memory_device_count = 0;

    bool heterogeneous_architectures = false;
};

struct CudaDevicePool {
    CudaDeviceDiscoveryResult discovery{};
    std::vector<CudaPoolDevice> devices{};
    std::vector<CudaPeerAccessRecord> peer_access{};
    CudaDevicePoolSummary summary{};

    [[nodiscard]] const CudaPoolDevice* find_device(
        const std::string& persistent_key) const noexcept;

    [[nodiscard]] CudaPoolDevice* find_device(
        const std::string& persistent_key) noexcept;

    [[nodiscard]] const CudaPeerAccessRecord* find_peer_access(
        const std::string& source_persistent_key,
        const std::string& target_persistent_key) const noexcept;
};

struct CudaDevicePoolBuildOptions {
    bool query_health = true;
    CudaHealthQueryOptions health_options{};
    bool query_peer_access = true;
};

enum class CudaDevicePoolBuildCode : std::uint8_t {
    success = 0,
    no_devices,
    partial_success,
    discovery_failed,
};

struct CudaDevicePoolBuildStatus {
    CudaDevicePoolBuildCode code = CudaDevicePoolBuildCode::success;
    std::string message{};

    [[nodiscard]] bool completed() const noexcept {
        return code == CudaDevicePoolBuildCode::success ||
               code == CudaDevicePoolBuildCode::no_devices ||
               code == CudaDevicePoolBuildCode::partial_success;
    }

    [[nodiscard]] bool complete() const noexcept {
        return code == CudaDevicePoolBuildCode::success ||
               code == CudaDevicePoolBuildCode::no_devices;
    }
};

struct CudaDevicePoolBuildResult {
    CudaDevicePoolBuildStatus status{};
    CudaDevicePool pool{};
};

// Builds a point-in-time, headless in-memory inventory. No device is selected
// as a master, no peer link is enabled, and no task is dispatched.
[[nodiscard]] CudaDevicePoolBuildResult discover_cuda_device_pool(
    const CudaDevicePoolBuildOptions& options = {});

// Describes how available memory was established for an advisory capacity
// calculation. "Static total" is intentionally distinguishable from observed
// free memory so callers can require live evidence before scheduling.
enum class CudaCapacityMemorySource : std::uint8_t {
    unavailable = 0,
    observed_free_memory,
    static_total_memory,
};

struct CudaCapacityCommitment {
    std::string persistent_key{};
    std::uint64_t reserved_memory_bytes = 0;
    std::size_t instance_count = 0;
};

enum class CudaTaskPlacementPolicy : std::uint8_t {
    stable_order = 0,
    pack,
    spread,
};

// One request represents N independent specialized task instances. Each
// instance is intentionally placed whole on one GPU. This is not model/tensor
// sharding and does not authorize execution.
struct CudaTaskInstanceRequest {
    std::string request_id{};
    std::size_t instance_count = 1;
    std::uint64_t memory_bytes_per_instance = 0;

    // Amount that must remain uncommitted on every selected device after a
    // placement. This provides room for CUDA runtime overhead and transient
    // allocations without inventing a universal percentage policy.
    std::uint64_t memory_headroom_bytes = 0;

    CudaComputeCapability minimum_compute_capability{};
    int minimum_multiprocessor_count = 0;

    bool require_registration_ready = true;
    bool require_runtime_binding = true;
    bool require_observed_free_memory = true;

    bool allow_multiple_instances_per_device = true;
    // Zero means no explicit count cap beyond memory and other requirements.
    std::size_t maximum_instances_per_device = 0;

    // Empty allowed list means every pool device may be considered.
    std::vector<std::string> allowed_device_keys{};
    std::vector<std::string> excluded_device_keys{};

    CudaTaskPlacementPolicy policy = CudaTaskPlacementPolicy::spread;
};

struct CudaDeviceCapacityView {
    std::string persistent_key{};
    int runtime_ordinal_snapshot = -1;

    CudaArchitectureFamily architecture = CudaArchitectureFamily::unknown;
    CudaComputeCapability compute_capability{};
    int multiprocessor_count = 0;

    bool registration_ready = false;
    bool runtime_binding_usable = false;

    CudaCapacityMemorySource memory_source =
        CudaCapacityMemorySource::unavailable;
    std::uint64_t total_memory_bytes = 0;
    std::uint64_t available_memory_bytes = 0;

    std::uint64_t preexisting_reserved_memory_bytes = 0;
    std::size_t preexisting_instance_count = 0;
    std::uint64_t planned_reserved_memory_bytes = 0;
    std::size_t planned_instance_count = 0;
};

enum class CudaPlacementIssueCode : std::uint8_t {
    invalid_request = 0,
    unknown_commitment_device,
    commitment_exceeds_capacity,
    device_not_allowed,
    device_excluded,
    registration_not_ready,
    runtime_binding_unusable,
    compute_capability_insufficient,
    multiprocessor_count_insufficient,
    memory_observation_unavailable,
    memory_capacity_insufficient,
    per_device_instance_limit_reached,
    no_eligible_device,
};

struct CudaPlacementIssue {
    CudaPlacementIssueCode code = CudaPlacementIssueCode::invalid_request;
    std::string persistent_key{};
    std::size_t instance_index = 0;
    std::string message{};
};

struct CudaTaskInstanceAssignment {
    std::size_t instance_index = 0;
    std::string persistent_key{};
    int runtime_ordinal_snapshot = -1;
    std::uint64_t reserved_memory_bytes = 0;
};

enum class CudaTaskPlacementCode : std::uint8_t {
    success = 0,
    partial_success,
    unsatisfied,
    invalid_request,
};

struct CudaTaskPlacementPlan {
    CudaTaskPlacementCode code = CudaTaskPlacementCode::success;
    std::string request_id{};
    std::vector<CudaTaskInstanceAssignment> assignments{};
    std::vector<std::size_t> unplaced_instance_indices{};
    std::vector<CudaDeviceCapacityView> devices{};
    std::vector<CudaPlacementIssue> issues{};

    [[nodiscard]] bool fully_satisfied() const noexcept {
        return code == CudaTaskPlacementCode::success;
    }
};

// Produces an advisory plan only. It does not allocate memory, create streams,
// set devices, or mutate the pool. Existing Prometheus reservations may be
// supplied as commitments so several unrelated task requests can be evaluated
// without pretending observed free memory is the entire scheduling budget.
[[nodiscard]] CudaTaskPlacementPlan plan_cuda_task_instances(
    const CudaDevicePool& pool,
    const CudaTaskInstanceRequest& request,
    const std::vector<CudaCapacityCommitment>& existing_commitments = {});

[[nodiscard]] const char* to_string(CudaPeerAccessState value) noexcept;
[[nodiscard]] const char* to_string(CudaDevicePoolBuildCode value) noexcept;
[[nodiscard]] const char* to_string(CudaCapacityMemorySource value) noexcept;
[[nodiscard]] const char* to_string(CudaTaskPlacementPolicy value) noexcept;
[[nodiscard]] const char* to_string(CudaPlacementIssueCode value) noexcept;
[[nodiscard]] const char* to_string(CudaTaskPlacementCode value) noexcept;

}  // namespace prometheus::backends::cuda

#endif  // SRC_BACKENDS_CUDA_CUDA_DEVICE_POOL_HPP
