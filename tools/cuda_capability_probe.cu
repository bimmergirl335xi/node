#include <cuda_runtime_api.h>

#include <cstddef>
#include <iomanip>
#include <iostream>

#include "cuda_capabilities.hpp"
#include "cuda_profiles.hpp"

namespace pcuda = prometheus::backends::cuda;

namespace {

void print_version(const char* label, const pcuda::CudaVersion& version,
                   bool available) {
    std::cout << "  " << label << ": ";
    if (!available || !version.valid()) {
        std::cout << "unavailable\n";
        return;
    }

    std::cout << version.major << '.' << version.minor;
    if (version.patch != 0) {
        std::cout << '.' << version.patch;
    }
    std::cout << " (encoded " << version.encoded << ")\n";
}

void print_support(const char* label, pcuda::CudaSupportState state) {
    std::cout << "  " << label << ": " << pcuda::to_string(state) << '\n';
}

void print_bytes(const char* label, std::size_t bytes) {
    constexpr double gib = 1024.0 * 1024.0 * 1024.0;
    std::cout << "  " << label << ": " << bytes << " bytes ("
              << std::fixed << std::setprecision(2)
              << static_cast<double>(bytes) / gib << " GiB)\n";
}

}  // namespace

int main() {
    int device_count = 0;
    const cudaError_t count_status = cudaGetDeviceCount(&device_count);
    if (count_status != cudaSuccess) {
        std::cerr << "cudaGetDeviceCount failed ("
                  << cudaGetErrorName(count_status) << "): "
                  << cudaGetErrorString(count_status) << '\n';
        return 1;
    }

    std::cout << "Prometheus CUDA capability probe\n";
    std::cout << "Discovered CUDA devices: " << device_count << "\n\n";

    const pcuda::CudaSoftwareVersions software =
        pcuda::query_cuda_software_versions();

    print_version("compile-time toolkit", software.compile_time_toolkit,
                  software.compile_time_toolkit.valid());
    print_version("loaded runtime", software.runtime,
                  software.runtime_version_available);
    print_version("driver-supported CUDA", software.driver,
                  software.driver_version_available);
    std::cout << "  compile-time release family: "
              << pcuda::to_string(software.compile_time_release_family) << '\n';
    std::cout << "  runtime release family: "
              << pcuda::to_string(software.runtime_release_family) << '\n';
    print_support("driver supports loaded runtime",
                  software.driver_supports_runtime);
    std::cout << '\n';

    bool all_queries_succeeded = true;

    for (int ordinal = 0; ordinal < device_count; ++ordinal) {
        cudaDeviceProp raw_properties{};
        const cudaError_t name_status =
            cudaGetDeviceProperties(&raw_properties, ordinal);

        const pcuda::CudaCapabilityQueryResult result =
            pcuda::query_cuda_device_capabilities(ordinal);

        std::cout << "Device ordinal " << ordinal;
        if (name_status == cudaSuccess) {
            std::cout << ": " << raw_properties.name;
        }
        std::cout << '\n';

        if (!result.status.ok()) {
            all_queries_succeeded = false;
            std::cout << "  query: failed\n"
                      << "  native error: " << result.status.native_error_code
                      << '\n'
                      << "  message: " << result.status.message << "\n\n";
            continue;
        }

        const pcuda::CudaDeviceCapabilities& caps = result.capabilities;
        std::cout << "  query: success\n";
        std::cout << "  compute capability: "
                  << caps.architecture.compute_capability.major << '.'
                  << caps.architecture.compute_capability.minor << '\n';
        std::cout << "  architecture family: "
                  << pcuda::to_string(caps.architecture.architecture) << '\n';
        print_support("Prometheus project policy",
                      caps.compatibility.prometheus_project);
        print_support("compile-time release compatibility",
                      caps.compatibility.compile_time_release_family);
        print_support("runtime release compatibility",
                      caps.compatibility.runtime_release_family);
        print_support("binary architecture image",
                      caps.compatibility.binary_architecture_image);
        print_support("registered kernel coverage",
                      caps.compatibility.registered_kernel_coverage);

        print_bytes("global memory", caps.total_global_memory_bytes);
        std::cout << "  multiprocessors: " << caps.multiprocessor_count << '\n';
        std::cout << "  warp size: " << caps.warp_size << '\n';
        std::cout << "  max threads per block: "
                  << caps.max_threads_per_block << '\n';
        std::cout << "  max threads per multiprocessor: "
                  << caps.max_threads_per_multiprocessor << '\n';
        std::cout << "  async engines: " << caps.async_engine_count << '\n';
        std::cout << "  memory bus width: "
                  << caps.memory_bus_width_bits << " bits\n";
        std::cout << "  L2 cache: " << caps.l2_cache_bytes << " bytes\n";

        print_support("concurrent kernels", caps.concurrent_kernels);
        print_support("copy/compute overlap", caps.copy_compute_overlap);
        print_support("unified addressing", caps.unified_addressing);
        print_support("managed memory", caps.managed_memory);
        print_support("cooperative launch", caps.cooperative_launch);
        print_support("stream priorities", caps.stream_priorities);
        print_support("memory pools", caps.memory_pools);
        print_support("ECC enabled", caps.ecc_enabled);
        std::cout << '\n';
    }

    return all_queries_succeeded ? 0 : 2;
}
