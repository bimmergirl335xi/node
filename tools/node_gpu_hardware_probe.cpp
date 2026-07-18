#include <cstdlib>
#include <iomanip>
#include <iostream>

#include "linux_pci_inventory.hpp"

namespace ph = prometheus::hardware;

namespace {

template <typename T>
void print_hex(T value, int width) {
    std::cout << "0x" << std::hex << std::nouppercase
              << std::setw(width) << std::setfill('0')
              << static_cast<std::uint64_t>(value)
              << std::dec << std::setfill(' ');
}

}  // namespace

int main() {
    const ph::LinuxPciInventoryResult result =
        ph::inventory_linux_pci_devices();

    std::cout << "GPU_HARDWARE_INVENTORY status="
              << ph::to_string(result.code)
              << " complete=" << (result.complete ? "true" : "false")
              << " scanned=" << result.scanned_entries
              << " matching=" << result.matching_entries
              << " devices=" << result.devices.size()
              << " issues=" << result.issues.size()
              << " dropped_issues=" << result.dropped_issues << '\n';

    for (const ph::LinuxPciDevice& device : result.devices) {
        std::cout << "GPU_DEVICE pci=" << device.address.canonical
                  << " vendor=";
        print_hex(device.identity.vendor_id, 4);
        std::cout << " device=";
        print_hex(device.identity.device_id, 4);
        std::cout << " subsystem_vendor=";
        if (device.identity.subsystem_vendor_observed) {
            print_hex(device.identity.subsystem_vendor_id, 4);
        } else {
            std::cout << "unknown";
        }
        std::cout << " subsystem_device=";
        if (device.identity.subsystem_device_observed) {
            print_hex(device.identity.subsystem_device_id, 4);
        } else {
            std::cout << "unknown";
        }
        std::cout << " class=";
        print_hex(device.identity.class_code, 6);
        std::cout << " revision=";
        if (device.identity.revision_observed) {
            print_hex(device.identity.revision, 2);
        } else {
            std::cout << "unknown";
        }
        std::cout << " vendor_family="
                  << ph::to_string(device.vendor_family)
                  << " class_family=" << ph::to_string(device.device_class)
                  << " driver_state=" << ph::to_string(device.driver_state)
                  << " driver=";
        if (device.driver_state == ph::DriverBindingState::bound) {
            std::cout << device.driver_name;
        } else if (device.driver_state == ph::DriverBindingState::unbound) {
            std::cout << "none";
        } else {
            std::cout << "unknown";
        }
        std::cout << '\n';
    }

    for (const ph::LinuxPciInventoryIssue& issue : result.issues) {
        std::cout << "GPU_INVENTORY_ISSUE code="
                  << ph::to_string(issue.code)
                  << " entry="
                  << (issue.entry.empty() ? "unknown" : issue.entry)
                  << " native_error=" << issue.native_error_code << '\n';
    }

    std::cout << "CUDA_RUNTIME status=not_evaluated\n";

    if (result.code == ph::LinuxPciInventoryCode::success ||
        result.code == ph::LinuxPciInventoryCode::partial) {
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
