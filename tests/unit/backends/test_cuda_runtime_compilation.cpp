#include <cstdlib>
#include <string>

#include "cuda_runtime_compilation.hpp"

namespace pc = prometheus::backends::cuda;

namespace {

[[nodiscard]] pc::CudaCompilationRequest valid_request() {
    pc::CudaCompilationRequest request{};
    request.request_id = "compile-request-001";
    request.source_kind = pc::CudaCompilationSourceKind::cuda_cpp;
    request.source =
        "extern \"C\" __global__ void node_test_kernel(float* value) {"
        "  if (threadIdx.x == 0) { *value += 1.0f; }"
        "}";
    request.source_validation = pc::CudaSourceValidationState::validated;
    request.compilation_permission = pc::CudaCompilationPermission::permitted;
    request.target.compute_capability = {7, 0};
    request.target.execution_profile = "sm_70";
    request.compile_options = {"--std=c++17"};
    request.provenance.source_identity = "source:test:trivial-kernel";
    request.provenance.producer_identity = "node-test-suite";
    request.provenance.origin_category = "checked-in-test";
    request.provenance.transformation_lineage = {"test-source-v1"};
    request.release_family = pc::CudaReleaseFamily::cuda_12_legacy;
    request.node_adapter_abi = "node-adapter-abi-v1";
    request.minimum_driver_version = 12000;
    return request;
}

[[nodiscard]] bool test_unavailable_provider() {
    pc::CudaRuntimeCompilationOptions options{};
    options.nvrtc_library_candidates = {
        "libnode_nvrtc_intentionally_unavailable.so"};
    options.cuda_driver_library_candidates = {
        "libnode_cuda_driver_intentionally_unavailable.so"};
    pc::CudaRuntimeCompilationProvider provider{options};
    const pc::CudaRuntimeCompilationSnapshot snapshot = provider.snapshot();
    if (snapshot.state !=
            pc::CudaRuntimeCompilationProviderState::unavailable ||
        snapshot.nvrtc_support != pc::CudaSupportState::unsupported ||
        snapshot.driver_jit_api_support != pc::CudaSupportState::unknown ||
        snapshot.message.empty()) {
        return false;
    }
    return provider.compile(valid_request()).code ==
           pc::CudaCompilationCode::provider_unavailable;
}

[[nodiscard]] bool test_request_bounds_and_authority() {
    pc::CudaRuntimeCompilationProvider provider{};
    pc::CudaCompilationRequest request = valid_request();

    request.source_validation = pc::CudaSourceValidationState::unvalidated;
    if (provider.compile(request).code !=
        pc::CudaCompilationCode::source_not_validated) {
        return false;
    }
    request.source_validation = pc::CudaSourceValidationState::validated;
    request.compilation_permission = pc::CudaCompilationPermission::denied;
    if (provider.compile(request).code !=
        pc::CudaCompilationCode::compilation_not_permitted) {
        return false;
    }
    request.compilation_permission = pc::CudaCompilationPermission::permitted;
    request.source_kind = pc::CudaCompilationSourceKind::ptx;
    if (provider.compile(request).code !=
        pc::CudaCompilationCode::unsupported_source) {
        return false;
    }
    request.source_kind = pc::CudaCompilationSourceKind::cuda_cpp;
    request.compile_options = {"-I/restricted/path"};
    if (provider.compile(request).code !=
        pc::CudaCompilationCode::invalid_request) {
        return false;
    }
    request = valid_request();
    request.source = "#include </restricted/path>\n" + request.source;
    if (provider.compile(request).code !=
        pc::CudaCompilationCode::invalid_request) {
        return false;
    }

    pc::CudaRuntimeCompilationOptions bounded_options{};
    bounded_options.limits.maximum_source_bytes = 64;
    bounded_options.limits.maximum_option_count = 1;
    bounded_options.limits.maximum_option_bytes = 16;
    bounded_options.limits.maximum_total_option_bytes = 16;
    pc::CudaRuntimeCompilationProvider bounded_provider{bounded_options};
    request = valid_request();
    if (bounded_provider.compile(request).code !=
        pc::CudaCompilationCode::invalid_request) {
        return false;
    }
    request.source = "extern \"C\" __global__ void k() {}";
    request.compile_options = {"--std=c++17", "--use_fast_math"};
    return bounded_provider.compile(request).code ==
           pc::CudaCompilationCode::invalid_request;
}

[[nodiscard]] bool test_cache_identity() {
    const pc::CudaCompilationRequest request = valid_request();
    const pc::CudaVersion compiler{12040, 12, 4, 0};
    const std::string first =
        pc::make_cuda_compilation_cache_key(request, compiler);
    const std::string second =
        pc::make_cuda_compilation_cache_key(request, compiler);
    if (first.empty() || first != second) {
        return false;
    }

    pc::CudaCompilationRequest changed_target = request;
    changed_target.target.compute_capability = {8, 0};
    if (pc::make_cuda_compilation_cache_key(changed_target, compiler) == first) {
        return false;
    }
    pc::CudaCompilationRequest changed_option = request;
    changed_option.compile_options.push_back("--use_fast_math");
    if (pc::make_cuda_compilation_cache_key(changed_option, compiler) == first) {
        return false;
    }
    pc::CudaCompilationRequest excessive = request;
    excessive.source.assign(4U * 1024U * 1024U + 1U, 'x');
    return pc::make_cuda_compilation_cache_key(excessive, compiler).empty();
}

[[nodiscard]] bool test_compile_paths() {
    pc::CudaRuntimeCompilationOptions options{};
    options.limits.maximum_log_bytes = 4096;
    pc::CudaRuntimeCompilationProvider provider{options};
    const pc::CudaRuntimeCompilationSnapshot snapshot = provider.snapshot();
    if (snapshot.state ==
        pc::CudaRuntimeCompilationProviderState::unavailable) {
        return true;
    }
    if (snapshot.state != pc::CudaRuntimeCompilationProviderState::ready ||
        snapshot.nvrtc_support != pc::CudaSupportState::supported ||
        !snapshot.nvrtc_version.valid() || snapshot.nvrtc_library.empty() ||
        snapshot.limits.maximum_simultaneous_compilations != 1 ||
        snapshot.limits.maximum_cache_entries != 0 ||
        snapshot.compile_duration_enforced) {
        return false;
    }

    pc::CudaCompilationRequest invalid_source = valid_request();
    invalid_source.source =
        "extern \"C\" __global__ void broken_kernel( {";
    const pc::CudaCompilationResult failure =
        provider.compile(invalid_source);
    if (failure.code != pc::CudaCompilationCode::compilation_failed ||
        failure.message.empty() ||
        failure.compile_log.size() > snapshot.limits.maximum_log_bytes) {
        return false;
    }

    const pc::CudaCompilationResult success =
        provider.compile(valid_request());
    if (!success.ok() || success.artifact.empty() ||
        success.artifact.size() > snapshot.limits.maximum_artifact_bytes ||
        success.metadata.kind != pc::CudaCompilationArtifactKind::ptx ||
        success.metadata.target.compute_capability.encoded() != 70 ||
        success.metadata.target.execution_profile != "sm_70" ||
        success.metadata.artifact_id.empty() ||
        success.metadata.cache_key.empty() ||
        success.metadata.source_digest.empty() ||
        success.metadata.validation !=
            pc::CudaArtifactValidationState::not_validated ||
        success.metadata.registration !=
            pc::CudaArtifactRegistrationState::not_registered ||
        success.metadata.execution_authorization !=
            pc::CudaExecutionAuthorizationState::not_authorized ||
        success.metadata.execution_ready()) {
        return false;
    }

    pc::CudaRuntimeCompilationOptions tiny_artifact_options{};
    tiny_artifact_options.limits.maximum_artifact_bytes = 16;
    pc::CudaRuntimeCompilationProvider tiny_artifact_provider{
        tiny_artifact_options};
    return tiny_artifact_provider.compile(valid_request()).code ==
           pc::CudaCompilationCode::artifact_too_large;
}

}  // namespace

int main() {
    return test_unavailable_provider() &&
                   test_request_bounds_and_authority() &&
                   test_cache_identity() && test_compile_paths()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
