#include "cuda_device.hpp"

#include <iomanip>
#include <iostream>

namespace cuda_backend = prometheus::backends::cuda;

namespace {

void print_pci_address(const cuda_backend::CudaPciAddress& address) {
    if (!address.valid) {
        std::cout << "unparsed";
        return;
    }

    std::cout << std::hex << std::setfill('0');
    if (address.domain_was_present) {
        std::cout << std::setw(4) << address.domain << ':';
    }
    std::cout << std::setw(2) << address.bus << ':'
              << std::setw(2) << address.device << '.'
              << address.function << std::dec << std::setfill(' ');
}

}  // namespace

int main() {
    const cuda_backend::CudaDeviceDiscoveryResult discovery =
        cuda_backend::discover_cuda_devices();

    std::cout << "Prometheus CUDA device discovery probe\n";
    std::cout << "Discovery status: "
              << cuda_backend::to_string(discovery.status.code) << '\n';
    if (!discovery.status.message.empty()) {
        std::cout << "Status message: " << discovery.status.message << '\n';
    }
    std::cout << "Reported visible devices: "
              << discovery.reported_device_count << '\n';
    std::cout << "Stable identities: "
              << discovery.stable_identity_count << '\n';
    std::cout << "Capability records: "
              << discovery.capability_count << '\n';
    std::cout << "Registration-ready records: "
              << discovery.registration_ready_count << "\n\n";

    for (const cuda_backend::CudaDiscoveredDevice& device :
         discovery.devices) {
        std::cout << "Device ordinal "
                  << device.identity.runtime_ordinal << '\n';
        std::cout << "  identity query: "
                  << cuda_backend::to_string(device.identity_status.code)
                  << '\n';
        std::cout << "  name: "
                  << (device.identity.name.empty()
                          ? "<unavailable>"
                          : device.identity.name)
                  << '\n';
        std::cout << "  UUID: "
                  << (device.identity.uuid_available
                          ? device.identity.uuid
                          : "<unavailable>")
                  << '\n';
        std::cout << "  persistent key: "
                  << (device.identity.persistent_key.empty()
                          ? "<unavailable>"
                          : device.identity.persistent_key)
                  << '\n';
        std::cout << "  PCI bus ID: "
                  << (device.identity.pci_bus_id_available
                          ? device.identity.pci_bus_id
                          : "<unavailable>")
                  << '\n';
        std::cout << "  parsed PCI address: ";
        print_pci_address(device.identity.pci_address);
        std::cout << '\n';
        std::cout << "  stable identity: "
                  << (device.has_stable_identity() ? "yes" : "no")
                  << '\n';
        std::cout << "  capabilities available: "
                  << (device.capabilities_available ? "yes" : "no")
                  << '\n';
        if (device.capabilities_available) {
            std::cout << "  compute capability: "
                      << device.capabilities.architecture.compute_capability.major
                      << '.'
                      << device.capabilities.architecture.compute_capability.minor
                      << '\n';
            std::cout << "  architecture: "
                      << cuda_backend::to_string(
                             device.capabilities.architecture.architecture)
                      << '\n';
        }
        std::cout << "  backend-registration ready: "
                  << (device.ready_for_backend_registration()
                          ? "yes"
                          : "no")
                  << '\n';

        if (device.issues.empty()) {
            std::cout << "  issues: none\n";
        } else {
            std::cout << "  issues:\n";
            for (const cuda_backend::CudaDeviceIssue& issue :
                 device.issues) {
                std::cout << "    - "
                          << cuda_backend::to_string(issue.code)
                          << " [" << (issue.fatal ? "fatal" : "non-fatal")
                          << "]: " << issue.message << '\n';
            }
        }
        std::cout << '\n';
    }

    if (!discovery.status.completed()) {
        return 1;
    }
    if (discovery.status.code == cuda_backend::CudaDiscoveryCode::partial_success) {
        return 2;
    }
    return 0;
}
