add_executable(test_linux_pci_inventory
    test_linux_pci_inventory.cpp
)

target_link_libraries(test_linux_pci_inventory
    PRIVATE
        prometheus_linux_pci_inventory
)

target_compile_features(test_linux_pci_inventory PRIVATE cxx_std_17)

add_test(
    NAME prometheus_linux_pci_inventory
    COMMAND test_linux_pci_inventory
)

add_executable(test_gpu_evidence_correlation
    test_gpu_evidence_correlation.cpp
)

target_link_libraries(test_gpu_evidence_correlation
    PRIVATE
        prometheus_gpu_evidence_correlation
)

target_compile_features(test_gpu_evidence_correlation PRIVATE cxx_std_17)

add_test(
    NAME prometheus_gpu_evidence_correlation
    COMMAND test_gpu_evidence_correlation
)
