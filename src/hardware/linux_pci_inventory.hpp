#ifndef SRC_HARDWARE_LINUX_PCI_INVENTORY_HPP
#define SRC_HARDWARE_LINUX_PCI_INVENTORY_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace prometheus::hardware {

inline constexpr std::size_t kDefaultMaximumPciEntries = 4096;
inline constexpr std::size_t kDefaultMaximumGpuDevices = 256;
inline constexpr std::size_t kDefaultMaximumInventoryPathBytes = 4096;
inline constexpr std::size_t kDefaultMaximumInventoryIdentifierBytes = 64;
inline constexpr std::size_t kDefaultMaximumDriverNameBytes = 128;
inline constexpr std::size_t kDefaultMaximumInventoryIssues = 128;
inline constexpr std::size_t kDefaultMaximumAttributeBytes = 64;

inline constexpr std::size_t kAbsoluteMaximumPciEntries = 65536;
inline constexpr std::size_t kAbsoluteMaximumGpuDevices = 4096;
inline constexpr std::size_t kAbsoluteMaximumInventoryPathBytes = 4096;
inline constexpr std::size_t kAbsoluteMaximumInventoryIdentifierBytes = 256;
inline constexpr std::size_t kAbsoluteMaximumDriverNameBytes = 256;
inline constexpr std::size_t kAbsoluteMaximumInventoryIssues = 4096;
inline constexpr std::size_t kAbsoluteMaximumAttributeBytes = 256;

struct LinuxPciInventoryOptions {
    std::string inventory_root = "/sys/bus/pci/devices";
    std::size_t maximum_scanned_entries = kDefaultMaximumPciEntries;
    std::size_t maximum_devices = kDefaultMaximumGpuDevices;
    std::size_t maximum_path_bytes = kDefaultMaximumInventoryPathBytes;
    std::size_t maximum_identifier_bytes =
        kDefaultMaximumInventoryIdentifierBytes;
    std::size_t maximum_driver_name_bytes = kDefaultMaximumDriverNameBytes;
    std::size_t maximum_issues = kDefaultMaximumInventoryIssues;
    std::size_t maximum_file_read_bytes = kDefaultMaximumAttributeBytes;
};

struct PciAddress {
    std::uint16_t domain = 0;
    std::uint8_t bus = 0;
    std::uint8_t device = 0;
    std::uint8_t function = 0;
    std::string canonical{};
    bool valid = false;
};

enum class PciDeviceClass : std::uint8_t {
    unknown = 0,
    vga_compatible_controller,
    display_controller,
    three_d_controller,
    processing_accelerator,
    other_relevant_pci_class,
};

enum class PciVendorFamily : std::uint8_t {
    unknown = 0,
    nvidia,
    amd,
    intel,
    other,
};

enum class DriverBindingState : std::uint8_t {
    unknown = 0,
    unbound,
    bound,
};

struct PciNumericIdentity {
    std::uint16_t vendor_id = 0;
    std::uint16_t device_id = 0;
    std::uint16_t subsystem_vendor_id = 0;
    std::uint16_t subsystem_device_id = 0;
    std::uint32_t class_code = 0;
    std::uint8_t revision = 0;
    bool subsystem_vendor_observed = false;
    bool subsystem_device_observed = false;
    bool revision_observed = false;
};

struct LinuxPciDevice {
    PciAddress address{};
    PciNumericIdentity identity{};
    PciDeviceClass device_class = PciDeviceClass::unknown;
    PciVendorFamily vendor_family = PciVendorFamily::unknown;
    DriverBindingState driver_state = DriverBindingState::unknown;
    std::string driver_name{};
};

enum class LinuxPciInventoryIssueCode : std::uint8_t {
    malformed_address = 0,
    path_too_long,
    attribute_unavailable,
    attribute_permission_denied,
    attribute_io_failure,
    attribute_too_large,
    malformed_attribute,
    driver_resolution_failed,
    driver_name_too_long,
    scan_capacity_exhausted,
    device_capacity_exhausted,
    issue_capacity_exhausted,
};

struct LinuxPciInventoryIssue {
    LinuxPciInventoryIssueCode code =
        LinuxPciInventoryIssueCode::malformed_address;
    std::string entry{};
    int native_error_code = 0;
};

enum class LinuxPciInventoryCode : std::uint8_t {
    success = 0,
    partial,
    unsupported_platform,
    inventory_root_unavailable,
    permission_denied,
    malformed_entry,
    resource_exhausted,
    io_failure,
    invalid_options,
};

struct LinuxPciInventoryResult {
    LinuxPciInventoryCode code = LinuxPciInventoryCode::success;
    bool complete = true;
    std::size_t scanned_entries = 0;
    std::size_t matching_entries = 0;
    std::size_t dropped_issues = 0;
    std::vector<LinuxPciDevice> devices{};
    std::vector<LinuxPciInventoryIssue> issues{};

    [[nodiscard]] bool inventory_available() const noexcept {
        return code == LinuxPciInventoryCode::success ||
               code == LinuxPciInventoryCode::partial ||
               code == LinuxPciInventoryCode::malformed_entry;
    }
};

[[nodiscard]] PciAddress parse_pci_address(const std::string& value);
[[nodiscard]] PciDeviceClass classify_pci_device(std::uint32_t class_code) noexcept;
[[nodiscard]] PciVendorFamily classify_pci_vendor(std::uint16_t vendor_id) noexcept;
[[nodiscard]] bool is_gpu_like_pci_class(PciDeviceClass value) noexcept;

// Reads immediate PCI sysfs entries only. It does not load a driver, query a
// compute runtime, allocate device memory, establish readiness, or invoke an
// external process.
[[nodiscard]] LinuxPciInventoryResult inventory_linux_pci_devices(
    const LinuxPciInventoryOptions& options = {}) noexcept;

[[nodiscard]] const char* to_string(PciDeviceClass value) noexcept;
[[nodiscard]] const char* to_string(PciVendorFamily value) noexcept;
[[nodiscard]] const char* to_string(DriverBindingState value) noexcept;
[[nodiscard]] const char* to_string(LinuxPciInventoryIssueCode value) noexcept;
[[nodiscard]] const char* to_string(LinuxPciInventoryCode value) noexcept;

}  // namespace prometheus::hardware

#endif  // SRC_HARDWARE_LINUX_PCI_INVENTORY_HPP
