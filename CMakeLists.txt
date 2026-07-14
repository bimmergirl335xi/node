cmake_minimum_required(VERSION 3.24)

project(prometheus_node VERSION 0.5.2 LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_EXTENSIONS OFF)

set(
    PROMETHEUS_CUDA_ARCHITECTURES
    "61;70"
    CACHE STRING
    "CUDA architectures compiled into Prometheus binaries"
)
set(CMAKE_CUDA_ARCHITECTURES "${PROMETHEUS_CUDA_ARCHITECTURES}")

option(PROMETHEUS_BUILD_TESTS "Build Prometheus tests" ON)
option(PROMETHEUS_BUILD_BENCHMARKS "Build Prometheus benchmarks" ON)
option(PROMETHEUS_BUILD_LEGACY_VISION "Build legacy/vision_swarm_11.cu" OFF)
option(PROMETHEUS_CUDA_FAST_MATH "Enable CUDA --use_fast_math" OFF)

find_package(CUDAToolkit REQUIRED)
find_package(Threads REQUIRED)

foreach(PROMETHEUS_ARCH IN LISTS CMAKE_CUDA_ARCHITECTURES)
    if(CMAKE_CUDA_COMPILER_VERSION VERSION_GREATER_EQUAL 12.0 AND PROMETHEUS_ARCH LESS 50)
        message(
            FATAL_ERROR
            "CUDA ${CMAKE_CUDA_COMPILER_VERSION} cannot build sm_${PROMETHEUS_ARCH}. "
            "Use CUDA 11.4 for the Kepler release."
        )
    endif()

    if(CMAKE_CUDA_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0 AND PROMETHEUS_ARCH LESS 75)
        message(
            FATAL_ERROR
            "Use CUDA 12.x for Pascal/Volta or CUDA 11.4 for Kepler."
        )
    endif()
endforeach()

add_subdirectory(src/kernels)
add_subdirectory(src/backends)

function(prometheus_configure_cuda_target target)
    target_link_libraries(
        ${target}
        PRIVATE
            prometheus_cuda_kernels
            CUDA::cudart
    )
    set_target_properties(
        ${target}
        PROPERTIES
            CUDA_SEPARABLE_COMPILATION ON
    )
endfunction()

if(PROMETHEUS_BUILD_TESTS)
    enable_testing()

    function(prometheus_add_cuda_test target source test_name)
        add_executable(${target} ${source})
        prometheus_configure_cuda_target(${target})
        add_test(NAME ${test_name} COMMAND ${target})
    endfunction()

    prometheus_add_cuda_test(
        test_kernel_foundation
        tests/unit/kernels/test_kernel_foundation.cu
        kernel_foundation
    )
    prometheus_add_cuda_test(
        test_dense_kernels
        tests/unit/kernels/test_dense_kernels.cu
        dense_kernels
    )
    prometheus_add_cuda_test(
        test_normalization_kernels
        tests/unit/kernels/test_normalization_kernels.cu
        normalization_kernels
    )
    prometheus_add_cuda_test(
        test_temporal_kernels
        tests/unit/kernels/test_temporal_kernels.cu
        temporal_kernels
    )
    prometheus_add_cuda_test(
        test_learning_kernels
        tests/unit/kernels/test_learning_kernels.cu
        learning_kernels
    )
    prometheus_add_cuda_test(
        test_optimizer_kernels
        tests/unit/kernels/test_optimizer_kernels.cu
        optimizer_kernels
    )

    add_subdirectory(tests/unit/backends)
endif()

if(PROMETHEUS_BUILD_BENCHMARKS)
    function(prometheus_add_cuda_benchmark target source)
        add_executable(${target} ${source})
        prometheus_configure_cuda_target(${target})
    endfunction()

    prometheus_add_cuda_benchmark(
        benchmark_dense_kernels
        benchmarks/dense_benchmark.cu
    )
    prometheus_add_cuda_benchmark(
        benchmark_normalization_kernels
        benchmarks/normalization_benchmark.cu
    )
    prometheus_add_cuda_benchmark(
        benchmark_temporal_kernels
        benchmarks/temporal_benchmark.cu
    )
    prometheus_add_cuda_benchmark(
        benchmark_learning_kernels
        benchmarks/learning_benchmark.cu
    )
    prometheus_add_cuda_benchmark(
        benchmark_optimizer_kernels
        benchmarks/optimizer_benchmark.cu
    )
endif()

if(PROMETHEUS_BUILD_LEGACY_VISION)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/legacy/vision_swarm_11.cu")
        message(
            FATAL_ERROR
            "PROMETHEUS_BUILD_LEGACY_VISION=ON, but "
            "legacy/vision_swarm_11.cu does not exist."
        )
    endif()

    find_package(Threads REQUIRED)

    add_executable(
        prometheus_legacy_vision
        legacy/vision_swarm_11.cu
    )
    target_link_libraries(
        prometheus_legacy_vision
        PRIVATE
            CUDA::cudart
            Threads::Threads
    )
    set_target_properties(
        prometheus_legacy_vision
        PROPERTIES
            CUDA_SEPARABLE_COMPILATION ON
    )
endif()

message(STATUS "Prometheus CUDA compiler: ${CMAKE_CUDA_COMPILER_VERSION}")
message(STATUS "Prometheus CUDA architectures: ${CMAKE_CUDA_ARCHITECTURES}")
message(STATUS "Prometheus kernel tests: ${PROMETHEUS_BUILD_TESTS}")
message(STATUS "Prometheus kernel benchmarks: ${PROMETHEUS_BUILD_BENCHMARKS}")
