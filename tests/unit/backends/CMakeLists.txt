if(NOT TARGET prometheus_cpu_foundation)
    message(
        FATAL_ERROR
        "prometheus_cpu_foundation target was not created before backend tests"
    )
endif()

if(NOT TARGET prometheus_cpu_backend)
    message(
        FATAL_ERROR
        "prometheus_cpu_backend target was not created before backend tests"
    )
endif()

if(NOT TARGET prometheus_arm_capabilities)
    message(
        FATAL_ERROR
        "prometheus_arm_capabilities target was not created before backend tests"
    )
endif()

add_executable(test_cpu_foundation
    test_cpu_foundation.cpp
)

target_link_libraries(test_cpu_foundation
    PRIVATE
        prometheus_cpu_foundation
)

target_include_directories(test_cpu_foundation
    PRIVATE
        "${PROJECT_SOURCE_DIR}/src/backends/cpu"
)

target_compile_features(test_cpu_foundation
    PRIVATE
        cxx_std_17
)

add_test(
    NAME prometheus_cpu_foundation
    COMMAND test_cpu_foundation
)


add_executable(test_cpu_backend_registry
    test_cpu_backend_registry.cpp
)

target_link_libraries(test_cpu_backend_registry
    PRIVATE
        prometheus_cpu_backend
)

target_compile_features(test_cpu_backend_registry
    PRIVATE
        cxx_std_17
)

add_test(
    NAME prometheus_cpu_backend_registry
    COMMAND test_cpu_backend_registry
)


add_executable(test_cuda_backend_registry
    test_cuda_backend_registry.cu
)

target_link_libraries(test_cuda_backend_registry
    PRIVATE
        prometheus_cuda_backend
        prometheus_cpu_backend
)

target_compile_features(test_cuda_backend_registry PRIVATE cxx_std_17)

set_target_properties(test_cuda_backend_registry PROPERTIES
    CUDA_STANDARD 17
    CUDA_STANDARD_REQUIRED YES
    CUDA_EXTENSIONS OFF
)

add_test(
    NAME prometheus_cuda_backend_registry
    COMMAND test_cuda_backend_registry
)


add_executable(test_cuda_runtime_resources
    test_cuda_runtime_resources.cu
)

target_link_libraries(test_cuda_runtime_resources
    PRIVATE
        prometheus_cuda_backend
)

target_compile_features(test_cuda_runtime_resources PRIVATE cxx_std_17)

set_target_properties(test_cuda_runtime_resources PROPERTIES
    CUDA_STANDARD 17
    CUDA_STANDARD_REQUIRED YES
    CUDA_EXTENSIONS OFF
)

add_test(
    NAME prometheus_cuda_runtime_resources
    COMMAND test_cuda_runtime_resources
)

if(NOT TARGET prometheus_cuda_runtime_compilation)
    message(
        FATAL_ERROR
        "prometheus_cuda_runtime_compilation target is unavailable"
    )
endif()

add_executable(test_cuda_runtime_compilation
    test_cuda_runtime_compilation.cpp
)

target_link_libraries(test_cuda_runtime_compilation
    PRIVATE
        prometheus_cuda_runtime_compilation
)

target_compile_features(test_cuda_runtime_compilation PRIVATE cxx_std_17)

add_test(
    NAME prometheus_cuda_runtime_compilation
    COMMAND test_cuda_runtime_compilation
)


add_executable(test_cpu_health_capacity
    test_cpu_health_capacity.cpp
)

target_link_libraries(test_cpu_health_capacity
    PRIVATE
        prometheus_cpu_foundation
        Threads::Threads
)

target_compile_features(test_cpu_health_capacity
    PRIVATE
        cxx_std_17
)

add_test(
    NAME prometheus_cpu_health_capacity
    COMMAND test_cpu_health_capacity
)


add_executable(test_arm_capabilities
    test_arm_capabilities.cpp
)

target_link_libraries(test_arm_capabilities
    PRIVATE
        prometheus_arm_capabilities
)

target_compile_features(test_arm_capabilities
    PRIVATE
        cxx_std_17
)

add_test(
    NAME prometheus_arm_capabilities
    COMMAND test_arm_capabilities
)


add_executable(test_arm_linux_auxv
    test_arm_linux_auxv.cpp
)

target_link_libraries(test_arm_linux_auxv
    PRIVATE
        prometheus_arm_capabilities
)

target_compile_features(test_arm_linux_auxv
    PRIVATE
        cxx_std_17
)

add_test(
    NAME prometheus_arm_linux_auxv
    COMMAND test_arm_linux_auxv
)


add_executable(test_arm_processor_identity
    test_arm_processor_identity.cpp
)

target_link_libraries(test_arm_processor_identity
    PRIVATE
        prometheus_arm_capabilities
)

target_compile_features(test_arm_processor_identity
    PRIVATE
        cxx_std_17
)

add_test(
    NAME prometheus_arm_processor_identity
    COMMAND test_arm_processor_identity
)


if(NOT TARGET prometheus_cpu_thread_pool)
    message(
        FATAL_ERROR
        "prometheus_cpu_thread_pool target was not created before backend tests"
    )
endif()

add_executable(test_cpu_thread_pool
    test_cpu_thread_pool.cpp
)

target_link_libraries(test_cpu_thread_pool
    PRIVATE
        prometheus_cpu_thread_pool
)

target_compile_features(test_cpu_thread_pool
    PRIVATE
        cxx_std_17
)

add_test(
    NAME prometheus_cpu_thread_pool
    COMMAND test_cpu_thread_pool
)


add_executable(test_cpu_simd_dispatch
    test_cpu_simd_dispatch.cpp
)

target_link_libraries(test_cpu_simd_dispatch
    PRIVATE
        prometheus_cpu_foundation
)

target_compile_features(test_cpu_simd_dispatch
    PRIVATE
        cxx_std_17
)

add_test(
    NAME prometheus_cpu_simd_dispatch
    COMMAND test_cpu_simd_dispatch
)

if(NOT TARGET prometheus_hailo_backend)
    message(FATAL_ERROR "prometheus_hailo_backend target is unavailable")
endif()

add_executable(test_hailo_backend
    test_hailo_backend.cpp
)

target_link_libraries(test_hailo_backend
    PRIVATE
        prometheus_hailo_backend
)

target_compile_features(test_hailo_backend PRIVATE cxx_std_17)

add_test(
    NAME prometheus_hailo_backend
    COMMAND test_hailo_backend
)
