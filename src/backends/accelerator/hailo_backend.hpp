#ifndef SRC_BACKENDS_ACCELERATOR_HAILO_BACKEND_HPP
#define SRC_BACKENDS_ACCELERATOR_HAILO_BACKEND_HPP

#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "../compute_backend.hpp"
#include "accelerator_backend.hpp"

namespace prometheus::backends::accelerator {

struct HailoHostObservation {
    bool aarch64 = false;
    std::string board_model{};
    std::vector<std::string> device_paths{};
};

enum class HailoHostEligibility {
    eligible = 0,
    not_aarch64,
    not_raspberry_pi,
    raspberry_pi_before_5,
    board_model_unavailable,
};

struct HailoCapabilitySnapshot {
    HailoHostObservation host{};
    HailoHostEligibility eligibility =
        HailoHostEligibility::board_model_unavailable;
    std::vector<AcceleratorDeviceIdentity> devices{};
    bool hailort_runtime_loaded = false;
    std::string message{};
};

using HailoProbe = std::function<HailoHostObservation()>;

// The first Hailo backend slice deliberately supports only AArch64 Raspberry
// Pi 5-or-later hosts. Device discovery does not imply that HailoRT is loaded
// or that a model is executable.
class HailoBackend final : public ComputeBackend {
public:
    explicit HailoBackend(HailoProbe probe = {});

    [[nodiscard]] std::string_view backend_id() const noexcept override;
    [[nodiscard]] std::string_view display_name() const noexcept override;
    [[nodiscard]] ComputeBackendKind kind() const noexcept override;

    ComputeBackendStatus initialize() override;
    ComputeBackendStatus refresh() override;
    void shutdown() noexcept override;

    [[nodiscard]] ComputeBackendStatus status() const override;
    [[nodiscard]] ComputeBackendSnapshot snapshot() const override;
    [[nodiscard]] HailoCapabilitySnapshot capabilities() const;

private:
    ComputeBackendStatus probe_locked();

    HailoProbe probe_{};
    mutable std::mutex mutex_{};
    ComputeBackendStatus status_{};
    HailoCapabilitySnapshot capabilities_{};
};

[[nodiscard]] HailoHostObservation probe_local_hailo_host();
[[nodiscard]] HailoHostEligibility evaluate_hailo_host(
    const HailoHostObservation& observation) noexcept;
[[nodiscard]] const char* to_string(HailoHostEligibility value) noexcept;

}  // namespace prometheus::backends::accelerator

#endif  // SRC_BACKENDS_ACCELERATOR_HAILO_BACKEND_HPP
