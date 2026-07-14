#include <cstdlib>
#include <memory>

#include "backend_registry.hpp"
#include "cpu_backend.hpp"

namespace pb = prometheus::backends;
namespace pc = prometheus::backends::cpu;

int main() {
    auto backend = std::make_shared<pc::CpuBackend>();

    if (backend->backend_id() != "cpu" ||
        backend->kind() != pb::ComputeBackendKind::cpu ||
        backend->status().state != pb::ComputeBackendState::constructed) {
        return EXIT_FAILURE;
    }

    pb::BackendRegistry registry{};
    if (!registry.register_backend(backend).ok()) {
        return EXIT_FAILURE;
    }
    if (registry.register_backend(backend).code !=
        pb::BackendRegistrationCode::duplicate_backend_id) {
        return EXIT_FAILURE;
    }
    if (registry.size() != 1 || registry.find_backend("cpu") != backend) {
        return EXIT_FAILURE;
    }

    const pb::ComputeBackendStatus status = backend->initialize();
    if (!status.usable ||
        (status.state != pb::ComputeBackendState::ready &&
         status.state != pb::ComputeBackendState::degraded)) {
        return EXIT_FAILURE;
    }

    const pb::ComputeBackendSnapshot snapshot = backend->snapshot();
    if (snapshot.backend_id != "cpu" ||
        snapshot.kind != pb::ComputeBackendKind::cpu ||
        !snapshot.usable || snapshot.visible_resource_count == 0 ||
        snapshot.usable_resource_count == 0 ||
        snapshot.usable_resource_count > snapshot.visible_resource_count) {
        return EXIT_FAILURE;
    }

    const pc::CpuBackendInventorySnapshot inventory =
        backend->inventory_snapshot();
    if (!inventory.topology_status.completed() ||
        inventory.topology.logical_processors.empty() ||
        inventory.capabilities.target_logical_processor_count == 0) {
        return EXIT_FAILURE;
    }

    const std::vector<pb::ComputeBackendSnapshot> snapshots =
        registry.snapshots();
    if (snapshots.size() != 1 || snapshots.front().backend_id != "cpu") {
        return EXIT_FAILURE;
    }

    const pb::ComputeBackendStatus refreshed = backend->refresh();
    if (!refreshed.usable) {
        return EXIT_FAILURE;
    }

    backend->shutdown();
    const pb::ComputeBackendSnapshot stopped = backend->snapshot();
    return backend->status().state == pb::ComputeBackendState::stopped &&
                   !stopped.usable && stopped.visible_resource_count == 0 &&
                   stopped.usable_resource_count == 0
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
