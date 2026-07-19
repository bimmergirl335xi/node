#include "linux_pci_inventory.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <filesystem>
#include <fcntl.h>
#include <limits>
#include <system_error>
#include <type_traits>
#include <unistd.h>
#include <utility>

namespace prometheus::hardware {
namespace {

namespace fs = std::filesystem;

enum class AttributeReadCode : std::uint8_t {
    success = 0,
    missing,
    permission_denied,
    too_large,
    io_failure,
};

struct AttributeReadResult {
    AttributeReadCode code = AttributeReadCode::success;
    std::string value{};
    int native_error_code = 0;
};

bool valid_options(const LinuxPciInventoryOptions& options) noexcept {
    return !options.inventory_root.empty() &&
           options.maximum_scanned_entries > 0 &&
           options.maximum_scanned_entries <= kAbsoluteMaximumPciEntries &&
           options.maximum_devices > 0 &&
           options.maximum_devices <= kAbsoluteMaximumGpuDevices &&
           options.maximum_path_bytes > 0 &&
           options.maximum_path_bytes <= kAbsoluteMaximumInventoryPathBytes &&
           options.inventory_root.size() <= options.maximum_path_bytes &&
           options.maximum_identifier_bytes >= 12 &&
           options.maximum_identifier_bytes <=
               kAbsoluteMaximumInventoryIdentifierBytes &&
           options.maximum_driver_name_bytes > 0 &&
           options.maximum_driver_name_bytes <=
               kAbsoluteMaximumDriverNameBytes &&
           options.maximum_issues > 0 &&
           options.maximum_issues <= kAbsoluteMaximumInventoryIssues &&
           options.maximum_file_read_bytes > 0 &&
           options.maximum_file_read_bytes <=
               kAbsoluteMaximumAttributeBytes;
}

bool permission_error(int value) noexcept {
    return value == EACCES || value == EPERM;
}

bool unavailable_error(int value) noexcept {
    return value == ENOENT || value == ENOTDIR;
}

AttributeReadResult read_attribute(
    const fs::path& path, std::size_t maximum_bytes) {
    AttributeReadResult result{};
    const int descriptor = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (descriptor < 0) {
        result.native_error_code = errno;
        if (unavailable_error(errno)) {
            result.code = AttributeReadCode::missing;
        } else if (permission_error(errno)) {
            result.code = AttributeReadCode::permission_denied;
        } else {
            result.code = AttributeReadCode::io_failure;
        }
        return result;
    }

    std::array<char, kAbsoluteMaximumAttributeBytes + 1> buffer{};
    std::size_t total = 0;
    while (total <= maximum_bytes) {
        const std::size_t remaining = maximum_bytes + 1 - total;
        const ssize_t count = ::read(
            descriptor, buffer.data() + total, remaining);
        if (count > 0) {
            total += static_cast<std::size_t>(count);
            continue;
        }
        if (count == 0) break;
        if (errno == EINTR) continue;
        result.code = AttributeReadCode::io_failure;
        result.native_error_code = errno;
        static_cast<void>(::close(descriptor));
        return result;
    }

    if (::close(descriptor) != 0) {
        result.code = AttributeReadCode::io_failure;
        result.native_error_code = errno;
        return result;
    }
    if (total > maximum_bytes) {
        result.code = AttributeReadCode::too_large;
        return result;
    }

    while (total > 0 &&
           std::isspace(static_cast<unsigned char>(buffer[total - 1])) != 0) {
        --total;
    }
    std::size_t begin = 0;
    while (begin < total &&
           std::isspace(static_cast<unsigned char>(buffer[begin])) != 0) {
        ++begin;
    }
    result.value.assign(buffer.data() + begin, total - begin);
    return result;
}

template <typename T>
bool parse_hex_value(const std::string& text, T maximum, T& output) noexcept {
    static_assert(std::is_unsigned_v<T>);
    if (text.empty()) return false;

    std::size_t index = 0;
    if (text.size() >= 2 && text[0] == '0' &&
        (text[1] == 'x' || text[1] == 'X')) {
        index = 2;
    }
    if (index == text.size()) return false;

    T value = 0;
    for (; index < text.size(); ++index) {
        const unsigned char character =
            static_cast<unsigned char>(text[index]);
        unsigned digit = 0;
        if (character >= '0' && character <= '9') {
            digit = character - '0';
        } else if (character >= 'a' && character <= 'f') {
            digit = character - 'a' + 10U;
        } else if (character >= 'A' && character <= 'F') {
            digit = character - 'A' + 10U;
        } else {
            return false;
        }
        if (digit > maximum) return false;
        if (value > static_cast<T>((maximum - digit) / 16U)) return false;
        value = static_cast<T>((value * 16U) + digit);
    }
    output = value;
    return true;
}

bool hex_digit(char value, unsigned& output) noexcept {
    if (value >= '0' && value <= '9') {
        output = static_cast<unsigned>(value - '0');
        return true;
    }
    if (value >= 'a' && value <= 'f') {
        output = static_cast<unsigned>(value - 'a') + 10U;
        return true;
    }
    if (value >= 'A' && value <= 'F') {
        output = static_cast<unsigned>(value - 'A') + 10U;
        return true;
    }
    return false;
}

bool parse_fixed_hex(
    const std::string& text, std::size_t begin, std::size_t count,
    unsigned maximum, unsigned& output) noexcept {
    unsigned value = 0;
    for (std::size_t index = 0; index < count; ++index) {
        unsigned digit = 0;
        if (!hex_digit(text[begin + index], digit)) return false;
        if (digit > maximum) return false;
        if (value > (maximum - digit) / 16U) return false;
        value = (value * 16U) + digit;
    }
    output = value;
    return true;
}

void retain_issue(
    LinuxPciInventoryResult& result,
    const LinuxPciInventoryOptions& options,
    LinuxPciInventoryIssueCode code,
    const std::string& entry,
    int native_error_code = 0) {
    result.complete = false;
    if (result.issues.size() >= options.maximum_issues) {
        if (result.dropped_issues < std::numeric_limits<std::size_t>::max()) {
            ++result.dropped_issues;
        }
        return;
    }
    LinuxPciInventoryIssue issue{};
    issue.code = code;
    if (entry.size() <= options.maximum_identifier_bytes) {
        issue.entry = entry;
    }
    issue.native_error_code = native_error_code;
    result.issues.push_back(std::move(issue));
}

LinuxPciInventoryIssueCode issue_for_read(AttributeReadCode code) noexcept {
    switch (code) {
        case AttributeReadCode::permission_denied:
            return LinuxPciInventoryIssueCode::attribute_permission_denied;
        case AttributeReadCode::too_large:
            return LinuxPciInventoryIssueCode::attribute_too_large;
        case AttributeReadCode::io_failure:
            return LinuxPciInventoryIssueCode::attribute_io_failure;
        case AttributeReadCode::missing:
        case AttributeReadCode::success:
            return LinuxPciInventoryIssueCode::attribute_unavailable;
    }
    return LinuxPciInventoryIssueCode::attribute_io_failure;
}

template <typename T>
bool read_required_hex(
    const fs::path& directory,
    const char* name,
    T maximum,
    T& output,
    const std::string& entry,
    const LinuxPciInventoryOptions& options,
    LinuxPciInventoryResult& result) {
    const auto read = read_attribute(
        directory / name, options.maximum_file_read_bytes);
    if (read.code != AttributeReadCode::success) {
        retain_issue(
            result, options, issue_for_read(read.code), entry,
            read.native_error_code);
        return false;
    }
    if (!parse_hex_value(read.value, maximum, output)) {
        retain_issue(
            result, options,
            LinuxPciInventoryIssueCode::malformed_attribute, entry);
        return false;
    }
    return true;
}

template <typename T>
void read_optional_hex(
    const fs::path& directory,
    const char* name,
    T maximum,
    T& output,
    bool& observed,
    const std::string& entry,
    const LinuxPciInventoryOptions& options,
    LinuxPciInventoryResult& result) {
    const auto read = read_attribute(
        directory / name, options.maximum_file_read_bytes);
    if (read.code == AttributeReadCode::missing) return;
    if (read.code != AttributeReadCode::success) {
        retain_issue(
            result, options, issue_for_read(read.code), entry,
            read.native_error_code);
        return;
    }
    if (!parse_hex_value(read.value, maximum, output)) {
        retain_issue(
            result, options,
            LinuxPciInventoryIssueCode::malformed_attribute, entry);
        return;
    }
    observed = true;
}

void inspect_driver(
    const fs::path& directory,
    LinuxPciDevice& device,
    const std::string& entry,
    const LinuxPciInventoryOptions& options,
    LinuxPciInventoryResult& result) {
    const fs::path link = directory / "driver";
    std::error_code error{};
    const fs::file_status status = fs::symlink_status(link, error);
    if (error) {
        if (unavailable_error(error.value())) {
            device.driver_state = DriverBindingState::unbound;
            return;
        }
        retain_issue(
            result, options,
            LinuxPciInventoryIssueCode::driver_resolution_failed,
            entry, error.value());
        return;
    }
    if (status.type() == fs::file_type::not_found) {
        device.driver_state = DriverBindingState::unbound;
        return;
    }
    if (!fs::is_symlink(status)) {
        retain_issue(
            result, options,
            LinuxPciInventoryIssueCode::driver_resolution_failed, entry);
        return;
    }

    const fs::path target = fs::read_symlink(link, error);
    if (error || target.empty()) {
        retain_issue(
            result, options,
            LinuxPciInventoryIssueCode::driver_resolution_failed,
            entry, error.value());
        return;
    }
    const std::string target_text = target.string();
    const std::string driver_name = target.filename().string();
    if (target_text.size() > options.maximum_path_bytes ||
        driver_name.empty() ||
        driver_name.size() > options.maximum_driver_name_bytes) {
        retain_issue(
            result, options,
            LinuxPciInventoryIssueCode::driver_name_too_long, entry);
        return;
    }
    device.driver_state = DriverBindingState::bound;
    device.driver_name = driver_name;
}

LinuxPciInventoryResult resource_failure() noexcept {
    LinuxPciInventoryResult result{};
    result.code = LinuxPciInventoryCode::resource_exhausted;
    result.complete = false;
    return result;
}

}  // namespace

PciAddress parse_pci_address(const std::string& value) {
    PciAddress result{};
    if (value.size() != 12 || value[4] != ':' || value[7] != ':' ||
        value[10] != '.') {
        return result;
    }

    unsigned domain = 0;
    unsigned bus = 0;
    unsigned device = 0;
    unsigned function = 0;
    if (!parse_fixed_hex(value, 0, 4, 0xffffU, domain) ||
        !parse_fixed_hex(value, 5, 2, 0xffU, bus) ||
        !parse_fixed_hex(value, 8, 2, 0x1fU, device) ||
        !parse_fixed_hex(value, 11, 1, 0x07U, function)) {
        return result;
    }

    result.domain = static_cast<std::uint16_t>(domain);
    result.bus = static_cast<std::uint8_t>(bus);
    result.device = static_cast<std::uint8_t>(device);
    result.function = static_cast<std::uint8_t>(function);
    result.canonical = value;
    std::transform(
        result.canonical.begin(), result.canonical.end(),
        result.canonical.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    result.valid = true;
    return result;
}

PciDeviceClass classify_pci_device(std::uint32_t class_code) noexcept {
    const std::uint8_t base = static_cast<std::uint8_t>(class_code >> 16U);
    const std::uint8_t subclass =
        static_cast<std::uint8_t>((class_code >> 8U) & 0xffU);
    if (base == 0x03U) {
        if (subclass == 0x00U) {
            return PciDeviceClass::vga_compatible_controller;
        }
        if (subclass == 0x02U) {
            return PciDeviceClass::three_d_controller;
        }
        return PciDeviceClass::display_controller;
    }
    if (base == 0x12U) {
        return PciDeviceClass::processing_accelerator;
    }
    return PciDeviceClass::unknown;
}

PciVendorFamily classify_pci_vendor(std::uint16_t vendor_id) noexcept {
    switch (vendor_id) {
        case 0x10deU: return PciVendorFamily::nvidia;
        case 0x1002U: return PciVendorFamily::amd;
        case 0x8086U: return PciVendorFamily::intel;
        case 0x0000U: return PciVendorFamily::unknown;
        default: return PciVendorFamily::other;
    }
}

bool is_gpu_like_pci_class(PciDeviceClass value) noexcept {
    return value == PciDeviceClass::vga_compatible_controller ||
           value == PciDeviceClass::display_controller ||
           value == PciDeviceClass::three_d_controller ||
           value == PciDeviceClass::processing_accelerator ||
           value == PciDeviceClass::other_relevant_pci_class;
}

LinuxPciInventoryResult inventory_linux_pci_devices(
    const LinuxPciInventoryOptions& options) noexcept {
    try {
        LinuxPciInventoryResult result{};
        if (!valid_options(options)) {
            result.code = LinuxPciInventoryCode::invalid_options;
            result.complete = false;
            return result;
        }

#if !defined(__linux__)
        result.code = LinuxPciInventoryCode::unsupported_platform;
        result.complete = false;
        return result;
#else
        const fs::path root{options.inventory_root};
        std::error_code error{};
        const fs::file_status root_status = fs::status(root, error);
        if (error || !fs::is_directory(root_status)) {
            const int native_error = error.value();
            result.complete = false;
            if (permission_error(native_error)) {
                result.code = LinuxPciInventoryCode::permission_denied;
            } else if (unavailable_error(native_error) ||
                       !fs::exists(root_status)) {
                result.code =
                    LinuxPciInventoryCode::inventory_root_unavailable;
            } else {
                result.code = LinuxPciInventoryCode::io_failure;
            }
            return result;
        }

        std::vector<fs::path> entries{};
        entries.reserve(options.maximum_scanned_entries);
        fs::directory_iterator iterator{root, error};
        const fs::directory_iterator end{};
        if (error) {
            result.code = permission_error(error.value())
                              ? LinuxPciInventoryCode::permission_denied
                              : LinuxPciInventoryCode::io_failure;
            result.complete = false;
            return result;
        }
        while (iterator != end &&
               entries.size() < options.maximum_scanned_entries) {
            entries.push_back(iterator->path());
            iterator.increment(error);
            if (error) {
                result.code = permission_error(error.value())
                                  ? LinuxPciInventoryCode::permission_denied
                                  : LinuxPciInventoryCode::io_failure;
                result.complete = false;
                return result;
            }
        }
        result.scanned_entries = entries.size();
        if (iterator != end) {
            result.code = LinuxPciInventoryCode::resource_exhausted;
            retain_issue(
                result, options,
                LinuxPciInventoryIssueCode::scan_capacity_exhausted,
                options.inventory_root);
            return result;
        }

        std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.filename().string() < rhs.filename().string();
        });
        result.devices.reserve(options.maximum_devices);
        result.issues.reserve(options.maximum_issues);

        bool malformed = false;
        for (const fs::path& directory : entries) {
            const std::string entry = directory.filename().string();
            if (entry.size() > options.maximum_identifier_bytes ||
                directory.string().size() > options.maximum_path_bytes) {
                retain_issue(
                    result, options,
                    LinuxPciInventoryIssueCode::path_too_long, entry);
                malformed = true;
                continue;
            }

            PciAddress address = parse_pci_address(entry);
            if (!address.valid) {
                retain_issue(
                    result, options,
                    LinuxPciInventoryIssueCode::malformed_address, entry);
                malformed = true;
                continue;
            }

            LinuxPciDevice device{};
            device.address = std::move(address);
            bool required_valid = true;
            required_valid &= read_required_hex(
                directory, "vendor", std::uint16_t{0xffffU},
                device.identity.vendor_id, entry, options, result);
            required_valid &= read_required_hex(
                directory, "device", std::uint16_t{0xffffU},
                device.identity.device_id, entry, options, result);
            required_valid &= read_required_hex(
                directory, "class", std::uint32_t{0x00ffffffU},
                device.identity.class_code, entry, options, result);
            if (!required_valid) {
                malformed = true;
                continue;
            }

            device.device_class =
                classify_pci_device(device.identity.class_code);
            if (!is_gpu_like_pci_class(device.device_class)) continue;
            ++result.matching_entries;

            device.vendor_family =
                classify_pci_vendor(device.identity.vendor_id);
            read_optional_hex(
                directory, "subsystem_vendor", std::uint16_t{0xffffU},
                device.identity.subsystem_vendor_id,
                device.identity.subsystem_vendor_observed,
                entry, options, result);
            read_optional_hex(
                directory, "subsystem_device", std::uint16_t{0xffffU},
                device.identity.subsystem_device_id,
                device.identity.subsystem_device_observed,
                entry, options, result);
            read_optional_hex(
                directory, "revision", std::uint8_t{0xffU},
                device.identity.revision,
                device.identity.revision_observed,
                entry, options, result);
            inspect_driver(directory, device, entry, options, result);

            if (result.devices.size() >= options.maximum_devices) {
                retain_issue(
                    result, options,
                    LinuxPciInventoryIssueCode::device_capacity_exhausted,
                    entry);
                result.code = LinuxPciInventoryCode::resource_exhausted;
                return result;
            }
            result.devices.push_back(std::move(device));
        }

        if (result.dropped_issues != 0) {
            result.code = LinuxPciInventoryCode::resource_exhausted;
            result.complete = false;
        } else if (!result.issues.empty()) {
            result.code = malformed && result.devices.empty()
                              ? LinuxPciInventoryCode::malformed_entry
                              : LinuxPciInventoryCode::partial;
            result.complete = false;
        }
        return result;
#endif
    } catch (...) {
        return resource_failure();
    }
}

const char* to_string(PciDeviceClass value) noexcept {
    switch (value) {
        case PciDeviceClass::vga_compatible_controller:
            return "vga_compatible_controller";
        case PciDeviceClass::display_controller: return "display_controller";
        case PciDeviceClass::three_d_controller: return "3d_controller";
        case PciDeviceClass::processing_accelerator:
            return "processing_accelerator";
        case PciDeviceClass::other_relevant_pci_class:
            return "other_relevant_pci_class";
        case PciDeviceClass::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(PciVendorFamily value) noexcept {
    switch (value) {
        case PciVendorFamily::nvidia: return "nvidia";
        case PciVendorFamily::amd: return "amd";
        case PciVendorFamily::intel: return "intel";
        case PciVendorFamily::other: return "other";
        case PciVendorFamily::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(DriverBindingState value) noexcept {
    switch (value) {
        case DriverBindingState::unbound: return "unbound";
        case DriverBindingState::bound: return "bound";
        case DriverBindingState::unknown: return "unknown";
    }
    return "unknown";
}

const char* to_string(LinuxPciInventoryIssueCode value) noexcept {
    switch (value) {
        case LinuxPciInventoryIssueCode::malformed_address:
            return "malformed_address";
        case LinuxPciInventoryIssueCode::path_too_long: return "path_too_long";
        case LinuxPciInventoryIssueCode::attribute_unavailable:
            return "attribute_unavailable";
        case LinuxPciInventoryIssueCode::attribute_permission_denied:
            return "attribute_permission_denied";
        case LinuxPciInventoryIssueCode::attribute_io_failure:
            return "attribute_io_failure";
        case LinuxPciInventoryIssueCode::attribute_too_large:
            return "attribute_too_large";
        case LinuxPciInventoryIssueCode::malformed_attribute:
            return "malformed_attribute";
        case LinuxPciInventoryIssueCode::driver_resolution_failed:
            return "driver_resolution_failed";
        case LinuxPciInventoryIssueCode::driver_name_too_long:
            return "driver_name_too_long";
        case LinuxPciInventoryIssueCode::scan_capacity_exhausted:
            return "scan_capacity_exhausted";
        case LinuxPciInventoryIssueCode::device_capacity_exhausted:
            return "device_capacity_exhausted";
        case LinuxPciInventoryIssueCode::issue_capacity_exhausted:
            return "issue_capacity_exhausted";
    }
    return "unknown";
}

const char* to_string(LinuxPciInventoryCode value) noexcept {
    switch (value) {
        case LinuxPciInventoryCode::success: return "success";
        case LinuxPciInventoryCode::partial: return "partial";
        case LinuxPciInventoryCode::unsupported_platform:
            return "unsupported_platform";
        case LinuxPciInventoryCode::inventory_root_unavailable:
            return "inventory_root_unavailable";
        case LinuxPciInventoryCode::permission_denied:
            return "permission_denied";
        case LinuxPciInventoryCode::malformed_entry: return "malformed_entry";
        case LinuxPciInventoryCode::resource_exhausted:
            return "resource_exhausted";
        case LinuxPciInventoryCode::io_failure: return "io_failure";
        case LinuxPciInventoryCode::invalid_options: return "invalid_options";
    }
    return "unknown";
}

}  // namespace prometheus::hardware
