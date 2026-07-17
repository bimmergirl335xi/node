#ifndef SRC_BACKENDS_CUDA_CUDA_RUNTIME_COMPILATION_HPP
#define SRC_BACKENDS_CUDA_CUDA_RUNTIME_COMPILATION_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "cuda_profiles.hpp"

namespace prometheus::backends::cuda {

enum class CudaRuntimeCompilationProviderState : std::uint8_t {
    unprobed = 0,
    unavailable,
    ready,
    failed,
};

enum class CudaCompilationSourceKind : std::uint8_t {
    unknown = 0,
    cuda_cpp,
    ptx,
};

enum class CudaSourceValidationState : std::uint8_t {
    unvalidated = 0,
    validated,
    rejected,
};

enum class CudaCompilationPermission : std::uint8_t {
    unknown = 0,
    denied,
    permitted,
};

enum class CudaCompilationArtifactKind : std::uint8_t {
    none = 0,
    ptx,
    cubin,
};

enum class CudaArtifactValidationState : std::uint8_t {
    not_validated = 0,
    validated,
    rejected,
};

enum class CudaArtifactRegistrationState : std::uint8_t {
    not_registered = 0,
    registered,
    rejected,
};

enum class CudaExecutionAuthorizationState : std::uint8_t {
    not_authorized = 0,
    authorized,
    denied,
};

struct CudaRuntimeCompilationLimits {
    std::size_t maximum_source_bytes = 256U * 1024U;
    std::size_t maximum_option_count = 32;
    std::size_t maximum_option_bytes = 4096;
    std::size_t maximum_total_option_bytes = 32U * 1024U;
    std::size_t maximum_log_bytes = 64U * 1024U;
    std::size_t maximum_artifact_bytes = 32U * 1024U * 1024U;
    std::size_t maximum_metadata_string_bytes = 256;
    std::size_t maximum_lineage_entries = 16;
    std::size_t maximum_simultaneous_compilations = 1;
    std::size_t maximum_cache_entries = 0;
};

struct CudaRuntimeCompilationOptions {
    CudaRuntimeCompilationLimits limits{};
    std::vector<std::string> nvrtc_library_candidates{
        "libnvrtc.so.12",
        "libnvrtc.so",
    };
    std::vector<std::string> cuda_driver_library_candidates{
        "libcuda.so.1",
        "libcuda.so",
    };
};

struct CudaRuntimeCompilationSnapshot {
    CudaRuntimeCompilationProviderState state =
        CudaRuntimeCompilationProviderState::unprobed;
    CudaSupportState nvrtc_support = CudaSupportState::unknown;
    CudaVersion nvrtc_version{};
    CudaSupportState driver_jit_api_support = CudaSupportState::unknown;
    std::string nvrtc_library{};
    std::string cuda_driver_library{};
    std::string message{};
    CudaRuntimeCompilationLimits limits{};
    bool compilation_active = false;
    bool compile_duration_enforced = false;
};

struct CudaCompilationTarget {
    CudaComputeCapability compute_capability{};
    std::string execution_profile{};
};

struct CudaCompilationProvenance {
    std::string source_identity{};
    std::string producer_identity{};
    std::string origin_category{};
    std::vector<std::string> transformation_lineage{};
};

struct CudaCompilationRequest {
    std::string request_id{};
    CudaCompilationSourceKind source_kind =
        CudaCompilationSourceKind::unknown;
    std::string source{};
    CudaSourceValidationState source_validation =
        CudaSourceValidationState::unvalidated;
    CudaCompilationPermission compilation_permission =
        CudaCompilationPermission::unknown;
    CudaCompilationTarget target{};
    std::vector<std::string> compile_options{};
    CudaCompilationProvenance provenance{};
    CudaReleaseFamily release_family = CudaReleaseFamily::unknown;
    std::string node_adapter_abi{};
    int minimum_driver_version = 0;
};

struct CudaCompilationArtifactMetadata {
    std::string artifact_id{};
    CudaCompilationArtifactKind kind = CudaCompilationArtifactKind::none;
    std::string cache_key{};
    std::string source_digest{};
    CudaCompilationProvenance provenance{};
    CudaSourceValidationState source_validation =
        CudaSourceValidationState::unvalidated;
    CudaCompilationPermission compilation_permission =
        CudaCompilationPermission::unknown;
    CudaCompilationTarget target{};
    CudaVersion compiler_version{};
    std::vector<std::string> compile_options{};
    CudaReleaseFamily release_family = CudaReleaseFamily::unknown;
    std::string node_adapter_abi{};
    int minimum_driver_version = 0;
    CudaArtifactValidationState validation =
        CudaArtifactValidationState::not_validated;
    CudaArtifactRegistrationState registration =
        CudaArtifactRegistrationState::not_registered;
    std::string registration_identity{};
    CudaExecutionAuthorizationState execution_authorization =
        CudaExecutionAuthorizationState::not_authorized;

    [[nodiscard]] bool execution_ready() const noexcept {
        return validation == CudaArtifactValidationState::validated &&
               registration == CudaArtifactRegistrationState::registered &&
               execution_authorization ==
                   CudaExecutionAuthorizationState::authorized;
    }
};

enum class CudaCompilationCode : std::uint8_t {
    success = 0,
    provider_unavailable,
    invalid_request,
    unsupported_source,
    source_not_validated,
    compilation_not_permitted,
    busy,
    compilation_failed,
    artifact_too_large,
    provider_error,
};

struct CudaCompilationResult {
    CudaCompilationCode code = CudaCompilationCode::provider_unavailable;
    int native_result = 0;
    std::string message{};
    std::string compile_log{};
    bool compile_log_truncated = false;
    CudaCompilationArtifactMetadata metadata{};
    std::vector<std::uint8_t> artifact{};

    [[nodiscard]] bool ok() const noexcept {
        return code == CudaCompilationCode::success;
    }
};

// The key describes compatibility inputs only. A hit never grants validation,
// registration, execution authorization, or permission to bypass admission.
[[nodiscard]] std::string make_cuda_compilation_cache_key(
    const CudaCompilationRequest& request,
    CudaVersion compiler_version);

// Dynamically loads NVRTC and observes the Driver API JIT entry point. It can
// compile validated, permitted CUDA C++ to PTX, but exposes no module-loading,
// kernel-launch, promotion, or execution operation.
class CudaRuntimeCompilationProvider {
public:
    explicit CudaRuntimeCompilationProvider(
        CudaRuntimeCompilationOptions options = {});
    ~CudaRuntimeCompilationProvider();

    CudaRuntimeCompilationProvider(
        const CudaRuntimeCompilationProvider&) = delete;
    CudaRuntimeCompilationProvider& operator=(
        const CudaRuntimeCompilationProvider&) = delete;
    CudaRuntimeCompilationProvider(CudaRuntimeCompilationProvider&&) = delete;
    CudaRuntimeCompilationProvider& operator=(
        CudaRuntimeCompilationProvider&&) = delete;

    [[nodiscard]] CudaRuntimeCompilationSnapshot snapshot() const;
    [[nodiscard]] CudaCompilationResult compile(
        const CudaCompilationRequest& request);

private:
    class Impl;
    std::unique_ptr<Impl> impl_{};
};

[[nodiscard]] const char* to_string(
    CudaRuntimeCompilationProviderState value) noexcept;
[[nodiscard]] const char* to_string(CudaCompilationSourceKind value) noexcept;
[[nodiscard]] const char* to_string(CudaSourceValidationState value) noexcept;
[[nodiscard]] const char* to_string(CudaCompilationPermission value) noexcept;
[[nodiscard]] const char* to_string(CudaCompilationArtifactKind value) noexcept;
[[nodiscard]] const char* to_string(CudaArtifactValidationState value) noexcept;
[[nodiscard]] const char* to_string(
    CudaArtifactRegistrationState value) noexcept;
[[nodiscard]] const char* to_string(
    CudaExecutionAuthorizationState value) noexcept;
[[nodiscard]] const char* to_string(CudaCompilationCode value) noexcept;

}  // namespace prometheus::backends::cuda

#endif  // SRC_BACKENDS_CUDA_CUDA_RUNTIME_COMPILATION_HPP
