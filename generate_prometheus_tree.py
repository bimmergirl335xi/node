#!/usr/bin/env python3
"""
Generate the Prometheus Node project skeleton in the folder containing this script.

Default behavior:
- Creates directories and missing files.
- Never overwrites an existing file.
- Adds small type-appropriate placeholders so files are visible and editable.
- Can optionally copy the current CUDA prototype into legacy/vision_swarm_11.cu.

Examples:
    python3 generate_prometheus_tree.py
    python3 generate_prometheus_tree.py --dry-run
    python3 generate_prometheus_tree.py --root /path/to/prometheus_node
    python3 generate_prometheus_tree.py --copy-legacy "/path/to/vision_swarm(11).cu"
    python3 generate_prometheus_tree.py --force
"""

from __future__ import annotations

import argparse
import shutil
import stat
import sys
from pathlib import Path


FILES = """
README.md
ARCHITECTURE.md
PROTOCOL.md
TIME.md
SECURITY.md
THREAT_MODEL.md
CONTRIBUTING.md
CHANGELOG.md
LICENSE
CMakeLists.txt
pyproject.toml
requirements-lock.txt

cmake/CompilerOptions.cmake
cmake/CudaArchitectures.cmake
cmake/Dependencies.cmake
cmake/Sanitizers.cmake
cmake/VersionInfo.cmake

config/node.example.yaml
config/mesh.example.yaml
config/security.example.yaml
config/services.example.yaml
config/logging.example.yaml
config/hardware_profiles/generic_cpu.yaml
config/hardware_profiles/raspberry_pi.yaml
config/hardware_profiles/nvidia_pascal.yaml
config/hardware_profiles/nvidia_volta.yaml
config/hardware_profiles/nvidia_modern.yaml
config/hardware_profiles/robot_vision_node.yaml
config/hardware_profiles/robot_body_node.yaml
config/hardware_profiles/nexus_thin_client.yaml

include/prometheus/version.hpp
include/prometheus/result.hpp
include/prometheus/identifiers.hpp
include/prometheus/service.hpp
include/prometheus/message.hpp
include/prometheus/capability.hpp
include/prometheus/permissions.hpp
include/prometheus/timestamps.hpp

src/main.cpp

src/core/node_identity.hpp
src/core/node_identity.cpp
src/core/node_runtime.hpp
src/core/node_runtime.cpp
src/core/service.hpp
src/core/service.cpp
src/core/service_context.hpp
src/core/service_context.cpp
src/core/service_manager.hpp
src/core/service_manager.cpp
src/core/capability_registry.hpp
src/core/capability_registry.cpp
src/core/task_scheduler.hpp
src/core/task_scheduler.cpp
src/core/resource_manager.hpp
src/core/resource_manager.cpp
src/core/health_monitor.hpp
src/core/health_monitor.cpp
src/core/shutdown_manager.hpp
src/core/shutdown_manager.cpp
src/core/configuration.hpp
src/core/configuration.cpp

src/protocol/robot_protocol.hpp
src/protocol/mesh_protocol.hpp
src/protocol/diagnostic_protocol.hpp
src/protocol/provisioning_protocol.hpp
src/protocol/model_protocol.hpp
src/protocol/protocol_version.hpp
src/protocol/checksum.hpp
src/protocol/checksum.cpp
src/protocol/serializer.hpp
src/protocol/serializer.cpp

src/time/pico_clock.hpp
src/time/pico_clock.cpp
src/time/clock_alignment.hpp
src/time/clock_alignment.cpp
src/time/clock_quality.hpp
src/time/clock_quality.cpp
src/time/holdover_clock.hpp
src/time/holdover_clock.cpp

src/messaging/local_message_bus.hpp
src/messaging/local_message_bus.cpp
src/messaging/message_queue.hpp
src/messaging/message_queue.cpp
src/messaging/subscription.hpp
src/messaging/subscription.cpp
src/messaging/topic_registry.hpp
src/messaging/topic_registry.cpp
src/messaging/shared_memory_channel.hpp
src/messaging/shared_memory_channel.cpp
src/messaging/unix_socket_channel.hpp
src/messaging/unix_socket_channel.cpp

src/mesh/mesh_service.hpp
src/mesh/mesh_service.cpp
src/mesh/peer_identity.hpp
src/mesh/peer_identity.cpp
src/mesh/peer_table.hpp
src/mesh/peer_table.cpp
src/mesh/discovery.hpp
src/mesh/discovery.cpp
src/mesh/membership.hpp
src/mesh/membership.cpp
src/mesh/transport.hpp
src/mesh/transport.cpp
src/mesh/connection_manager.hpp
src/mesh/connection_manager.cpp
src/mesh/link_metrics.hpp
src/mesh/link_metrics.cpp
src/mesh/proximity_graph.hpp
src/mesh/proximity_graph.cpp
src/mesh/diffusion_policy.hpp
src/mesh/diffusion_policy.cpp
src/mesh/route_selector.hpp
src/mesh/route_selector.cpp
src/mesh/subscriptions.hpp
src/mesh/subscriptions.cpp
src/mesh/anti_entropy.hpp
src/mesh/anti_entropy.cpp
src/mesh/replication.hpp
src/mesh/replication.cpp
src/mesh/lease_manager.hpp
src/mesh/lease_manager.cpp
src/mesh/task_router.hpp
src/mesh/task_router.cpp

src/backends/compute_backend.hpp
src/backends/backend_registry.hpp
src/backends/backend_registry.cpp

src/backends/cpu/cpu_backend.hpp
src/backends/cpu/cpu_backend.cpp
src/backends/cpu/cpu_capabilities.hpp
src/backends/cpu/cpu_capabilities.cpp
src/backends/cpu/cpu_thread_pool.hpp
src/backends/cpu/cpu_thread_pool.cpp
src/backends/cpu/simd_dispatch.hpp
src/backends/cpu/simd_dispatch.cpp

src/backends/cuda/cuda_backend.hpp
src/backends/cuda/cuda_backend.cu
src/backends/cuda/cuda_device.hpp
src/backends/cuda/cuda_device.cu
src/backends/cuda/cuda_device_pool.hpp
src/backends/cuda/cuda_device_pool.cu
src/backends/cuda/cuda_capabilities.hpp
src/backends/cuda/cuda_capabilities.cu
src/backends/cuda/cuda_memory_pool.hpp
src/backends/cuda/cuda_memory_pool.cu
src/backends/cuda/cuda_job_queue.hpp
src/backends/cuda/cuda_job_queue.cu
src/backends/cuda/cuda_kernel_registry.hpp
src/backends/cuda/cuda_kernel_registry.cu
src/backends/cuda/cuda_profiles.hpp
src/backends/cuda/cuda_profiles.cpp

src/backends/arm/arm_capabilities.hpp
src/backends/arm/arm_capabilities.cpp
src/backends/arm/neon_dispatch.hpp
src/backends/arm/neon_dispatch.cpp

src/backends/storage/storage_backend.hpp
src/backends/storage/storage_backend.cpp

src/backends/accelerator/accelerator_backend.hpp
src/backends/accelerator/hailo_backend.cpp
src/backends/accelerator/imx500_backend.cpp

src/services/robot_ingress/robot_ingress_service.hpp
src/services/robot_ingress/robot_ingress_service.cpp
src/services/robot_ingress/pi_connection.hpp
src/services/robot_ingress/pi_connection.cpp
src/services/robot_ingress/tensor_receiver.hpp
src/services/robot_ingress/tensor_receiver.cpp

src/services/giga_telemetry/giga_telemetry_service.hpp
src/services/giga_telemetry/giga_telemetry_service.cpp
src/services/giga_telemetry/giga_link_state.hpp
src/services/giga_telemetry/giga_link_state.cpp

src/services/frame_assembler/frame_assembler_service.hpp
src/services/frame_assembler/frame_assembler_service.cpp
src/services/frame_assembler/frame_assembly.hpp
src/services/frame_assembler/frame_assembly.cpp

src/services/visual_perception/visual_perception_service.hpp
src/services/visual_perception/visual_perception_service.cu
src/services/visual_perception/visual_perception_worker.hpp
src/services/visual_perception/visual_perception_worker.cu
src/services/visual_perception/perception_result.hpp
src/services/visual_perception/stream_assignment.hpp
src/services/visual_perception/stream_assignment.cpp

src/services/temporal_prediction/temporal_prediction_service.hpp
src/services/temporal_prediction/temporal_prediction_service.cu
src/services/temporal_prediction/temporal_state.hpp
src/services/temporal_prediction/temporal_state.cpp

src/services/experience_recorder/experience_recorder_service.hpp
src/services/experience_recorder/experience_recorder_service.cpp
src/services/experience_recorder/replay_ring.hpp
src/services/experience_recorder/replay_ring.cpp

src/services/maintenance_port/maintenance_port_service.hpp
src/services/maintenance_port/maintenance_port_service.cpp
src/services/maintenance_port/analyzer_session.hpp
src/services/maintenance_port/analyzer_session.cpp

src/services/node_observer/node_observer_service.hpp
src/services/node_observer/node_observer_service.cpp

src/models/model.hpp
src/models/model.cpp
src/models/model_manifest.hpp
src/models/model_manifest.cpp
src/models/model_loader.hpp
src/models/model_loader.cpp
src/models/model_registry.hpp
src/models/model_registry.cpp
src/models/model_version.hpp
src/models/model_version.cpp
src/models/model_delta.hpp
src/models/model_delta.cpp

src/learning/learning_mode.hpp
src/learning/online_plasticity.hpp
src/learning/online_plasticity.cu
src/learning/predictive_learning.hpp
src/learning/predictive_learning.cu
src/learning/weight_consolidation.hpp
src/learning/weight_consolidation.cu
src/learning/candidate_model.hpp
src/learning/candidate_model.cpp
src/learning/model_evaluator.hpp
src/learning/model_evaluator.cpp

src/kernels/dense_kernels.cuh
src/kernels/dense_kernels.cu
src/kernels/activation_kernels.cuh
src/kernels/activation_kernels.cu
src/kernels/normalization_kernels.cuh
src/kernels/normalization_kernels.cu
src/kernels/temporal_kernels.cuh
src/kernels/temporal_kernels.cu
src/kernels/learning_kernels.cuh
src/kernels/learning_kernels.cu
src/kernels/reduction_kernels.cuh
src/kernels/reduction_kernels.cu

src/memory/working_memory.hpp
src/memory/working_memory.cpp
src/memory/episodic_memory.hpp
src/memory/episodic_memory.cpp
src/memory/semantic_memory.hpp
src/memory/semantic_memory.cpp
src/memory/procedural_memory.hpp
src/memory/procedural_memory.cpp
src/memory/memory_object.hpp
src/memory/memory_object.cpp
src/memory/memory_index.hpp
src/memory/memory_index.cpp
src/memory/memory_replication.hpp
src/memory/memory_replication.cpp

src/storage/object_store.hpp
src/storage/object_store.cpp
src/storage/content_address.hpp
src/storage/content_address.cpp
src/storage/checkpoint_store.hpp
src/storage/checkpoint_store.cpp
src/storage/replay_store.hpp
src/storage/replay_store.cpp
src/storage/model_store.hpp
src/storage/model_store.cpp
src/storage/metadata_store.hpp
src/storage/metadata_store.cpp

src/diagnostics/log_bus.hpp
src/diagnostics/log_bus.cpp
src/diagnostics/log_record.hpp
src/diagnostics/log_record.cpp
src/diagnostics/log_sink.hpp
src/diagnostics/log_sink.cpp
src/diagnostics/audit_log.hpp
src/diagnostics/audit_log.cpp
src/diagnostics/metrics.hpp
src/diagnostics/metrics.cpp
src/diagnostics/fault_report.hpp
src/diagnostics/fault_report.cpp
src/diagnostics/serial_channel.hpp
src/diagnostics/serial_channel.cpp

src/security/operator_identity.hpp
src/security/operator_identity.cpp
src/security/node_identity_store.hpp
src/security/node_identity_store.cpp
src/security/service_identity.hpp
src/security/service_identity.cpp
src/security/trust_store.hpp
src/security/trust_store.cpp
src/security/certificate_store.hpp
src/security/certificate_store.cpp
src/security/authorization.hpp
src/security/authorization.cpp
src/security/permission_manifest.hpp
src/security/permission_manifest.cpp
src/security/replay_protection.hpp
src/security/replay_protection.cpp
src/security/artifact_verifier.hpp
src/security/artifact_verifier.cpp

src/runtime/script_runtime.hpp
src/runtime/script_runtime.cpp
src/runtime/script_manifest.hpp
src/runtime/script_manifest.cpp
src/runtime/python_host.hpp
src/runtime/python_host.cpp
src/runtime/python_service.hpp
src/runtime/python_service.cpp
src/runtime/process_supervisor.hpp
src/runtime/process_supervisor.cpp

src/provisioning/release_manifest.hpp
src/provisioning/release_manifest.cpp
src/provisioning/hardware_probe.hpp
src/provisioning/hardware_probe.cpp
src/provisioning/profile_selector.hpp
src/provisioning/profile_selector.cpp
src/provisioning/package_selector.hpp
src/provisioning/package_selector.cpp
src/provisioning/installer.hpp
src/provisioning/installer.cpp
src/provisioning/rollback.hpp
src/provisioning/rollback.cpp
src/provisioning/system_slot.hpp
src/provisioning/system_slot.cpp

src/experiments/neural_mirror_legacy.hpp
src/experiments/neural_mirror_legacy.cpp
src/experiments/fungal_diffusion.hpp
src/experiments/fungal_diffusion.cpp

python/prometheus/__init__.py
python/prometheus/node.py
python/prometheus/service.py
python/prometheus/messages.py
python/prometheus/topics.py
python/prometheus/tensors.py
python/prometheus/time.py
python/prometheus/capabilities.py
python/prometheus/logging.py
python/prometheus/storage.py
python/prometheus/tasks.py
python/prometheus/permissions.py
python/prometheus/diagnostics.py

python/services/example_service.py
python/services/imu_analyzer.py
python/services/sensor_adapter.py
python/services/node_diagnostics.py

python/tests/test_messages.py
python/tests/test_service.py
python/tests/test_tensors.py

schemas/mesh_message.schema.json
schemas/capability_manifest.schema.json
schemas/service_manifest.schema.json
schemas/script_manifest.schema.json
schemas/model_manifest.schema.json
schemas/release_manifest.schema.json
schemas/node_configuration.schema.json
schemas/diagnostic_message.schema.json
schemas/robot_packet_layout.md

manifests/services/robot_ingress.yaml
manifests/services/giga_telemetry.yaml
manifests/services/frame_assembler.yaml
manifests/services/visual_perception.yaml
manifests/services/temporal_prediction.yaml
manifests/services/experience_recorder.yaml
manifests/services/maintenance_port.yaml

manifests/models/visual_perception_v1.yaml
manifests/models/temporal_predictor_v1.yaml

manifests/permissions/default_node.yaml
manifests/permissions/robot_vision_node.yaml
manifests/permissions/robot_body_node.yaml
manifests/permissions/nexus_node.yaml
manifests/permissions/pocket_decoder.yaml

models/README.md

tools/build_release.py
tools/sign_release.py
tools/verify_release.py
tools/generate_manifest.py
tools/inspect_checkpoint.py
tools/inspect_packet.py
tools/node_probe.py
tools/mesh_monitor.py
tools/replay_viewer.py
tools/migrate_checkpoint.py

scripts/configure.sh
scripts/build.sh
scripts/install.sh
scripts/run_node.sh
scripts/run_simulator.sh
scripts/create_release.sh
scripts/clean.sh

systemd/prometheus-node.service
systemd/prometheus-python-host.service
systemd/prometheus-timesource.service
systemd/prometheus-maintenance.socket

udev/70-prometheus-pico.rules
udev/71-prometheus-giga.rules
udev/72-prometheus-decoder.rules

tests/integration/test_robot_ingress.cpp
tests/integration/test_giga_telemetry.cpp
tests/integration/test_frame_assembly.cpp
tests/integration/test_cuda_discovery.cu
tests/integration/test_node_discovery.cpp
tests/integration/test_service_migration.cpp
tests/integration/test_mesh_partition.cpp
tests/integration/test_clock_holdover.cpp
tests/integration/test_pocket_decoder.cpp

simulator/README.md
simulator/virtual_node.hpp
simulator/virtual_node.cpp
simulator/virtual_mesh.hpp
simulator/virtual_mesh.cpp
simulator/virtual_robot.hpp
simulator/virtual_robot.cpp
simulator/fault_injector.hpp
simulator/fault_injector.cpp
simulator/scenarios/five_node_mesh.yaml
simulator/scenarios/node_failure.yaml
simulator/scenarios/network_partition.yaml
simulator/scenarios/clock_failure.yaml
simulator/scenarios/service_migration.yaml
simulator/scenarios/model_rollout.yaml
simulator/main.cpp

docs/architecture/node_runtime.md
docs/architecture/service_model.md
docs/architecture/distributed_mesh.md
docs/architecture/proximity_gradient.md
docs/architecture/cognitive_services.md
docs/architecture/data_flow.md

docs/protocols/robot_protocol.md
docs/protocols/mesh_protocol.md
docs/protocols/diagnostics_protocol.md
docs/protocols/provisioning_protocol.md

docs/hardware/raspberry_pi.md
docs/hardware/arduino_giga.md
docs/hardware/pico_clock.md
docs/hardware/nvidia_gpu_nodes.md
docs/hardware/nexus_thin_client.md
docs/hardware/pocket_decoder.md

docs/security/identity.md
docs/security/enrollment.md
docs/security/operator_authority.md
docs/security/artifact_signing.md
docs/security/airgap_workflow.md
docs/security/recovery.md

docs/development/build.md
docs/development/testing.md
docs/development/coding_style.md
docs/development/adding_services.md
docs/development/adding_backends.md
docs/development/python_sdk.md

docs/migration/vision_swarm_breakdown.md
docs/migration/phase_01_protocol.md
docs/migration/phase_02_time.md
docs/migration/phase_03_telemetry.md
docs/migration/phase_04_ingress.md
docs/migration/phase_05_cuda_backend.md
docs/migration/phase_06_mesh.md

legacy/README.md
legacy/vision_swarm_11.cu
legacy/legacy_checkpoint_format.md
legacy/legacy_mirror_protocol.md

third_party/README.md
build/.gitkeep
logs/.gitkeep
""".strip().splitlines()


DIRECTORIES = """
models/approved
models/candidates
models/archived
models/test_fixtures

runtime/python
runtime/wheelhouse
runtime/cuda_legacy
runtime/cuda_modern
runtime/cpu
runtime/recovery

provisioning/forge/releases/stable
provisioning/forge/releases/experimental
provisioning/forge/releases/recovery
provisioning/forge/architectures/x86_64
provisioning/forge/architectures/aarch64
provisioning/forge/architectures/armv7
provisioning/forge/architectures/microcontroller
provisioning/forge/backends/cpu
provisioning/forge/backends/cuda_legacy
provisioning/forge/backends/cuda_modern
provisioning/forge/backends/edge
provisioning/forge/adapters/cameras
provisioning/forge/adapters/sensors
provisioning/forge/adapters/networking
provisioning/forge/python/runtimes
provisioning/forge/python/wheelhouse
provisioning/forge/signatures
provisioning/forge/manifests
provisioning/forge/recovery_tools

provisioning/pocket_decoder/client
provisioning/pocket_decoder/display
provisioning/pocket_decoder/usb_gadget
provisioning/pocket_decoder/provisioning_ui
provisioning/pocket_decoder/recovery_ui
provisioning/pocket_decoder/configuration

tests/unit/core
tests/unit/protocol
tests/unit/messaging
tests/unit/mesh
tests/unit/backends
tests/unit/services
tests/unit/storage
tests/unit/security
tests/unit/provisioning

tests/protocol/packet_vectors
tests/protocol/malformed_packets

tests/failure/gpu_loss
tests/failure/node_loss
tests/failure/nexus_loss
tests/failure/clock_loss
tests/failure/corrupted_checkpoint

tests/fixtures/models
tests/fixtures/checkpoints
tests/fixtures/telemetry
tests/fixtures/tensor_packets

third_party/licenses

data/checkpoints
data/replay
data/models
data/identities
data/trust
data/cache
data/temporary
""".strip().splitlines()


EXECUTABLE_SUFFIXES = {".sh"}
EXECUTABLE_NAMES = {
    "build_release.py",
    "sign_release.py",
    "verify_release.py",
    "generate_manifest.py",
    "inspect_checkpoint.py",
    "inspect_packet.py",
    "node_probe.py",
    "mesh_monitor.py",
    "replay_viewer.py",
    "migrate_checkpoint.py",
}


def clean_entries(entries: list[str]) -> list[str]:
    return [entry.strip() for entry in entries if entry.strip()]


def display_name(path: Path) -> str:
    name = path.stem.replace("_", " ").replace("-", " ").strip()
    return name.title() or path.name


def placeholder_for(relative: Path) -> str:
    """Return a minimal placeholder appropriate for the file type."""
    name = relative.name
    suffix = relative.suffix.lower()
    title = display_name(relative)

    if name == ".gitkeep":
        return ""

    if name == "CMakeLists.txt":
        return (
            "# Prometheus Node build definition.\n"
            "# This file is intentionally skeletal and will be filled during migration.\n"
        )

    if name == "pyproject.toml":
        return (
            "# Prometheus Node Python project configuration.\n"
            "# Add project metadata and build settings when the Python SDK is implemented.\n"
        )

    if name == "requirements-lock.txt":
        return (
            "# Offline, pinned Python dependencies for Prometheus Node.\n"
            "# Keep this file deterministic and generated from the approved wheelhouse.\n"
        )

    if name == "LICENSE":
        return (
            "Prometheus Node license has not yet been selected.\n"
            "Choose and add a license before accepting external contributions.\n"
        )

    if suffix in {".md"}:
        return f"# {title}\n\nTODO: Document this Prometheus component.\n"

    if suffix in {".hpp", ".cuh"}:
        guard = "_".join(relative.parts).replace(".", "_").replace("-", "_").upper()
        return (
            f"#ifndef {guard}\n"
            f"#define {guard}\n\n"
            f"// TODO: Implement {relative.as_posix()}.\n\n"
            f"#endif  // {guard}\n"
        )

    if suffix in {".cpp", ".cu"}:
        return f"// TODO: Implement {relative.as_posix()}.\n"

    if suffix == ".py":
        if name == "__init__.py":
            return '"""Prometheus Python package."""\n'
        return (
            "#!/usr/bin/env python3\n"
            f'"""TODO: Implement {relative.as_posix()}."""\n\n'
            "\n"
            "def main() -> int:\n"
            '    """Entry point placeholder."""\n'
            "    return 0\n\n"
            "\n"
            'if __name__ == "__main__":\n'
            "    raise SystemExit(main())\n"
        )

    if suffix == ".sh":
        return (
            "#!/usr/bin/env bash\n"
            "set -euo pipefail\n\n"
            f"# TODO: Implement {relative.as_posix()}.\n"
        )

    if suffix == ".json":
        return "{}\n"

    if suffix in {".yaml", ".yml"}:
        return f"# TODO: Define {relative.as_posix()}.\n"

    if suffix == ".cmake":
        return f"# TODO: Implement {relative.as_posix()}.\n"

    if suffix in {".service", ".socket", ".rules"}:
        return f"# TODO: Define {relative.as_posix()}.\n"

    if suffix == ".toml":
        return f"# TODO: Define {relative.as_posix()}.\n"

    return ""


def safe_target(root: Path, relative: Path) -> Path:
    """Prevent accidental path traversal outside the selected root."""
    root_resolved = root.resolve()
    target = (root / relative).resolve()
    try:
        target.relative_to(root_resolved)
    except ValueError as exc:
        raise ValueError(f"Refusing path outside root: {relative}") from exc
    return target


def create_directory(path: Path, dry_run: bool) -> bool:
    if path.exists():
        if not path.is_dir():
            raise RuntimeError(f"Expected directory but found file: {path}")
        return False

    print(f"[mkdir]  {path}")
    if not dry_run:
        path.mkdir(parents=True, exist_ok=True)
    return True


def create_file(path: Path, relative: Path, force: bool, dry_run: bool) -> str:
    if path.exists() and not force:
        print(f"[keep]   {path}")
        return "kept"

    action = "write" if path.exists() else "create"
    print(f"[{action:<6}] {path}")

    if not dry_run:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(placeholder_for(relative), encoding="utf-8", newline="\n")

        if relative.suffix in EXECUTABLE_SUFFIXES or relative.name in EXECUTABLE_NAMES:
            current_mode = path.stat().st_mode
            path.chmod(current_mode | stat.S_IXUSR | stat.S_IXGRP)

    return action


def copy_legacy_source(
    source: Path,
    target: Path,
    force: bool,
    dry_run: bool,
) -> str:
    if not source.exists() or not source.is_file():
        raise FileNotFoundError(f"Legacy CUDA source not found: {source}")

    if target.exists() and not force:
        print(f"[keep]   {target} (legacy target already exists)")
        return "kept"

    print(f"[copy]   {source} -> {target}")
    if not dry_run:
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    return "copied"


def parse_args() -> argparse.Namespace:
    script_dir = Path(__file__).resolve().parent

    parser = argparse.ArgumentParser(
        description="Generate the Prometheus Node project skeleton."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=script_dir,
        help=(
            "Target project root. Defaults to the directory containing this script. "
            "Use --root prometheus_node when running the script from one directory above."
        ),
    )
    parser.add_argument(
        "--copy-legacy",
        type=Path,
        metavar="CUDA_FILE",
        help="Copy an existing CUDA prototype to legacy/vision_swarm_11.cu.",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Overwrite existing generated files. Use with care.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print intended actions without changing the filesystem.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = args.root.expanduser().resolve()

    files = [Path(entry) for entry in clean_entries(FILES)]
    directories = [Path(entry) for entry in clean_entries(DIRECTORIES)]

    print(f"Prometheus project root: {root}")
    if args.dry_run:
        print("Dry-run mode: no filesystem changes will be made.")

    if not args.dry_run:
        root.mkdir(parents=True, exist_ok=True)

    created_dirs = 0
    created_files = 0
    overwritten_files = 0
    kept_files = 0

    all_directories = set(directories)
    for relative_file in files:
        parent = relative_file.parent
        while parent != Path("."):
            all_directories.add(parent)
            parent = parent.parent

    for relative_dir in sorted(all_directories, key=lambda p: (len(p.parts), p.as_posix())):
        target_dir = safe_target(root, relative_dir)
        if create_directory(target_dir, args.dry_run):
            created_dirs += 1

    for relative_file in files:
        target_file = safe_target(root, relative_file)
        result = create_file(
            target_file,
            relative_file,
            force=args.force,
            dry_run=args.dry_run,
        )
        if result == "create":
            created_files += 1
        elif result == "write":
            overwritten_files += 1
        else:
            kept_files += 1

    if args.copy_legacy is not None:
        legacy_target = safe_target(root, Path("legacy/vision_swarm_11.cu"))
        copy_result = copy_legacy_source(
            args.copy_legacy.expanduser().resolve(),
            legacy_target,
            force=args.force,
            dry_run=args.dry_run,
        )
        if copy_result == "copied":
            # The placeholder may already have been counted as created or overwritten.
            pass

    print()
    print("Prometheus tree generation complete.")
    print(f"  Directories created: {created_dirs}")
    print(f"  Files created:       {created_files}")
    print(f"  Files overwritten:   {overwritten_files}")
    print(f"  Existing files kept: {kept_files}")

    if args.force:
        print("  Warning: --force was enabled.")
    else:
        print("  Existing work was preserved.")

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        raise SystemExit(1)
