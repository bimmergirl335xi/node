add_library(prometheus_cpu_foundation STATIC
    cpu_identity.cpp
    cpu_topology.cpp
    cpu_capabilities.cpp
    cpu_health.cpp
    cpu_capacity.cpp
)

target_include_directories(prometheus_cpu_foundation
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_features(prometheus_cpu_foundation
    PUBLIC
        cxx_std_17
)

add_library(prometheus_cpu_thread_pool STATIC
    cpu_thread_pool.cpp
)

target_include_directories(prometheus_cpu_thread_pool
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(prometheus_cpu_thread_pool
    PUBLIC
        Threads::Threads
)

target_compile_features(prometheus_cpu_thread_pool
    PUBLIC
        cxx_std_17
)

add_library(prometheus_cpu_backend STATIC
    cpu_backend.cpp
)

target_include_directories(prometheus_cpu_backend
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(prometheus_cpu_backend
    PUBLIC
        prometheus_backend_core
        prometheus_cpu_foundation
    PRIVATE
        Threads::Threads
)

target_compile_features(prometheus_cpu_backend
    PUBLIC
        cxx_std_17
)

add_executable(prometheus_cpu_backend_probe EXCLUDE_FROM_ALL
    "${PROJECT_SOURCE_DIR}/tools/cpu_backend_probe.cpp"
)

target_link_libraries(prometheus_cpu_backend_probe
    PRIVATE
        prometheus_cpu_backend
)

target_compile_features(prometheus_cpu_backend_probe
    PRIVATE
        cxx_std_17
)
