add_subdirectory(acs)

add_library(prometheus_runtime_core STATIC
    adaptive_state.cpp
    architecture_graph.cpp
    architecture_shadow.cpp
    execution_policy.cpp
    proposal_abi.cpp
    service.cpp
    service_context.cpp
    service_manager.cpp
)

target_include_directories(prometheus_runtime_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/core
)

target_link_libraries(prometheus_runtime_core
    PUBLIC
        prometheus_backend_core
        prometheus_acs
    PRIVATE
        Threads::Threads
)

target_compile_features(prometheus_runtime_core PUBLIC cxx_std_17)
