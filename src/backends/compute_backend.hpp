#ifndef SRC_BACKENDS_COMPUTE_BACKEND_HPP
#define SRC_BACKENDS_COMPUTE_BACKEND_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace prometheus::backends {

// Generic backend vocabulary. A backend describes one class of compute
// resources; it does not become a cognitive master and it does not imply that
// every node has the same hardware.
enum class ComputeBackendKind : std::uint8_t {
    unknown = 0,
    cpu,
    cuda,
    arm,
    accelerator,
    xeon_phi,
};

enum class ComputeBackendState : std::uint8_t {
    constructed = 0,
    initializing,
    ready,
    degraded,
    unavailable,
    failed,
    stopped,
};

struct ComputeBackendStatus {
    ComputeBackendState state = ComputeBackendState::constructed;
    bool usable = false;
    std::string message{};
};

struct ComputeBackendSnapshot {
    std::string backend_id{};
    std::string display_name{};
    ComputeBackendKind kind = ComputeBackendKind::unknown;
    ComputeBackendState state = ComputeBackendState::constructed;
    bool usable = false;
    std::size_t visible_resource_count = 0;
    std::size_t usable_resource_count = 0;
    std::string message{};
};

// No global singleton is provided. Ownership and initialization order belong to
// the node runtime/service manager so minimal headless nodes can instantiate
// only the backends their release contains.
class ComputeBackend {
public:
    virtual ~ComputeBackend() = default;

    [[nodiscard]] virtual std::string_view backend_id() const noexcept = 0;
    [[nodiscard]] virtual std::string_view display_name() const noexcept = 0;
    [[nodiscard]] virtual ComputeBackendKind kind() const noexcept = 0;

    virtual ComputeBackendStatus initialize() = 0;
    virtual ComputeBackendStatus refresh() = 0;
    virtual void shutdown() noexcept = 0;

    [[nodiscard]] virtual ComputeBackendStatus status() const = 0;
    [[nodiscard]] virtual ComputeBackendSnapshot snapshot() const = 0;
};

[[nodiscard]] inline const char* to_string(ComputeBackendKind value) noexcept {
    switch (value) {
        case ComputeBackendKind::cpu:
            return "cpu";
        case ComputeBackendKind::cuda:
            return "cuda";
        case ComputeBackendKind::arm:
            return "arm";
        case ComputeBackendKind::accelerator:
            return "accelerator";
        case ComputeBackendKind::xeon_phi:
            return "xeon_phi";
        case ComputeBackendKind::unknown:
            return "unknown";
    }
    return "unknown";
}

[[nodiscard]] inline const char* to_string(ComputeBackendState value) noexcept {
    switch (value) {
        case ComputeBackendState::constructed:
            return "constructed";
        case ComputeBackendState::initializing:
            return "initializing";
        case ComputeBackendState::ready:
            return "ready";
        case ComputeBackendState::degraded:
            return "degraded";
        case ComputeBackendState::unavailable:
            return "unavailable";
        case ComputeBackendState::failed:
            return "failed";
        case ComputeBackendState::stopped:
            return "stopped";
    }
    return "unknown";
}

}  // namespace prometheus::backends

#endif  // SRC_BACKENDS_COMPUTE_BACKEND_HPP
