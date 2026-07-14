add_library(prometheus_cuda_backend STATIC
    cuda_backend.cu
    cuda_capabilities.cu
    cuda_device.cu
    cuda_device_pool.cu
    cuda_health.cu
    cuda_job_queue.cu
    cuda_kernel_registry.cu
    cuda_memory_pool.cu
    cuda_profiles.cpp
)

target_include_directories(prometheus_cuda_backend
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src/backends
        ${PROJECT_SOURCE_DIR}/src/backends/cuda
)

target_link_libraries(prometheus_cuda_backend
    PUBLIC
        prometheus_backend_core
        CUDA::cudart
)

target_compile_features(prometheus_cuda_backend PUBLIC cxx_std_17)

set_target_properties(prometheus_cuda_backend PROPERTIES
    CUDA_STANDARD 17
    CUDA_STANDARD_REQUIRED YES
    CUDA_EXTENSIONS OFF
    CUDA_SEPARABLE_COMPILATION ON
)

# Do not add an unconditional -ldl here. The current glibc/CUDA 12.4 bench
# resolves dlopen from libc and nvlink rejects the compatibility libdl.a.
