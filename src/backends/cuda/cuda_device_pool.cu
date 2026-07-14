#include "cuda_device_pool.hpp"

#include <cuda_runtime_api.h>

#include <algorithm>
#include <limits>
#include <string>
#include <utility>

namespace prometheus::backends::cuda {
namespace {

[[nodiscard]] std::string make_cuda_error_message(
    cudaError_t error,
    const char* operation) {
    std::string message = operation;
    message += " failed";

    const char* error_name = cudaGetErrorName(error);
    const char* error_text = cudaGetErrorString(error);
    if (error_name != nullptr) {
        message += " (";
        message += error_name;
        message += ')';
    }
    if (error_text != nullptr) {
        message += ": ";
        message += error_text;
    }
    return message;
}

[[nodiscard]] bool contains_key(
    const std::vector<std::string>& keys,
    const std::string& key) {
    return std::find(keys.begin(), keys.end(), key) != keys.end();
}

[[nodiscard]] bool compute_capability_at_least(
    CudaComputeCapability actual,
    CudaComputeCapability required) noexcept {
    if (!required.valid()) {
        return true;
    }
    return actual.valid() && actual.encoded() >= required.encoded();
}

[[nodiscard]] bool can_fit_with_headroom(
    std::uint64_t available_bytes,
    std::uint64_t requested_bytes,
    std::uint64_t headroom_bytes) noexcept {
    if (available_bytes < headroom_bytes) {
        return false;
    }
    return (available_bytes - headroom_bytes) >= requested_bytes;
}

void append_placement_issue(
    CudaTaskPlacementPlan& plan,
    CudaPlacementIssueCode code,
    const std::string& persistent_key,
    std::size_t instance_index,
    std::string message) {
    CudaPlacementIssue issue{};
    issue.code = code;
    issue.persistent_key = persistent_key;
    issue.instance_index = instance_index;
    issue.message = std::move(message);
    plan.issues.push_back(std::move(issue));
}

[[nodiscard]] CudaDeviceCapacityView make_capacity_view(
    const CudaPoolDevice& device,
    bool require_observed_free_memory) {
    CudaDeviceCapacityView view{};
    view.persistent_key = device.discovered.identity.persistent_key;
    view.runtime_ordinal_snapshot =
        device.discovered.identity.runtime_ordinal;
    view.registration_ready = device.registration_ready();
    view.runtime_binding_usable = device.runtime_binding_usable();

    if (device.discovered.capabilities_available) {
        const CudaDeviceCapabilities& capabilities =
            device.discovered.capabilities;
        view.architecture = capabilities.architecture.architecture;
        view.compute_capability =
            capabilities.architecture.compute_capability;
        view.multiprocessor_count = capabilities.multiprocessor_count;
        view.total_memory_bytes = static_cast<std::uint64_t>(
            capabilities.total_global_memory_bytes);
    }

    if (device.health_available &&
        device.health.snapshot.memory_total_bytes.available()) {
        view.total_memory_bytes =
            device.health.snapshot.memory_total_bytes.value;
    }

    if (device.health_available &&
        device.health.snapshot.memory_free_bytes.available()) {
        view.memory_source =
            CudaCapacityMemorySource::observed_free_memory;
        view.available_memory_bytes =
            device.health.snapshot.memory_free_bytes.value;
    } else if (!require_observed_free_memory &&
               view.total_memory_bytes > 0) {
        view.memory_source = CudaCapacityMemorySource::static_total_memory;
        view.available_memory_bytes = view.total_memory_bytes;
    }

    return view;
}

[[nodiscard]] std::size_t total_instance_count(
    const CudaDeviceCapacityView& view) noexcept {
    return view.preexisting_instance_count + view.planned_instance_count;
}

[[nodiscard]] bool capacity_view_key_less(
    const CudaDeviceCapacityView& left,
    const CudaDeviceCapacityView& right) {
    return left.persistent_key < right.persistent_key;
}

[[nodiscard]] std::size_t choose_candidate(
    const std::vector<CudaDeviceCapacityView>& devices,
    const std::vector<std::size_t>& candidates,
    const CudaTaskInstanceRequest& request) {
    std::size_t selected = candidates.front();

    for (std::size_t candidate : candidates) {
        const CudaDeviceCapacityView& current = devices[candidate];
        const CudaDeviceCapacityView& best = devices[selected];

        bool prefer_current = false;
        if (request.policy == CudaTaskPlacementPolicy::stable_order) {
            prefer_current = capacity_view_key_less(current, best);
        } else if (request.policy == CudaTaskPlacementPolicy::pack) {
            const std::uint64_t current_remaining =
                current.available_memory_bytes -
                request.memory_bytes_per_instance;
            const std::uint64_t best_remaining =
                best.available_memory_bytes -
                request.memory_bytes_per_instance;
            if (current_remaining != best_remaining) {
                prefer_current = current_remaining < best_remaining;
            } else {
                prefer_current = capacity_view_key_less(current, best);
            }
        } else {
            const std::size_t current_instances =
                total_instance_count(current);
            const std::size_t best_instances = total_instance_count(best);
            if (current_instances != best_instances) {
                prefer_current = current_instances < best_instances;
            } else if (current.available_memory_bytes !=
                       best.available_memory_bytes) {
                prefer_current = current.available_memory_bytes >
                                 best.available_memory_bytes;
            } else {
                prefer_current = capacity_view_key_less(current, best);
            }
        }

        if (prefer_current) {
            selected = candidate;
        }
    }

    return selected;
}

void update_pool_summary(CudaDevicePool& pool) {
    CudaDevicePoolSummary summary{};
    summary.visible_device_count = pool.devices.size();

    CudaArchitectureFamily first_architecture =
        CudaArchitectureFamily::unknown;
    bool first_architecture_seen = false;

    for (const CudaPoolDevice& device : pool.devices) {
        if (device.discovered.has_stable_identity()) {
            ++summary.stable_identity_count;
        }
        if (device.registration_ready()) {
            ++summary.registration_ready_count;
        }
        if (device.health_available && device.health.status.completed()) {
            ++summary.health_completed_count;
        }
        if (device.runtime_binding_usable()) {
            ++summary.runtime_binding_usable_count;
        }

        if (device.discovered.capabilities_available) {
            const CudaDeviceCapabilities& capabilities =
                device.discovered.capabilities;
            summary.total_global_memory_bytes +=
                static_cast<std::uint64_t>(
                    capabilities.total_global_memory_bytes);

            const CudaArchitectureFamily architecture =
                capabilities.architecture.architecture;
            if (architecture != CudaArchitectureFamily::unknown) {
                if (!first_architecture_seen) {
                    first_architecture = architecture;
                    first_architecture_seen = true;
                } else if (architecture != first_architecture) {
                    summary.heterogeneous_architectures = true;
                }
            }
        }

        if (device.health_available &&
            device.health.snapshot.memory_free_bytes.available()) {
            summary.observed_free_memory_bytes +=
                device.health.snapshot.memory_free_bytes.value;
            ++summary.observed_free_memory_device_count;
        }
    }

    summary.peer_link_count = pool.peer_access.size();
    for (const CudaPeerAccessRecord& link : pool.peer_access) {
        if (link.state == CudaPeerAccessState::supported) {
            ++summary.peer_supported_link_count;
        }
    }

    pool.summary = summary;
}

[[nodiscard]] bool query_peer_access(
    const CudaPoolDevice& source,
    const CudaPoolDevice& target,
    CudaPeerAccessRecord& record) {
    record.source_persistent_key = source.persistent_key();
    record.target_persistent_key = target.persistent_key();
    record.source_runtime_ordinal =
        source.discovered.identity.runtime_ordinal;
    record.target_runtime_ordinal =
        target.discovered.identity.runtime_ordinal;

    if (record.source_persistent_key == record.target_persistent_key) {
        record.state = CudaPeerAccessState::same_device;
        return true;
    }

    if (record.source_runtime_ordinal < 0 ||
        record.target_runtime_ordinal < 0) {
        record.state = CudaPeerAccessState::query_failed;
        record.message =
            "Peer capability requires two usable CUDA runtime ordinals";
        return false;
    }

    int can_access = 0;
    const cudaError_t status = cudaDeviceCanAccessPeer(
        &can_access,
        record.source_runtime_ordinal,
        record.target_runtime_ordinal);
    if (status != cudaSuccess) {
        record.state = CudaPeerAccessState::query_failed;
        record.native_error_code = static_cast<int>(status);
        record.message = make_cuda_error_message(
            status,
            "cudaDeviceCanAccessPeer");
        return false;
    }

    record.state = can_access != 0
                       ? CudaPeerAccessState::supported
                       : CudaPeerAccessState::unsupported;
    return true;
}

}  // namespace

const CudaPoolDevice* CudaDevicePool::find_device(
    const std::string& persistent_key) const noexcept {
    for (const CudaPoolDevice& device : devices) {
        if (device.persistent_key() == persistent_key) {
            return &device;
        }
    }
    return nullptr;
}

CudaPoolDevice* CudaDevicePool::find_device(
    const std::string& persistent_key) noexcept {
    for (CudaPoolDevice& device : devices) {
        if (device.persistent_key() == persistent_key) {
            return &device;
        }
    }
    return nullptr;
}

const CudaPeerAccessRecord* CudaDevicePool::find_peer_access(
    const std::string& source_persistent_key,
    const std::string& target_persistent_key) const noexcept {
    for (const CudaPeerAccessRecord& record : peer_access) {
        if (record.source_persistent_key == source_persistent_key &&
            record.target_persistent_key == target_persistent_key) {
            return &record;
        }
    }
    return nullptr;
}

CudaDevicePoolBuildResult discover_cuda_device_pool(
    const CudaDevicePoolBuildOptions& options) {
    CudaDevicePoolBuildResult result{};
    CudaDevicePool& pool = result.pool;
    pool.discovery = discover_cuda_devices();

    if (!pool.discovery.status.completed()) {
        result.status.code = CudaDevicePoolBuildCode::discovery_failed;
        result.status.message = pool.discovery.status.message;
        return result;
    }

    bool partial =
        pool.discovery.status.code == CudaDiscoveryCode::partial_success;

    pool.devices.reserve(pool.discovery.devices.size());
    for (const CudaDiscoveredDevice& discovered :
         pool.discovery.devices) {
        CudaPoolDevice device{};
        device.discovered = discovered;
        device.health_was_requested = options.query_health;

        if (options.query_health &&
            discovered.identity.has_stable_identity()) {
            device.health = query_cuda_device_health(
                discovered.identity,
                options.health_options);
            device.health_available = true;
            if (device.health.status.code !=
                CudaHealthQueryCode::success) {
                partial = true;
            }
        } else if (options.query_health) {
            partial = true;
        }

        pool.devices.push_back(std::move(device));
    }

    if (options.query_peer_access) {
        for (const CudaPoolDevice& source : pool.devices) {
            if (!source.discovered.has_stable_identity()) {
                continue;
            }
            for (const CudaPoolDevice& target : pool.devices) {
                if (!target.discovered.has_stable_identity() ||
                    source.persistent_key() == target.persistent_key()) {
                    continue;
                }

                CudaPeerAccessRecord record{};
                if (!query_peer_access(source, target, record)) {
                    partial = true;
                }
                pool.peer_access.push_back(std::move(record));
            }
        }
    }

    update_pool_summary(pool);

    if (pool.discovery.status.code == CudaDiscoveryCode::no_devices ||
        pool.devices.empty()) {
        result.status.code = CudaDevicePoolBuildCode::no_devices;
        result.status.message = "No CUDA devices are visible to this process";
    } else if (partial) {
        result.status.code = CudaDevicePoolBuildCode::partial_success;
        result.status.message =
            "CUDA device pool discovery completed with partial information";
    }

    return result;
}

CudaTaskPlacementPlan plan_cuda_task_instances(
    const CudaDevicePool& pool,
    const CudaTaskInstanceRequest& request,
    const std::vector<CudaCapacityCommitment>& existing_commitments) {
    CudaTaskPlacementPlan plan{};
    plan.request_id = request.request_id;

    if (request.instance_count == 0) {
        plan.code = CudaTaskPlacementCode::invalid_request;
        append_placement_issue(
            plan,
            CudaPlacementIssueCode::invalid_request,
            {},
            0,
            "Task instance count must be greater than zero");
        return plan;
    }
    if (request.memory_bytes_per_instance == 0) {
        plan.code = CudaTaskPlacementCode::invalid_request;
        append_placement_issue(
            plan,
            CudaPlacementIssueCode::invalid_request,
            {},
            0,
            "Memory bytes per task instance must be greater than zero");
        return plan;
    }
    if (request.minimum_multiprocessor_count < 0) {
        plan.code = CudaTaskPlacementCode::invalid_request;
        append_placement_issue(
            plan,
            CudaPlacementIssueCode::invalid_request,
            {},
            0,
            "Minimum multiprocessor count cannot be negative");
        return plan;
    }

    plan.devices.reserve(pool.devices.size());
    for (const CudaPoolDevice& device : pool.devices) {
        plan.devices.push_back(make_capacity_view(
            device,
            request.require_observed_free_memory));
    }

    bool commitment_problem = false;
    for (const CudaCapacityCommitment& commitment :
         existing_commitments) {
        auto view_it = std::find_if(
            plan.devices.begin(),
            plan.devices.end(),
            [&commitment](const CudaDeviceCapacityView& view) {
                return view.persistent_key == commitment.persistent_key;
            });
        if (view_it == plan.devices.end()) {
            commitment_problem = true;
            append_placement_issue(
                plan,
                CudaPlacementIssueCode::unknown_commitment_device,
                commitment.persistent_key,
                0,
                "Existing commitment refers to a device outside this pool snapshot");
            continue;
        }

        view_it->preexisting_reserved_memory_bytes +=
            commitment.reserved_memory_bytes;
        view_it->preexisting_instance_count +=
            commitment.instance_count;

        if (commitment.reserved_memory_bytes >
            view_it->available_memory_bytes) {
            commitment_problem = true;
            view_it->available_memory_bytes = 0;
            append_placement_issue(
                plan,
                CudaPlacementIssueCode::commitment_exceeds_capacity,
                commitment.persistent_key,
                0,
                "Existing commitment exceeds the capacity visible in this snapshot");
        } else {
            view_it->available_memory_bytes -=
                commitment.reserved_memory_bytes;
        }
    }

    for (std::size_t instance_index = 0;
         instance_index < request.instance_count;
         ++instance_index) {
        std::vector<std::size_t> candidates{};
        candidates.reserve(plan.devices.size());

        for (std::size_t device_index = 0;
             device_index < plan.devices.size();
             ++device_index) {
            CudaDeviceCapacityView& view = plan.devices[device_index];
            const std::string& key = view.persistent_key;

            if (!request.allowed_device_keys.empty() &&
                !contains_key(request.allowed_device_keys, key)) {
                append_placement_issue(
                    plan,
                    CudaPlacementIssueCode::device_not_allowed,
                    key,
                    instance_index,
                    "Device is not in the request's allowed-device set");
                continue;
            }
            if (contains_key(request.excluded_device_keys, key)) {
                append_placement_issue(
                    plan,
                    CudaPlacementIssueCode::device_excluded,
                    key,
                    instance_index,
                    "Device is explicitly excluded by the request");
                continue;
            }
            if (request.require_registration_ready &&
                !view.registration_ready) {
                append_placement_issue(
                    plan,
                    CudaPlacementIssueCode::registration_not_ready,
                    key,
                    instance_index,
                    "Device is not ready for backend registration");
                continue;
            }
            if (request.require_runtime_binding &&
                !view.runtime_binding_usable) {
                append_placement_issue(
                    plan,
                    CudaPlacementIssueCode::runtime_binding_unusable,
                    key,
                    instance_index,
                    "Stable UUID is not currently matched to a usable runtime ordinal");
                continue;
            }
            if (!compute_capability_at_least(
                    view.compute_capability,
                    request.minimum_compute_capability)) {
                append_placement_issue(
                    plan,
                    CudaPlacementIssueCode::compute_capability_insufficient,
                    key,
                    instance_index,
                    "Device compute capability is below the request minimum");
                continue;
            }
            if (view.multiprocessor_count <
                request.minimum_multiprocessor_count) {
                append_placement_issue(
                    plan,
                    CudaPlacementIssueCode::multiprocessor_count_insufficient,
                    key,
                    instance_index,
                    "Device multiprocessor count is below the request minimum");
                continue;
            }
            if (view.memory_source ==
                CudaCapacityMemorySource::unavailable) {
                append_placement_issue(
                    plan,
                    CudaPlacementIssueCode::memory_observation_unavailable,
                    key,
                    instance_index,
                    request.require_observed_free_memory
                        ? "Observed free memory is required but unavailable"
                        : "No usable memory capacity is available");
                continue;
            }

            const std::size_t instances = total_instance_count(view);
            const std::size_t effective_limit =
                request.allow_multiple_instances_per_device
                    ? request.maximum_instances_per_device
                    : 1;
            if (effective_limit > 0 && instances >= effective_limit) {
                append_placement_issue(
                    plan,
                    CudaPlacementIssueCode::per_device_instance_limit_reached,
                    key,
                    instance_index,
                    "Device has reached the request's per-device instance limit");
                continue;
            }

            if (!can_fit_with_headroom(
                    view.available_memory_bytes,
                    request.memory_bytes_per_instance,
                    request.memory_headroom_bytes)) {
                append_placement_issue(
                    plan,
                    CudaPlacementIssueCode::memory_capacity_insufficient,
                    key,
                    instance_index,
                    "Device cannot fit the instance while preserving requested headroom");
                continue;
            }

            candidates.push_back(device_index);
        }

        if (candidates.empty()) {
            plan.unplaced_instance_indices.push_back(instance_index);
            append_placement_issue(
                plan,
                CudaPlacementIssueCode::no_eligible_device,
                {},
                instance_index,
                "No device in the pool can satisfy this task instance");
            continue;
        }

        const std::size_t selected_index = choose_candidate(
            plan.devices,
            candidates,
            request);
        CudaDeviceCapacityView& selected = plan.devices[selected_index];

        CudaTaskInstanceAssignment assignment{};
        assignment.instance_index = instance_index;
        assignment.persistent_key = selected.persistent_key;
        assignment.runtime_ordinal_snapshot =
            selected.runtime_ordinal_snapshot;
        assignment.reserved_memory_bytes =
            request.memory_bytes_per_instance;
        plan.assignments.push_back(std::move(assignment));

        selected.available_memory_bytes -=
            request.memory_bytes_per_instance;
        selected.planned_reserved_memory_bytes +=
            request.memory_bytes_per_instance;
        ++selected.planned_instance_count;
    }

    if (plan.unplaced_instance_indices.empty()) {
        plan.code = commitment_problem
                        ? CudaTaskPlacementCode::partial_success
                        : CudaTaskPlacementCode::success;
    } else if (plan.assignments.empty()) {
        plan.code = CudaTaskPlacementCode::unsatisfied;
    } else {
        plan.code = CudaTaskPlacementCode::partial_success;
    }

    return plan;
}

const char* to_string(CudaPeerAccessState value) noexcept {
    switch (value) {
        case CudaPeerAccessState::unknown:
            return "unknown";
        case CudaPeerAccessState::same_device:
            return "same_device";
        case CudaPeerAccessState::unsupported:
            return "unsupported";
        case CudaPeerAccessState::supported:
            return "supported";
        case CudaPeerAccessState::query_failed:
            return "query_failed";
    }
    return "unknown";
}

const char* to_string(CudaDevicePoolBuildCode value) noexcept {
    switch (value) {
        case CudaDevicePoolBuildCode::success:
            return "success";
        case CudaDevicePoolBuildCode::no_devices:
            return "no_devices";
        case CudaDevicePoolBuildCode::partial_success:
            return "partial_success";
        case CudaDevicePoolBuildCode::discovery_failed:
            return "discovery_failed";
    }
    return "unknown";
}

const char* to_string(CudaCapacityMemorySource value) noexcept {
    switch (value) {
        case CudaCapacityMemorySource::unavailable:
            return "unavailable";
        case CudaCapacityMemorySource::observed_free_memory:
            return "observed_free_memory";
        case CudaCapacityMemorySource::static_total_memory:
            return "static_total_memory";
    }
    return "unknown";
}

const char* to_string(CudaTaskPlacementPolicy value) noexcept {
    switch (value) {
        case CudaTaskPlacementPolicy::stable_order:
            return "stable_order";
        case CudaTaskPlacementPolicy::pack:
            return "pack";
        case CudaTaskPlacementPolicy::spread:
            return "spread";
    }
    return "unknown";
}

const char* to_string(CudaPlacementIssueCode value) noexcept {
    switch (value) {
        case CudaPlacementIssueCode::invalid_request:
            return "invalid_request";
        case CudaPlacementIssueCode::unknown_commitment_device:
            return "unknown_commitment_device";
        case CudaPlacementIssueCode::commitment_exceeds_capacity:
            return "commitment_exceeds_capacity";
        case CudaPlacementIssueCode::device_not_allowed:
            return "device_not_allowed";
        case CudaPlacementIssueCode::device_excluded:
            return "device_excluded";
        case CudaPlacementIssueCode::registration_not_ready:
            return "registration_not_ready";
        case CudaPlacementIssueCode::runtime_binding_unusable:
            return "runtime_binding_unusable";
        case CudaPlacementIssueCode::compute_capability_insufficient:
            return "compute_capability_insufficient";
        case CudaPlacementIssueCode::multiprocessor_count_insufficient:
            return "multiprocessor_count_insufficient";
        case CudaPlacementIssueCode::memory_observation_unavailable:
            return "memory_observation_unavailable";
        case CudaPlacementIssueCode::memory_capacity_insufficient:
            return "memory_capacity_insufficient";
        case CudaPlacementIssueCode::per_device_instance_limit_reached:
            return "per_device_instance_limit_reached";
        case CudaPlacementIssueCode::no_eligible_device:
            return "no_eligible_device";
    }
    return "unknown";
}

const char* to_string(CudaTaskPlacementCode value) noexcept {
    switch (value) {
        case CudaTaskPlacementCode::success:
            return "success";
        case CudaTaskPlacementCode::partial_success:
            return "partial_success";
        case CudaTaskPlacementCode::unsatisfied:
            return "unsatisfied";
        case CudaTaskPlacementCode::invalid_request:
            return "invalid_request";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cuda
