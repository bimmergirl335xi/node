add_library(prometheus_linux_pci_inventory STATIC
    linux_pci_inventory.cpp
)

target_include_directories(prometheus_linux_pci_inventory
    PUBLIC
        "${PROJECT_SOURCE_DIR}/src/hardware"
)

target_compile_features(prometheus_linux_pci_inventory PUBLIC cxx_std_17)

add_executable(node_gpu_hardware_probe
    "${PROJECT_SOURCE_DIR}/tools/node_gpu_hardware_probe.cpp"
)

target_link_libraries(node_gpu_hardware_probe
    PRIVATE
        prometheus_linux_pci_inventory
)

target_compile_features(node_gpu_hardware_probe PRIVATE cxx_std_17)
