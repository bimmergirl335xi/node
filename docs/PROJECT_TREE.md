# Node — Tracked Project Tree

This is a compact view of the tracked repository after the 2026-07-19 branch
policy reconciliation. Generated build directories and ignored runtime
artifacts are omitted. Directories not shown must not be inferred to exist.

```text
node/
├── .gitignore
├── README.md
├── AI_CONTEXT.md
├── ARCHITECTURE.md
├── CHANGELOG.md
├── CMakeLists.txt
├── CONTRIBUTING.md
├── LICENSE
├── PATCH_NOTES.md
├── PROTOCOL.md
├── SECURITY.md
├── THREAT_MODEL.md
├── TIME.md
├── cmake/
├── config/
├── include/
├── legacy/
├── manifests/
├── models/
├── python/
├── schemas/
├── scripts/
├── simulator/
├── systemd/
├── third_party/
├── tools/
├── udev/
├── benchmarks/
├── docs/
│   ├── CURRENT_STATE.md
│   ├── BRANCH_POLICY.md
│   ├── HANDOFF_AFTER_PHASE_5.md
│   ├── PROJECT_TREE.md
│   ├── architecture/
│   │   ├── node_runtime.md
│   │   ├── service_model.md
│   │   ├── distributed_mesh.md
│   │   ├── proximity_gradient.md
│   │   ├── cognitive_services.md
│   │   ├── data_flow.md
│   │   ├── acs/
│   │   │   ├── README.md
│   │   │   ├── ACS-0000-charter.md
│   │   │   ├── ACS-0001-core-principles.md
│   │   │   ├── ACS-0002-relationship-classes.md
│   │   │   ├── ACS-0003-signal-taxonomy.md
│   │   │   ├── ACS-0004-endpoints-and-ports.md
│   │   │   ├── ACS-0005-connection-lifecycle.md
│   │   │   ├── ACS-0006-admission-and-budgets.md
│   │   │   ├── ACS-0007-security-and-trust.md
│   │   │   ├── ACS-0008-immune-integration.md
│   │   │   └── ACS-0009-runtime-integration.md
│   │   ├── boot/
│   │   ├── kernel/
│   │   ├── memory/
│   │   │   ├── README.md
│   │   │   ├── MEM-0000-charter.md
│   │   │   ├── MEM-0001-core-principles.md
│   │   │   ├── MEM-0002-memory-roles.md
│   │   │   ├── MEM-0003-identity-and-versioning.md
│   │   │   ├── MEM-0004-operation-contracts.md
│   │   │   ├── MEM-0005-availability-and-consistency.md
│   │   │   ├── MEM-0006-retention-and-lifecycle.md
│   │   │   ├── MEM-0007-distributed-custody.md
│   │   │   ├── MEM-0008-recovery-and-reconstruction.md
│   │   │   ├── MEM-0009-acs-integration.md
│   │   │   └── MEM-0010-conformance.md
│   │   └── immune/
│   │       ├── README.md
│   │       ├── IMM-0000-charter-and-scope.md
│   │       └── IMM-0001-core-invariants.md
│   ├── development/
│   ├── handoffs/
│   ├── hardware/
│   ├── migration/
│   ├── protocols/
│   └── security/
├── src/
│   ├── backends/
│   │   ├── CMakeLists.txt
│   │   ├── compute_backend.hpp
│   │   ├── backend_registry.hpp
│   │   ├── backend_registry.cpp
│   │   ├── cpu/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── cpu_backend.hpp
│   │   │   ├── cpu_backend.cpp
│   │   │   ├── cpu_identity.hpp
│   │   │   ├── cpu_identity.cpp
│   │   │   ├── cpu_topology.hpp
│   │   │   ├── cpu_topology.cpp
│   │   │   ├── cpu_capabilities.hpp
│   │   │   ├── cpu_capabilities.cpp
│   │   │   ├── cpu_health.hpp
│   │   │   ├── cpu_health.cpp
│   │   │   ├── cpu_capacity.hpp
│   │   │   ├── cpu_capacity.cpp
│   │   │   ├── cpu_thread_pool.hpp
│   │   │   ├── cpu_thread_pool.cpp
│   │   │   ├── simd_dispatch.hpp
│   │   │   └── simd_dispatch.cpp
│   │   ├── arm/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── arm_capabilities.hpp
│   │   │   ├── arm_capabilities.cpp
│   │   │   ├── arm_linux_auxv.hpp
│   │   │   ├── arm_linux_auxv.cpp
│   │   │   ├── arm_processor_identity.hpp
│   │   │   ├── arm_processor_identity.cpp
│   │   │   ├── neon_dispatch.hpp
│   │   │   └── neon_dispatch.cpp
│   │   ├── cuda/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── cuda_backend.hpp
│   │   │   ├── cuda_backend.cu
│   │   │   ├── cuda_capabilities.hpp
│   │   │   ├── cuda_capabilities.cu
│   │   │   ├── cuda_device.hpp
│   │   │   ├── cuda_device.cu
│   │   │   ├── cuda_device_pool.hpp
│   │   │   ├── cuda_device_pool.cu
│   │   │   ├── cuda_health.hpp
│   │   │   ├── cuda_health.cu
│   │   │   ├── cuda_job_queue.hpp
│   │   │   ├── cuda_job_queue.cu
│   │   │   ├── cuda_kernel_registry.hpp
│   │   │   ├── cuda_kernel_registry.cu
│   │   │   ├── cuda_memory_pool.hpp
│   │   │   ├── cuda_memory_pool.cu
│   │   │   ├── cuda_profiles.hpp
│   │   │   ├── cuda_profiles.cpp
│   │   │   ├── cuda_runtime_compilation.hpp
│   │   │   └── cuda_runtime_compilation.cpp
│   │   ├── accelerator/
│   │   └── storage/
│   ├── core/
│   │   ├── CMakeLists.txt
│   │   ├── acs/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── acs_types.hpp
│   │   │   ├── acs_registry.hpp
│   │   │   ├── acs_registry.cpp
│   │   │   ├── acs_lifecycle.hpp
│   │   │   ├── acs_lifecycle.cpp
│   │   │   ├── acs_admission.hpp
│   │   │   └── acs_admission.cpp
│   │   ├── adaptive_state.hpp
│   │   ├── adaptive_state.cpp
│   │   ├── architecture_graph.hpp
│   │   ├── architecture_graph.cpp
│   │   ├── architecture_shadow.hpp
│   │   ├── architecture_shadow.cpp
│   │   ├── execution_policy.hpp
│   │   ├── execution_policy.cpp
│   │   ├── proposal_abi.h
│   │   ├── proposal_abi.cpp
│   │   ├── service.hpp
│   │   ├── service.cpp
│   │   ├── service_context.hpp
│   │   ├── service_context.cpp
│   │   ├── service_manager.hpp
│   │   ├── service_manager.cpp
│   │   ├── capability_registry.hpp
│   │   ├── capability_registry.cpp
│   │   ├── configuration.hpp
│   │   ├── configuration.cpp
│   │   ├── health_monitor.hpp
│   │   ├── health_monitor.cpp
│   │   ├── node_identity.hpp
│   │   ├── node_identity.cpp
│   │   ├── node_runtime.hpp
│   │   ├── node_runtime.cpp
│   │   ├── resource_manager.hpp
│   │   ├── resource_manager.cpp
│   │   ├── shutdown_manager.hpp
│   │   ├── shutdown_manager.cpp
│   │   ├── task_scheduler.hpp
│   │   └── task_scheduler.cpp
│   ├── kernels/
│   ├── diagnostics/
│   ├── experiments/
│   ├── learning/
│   ├── memory/
│   ├── mesh/
│   ├── messaging/
│   ├── models/
│   ├── protocol/
│   ├── provisioning/
│   ├── runtime/
│   ├── security/
│   ├── services/
│   ├── storage/
│   └── time/
└── tests/
    ├── integration/
    └── unit/
        ├── backends/
        │   ├── CMakeLists.txt
        │   ├── test_cpu_foundation.cpp
        │   ├── test_cpu_backend_registry.cpp
        │   ├── test_cpu_health_capacity.cpp
        │   ├── test_cpu_thread_pool.cpp
        │   ├── test_cpu_simd_dispatch.cpp
        │   ├── test_arm_capabilities.cpp
        │   ├── test_arm_linux_auxv.cpp
        │   ├── test_arm_processor_identity.cpp
        │   ├── test_cuda_backend_registry.cu
        │   ├── test_cuda_runtime_resources.cu
        │   ├── test_cuda_runtime_compilation.cpp
        │   └── test_hailo_backend.cpp
        ├── core/
        │   ├── CMakeLists.txt
        │   ├── acs_test_fixture.hpp
        │   ├── test_acs_types.cpp
        │   ├── test_acs_registry.cpp
        │   ├── test_acs_lifecycle.cpp
        │   ├── test_acs_admission.cpp
        │   ├── test_acs_concurrency.cpp
        │   ├── test_service_lifecycle.cpp
        │   ├── test_execution_policy.cpp
        │   ├── test_adaptive_state.cpp
        │   ├── test_architecture_graph.cpp
        │   ├── test_architecture_shadow.cpp
        │   └── test_proposal_abi.cpp
        ├── kernels/
        └── protocol/
```

There is no tracked `docs/architecture/bootstrap/` directory in ACS-R001. The
presence of placeholder source elsewhere in the tree does not imply completed
implementation.
