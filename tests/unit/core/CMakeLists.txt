add_executable(test_service_lifecycle
    test_service_lifecycle.cpp
)

target_link_libraries(test_service_lifecycle
    PRIVATE
        prometheus_runtime_core
)

target_compile_features(test_service_lifecycle PRIVATE cxx_std_17)

add_test(
    NAME prometheus_service_lifecycle
    COMMAND test_service_lifecycle
)

add_executable(test_execution_policy
    test_execution_policy.cpp
)

target_link_libraries(test_execution_policy
    PRIVATE
        prometheus_runtime_core
)

target_compile_features(test_execution_policy PRIVATE cxx_std_17)

add_test(
    NAME prometheus_execution_policy
    COMMAND test_execution_policy
)

add_executable(test_adaptive_state
    test_adaptive_state.cpp
)

target_link_libraries(test_adaptive_state
    PRIVATE
        prometheus_runtime_core
)

target_compile_features(test_adaptive_state PRIVATE cxx_std_17)

add_test(
    NAME prometheus_adaptive_state
    COMMAND test_adaptive_state
)

add_executable(test_architecture_graph
    test_architecture_graph.cpp
)

target_link_libraries(test_architecture_graph
    PRIVATE
        prometheus_runtime_core
)

target_compile_features(test_architecture_graph PRIVATE cxx_std_17)

add_test(
    NAME prometheus_architecture_graph
    COMMAND test_architecture_graph
)

add_executable(test_architecture_shadow
    test_architecture_shadow.cpp
)

target_link_libraries(test_architecture_shadow
    PRIVATE
        prometheus_runtime_core
)

target_compile_features(test_architecture_shadow PRIVATE cxx_std_17)

add_test(
    NAME prometheus_architecture_shadow
    COMMAND test_architecture_shadow
)

add_executable(test_proposal_abi
    test_proposal_abi.cpp
)

target_link_libraries(test_proposal_abi
    PRIVATE
        prometheus_runtime_core
)

target_compile_features(test_proposal_abi PRIVATE cxx_std_17)

add_test(
    NAME prometheus_proposal_abi
    COMMAND test_proposal_abi
)

add_executable(test_acs_types test_acs_types.cpp)
target_link_libraries(test_acs_types PRIVATE prometheus_acs)
target_compile_features(test_acs_types PRIVATE cxx_std_17)
add_test(NAME prometheus_acs_types COMMAND test_acs_types)

add_executable(test_acs_registry test_acs_registry.cpp)
target_link_libraries(test_acs_registry PRIVATE prometheus_acs)
target_compile_features(test_acs_registry PRIVATE cxx_std_17)
add_test(NAME prometheus_acs_registry COMMAND test_acs_registry)

add_executable(test_acs_lifecycle test_acs_lifecycle.cpp)
target_link_libraries(test_acs_lifecycle PRIVATE prometheus_acs)
target_compile_features(test_acs_lifecycle PRIVATE cxx_std_17)
add_test(NAME prometheus_acs_lifecycle COMMAND test_acs_lifecycle)

add_executable(test_acs_concurrency test_acs_concurrency.cpp)
target_link_libraries(test_acs_concurrency PRIVATE prometheus_acs)
target_compile_features(test_acs_concurrency PRIVATE cxx_std_17)
add_test(NAME prometheus_acs_concurrency COMMAND test_acs_concurrency)

add_executable(test_acs_admission test_acs_admission.cpp)
target_link_libraries(test_acs_admission PRIVATE prometheus_acs)
target_compile_features(test_acs_admission PRIVATE cxx_std_17)
add_test(NAME prometheus_acs_admission COMMAND test_acs_admission)
