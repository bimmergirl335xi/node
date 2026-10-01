#include <immintrin.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <charconv>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "cpu_thread_pool.hpp"

namespace cpu = prometheus::backends::cpu;

extern "C" const char* node_cpu_runtime_conformance_v1() noexcept;

namespace {

constexpr char kAbiIdentity[] = "node.cpu-runtime.conformance.v1";
constexpr std::uint64_t kExpectedProbeResult = 120;
constexpr std::size_t kMaximumWorkers = 16;
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

bool parse_worker_count(const char* value, std::size_t& output) noexcept {
    if (value == nullptr || value[0] == '\0') return false;
    const std::size_t length = std::strlen(value);
    const auto parsed = std::from_chars(value, value + length, output);
    return parsed.ec == std::errc{} && parsed.ptr == value + length &&
           output >= 1 && output <= kMaximumWorkers;
}

bool execute_probe(cpu::CpuThreadPool& pool,
                   std::size_t job_count,
                   std::uint64_t& observed,
                   std::size_t& completed) {
    std::atomic<std::uint64_t> result{0};
    std::atomic<std::size_t> completion_count{0};
    std::vector<cpu::CpuTaskHandle> handles{};
    handles.reserve(job_count);
    for (std::size_t index = 0; index < job_count; ++index) {
        cpu::CpuTaskSubmissionResult submission = pool.submit(
            [&result, &completion_count]() {
                result.fetch_add(avx2_probe(), std::memory_order_relaxed);
                completion_count.fetch_add(1, std::memory_order_relaxed);
            });
        if (!submission.accepted()) return false;
        handles.push_back(std::move(submission.handle));
    }
    for (const cpu::CpuTaskHandle& handle : handles) {
        if (!handle.wait_for(std::chrono::seconds{2}) ||
            !handle.result().succeeded()) return false;
    }
    observed = result.load(std::memory_order_relaxed);
    completed = completion_count.load(std::memory_order_relaxed);
    return completed == job_count &&
           observed == kExpectedProbeResult * job_count;
}

bool stop_pool(cpu::CpuThreadPool& pool) {
    return pool.request_shutdown(cpu::CpuThreadPoolStopMode::drain) ==
               cpu::CpuThreadPoolShutdownResult::request_accepted &&
           pool.wait_for_shutdown(std::chrono::seconds{1}) ==
               cpu::CpuThreadPoolShutdownResult::fully_stopped;
}

int run_one_shot_probe(std::size_t worker_count) {
    constexpr char handshake[] =
        "NODE_CPU_RUNTIME_ABI node.cpu-runtime.conformance.v1\n";
    if (std::strcmp(node_cpu_runtime_conformance_v1(), kAbiIdentity) != 0 ||
        !write_all(handshake, sizeof(handshake) - 1U)) return 65;

    cpu::CpuThreadPoolOptions options{};
    options.worker_count = worker_count;
    options.queue_capacity = worker_count;
    options.execution_group_key = "p01.cpu-runtime.avx2";
    cpu::CpuThreadPool pool{options};
    if (pool.start().state != cpu::CpuThreadPoolState::running) return 66;
    std::uint64_t observed = 0;
    std::size_t completed = 0;
    if (!execute_probe(pool, worker_count, observed, completed)) {
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

int run_resident(std::size_t worker_count) {
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
    options.worker_count = worker_count;
    options.queue_capacity = worker_count;
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
    char resident_event[320]{};
    const int resident_length = std::snprintf(
        resident_event, sizeof(resident_event),
        "{\"record\":\"cpu_runtime_resident\","
        "\"subject\":\"node.cpu.runtime\",\"outcome\":\"active\","
        "\"abi\":\"node.cpu-runtime.conformance.v1\","
        "\"profile\":\"avx2\",\"runtime_worker_count\":%zu,"
        "\"queue_capacity\":%zu,\"scope\":\"current_boot_lab\"}\n",
        worker_count, worker_count);
    if (resident_length > 0 &&
        static_cast<std::size_t>(resident_length) < sizeof(resident_event)) {
        (void)write_all(resident_event,
                        static_cast<std::size_t>(resident_length));
    }

    bool probed = false;
    for (;;) {
        int signal_number = 0;
        if (sigwait(&signals, &signal_number) != 0) break;
        if (signal_number == SIGUSR1) {
            if (probed) continue;
            std::uint64_t observed = 0;
            std::size_t completed = 0;
            const bool passed = execute_probe(
                pool, worker_count, observed, completed);
            char event[512]{};
            const int length = std::snprintf(
                event, sizeof(event),
                "{\"record\":\"runtime_post_transition_probe\","
                "\"subject\":\"node.cpu.runtime\",\"outcome\":\"%s\","
                "\"result\":%llu,\"expected_result\":%llu,"
                "\"runtime_worker_count\":%zu,\"queue_capacity\":%zu,"
                "\"multi_worker_probe_jobs\":%zu,"
                "\"multi_worker_probe_result\":\"%s\"}\n",
                passed ? "passed" : "failed",
                static_cast<unsigned long long>(observed),
                static_cast<unsigned long long>(
                    kExpectedProbeResult * worker_count),
                worker_count, worker_count, completed,
                passed ? "passed" : "failed");
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
    if (argument_count != 3) return 64;
    std::size_t worker_count = 0;
    if (!parse_worker_count(arguments[2], worker_count)) return 64;
    if (std::strcmp(arguments[1], "--serve-probe") == 0) {
        return run_one_shot_probe(worker_count);
    }
    if (std::strcmp(arguments[1], "--resident") == 0) {
        return run_resident(worker_count);
    }
    return 64;
}
