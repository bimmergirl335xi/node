#include <immintrin.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstring>

#include "cpu_thread_pool.hpp"

namespace cpu = prometheus::backends::cpu;

namespace {

constexpr char kAbiIdentity[] = "node.cpu-runtime.conformance.v1";
constexpr std::uint64_t kExpectedProbeResult = 120;

bool write_all(const char* value, std::size_t size) noexcept {
    while (size > 0) {
        const ssize_t count = write(STDOUT_FILENO, value, size);
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

}  // namespace

extern "C" const char* node_cpu_runtime_conformance_v1() noexcept {
    return kAbiIdentity;
}

int main(int argument_count, char** arguments) {
    if (argument_count != 2 ||
        std::strcmp(arguments[1], "--serve-probe") != 0) return 64;

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

    std::atomic<std::uint64_t> result{0};
    const cpu::CpuTaskSubmissionResult submission =
        pool.submit([&result]() { result.store(avx2_probe()); });
    if (!submission.accepted()) {
        (void)pool.stop();
        return 67;
    }
    if (!submission.handle.wait_for(std::chrono::seconds{1})) {
        (void)pool.stop();
        return 68;
    }
    if (!submission.handle.result().succeeded()) {
        (void)pool.stop();
        return 69;
    }
    if (pool.request_shutdown(cpu::CpuThreadPoolStopMode::drain) !=
            cpu::CpuThreadPoolShutdownResult::request_accepted ||
        pool.wait_for_shutdown(std::chrono::seconds{1}) !=
            cpu::CpuThreadPoolShutdownResult::fully_stopped) return 70;

    const std::uint64_t observed = result.load();
    char output[96]{};
    const int length = std::snprintf(
        output, sizeof(output), "NODE_CPU_RUNTIME_PROBE %llu\n",
        static_cast<unsigned long long>(observed));
    if (length <= 0 || static_cast<std::size_t>(length) >= sizeof(output) ||
        !write_all(output, static_cast<std::size_t>(length))) return 71;
    return observed == kExpectedProbeResult ? 0 : 72;
}
