add_library(
    node_p01_manifest
    STATIC
        src/node_p01_manifest.c
)
target_include_directories(
    node_p01_manifest
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(node_p01_manifest PUBLIC c_std_11)
target_compile_options(node_p01_manifest PRIVATE -Wall -Wextra -Wpedantic -Werror)

set(NODE_P01_ROOT "${PROJECT_BINARY_DIR}/assembly/p01-root")

add_executable(node_p01_init src/node_init.c)
target_link_libraries(node_p01_init PRIVATE node_p01_manifest)
target_compile_options(node_p01_init PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_link_options(node_p01_init PRIVATE -static)
set_target_properties(
    node_p01_init
    PROPERTIES
        OUTPUT_NAME init
        RUNTIME_OUTPUT_DIRECTORY "${NODE_P01_ROOT}"
)

function(node_p01_add_probe target output_name kind)
    add_executable(${target} src/p01_probe.c)
    target_compile_definitions(${target} PRIVATE NODE_P01_PROBE_KIND=${kind})
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_link_options(${target} PRIVATE -static)
    set_target_properties(
        ${target}
        PROPERTIES
            OUTPUT_NAME "${output_name}"
            RUNTIME_OUTPUT_DIRECTORY "${NODE_P01_ROOT}/node/services"
    )
endfunction()

node_p01_add_probe(node_p01_identity_probe identity_probe 1)
node_p01_add_probe(node_p01_volatile_filesystem_probe volatile_filesystem_probe 2)
node_p01_add_probe(node_p01_concurrent_delay_a concurrent_delay_a 3)
node_p01_add_probe(node_p01_concurrent_delay_b concurrent_delay_b 4)
node_p01_add_probe(node_p01_required_semantic_success required_semantic_success 5)
node_p01_add_probe(node_p01_optional_intentional_failure optional_intentional_failure 6)
node_p01_add_probe(node_p01_timeout_probe timeout_probe 7)
node_p01_add_probe(node_p01_signal_termination_probe signal_termination_probe 8)

file(MAKE_DIRECTORY "${NODE_P01_ROOT}/etc/node-p01")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/manifests/p01-public-startup-v1.manifest"
    "${NODE_P01_ROOT}/etc/node-p01/p01-public-startup-v1.manifest"
    COPYONLY
)

if(PROMETHEUS_BUILD_TESTS)
    add_executable(node_p01_manifest_tests tests/test_manifest.c)
    target_link_libraries(node_p01_manifest_tests PRIVATE node_p01_manifest)
    target_compile_options(
        node_p01_manifest_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
    add_test(NAME p01_manifest_contract COMMAND node_p01_manifest_tests)
    add_test(
        NAME p01_supervisor_host
        COMMAND
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_supervisor_tests.sh"
            "${NODE_P01_ROOT}"
            "$<TARGET_FILE:node_p01_init>"
            "${CMAKE_CURRENT_SOURCE_DIR}/manifests/p01-public-startup-v1.manifest"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/required-failure.manifest"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/launch-failure.manifest"
    )
    set_tests_properties(p01_supervisor_host PROPERTIES TIMEOUT 20)
endif()
