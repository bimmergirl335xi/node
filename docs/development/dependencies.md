# Dependency Boundaries

Prometheus Node will eventually provision minimal headless nodes from release
manifests selected for the detected hardware. A single `requirements.txt`
cannot describe the whole machine, so dependencies are separated by ownership.

## Python

- `requirements.txt`: direct Python runtime dependencies.
- `requirements-lock.txt`: pinned, reproducible Python dependency closure.
- Phase 6 adds no Python packages.

## Native Build

Phase 6 currently requires:

- a C++17 compiler;
- CMake integration once the project build files are connected;
- NVIDIA CUDA Toolkit 12.x for the Pascal/Volta/Turing-compatible release;
- an NVIDIA driver capable of loading the selected CUDA runtime.

A separate CUDA 13.x release family may target Turing and newer hardware. CUDA
must remain one optional backend, not a node-wide requirement.

## Native Runtime

- CUDA device discovery uses the CUDA runtime supplied with the selected native
  release.
- Rich health telemetry uses `libnvidia-ml.so.1` when present.
- NVML is loaded dynamically; its absence does not prevent CPU or other
  backends from running.
- On the current glibc environment, direct `nvcc` probe links must omit `-ldl`.
  The dynamic-loader functions resolve from libc. Future build integration must
  test older libc targets rather than assuming one linker rule globally.

## Provisioning Direction

Future release manifests—not pip—must describe:

- supported CPU architecture and instruction sets;
- CUDA release family and compiled GPU architectures;
- driver and firmware prerequisites;
- optional accelerators and peripheral drivers;
- storage roles and filesystem preparation;
- RAM/VRAM budgets and service profiles;
- recovery, rollback, signatures, and offline package sources.

Provisioning remains operator-authorized. Hardware discovery may recommend a
profile, but it must not repartition drives, install drivers, or activate
services without the authorized installation workflow.
