#include "cuda_runtime_compilation.hpp"

#include <dlfcn.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <exception>
#include <iomanip>
#include <limits>
#include <mutex>
#include <new>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <utility>

namespace prometheus::backends::cuda {
namespace {

using NvrtcResult = int;
using NvrtcProgram = void*;

inline constexpr NvrtcResult kNvrtcSuccess = 0;
inline constexpr std::size_t kMaximumLibraryCandidates = 8;
inline constexpr std::size_t kAbsoluteMaximumSourceBytes = 4U * 1024U * 1024U;
inline constexpr std::size_t kAbsoluteMaximumOptionCount = 128;
inline constexpr std::size_t kAbsoluteMaximumOptionBytes = 16U * 1024U;
inline constexpr std::size_t kAbsoluteMaximumTotalOptionBytes =
    256U * 1024U;
inline constexpr std::size_t kAbsoluteMaximumLogBytes = 1024U * 1024U;
inline constexpr std::size_t kAbsoluteMaximumArtifactBytes =
    128U * 1024U * 1024U;
inline constexpr std::size_t kAbsoluteMaximumMetadataBytes = 1024;
inline constexpr std::size_t kAbsoluteMaximumLineageEntries = 64;

using NvrtcVersionFn = NvrtcResult (*)(int*, int*);
using NvrtcCreateProgramFn = NvrtcResult (*)(
    NvrtcProgram*,
    const char*,
    const char*,
    int,
    const char* const*,
    const char* const*);
using NvrtcDestroyProgramFn = NvrtcResult (*)(NvrtcProgram*);
using NvrtcCompileProgramFn = NvrtcResult (*)(
    NvrtcProgram,
    int,
    const char* const*);
using NvrtcGetProgramLogSizeFn = NvrtcResult (*)(NvrtcProgram, std::size_t*);
using NvrtcGetProgramLogFn = NvrtcResult (*)(NvrtcProgram, char*);
using NvrtcGetPtxSizeFn = NvrtcResult (*)(NvrtcProgram, std::size_t*);
using NvrtcGetPtxFn = NvrtcResult (*)(NvrtcProgram, char*);
using NvrtcGetErrorStringFn = const char* (*)(NvrtcResult);
using CuModuleLoadDataExFn = int (*)(
    void**,
    const void*,
    unsigned int,
    void*,
    void*);

template <typename FunctionType>
[[nodiscard]] FunctionType load_function(
    void* library,
    const char* symbol_name) noexcept {
    static_assert(std::is_pointer<FunctionType>::value,
                  "FunctionType must be a function pointer");
    FunctionType function = nullptr;
    void* symbol = dlsym(library, symbol_name);
    static_assert(sizeof(function) == sizeof(symbol),
                  "Function and object pointers must have equal size here");
    std::memcpy(&function, &symbol, sizeof(function));
    return function;
}

[[nodiscard]] std::string bounded_string(
    std::string value,
    std::size_t maximum_bytes) {
    if (value.size() > maximum_bytes) {
        value.resize(maximum_bytes);
    }
    return value;
}

[[nodiscard]] bool contains_nul(std::string_view value) noexcept {
    return value.find('\0') != std::string_view::npos;
}

[[nodiscard]] bool starts_with(std::string_view value,
                               std::string_view prefix) noexcept {
    return value.size() >= prefix.size() &&
           value.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] bool valid_limits(
    const CudaRuntimeCompilationLimits& limits) noexcept {
    return limits.maximum_source_bytes > 0 &&
           limits.maximum_source_bytes <= kAbsoluteMaximumSourceBytes &&
           limits.maximum_option_count > 0 &&
           limits.maximum_option_count <= kAbsoluteMaximumOptionCount &&
           limits.maximum_option_bytes > 0 &&
           limits.maximum_option_bytes <= kAbsoluteMaximumOptionBytes &&
           limits.maximum_total_option_bytes > 0 &&
           limits.maximum_total_option_bytes <=
               kAbsoluteMaximumTotalOptionBytes &&
           limits.maximum_log_bytes > 0 &&
           limits.maximum_log_bytes <= kAbsoluteMaximumLogBytes &&
           limits.maximum_artifact_bytes > 0 &&
           limits.maximum_artifact_bytes <= kAbsoluteMaximumArtifactBytes &&
           limits.maximum_metadata_string_bytes >= 128 &&
           limits.maximum_metadata_string_bytes <=
               kAbsoluteMaximumMetadataBytes &&
           limits.maximum_lineage_entries <=
               kAbsoluteMaximumLineageEntries &&
           limits.maximum_simultaneous_compilations == 1 &&
           limits.maximum_cache_entries == 0;
}

[[nodiscard]] bool valid_library_candidates(
    const std::vector<std::string>& candidates,
    std::size_t maximum_string_bytes) noexcept {
    if (candidates.empty() || candidates.size() > kMaximumLibraryCandidates) {
        return false;
    }
    return std::all_of(
        candidates.begin(),
        candidates.end(),
        [maximum_string_bytes](const std::string& candidate) {
            return !candidate.empty() &&
                   candidate.size() <= maximum_string_bytes &&
                   !contains_nul(candidate);
        });
}

[[nodiscard]] void* open_first_library(
    const std::vector<std::string>& candidates,
    std::string& loaded_name,
    std::string& error,
    std::size_t maximum_message_bytes) {
    for (const std::string& candidate : candidates) {
        dlerror();
        void* library = dlopen(candidate.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (library != nullptr) {
            loaded_name = candidate;
            error.clear();
            return library;
        }
        const char* native_error = dlerror();
        error = native_error == nullptr
                    ? "Dynamic library was not found"
                    : native_error;
    }
    error = bounded_string(std::move(error), maximum_message_bytes);
    return nullptr;
}

class StableHash {
public:
    void add_byte(std::uint8_t value) noexcept {
        value_ ^= value;
        value_ *= 1099511628211ULL;
    }

    void add_u64(std::uint64_t value) noexcept {
        for (unsigned int index = 0; index < 8; ++index) {
            add_byte(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xffU));
        }
    }

    void add_string(std::string_view value) noexcept {
        add_u64(static_cast<std::uint64_t>(value.size()));
        for (const char character : value) {
            add_byte(static_cast<std::uint8_t>(character));
        }
    }

    void add_bytes(const std::vector<std::uint8_t>& bytes) noexcept {
        add_u64(static_cast<std::uint64_t>(bytes.size()));
        for (const std::uint8_t byte : bytes) {
            add_byte(byte);
        }
    }

    [[nodiscard]] std::string hex() const {
        std::ostringstream stream{};
        stream << std::hex << std::setfill('0') << std::setw(16) << value_;
        return stream.str();
    }

private:
    std::uint64_t value_ = 14695981039346656037ULL;
};

void add_request_cache_inputs(StableHash& hash,
                              const CudaCompilationRequest& request,
                              CudaVersion compiler_version) noexcept {
    hash.add_string("node.cuda.runtime-compilation.cache.v1");
    hash.add_u64(static_cast<std::uint64_t>(request.source_kind));
    hash.add_string(request.source);
    hash.add_string(request.provenance.source_identity);
    hash.add_string(request.provenance.producer_identity);
    hash.add_string(request.provenance.origin_category);
    hash.add_u64(request.provenance.transformation_lineage.size());
    for (const std::string& lineage :
         request.provenance.transformation_lineage) {
        hash.add_string(lineage);
    }
    hash.add_u64(static_cast<std::uint64_t>(
        request.target.compute_capability.major));
    hash.add_u64(static_cast<std::uint64_t>(
        request.target.compute_capability.minor));
    hash.add_string(request.target.execution_profile);
    hash.add_u64(request.compile_options.size());
    for (const std::string& option : request.compile_options) {
        hash.add_string(option);
    }
    hash.add_u64(static_cast<std::uint64_t>(compiler_version.major));
    hash.add_u64(static_cast<std::uint64_t>(compiler_version.minor));
    hash.add_u64(static_cast<std::uint64_t>(compiler_version.patch));
    hash.add_u64(static_cast<std::uint64_t>(request.release_family));
    hash.add_string(request.node_adapter_abi);
    hash.add_u64(static_cast<std::uint64_t>(request.minimum_driver_version));
}

[[nodiscard]] std::string source_digest(std::string_view source) {
    StableHash hash{};
    hash.add_string("node.cuda.source.fingerprint.v1");
    hash.add_string(source);
    return "fnv1a64:" + hash.hex();
}

[[nodiscard]] std::string artifact_identity(
    std::string_view cache_key,
    const std::vector<std::uint8_t>& artifact) {
    StableHash hash{};
    hash.add_string("node.cuda.artifact.v1");
    hash.add_string(cache_key);
    hash.add_bytes(artifact);
    return "cuda-artifact-v1:" + hash.hex();
}

[[nodiscard]] bool option_accesses_filesystem_or_overrides_target(
    std::string_view option) noexcept {
    return starts_with(option, "--gpu-architecture") ||
           starts_with(option, "--gpu-code") ||
           starts_with(option, "-arch") || starts_with(option, "-I") ||
           starts_with(option, "--include-path") ||
           starts_with(option, "--pre-include") ||
           starts_with(option, "-include") || starts_with(option, "@");
}

[[nodiscard]] bool source_requests_external_include(
    std::string_view source) noexcept {
    if (source.find("__has_include") != std::string_view::npos) {
        return true;
    }
    std::size_t line_start = 0;
    while (line_start < source.size()) {
        const std::size_t line_end = source.find('\n', line_start);
        std::string_view line = source.substr(
            line_start,
            line_end == std::string_view::npos
                ? std::string_view::npos
                : line_end - line_start);
        while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
            line.remove_prefix(1);
        }
        if (!line.empty() && line.front() == '#') {
            line.remove_prefix(1);
            while (!line.empty() &&
                   (line.front() == ' ' || line.front() == '\t')) {
                line.remove_prefix(1);
            }
            if (starts_with(line, "include") ||
                starts_with(line, "include_next")) {
                return true;
            }
        }
        if (line_end == std::string_view::npos) {
            break;
        }
        line_start = line_end + 1;
    }
    return false;
}

[[nodiscard]] bool cache_inputs_within_absolute_bounds(
    const CudaCompilationRequest& request) noexcept {
    if (request.source.size() > kAbsoluteMaximumSourceBytes ||
        request.compile_options.size() > kAbsoluteMaximumOptionCount ||
        request.provenance.transformation_lineage.size() >
            kAbsoluteMaximumLineageEntries ||
        request.provenance.source_identity.size() >
            kAbsoluteMaximumMetadataBytes ||
        request.provenance.producer_identity.size() >
            kAbsoluteMaximumMetadataBytes ||
        request.provenance.origin_category.size() >
            kAbsoluteMaximumMetadataBytes ||
        request.target.execution_profile.size() >
            kAbsoluteMaximumMetadataBytes ||
        request.node_adapter_abi.size() > kAbsoluteMaximumMetadataBytes) {
        return false;
    }
    std::size_t total_option_bytes = 0;
    for (const std::string& option : request.compile_options) {
        if (option.size() > kAbsoluteMaximumOptionBytes ||
            total_option_bytes > kAbsoluteMaximumTotalOptionBytes -
                                     option.size()) {
            return false;
        }
        total_option_bytes += option.size();
    }
    return std::all_of(
        request.provenance.transformation_lineage.begin(),
        request.provenance.transformation_lineage.end(),
        [](const std::string& value) {
            return value.size() <= kAbsoluteMaximumMetadataBytes;
        });
}

}  // namespace

class CudaRuntimeCompilationProvider::Impl {
public:
    explicit Impl(CudaRuntimeCompilationOptions options)
        : options_(std::move(options)) {
        snapshot_.limits = options_.limits;
        initialize();
    }

    ~Impl() {
        if (driver_library_ != nullptr) {
            dlclose(driver_library_);
        }
        if (nvrtc_library_ != nullptr) {
            dlclose(nvrtc_library_);
        }
    }

    [[nodiscard]] CudaRuntimeCompilationSnapshot snapshot() const {
        CudaRuntimeCompilationSnapshot result = snapshot_;
        result.compilation_active = compilation_active_.load();
        return result;
    }

    [[nodiscard]] CudaCompilationResult compile(
        const CudaCompilationRequest& request) {
        try {
            return compile_internal(request);
        } catch (const std::bad_alloc&) {
            CudaCompilationResult result{};
            result.code = CudaCompilationCode::provider_error;
            result.message = "Runtime compilation exhausted its allocation budget";
            return result;
        } catch (const std::exception&) {
            CudaCompilationResult result{};
            result.code = CudaCompilationCode::provider_error;
            result.message = "Runtime compilation encountered an exception";
            return result;
        } catch (...) {
            CudaCompilationResult result{};
            result.code = CudaCompilationCode::provider_error;
            result.message = "Runtime compilation encountered an unknown exception";
            return result;
        }
    }

    [[nodiscard]] CudaCompilationResult compile_internal(
        const CudaCompilationRequest& request) {
        CudaCompilationResult result{};
        result.metadata.target = request.target;
        result.metadata.provenance = request.provenance;
        result.metadata.source_validation = request.source_validation;
        result.metadata.compilation_permission =
            request.compilation_permission;
        result.metadata.release_family = request.release_family;
        result.metadata.node_adapter_abi = request.node_adapter_abi;
        result.metadata.minimum_driver_version = request.minimum_driver_version;
        result.metadata.compiler_version = snapshot_.nvrtc_version;

        if (!validate_request(request, result)) {
            return result;
        }
        result.metadata.source_digest = source_digest(request.source);
        result.metadata.cache_key = make_cuda_compilation_cache_key(
            request, snapshot_.nvrtc_version);
        result.metadata.compile_options = request.compile_options;

        if (snapshot_.state !=
                CudaRuntimeCompilationProviderState::ready ||
            nvrtc_library_ == nullptr) {
            result.code = CudaCompilationCode::provider_unavailable;
            result.message = bounded_string(
                snapshot_.message,
                options_.limits.maximum_metadata_string_bytes);
            return result;
        }

        std::unique_lock<std::mutex> lock(
            compilation_mutex_, std::try_to_lock);
        if (!lock.owns_lock()) {
            result.code = CudaCompilationCode::busy;
            result.message = "Runtime compilation provider is busy";
            return result;
        }
        compilation_active_.store(true);
        struct ActiveGuard {
            std::atomic_bool& active;
            ~ActiveGuard() { active.store(false); }
        } active_guard{compilation_active_};

        NvrtcProgram program = nullptr;
        NvrtcResult native_result = create_program_(
            &program,
            request.source.c_str(),
            "node_runtime_compilation.cu",
            0,
            nullptr,
            nullptr);
        if (native_result != kNvrtcSuccess || program == nullptr) {
            result.code = CudaCompilationCode::provider_error;
            result.native_result = native_result;
            result.message = error_message("NVRTC program creation failed",
                                           native_result);
            return result;
        }

        struct ProgramGuard {
            NvrtcProgram& program;
            NvrtcDestroyProgramFn destroy;
            ~ProgramGuard() {
                if (program != nullptr && destroy != nullptr) {
                    (void)destroy(&program);
                    program = nullptr;
                }
            }
        } program_guard{program, destroy_program_};

        const auto destroy_program = [&]() {
            if (program == nullptr) {
                return kNvrtcSuccess;
            }
            const NvrtcResult destroy_result = destroy_program_(&program);
            program = nullptr;
            return destroy_result;
        };

        std::vector<std::string> options = request.compile_options;
        options.push_back(
            "--gpu-architecture=compute_" +
            std::to_string(request.target.compute_capability.encoded()));
        std::vector<const char*> option_pointers{};
        option_pointers.reserve(options.size());
        for (const std::string& option : options) {
            option_pointers.push_back(option.c_str());
        }

        native_result = compile_program_(
            program,
            static_cast<int>(option_pointers.size()),
            option_pointers.data());
        result.native_result = native_result;
        capture_log(program, result);
        if (native_result != kNvrtcSuccess) {
            result.code = CudaCompilationCode::compilation_failed;
            result.message = error_message("NVRTC compilation failed",
                                           native_result);
            (void)destroy_program();
            return result;
        }

        std::size_t ptx_size = 0;
        native_result = get_ptx_size_(program, &ptx_size);
        if (native_result != kNvrtcSuccess || ptx_size == 0) {
            result.code = CudaCompilationCode::provider_error;
            result.native_result = native_result;
            result.message = error_message("NVRTC PTX size query failed",
                                           native_result);
            (void)destroy_program();
            return result;
        }
        if (ptx_size > options_.limits.maximum_artifact_bytes) {
            result.code = CudaCompilationCode::artifact_too_large;
            result.message = "Compiled artifact exceeds the configured bound";
            (void)destroy_program();
            return result;
        }

        result.artifact.resize(ptx_size);
        native_result = get_ptx_(
            program, reinterpret_cast<char*>(result.artifact.data()));
        if (native_result != kNvrtcSuccess) {
            result.artifact.clear();
            result.code = CudaCompilationCode::provider_error;
            result.native_result = native_result;
            result.message = error_message("NVRTC PTX retrieval failed",
                                           native_result);
            (void)destroy_program();
            return result;
        }

        const NvrtcResult destroy_result = destroy_program();
        if (destroy_result != kNvrtcSuccess) {
            result.artifact.clear();
            result.code = CudaCompilationCode::provider_error;
            result.native_result = destroy_result;
            result.message = error_message("NVRTC program cleanup failed",
                                           destroy_result);
            return result;
        }

        result.code = CudaCompilationCode::success;
        result.message = "CUDA C++ source compiled to an unvalidated PTX artifact";
        result.metadata.kind = CudaCompilationArtifactKind::ptx;
        result.metadata.artifact_id = artifact_identity(
            result.metadata.cache_key, result.artifact);
        // Compilation deliberately leaves validation, registration, and
        // execution authorization at their conservative defaults.
        return result;
    }

private:
    void initialize() {
        snapshot_.state = CudaRuntimeCompilationProviderState::unprobed;
        snapshot_.nvrtc_support = CudaSupportState::unknown;
        snapshot_.driver_jit_api_support = CudaSupportState::unknown;
        snapshot_.compile_duration_enforced = false;

        if (!valid_limits(options_.limits) ||
            !valid_library_candidates(
                options_.nvrtc_library_candidates,
                options_.limits.maximum_metadata_string_bytes) ||
            !valid_library_candidates(
                options_.cuda_driver_library_candidates,
                options_.limits.maximum_metadata_string_bytes)) {
            snapshot_.state = CudaRuntimeCompilationProviderState::failed;
            snapshot_.nvrtc_support = CudaSupportState::unsupported;
            snapshot_.message =
                "Runtime compilation provider options violate hard bounds";
            return;
        }

        std::string nvrtc_error{};
        nvrtc_library_ = open_first_library(
            options_.nvrtc_library_candidates,
            snapshot_.nvrtc_library,
            nvrtc_error,
            options_.limits.maximum_metadata_string_bytes);
        if (nvrtc_library_ == nullptr) {
            snapshot_.state = CudaRuntimeCompilationProviderState::unavailable;
            snapshot_.nvrtc_support = CudaSupportState::unsupported;
            snapshot_.message = bounded_string(
                "NVRTC shared library is unavailable: " + nvrtc_error,
                options_.limits.maximum_metadata_string_bytes);
            observe_driver_jit();
            return;
        }

        version_ = load_function<NvrtcVersionFn>(
            nvrtc_library_, "nvrtcVersion");
        create_program_ = load_function<NvrtcCreateProgramFn>(
            nvrtc_library_, "nvrtcCreateProgram");
        destroy_program_ = load_function<NvrtcDestroyProgramFn>(
            nvrtc_library_, "nvrtcDestroyProgram");
        compile_program_ = load_function<NvrtcCompileProgramFn>(
            nvrtc_library_, "nvrtcCompileProgram");
        get_program_log_size_ = load_function<NvrtcGetProgramLogSizeFn>(
            nvrtc_library_, "nvrtcGetProgramLogSize");
        get_program_log_ = load_function<NvrtcGetProgramLogFn>(
            nvrtc_library_, "nvrtcGetProgramLog");
        get_ptx_size_ = load_function<NvrtcGetPtxSizeFn>(
            nvrtc_library_, "nvrtcGetPTXSize");
        get_ptx_ = load_function<NvrtcGetPtxFn>(
            nvrtc_library_, "nvrtcGetPTX");
        get_error_string_ = load_function<NvrtcGetErrorStringFn>(
            nvrtc_library_, "nvrtcGetErrorString");

        if (version_ == nullptr || create_program_ == nullptr ||
            destroy_program_ == nullptr || compile_program_ == nullptr ||
            get_program_log_size_ == nullptr ||
            get_program_log_ == nullptr || get_ptx_size_ == nullptr ||
            get_ptx_ == nullptr || get_error_string_ == nullptr) {
            snapshot_.state = CudaRuntimeCompilationProviderState::failed;
            snapshot_.nvrtc_support = CudaSupportState::unsupported;
            snapshot_.message =
                "NVRTC shared library is missing required symbols";
            observe_driver_jit();
            return;
        }

        int major = 0;
        int minor = 0;
        const NvrtcResult version_result = version_(&major, &minor);
        if (version_result != kNvrtcSuccess || major <= 0 || minor < 0) {
            snapshot_.state = CudaRuntimeCompilationProviderState::failed;
            snapshot_.nvrtc_support = CudaSupportState::unsupported;
            snapshot_.message = error_message(
                "NVRTC version observation failed", version_result);
            observe_driver_jit();
            return;
        }

        snapshot_.nvrtc_version = {
            major * 1000 + minor * 10,
            major,
            minor,
            0,
        };
        snapshot_.state = CudaRuntimeCompilationProviderState::ready;
        snapshot_.nvrtc_support = CudaSupportState::supported;
        snapshot_.message = "NVRTC runtime compilation is available";
        observe_driver_jit();
    }

    void observe_driver_jit() {
        std::string driver_error{};
        driver_library_ = open_first_library(
            options_.cuda_driver_library_candidates,
            snapshot_.cuda_driver_library,
            driver_error,
            options_.limits.maximum_metadata_string_bytes);
        if (driver_library_ == nullptr) {
            snapshot_.driver_jit_api_support = CudaSupportState::unknown;
            return;
        }
        const CuModuleLoadDataExFn module_load =
            load_function<CuModuleLoadDataExFn>(
                driver_library_, "cuModuleLoadDataEx");
        snapshot_.driver_jit_api_support =
            module_load == nullptr ? CudaSupportState::unsupported
                                   : CudaSupportState::supported;
    }

    [[nodiscard]] bool validate_request(
        const CudaCompilationRequest& request,
        CudaCompilationResult& result) const {
        const CudaRuntimeCompilationLimits& limits = options_.limits;
        const auto valid_metadata = [&](const std::string& value,
                                        bool required) {
            return (!required || !value.empty()) &&
                   value.size() <= limits.maximum_metadata_string_bytes &&
                   !contains_nul(value);
        };

        if (!valid_metadata(request.request_id, true) ||
            request.source.empty() ||
            request.source.size() > limits.maximum_source_bytes ||
            contains_nul(request.source) ||
            source_requests_external_include(request.source) ||
            !request.target.compute_capability.valid() ||
            request.target.compute_capability.major > 99 ||
            request.target.compute_capability.minor > 9 ||
            !valid_metadata(request.target.execution_profile, true) ||
            !valid_metadata(request.provenance.source_identity, true) ||
            !valid_metadata(request.provenance.producer_identity, true) ||
            !valid_metadata(request.provenance.origin_category, true) ||
            !valid_metadata(request.node_adapter_abi, true) ||
            request.minimum_driver_version < 0 ||
            request.provenance.transformation_lineage.size() >
                limits.maximum_lineage_entries) {
            result.code = CudaCompilationCode::invalid_request;
            result.message =
                "Compilation request metadata or source violates configured bounds";
            return false;
        }
        for (const std::string& lineage :
             request.provenance.transformation_lineage) {
            if (!valid_metadata(lineage, true)) {
                result.code = CudaCompilationCode::invalid_request;
                result.message = "Compilation lineage metadata is invalid";
                return false;
            }
        }
        if (request.source_kind != CudaCompilationSourceKind::cuda_cpp) {
            result.code = CudaCompilationCode::unsupported_source;
            result.message =
                "NVRTC provider accepts validated CUDA C++ source only";
            return false;
        }
        if (request.source_validation != CudaSourceValidationState::validated) {
            result.code = CudaCompilationCode::source_not_validated;
            result.message = "CUDA source has not passed source validation";
            return false;
        }
        if (request.compilation_permission !=
            CudaCompilationPermission::permitted) {
            result.code = CudaCompilationCode::compilation_not_permitted;
            result.message = "Runtime compilation has not been permitted";
            return false;
        }
        if (request.compile_options.size() > limits.maximum_option_count) {
            result.code = CudaCompilationCode::invalid_request;
            result.message = "Compilation option count exceeds its bound";
            return false;
        }

        std::size_t total_option_bytes = 0;
        for (const std::string& option : request.compile_options) {
            if (option.empty() || option.size() > limits.maximum_option_bytes ||
                contains_nul(option) ||
                option_accesses_filesystem_or_overrides_target(option) ||
                total_option_bytes > limits.maximum_total_option_bytes -
                                         option.size()) {
                result.code = CudaCompilationCode::invalid_request;
                result.message =
                    "Compilation option is invalid, unsafe, or over budget";
                return false;
            }
            total_option_bytes += option.size();
        }
        return true;
    }

    void capture_log(NvrtcProgram program,
                     CudaCompilationResult& result) const {
        std::size_t log_size = 0;
        const NvrtcResult size_result =
            get_program_log_size_(program, &log_size);
        if (size_result != kNvrtcSuccess || log_size == 0) {
            return;
        }
        if (log_size > options_.limits.maximum_log_bytes + 1U) {
            result.compile_log_truncated = true;
            result.compile_log = bounded_string(
                "NVRTC compile log exceeded the configured retrieval bound",
                options_.limits.maximum_log_bytes);
            return;
        }

        std::vector<char> buffer(log_size, '\0');
        const NvrtcResult log_result =
            get_program_log_(program, buffer.data());
        if (log_result != kNvrtcSuccess) {
            result.compile_log = bounded_string(
                "NVRTC compile log retrieval failed",
                options_.limits.maximum_log_bytes);
            return;
        }
        std::size_t length = buffer.size();
        while (length > 0 && buffer[length - 1] == '\0') {
            --length;
        }
        result.compile_log.assign(buffer.data(), length);
        if (result.compile_log.size() > options_.limits.maximum_log_bytes) {
            result.compile_log.resize(options_.limits.maximum_log_bytes);
            result.compile_log_truncated = true;
        }
    }

    [[nodiscard]] std::string error_message(
        std::string prefix,
        NvrtcResult result) const {
        if (get_error_string_ != nullptr) {
            const char* native_message = get_error_string_(result);
            if (native_message != nullptr) {
                prefix += ": ";
                prefix += native_message;
            }
        }
        return bounded_string(
            std::move(prefix),
            options_.limits.maximum_metadata_string_bytes);
    }

    CudaRuntimeCompilationOptions options_{};
    CudaRuntimeCompilationSnapshot snapshot_{};
    void* nvrtc_library_ = nullptr;
    void* driver_library_ = nullptr;
    NvrtcVersionFn version_ = nullptr;
    NvrtcCreateProgramFn create_program_ = nullptr;
    NvrtcDestroyProgramFn destroy_program_ = nullptr;
    NvrtcCompileProgramFn compile_program_ = nullptr;
    NvrtcGetProgramLogSizeFn get_program_log_size_ = nullptr;
    NvrtcGetProgramLogFn get_program_log_ = nullptr;
    NvrtcGetPtxSizeFn get_ptx_size_ = nullptr;
    NvrtcGetPtxFn get_ptx_ = nullptr;
    NvrtcGetErrorStringFn get_error_string_ = nullptr;
    mutable std::mutex compilation_mutex_{};
    std::atomic_bool compilation_active_{false};
};

std::string make_cuda_compilation_cache_key(
    const CudaCompilationRequest& request,
    CudaVersion compiler_version) {
    if (!cache_inputs_within_absolute_bounds(request)) {
        return {};
    }
    StableHash hash{};
    add_request_cache_inputs(hash, request, compiler_version);
    return "cuda-cache-v1:" + hash.hex();
}

CudaRuntimeCompilationProvider::CudaRuntimeCompilationProvider(
    CudaRuntimeCompilationOptions options)
    : impl_(std::make_unique<Impl>(std::move(options))) {}

CudaRuntimeCompilationProvider::~CudaRuntimeCompilationProvider() = default;

CudaRuntimeCompilationSnapshot CudaRuntimeCompilationProvider::snapshot()
    const {
    return impl_->snapshot();
}

CudaCompilationResult CudaRuntimeCompilationProvider::compile(
    const CudaCompilationRequest& request) {
    return impl_->compile(request);
}

const char* to_string(CudaRuntimeCompilationProviderState value) noexcept {
    switch (value) {
        case CudaRuntimeCompilationProviderState::unprobed: return "unprobed";
        case CudaRuntimeCompilationProviderState::unavailable: return "unavailable";
        case CudaRuntimeCompilationProviderState::ready: return "ready";
        case CudaRuntimeCompilationProviderState::failed: return "failed";
    }
    return "unknown";
}

const char* to_string(CudaCompilationSourceKind value) noexcept {
    switch (value) {
        case CudaCompilationSourceKind::unknown: return "unknown";
        case CudaCompilationSourceKind::cuda_cpp: return "cuda_cpp";
        case CudaCompilationSourceKind::ptx: return "ptx";
    }
    return "unknown";
}

const char* to_string(CudaSourceValidationState value) noexcept {
    switch (value) {
        case CudaSourceValidationState::unvalidated: return "unvalidated";
        case CudaSourceValidationState::validated: return "validated";
        case CudaSourceValidationState::rejected: return "rejected";
    }
    return "unknown";
}

const char* to_string(CudaCompilationPermission value) noexcept {
    switch (value) {
        case CudaCompilationPermission::unknown: return "unknown";
        case CudaCompilationPermission::denied: return "denied";
        case CudaCompilationPermission::permitted: return "permitted";
    }
    return "unknown";
}

const char* to_string(CudaCompilationArtifactKind value) noexcept {
    switch (value) {
        case CudaCompilationArtifactKind::none: return "none";
        case CudaCompilationArtifactKind::ptx: return "ptx";
        case CudaCompilationArtifactKind::cubin: return "cubin";
    }
    return "unknown";
}

const char* to_string(CudaArtifactValidationState value) noexcept {
    switch (value) {
        case CudaArtifactValidationState::not_validated: return "not_validated";
        case CudaArtifactValidationState::validated: return "validated";
        case CudaArtifactValidationState::rejected: return "rejected";
    }
    return "unknown";
}

const char* to_string(CudaArtifactRegistrationState value) noexcept {
    switch (value) {
        case CudaArtifactRegistrationState::not_registered: return "not_registered";
        case CudaArtifactRegistrationState::registered: return "registered";
        case CudaArtifactRegistrationState::rejected: return "rejected";
    }
    return "unknown";
}

const char* to_string(CudaExecutionAuthorizationState value) noexcept {
    switch (value) {
        case CudaExecutionAuthorizationState::not_authorized: return "not_authorized";
        case CudaExecutionAuthorizationState::authorized: return "authorized";
        case CudaExecutionAuthorizationState::denied: return "denied";
    }
    return "unknown";
}

const char* to_string(CudaCompilationCode value) noexcept {
    switch (value) {
        case CudaCompilationCode::success: return "success";
        case CudaCompilationCode::provider_unavailable: return "provider_unavailable";
        case CudaCompilationCode::invalid_request: return "invalid_request";
        case CudaCompilationCode::unsupported_source: return "unsupported_source";
        case CudaCompilationCode::source_not_validated: return "source_not_validated";
        case CudaCompilationCode::compilation_not_permitted: return "compilation_not_permitted";
        case CudaCompilationCode::busy: return "busy";
        case CudaCompilationCode::compilation_failed: return "compilation_failed";
        case CudaCompilationCode::artifact_too_large: return "artifact_too_large";
        case CudaCompilationCode::provider_error: return "provider_error";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cuda
