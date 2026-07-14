#include "cuda_kernel_registry.hpp"

#include <algorithm>
#include <utility>

namespace prometheus::backends::cuda {
namespace {

[[nodiscard]] bool capability_less(
    CudaComputeCapability lhs,
    CudaComputeCapability rhs) noexcept {
    return lhs.major < rhs.major ||
           (lhs.major == rhs.major && lhs.minor < rhs.minor);
}

[[nodiscard]] CudaSupportState support_for_release(
    const CudaKernelDescriptor& descriptor,
    CudaReleaseFamily family) noexcept {
    switch (family) {
        case CudaReleaseFamily::cuda_12_legacy:
            return descriptor.cuda_12_release_support;
        case CudaReleaseFamily::cuda_13_modern:
            return descriptor.cuda_13_release_support;
        case CudaReleaseFamily::unknown:
            return CudaSupportState::unknown;
    }
    return CudaSupportState::unknown;
}

[[nodiscard]] CudaKernelCoverageItem evaluate_descriptor(
    const CudaKernelDescriptor& descriptor,
    const CudaDeviceCapabilities& capabilities) {
    CudaKernelCoverageItem item{};
    item.kernel_id = descriptor.kernel_id;
    item.descriptor_found = true;
    item.binary_state = descriptor.binary_state;
    item.binding_state = descriptor.binding_state;

    const CudaComputeCapability device_capability =
        capabilities.architecture.compute_capability;
    if (!device_capability.valid()) {
        item.message = "Device compute capability is unavailable";
        return item;
    }

    if (descriptor.minimum_compute_capability.valid() &&
        capability_less(device_capability,
                        descriptor.minimum_compute_capability)) {
        item.compatibility = CudaKernelCompatibilityState::unsupported;
        item.message = "Device compute capability is below the kernel minimum";
        return item;
    }

    if (descriptor.maximum_compute_capability.valid() &&
        capability_less(descriptor.maximum_compute_capability,
                        device_capability)) {
        item.compatibility = CudaKernelCompatibilityState::unsupported;
        item.message = "Device compute capability is above the kernel maximum";
        return item;
    }

    const CudaSupportState release_support = support_for_release(
        descriptor,
        capabilities.software.compile_time_release_family);
    if (release_support == CudaSupportState::unsupported) {
        item.compatibility = CudaKernelCompatibilityState::unsupported;
        item.message = "Kernel is not supported by this CUDA release family";
        return item;
    }
    if (release_support == CudaSupportState::unknown) {
        item.message = "CUDA release-family support is unresolved";
        return item;
    }

    if (descriptor.binary_state == CudaKernelBinaryState::unavailable) {
        item.compatibility = CudaKernelCompatibilityState::unsupported;
        item.message = "Compatible binary image is explicitly unavailable";
        return item;
    }
    if (descriptor.binary_state == CudaKernelBinaryState::unknown) {
        item.message = "Compatible binary image has not been proven";
        return item;
    }

    if (descriptor.binding_state == CudaKernelBindingState::disabled) {
        item.compatibility = CudaKernelCompatibilityState::unsupported;
        item.message = "Kernel adapter is disabled";
        return item;
    }
    if (descriptor.binding_state !=
        CudaKernelBindingState::adapter_available) {
        item.message = "Kernel metadata exists but no typed adapter is available";
        return item;
    }

    item.compatibility = CudaKernelCompatibilityState::supported;
    return item;
}

}  // namespace

CudaKernelRegistrationResult CudaKernelRegistry::register_kernel(
    CudaKernelDescriptor descriptor) {
    if (descriptor.kernel_id.empty()) {
        return {CudaKernelRegistrationCode::invalid_descriptor,
                "Kernel ID cannot be empty"};
    }
    if (!descriptor.minimum_compute_capability.valid()) {
        return {CudaKernelRegistrationCode::invalid_descriptor,
                "Kernel minimum compute capability must be explicit"};
    }
    if (descriptor.maximum_compute_capability.valid() &&
        capability_less(descriptor.maximum_compute_capability,
                        descriptor.minimum_compute_capability)) {
        return {CudaKernelRegistrationCode::invalid_descriptor,
                "Kernel maximum compute capability is below its minimum"};
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto duplicate = std::find_if(
        descriptors_.begin(),
        descriptors_.end(),
        [&descriptor](const CudaKernelDescriptor& existing) {
            return existing.kernel_id == descriptor.kernel_id;
        });
    if (duplicate != descriptors_.end()) {
        return {CudaKernelRegistrationCode::duplicate_kernel_id,
                "Kernel ID is already registered: " + descriptor.kernel_id};
    }

    descriptors_.push_back(std::move(descriptor));
    return {};
}

CudaKernelRegistrationResult CudaKernelRegistry::remove_kernel(
    const std::string& kernel_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iterator = std::find_if(
        descriptors_.begin(),
        descriptors_.end(),
        [&kernel_id](const CudaKernelDescriptor& descriptor) {
            return descriptor.kernel_id == kernel_id;
        });
    if (iterator == descriptors_.end()) {
        return {CudaKernelRegistrationCode::not_found,
                "Kernel ID was not found: " + kernel_id};
    }

    descriptors_.erase(iterator);
    return {};
}

bool CudaKernelRegistry::contains(const std::string& kernel_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::any_of(
        descriptors_.begin(),
        descriptors_.end(),
        [&kernel_id](const CudaKernelDescriptor& descriptor) {
            return descriptor.kernel_id == kernel_id;
        });
}

std::vector<CudaKernelDescriptor> CudaKernelRegistry::descriptors() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return descriptors_;
}

std::size_t CudaKernelRegistry::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return descriptors_.size();
}

CudaKernelCoverageReport CudaKernelRegistry::evaluate_device_coverage(
    const CudaDeviceCapabilities& capabilities,
    const std::vector<std::string>& required_kernel_ids) const {
    CudaKernelCoverageReport report{};
    report.required_count = required_kernel_ids.size();
    if (required_kernel_ids.empty()) {
        return report;
    }

    const std::vector<CudaKernelDescriptor> snapshot = descriptors();
    report.items.reserve(required_kernel_ids.size());

    for (const std::string& kernel_id : required_kernel_ids) {
        const auto iterator = std::find_if(
            snapshot.begin(),
            snapshot.end(),
            [&kernel_id](const CudaKernelDescriptor& descriptor) {
                return descriptor.kernel_id == kernel_id;
            });

        if (iterator == snapshot.end()) {
            CudaKernelCoverageItem item{};
            item.kernel_id = kernel_id;
            item.message = "Required kernel is not registered";
            item.compatibility = CudaKernelCompatibilityState::unsupported;
            ++report.unavailable_count;
            report.items.push_back(std::move(item));
            continue;
        }

        CudaKernelCoverageItem item = evaluate_descriptor(*iterator, capabilities);
        switch (item.compatibility) {
            case CudaKernelCompatibilityState::supported:
                ++report.supported_count;
                break;
            case CudaKernelCompatibilityState::unsupported:
                ++report.unavailable_count;
                break;
            case CudaKernelCompatibilityState::unknown:
                ++report.unresolved_count;
                break;
        }
        report.items.push_back(std::move(item));
    }

    if (report.supported_count == report.required_count) {
        report.state = CudaKernelCoverageState::complete;
    } else if (report.supported_count > 0) {
        report.state = CudaKernelCoverageState::partial;
    } else if (report.unavailable_count > 0) {
        report.state = CudaKernelCoverageState::unavailable;
    } else {
        report.state = CudaKernelCoverageState::unknown;
    }
    return report;
}

const char* to_string(CudaKernelFamily value) noexcept {
    switch (value) {
        case CudaKernelFamily::activation:
            return "activation";
        case CudaKernelFamily::reduction:
            return "reduction";
        case CudaKernelFamily::dense:
            return "dense";
        case CudaKernelFamily::normalization:
            return "normalization";
        case CudaKernelFamily::temporal:
            return "temporal";
        case CudaKernelFamily::learning:
            return "learning";
        case CudaKernelFamily::optimizer:
            return "optimizer";
        case CudaKernelFamily::candidate_validation:
            return "candidate_validation";
        case CudaKernelFamily::unknown:
            return "unknown";
    }
    return "unknown";
}

const char* to_string(CudaScalarType value) noexcept {
    switch (value) {
        case CudaScalarType::fp16:
            return "fp16";
        case CudaScalarType::fp32:
            return "fp32";
        case CudaScalarType::fp64:
            return "fp64";
        case CudaScalarType::int8:
            return "int8";
        case CudaScalarType::int16:
            return "int16";
        case CudaScalarType::int32:
            return "int32";
        case CudaScalarType::int64:
            return "int64";
        case CudaScalarType::uint8:
            return "uint8";
        case CudaScalarType::uint16:
            return "uint16";
        case CudaScalarType::uint32:
            return "uint32";
        case CudaScalarType::uint64:
            return "uint64";
        case CudaScalarType::boolean:
            return "boolean";
        case CudaScalarType::unknown:
            return "unknown";
    }
    return "unknown";
}

const char* to_string(CudaKernelBindingState value) noexcept {
    switch (value) {
        case CudaKernelBindingState::metadata_only:
            return "metadata_only";
        case CudaKernelBindingState::adapter_available:
            return "adapter_available";
        case CudaKernelBindingState::disabled:
            return "disabled";
    }
    return "unknown";
}

const char* to_string(CudaKernelBinaryState value) noexcept {
    switch (value) {
        case CudaKernelBinaryState::unknown:
            return "unknown";
        case CudaKernelBinaryState::unavailable:
            return "unavailable";
        case CudaKernelBinaryState::available:
            return "available";
    }
    return "unknown";
}

const char* to_string(CudaKernelRegistrationCode value) noexcept {
    switch (value) {
        case CudaKernelRegistrationCode::success:
            return "success";
        case CudaKernelRegistrationCode::invalid_descriptor:
            return "invalid_descriptor";
        case CudaKernelRegistrationCode::duplicate_kernel_id:
            return "duplicate_kernel_id";
        case CudaKernelRegistrationCode::not_found:
            return "not_found";
    }
    return "unknown";
}

const char* to_string(CudaKernelCompatibilityState value) noexcept {
    switch (value) {
        case CudaKernelCompatibilityState::unknown:
            return "unknown";
        case CudaKernelCompatibilityState::unsupported:
            return "unsupported";
        case CudaKernelCompatibilityState::supported:
            return "supported";
    }
    return "unknown";
}

const char* to_string(CudaKernelCoverageState value) noexcept {
    switch (value) {
        case CudaKernelCoverageState::unknown:
            return "unknown";
        case CudaKernelCoverageState::unavailable:
            return "unavailable";
        case CudaKernelCoverageState::partial:
            return "partial";
        case CudaKernelCoverageState::complete:
            return "complete";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cuda
