#include "cuda_capabilities.hpp"

#include <cuda_runtime_api.h>

#include <utility>

namespace prometheus::backends::cuda {
namespace {

[[nodiscard]] constexpr CudaSupportState state_from_bool(bool value) noexcept {
    return value ? CudaSupportState::supported : CudaSupportState::unsupported;
}

[[nodiscard]] CudaCapabilityQueryStatus make_cuda_error_status(
    CudaCapabilityQueryCode code,
    cudaError_t error,
    const char* operation) {
    CudaCapabilityQueryStatus status{};
    status.code = code;
    status.native_error_code = static_cast<int>(error);

    const char* error_name = cudaGetErrorName(error);
    const char* error_text = cudaGetErrorString(error);

    status.message = operation;
    status.message += " failed";
    if (error_name != nullptr) {
        status.message += " (";
        status.message += error_name;
        status.message += ")";
    }
    if (error_text != nullptr) {
        status.message += ": ";
        status.message += error_text;
    }
    return status;
}

[[nodiscard]] CudaSupportState support_for_release_family(
    const CudaArchitectureProfile& architecture,
    CudaReleaseFamily family) noexcept {
    return cuda_release_supports_architecture(family, architecture.architecture);
}

void populate_compatibility(CudaDeviceCapabilities& capabilities) noexcept {
    capabilities.compatibility.prometheus_project =
        capabilities.architecture.prometheus_supported;

    capabilities.compatibility.compile_time_release_family =
        support_for_release_family(
            capabilities.architecture,
            capabilities.software.compile_time_release_family);

    capabilities.compatibility.runtime_release_family =
        support_for_release_family(
            capabilities.architecture,
            capabilities.software.runtime_release_family);

    // Deliberately not inferred from compute capability.  The build system and
    // kernel registry must prove these later.
    capabilities.compatibility.binary_architecture_image =
        CudaSupportState::unknown;
    capabilities.compatibility.registered_kernel_coverage =
        CudaSupportState::unknown;
}

}  // namespace

CudaSoftwareVersions query_cuda_software_versions() noexcept {
    CudaSoftwareVersions versions{};

#ifdef CUDART_VERSION
    versions.compile_time_toolkit = decode_cuda_version(CUDART_VERSION);
    versions.compile_time_release_family =
        classify_cuda_release_family(CUDART_VERSION);
#endif

    int runtime_version = 0;
    const cudaError_t runtime_status = cudaRuntimeGetVersion(&runtime_version);
    if (runtime_status == cudaSuccess) {
        versions.runtime = decode_cuda_version(runtime_version);
        versions.runtime_release_family =
            classify_cuda_release_family(runtime_version);
        versions.runtime_version_available = true;
    }

    int driver_version = 0;
    const cudaError_t driver_status = cudaDriverGetVersion(&driver_version);
    if (driver_status == cudaSuccess) {
        versions.driver = decode_cuda_version(driver_version);
        versions.driver_version_available = true;
    }

    if (versions.runtime_version_available &&
        versions.driver_version_available) {
        versions.driver_supports_runtime = state_from_bool(
            versions.driver.encoded >= versions.runtime.encoded);
    }

    return versions;
}

CudaCapabilityQueryResult query_cuda_device_capabilities(
    int runtime_ordinal) {
    CudaCapabilityQueryResult result{};

    if (runtime_ordinal < 0) {
        result.status.code =
            CudaCapabilityQueryCode::invalid_device_ordinal;
        result.status.message = "CUDA runtime ordinal must be non-negative";
        return result;
    }

    result.capabilities.runtime_ordinal = runtime_ordinal;
    result.capabilities.software = query_cuda_software_versions();

    int device_count = 0;
    const cudaError_t count_status = cudaGetDeviceCount(&device_count);
    if (count_status != cudaSuccess) {
        result.status = make_cuda_error_status(
            CudaCapabilityQueryCode::runtime_query_failed,
            count_status,
            "cudaGetDeviceCount");
        return result;
    }

    if (runtime_ordinal >= device_count) {
        result.status.code =
            CudaCapabilityQueryCode::invalid_device_ordinal;
        result.status.message = "CUDA runtime ordinal is outside the discovered range";
        return result;
    }

    cudaDeviceProp properties{};
    const cudaError_t property_status =
        cudaGetDeviceProperties(&properties, runtime_ordinal);
    if (property_status != cudaSuccess) {
        result.status = make_cuda_error_status(
            CudaCapabilityQueryCode::device_property_query_failed,
            property_status,
            "cudaGetDeviceProperties");
        return result;
    }

    CudaDeviceCapabilities& capabilities = result.capabilities;
    capabilities.architecture = make_cuda_architecture_profile(
        CudaComputeCapability{properties.major, properties.minor});

    capabilities.total_global_memory_bytes = properties.totalGlobalMem;
    capabilities.total_constant_memory_bytes = properties.totalConstMem;
    capabilities.shared_memory_per_block_bytes = properties.sharedMemPerBlock;
    capabilities.shared_memory_per_multiprocessor_bytes =
        properties.sharedMemPerMultiprocessor;
    capabilities.shared_memory_per_block_optin_bytes =
        properties.sharedMemPerBlockOptin;

    capabilities.multiprocessor_count = properties.multiProcessorCount;
    capabilities.warp_size = properties.warpSize;
    capabilities.registers_per_block = properties.regsPerBlock;
    capabilities.registers_per_multiprocessor = properties.regsPerMultiprocessor;

    capabilities.max_threads_per_block = properties.maxThreadsPerBlock;
    capabilities.max_threads_per_multiprocessor =
        properties.maxThreadsPerMultiProcessor;
    capabilities.max_blocks_per_multiprocessor =
        properties.maxBlocksPerMultiProcessor;
    capabilities.max_block_dimensions = {
        properties.maxThreadsDim[0],
        properties.maxThreadsDim[1],
        properties.maxThreadsDim[2],
    };
    capabilities.max_grid_dimensions = {
        properties.maxGridSize[0],
        properties.maxGridSize[1],
        properties.maxGridSize[2],
    };

    capabilities.core_clock_khz = properties.clockRate;
    capabilities.memory_clock_khz = properties.memoryClockRate;
    capabilities.memory_bus_width_bits = properties.memoryBusWidth;
    capabilities.l2_cache_bytes = properties.l2CacheSize;
    capabilities.async_engine_count = properties.asyncEngineCount;

    capabilities.concurrent_kernels =
        state_from_bool(properties.concurrentKernels != 0);
    capabilities.copy_compute_overlap =
        state_from_bool(properties.deviceOverlap != 0);
    capabilities.unified_addressing =
        state_from_bool(properties.unifiedAddressing != 0);
    capabilities.managed_memory =
        state_from_bool(properties.managedMemory != 0);
    capabilities.concurrent_managed_access =
        state_from_bool(properties.concurrentManagedAccess != 0);
    capabilities.pageable_memory_access =
        state_from_bool(properties.pageableMemoryAccess != 0);
    capabilities.direct_managed_memory_access_from_host =
        state_from_bool(properties.directManagedMemAccessFromHost != 0);
    capabilities.host_native_atomic_operations =
        state_from_bool(properties.hostNativeAtomicSupported != 0);
    capabilities.cooperative_launch =
        state_from_bool(properties.cooperativeLaunch != 0);
    capabilities.stream_priorities =
        state_from_bool(properties.streamPrioritiesSupported != 0);
    capabilities.memory_pools =
        state_from_bool(properties.memoryPoolsSupported != 0);
    capabilities.compute_preemption =
        state_from_bool(properties.computePreemptionSupported != 0);
    capabilities.can_map_host_memory =
        state_from_bool(properties.canMapHostMemory != 0);
    capabilities.integrated_gpu =
        state_from_bool(properties.integrated != 0);
    capabilities.kernel_execution_timeout =
        state_from_bool(properties.kernelExecTimeoutEnabled != 0);
    capabilities.ecc_enabled =
        state_from_bool(properties.ECCEnabled != 0);

    populate_compatibility(capabilities);
    result.status = {};
    return result;
}

}  // namespace prometheus::backends::cuda
