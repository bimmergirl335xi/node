#ifndef SRC_BACKENDS_CUDA_CUDA_PROFILES_HPP
#define SRC_BACKENDS_CUDA_CUDA_PROFILES_HPP

#include <cstdint>

namespace prometheus::backends::cuda {

// Hardware architecture classification.  This is intentionally independent
// from CUDA device ordinals, scheduling policy, and kernel availability.
enum class CudaArchitectureFamily : std::uint8_t {
    unknown = 0,
    kepler,
    maxwell,
    pascal,
    volta,
    turing,
    ampere,
    ada,
    hopper,
    blackwell,
};

// Prometheus release-package families.  These describe how a backend binary
// was built, not what role a device should perform in the cognitive mesh.
enum class CudaReleaseFamily : std::uint8_t {
    unknown = 0,
    cuda_12_legacy,
    cuda_13_modern,
};

// Tri-state reporting prevents an unavailable query from being mistaken for
// a negative capability result.
enum class CudaSupportState : std::uint8_t {
    unknown = 0,
    unsupported,
    supported,
};

struct CudaComputeCapability {
    int major = 0;
    int minor = 0;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return major > 0 && minor >= 0;
    }

    [[nodiscard]] constexpr int encoded() const noexcept {
        return valid() ? (major * 10) + minor : 0;
    }
};

struct CudaVersion {
    int encoded = 0;
    int major = 0;
    int minor = 0;
    int patch = 0;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return encoded > 0 && major > 0;
    }
};

// Architecture-derived feature vocabulary.  These fields describe broad
// instruction-family availability only.  Runtime-queryable properties belong
// in CudaDeviceCapabilities.
struct CudaArchitectureProfile {
    CudaComputeCapability compute_capability{};
    CudaArchitectureFamily architecture = CudaArchitectureFamily::unknown;

    CudaSupportState prometheus_supported = CudaSupportState::unknown;
    CudaSupportState cuda_12_release_supported = CudaSupportState::unknown;
    CudaSupportState cuda_13_release_supported = CudaSupportState::unknown;

    CudaSupportState native_fp16_arithmetic = CudaSupportState::unknown;
    CudaSupportState native_bfloat16_arithmetic = CudaSupportState::unknown;
    CudaSupportState tensor_core_operations = CudaSupportState::unknown;
    CudaSupportState tf32_tensor_operations = CudaSupportState::unknown;
    CudaSupportState int8_dot_product = CudaSupportState::unknown;
};

[[nodiscard]] CudaArchitectureFamily classify_cuda_architecture(
    CudaComputeCapability capability) noexcept;

[[nodiscard]] CudaArchitectureProfile make_cuda_architecture_profile(
    CudaComputeCapability capability) noexcept;

[[nodiscard]] CudaVersion decode_cuda_version(int encoded_version) noexcept;

[[nodiscard]] CudaReleaseFamily classify_cuda_release_family(
    int encoded_version) noexcept;

[[nodiscard]] CudaSupportState cuda_release_supports_architecture(
    CudaReleaseFamily release_family,
    CudaArchitectureFamily architecture) noexcept;

[[nodiscard]] const char* to_string(CudaArchitectureFamily value) noexcept;
[[nodiscard]] const char* to_string(CudaReleaseFamily value) noexcept;
[[nodiscard]] const char* to_string(CudaSupportState value) noexcept;

}  // namespace prometheus::backends::cuda

#endif  // SRC_BACKENDS_CUDA_CUDA_PROFILES_HPP
