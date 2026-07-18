#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#include "linux_pci_inventory.hpp"

namespace fs = std::filesystem;
namespace ph = prometheus::hardware;

namespace {

class FixtureRoot {
public:
    FixtureRoot() {
        char pattern[] = "/tmp/prometheus-pci-inventory-XXXXXX";
        const char* created = ::mkdtemp(pattern);
        if (created != nullptr) root_ = created;
    }

    ~FixtureRoot() {
        std::error_code error{};
        fs::remove_all(root_, error);
    }

    FixtureRoot(const FixtureRoot&) = delete;
    FixtureRoot& operator=(const FixtureRoot&) = delete;

    [[nodiscard]] bool valid() const noexcept { return !root_.empty(); }
    [[nodiscard]] const fs::path& path() const noexcept { return root_; }

    fs::path add_device(
        const std::string& address,
        const std::string& vendor,
        const std::string& device,
        const std::string& device_class) const {
        const fs::path directory = root_ / address;
        fs::create_directories(directory);
        write(directory / "vendor", vendor);
        write(directory / "device", device);
        write(directory / "class", device_class);
        return directory;
    }

    static void write(const fs::path& path, const std::string& value) {
        std::ofstream output{path};
        output << value << '\n';
    }

private:
    fs::path root_{};
};

ph::LinuxPciInventoryOptions options_for(const FixtureRoot& fixture) {
    ph::LinuxPciInventoryOptions options{};
    options.inventory_root = fixture.path().string();
    options.maximum_scanned_entries = 32;
    options.maximum_devices = 16;
    options.maximum_issues = 16;
    return options;
}

bool has_issue(
    const ph::LinuxPciInventoryResult& result,
    ph::LinuxPciInventoryIssueCode code) {
    for (const auto& issue : result.issues) {
        if (issue.code == code) return true;
    }
    return false;
}

bool test_address_parser() {
    const auto parsed = ph::parse_pci_address("ABCD:ef:1f.7");
    const auto invalid_device = ph::parse_pci_address("0000:01:20.0");
    const auto invalid_function = ph::parse_pci_address("0000:01:00.8");
    const auto invalid_suffix = ph::parse_pci_address("0000:01:00.0-extra");
    const bool passed = parsed.valid && parsed.domain == 0xabcdU && parsed.bus == 0xefU &&
           parsed.device == 0x1fU && parsed.function == 7U &&
           parsed.canonical == "abcd:ef:1f.7" &&
           !invalid_device.valid && !invalid_function.valid && !invalid_suffix.valid;
    if (!passed) {
        std::cerr << "address valid=" << parsed.valid
                  << " domain=" << parsed.domain
                  << " bus=" << static_cast<unsigned>(parsed.bus)
                  << " device=" << static_cast<unsigned>(parsed.device)
                  << " function=" << static_cast<unsigned>(parsed.function)
                  << " canonical=" << parsed.canonical
                  << " invalid_device=" << invalid_device.valid
                  << " invalid_function=" << invalid_function.valid
                  << " invalid_suffix=" << invalid_suffix.valid << '\n';
    }
    return passed;
}

bool test_nvidia_unbound() {
    FixtureRoot fixture{};
    if (!fixture.valid()) return false;
    fixture.add_device("0000:01:00.0", "0x10de", "0x1eb1", "0x030200");
    const auto result = ph::inventory_linux_pci_devices(options_for(fixture));
    return result.code == ph::LinuxPciInventoryCode::success && result.complete &&
           result.devices.size() == 1 &&
           result.devices[0].vendor_family == ph::PciVendorFamily::nvidia &&
           result.devices[0].device_class == ph::PciDeviceClass::three_d_controller &&
           result.devices[0].driver_state == ph::DriverBindingState::unbound;
}

bool test_bound_driver(const std::string& driver) {
    FixtureRoot fixture{};
    if (!fixture.valid()) return false;
    const fs::path device = fixture.add_device(
        "0000:02:00.0", "0x10de", "0x1eb1", "0x030200");
    fs::create_directory_symlink(fs::path{"/drivers"} / driver, device / "driver");
    const auto result = ph::inventory_linux_pci_devices(options_for(fixture));
    return result.code == ph::LinuxPciInventoryCode::success &&
           result.devices.size() == 1 &&
           result.devices[0].driver_state == ph::DriverBindingState::bound &&
           result.devices[0].driver_name == driver;
}

bool test_vendor(
    std::uint16_t expected,
    ph::PciVendorFamily expected_family,
    const std::string& vendor) {
    FixtureRoot fixture{};
    if (!fixture.valid()) return false;
    fixture.add_device("0000:03:00.0", vendor, "0x1234", "0x030000");
    const auto result = ph::inventory_linux_pci_devices(options_for(fixture));
    return result.code == ph::LinuxPciInventoryCode::success &&
           result.devices.size() == 1 &&
           result.devices[0].identity.vendor_id == expected &&
           result.devices[0].vendor_family == expected_family;
}

bool test_non_gpu_and_empty() {
    FixtureRoot fixture{};
    if (!fixture.valid()) return false;
    fixture.add_device("0000:04:00.0", "0x10de", "0x1234", "0x020000");
    const auto result = ph::inventory_linux_pci_devices(options_for(fixture));
    return result.code == ph::LinuxPciInventoryCode::success && result.complete &&
           result.devices.empty() && result.matching_entries == 0;
}

bool test_empty_root() {
    FixtureRoot fixture{};
    if (!fixture.valid()) return false;
    const auto result = ph::inventory_linux_pci_devices(options_for(fixture));
    return result.code == ph::LinuxPciInventoryCode::success && result.complete &&
           result.devices.empty() && result.scanned_entries == 0;
}

bool test_unavailable_root() {
    ph::LinuxPciInventoryOptions options{};
    options.inventory_root = "/tmp/prometheus-pci-inventory-does-not-exist";
    const auto result = ph::inventory_linux_pci_devices(options);
    return result.code == ph::LinuxPciInventoryCode::inventory_root_unavailable &&
           !result.complete && result.devices.empty();
}

bool test_malformed_entries() {
    FixtureRoot fixture{};
    if (!fixture.valid()) return false;
    fixture.add_device("invalid-address", "0x10de", "0x1eb1", "0x030200");
    fixture.add_device("0000:05:00.0", "not-hex", "0x1eb1", "0x030200");
    fixture.add_device("0000:06:00.0", "0x10de", "0x1eb1", "0x03xx00");
    const auto result = ph::inventory_linux_pci_devices(options_for(fixture));
    return result.code == ph::LinuxPciInventoryCode::malformed_entry &&
           !result.complete && result.devices.empty() &&
           has_issue(result, ph::LinuxPciInventoryIssueCode::malformed_address) &&
           has_issue(result, ph::LinuxPciInventoryIssueCode::malformed_attribute);
}

bool test_capacity_exhaustion() {
    FixtureRoot fixture{};
    if (!fixture.valid()) return false;
    fixture.add_device("0000:07:00.0", "0x1002", "0x1111", "0x030000");
    fixture.add_device("0000:08:00.0", "0x8086", "0x2222", "0x030000");
    auto options = options_for(fixture);
    options.maximum_devices = 1;
    const auto result = ph::inventory_linux_pci_devices(options);
    if (result.code != ph::LinuxPciInventoryCode::resource_exhausted ||
        result.complete || result.devices.size() != 1 ||
        !has_issue(result, ph::LinuxPciInventoryIssueCode::device_capacity_exhausted)) {
        return false;
    }

    options = options_for(fixture);
    options.maximum_scanned_entries = 1;
    const auto scan_result = ph::inventory_linux_pci_devices(options);
    return scan_result.code == ph::LinuxPciInventoryCode::resource_exhausted &&
           !scan_result.complete && scan_result.devices.empty() &&
           has_issue(scan_result, ph::LinuxPciInventoryIssueCode::scan_capacity_exhausted);
}

bool test_unknown_vendor() {
    FixtureRoot fixture{};
    if (!fixture.valid()) return false;
    fixture.add_device("0000:09:00.0", "0x1234", "0xabcd", "0x120000");
    const auto result = ph::inventory_linux_pci_devices(options_for(fixture));
    return result.code == ph::LinuxPciInventoryCode::success &&
           result.devices.size() == 1 &&
           result.devices[0].identity.vendor_id == 0x1234U &&
           result.devices[0].vendor_family == ph::PciVendorFamily::other &&
           result.devices[0].device_class ==
               ph::PciDeviceClass::processing_accelerator;
}

bool test_driver_resolution_failure() {
    FixtureRoot fixture{};
    if (!fixture.valid()) return false;
    const fs::path device = fixture.add_device(
        "0000:0a:00.0", "0x10de", "0x1eb1", "0x030200");
    FixtureRoot::write(device / "driver", "not-a-symlink");
    const auto result = ph::inventory_linux_pci_devices(options_for(fixture));
    return result.code == ph::LinuxPciInventoryCode::partial &&
           result.devices.size() == 1 &&
           result.devices[0].driver_state == ph::DriverBindingState::unknown &&
           has_issue(result, ph::LinuxPciInventoryIssueCode::driver_resolution_failed);
}

bool test_issue_bound() {
    FixtureRoot fixture{};
    if (!fixture.valid()) return false;
    fixture.add_device("bad-one", "bad", "bad", "bad");
    fixture.add_device("bad-two", "bad", "bad", "bad");
    fixture.add_device("bad-three", "bad", "bad", "bad");
    auto options = options_for(fixture);
    options.maximum_issues = 1;
    const auto result = ph::inventory_linux_pci_devices(options);
    return result.code == ph::LinuxPciInventoryCode::resource_exhausted &&
           !result.complete && result.issues.size() == 1 &&
           result.dropped_issues == 2;
}

bool test_optional_identity() {
    FixtureRoot fixture{};
    if (!fixture.valid()) return false;
    const fs::path device = fixture.add_device(
        "0000:0b:00.0", "0x8086", "0x1234", "0x030000");
    FixtureRoot::write(device / "subsystem_vendor", "0x1a2b");
    FixtureRoot::write(device / "subsystem_device", "0x3c4d");
    FixtureRoot::write(device / "revision", "0x7f");
    const auto result = ph::inventory_linux_pci_devices(options_for(fixture));
    if (result.code != ph::LinuxPciInventoryCode::success ||
        result.devices.size() != 1) return false;
    const auto& identity = result.devices[0].identity;
    return identity.subsystem_vendor_observed &&
           identity.subsystem_vendor_id == 0x1a2bU &&
           identity.subsystem_device_observed &&
           identity.subsystem_device_id == 0x3c4dU &&
           identity.revision_observed && identity.revision == 0x7fU;
}

bool test_file_and_option_bounds() {
    FixtureRoot fixture{};
    if (!fixture.valid()) return false;
    fixture.add_device(
        "0000:0c:00.0", "0x10de-padding", "0x1234", "0x030000");
    auto options = options_for(fixture);
    options.maximum_file_read_bytes = 8;
    const auto oversized = ph::inventory_linux_pci_devices(options);
    if (oversized.code != ph::LinuxPciInventoryCode::malformed_entry ||
        oversized.complete || !oversized.devices.empty() ||
        !has_issue(
            oversized,
            ph::LinuxPciInventoryIssueCode::attribute_too_large)) {
        return false;
    }

    options = options_for(fixture);
    options.maximum_devices = 0;
    const auto invalid = ph::inventory_linux_pci_devices(options);
    return invalid.code == ph::LinuxPciInventoryCode::invalid_options &&
           !invalid.complete && invalid.devices.empty() &&
           invalid.issues.empty();
}

bool test_deterministic_ordering() {
    FixtureRoot fixture{};
    if (!fixture.valid()) return false;
    fixture.add_device("0000:0f:00.0", "0x10de", "0x0003", "0x030200");
    fixture.add_device("0000:0d:00.0", "0x1002", "0x0001", "0x030000");
    fixture.add_device("0000:0e:00.0", "0x8086", "0x0002", "0x030000");
    const auto first = ph::inventory_linux_pci_devices(options_for(fixture));
    const auto second = ph::inventory_linux_pci_devices(options_for(fixture));
    if (first.code != ph::LinuxPciInventoryCode::success ||
        second.code != ph::LinuxPciInventoryCode::success ||
        first.devices.size() != 3 || second.devices.size() != 3) {
        return false;
    }
    for (std::size_t index = 0; index < first.devices.size(); ++index) {
        if (first.devices[index].address.canonical !=
            second.devices[index].address.canonical) {
            return false;
        }
    }
    return first.devices[0].address.canonical == "0000:0d:00.0" &&
           first.devices[1].address.canonical == "0000:0e:00.0" &&
           first.devices[2].address.canonical == "0000:0f:00.0";
}

}  // namespace

int main() {
    const auto check = [](bool passed, const char* name) {
        if (!passed) std::cerr << "FAILED: " << name << '\n';
        return passed;
    };
    bool passed = true;
    passed &= check(test_address_parser(), "address_parser");
    passed &= check(test_nvidia_unbound(), "nvidia_unbound");
    passed &= check(test_bound_driver("nouveau"), "nouveau_bound");
    passed &= check(test_bound_driver("vfio-pci"), "vfio_bound");
    passed &= check(
        test_vendor(0x1002U, ph::PciVendorFamily::amd, "0x1002"),
        "amd_vendor");
    passed &= check(
        test_vendor(0x8086U, ph::PciVendorFamily::intel, "0x8086"),
        "intel_vendor");
    passed &= check(test_non_gpu_and_empty(), "nvidia_non_gpu");
    passed &= check(test_empty_root(), "empty_root");
    passed &= check(test_unavailable_root(), "unavailable_root");
    passed &= check(test_malformed_entries(), "malformed_entries");
    passed &= check(test_capacity_exhaustion(), "capacity_exhaustion");
    passed &= check(test_unknown_vendor(), "unknown_vendor");
    passed &= check(test_driver_resolution_failure(), "driver_resolution_failure");
    passed &= check(test_issue_bound(), "issue_bound");
    passed &= check(test_optional_identity(), "optional_identity");
    passed &= check(test_file_and_option_bounds(), "file_and_option_bounds");
    passed &= check(test_deterministic_ordering(), "deterministic_ordering");
    if (!passed) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
