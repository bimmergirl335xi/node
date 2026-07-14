#include "hailo_backend.hpp"

#include <array>
#include <cctype>
#include <fstream>
#include <utility>

namespace prometheus::backends::accelerator {
namespace {

std::string read_board_model() {
    std::ifstream input("/proc/device-tree/model", std::ios::binary);
    std::string model{};
    std::getline(input, model, '\0');
    return model;
}

bool file_exists(const char* path) {
    std::ifstream input(path);
    return input.good();
}

int raspberry_pi_generation(const std::string& model) {
    constexpr const char prefix[] = "Raspberry Pi ";
    const std::size_t position = model.find(prefix);
    if (position == std::string::npos) {
        return 0;
    }
    std::size_t digit = position + sizeof(prefix) - 1;
    if (digit >= model.size() || !std::isdigit(
            static_cast<unsigned char>(model[digit]))) {
        return 0;
    }
    int generation = 0;
    while (digit < model.size() && std::isdigit(
               static_cast<unsigned char>(model[digit]))) {
        generation = generation * 10 + (model[digit] - '0');
        ++digit;
    }
    return generation;
}

}  // namespace

HailoBackend::HailoBackend(HailoProbe probe)
    : probe_(probe ? std::move(probe) : HailoProbe{probe_local_hailo_host}) {}

std::string_view HailoBackend::backend_id() const noexcept {
    return "hailo";
}

std::string_view HailoBackend::display_name() const noexcept {
    return "Hailo accelerator";
}

ComputeBackendKind HailoBackend::kind() const noexcept {
    return ComputeBackendKind::accelerator;
}

ComputeBackendStatus HailoBackend::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    status_.state = ComputeBackendState::initializing;
    status_.usable = false;
    status_.message = "Probing Hailo accelerator support";
    return probe_locked();
}

ComputeBackendStatus HailoBackend::refresh() {
    std::lock_guard<std::mutex> lock(mutex_);
    return probe_locked();
}

void HailoBackend::shutdown() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    status_ = {ComputeBackendState::stopped, false,
               "Hailo backend is stopped"};
}

ComputeBackendStatus HailoBackend::status() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

ComputeBackendSnapshot HailoBackend::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {"hailo", "Hailo accelerator", ComputeBackendKind::accelerator,
            status_.state, status_.usable, capabilities_.devices.size(),
            status_.usable ? capabilities_.devices.size() : 0,
            status_.message};
}

HailoCapabilitySnapshot HailoBackend::capabilities() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return capabilities_;
}

ComputeBackendStatus HailoBackend::probe_locked() {
    capabilities_ = {};
    capabilities_.host = probe_();
    capabilities_.eligibility = evaluate_hailo_host(capabilities_.host);

    if (capabilities_.eligibility != HailoHostEligibility::eligible) {
        capabilities_.message = std::string{"Unsupported Hailo host: "} +
                                to_string(capabilities_.eligibility);
        status_ = {ComputeBackendState::unavailable, false,
                   capabilities_.message};
        return status_;
    }

    for (std::size_t index = 0;
         index < capabilities_.host.device_paths.size(); ++index) {
        capabilities_.devices.push_back(
            {AcceleratorVendor::hailo, "hailo" + std::to_string(index),
             capabilities_.host.device_paths[index], "Hailo NPU"});
    }

    if (capabilities_.devices.empty()) {
        capabilities_.message =
            "Eligible Raspberry Pi host has no visible Hailo device";
        status_ = {ComputeBackendState::unavailable, false,
                   capabilities_.message};
        return status_;
    }

    // HailoRT integration is intentionally deferred. A visible device is
    // reported as degraded rather than usable until the runtime is loaded.
    capabilities_.message =
        "Hailo device detected; HailoRT integration is not loaded yet";
    status_ = {ComputeBackendState::degraded, false, capabilities_.message};
    return status_;
}

HailoHostObservation probe_local_hailo_host() {
    HailoHostObservation result{};
#if defined(__aarch64__)
    result.aarch64 = true;
#endif
    result.board_model = read_board_model();
    constexpr std::array<const char*, 4> paths{
        "/dev/hailo0", "/dev/hailo1", "/dev/hailo2", "/dev/hailo3"};
    for (const char* path : paths) {
        if (file_exists(path)) {
            result.device_paths.emplace_back(path);
        }
    }
    return result;
}

HailoHostEligibility evaluate_hailo_host(
    const HailoHostObservation& observation) noexcept {
    if (!observation.aarch64) {
        return HailoHostEligibility::not_aarch64;
    }
    if (observation.board_model.empty()) {
        return HailoHostEligibility::board_model_unavailable;
    }
    if (observation.board_model.find("Raspberry Pi") == std::string::npos) {
        return HailoHostEligibility::not_raspberry_pi;
    }
    return raspberry_pi_generation(observation.board_model) >= 5
               ? HailoHostEligibility::eligible
               : HailoHostEligibility::raspberry_pi_before_5;
}

const char* to_string(HailoHostEligibility value) noexcept {
    switch (value) {
        case HailoHostEligibility::eligible: return "eligible";
        case HailoHostEligibility::not_aarch64: return "not_aarch64";
        case HailoHostEligibility::not_raspberry_pi: return "not_raspberry_pi";
        case HailoHostEligibility::raspberry_pi_before_5:
            return "raspberry_pi_before_5";
        case HailoHostEligibility::board_model_unavailable:
            return "board_model_unavailable";
    }
    return "unknown";
}

}  // namespace prometheus::backends::accelerator
