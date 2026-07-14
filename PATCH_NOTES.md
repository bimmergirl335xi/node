# Phase 6 CUDA Runtime-Resource Closeout

## Actual remaining placeholders

Implemented:

- `src/backends/cuda/cuda_memory_pool.hpp`
- `src/backends/cuda/cuda_memory_pool.cu`
- `src/backends/cuda/cuda_job_queue.hpp`
- `src/backends/cuda/cuda_job_queue.cu`

`cuda_profiles.hpp` and `cuda_profiles.cpp` were already implemented and
validated in Phase 6.1. The project tree uses `.cpp`, not `.cu`; do not create a
second `cuda_profiles.cu` translation unit.

## Memory-pool boundary

The memory pool:

- records stable UUID-based device identities during initialization;
- allocates only after an explicit caller request;
- re-resolves the current CUDA ordinal from the stable identity;
- preserves the caller's selected CUDA device;
- uses portable `cudaMalloc`/`cudaFree` for Pascal-and-newer compatibility;
- reuses best-fit cached blocks;
- applies per-device cache limits;
- uses unique lease IDs so stale releases cannot free a newer checkout;
- refuses ordinary shutdown while active leases exist;
- never calls `cudaDeviceReset`;
- creates no GUI, worker thread, or display dependency.

## Job-queue boundary

The job queue:

- stores jobs against stable device keys;
- validates current device readiness and kernel-registry execution coverage;
- orders claims by priority and then submission sequence;
- uses claim tokens to reject stale state transitions;
- supports queued -> claimed -> running -> completed/failed;
- permits cancellation only before running;
- can requeue an unstarted claim;
- retains bounded terminal history;
- creates no CUDA stream or worker thread;
- launches no kernel.

This is an execution-queue foundation, not a production dispatcher. A later
worker will connect typed kernel adapters, streams, events, deadlines, and
failure recovery.

## Build integration

Replace:

- `src/backends/cuda/CMakeLists.txt`
- `tests/unit/backends/CMakeLists.txt`

Add the new four source files and:

- `tests/unit/backends/test_cuda_runtime_resources.cu`
- `tools/cuda_runtime_resources_probe.cu`

## Validation

```bash
cmake   -S .   -B build/phase-6-runtime-resources   -DCMAKE_BUILD_TYPE=RelWithDebInfo   -DPROMETHEUS_CUDA_ARCHITECTURES="75"   -DPROMETHEUS_BUILD_TESTS=ON   -DPROMETHEUS_BUILD_BENCHMARKS=ON   -DPROMETHEUS_BUILD_LEGACY_VISION=OFF

cmake --build build/phase-6-runtime-resources --parallel --verbose

ctest   --test-dir build/phase-6-runtime-resources   --output-on-failure
```

Expected test count: 8.

Optional human-readable probe:

```bash
nvcc   -std=c++17   -Xcompiler -pthread   -Isrc/backends   -Isrc/backends/cuda   tools/cuda_runtime_resources_probe.cu   src/backends/backend_registry.cpp   src/backends/cuda/cuda_backend.cu   src/backends/cuda/cuda_capabilities.cu   src/backends/cuda/cuda_device.cu   src/backends/cuda/cuda_device_pool.cu   src/backends/cuda/cuda_health.cu   src/backends/cuda/cuda_job_queue.cu   src/backends/cuda/cuda_kernel_registry.cu   src/backends/cuda/cuda_memory_pool.cu   src/backends/cuda/cuda_profiles.cpp   -o build/phase-6-runtime-resources/cuda_runtime_resources_probe

./build/phase-6-runtime-resources/cuda_runtime_resources_probe
```

Do not add `-ldl`.
