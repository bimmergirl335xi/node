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
