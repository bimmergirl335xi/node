#ifndef SRC_BACKENDS_ACCELERATOR_ACCELERATOR_BACKEND_HPP
#define SRC_BACKENDS_ACCELERATOR_ACCELERATOR_BACKEND_HPP

#include <cstdint>
#include <string>

namespace prometheus::backends::accelerator {

enum class AcceleratorVendor : std::uint8_t {
    unknown = 0,
    hailo,
    sony,
};

struct AcceleratorDeviceIdentity {
    AcceleratorVendor vendor = AcceleratorVendor::unknown;
    std::string device_id{};
    std::string device_path{};
    std::string product_name{};
};

[[nodiscard]] inline const char* to_string(AcceleratorVendor value) noexcept {
    switch (value) {
        case AcceleratorVendor::hailo: return "hailo";
        case AcceleratorVendor::sony: return "sony";
        case AcceleratorVendor::unknown: return "unknown";
    }
    return "unknown";
}

}  // namespace prometheus::backends::accelerator

#endif  // SRC_BACKENDS_ACCELERATOR_ACCELERATOR_BACKEND_HPP
