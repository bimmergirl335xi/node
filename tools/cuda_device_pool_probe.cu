#include "cuda_device_pool.hpp"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

namespace cuda_backend = prometheus::backends::cuda;

namespace {

constexpr std::uint64_t kMebibyte = 1024ULL * 1024ULL;
constexpr std::uint64_t kGibibyte = 1024ULL * 1024ULL * 1024ULL;

void print_bytes(const char* label, std::uint64_t bytes) {
    const double gib = static_cast<double>(bytes) /
                       static_cast<double>(kGibibyte);
    std::cout << label << bytes << " bytes ("
              << std::fixed << std::setprecision(2) << gib
              << " GiB)" << std::defaultfloat;
}

void print_plan(const cuda_backend::CudaTaskPlacementPlan& plan) {
    std::cout << "Plan "
              << (plan.request_id.empty() ? "<unnamed>" : plan.request_id)
              << "\n";
    std::cout << "  status: "
              << cuda_backend::to_string(plan.code) << '\n';
    std::cout << "  assignments: " << plan.assignments.size() << '\n';
    std::cout << "  unplaced instances: "
              << plan.unplaced_instance_indices.size() << '\n';

    for (const cuda_backend::CudaTaskInstanceAssignment& assignment :
         plan.assignments) {
        std::cout << "    instance " << assignment.instance_index
                  << " -> " << assignment.persistent_key
                  << " (ordinal snapshot "
                  << assignment.runtime_ordinal_snapshot << ", ";
        print_bytes("reservation ", assignment.reserved_memory_bytes);
        std::cout << ")\n";
    }

    std::cout << "  per-device advisory totals:\n";
    for (const cuda_backend::CudaDeviceCapacityView& device :
         plan.devices) {
        std::cout << "    " << device.persistent_key
                  << ": planned instances="
                  << device.planned_instance_count << ", ";
        print_bytes(
            "planned memory=",
            device.planned_reserved_memory_bytes);
        std::cout << ", source="
                  << cuda_backend::to_string(device.memory_source)
                  << '\n';
    }
}

}  // namespace

int main() {
    const cuda_backend::CudaDevicePoolBuildResult build =
        cuda_backend::discover_cuda_device_pool();

    std::cout << "Prometheus CUDA device-pool probe\n";
    std::cout << "Pool status: "
              << cuda_backend::to_string(build.status.code) << '\n';
    if (!build.status.message.empty()) {
        std::cout << "Status message: " << build.status.message << '\n';
    }

    const cuda_backend::CudaDevicePool& pool = build.pool;
    std::cout << "Visible devices: "
              << pool.summary.visible_device_count << '\n';
    std::cout << "Stable identities: "
              << pool.summary.stable_identity_count << '\n';
    std::cout << "Registration-ready devices: "
              << pool.summary.registration_ready_count << '\n';
    std::cout << "Usable runtime bindings: "
              << pool.summary.runtime_binding_usable_count << '\n';
    std::cout << "Observed-memory devices: "
              << pool.summary.observed_free_memory_device_count << '\n';
    std::cout << "Heterogeneous architectures: "
              << (pool.summary.heterogeneous_architectures ? "yes" : "no")
              << '\n';
    print_bytes(
        "Total static GPU memory: ",
        pool.summary.total_global_memory_bytes);
    std::cout << '\n';
    print_bytes(
        "Observed free GPU memory: ",
        pool.summary.observed_free_memory_bytes);
    std::cout << "\n\n";

    for (const cuda_backend::CudaPoolDevice& device : pool.devices) {
        std::cout << "Device " << device.persistent_key() << '\n';
        std::cout << "  ordinal snapshot: "
                  << device.discovered.identity.runtime_ordinal << '\n';
        std::cout << "  architecture: ";
        if (device.discovered.capabilities_available) {
            std::cout << cuda_backend::to_string(
                device.discovered.capabilities.architecture.architecture);
        } else {
            std::cout << "unknown";
        }
        std::cout << '\n';
        std::cout << "  registration ready: "
                  << (device.registration_ready() ? "yes" : "no") << '\n';
        std::cout << "  runtime binding usable: "
                  << (device.runtime_binding_usable() ? "yes" : "no")
                  << '\n';
        if (device.health_available &&
            device.health.snapshot.memory_free_bytes.available()) {
            print_bytes(
                "  observed free memory: ",
                device.health.snapshot.memory_free_bytes.value);
            std::cout << '\n';
        }
    }

    std::cout << "\nDirected peer capability records: "
              << pool.summary.peer_link_count << '\n';
    for (const cuda_backend::CudaPeerAccessRecord& link :
         pool.peer_access) {
        std::cout << "  " << link.source_persistent_key
                  << " -> " << link.target_persistent_key
                  << ": " << cuda_backend::to_string(link.state);
        if (!link.message.empty()) {
            std::cout << " (" << link.message << ')';
        }
        std::cout << '\n';
    }
    std::cout << '\n';

    cuda_backend::CudaTaskInstanceRequest spread{};
    spread.request_id = "four-independent-specialists";
    spread.instance_count = 4;
    spread.memory_bytes_per_instance = 256ULL * kMebibyte;
    spread.memory_headroom_bytes = 256ULL * kMebibyte;
    spread.minimum_compute_capability = {6, 0};
    spread.policy = cuda_backend::CudaTaskPlacementPolicy::spread;

    const cuda_backend::CudaTaskPlacementPlan spread_plan =
        cuda_backend::plan_cuda_task_instances(pool, spread);
    print_plan(spread_plan);
    std::cout << '\n';

    cuda_backend::CudaTaskPlacementPlan pinned_plan{};
    if (!pool.devices.empty()) {
        cuda_backend::CudaTaskInstanceRequest pinned{};
        pinned.request_id = "two-specialists-on-one-device";
        pinned.instance_count = 2;
        pinned.memory_bytes_per_instance = 128ULL * kMebibyte;
        pinned.memory_headroom_bytes = 256ULL * kMebibyte;
        pinned.minimum_compute_capability = {6, 0};
        pinned.allowed_device_keys.push_back(
            pool.devices.front().persistent_key());
        pinned.policy = cuda_backend::CudaTaskPlacementPolicy::pack;
        pinned_plan = cuda_backend::plan_cuda_task_instances(pool, pinned);
        print_plan(pinned_plan);
    }

    if (!build.status.completed()) {
        return 1;
    }
    if (!spread_plan.fully_satisfied() ||
        (!pool.devices.empty() && !pinned_plan.fully_satisfied())) {
        return 1;
    }
    if (!build.status.complete()) {
        return 2;
    }
    return 0;
}
