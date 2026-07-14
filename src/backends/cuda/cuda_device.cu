#include "cuda_device.hpp"

#include <cuda_runtime_api.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <utility>

namespace prometheus::backends::cuda {
namespace {

[[nodiscard]] std::string make_cuda_error_message(
    cudaError_t error,
    const char* operation) {
    std::string message = operation;
    message += " failed";

    const char* error_name = cudaGetErrorName(error);
    const char* error_text = cudaGetErrorString(error);

    if (error_name != nullptr) {
        message += " (";
        message += error_name;
        message += ")";
    }
    if (error_text != nullptr) {
        message += ": ";
        message += error_text;
    }
    return message;
}

[[nodiscard]] bool uuid_has_nonzero_byte(
    const std::array<std::uint8_t, kCudaUuidByteCount>& bytes) noexcept {
    return std::any_of(bytes.begin(), bytes.end(), [](std::uint8_t value) {
        return value != 0;
    });
}

[[nodiscard]] CudaIdentityQueryResult query_identity_from_valid_ordinal(
    int runtime_ordinal) {
    CudaIdentityQueryResult result{};
    result.identity.runtime_ordinal = runtime_ordinal;

    cudaDeviceProp properties{};
    const cudaError_t property_status =
        cudaGetDeviceProperties(&properties, runtime_ordinal);
    if (property_status != cudaSuccess) {
        result.status.code =
            CudaIdentityQueryCode::device_property_query_failed;
        result.status.native_error_code = static_cast<int>(property_status);
        result.status.message = make_cuda_error_message(
            property_status,
            "cudaGetDeviceProperties");
        return result;
    }

    result.identity.name = properties.name;
    for (std::size_t index = 0; index < kCudaUuidByteCount; ++index) {
        result.identity.uuid_bytes[index] = static_cast<std::uint8_t>(
            static_cast<unsigned char>(properties.uuid.bytes[index]));
    }

    result.identity.uuid_available =
        uuid_has_nonzero_byte(result.identity.uuid_bytes);
    if (result.identity.uuid_available) {
        result.identity.uuid = format_cuda_uuid(result.identity.uuid_bytes);
        result.identity.persistent_key =
            make_cuda_persistent_key(result.identity.uuid);
    } else {
        CudaDeviceIssue issue{};
        issue.code = CudaDeviceIssueCode::uuid_unavailable;
        issue.fatal = true;
        issue.message =
            "CUDA reported an empty device UUID; stable identity is unavailable";
        result.issues.push_back(std::move(issue));
    }

    std::array<char, 32> pci_buffer{};
    const cudaError_t pci_status = cudaDeviceGetPCIBusId(
        pci_buffer.data(),
        static_cast<int>(pci_buffer.size()),
        runtime_ordinal);
    if (pci_status == cudaSuccess) {
        result.identity.pci_bus_id = pci_buffer.data();
        result.identity.pci_bus_id_available =
            !result.identity.pci_bus_id.empty();

        if (result.identity.pci_bus_id_available) {
            result.identity.pci_address =
                parse_cuda_pci_bus_id(result.identity.pci_bus_id);
            if (!result.identity.pci_address.valid) {
                CudaDeviceIssue issue{};
                issue.code = CudaDeviceIssueCode::pci_bus_id_parse_failed;
                issue.message = "CUDA returned an unrecognized PCI bus ID: ";
                issue.message += result.identity.pci_bus_id;
                result.issues.push_back(std::move(issue));
            }
        }
    } else {
        CudaDeviceIssue issue{};
        issue.code = CudaDeviceIssueCode::pci_bus_id_query_failed;
        issue.native_error_code = static_cast<int>(pci_status);
        issue.message = make_cuda_error_message(
            pci_status,
            "cudaDeviceGetPCIBusId");
        result.issues.push_back(std::move(issue));
    }

    if (result.issues.empty()) {
        result.status = {};
    } else {
        result.status.code = CudaIdentityQueryCode::partial_success;
        result.status.message =
            "CUDA device identity was queried with one or more unavailable fields";
    }
    return result;
}

[[nodiscard]] CudaDeviceIssue make_capability_issue(
    const CudaCapabilityQueryStatus& status) {
    CudaDeviceIssue issue{};
    issue.code = CudaDeviceIssueCode::capability_query_failed;
    issue.native_error_code = status.native_error_code;
    issue.fatal = true;
    issue.message = status.message.empty()
                        ? "CUDA capability query failed"
                        : status.message;
    return issue;
}

void mark_duplicate_stable_identities(
    std::vector<CudaDiscoveredDevice>& devices) {
    std::unordered_map<std::string, std::size_t> first_index_by_key;

    for (std::size_t index = 0; index < devices.size(); ++index) {
        CudaDiscoveredDevice& device = devices[index];
        if (!device.has_stable_identity()) {
            continue;
        }

        const auto [iterator, inserted] = first_index_by_key.emplace(
            device.identity.persistent_key,
            index);
        if (inserted) {
            continue;
        }

        CudaDeviceIssue duplicate_issue{};
        duplicate_issue.code =
            CudaDeviceIssueCode::duplicate_stable_identity;
        duplicate_issue.fatal = true;
        duplicate_issue.message =
            "Duplicate CUDA stable identity discovered at runtime ordinals ";
        duplicate_issue.message +=
            std::to_string(devices[iterator->second].identity.runtime_ordinal);
        duplicate_issue.message += " and ";
        duplicate_issue.message +=
            std::to_string(device.identity.runtime_ordinal);
        duplicate_issue.message += ": ";
        duplicate_issue.message += device.identity.persistent_key;

        devices[iterator->second].issues.push_back(duplicate_issue);
        device.issues.push_back(std::move(duplicate_issue));
    }
}

void count_discovery_results(CudaDeviceDiscoveryResult& result) noexcept {
    result.stable_identity_count = 0;
    result.capability_count = 0;
    result.registration_ready_count = 0;

    for (const CudaDiscoveredDevice& device : result.devices) {
        if (device.has_stable_identity()) {
            ++result.stable_identity_count;
        }
        if (device.capabilities_available) {
            ++result.capability_count;
        }
        if (device.ready_for_backend_registration()) {
            ++result.registration_ready_count;
        }
    }
}

}  // namespace

bool CudaDiscoveredDevice::has_fatal_issue() const noexcept {
    return std::any_of(
        issues.begin(),
        issues.end(),
        [](const CudaDeviceIssue& issue) { return issue.fatal; });
}

std::string format_cuda_uuid(
    const std::array<std::uint8_t, kCudaUuidByteCount>& bytes) {
    static constexpr char kHexDigits[] = "0123456789abcdef";

    std::string formatted{};
    formatted.reserve(40);
    formatted += "GPU-";

    for (std::size_t index = 0; index < bytes.size(); ++index) {
        if (index == 4 || index == 6 || index == 8 || index == 10) {
            formatted.push_back('-');
        }

        const std::uint8_t byte = bytes[index];
        formatted.push_back(kHexDigits[(byte >> 4U) & 0x0FU]);
        formatted.push_back(kHexDigits[byte & 0x0FU]);
    }
    return formatted;
}

std::string make_cuda_persistent_key(const std::string& canonical_uuid) {
    if (canonical_uuid.empty()) {
        return {};
    }
    return "cuda:" + canonical_uuid;
}

CudaPciAddress parse_cuda_pci_bus_id(
    const std::string& pci_bus_id) noexcept {
    CudaPciAddress address{};
    if (pci_bus_id.empty()) {
        return address;
    }

    unsigned int domain = 0;
    unsigned int bus = 0;
    unsigned int device = 0;
    unsigned int function = 0;
    char trailing = '\0';

    const int full_fields = std::sscanf(
        pci_bus_id.c_str(),
        "%x:%x:%x.%x%c",
        &domain,
        &bus,
        &device,
        &function,
        &trailing);
    if (full_fields == 4) {
        address.domain = domain;
        address.bus = bus;
        address.device = device;
        address.function = function;
        address.domain_was_present = true;
        address.valid = true;
        return address;
    }

    domain = 0;
    bus = 0;
    device = 0;
    function = 0;
    trailing = '\0';
    const int short_fields = std::sscanf(
        pci_bus_id.c_str(),
        "%x:%x.%x%c",
        &bus,
        &device,
        &function,
        &trailing);
    if (short_fields == 3) {
        address.domain = 0;
        address.bus = bus;
        address.device = device;
        address.function = function;
        address.domain_was_present = false;
        address.valid = true;
    }
    return address;
}

CudaIdentityQueryResult query_cuda_device_identity(int runtime_ordinal) {
    CudaIdentityQueryResult result{};
    result.identity.runtime_ordinal = runtime_ordinal;

    if (runtime_ordinal < 0) {
        result.status.code = CudaIdentityQueryCode::invalid_device_ordinal;
        result.status.message = "CUDA runtime ordinal must be non-negative";
        return result;
    }

    int device_count = 0;
    const cudaError_t count_status = cudaGetDeviceCount(&device_count);
    if (count_status == cudaErrorNoDevice) {
        result.status.code = CudaIdentityQueryCode::invalid_device_ordinal;
        result.status.native_error_code = static_cast<int>(count_status);
        result.status.message = "No CUDA devices are visible to this process";
        return result;
    }
    if (count_status != cudaSuccess) {
        result.status.code = CudaIdentityQueryCode::runtime_query_failed;
        result.status.native_error_code = static_cast<int>(count_status);
        result.status.message = make_cuda_error_message(
            count_status,
            "cudaGetDeviceCount");
        return result;
    }

    if (runtime_ordinal >= device_count) {
        result.status.code = CudaIdentityQueryCode::invalid_device_ordinal;
        result.status.message =
            "CUDA runtime ordinal is outside the discovered range";
        return result;
    }

    return query_identity_from_valid_ordinal(runtime_ordinal);
}

CudaDeviceDiscoveryResult discover_cuda_devices() {
    CudaDeviceDiscoveryResult result{};

    int device_count = 0;
    const cudaError_t count_status = cudaGetDeviceCount(&device_count);
    if (count_status == cudaErrorNoDevice) {
        result.status.code = CudaDiscoveryCode::no_devices;
        result.status.native_error_code = static_cast<int>(count_status);
        result.status.message = "No CUDA devices are visible to this process";
        return result;
    }
    if (count_status != cudaSuccess) {
        result.status.code = CudaDiscoveryCode::runtime_query_failed;
        result.status.native_error_code = static_cast<int>(count_status);
        result.status.message = make_cuda_error_message(
            count_status,
            "cudaGetDeviceCount");
        return result;
    }

    result.reported_device_count = device_count;
    if (device_count == 0) {
        result.status.code = CudaDiscoveryCode::no_devices;
        result.status.message = "No CUDA devices are visible to this process";
        return result;
    }

    result.devices.reserve(static_cast<std::size_t>(device_count));
    bool partial = false;

    for (int runtime_ordinal = 0;
         runtime_ordinal < device_count;
         ++runtime_ordinal) {
        CudaDiscoveredDevice device{};

        CudaIdentityQueryResult identity_result =
            query_identity_from_valid_ordinal(runtime_ordinal);
        device.identity = std::move(identity_result.identity);
        device.identity_status = std::move(identity_result.status);
        device.issues = std::move(identity_result.issues);
        if (!device.identity_status.complete()) {
            partial = true;
        }

        CudaCapabilityQueryResult capability_result =
            query_cuda_device_capabilities(runtime_ordinal);
        if (capability_result.status.ok()) {
            device.capabilities = std::move(capability_result.capabilities);
            device.capabilities_available = true;
        } else {
            device.issues.push_back(
                make_capability_issue(capability_result.status));
            partial = true;
        }

        result.devices.push_back(std::move(device));
    }

    mark_duplicate_stable_identities(result.devices);
    for (const CudaDiscoveredDevice& device : result.devices) {
        if (!device.complete()) {
            partial = true;
            break;
        }
    }

    count_discovery_results(result);

    if (partial) {
        result.status.code = CudaDiscoveryCode::partial_success;
        result.status.message =
            "CUDA discovery completed with one or more partial device records";
    } else {
        result.status = {};
    }
    return result;
}

const char* to_string(CudaDeviceIssueCode value) noexcept {
    switch (value) {
        case CudaDeviceIssueCode::uuid_unavailable:
            return "uuid_unavailable";
        case CudaDeviceIssueCode::pci_bus_id_query_failed:
            return "pci_bus_id_query_failed";
        case CudaDeviceIssueCode::pci_bus_id_parse_failed:
            return "pci_bus_id_parse_failed";
        case CudaDeviceIssueCode::capability_query_failed:
            return "capability_query_failed";
        case CudaDeviceIssueCode::duplicate_stable_identity:
            return "duplicate_stable_identity";
    }
    return "unknown";
}

const char* to_string(CudaIdentityQueryCode value) noexcept {
    switch (value) {
        case CudaIdentityQueryCode::success:
            return "success";
        case CudaIdentityQueryCode::partial_success:
            return "partial_success";
        case CudaIdentityQueryCode::invalid_device_ordinal:
            return "invalid_device_ordinal";
        case CudaIdentityQueryCode::runtime_query_failed:
            return "runtime_query_failed";
        case CudaIdentityQueryCode::device_property_query_failed:
            return "device_property_query_failed";
    }
    return "unknown";
}

const char* to_string(CudaDiscoveryCode value) noexcept {
    switch (value) {
        case CudaDiscoveryCode::success:
            return "success";
        case CudaDiscoveryCode::no_devices:
            return "no_devices";
        case CudaDiscoveryCode::partial_success:
            return "partial_success";
        case CudaDiscoveryCode::runtime_query_failed:
            return "runtime_query_failed";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cuda
