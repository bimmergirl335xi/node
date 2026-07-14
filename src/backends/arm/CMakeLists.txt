add_library(prometheus_arm_capabilities STATIC
    arm_capabilities.cpp
    arm_linux_auxv.cpp
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
