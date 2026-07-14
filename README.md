# Prometheus Node

Prometheus Node is the evolving runtime for Project Prometheus, a distributed
cognitive system designed to preserve coherent logical identity across changing
hardware, processes, transports, and physical nodes.

The project is specification-driven and under active development. Interfaces
and implementations may change while the architecture documents remain in
Draft status.

## Current development focus

Current work is centered on two related areas:

- heterogeneous compute backends for CPU, CUDA, and ARM systems;
- the Adaptive Connection Substrate (ACS), which defines persistent logical
  relationships, bounded communication, trust, resilience, and adaptation.

Backend discovery reports hardware identity, capabilities, topology, health,
and advisory capacity without making a backend the cognitive master of a node
or assigning it unbounded execution resources.

The ACS specification series begins in
[`docs/architecture/acs`](docs/architecture/acs/README.md).

## Building

The current native build requires CMake 3.24 or newer, a C++17 compiler, and a
CUDA toolkit supported by the selected architecture profile.

Use an out-of-source build directory:

```bash
cmake \
  -S . \
  -B build/local \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DPROMETHEUS_CUDA_ARCHITECTURES="61;70" \
  -DPROMETHEUS_BUILD_TESTS=ON \
  -DPROMETHEUS_BUILD_BENCHMARKS=OFF

cmake --build build/local --parallel 2
ctest --test-dir build/local --output-on-failure
```

Choose CUDA architectures appropriate for the target machines. Conservative
build parallelism is recommended on memory-constrained systems.

## Repository layout

- `src/backends`: CPU, CUDA, ARM, accelerator, and storage backend work
- `src/kernels`: CUDA compute kernels
- `tests/unit`: native unit and hardware-integration tests
- `python`: Python runtime and service foundations
- `docs/architecture`: architectural specifications and design records
- `config`, `manifests`, and `schemas`: configuration and interface definitions
- `simulator`: distributed runtime and failure-scenario simulation

## Engineering boundaries

Correctness, bounded resource use, explicit uncertainty, recoverability, and
security take precedence over isolated component performance. Generated build
outputs, runtime data, downloaded artifacts, credentials, and local model files
must remain outside version control.

Read [`AI_CONTEXT.md`](AI_CONTEXT.md),
[`ARCHITECTURE.md`](ARCHITECTURE.md), and the relevant specification before
making foundational architectural changes.

## License

See [`LICENSE`](LICENSE).
