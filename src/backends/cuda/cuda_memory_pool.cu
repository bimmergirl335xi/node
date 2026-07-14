#include "cuda_memory_pool.hpp"

#include <cuda_runtime_api.h>

#include <algorithm>
#include <limits>
#include <utility>

namespace prometheus::backends::cuda {
namespace {

class ScopedCudaDevice {
public:
    explicit ScopedCudaDevice(int target_ordinal) {
        cudaError_t error = cudaGetDevice(&previous_ordinal_);
        if (error != cudaSuccess) {
            status_ = error;
            return;
        }
        previous_available_ = true;

        if (previous_ordinal_ != target_ordinal) {
            error = cudaSetDevice(target_ordinal);
            if (error != cudaSuccess) {
                status_ = error;
                return;
            }
            changed_ = true;
        }

        status_ = cudaSuccess;
        active_ = true;
    }

    ~ScopedCudaDevice() {
        if (active_ && changed_ && previous_available_) {
            restore_status_ = cudaSetDevice(previous_ordinal_);
        }
    }

    [[nodiscard]] bool ok() const noexcept {
        return active_ && status_ == cudaSuccess;
    }

    [[nodiscard]] cudaError_t status() const noexcept {
        return status_;
    }

    [[nodiscard]] cudaError_t restore_now() noexcept {
        if (!active_ || !changed_ || !previous_available_) {
            return cudaSuccess;
        }

        const cudaError_t error = cudaSetDevice(previous_ordinal_);
        if (error == cudaSuccess) {
            changed_ = false;
        }
        restore_status_ = error;
        return error;
    }

private:
    int previous_ordinal_ = -1;
    bool previous_available_ = false;
    bool changed_ = false;
    bool active_ = false;
    cudaError_t status_ = cudaSuccess;
    cudaError_t restore_status_ = cudaSuccess;
};

[[nodiscard]] std::string cuda_error_message(
    const char* operation,
    cudaError_t error) {
    std::string message{operation};
    message += " failed: ";
    message += cudaGetErrorName(error);
    message += " (";
    message += cudaGetErrorString(error);
    message += ")";
    return message;
}

}  // namespace

CudaMemoryPool::CudaMemoryPool(CudaMemoryPoolOptions options)
    : options_(std::move(options)) {}

CudaMemoryPool::~CudaMemoryPool() {
    (void)shutdown(true);
}

CudaMemoryPoolStatus CudaMemoryPool::initialize(
    const CudaDevicePool& device_pool) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        return {
            CudaMemoryPoolCode::already_initialized,
            0,
            "CUDA memory pool is already initialized",
        };
    }

    devices_.clear();
    blocks_.clear();

    for (const CudaPoolDevice& pool_device : device_pool.devices) {
        const CudaDeviceIdentity& identity =
            pool_device.discovered.identity;

        if (!pool_device.registration_ready() ||
            !identity.has_stable_identity()) {
            continue;
        }

        DeviceRecord record{};
        record.persistent_key = identity.persistent_key;
        record.canonical_uuid = identity.uuid;
        record.runtime_ordinal_snapshot = identity.runtime_ordinal;
        devices_.push_back(std::move(record));
    }

    std::sort(
        devices_.begin(),
        devices_.end(),
        [](const DeviceRecord& left, const DeviceRecord& right) {
            return left.persistent_key < right.persistent_key;
        });

    devices_.erase(
        std::unique(
            devices_.begin(),
            devices_.end(),
            [](const DeviceRecord& left, const DeviceRecord& right) {
                return left.persistent_key == right.persistent_key;
            }),
        devices_.end());

    initialized_ = true;

    if (devices_.empty()) {
        return {
            CudaMemoryPoolCode::partial_success,
            0,
            "CUDA memory pool initialized without an eligible device",
        };
    }

    return {
        CudaMemoryPoolCode::success,
        0,
        "CUDA memory pool initialized",
    };
}

CudaMemoryAllocationResult CudaMemoryPool::allocate(
    const std::string& persistent_key,
    std::uint64_t requested_bytes,
    std::string owner_tag) {
    std::lock_guard<std::mutex> lock(mutex_);

    CudaMemoryAllocationResult result{};

    if (!initialized_) {
        result.status = {
            CudaMemoryPoolCode::not_initialized,
            0,
            "CUDA memory pool is not initialized",
        };
        return result;
    }

    if (persistent_key.empty() || requested_bytes == 0 ||
        requested_bytes >
            static_cast<std::uint64_t>(
                std::numeric_limits<std::size_t>::max())) {
        result.status = {
            CudaMemoryPoolCode::invalid_argument,
            0,
            "Allocation requires a device key and a nonzero representable size",
        };
        return result;
    }

    const DeviceRecord* device = find_device_locked(persistent_key);
    if (device == nullptr) {
        result.status = {
            CudaMemoryPoolCode::unknown_device,
            0,
            "Persistent CUDA device key is not registered in the pool",
        };
        return result;
    }

    CudaMemoryPoolStatus resolve_status{};
    const int runtime_ordinal =
        resolve_runtime_ordinal_locked(*device, resolve_status);
    if (runtime_ordinal < 0) {
        result.status = std::move(resolve_status);
        return result;
    }

    ScopedCudaDevice selected_device(runtime_ordinal);
    if (!selected_device.ok()) {
        result.status = {
            CudaMemoryPoolCode::device_unavailable,
            static_cast<int>(selected_device.status()),
            cuda_error_message("cudaSetDevice", selected_device.status()),
        };
        return result;
    }

    std::size_t best_index = blocks_.size();
    std::uint64_t best_capacity =
        std::numeric_limits<std::uint64_t>::max();

    for (std::size_t index = 0; index < blocks_.size(); ++index) {
        const Block& block = blocks_[index];
        if (block.persistent_key != persistent_key ||
            block.state != BlockState::cached ||
            block.capacity_bytes < requested_bytes) {
            continue;
        }

        if (block.capacity_bytes < best_capacity) {
            best_capacity = block.capacity_bytes;
            best_index = index;
        }
    }

    bool reused = false;
    Block* block = nullptr;

    if (best_index != blocks_.size()) {
        block = &blocks_[best_index];
        reused = true;
    } else {
        void* pointer = nullptr;
        const cudaError_t allocation_error =
            cudaMalloc(
                &pointer,
                static_cast<std::size_t>(requested_bytes));
        if (allocation_error != cudaSuccess) {
            (void)selected_device.restore_now();
            result.status = {
                CudaMemoryPoolCode::allocation_failed,
                static_cast<int>(allocation_error),
                cuda_error_message("cudaMalloc", allocation_error),
            };
            return result;
        }

        Block new_block{};
        new_block.block_id = next_block_id_++;
        new_block.persistent_key = persistent_key;
        new_block.runtime_ordinal_snapshot = runtime_ordinal;
        new_block.pointer = pointer;
        new_block.capacity_bytes = requested_bytes;
        new_block.state = BlockState::cached;
        blocks_.push_back(std::move(new_block));
        block = &blocks_.back();
    }

    block->lease_id = next_lease_id_++;
    block->runtime_ordinal_snapshot = runtime_ordinal;
    block->requested_bytes = requested_bytes;
    block->state = BlockState::in_use;
    block->owner_tag = std::move(owner_tag);

    result.allocation.lease_id = block->lease_id;
    result.allocation.persistent_key = block->persistent_key;
    result.allocation.runtime_ordinal_snapshot = runtime_ordinal;
    result.allocation.device_pointer = block->pointer;
    result.allocation.requested_bytes = requested_bytes;
    result.allocation.capacity_bytes = block->capacity_bytes;
    result.allocation.reused_cached_block = reused;
    result.allocation.owner_tag = block->owner_tag;

    const cudaError_t restore_error = selected_device.restore_now();
    if (restore_error != cudaSuccess) {
        result.status = {
            CudaMemoryPoolCode::partial_success,
            static_cast<int>(restore_error),
            cuda_error_message(
                "Restoring the caller CUDA device",
                restore_error),
        };
        return result;
    }

    result.status = {
        CudaMemoryPoolCode::success,
        0,
        reused ? "Reused a cached CUDA allocation"
               : "Created a CUDA allocation",
    };
    return result;
}

CudaMemoryPoolStatus CudaMemoryPool::release(std::uint64_t lease_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return {
            CudaMemoryPoolCode::not_initialized,
            0,
            "CUDA memory pool is not initialized",
        };
    }

    if (lease_id == 0) {
        return {
            CudaMemoryPoolCode::invalid_argument,
            0,
            "Lease ID must be nonzero",
        };
    }

    auto iterator = std::find_if(
        blocks_.begin(),
        blocks_.end(),
        [lease_id](const Block& block) {
            return block.lease_id == lease_id;
        });

    if (iterator == blocks_.end()) {
        return {
            CudaMemoryPoolCode::allocation_not_found,
            0,
            "CUDA allocation lease was not found",
        };
    }

    if (iterator->state != BlockState::in_use) {
        return {
            CudaMemoryPoolCode::allocation_not_in_use,
            0,
            "CUDA allocation lease is not active",
        };
    }

    const bool may_cache =
        options_.cache_released_blocks &&
        options_.maximum_cached_bytes_per_device > 0 &&
        iterator->capacity_bytes <= options_.maximum_cached_block_bytes &&
        cached_bytes_locked(iterator->persistent_key) +
                iterator->capacity_bytes <=
            options_.maximum_cached_bytes_per_device;

    if (may_cache) {
        iterator->lease_id = 0;
        iterator->requested_bytes = 0;
        iterator->state = BlockState::cached;
        iterator->owner_tag.clear();
        return {
            CudaMemoryPoolCode::success,
            0,
            "CUDA allocation returned to the cache",
        };
    }

    const std::size_t index =
        static_cast<std::size_t>(
            std::distance(blocks_.begin(), iterator));
    return free_block_locked(index);
}

CudaMemoryPoolStatus CudaMemoryPool::trim_device(
    const std::string& persistent_key) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return {
            CudaMemoryPoolCode::not_initialized,
            0,
            "CUDA memory pool is not initialized",
        };
    }

    if (find_device_locked(persistent_key) == nullptr) {
        return {
            CudaMemoryPoolCode::unknown_device,
            0,
            "Persistent CUDA device key is not registered in the pool",
        };
    }

    CudaMemoryPoolStatus aggregate{
        CudaMemoryPoolCode::success,
        0,
        "Cached CUDA allocations were trimmed",
    };

    for (std::size_t index = blocks_.size(); index > 0; --index) {
        const std::size_t current = index - 1;
        const Block& block = blocks_[current];
        if (block.persistent_key != persistent_key ||
            block.state == BlockState::in_use) {
            continue;
        }

        CudaMemoryPoolStatus status = free_block_locked(current);
        if (!status.ok()) {
            aggregate.code = CudaMemoryPoolCode::partial_success;
            aggregate.native_error_code = status.native_error_code;
            aggregate.message =
                "One or more cached CUDA allocations could not be trimmed";
        }
    }

    return aggregate;
}

CudaMemoryPoolStatus CudaMemoryPool::trim_all() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return {
            CudaMemoryPoolCode::not_initialized,
            0,
            "CUDA memory pool is not initialized",
        };
    }

    CudaMemoryPoolStatus aggregate{
        CudaMemoryPoolCode::success,
        0,
        "All cached CUDA allocations were trimmed",
    };

    for (std::size_t index = blocks_.size(); index > 0; --index) {
        const std::size_t current = index - 1;
        if (blocks_[current].state == BlockState::in_use) {
            continue;
        }

        CudaMemoryPoolStatus status = free_block_locked(current);
        if (!status.ok()) {
            aggregate.code = CudaMemoryPoolCode::partial_success;
            aggregate.native_error_code = status.native_error_code;
            aggregate.message =
                "One or more cached CUDA allocations could not be trimmed";
        }
    }

    return aggregate;
}

CudaMemoryPoolStatus CudaMemoryPool::shutdown(
    bool force_release_in_use) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return {
            CudaMemoryPoolCode::success,
            0,
            "CUDA memory pool is already stopped",
        };
    }

    const bool has_active = std::any_of(
        blocks_.begin(),
        blocks_.end(),
        [](const Block& block) {
            return block.state == BlockState::in_use;
        });

    if (has_active && !force_release_in_use) {
        return {
            CudaMemoryPoolCode::active_allocations_present,
            0,
            "Active CUDA allocation leases prevent shutdown",
        };
    }

    CudaMemoryPoolStatus aggregate{
        CudaMemoryPoolCode::success,
        0,
        "CUDA memory pool stopped",
    };

    for (std::size_t index = blocks_.size(); index > 0; --index) {
        CudaMemoryPoolStatus status = free_block_locked(index - 1);
        if (!status.ok()) {
            aggregate.code = CudaMemoryPoolCode::partial_success;
            aggregate.native_error_code = status.native_error_code;
            aggregate.message =
                "CUDA memory pool stopped with unreleased allocations";
        }
    }

    if (blocks_.empty()) {
        devices_.clear();
        initialized_ = false;
    }

    return aggregate;
}

CudaMemoryPoolSnapshot CudaMemoryPool::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);

    CudaMemoryPoolSnapshot result{};
    result.initialized = initialized_;
    result.device_count = devices_.size();

    for (const DeviceRecord& device : devices_) {
        CudaMemoryPoolDeviceSnapshot device_snapshot{};
        device_snapshot.persistent_key = device.persistent_key;
        device_snapshot.runtime_ordinal_snapshot =
            device.runtime_ordinal_snapshot;

        for (const Block& block : blocks_) {
            if (block.persistent_key != device.persistent_key) {
                continue;
            }

            switch (block.state) {
                case BlockState::in_use:
                    device_snapshot.in_use_bytes += block.capacity_bytes;
                    ++device_snapshot.in_use_block_count;
                    break;
                case BlockState::cached:
                    device_snapshot.cached_bytes += block.capacity_bytes;
                    ++device_snapshot.cached_block_count;
                    break;
                case BlockState::release_failed:
                    device_snapshot.release_failed_bytes +=
                        block.capacity_bytes;
                    ++device_snapshot.release_failed_block_count;
                    break;
            }
        }

        result.in_use_bytes += device_snapshot.in_use_bytes;
        result.cached_bytes += device_snapshot.cached_bytes;
        result.release_failed_bytes +=
            device_snapshot.release_failed_bytes;
        result.in_use_block_count +=
            device_snapshot.in_use_block_count;
        result.cached_block_count +=
            device_snapshot.cached_block_count;
        result.release_failed_block_count +=
            device_snapshot.release_failed_block_count;
        result.devices.push_back(std::move(device_snapshot));
    }

    return result;
}

bool CudaMemoryPool::initialized() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return initialized_;
}

const CudaMemoryPool::DeviceRecord* CudaMemoryPool::find_device_locked(
    const std::string& persistent_key) const noexcept {
    const auto iterator = std::find_if(
        devices_.begin(),
        devices_.end(),
        [&persistent_key](const DeviceRecord& device) {
            return device.persistent_key == persistent_key;
        });

    return iterator == devices_.end() ? nullptr : &*iterator;
}

int CudaMemoryPool::resolve_runtime_ordinal_locked(
    const DeviceRecord& device,
    CudaMemoryPoolStatus& status) const {
    int device_count = 0;
    const cudaError_t count_error = cudaGetDeviceCount(&device_count);
    if (count_error != cudaSuccess) {
        status = {
            CudaMemoryPoolCode::device_unavailable,
            static_cast<int>(count_error),
            cuda_error_message("cudaGetDeviceCount", count_error),
        };
        return -1;
    }

    for (int ordinal = 0; ordinal < device_count; ++ordinal) {
        const CudaIdentityQueryResult identity =
            query_cuda_device_identity(ordinal);
        if (!identity.status.completed() ||
            !identity.identity.has_stable_identity()) {
            continue;
        }

        if (identity.identity.persistent_key == device.persistent_key) {
            status = {
                CudaMemoryPoolCode::success,
                0,
                "Resolved CUDA runtime ordinal by stable identity",
            };
            return ordinal;
        }
    }

    status = {
        CudaMemoryPoolCode::device_unavailable,
        0,
        "Stable CUDA device identity is no longer visible",
    };
    return -1;
}

CudaMemoryPoolStatus CudaMemoryPool::free_block_locked(
    std::size_t block_index) {
    if (block_index >= blocks_.size()) {
        return {
            CudaMemoryPoolCode::allocation_not_found,
            0,
            "CUDA allocation block index is invalid",
        };
    }

    Block& block = blocks_[block_index];
    const DeviceRecord* device =
        find_device_locked(block.persistent_key);
    if (device == nullptr) {
        block.state = BlockState::release_failed;
        return {
            CudaMemoryPoolCode::unknown_device,
            0,
            "Allocation device is no longer registered",
        };
    }

    CudaMemoryPoolStatus resolve_status{};
    const int runtime_ordinal =
        resolve_runtime_ordinal_locked(*device, resolve_status);
    if (runtime_ordinal < 0) {
        block.state = BlockState::release_failed;
        return resolve_status;
    }

    ScopedCudaDevice selected_device(runtime_ordinal);
    if (!selected_device.ok()) {
        block.state = BlockState::release_failed;
        return {
            CudaMemoryPoolCode::device_unavailable,
            static_cast<int>(selected_device.status()),
            cuda_error_message("cudaSetDevice", selected_device.status()),
        };
    }

    const cudaError_t free_error = cudaFree(block.pointer);
    const cudaError_t restore_error = selected_device.restore_now();

    if (free_error != cudaSuccess) {
        block.state = BlockState::release_failed;
        return {
            CudaMemoryPoolCode::release_failed,
            static_cast<int>(free_error),
            cuda_error_message("cudaFree", free_error),
        };
    }

    blocks_.erase(blocks_.begin() + static_cast<std::ptrdiff_t>(block_index));

    if (restore_error != cudaSuccess) {
        return {
            CudaMemoryPoolCode::partial_success,
            static_cast<int>(restore_error),
            cuda_error_message(
                "Restoring the caller CUDA device",
                restore_error),
        };
    }

    return {
        CudaMemoryPoolCode::success,
        0,
        "CUDA allocation returned to the runtime",
    };
}

std::uint64_t CudaMemoryPool::cached_bytes_locked(
    const std::string& persistent_key) const noexcept {
    std::uint64_t total = 0;
    for (const Block& block : blocks_) {
        if (block.persistent_key == persistent_key &&
            block.state == BlockState::cached) {
            total += block.capacity_bytes;
        }
    }
    return total;
}

void CudaMemoryPool::prune_device_records_locked() noexcept {
    devices_.erase(
        std::remove_if(
            devices_.begin(),
            devices_.end(),
            [this](const DeviceRecord& device) {
                return std::none_of(
                    blocks_.begin(),
                    blocks_.end(),
                    [&device](const Block& block) {
                        return block.persistent_key ==
                               device.persistent_key;
                    });
            }),
        devices_.end());
}

const char* to_string(CudaMemoryPoolCode value) noexcept {
    switch (value) {
        case CudaMemoryPoolCode::success:
            return "success";
        case CudaMemoryPoolCode::partial_success:
            return "partial_success";
        case CudaMemoryPoolCode::not_initialized:
            return "not_initialized";
        case CudaMemoryPoolCode::already_initialized:
            return "already_initialized";
        case CudaMemoryPoolCode::invalid_argument:
            return "invalid_argument";
        case CudaMemoryPoolCode::unknown_device:
            return "unknown_device";
        case CudaMemoryPoolCode::device_unavailable:
            return "device_unavailable";
        case CudaMemoryPoolCode::allocation_failed:
            return "allocation_failed";
        case CudaMemoryPoolCode::allocation_not_found:
            return "allocation_not_found";
        case CudaMemoryPoolCode::allocation_not_in_use:
            return "allocation_not_in_use";
        case CudaMemoryPoolCode::active_allocations_present:
            return "active_allocations_present";
        case CudaMemoryPoolCode::release_failed:
            return "release_failed";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cuda
