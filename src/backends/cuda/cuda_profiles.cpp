#include "cuda_profiles.hpp"

namespace prometheus::backends::cuda {
namespace {

[[nodiscard]] constexpr CudaSupportState state_from_bool(bool value) noexcept {
    return value ? CudaSupportState::supported : CudaSupportState::unsupported;
}

[[nodiscard]] constexpr bool at_least(
    CudaComputeCapability capability,
    int required_major,
    int required_minor) noexcept {
    return capability.major > required_major ||
           (capability.major == required_major &&
            capability.minor >= required_minor);
}

}  // namespace

CudaArchitectureFamily classify_cuda_architecture(
    CudaComputeCapability capability) noexcept {
    if (!capability.valid()) {
        return CudaArchitectureFamily::unknown;
    }

    switch (capability.major) {
        case 3:
            return CudaArchitectureFamily::kepler;
        case 5:
            return CudaArchitectureFamily::maxwell;
        case 6:
            return CudaArchitectureFamily::pascal;
        case 7:
            return capability.minor >= 5
                       ? CudaArchitectureFamily::turing
                       : CudaArchitectureFamily::volta;
        case 8:
            return capability.minor >= 9
                       ? CudaArchitectureFamily::ada
                       : CudaArchitectureFamily::ampere;
        case 9:
            return CudaArchitectureFamily::hopper;
        case 10:
        case 12:
            return CudaArchitectureFamily::blackwell;
        default:
            return CudaArchitectureFamily::unknown;
    }
}

CudaArchitectureProfile make_cuda_architecture_profile(
    CudaComputeCapability capability) noexcept {
    CudaArchitectureProfile profile{};
    profile.compute_capability = capability;
    profile.architecture = classify_cuda_architecture(capability);

    if (profile.architecture == CudaArchitectureFamily::unknown) {
        return profile;
    }

    // Project policy for the first CUDA backend release is Pascal and newer.
    profile.prometheus_supported = state_from_bool(capability.major >= 6);

    profile.cuda_12_release_supported = cuda_release_supports_architecture(
        CudaReleaseFamily::cuda_12_legacy,
        profile.architecture);
    profile.cuda_13_release_supported = cuda_release_supports_architecture(
        CudaReleaseFamily::cuda_13_modern,
        profile.architecture);

    profile.native_fp16_arithmetic = state_from_bool(
        at_least(capability, 5, 3));
    profile.tensor_core_operations = state_from_bool(
        at_least(capability, 7, 0));
    profile.native_bfloat16_arithmetic = state_from_bool(
        at_least(capability, 8, 0));
    profile.tf32_tensor_operations = state_from_bool(
        at_least(capability, 8, 0));
    profile.int8_dot_product = state_from_bool(
        at_least(capability, 6, 1));

    return profile;
}

CudaVersion decode_cuda_version(int encoded_version) noexcept {
    CudaVersion version{};
    if (encoded_version <= 0) {
        return version;
    }

    version.encoded = encoded_version;
    version.major = encoded_version / 1000;
    version.minor = (encoded_version % 1000) / 10;
    version.patch = encoded_version % 10;
    return version;
}

CudaReleaseFamily classify_cuda_release_family(int encoded_version) noexcept {
    const CudaVersion version = decode_cuda_version(encoded_version);
    if (!version.valid()) {
        return CudaReleaseFamily::unknown;
    }

    if (version.major == 12) {
        return CudaReleaseFamily::cuda_12_legacy;
    }

    if (version.major >= 13) {
        return CudaReleaseFamily::cuda_13_modern;
    }

    return CudaReleaseFamily::unknown;
}

CudaSupportState cuda_release_supports_architecture(
    CudaReleaseFamily release_family,
    CudaArchitectureFamily architecture) noexcept {
    if (architecture == CudaArchitectureFamily::unknown ||
        release_family == CudaReleaseFamily::unknown) {
        return CudaSupportState::unknown;
    }

    switch (release_family) {
        case CudaReleaseFamily::cuda_12_legacy:
            switch (architecture) {
                case CudaArchitectureFamily::maxwell:
                case CudaArchitectureFamily::pascal:
                case CudaArchitectureFamily::volta:
                case CudaArchitectureFamily::turing:
                case CudaArchitectureFamily::ampere:
                case CudaArchitectureFamily::ada:
                case CudaArchitectureFamily::hopper:
                    return CudaSupportState::supported;
                case CudaArchitectureFamily::blackwell:
                    // Blackwell support depends on the CUDA 12 minor release;
                    // the release family alone is not specific enough.
                    return CudaSupportState::unknown;
                case CudaArchitectureFamily::kepler:
                    return CudaSupportState::unsupported;
                case CudaArchitectureFamily::unknown:
                    return CudaSupportState::unknown;
            }
            break;

        case CudaReleaseFamily::cuda_13_modern:
            switch (architecture) {
                case CudaArchitectureFamily::turing:
                case CudaArchitectureFamily::ampere:
                case CudaArchitectureFamily::ada:
                case CudaArchitectureFamily::hopper:
                case CudaArchitectureFamily::blackwell:
                    return CudaSupportState::supported;
                case CudaArchitectureFamily::kepler:
                case CudaArchitectureFamily::maxwell:
                case CudaArchitectureFamily::pascal:
                case CudaArchitectureFamily::volta:
                    return CudaSupportState::unsupported;
                case CudaArchitectureFamily::unknown:
                    return CudaSupportState::unknown;
            }
            break;

        case CudaReleaseFamily::unknown:
            return CudaSupportState::unknown;
    }

    return CudaSupportState::unknown;
}

const char* to_string(CudaArchitectureFamily value) noexcept {
    switch (value) {
        case CudaArchitectureFamily::kepler:
            return "kepler";
        case CudaArchitectureFamily::maxwell:
            return "maxwell";
        case CudaArchitectureFamily::pascal:
            return "pascal";
        case CudaArchitectureFamily::volta:
            return "volta";
        case CudaArchitectureFamily::turing:
            return "turing";
        case CudaArchitectureFamily::ampere:
            return "ampere";
        case CudaArchitectureFamily::ada:
            return "ada";
        case CudaArchitectureFamily::hopper:
            return "hopper";
        case CudaArchitectureFamily::blackwell:
            return "blackwell";
        case CudaArchitectureFamily::unknown:
            return "unknown";
    }
    return "unknown";
}

const char* to_string(CudaReleaseFamily value) noexcept {
    switch (value) {
        case CudaReleaseFamily::cuda_12_legacy:
            return "cuda_12_legacy";
        case CudaReleaseFamily::cuda_13_modern:
            return "cuda_13_modern";
        case CudaReleaseFamily::unknown:
            return "unknown";
    }
    return "unknown";
}

const char* to_string(CudaSupportState value) noexcept {
    switch (value) {
        case CudaSupportState::unsupported:
            return "unsupported";
        case CudaSupportState::supported:
            return "supported";
        case CudaSupportState::unknown:
            return "unknown";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cuda
