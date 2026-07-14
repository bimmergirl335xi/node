add_library(prometheus_hailo_backend STATIC
    hailo_backend.cpp
)

target_include_directories(prometheus_hailo_backend
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/src/backends"
)

target_compile_features(prometheus_hailo_backend PUBLIC cxx_std_17)
