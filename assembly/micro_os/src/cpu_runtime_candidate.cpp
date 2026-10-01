#include <immintrin.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>

#include "cpu_thread_pool.hpp"

namespace cpu = prometheus::backends::cpu;

extern "C" const char* node_cpu_runtime_conformance_v1() noexcept;

namespace {

constexpr char kAbiIdentity[] = "node.cpu-runtime.conformance.v1";
constexpr std::uint64_t kExpectedProbeResult = 120;
constexpr char kReadyPath[] =
    "/run/node-p01-results/cpu_runtime_assembly.ready";
constexpr char kHealthyPath[] =
    "/run/node-p01-results/cpu_runtime_assembly.healthy";

int event_output_fd = STDOUT_FILENO;

bool write_all(const char* value, std::size_t size) noexcept {
    while (size > 0) {
        const ssize_t count = write(event_output_fd, value, size);
        if (count > 0) {
            value += count;
            size -= static_cast<std::size_t>(count);
        } else if (count < 0 && errno == EINTR) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

bool root_prefix(std::string& root) {
    const char* value = std::getenv("NODE_P01_HOST_ROOT");
    if (value == nullptr || value[0] == '\0') return true;
    root = value;
    return root.size() <= 256 && root.front() == '/' && root.back() != '/' &&
           root.find("..") == std::string::npos;
}

bool write_marker(const std::string& path, const char* value) noexcept {
    struct stat existing {};
    if (lstat(path.c_str(), &existing) == 0) {
        if (!S_ISREG(existing.st_mode) || unlink(path.c_str()) != 0) return false;
    } else if (errno != ENOENT) {
        return false;
    }
    const int descriptor = open(
        path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
        0600);
    if (descriptor < 0) return false;
    const std::size_t size = std::strlen(value);
    std::size_t offset = 0;
    while (offset < size) {
        const ssize_t count = write(descriptor, value + offset, size - offset);
        if (count > 0) offset += static_cast<std::size_t>(count);
        else if (count < 0 && errno == EINTR) continue;
        else break;
    }
    const int closed = close(descriptor);
    const bool success = offset == size && closed == 0;
    if (!success) (void)unlink(path.c_str());
    return success;
}

std::uint64_t avx2_probe() noexcept {
    alignas(32) constexpr std::array<std::int32_t, 8> left{
        1, 2, 3, 4, 5, 6, 7, 8,
    };
    alignas(32) constexpr std::array<std::int32_t, 8> right{
        8, 7, 6, 5, 4, 3, 2, 1,
    };
    alignas(32) std::array<std::int32_t, 8> products{};
    const __m256i first =
        _mm256_load_si256(reinterpret_cast<const __m256i*>(left.data()));
    const __m256i second =
        _mm256_load_si256(reinterpret_cast<const __m256i*>(right.data()));
    const __m256i multiplied = _mm256_mullo_epi32(first, second);
    _mm256_store_si256(reinterpret_cast<__m256i*>(products.data()), multiplied);
    std::uint64_t total = 0;
    for (const std::int32_t value : products) {
        total += static_cast<std::uint64_t>(value);
    }
    return total;
}

bool execute_probe(cpu::CpuThreadPool& pool, std::uint64_t& observed) {
    std::atomic<std::uint64_t> result{0};
    const cpu::CpuTaskSubmissionResult submission =
        pool.submit([&result]() { result.store(avx2_probe()); });
    if (!submission.accepted() ||
        !submission.handle.wait_for(std::chrono::seconds{1}) ||
        !submission.handle.result().succeeded()) return false;
    observed = result.load();
    return observed == kExpectedProbeResult;
}

bool stop_pool(cpu::CpuThreadPool& pool) {
    return pool.request_shutdown(cpu::CpuThreadPoolStopMode::drain) ==
               cpu::CpuThreadPoolShutdownResult::request_accepted &&
           pool.wait_for_shutdown(std::chrono::seconds{1}) ==
               cpu::CpuThreadPoolShutdownResult::fully_stopped;
}

int run_one_shot_probe() {
    constexpr char handshake[] =
        "NODE_CPU_RUNTIME_ABI node.cpu-runtime.conformance.v1\n";
    if (std::strcmp(node_cpu_runtime_conformance_v1(), kAbiIdentity) != 0 ||
        !write_all(handshake, sizeof(handshake) - 1U)) return 65;

    cpu::CpuThreadPoolOptions options{};
    options.worker_count = 1;
    options.queue_capacity = 1;
    options.execution_group_key = "p01.cpu-runtime.avx2";
    cpu::CpuThreadPool pool{options};
    if (pool.start().state != cpu::CpuThreadPoolState::running) return 66;
    std::uint64_t observed = 0;
    if (!execute_probe(pool, observed)) {
        (void)stop_pool(pool);
        return 69;
    }
    if (!stop_pool(pool)) return 70;

    char output[96]{};
    const int length = std::snprintf(
        output, sizeof(output), "NODE_CPU_RUNTIME_PROBE %llu\n",
        static_cast<unsigned long long>(observed));
    if (length <= 0 || static_cast<std::size_t>(length) >= sizeof(output) ||
        !write_all(output, static_cast<std::size_t>(length))) return 71;
    return 0;
}

int run_resident() {
    sigset_t signals;
    (void)sigemptyset(&signals);
    (void)sigaddset(&signals, SIGTERM);
    (void)sigaddset(&signals, SIGINT);
    (void)sigaddset(&signals, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &signals, nullptr) != 0) return 73;

    const int serial = open(
        "/dev/ttyS0", O_WRONLY | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (serial >= 0) event_output_fd = serial;
    std::string root{};
    if (!root_prefix(root)) return 74;
    const std::string ready_path = root + kReadyPath;
    const std::string healthy_path = root + kHealthyPath;
    (void)unlink(ready_path.c_str());
    (void)unlink(healthy_path.c_str());

    cpu::CpuThreadPoolOptions options{};
    options.worker_count = 1;
    options.queue_capacity = 1;
    options.execution_group_key = "node.cpu-runtime.resident.avx2";
    cpu::CpuThreadPool pool{options};
    if (pool.start().state != cpu::CpuThreadPoolState::running) return 75;
    constexpr char ready[] =
        "schema=node.resident-service-ready.v1\n"
        "service=cpu_runtime_assembly\n"
        "state=ready\n";
    if (!write_marker(ready_path, ready)) {
        (void)stop_pool(pool);
        return 76;
    }
    constexpr char resident_event[] =
        "{\"record\":\"cpu_runtime_resident\","
        "\"subject\":\"node.cpu.runtime\",\"outcome\":\"active\","
        "\"abi\":\"node.cpu-runtime.conformance.v1\","
        "\"profile\":\"avx2\",\"scope\":\"current_boot_lab\"}\n";
    (void)write_all(resident_event, sizeof(resident_event) - 1U);

    bool probed = false;
    for (;;) {
        int signal_number = 0;
        if (sigwait(&signals, &signal_number) != 0) break;
        if (signal_number == SIGUSR1) {
            if (probed) continue;
            std::uint64_t observed = 0;
            const bool passed = execute_probe(pool, observed);
            char event[320]{};
            const int length = std::snprintf(
                event, sizeof(event),
                "{\"record\":\"runtime_post_transition_probe\","
                "\"subject\":\"node.cpu.runtime\",\"outcome\":\"%s\","
                "\"result\":%llu,\"expected_result\":120,"
                "\"worker_count\":1,\"queue_capacity\":1}\n",
                passed ? "passed" : "failed",
                static_cast<unsigned long long>(observed));
            if (length > 0 && static_cast<std::size_t>(length) < sizeof(event)) {
                (void)write_all(event, static_cast<std::size_t>(length));
            }
            if (!passed) break;
            constexpr char healthy[] =
                "schema=node.resident-service-health.v1\n"
                "service=cpu_runtime_assembly\n"
                "post_transition_probe=passed\n";
            if (!write_marker(healthy_path, healthy)) break;
            probed = true;
        } else {
            break;
        }
    }
    (void)unlink(ready_path.c_str());
    (void)unlink(healthy_path.c_str());
    const bool stopped = stop_pool(pool);
    constexpr char stopped_event[] =
        "{\"record\":\"cpu_runtime_resident\","
        "\"subject\":\"node.cpu.runtime\",\"outcome\":\"stopped\","
        "\"scope\":\"current_boot_lab\"}\n";
    (void)write_all(stopped_event, sizeof(stopped_event) - 1U);
    if (serial >= 0) (void)close(serial);
    return stopped ? 0 : 77;
}

}  // namespace

extern "C" const char* node_cpu_runtime_conformance_v1() noexcept {
    return kAbiIdentity;
}

int main(int argument_count, char** arguments) {
    if (argument_count != 2) return 64;
    if (std::strcmp(arguments[1], "--serve-probe") == 0) {
        return run_one_shot_probe();
    }
    if (std::strcmp(arguments[1], "--resident") == 0) {
        return run_resident();
    }
    return 64;
}
