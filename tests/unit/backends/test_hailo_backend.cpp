#include "hailo_backend.hpp"

#include <cassert>

using namespace prometheus::backends;
using namespace prometheus::backends::accelerator;

int main() {
    assert(evaluate_hailo_host({true, "Raspberry Pi 5 Model B Rev 1.0", {}}) ==
           HailoHostEligibility::eligible);
    assert(evaluate_hailo_host({true, "Raspberry Pi 10 Model B", {}}) ==
           HailoHostEligibility::eligible);
    assert(evaluate_hailo_host({true, "Raspberry Pi 4 Model B", {}}) ==
           HailoHostEligibility::raspberry_pi_before_5);
    assert(evaluate_hailo_host({false, "Raspberry Pi 5 Model B", {}}) ==
           HailoHostEligibility::not_aarch64);

    HailoBackend backend{[] {
        return HailoHostObservation{
            true, "Raspberry Pi 5 Model B Rev 1.0", {"/dev/hailo0"}};
    }};
    const ComputeBackendStatus initialized = backend.initialize();
    assert(initialized.state == ComputeBackendState::degraded);
    assert(!initialized.usable);

    const HailoCapabilitySnapshot capabilities = backend.capabilities();
    assert(capabilities.eligibility == HailoHostEligibility::eligible);
    assert(capabilities.devices.size() == 1);
    assert(capabilities.devices.front().device_path == "/dev/hailo0");

    const ComputeBackendSnapshot snapshot = backend.snapshot();
    assert(snapshot.visible_resource_count == 1);
    assert(snapshot.usable_resource_count == 0);

    backend.shutdown();
    assert(backend.status().state == ComputeBackendState::stopped);
    return 0;
}
