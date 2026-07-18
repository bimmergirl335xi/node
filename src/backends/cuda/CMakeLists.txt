add_library(prometheus_cuda_runtime_compilation STATIC
    cuda_runtime_compilation.cpp
)

target_include_directories(prometheus_cuda_runtime_compilation
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src/backends
        ${PROJECT_SOURCE_DIR}/src/backends/cuda
)

target_link_libraries(prometheus_cuda_runtime_compilation
    PRIVATE
        Threads::Threads
)

target_compile_features(
    prometheus_cuda_runtime_compilation
    PUBLIC
        cxx_std_17
)

if(PROMETHEUS_CUDA_AVAILABLE)
    add_library(prometheus_cuda_backend STATIC
        cuda_backend.cu
        cuda_capabilities.cu
        cuda_device.cu
        cuda_device_pool.cu
        cuda_device_worker.cpp
        cuda_evidence_adapter.cpp
        cuda_health.cu
        cuda_job_queue.cu
        cuda_kernel_adapter.cpp
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
            prometheus_gpu_evidence_correlation
            prometheus_cuda_kernels
            CUDA::cudart
            Threads::Threads
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

    add_executable(cuda_evidence_correlation_probe
        "${PROJECT_SOURCE_DIR}/tools/cuda_evidence_correlation_probe.cpp"
    )

    target_link_libraries(cuda_evidence_correlation_probe
        PRIVATE
            prometheus_cuda_backend
            prometheus_linux_pci_inventory
    )

    target_compile_features(cuda_evidence_correlation_probe PRIVATE cxx_std_17)

    add_executable(cuda_device_worker_probe
        "${PROJECT_SOURCE_DIR}/tools/cuda_device_worker_probe.cpp"
    )

    target_link_libraries(cuda_device_worker_probe
        PRIVATE
            prometheus_cuda_backend
    )

    target_compile_features(cuda_device_worker_probe PRIVATE cxx_std_17)
endif()
