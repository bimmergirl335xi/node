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

add_library(
    node_runtime_transition
    STATIC
        src/node_runtime_transition.c
)
target_include_directories(
    node_runtime_transition
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(node_runtime_transition PUBLIC c_std_11)
target_compile_options(
    node_runtime_transition
    PRIVATE -Wall -Wextra -Wpedantic -Werror
)

set(NODE_P01_ROOT "${PROJECT_BINARY_DIR}/assembly/p01-root")
file(MAKE_DIRECTORY "${NODE_P01_ROOT}/node/candidates")

add_executable(node_p01_init src/node_init.c)
target_link_libraries(
    node_p01_init
    PRIVATE node_p01_manifest node_runtime_transition
)
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

add_library(
    node_boot_cpu_assembly_decision
    STATIC
        src/node_cpu_assembly_decision.cpp
)
target_include_directories(
    node_boot_cpu_assembly_decision
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(
    node_boot_cpu_assembly_decision
    PUBLIC
        prometheus_cpu_foundation
)
target_compile_features(node_boot_cpu_assembly_decision PUBLIC cxx_std_17)
target_compile_options(
    node_boot_cpu_assembly_decision
    PRIVATE -Wall -Wextra -Wpedantic -Werror
)

add_executable(
    node_p01_cpu_assembly_decision
    src/cpu_assembly_service.cpp
)
target_link_libraries(
    node_p01_cpu_assembly_decision
    PRIVATE
        node_boot_cpu_assembly_decision
)
target_compile_features(node_p01_cpu_assembly_decision PRIVATE cxx_std_17)
target_compile_options(
    node_p01_cpu_assembly_decision
    PRIVATE -Wall -Wextra -Wpedantic -Werror
)
target_link_options(node_p01_cpu_assembly_decision PRIVATE -static)
set_target_properties(
    node_p01_cpu_assembly_decision
    PROPERTIES
        OUTPUT_NAME cpu_assembly_decision
        RUNTIME_OUTPUT_DIRECTORY "${NODE_P01_ROOT}/node/services"
)

add_library(
    node_boot_cpu_runtime_assembly
    STATIC
        src/node_cpu_runtime_assembly.cpp
)
target_include_directories(
    node_boot_cpu_runtime_assembly
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(node_boot_cpu_runtime_assembly PUBLIC cxx_std_17)
target_compile_options(
    node_boot_cpu_runtime_assembly
    PRIVATE -Wall -Wextra -Wpedantic -Werror
)

add_executable(
    node_p01_cpu_runtime_candidate
    src/cpu_runtime_candidate.cpp
)
target_link_libraries(
    node_p01_cpu_runtime_candidate
    PRIVATE
        prometheus_cpu_thread_pool
        Threads::Threads
)
target_compile_features(node_p01_cpu_runtime_candidate PRIVATE cxx_std_17)
target_compile_options(
    node_p01_cpu_runtime_candidate
    PRIVATE -mavx2 -fno-pie -Wall -Wextra -Wpedantic -Werror
)
target_link_options(node_p01_cpu_runtime_candidate PRIVATE -static -no-pie)
set_target_properties(
    node_p01_cpu_runtime_candidate
    PROPERTIES
        OUTPUT_NAME cpu-runtime-x86_64-avx2
        RUNTIME_OUTPUT_DIRECTORY "${NODE_P01_ROOT}/node/candidates"
)
set(NODE_CPU_RUNTIME_PROVENANCE
    "${NODE_P01_ROOT}/node/candidates/cpu-runtime-x86_64-avx2.provenance"
)
add_custom_command(
    OUTPUT "${NODE_CPU_RUNTIME_PROVENANCE}"
    COMMAND
        "${CMAKE_COMMAND}"
        -DINPUT=$<TARGET_FILE:node_p01_cpu_runtime_candidate>
        -DOUTPUT=${NODE_CPU_RUNTIME_PROVENANCE}
        -DTOOLCHAIN_IDENTITY=${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateCpuRuntimeProvenance.cmake"
    DEPENDS
        node_p01_cpu_runtime_candidate
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateCpuRuntimeProvenance.cmake"
    VERBATIM
)
add_custom_target(
    node_p01_cpu_runtime_provenance ALL
    DEPENDS "${NODE_CPU_RUNTIME_PROVENANCE}"
)

add_executable(
    node_p01_cpu_runtime_assembly
    src/cpu_runtime_assembly_service.cpp
)
target_link_libraries(
    node_p01_cpu_runtime_assembly
    PRIVATE
        node_boot_cpu_runtime_assembly
)
target_compile_features(node_p01_cpu_runtime_assembly PRIVATE cxx_std_17)
target_compile_options(
    node_p01_cpu_runtime_assembly
    PRIVATE -Wall -Wextra -Wpedantic -Werror
)
target_link_options(node_p01_cpu_runtime_assembly PRIVATE -static)
set_target_properties(
    node_p01_cpu_runtime_assembly
    PROPERTIES
        OUTPUT_NAME cpu_runtime_assembly
        RUNTIME_OUTPUT_DIRECTORY "${NODE_P01_ROOT}/node/services"
)

add_executable(
    node_p01_acs_reference_transport
    src/acs_reference_service.cpp
)
target_link_libraries(
    node_p01_acs_reference_transport
    PRIVATE
        prometheus_acs
        Threads::Threads
)
target_compile_features(node_p01_acs_reference_transport PRIVATE cxx_std_17)
target_compile_options(
    node_p01_acs_reference_transport
    PRIVATE -Wall -Wextra -Wpedantic -Werror
)
target_link_options(node_p01_acs_reference_transport PRIVATE -static)
set_target_properties(
    node_p01_acs_reference_transport
    PROPERTIES
        OUTPUT_NAME acs_reference_transport
        RUNTIME_OUTPUT_DIRECTORY "${NODE_P01_ROOT}/node/services"
)

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
    add_executable(
        node_runtime_transition_tests
        tests/test_runtime_transition.c
    )
    target_link_libraries(
        node_runtime_transition_tests
        PRIVATE node_runtime_transition
    )
    target_compile_options(
        node_runtime_transition_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
    add_test(
        NAME p01_runtime_transition
        COMMAND node_runtime_transition_tests
    )
    add_executable(
        node_cpu_assembly_decision_tests
        tests/test_cpu_assembly_decision.cpp
    )
    target_link_libraries(
        node_cpu_assembly_decision_tests
        PRIVATE node_boot_cpu_assembly_decision
    )
    target_compile_options(
        node_cpu_assembly_decision_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
    add_test(
        NAME p01_cpu_assembly_decision
        COMMAND node_cpu_assembly_decision_tests
    )
    add_executable(
        node_cpu_runtime_assembly_tests
        tests/test_cpu_runtime_assembly.cpp
    )
    target_link_libraries(
        node_cpu_runtime_assembly_tests
        PRIVATE node_boot_cpu_runtime_assembly
    )
    target_compile_options(
        node_cpu_runtime_assembly_tests
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
    add_test(
        NAME p01_cpu_runtime_assembly
        COMMAND node_cpu_runtime_assembly_tests
                "$<TARGET_FILE:node_p01_cpu_runtime_candidate>"
    )
    add_test(
        NAME p01_cpu_runtime_candidate_probe
        COMMAND "$<TARGET_FILE:node_p01_cpu_runtime_candidate>" --serve-probe 1
    )
    add_test(
        NAME p01_cpu_runtime_candidate_multi_worker_probe
        COMMAND "$<TARGET_FILE:node_p01_cpu_runtime_candidate>" --serve-probe 4
    )
    set_tests_properties(
        p01_cpu_runtime_candidate_multi_worker_probe
        PROPERTIES
            PASS_REGULAR_EXPRESSION "NODE_CPU_RUNTIME_PROBE 480"
            TIMEOUT 3
    )
    set_tests_properties(
        p01_cpu_runtime_candidate_probe
        PROPERTIES
            PASS_REGULAR_EXPRESSION "NODE_CPU_RUNTIME_PROBE 120"
            TIMEOUT 3
    )
    add_dependencies(
        node_cpu_runtime_assembly_tests
        node_p01_cpu_runtime_provenance
    )
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
