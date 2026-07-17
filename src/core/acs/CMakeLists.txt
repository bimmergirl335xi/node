add_library(prometheus_acs STATIC
    acs_registry.cpp
    acs_lifecycle.cpp
    acs_admission.cpp
)

target_include_directories(prometheus_acs
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(prometheus_acs PRIVATE Threads::Threads)
target_compile_features(prometheus_acs PUBLIC cxx_std_17)
