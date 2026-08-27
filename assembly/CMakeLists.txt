add_library(
    node_external_component_manifest
    STATIC
        providers/external_component_manifest.c
)
target_include_directories(
    node_external_component_manifest
    PUBLIC
        "${PROJECT_SOURCE_DIR}/interfaces"
        "${CMAKE_CURRENT_SOURCE_DIR}/providers"
)
target_compile_features(node_external_component_manifest PUBLIC c_std_11)
target_compile_options(
    node_external_component_manifest
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)

add_executable(
    node_component_manifest_validator
    providers/manifest_validator_main.c
)
target_link_libraries(
    node_component_manifest_validator
    PRIVATE
        node_external_component_manifest
)
target_compile_options(
    node_component_manifest_validator
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)

add_subdirectory(micro_os)

if(PROMETHEUS_BUILD_TESTS)
    add_test(
        NAME p01_device_write_prohibition
        COMMAND
            "${CMAKE_CURRENT_SOURCE_DIR}/p01_boot/tests/test-device-prohibition.sh"
            "${CMAKE_CURRENT_SOURCE_DIR}/p01_boot/scripts/lib-p01.sh"
    )
endif()

if(PROMETHEUS_BUILD_TESTS)
    add_subdirectory(tests)
endif()
