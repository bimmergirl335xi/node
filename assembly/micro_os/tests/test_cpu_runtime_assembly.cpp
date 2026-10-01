#include "node_cpu_runtime_assembly.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace boot = node::boot;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "CPU runtime assembly test failed: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::vector<std::uint8_t> read_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

std::string provenance_text(const std::vector<std::uint8_t>& bytes) {
    return
        "schema=node.cpu-runtime-candidate.v1\n"
        "component=node.cpu.runtime\n"
        "abi=node.cpu-runtime.conformance.v1\n"
        "architecture=x86_64\n"
        "profile=avx2\n"
        "source_identity=node.cpu-runtime-conformance-source\n"
        "source_revision=1\n"
        "toolchain_identity=GNU-test\n"
        "configuration=p01.cpu-runtime.x86_64.avx2.static.v1\n"
        "compile_location=host-canonical-image-build\n"
        "artifact_sha256=" + boot::sha256_hex(bytes.data(), bytes.size()) + "\n" +
        "artifact_size=" + std::to_string(bytes.size()) + "\n"
        "parallelism=1\n"
        "source_available=true\n"
        "toolchain_available=true\n"
        "compile_result=success\n"
        "link_result=success\n";
}

std::string temporary_file(const std::vector<std::uint8_t>& bytes) {
    char path[] = "/tmp/node-cpu-runtime-test.XXXXXX";
    const int descriptor = mkstemp(path);
    require(descriptor >= 0, "temporary artifact must be creatable");
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const ssize_t count = write(descriptor, bytes.data() + offset,
                                    bytes.size() - offset);
        require(count > 0, "temporary artifact must be writable");
        offset += static_cast<std::size_t>(count);
    }
    require(close(descriptor) == 0, "temporary artifact must close");
    return path;
}

boot::CpuRuntimeBuildRequest valid_request() {
    boot::CpuRuntimeBuildRequest request{};
    request.node_identity = "node-001";
    request.generation_identity = boot::kCpuRuntimeGenerationIdentity;
    request.component_identity = boot::kCpuRuntimeComponentIdentity;
    request.architecture = boot::kCpuRuntimeArchitecture;
    request.profile = boot::kCpuRuntimeProfile;
    request.source_identity = boot::kCpuRuntimeSourceIdentity;
    request.source_revision = boot::kCpuRuntimeSourceRevision;
    request.toolchain_identity = "GNU-test";
    request.configuration_identity = "p01.cpu-runtime.x86_64.avx2.static.v1";
    request.request_identity = "p01.cpu-runtime.build-request.1";
    request.attempt_identity = "p01.cpu-runtime.build-attempt.1";
    request.component_required = true;
    request.profile_compatible = true;
    return request;
}

}  // namespace

int main(int argument_count, char** arguments) {
    require(argument_count == 2, "candidate artifact argument is required");
    const std::string candidate_path = arguments[1];
    const std::vector<std::uint8_t> candidate = read_file(candidate_path);
    require(!candidate.empty(), "candidate artifact must be readable");
    require(boot::select_cpu_runtime_worker_count(1) == 1,
            "one allowed CPU must preserve the single-worker path");
    require(boot::select_cpu_runtime_worker_count(4) == 4,
            "four allowed CPUs must select four workers");
    require(boot::select_cpu_runtime_worker_count(0) == 0,
            "zero allowed CPUs must fail closed");
    require(boot::select_cpu_runtime_worker_count(
                boot::kCpuRuntimeMaximumWorkers + 1U) == 0,
            "unbounded allowed CPU counts must fail closed");
    require(boot::valid_cpu_runtime_worker_count(4, 4),
            "worker count may match allowed CPU count");
    require(!boot::valid_cpu_runtime_worker_count(2, 3),
            "worker count must not exceed allowed CPU count");
    require(boot::sha256_hex(
                reinterpret_cast<const std::uint8_t*>("abc"), 3) ==
                "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            "SHA-256 implementation must match the standard vector");

    auto request = valid_request();
    require(boot::validate_cpu_runtime_build_request(request) ==
                boot::CpuRuntimeBuildCode::success,
            "valid decision-derived build request must pass");
    request.component_required = false;
    require(boot::validate_cpu_runtime_build_request(request) ==
                boot::CpuRuntimeBuildCode::required_component_missing,
            "missing required-component decision must block build");
    request = valid_request();
    request.profile = "avx512f";
    require(boot::validate_cpu_runtime_build_request(request) ==
                boot::CpuRuntimeBuildCode::invalid_request,
            "unsupported profile must block build");

    boot::CpuRuntimeBuildEvidence build{true, true, true, true};
    require(boot::evaluate_cpu_runtime_build(build) ==
                boot::CpuRuntimeBuildCode::success,
            "complete provider evidence must pass");
    build.source_available = false;
    require(boot::evaluate_cpu_runtime_build(build) ==
                boot::CpuRuntimeBuildCode::source_missing,
            "missing source must be explicit");
    build = {true, false, false, false};
    require(boot::evaluate_cpu_runtime_build(build) ==
                boot::CpuRuntimeBuildCode::toolchain_unavailable,
            "unavailable toolchain must be explicit");
    build = {true, true, false, false};
    require(boot::evaluate_cpu_runtime_build(build) ==
                boot::CpuRuntimeBuildCode::compile_failure,
            "compile failure must be explicit");
    build = {true, true, true, false};
    require(boot::evaluate_cpu_runtime_build(build) ==
                boot::CpuRuntimeBuildCode::link_failure,
            "link failure must be explicit");

    boot::CpuRuntimeProvenance provenance{};
    const std::string encoded = provenance_text(candidate);
    require(boot::parse_cpu_runtime_provenance(encoded, provenance),
            "valid bounded provenance must parse");
    require(!boot::parse_cpu_runtime_provenance(encoded + "extra=value\n", provenance),
            "unknown provenance fields must fail closed");
    require(!boot::parse_cpu_runtime_provenance(encoded.substr(0, encoded.size() - 1),
                                                provenance),
            "unterminated provenance must be malformed");
    require(boot::parse_cpu_runtime_provenance(encoded, provenance),
            "provenance must parse after negative cases");

    const auto valid_artifact = boot::validate_cpu_runtime_artifact(
        candidate_path, provenance, "x86_64", "avx2");
    require(valid_artifact.code == boot::CpuRuntimeArtifactCode::valid,
            "real candidate must pass structural validation");
    require(boot::validate_cpu_runtime_artifact(
                "/tmp/node-cpu-runtime-does-not-exist", provenance,
                "x86_64", "avx2").code ==
                boot::CpuRuntimeArtifactCode::missing,
            "missing artifact must be explicit");
    require(boot::validate_cpu_runtime_artifact(
                candidate_path, provenance, "x86_64", "sse2").code ==
                boot::CpuRuntimeArtifactCode::incompatible,
            "profile incompatibility must be explicit");

    boot::CpuRuntimeProvenance wrong_digest = provenance;
    wrong_digest.artifact_sha256.assign(64, '0');
    require(boot::validate_cpu_runtime_artifact(
                candidate_path, wrong_digest, "x86_64", "avx2").code ==
                boot::CpuRuntimeArtifactCode::digest_mismatch,
            "digest mismatch must be explicit");

    const std::vector<std::uint8_t> malformed{'n', 'o', 't', '-', 'e', 'l', 'f'};
    const std::string malformed_path = temporary_file(malformed);
    boot::CpuRuntimeProvenance malformed_provenance{};
    require(boot::parse_cpu_runtime_provenance(
                provenance_text(malformed), malformed_provenance),
            "malformed-artifact provenance must still be structurally valid");
    require(boot::validate_cpu_runtime_artifact(
                malformed_path, malformed_provenance, "x86_64", "avx2").code ==
                boot::CpuRuntimeArtifactCode::malformed,
            "malformed artifact must be explicit");
    (void)unlink(malformed_path.c_str());

    char oversized_path[] = "/tmp/node-cpu-runtime-oversized.XXXXXX";
    const int oversized_descriptor = mkstemp(oversized_path);
    require(oversized_descriptor >= 0, "oversized artifact must be creatable");
    require(ftruncate(
                oversized_descriptor,
                static_cast<off_t>(boot::kCpuRuntimeArtifactMaximumBytes + 1U)) == 0,
            "oversized artifact must be constructible");
    require(close(oversized_descriptor) == 0,
            "oversized artifact must close");
    boot::CpuRuntimeProvenance oversized_provenance = provenance;
    oversized_provenance.artifact_size =
        boot::kCpuRuntimeArtifactMaximumBytes + 1U;
    require(boot::validate_cpu_runtime_artifact(
                oversized_path, oversized_provenance, "x86_64", "avx2").code ==
                boot::CpuRuntimeArtifactCode::too_large,
            "oversized artifact must be rejected before allocation");
    (void)unlink(oversized_path);

    std::vector<std::uint8_t> missing_symbol = candidate;
    const std::string symbol = "node_cpu_runtime_conformance_v1";
    bool replaced = false;
    for (std::size_t offset = 0; offset + symbol.size() <= missing_symbol.size();
         ++offset) {
        if (std::equal(symbol.begin(), symbol.end(),
                       missing_symbol.begin() + static_cast<std::ptrdiff_t>(offset))) {
            missing_symbol[offset] = 'x';
            replaced = true;
        }
    }
    require(replaced, "candidate must contain its required symbol identity");
    const std::string missing_symbol_path = temporary_file(missing_symbol);
    boot::CpuRuntimeProvenance missing_symbol_provenance{};
    require(boot::parse_cpu_runtime_provenance(
                provenance_text(missing_symbol), missing_symbol_provenance),
            "mutated artifact provenance must parse");
    require(boot::validate_cpu_runtime_artifact(
                missing_symbol_path, missing_symbol_provenance,
                "x86_64", "avx2").code ==
                boot::CpuRuntimeArtifactCode::required_symbol_missing,
            "missing required symbol must be explicit");
    (void)unlink(missing_symbol_path.c_str());

    boot::CpuRuntimeProcessEvidence process{};
    require(boot::evaluate_cpu_runtime_process(process, 120) ==
                boot::CpuRuntimeProcessCode::activation_failure,
            "launch failure must not become activation success");
    process = {true, true, false, false, -1, false, 0};
    require(boot::evaluate_cpu_runtime_process(process, 120) ==
                boot::CpuRuntimeProcessCode::runtime_process_failure,
            "runtime process failure must remain explicit");
    process = {true, true, true, false, -1, false, 0};
    require(boot::evaluate_cpu_runtime_process(process, 120) ==
                boot::CpuRuntimeProcessCode::probe_timeout,
            "probe timeout must remain explicit");
    process = {true, true, false, true, 0, true, 119};
    require(boot::evaluate_cpu_runtime_process(process, 120) ==
                boot::CpuRuntimeProcessCode::probe_incorrect_result,
            "incorrect probe result must fail");
    process.probe_result = 120;
    require(boot::evaluate_cpu_runtime_process(process, 120) ==
                boot::CpuRuntimeProcessCode::success,
            "complete probe evidence must pass");
    return EXIT_SUCCESS;
}
