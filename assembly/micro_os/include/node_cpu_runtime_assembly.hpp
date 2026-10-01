#ifndef NODE_CPU_RUNTIME_ASSEMBLY_HPP
#define NODE_CPU_RUNTIME_ASSEMBLY_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace node::boot {

inline constexpr std::size_t kCpuRuntimeArtifactMaximumBytes = 4U * 1024U * 1024U;
inline constexpr std::size_t kCpuRuntimeProvenanceMaximumBytes = 4096U;
inline constexpr std::string_view kCpuRuntimeComponentIdentity = "node.cpu.runtime";
inline constexpr std::string_view kCpuRuntimeAbiIdentity =
    "node.cpu-runtime.conformance.v1";
inline constexpr std::string_view kCpuRuntimeSourceIdentity =
    "node.cpu-runtime-conformance-source";
inline constexpr std::string_view kCpuRuntimeSourceRevision = "1";
inline constexpr std::string_view kCpuRuntimeArchitecture = "x86_64";
inline constexpr std::string_view kCpuRuntimeProfile = "avx2";
inline constexpr std::string_view kCpuRuntimeGenerationIdentity =
    "p01.cpu-runtime.test-generation.1";

enum class CpuRuntimeBuildCode : std::uint8_t {
    success = 0,
    invalid_request,
    required_component_missing,
    source_missing,
    toolchain_unavailable,
    compile_failure,
    link_failure,
};

enum class CpuRuntimeArtifactCode : std::uint8_t {
    valid = 0,
    missing,
    malformed,
    incompatible,
    too_large,
    digest_mismatch,
    required_symbol_missing,
};

enum class CpuRuntimeProcessCode : std::uint8_t {
    success = 0,
    activation_failure,
    runtime_process_failure,
    probe_timeout,
    probe_incorrect_result,
};

struct CpuRuntimeBuildRequest {
    std::string node_identity{};
    std::string generation_identity{};
    std::string component_identity{};
    std::string architecture{};
    std::string profile{};
    std::string source_identity{};
    std::string source_revision{};
    std::string toolchain_identity{};
    std::string configuration_identity{};
    std::string request_identity{};
    std::string attempt_identity{};
    bool component_required = false;
    bool profile_compatible = false;
};

struct CpuRuntimeBuildEvidence {
    bool source_available = false;
    bool toolchain_available = false;
    bool compile_succeeded = false;
    bool link_succeeded = false;
};

struct CpuRuntimeProvenance {
    std::string component_identity{};
    std::string abi_identity{};
    std::string architecture{};
    std::string profile{};
    std::string source_identity{};
    std::string source_revision{};
    std::string toolchain_identity{};
    std::string configuration_identity{};
    std::string compile_location{};
    std::string artifact_sha256{};
    std::size_t artifact_size = 0;
    std::size_t parallelism = 0;
    CpuRuntimeBuildEvidence build{};
};

struct CpuRuntimeArtifactValidation {
    CpuRuntimeArtifactCode code = CpuRuntimeArtifactCode::missing;
    std::string sha256{};
    std::size_t size_bytes = 0;

    [[nodiscard]] bool valid() const noexcept {
        return code == CpuRuntimeArtifactCode::valid;
    }
};

struct CpuRuntimeProcessEvidence {
    bool launched = false;
    bool interface_observed = false;
    bool timed_out = false;
    bool exited_normally = false;
    int exit_code = -1;
    bool probe_output_observed = false;
    std::uint64_t probe_result = 0;
};

[[nodiscard]] bool bounded_identity(std::string_view value) noexcept;
[[nodiscard]] CpuRuntimeBuildCode validate_cpu_runtime_build_request(
    const CpuRuntimeBuildRequest& request) noexcept;
[[nodiscard]] CpuRuntimeBuildCode evaluate_cpu_runtime_build(
    const CpuRuntimeBuildEvidence& evidence) noexcept;

[[nodiscard]] bool parse_cpu_runtime_provenance(
    std::string_view encoded,
    CpuRuntimeProvenance& output) noexcept;

[[nodiscard]] std::string sha256_hex(
    const std::uint8_t* data,
    std::size_t size);
[[nodiscard]] CpuRuntimeArtifactValidation validate_cpu_runtime_artifact(
    const std::string& path,
    const CpuRuntimeProvenance& provenance,
    std::string_view expected_architecture,
    std::string_view expected_profile) noexcept;

[[nodiscard]] CpuRuntimeProcessCode evaluate_cpu_runtime_process(
    const CpuRuntimeProcessEvidence& evidence,
    std::uint64_t expected_probe_result) noexcept;

[[nodiscard]] const char* to_string(CpuRuntimeBuildCode value) noexcept;
[[nodiscard]] const char* to_string(CpuRuntimeArtifactCode value) noexcept;
[[nodiscard]] const char* to_string(CpuRuntimeProcessCode value) noexcept;

}  // namespace node::boot

#endif
