#ifndef SRC_BACKENDS_CUDA_CUDA_MEMORY_POOL_HPP
#define SRC_BACKENDS_CUDA_CUDA_MEMORY_POOL_HPP

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "cuda_device_pool.hpp"

namespace prometheus::backends::cuda {

enum class CudaMemoryPoolCode : std::uint8_t {
    success = 0,
    partial_success,
    not_initialized,
    already_initialized,
    invalid_argument,
    unknown_device,
    device_unavailable,
    allocation_failed,
    allocation_not_found,
    allocation_not_in_use,
    active_allocations_present,
    release_failed,
};

struct CudaMemoryPoolStatus {
    CudaMemoryPoolCode code = CudaMemoryPoolCode::success;
    int native_error_code = 0;
    std::string message{};

    [[nodiscard]] bool ok() const noexcept {
        return code == CudaMemoryPoolCode::success ||
               code == CudaMemoryPoolCode::partial_success;
    }
};

struct CudaMemoryPoolOptions {
    // Cached blocks larger than this are returned directly to CUDA.
    std::uint64_t maximum_cached_block_bytes =
        256ULL * 1024ULL * 1024ULL;

    // Per-device cache budget. Zero disables block caching.
    std::uint64_t maximum_cached_bytes_per_device =
        1024ULL * 1024ULL * 1024ULL;

    // Keep the default conservative and portable across Pascal and newer
    // devices. The pool intentionally uses cudaMalloc/cudaFree rather than
    // requiring stream-ordered allocator support.
    bool cache_released_blocks = true;
};

struct CudaMemoryAllocation {
    std::uint64_t lease_id = 0;
    std::string persistent_key{};
    int runtime_ordinal_snapshot = -1;
    void* device_pointer = nullptr;
    std::uint64_t requested_bytes = 0;
    std::uint64_t capacity_bytes = 0;
    bool reused_cached_block = false;
    std::string owner_tag{};

    [[nodiscard]] bool valid() const noexcept {
        return lease_id != 0 && device_pointer != nullptr &&
               capacity_bytes >= requested_bytes && requested_bytes > 0;
    }
};

struct CudaMemoryAllocationResult {
    CudaMemoryPoolStatus status{};
    CudaMemoryAllocation allocation{};
};

struct CudaMemoryPoolDeviceSnapshot {
    std::string persistent_key{};
    int runtime_ordinal_snapshot = -1;
    std::uint64_t in_use_bytes = 0;
    std::uint64_t cached_bytes = 0;
    std::uint64_t release_failed_bytes = 0;
    std::size_t in_use_block_count = 0;
    std::size_t cached_block_count = 0;
    std::size_t release_failed_block_count = 0;
};

struct CudaMemoryPoolSnapshot {
    bool initialized = false;
    std::size_t device_count = 0;
    std::uint64_t in_use_bytes = 0;
    std::uint64_t cached_bytes = 0;
    std::uint64_t release_failed_bytes = 0;
    std::size_t in_use_block_count = 0;
    std::size_t cached_block_count = 0;
    std::size_t release_failed_block_count = 0;
    std::vector<CudaMemoryPoolDeviceSnapshot> devices{};
};

// Explicit, headless, per-process device-memory reuse pool.
//
// initialize() records eligible stable device identities but does not allocate
// memory or create streams. allocate() is the first operation that may create
// CUDA runtime/context state. No global singleton is provided.
class CudaMemoryPool {
public:
    explicit CudaMemoryPool(CudaMemoryPoolOptions options = {});
    ~CudaMemoryPool();

    CudaMemoryPool(const CudaMemoryPool&) = delete;
    CudaMemoryPool& operator=(const CudaMemoryPool&) = delete;
    CudaMemoryPool(CudaMemoryPool&&) = delete;
    CudaMemoryPool& operator=(CudaMemoryPool&&) = delete;

    [[nodiscard]] CudaMemoryPoolStatus initialize(
        const CudaDevicePool& device_pool);

    [[nodiscard]] CudaMemoryAllocationResult allocate(
        const std::string& persistent_key,
        std::uint64_t requested_bytes,
        std::string owner_tag = {});

    [[nodiscard]] CudaMemoryPoolStatus release(std::uint64_t lease_id);

    [[nodiscard]] CudaMemoryPoolStatus trim_device(
        const std::string& persistent_key);

    [[nodiscard]] CudaMemoryPoolStatus trim_all();

    // Refuses to release active leases unless force_release_in_use is true.
    // This never calls cudaDeviceReset().
    [[nodiscard]] CudaMemoryPoolStatus shutdown(
        bool force_release_in_use = false);

    [[nodiscard]] CudaMemoryPoolSnapshot snapshot() const;
    [[nodiscard]] bool initialized() const noexcept;

private:
    enum class BlockState : std::uint8_t {
        in_use = 0,
        cached,
        release_failed,
    };

    struct DeviceRecord {
        std::string persistent_key{};
        std::string canonical_uuid{};
        int runtime_ordinal_snapshot = -1;
    };

    struct Block {
        std::uint64_t block_id = 0;
        std::uint64_t lease_id = 0;
        std::string persistent_key{};
        int runtime_ordinal_snapshot = -1;
        void* pointer = nullptr;
        std::uint64_t requested_bytes = 0;
        std::uint64_t capacity_bytes = 0;
        BlockState state = BlockState::cached;
        std::string owner_tag{};
    };

    [[nodiscard]] const DeviceRecord* find_device_locked(
        const std::string& persistent_key) const noexcept;

    [[nodiscard]] int resolve_runtime_ordinal_locked(
        const DeviceRecord& device,
        CudaMemoryPoolStatus& status) const;

    [[nodiscard]] CudaMemoryPoolStatus free_block_locked(
        std::size_t block_index);

    [[nodiscard]] std::uint64_t cached_bytes_locked(
        const std::string& persistent_key) const noexcept;

    void prune_device_records_locked() noexcept;

    CudaMemoryPoolOptions options_{};
    mutable std::mutex mutex_{};
    bool initialized_ = false;
    std::uint64_t next_block_id_ = 1;
    std::uint64_t next_lease_id_ = 1;
    std::vector<DeviceRecord> devices_{};
    std::vector<Block> blocks_{};
};

[[nodiscard]] const char* to_string(CudaMemoryPoolCode value) noexcept;

}  // namespace prometheus::backends::cuda

#endif  // SRC_BACKENDS_CUDA_CUDA_MEMORY_POOL_HPP
