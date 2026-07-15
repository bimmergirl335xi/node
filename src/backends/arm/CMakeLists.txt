add_library(prometheus_arm_capabilities STATIC
    arm_capabilities.cpp
    arm_linux_auxv.cpp
    arm_processor_identity.cpp
)

target_include_directories(prometheus_arm_capabilities
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/backends/cpu
)

target_link_libraries(prometheus_arm_capabilities
    PUBLIC
        prometheus_cpu_foundation
)

target_compile_features(prometheus_arm_capabilities
    PUBLIC
        cxx_std_17
)

add_executable(prometheus_arm_capability_probe EXCLUDE_FROM_ALL
    "${PROJECT_SOURCE_DIR}/tools/arm_capability_probe.cpp"
)

target_link_libraries(prometheus_arm_capability_probe
    PRIVATE
        prometheus_arm_capabilities
        prometheus_cpu_foundation
)

target_compile_features(prometheus_arm_capability_probe
    PRIVATE
        cxx_std_17
)
