#include "node_cpu_runtime_assembly.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>

namespace boot = node::boot;

namespace {

constexpr std::string_view kDecisionPath =
    "/run/node-p01-results/cpu-assembly-decision.v1";
constexpr std::string_view kCandidatePath =
    "/node/candidates/cpu-runtime-x86_64-avx2";
constexpr std::string_view kProvenancePath =
    "/node/candidates/cpu-runtime-x86_64-avx2.provenance";
constexpr std::string_view kWorkspacePath = "/run/node-p01-runtime";
constexpr std::string_view kActivePath =
    "/run/node-p01-runtime/cpu-runtime-active";
constexpr std::size_t kDecisionMaximumBytes = 2048;
constexpr std::size_t kProcessOutputMaximumBytes = 512;
constexpr std::uint64_t kExpectedProbeResult = 120;
constexpr std::uint64_t kProbeTimeoutMilliseconds = 2000;

int event_output_fd = STDOUT_FILENO;

struct DecisionRecord {
    std::string node_identity{};
    std::string evaluation{};
    std::string cpu_requirement{};
    std::string gpu_requirement{};
    std::string architecture{};
    std::string profile{};
};

bool write_all(int descriptor, const char* data, std::size_t size) noexcept {
    while (size > 0) {
        const ssize_t count = write(descriptor, data, size);
        if (count > 0) {
            data += count;
            size -= static_cast<std::size_t>(count);
        } else if (count < 0 && errno == EINTR) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

template <typename... Arguments>
void emit(const char* format, Arguments... arguments) noexcept {
    std::array<char, 1024> line{};
    const int length = std::snprintf(line.data(), line.size(), format,
                                     arguments...);
    if (length > 0 && static_cast<std::size_t>(length) < line.size()) {
        (void)write_all(event_output_fd, line.data(),
                        static_cast<std::size_t>(length));
    }
}

bool root_prefix(std::string& root) {
    const char* value = std::getenv("NODE_P01_HOST_ROOT");
    if (value == nullptr || value[0] == '\0') {
        root.clear();
        return true;
    }
    root = value;
    if (root.size() > 256 || root.front() != '/' || root.find("..") != std::string::npos ||
        root.back() == '/') return false;
    return true;
}

std::string rooted(const std::string& root, std::string_view path) {
    return root + std::string{path};
}

bool read_bounded_file(const std::string& path,
                       std::size_t maximum,
                       std::string& output) {
    const int descriptor = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (descriptor < 0) return false;
    struct stat status {};
    if (fstat(descriptor, &status) != 0 || !S_ISREG(status.st_mode) ||
        status.st_size <= 0 ||
        static_cast<std::uint64_t>(status.st_size) > maximum) {
        (void)close(descriptor);
        return false;
    }
    output.assign(static_cast<std::size_t>(status.st_size), '\0');
    std::size_t offset = 0;
    while (offset < output.size()) {
        const ssize_t count = read(descriptor, output.data() + offset,
                                   output.size() - offset);
        if (count > 0) offset += static_cast<std::size_t>(count);
        else if (count < 0 && errno == EINTR) continue;
        else {
            (void)close(descriptor);
            return false;
        }
    }
    return close(descriptor) == 0;
}

bool parse_decision(std::string_view encoded, DecisionRecord& output) {
    if (encoded.empty() || encoded.size() > kDecisionMaximumBytes ||
        encoded.back() != '\n') return false;
    std::array<bool, 8> seen{};
    DecisionRecord parsed{};
    std::size_t offset = 0;
    while (offset < encoded.size()) {
        const std::size_t end = encoded.find('\n', offset);
        if (end == std::string_view::npos || end == offset || end - offset > 256) {
            return false;
        }
        const std::string_view line = encoded.substr(offset, end - offset);
        const std::size_t separator = line.find('=');
        if (separator == std::string_view::npos || separator == 0 ||
            separator + 1 >= line.size()) return false;
        const std::string_view key = line.substr(0, separator);
        const std::string_view value = line.substr(separator + 1);
        std::size_t index = 0;
        if (key == "schema") {
            index = 0;
            if (value != "node.cpu-assembly-decision.v1") return false;
        } else if (key == "node") {
            index = 1;
            parsed.node_identity = value;
        } else if (key == "component_set") {
            index = 2;
            if (value != "p01.cpu-only.first-boot") return false;
        } else if (key == "evaluation") {
            index = 3;
            parsed.evaluation = value;
        } else if (key == "cpu_runtime") {
            index = 4;
            parsed.cpu_requirement = value;
        } else if (key == "gpu_runtime") {
            index = 5;
            parsed.gpu_requirement = value;
        } else if (key == "architecture") {
            index = 6;
            parsed.architecture = value;
        } else if (key == "profile") {
            index = 7;
            parsed.profile = value;
        } else {
            return false;
        }
        if (seen[index] || !boot::bounded_identity(value)) return false;
        seen[index] = true;
        offset = end + 1;
    }
    for (const bool present : seen) {
        if (!present) return false;
    }
    output = std::move(parsed);
    return true;
}

bool ensure_workspace(const std::string& path) {
    if (mkdir(path.c_str(), 0700) != 0 && errno != EEXIST) return false;
    struct stat status {};
    return lstat(path.c_str(), &status) == 0 && S_ISDIR(status.st_mode) &&
           !S_ISLNK(status.st_mode);
}

bool materialize_candidate(const std::string& source,
                           const std::string& destination,
                           std::size_t expected_size) {
    if (expected_size == 0 ||
        expected_size > boot::kCpuRuntimeArtifactMaximumBytes) return false;
    const int input = open(source.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (input < 0) return false;
    struct stat input_status {};
    if (fstat(input, &input_status) != 0 || !S_ISREG(input_status.st_mode) ||
        static_cast<std::size_t>(input_status.st_size) != expected_size) {
        (void)close(input);
        return false;
    }
    struct stat existing {};
    if (lstat(destination.c_str(), &existing) == 0) {
        if (!S_ISREG(existing.st_mode) || unlink(destination.c_str()) != 0) {
            (void)close(input);
            return false;
        }
    } else if (errno != ENOENT) {
        (void)close(input);
        return false;
    }
    const int output = open(destination.c_str(),
                            O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
                            0500);
    if (output < 0) {
        (void)close(input);
        return false;
    }
    std::array<std::uint8_t, 16384> buffer{};
    std::size_t copied = 0;
    bool success = true;
    while (copied < expected_size) {
        const std::size_t request =
            std::min(buffer.size(), expected_size - copied);
        const ssize_t count = read(input, buffer.data(), request);
        if (count > 0) {
            if (!write_all(output,
                           reinterpret_cast<const char*>(buffer.data()),
                           static_cast<std::size_t>(count))) {
                success = false;
                break;
            }
            copied += static_cast<std::size_t>(count);
        } else if (count < 0 && errno == EINTR) {
            continue;
        } else {
            success = false;
            break;
        }
    }
    if (fchmod(output, 0500) != 0) success = false;
    if (close(output) != 0) success = false;
    if (close(input) != 0) success = false;
    if (!success || copied != expected_size) {
        (void)unlink(destination.c_str());
        return false;
    }
    return true;
}

boot::CpuRuntimeProcessEvidence run_probe(const std::string& executable,
                                         std::string& captured) {
    boot::CpuRuntimeProcessEvidence evidence{};
    int output_pipe[2]{};
    if (pipe2(output_pipe, O_CLOEXEC) != 0) return evidence;
    const pid_t child = fork();
    if (child < 0) {
        (void)close(output_pipe[0]);
        (void)close(output_pipe[1]);
        return evidence;
    }
    if (child == 0) {
        (void)close(output_pipe[0]);
        if (dup2(output_pipe[1], STDOUT_FILENO) < 0) _exit(126);
        (void)close(output_pipe[1]);
        char* const arguments[] = {
            const_cast<char*>(executable.c_str()),
            const_cast<char*>("--serve-probe"),
            nullptr,
        };
        char* const environment[] = {
            const_cast<char*>("LANG=C"),
            const_cast<char*>("LC_ALL=C"),
            nullptr,
        };
        execve(executable.c_str(), arguments, environment);
        _exit(127);
    }
    evidence.launched = true;
    (void)close(output_pipe[1]);
    int wait_status = 0;
    bool reaped = false;
    bool wait_failed = false;
    constexpr std::size_t maximum_poll_attempts =
        kProbeTimeoutMilliseconds / 10U;
    for (std::size_t attempt = 0; attempt < maximum_poll_attempts; ++attempt) {
        const pid_t waited = waitpid(child, &wait_status, WNOHANG);
        if (waited == child) {
            reaped = true;
            break;
        }
        if (waited < 0 && errno != EINTR) {
            wait_failed = true;
            break;
        }
        const struct timespec delay {0, 10 * 1000 * 1000};
        (void)nanosleep(&delay, nullptr);
    }
    if (!reaped) {
        evidence.timed_out = !wait_failed;
        (void)kill(child, SIGKILL);
        pid_t waited = -1;
        do {
            waited = waitpid(child, &wait_status, 0);
        } while (waited < 0 && errno == EINTR);
        reaped = waited == child;
    }
    std::array<char, kProcessOutputMaximumBytes + 1> buffer{};
    std::size_t size = 0;
    while (size < kProcessOutputMaximumBytes) {
        const ssize_t count = read(output_pipe[0], buffer.data() + size,
                                   kProcessOutputMaximumBytes - size);
        if (count > 0) size += static_cast<std::size_t>(count);
        else if (count < 0 && errno == EINTR) continue;
        else break;
    }
    (void)close(output_pipe[0]);
    captured.assign(buffer.data(), size);
    evidence.exited_normally = reaped && WIFEXITED(wait_status);
    if (evidence.exited_normally) evidence.exit_code = WEXITSTATUS(wait_status);
    constexpr std::string_view handshake =
        "NODE_CPU_RUNTIME_ABI node.cpu-runtime.conformance.v1\n";
    evidence.interface_observed = captured.find(handshake) != std::string::npos;
    constexpr std::string_view probe_prefix = "NODE_CPU_RUNTIME_PROBE ";
    const std::size_t probe = captured.find(probe_prefix);
    if (probe != std::string::npos) {
        const std::size_t begin = probe + probe_prefix.size();
        const std::size_t end = captured.find('\n', begin);
        if (end != std::string::npos) {
            std::uint64_t value = 0;
            const auto parsed = std::from_chars(
                captured.data() + begin, captured.data() + end, value);
            if (parsed.ec == std::errc{} && parsed.ptr == captured.data() + end) {
                evidence.probe_output_observed = true;
                evidence.probe_result = value;
            }
        }
    }
    return evidence;
}

void emit_phase_result(const std::string& node,
                       const char* outcome,
                       bool compiled,
                       bool validated,
                       bool activated,
                       bool probe_passed,
                       const char* failure) noexcept {
    emit("{\"record\":\"cpu_runtime_phase_result\",\"subject\":\"%s\","
         "\"outcome\":\"%s\",\"failure_category\":\"%s\","
         "\"runtime_compiled\":%s,\"runtime_validated\":%s,"
         "\"runtime_activated\":%s,\"cpu_runtime_probe\":\"%s\","
         "\"global_runtime_ready\":false,"
         "\"detail\":\"bounded_current_boot_conformance_only\"}\n",
         node.c_str(), outcome, failure, compiled ? "true" : "false",
         validated ? "true" : "false", activated ? "true" : "false",
         probe_passed ? "passed" : "not_passed");
}

int run() {
    std::string root{};
    if (!root_prefix(root)) {
        emit_phase_result("unmanaged-node", "failed", false, false, false,
                          false, "invalid_workspace_root");
        return 39;
    }
    std::string encoded_decision{};
    DecisionRecord decision{};
    if (!read_bounded_file(rooted(root, kDecisionPath), kDecisionMaximumBytes,
                           encoded_decision) ||
        !parse_decision(encoded_decision, decision)) {
        emit_phase_result("unmanaged-node", "failed", false, false, false,
                          false, "required_component_missing");
        return 40;
    }

    std::string encoded_provenance{};
    boot::CpuRuntimeProvenance provenance{};
    if (!read_bounded_file(rooted(root, kProvenancePath),
                           boot::kCpuRuntimeProvenanceMaximumBytes,
                           encoded_provenance) ||
        !boot::parse_cpu_runtime_provenance(encoded_provenance, provenance)) {
        emit("{\"record\":\"cpu_runtime_build_requested\","
             "\"subject\":\"%s\",\"component\":\"node.cpu.runtime\","
             "\"profile\":\"%s\",\"outcome\":\"source_missing\","
             "\"source_identity\":\"node.cpu-runtime-conformance-source\","
             "\"source_revision\":\"1\",\"toolchain\":\"unknown\"}\n",
             decision.node_identity.c_str(), decision.profile.c_str());
        emit_phase_result(decision.node_identity, "failed", false, false, false,
                          false, "source_missing");
        return 42;
    }

    boot::CpuRuntimeBuildRequest request{};
    request.node_identity = decision.node_identity;
    request.generation_identity = boot::kCpuRuntimeGenerationIdentity;
    request.component_identity = boot::kCpuRuntimeComponentIdentity;
    request.architecture = decision.architecture;
    request.profile = decision.profile;
    request.source_identity = boot::kCpuRuntimeSourceIdentity;
    request.source_revision = boot::kCpuRuntimeSourceRevision;
    request.toolchain_identity = provenance.toolchain_identity;
    request.configuration_identity = "p01.cpu-runtime.x86_64.avx2.static.v1";
    request.request_identity = "p01.cpu-runtime.build-request.1";
    request.attempt_identity = "p01.cpu-runtime.build-attempt.1";
    request.component_required = decision.cpu_requirement == "required";
    request.profile_compatible = decision.evaluation == "compatible" &&
                                 decision.gpu_requirement == "not_required";
    const boot::CpuRuntimeBuildCode request_code =
        boot::validate_cpu_runtime_build_request(request);
    emit("{\"record\":\"cpu_runtime_build_requested\",\"subject\":\"%s\","
         "\"component\":\"node.cpu.runtime\",\"profile\":\"%s\","
         "\"generation\":\"p01.cpu-runtime.test-generation.1\","
         "\"request\":\"p01.cpu-runtime.build-request.1\","
         "\"attempt\":\"p01.cpu-runtime.build-attempt.1\","
         "\"source_identity\":\"%s\",\"source_revision\":\"%s\","
         "\"toolchain\":\"%s\",\"configuration\":\"%s\","
         "\"outcome\":\"%s\",\"detail\":\"triggered_by_dev003a_decision\"}\n",
         decision.node_identity.c_str(), decision.profile.c_str(),
         request.source_identity.c_str(), request.source_revision.c_str(),
         request.toolchain_identity.c_str(),
         request.configuration_identity.c_str(),
         boot::to_string(request_code));
    if (request_code != boot::CpuRuntimeBuildCode::success) {
        emit_phase_result(decision.node_identity, "failed", false, false, false,
                          false, boot::to_string(request_code));
        return 41;
    }
    const boot::CpuRuntimeBuildCode build_code =
        boot::evaluate_cpu_runtime_build(provenance.build);
    emit("{\"record\":\"cpu_runtime_build_started\",\"subject\":\"%s\","
         "\"profile\":\"avx2\",\"strategy\":\"shared_iso_candidate\","
         "\"compile_location\":\"%s\",\"parallelism\":%zu,"
         "\"toolchain\":\"%s\","
         "\"detail\":\"node_local_volatile_materialization_started\"}\n",
         decision.node_identity.c_str(), provenance.compile_location.c_str(),
         provenance.parallelism, provenance.toolchain_identity.c_str());
    if (build_code != boot::CpuRuntimeBuildCode::success ||
        !ensure_workspace(rooted(root, kWorkspacePath)) ||
        !materialize_candidate(rooted(root, kCandidatePath),
                               rooted(root, kActivePath),
                               provenance.artifact_size)) {
        const char* failure = build_code == boot::CpuRuntimeBuildCode::success
                                  ? "artifact_missing"
                                  : boot::to_string(build_code);
        const bool compiled = build_code == boot::CpuRuntimeBuildCode::success;
        emit("{\"record\":\"cpu_runtime_build_result\",\"subject\":\"%s\","
             "\"outcome\":\"failed\",\"failure_category\":\"%s\","
             "\"runtime_compiled\":%s,\"compiled_this_boot\":false}\n",
             decision.node_identity.c_str(), failure,
             compiled ? "true" : "false");
        emit_phase_result(decision.node_identity, "failed", compiled, false,
                          false, false, failure);
        return 43;
    }
    emit("{\"record\":\"cpu_runtime_build_result\",\"subject\":\"%s\","
         "\"outcome\":\"success\",\"runtime_compiled\":true,"
         "\"compiled_this_boot\":false,"
         "\"detail\":\"shared_candidate_resolved_into_volatile_workspace\"}\n",
         decision.node_identity.c_str());
    emit("{\"record\":\"cpu_runtime_artifact_created\",\"subject\":\"%s\","
         "\"outcome\":\"candidate\",\"sha256\":\"%s\","
         "\"size_bytes\":%zu,\"artifact_identity\":"
         "\"node.cpu.runtime.candidate.sha256.%s\","
         "\"security_verification\":\"not_performed\"}\n",
         decision.node_identity.c_str(), provenance.artifact_sha256.c_str(),
         provenance.artifact_size, provenance.artifact_sha256.c_str());

    const boot::CpuRuntimeArtifactValidation validation =
        boot::validate_cpu_runtime_artifact(rooted(root, kActivePath), provenance,
                                            decision.architecture,
                                            decision.profile);
    emit("{\"record\":\"cpu_runtime_artifact_validation\","
         "\"subject\":\"%s\",\"outcome\":\"%s\","
         "\"architecture\":\"%s\",\"profile\":\"%s\","
         "\"abi\":\"node.cpu-runtime.conformance.v1\","
         "\"entry_point\":\"%s\",\"required_symbol\":\"%s\","
         "\"security_scope\":\"mechanical_not_cryptographic_trust\"}\n",
         decision.node_identity.c_str(), boot::to_string(validation.code),
         decision.architecture.c_str(), decision.profile.c_str(),
         validation.valid() ? "present" : "not_verified",
         validation.valid() ? "node_cpu_runtime_conformance_v1"
                            : "not_verified");
    if (!validation.valid()) {
        emit_phase_result(decision.node_identity, "failed", true, false, false,
                          false, boot::to_string(validation.code));
        return 44;
    }
    emit("{\"record\":\"cpu_runtime_boot_acceptance\","
         "\"subject\":\"%s\",\"outcome\":\"accepted\","
         "\"generation\":\"p01.cpu-runtime.test-generation.1\","
         "\"artifact_identity\":\"node.cpu.runtime.candidate.sha256.%s\","
         "\"scope\":\"bounded_current_boot_conformance\","
         "\"secure_release_claim\":false,\"global_runtime_ready\":false}\n",
         decision.node_identity.c_str(), validation.sha256.c_str());

    emit("{\"record\":\"cpu_runtime_activation_requested\","
         "\"subject\":\"%s\",\"outcome\":\"requested\","
         "\"runtime\":\"node.cpu-runtime.conformance.v1\","
         "\"scope\":\"current_boot_only\"}\n",
         decision.node_identity.c_str());
    std::string process_output{};
    const boot::CpuRuntimeProcessEvidence process =
        run_probe(rooted(root, kActivePath), process_output);
    const boot::CpuRuntimeProcessCode process_code =
        boot::evaluate_cpu_runtime_process(process, kExpectedProbeResult);
    emit("{\"record\":\"cpu_runtime_activation_result\","
         "\"subject\":\"%s\",\"outcome\":\"%s\","
         "\"runtime_activated\":%s,\"interface_observed\":%s,"
         "\"scope\":\"current_boot_only\"}\n",
         decision.node_identity.c_str(),
         process.interface_observed ? "success" : "failed",
         process.interface_observed ? "true" : "false",
         process.interface_observed ? "true" : "false");
    if (process.interface_observed) {
        emit("{\"record\":\"cpu_runtime_probe_started\",\"subject\":\"%s\","
             "\"outcome\":\"started\",\"workload\":"
             "\"avx2_int32_vector_multiply_sum\","
             "\"expected_result\":%llu,\"worker_count\":1,"
             "\"queue_capacity\":1}\n",
             decision.node_identity.c_str(),
             static_cast<unsigned long long>(kExpectedProbeResult));
    }
    emit("{\"record\":\"cpu_runtime_probe_result\",\"subject\":\"%s\","
         "\"outcome\":\"%s\",\"result\":%llu,"
         "\"process_exit\":%d,\"timed_out\":%s}\n",
         decision.node_identity.c_str(),
         process_code == boot::CpuRuntimeProcessCode::success ? "passed" : "failed",
         static_cast<unsigned long long>(process.probe_result), process.exit_code,
         process.timed_out ? "true" : "false");
    if (process_code != boot::CpuRuntimeProcessCode::success) {
        emit_phase_result(decision.node_identity, "failed", true, true,
                          process.interface_observed, false,
                          boot::to_string(process_code));
        return 45;
    }
    emit_phase_result(decision.node_identity, "success", true, true, true, true,
                      "none");
    return 0;
}

}  // namespace

int main() {
    const int serial = open(
        "/dev/ttyS0", O_WRONLY | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (serial >= 0) event_output_fd = serial;
    try {
        const int result = run();
        const char* mode = std::getenv("NODE_MICRO_OS_MODE");
        if (result == 0 && mode != nullptr && std::strcmp(mode, "lab") == 0) {
            std::string root{};
            if (!root_prefix(root)) return 47;
            const std::string executable = rooted(root, kActivePath);
            execl(executable.c_str(), executable.c_str(), "--resident",
                  static_cast<char*>(nullptr));
            emit("{\"record\":\"cpu_runtime_resident\","
                 "\"subject\":\"node.cpu.runtime\","
                 "\"outcome\":\"failed\","
                 "\"detail\":\"resident_exec_failed\"}\n");
            return 47;
        }
        return result;
    } catch (...) {
        emit_phase_result("unmanaged-node", "failed", false, false, false,
                          false, "exception_contained");
        return 46;
    }
}
