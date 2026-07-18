add_executable(
    test_external_component_manifest
    test_external_component_manifest.c
)
target_link_libraries(
    test_external_component_manifest
    PRIVATE
        node_external_component_manifest
)
target_compile_definitions(
    test_external_component_manifest
    PRIVATE
        NODE_COMPONENT_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)
target_compile_options(
    test_external_component_manifest
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)

add_test(
    NAME external_component_manifest_contract
    COMMAND test_external_component_manifest
)
add_test(
    NAME external_component_manifest_absent
    COMMAND node_component_manifest_validator
)
add_test(
    NAME external_component_manifest_valid_argument
    COMMAND node_component_manifest_validator
        --manifest
        "${PROJECT_SOURCE_DIR}/assembly/manifests/public/example.manifest"
)
add_test(
    NAME external_component_manifest_valid_environment
    COMMAND "${CMAKE_COMMAND}" -E env
        "NODE_EXTERNAL_COMPONENT_MANIFEST=${PROJECT_SOURCE_DIR}/assembly/manifests/public/example.manifest"
        "$<TARGET_FILE:node_component_manifest_validator>"
)
add_test(
    NAME external_component_manifest_malformed_rejected
    COMMAND node_component_manifest_validator
        --manifest
        "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/malformed.manifest"
)
set_tests_properties(
    external_component_manifest_malformed_rejected
    PROPERTIES WILL_FAIL TRUE
)
add_test(
    NAME external_component_manifest_incompatible_rejected
    COMMAND node_component_manifest_validator
        --manifest
        "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/incompatible.manifest"
)
set_tests_properties(
    external_component_manifest_incompatible_rejected
    PROPERTIES WILL_FAIL TRUE
)
add_test(
    NAME external_component_manifest_ambiguous_input_rejected
    COMMAND "${CMAKE_COMMAND}" -E env
        "NODE_EXTERNAL_COMPONENT_MANIFEST=${PROJECT_SOURCE_DIR}/assembly/manifests/public/example.manifest"
        "$<TARGET_FILE:node_component_manifest_validator>"
        --manifest
        "${PROJECT_SOURCE_DIR}/assembly/manifests/public/example.manifest"
)
set_tests_properties(
    external_component_manifest_ambiguous_input_rejected
    PROPERTIES WILL_FAIL TRUE
)
