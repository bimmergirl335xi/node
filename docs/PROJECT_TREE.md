# Prometheus Node — Project File Tree

```text
prometheus_node/
├── README.md
├── ARCHITECTURE.md
├── PROTOCOL.md
├── TIME.md
├── SECURITY.md
├── THREAT_MODEL.md
├── CONTRIBUTING.md
├── CHANGELOG.md
├── LICENSE
├── CMakeLists.txt
├── pyproject.toml
├── requirements-lock.txt
│
├── cmake/
│   ├── CompilerOptions.cmake
│   ├── CudaArchitectures.cmake
│   ├── Dependencies.cmake
│   ├── Sanitizers.cmake
│   └── VersionInfo.cmake
│
├── config/
│   ├── node.example.yaml
│   ├── mesh.example.yaml
│   ├── security.example.yaml
│   ├── services.example.yaml
│   ├── logging.example.yaml
│   └── hardware_profiles/
│       ├── generic_cpu.yaml
│       ├── raspberry_pi.yaml
│       ├── nvidia_pascal.yaml
│       ├── nvidia_volta.yaml
│       ├── nvidia_modern.yaml
│       ├── robot_vision_node.yaml
│       ├── robot_body_node.yaml
│       └── nexus_thin_client.yaml
│
├── include/
│   └── prometheus/
│       ├── version.hpp
│       ├── result.hpp
│       ├── identifiers.hpp
│       ├── service.hpp
│       ├── message.hpp
│       ├── capability.hpp
│       ├── permissions.hpp
│       └── timestamps.hpp
│
├── src/
│   ├── main.cpp
│   │
│   ├── core/
│   │   ├── node_identity.hpp
│   │   ├── node_identity.cpp
│   │   ├── node_runtime.hpp
│   │   ├── node_runtime.cpp
│   │   ├── service.hpp
│   │   ├── service.cpp
│   │   ├── service_context.hpp
│   │   ├── service_context.cpp
│   │   ├── service_manager.hpp
│   │   ├── service_manager.cpp
│   │   ├── capability_registry.hpp
│   │   ├── capability_registry.cpp
│   │   ├── task_scheduler.hpp
│   │   ├── task_scheduler.cpp
│   │   ├── resource_manager.hpp
│   │   ├── resource_manager.cpp
│   │   ├── health_monitor.hpp
│   │   ├── health_monitor.cpp
│   │   ├── shutdown_manager.hpp
│   │   ├── shutdown_manager.cpp
│   │   ├── configuration.hpp
│   │   └── configuration.cpp
│   │
│   ├── protocol/
│   │   ├── robot_protocol.hpp
│   │   ├── mesh_protocol.hpp
│   │   ├── diagnostic_protocol.hpp
│   │   ├── provisioning_protocol.hpp
│   │   ├── model_protocol.hpp
│   │   ├── protocol_version.hpp
│   │   ├── checksum.hpp
│   │   ├── checksum.cpp
│   │   ├── serializer.hpp
│   │   └── serializer.cpp
│   │
│   ├── time/
│   │   ├── pico_clock.hpp
│   │   ├── pico_clock.cpp
│   │   ├── clock_alignment.hpp
│   │   ├── clock_alignment.cpp
│   │   ├── clock_quality.hpp
│   │   ├── clock_quality.cpp
│   │   ├── holdover_clock.hpp
│   │   └── holdover_clock.cpp
│   │
│   ├── messaging/
│   │   ├── local_message_bus.hpp
│   │   ├── local_message_bus.cpp
│   │   ├── message_queue.hpp
│   │   ├── message_queue.cpp
│   │   ├── subscription.hpp
│   │   ├── subscription.cpp
│   │   ├── topic_registry.hpp
│   │   ├── topic_registry.cpp
│   │   ├── shared_memory_channel.hpp
│   │   ├── shared_memory_channel.cpp
│   │   ├── unix_socket_channel.hpp
│   │   └── unix_socket_channel.cpp
│   │
│   ├── mesh/
│   │   ├── mesh_service.hpp
│   │   ├── mesh_service.cpp
│   │   ├── peer_identity.hpp
│   │   ├── peer_identity.cpp
│   │   ├── peer_table.hpp
│   │   ├── peer_table.cpp
│   │   ├── discovery.hpp
│   │   ├── discovery.cpp
│   │   ├── membership.hpp
│   │   ├── membership.cpp
│   │   ├── transport.hpp
│   │   ├── transport.cpp
│   │   ├── connection_manager.hpp
│   │   ├── connection_manager.cpp
│   │   ├── link_metrics.hpp
│   │   ├── link_metrics.cpp
│   │   ├── proximity_graph.hpp
│   │   ├── proximity_graph.cpp
│   │   ├── diffusion_policy.hpp
│   │   ├── diffusion_policy.cpp
│   │   ├── route_selector.hpp
│   │   ├── route_selector.cpp
│   │   ├── subscriptions.hpp
│   │   ├── subscriptions.cpp
│   │   ├── anti_entropy.hpp
│   │   ├── anti_entropy.cpp
│   │   ├── replication.hpp
│   │   ├── replication.cpp
│   │   ├── lease_manager.hpp
│   │   ├── lease_manager.cpp
│   │   ├── task_router.hpp
│   │   └── task_router.cpp
│   │
│   ├── backends/
│   │   ├── compute_backend.hpp
│   │   ├── backend_registry.hpp
│   │   ├── backend_registry.cpp
│   │   │
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
│   │   │
│   │   ├── cuda/
│   │   │   ├── cuda_backend.hpp
│   │   │   ├── cuda_backend.cu
│   │   │   ├── cuda_device.hpp
│   │   │   ├── cuda_device.cu
│   │   │   ├── cuda_device_pool.hpp
│   │   │   ├── cuda_device_pool.cu
│   │   │   ├── cuda_capabilities.hpp
│   │   │   ├── cuda_capabilities.cu
│   │   │   ├── cuda_memory_pool.hpp
│   │   │   ├── cuda_memory_pool.cu
│   │   │   ├── cuda_job_queue.hpp
│   │   │   ├── cuda_job_queue.cu
│   │   │   ├── cuda_kernel_registry.hpp
│   │   │   ├── cuda_kernel_registry.cu
│   │   │   ├── cuda_profiles.hpp
│   │   │   └── cuda_profiles.cpp
│   │   │
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
│   │   │
│   │   ├── storage/
│   │   │   ├── storage_backend.hpp
│   │   │   └── storage_backend.cpp
│   │   │
│   │   └── accelerator/
│   │       ├── CMakeLists.txt
│   │       ├── accelerator_backend.hpp
│   │       ├── hailo_backend.hpp
│   │       ├── hailo_backend.cpp
│   │       └── imx500_backend.cpp
│   │
│   ├── services/
│   │   ├── robot_ingress/
│   │   │   ├── robot_ingress_service.hpp
│   │   │   ├── robot_ingress_service.cpp
│   │   │   ├── pi_connection.hpp
│   │   │   ├── pi_connection.cpp
│   │   │   ├── tensor_receiver.hpp
│   │   │   └── tensor_receiver.cpp
│   │   │
│   │   ├── giga_telemetry/
│   │   │   ├── giga_telemetry_service.hpp
│   │   │   ├── giga_telemetry_service.cpp
│   │   │   ├── giga_link_state.hpp
│   │   │   └── giga_link_state.cpp
│   │   │
│   │   ├── frame_assembler/
│   │   │   ├── frame_assembler_service.hpp
│   │   │   ├── frame_assembler_service.cpp
│   │   │   ├── frame_assembly.hpp
│   │   │   └── frame_assembly.cpp
│   │   │
│   │   ├── visual_perception/
│   │   │   ├── visual_perception_service.hpp
│   │   │   ├── visual_perception_service.cu
│   │   │   ├── visual_perception_worker.hpp
│   │   │   ├── visual_perception_worker.cu
│   │   │   ├── perception_result.hpp
│   │   │   ├── stream_assignment.hpp
│   │   │   └── stream_assignment.cpp
│   │   │
│   │   ├── temporal_prediction/
│   │   │   ├── temporal_prediction_service.hpp
│   │   │   ├── temporal_prediction_service.cu
│   │   │   ├── temporal_state.hpp
│   │   │   └── temporal_state.cpp
│   │   │
│   │   ├── experience_recorder/
│   │   │   ├── experience_recorder_service.hpp
│   │   │   ├── experience_recorder_service.cpp
│   │   │   ├── replay_ring.hpp
│   │   │   └── replay_ring.cpp
│   │   │
│   │   ├── maintenance_port/
│   │   │   ├── maintenance_port_service.hpp
│   │   │   ├── maintenance_port_service.cpp
│   │   │   ├── analyzer_session.hpp
│   │   │   └── analyzer_session.cpp
│   │   │
│   │   └── node_observer/
│   │       ├── node_observer_service.hpp
│   │       └── node_observer_service.cpp
│   │
│   ├── models/
│   │   ├── model.hpp
│   │   ├── model.cpp
│   │   ├── model_manifest.hpp
│   │   ├── model_manifest.cpp
│   │   ├── model_loader.hpp
│   │   ├── model_loader.cpp
│   │   ├── model_registry.hpp
│   │   ├── model_registry.cpp
│   │   ├── model_version.hpp
│   │   ├── model_version.cpp
│   │   ├── model_delta.hpp
│   │   └── model_delta.cpp
│   │
│   ├── learning/
│   │   ├── learning_mode.hpp
│   │   ├── online_plasticity.hpp
│   │   ├── online_plasticity.cu
│   │   ├── predictive_learning.hpp
│   │   ├── predictive_learning.cu
│   │   ├── weight_consolidation.hpp
│   │   ├── weight_consolidation.cu
│   │   ├── candidate_model.hpp
│   │   ├── candidate_model.cpp
│   │   ├── model_evaluator.hpp
│   │   └── model_evaluator.cpp
│   │
│   ├── kernels/
│   │   ├── dense_kernels.cuh
│   │   ├── dense_kernels.cu
│   │   ├── activation_kernels.cuh
│   │   ├── activation_kernels.cu
│   │   ├── normalization_kernels.cuh
│   │   ├── normalization_kernels.cu
│   │   ├── temporal_kernels.cuh
│   │   ├── temporal_kernels.cu
│   │   ├── learning_kernels.cuh
│   │   ├── learning_kernels.cu
│   │   ├── reduction_kernels.cuh
│   │   └── reduction_kernels.cu
│   │
│   ├── memory/
│   │   ├── working_memory.hpp
│   │   ├── working_memory.cpp
│   │   ├── episodic_memory.hpp
│   │   ├── episodic_memory.cpp
│   │   ├── semantic_memory.hpp
│   │   ├── semantic_memory.cpp
│   │   ├── procedural_memory.hpp
│   │   ├── procedural_memory.cpp
│   │   ├── memory_object.hpp
│   │   ├── memory_object.cpp
│   │   ├── memory_index.hpp
│   │   ├── memory_index.cpp
│   │   ├── memory_replication.hpp
│   │   └── memory_replication.cpp
│   │
│   ├── storage/
│   │   ├── object_store.hpp
│   │   ├── object_store.cpp
│   │   ├── content_address.hpp
│   │   ├── content_address.cpp
│   │   ├── checkpoint_store.hpp
│   │   ├── checkpoint_store.cpp
│   │   ├── replay_store.hpp
│   │   ├── replay_store.cpp
│   │   ├── model_store.hpp
│   │   ├── model_store.cpp
│   │   ├── metadata_store.hpp
│   │   └── metadata_store.cpp
│   │
│   ├── diagnostics/
│   │   ├── log_bus.hpp
│   │   ├── log_bus.cpp
│   │   ├── log_record.hpp
│   │   ├── log_record.cpp
│   │   ├── log_sink.hpp
│   │   ├── log_sink.cpp
│   │   ├── audit_log.hpp
│   │   ├── audit_log.cpp
│   │   ├── metrics.hpp
│   │   ├── metrics.cpp
│   │   ├── fault_report.hpp
│   │   ├── fault_report.cpp
│   │   ├── serial_channel.hpp
│   │   └── serial_channel.cpp
│   │
│   ├── security/
│   │   ├── operator_identity.hpp
│   │   ├── operator_identity.cpp
│   │   ├── node_identity_store.hpp
│   │   ├── node_identity_store.cpp
│   │   ├── service_identity.hpp
│   │   ├── service_identity.cpp
│   │   ├── trust_store.hpp
│   │   ├── trust_store.cpp
│   │   ├── certificate_store.hpp
│   │   ├── certificate_store.cpp
│   │   ├── authorization.hpp
│   │   ├── authorization.cpp
│   │   ├── permission_manifest.hpp
│   │   ├── permission_manifest.cpp
│   │   ├── replay_protection.hpp
│   │   ├── replay_protection.cpp
│   │   ├── artifact_verifier.hpp
│   │   └── artifact_verifier.cpp
│   │
│   ├── runtime/
│   │   ├── script_runtime.hpp
│   │   ├── script_runtime.cpp
│   │   ├── script_manifest.hpp
│   │   ├── script_manifest.cpp
│   │   ├── python_host.hpp
│   │   ├── python_host.cpp
│   │   ├── python_service.hpp
│   │   ├── python_service.cpp
│   │   ├── process_supervisor.hpp
│   │   └── process_supervisor.cpp
│   │
│   ├── provisioning/
│   │   ├── release_manifest.hpp
│   │   ├── release_manifest.cpp
│   │   ├── hardware_probe.hpp
│   │   ├── hardware_probe.cpp
│   │   ├── profile_selector.hpp
│   │   ├── profile_selector.cpp
│   │   ├── package_selector.hpp
│   │   ├── package_selector.cpp
│   │   ├── installer.hpp
│   │   ├── installer.cpp
│   │   ├── rollback.hpp
│   │   ├── rollback.cpp
│   │   ├── system_slot.hpp
│   │   └── system_slot.cpp
│   │
│   └── experiments/
│       ├── neural_mirror_legacy.hpp
│       ├── neural_mirror_legacy.cpp
│       ├── fungal_diffusion.hpp
│       └── fungal_diffusion.cpp
│
├── python/
│   ├── prometheus/
│   │   ├── __init__.py
│   │   ├── node.py
│   │   ├── service.py
│   │   ├── messages.py
│   │   ├── topics.py
│   │   ├── tensors.py
│   │   ├── time.py
│   │   ├── capabilities.py
│   │   ├── logging.py
│   │   ├── storage.py
│   │   ├── tasks.py
│   │   ├── permissions.py
│   │   └── diagnostics.py
│   │
│   ├── services/
│   │   ├── example_service.py
│   │   ├── imu_analyzer.py
│   │   ├── sensor_adapter.py
│   │   └── node_diagnostics.py
│   │
│   └── tests/
│       ├── test_messages.py
│       ├── test_service.py
│       └── test_tensors.py
│
├── schemas/
│   ├── mesh_message.schema.json
│   ├── capability_manifest.schema.json
│   ├── service_manifest.schema.json
│   ├── script_manifest.schema.json
│   ├── model_manifest.schema.json
│   ├── release_manifest.schema.json
│   ├── node_configuration.schema.json
│   ├── diagnostic_message.schema.json
│   └── robot_packet_layout.md
│
├── manifests/
│   ├── services/
│   │   ├── robot_ingress.yaml
│   │   ├── giga_telemetry.yaml
│   │   ├── frame_assembler.yaml
│   │   ├── visual_perception.yaml
│   │   ├── temporal_prediction.yaml
│   │   ├── experience_recorder.yaml
│   │   └── maintenance_port.yaml
│   │
│   ├── models/
│   │   ├── visual_perception_v1.yaml
│   │   └── temporal_predictor_v1.yaml
│   │
│   └── permissions/
│       ├── default_node.yaml
│       ├── robot_vision_node.yaml
│       ├── robot_body_node.yaml
│       ├── nexus_node.yaml
│       └── pocket_decoder.yaml
│
├── models/
│   ├── README.md
│   ├── approved/
│   ├── candidates/
│   ├── archived/
│   └── test_fixtures/
│
├── runtime/
│   ├── python/
│   ├── wheelhouse/
│   ├── cuda_legacy/
│   ├── cuda_modern/
│   ├── cpu/
│   └── recovery/
│
├── provisioning/
│   ├── forge/
│   │   ├── releases/
│   │   │   ├── stable/
│   │   │   ├── experimental/
│   │   │   └── recovery/
│   │   ├── architectures/
│   │   │   ├── x86_64/
│   │   │   ├── aarch64/
│   │   │   ├── armv7/
│   │   │   └── microcontroller/
│   │   ├── backends/
│   │   │   ├── cpu/
│   │   │   ├── cuda_legacy/
│   │   │   ├── cuda_modern/
│   │   │   └── edge/
│   │   ├── adapters/
│   │   │   ├── cameras/
│   │   │   ├── sensors/
│   │   │   └── networking/
│   │   ├── python/
│   │   │   ├── runtimes/
│   │   │   └── wheelhouse/
│   │   ├── signatures/
│   │   ├── manifests/
│   │   └── recovery_tools/
│   │
│   └── pocket_decoder/
│       ├── README.md
│       ├── client/
│       ├── display/
│       ├── usb_gadget/
│       ├── provisioning_ui/
│       ├── recovery_ui/
│       └── configuration/
│
├── tools/
│   ├── build_release.py
│   ├── sign_release.py
│   ├── verify_release.py
│   ├── generate_manifest.py
│   ├── inspect_checkpoint.py
│   ├── inspect_packet.py
│   ├── node_probe.py
│   ├── mesh_monitor.py
│   ├── replay_viewer.py
│   ├── migrate_checkpoint.py
│   ├── cuda_capability_probe.cu
│   ├── cuda_device_discovery_probe.cu
│   ├── cuda_health_probe.cu
│   ├── cuda_device_pool_probe.cu
│   ├── cuda_backend_registry_probe.cu
│   ├── cpu_backend_probe.cpp
│   └── arm_capability_probe.cpp
│
├── scripts/
│   ├── configure.sh
│   ├── build.sh
│   ├── install.sh
│   ├── run_node.sh
│   ├── run_simulator.sh
│   ├── create_release.sh
│   └── clean.sh
│
├── systemd/
│   ├── prometheus-node.service
│   ├── prometheus-python-host.service
│   ├── prometheus-timesource.service
│   └── prometheus-maintenance.socket
│
├── udev/
│   ├── 70-prometheus-pico.rules
│   ├── 71-prometheus-giga.rules
│   └── 72-prometheus-decoder.rules
│
├── tests/
│   ├── unit/
│   │   ├── core/
│   │   ├── protocol/
│   │   ├── messaging/
│   │   ├── mesh/
│   │   ├── backends/
│   │   │   ├── test_cpu_foundation.cpp
│   │   │   ├── test_cpu_backend_registry.cpp
│   │   │   ├── test_cpu_health_capacity.cpp
│   │   │   ├── test_cpu_thread_pool.cpp
│   │   │   ├── test_cpu_simd_dispatch.cpp
│   │   │   ├── test_arm_capabilities.cpp
│   │   │   ├── test_arm_linux_auxv.cpp
│   │   │   ├── test_arm_processor_identity.cpp
│   │   │   ├── test_cuda_backend_registry.cu
│   │   │   ├── test_cuda_runtime_resources.cu
│   │   │   └── test_hailo_backend.cpp
│   │   ├── services/
│   │   ├── storage/
│   │   ├── security/
│   │   └── provisioning/
│   │
│   ├── integration/
│   │   ├── test_robot_ingress.cpp
│   │   ├── test_giga_telemetry.cpp
│   │   ├── test_frame_assembly.cpp
│   │   ├── test_cuda_discovery.cu
│   │   ├── test_node_discovery.cpp
│   │   ├── test_service_migration.cpp
│   │   ├── test_mesh_partition.cpp
│   │   ├── test_clock_holdover.cpp
│   │   └── test_pocket_decoder.cpp
│   │
│   ├── protocol/
│   │   ├── packet_vectors/
│   │   └── malformed_packets/
│   │
│   ├── failure/
│   │   ├── gpu_loss/
│   │   ├── node_loss/
│   │   ├── nexus_loss/
│   │   ├── clock_loss/
│   │   └── corrupted_checkpoint/
│   │
│   └── fixtures/
│       ├── models/
│       ├── checkpoints/
│       ├── telemetry/
│       └── tensor_packets/
│
├── simulator/
│   ├── README.md
│   ├── virtual_node.hpp
│   ├── virtual_node.cpp
│   ├── virtual_mesh.hpp
│   ├── virtual_mesh.cpp
│   ├── virtual_robot.hpp
│   ├── virtual_robot.cpp
│   ├── fault_injector.hpp
│   ├── fault_injector.cpp
│   ├── scenarios/
│   │   ├── five_node_mesh.yaml
│   │   ├── node_failure.yaml
│   │   ├── network_partition.yaml
│   │   ├── clock_failure.yaml
│   │   ├── service_migration.yaml
│   │   └── model_rollout.yaml
│   └── main.cpp
│
├── docs/
│   ├── PROJECT_TREE.md
│   ├── handoffs/
│   │   └── ARM_A1_CODEX_HANDOFF.md
│   ├── architecture/
│   │   ├── node_runtime.md
│   │   ├── service_model.md
│   │   ├── distributed_mesh.md
│   │   ├── proximity_gradient.md
│   │   ├── cognitive_services.md
│   │   └── data_flow.md
│   │
│   ├── protocols/
│   │   ├── robot_protocol.md
│   │   ├── mesh_protocol.md
│   │   ├── diagnostics_protocol.md
│   │   └── provisioning_protocol.md
│   │
│   ├── hardware/
│   │   ├── raspberry_pi.md
│   │   ├── arduino_giga.md
│   │   ├── pico_clock.md
│   │   ├── nvidia_gpu_nodes.md
│   │   ├── nexus_thin_client.md
│   │   └── pocket_decoder.md
│   │
│   ├── security/
│   │   ├── identity.md
│   │   ├── enrollment.md
│   │   ├── operator_authority.md
│   │   ├── artifact_signing.md
│   │   ├── airgap_workflow.md
│   │   └── recovery.md
│   │
│   ├── development/
│   │   ├── build.md
│   │   ├── testing.md
│   │   ├── coding_style.md
│   │   ├── adding_services.md
│   │   ├── adding_backends.md
│   │   └── python_sdk.md
│   │
│   └── migration/
│       ├── vision_swarm_breakdown.md
│       ├── phase_01_protocol.md
│       ├── phase_02_time.md
│       ├── phase_03_telemetry.md
│       ├── phase_04_ingress.md
│       ├── phase_05_cuda_backend.md
│       └── phase_06_mesh.md
│
├── legacy/
│   ├── README.md
│   ├── vision_swarm_11.cu
│   ├── legacy_checkpoint_format.md
│   └── legacy_mirror_protocol.md
│
├── third_party/
│   ├── README.md
│   └── licenses/
│
├── build/
│   └── .gitkeep
│
├── logs/
│   └── .gitkeep
│
└── data/
    ├── checkpoints/
    ├── replay/
    ├── models/
    ├── identities/
    ├── trust/
    ├── cache/
    └── temporary/
```
