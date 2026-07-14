add_library(prometheus_backend_core STATIC
    backend_registry.cpp
)

target_include_directories(prometheus_backend_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src/backends
)

target_link_libraries(prometheus_backend_core
    PRIVATE
        Threads::Threads
)

target_compile_features(prometheus_backend_core PUBLIC cxx_std_17)
add_subdirectory(cuda)
add_subdirectory(cpu)
add_subdirectory(arm)
add_subdirectory(accelerator)
