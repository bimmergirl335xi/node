# Node

Node is a portable, headless runtime foundation for a distributed embodied
cognitive system. The current repository keeps production robot behavior on
the legacy path while building typed, bounded infrastructure for future
services and heterogeneous compute.

## Integrated foundations

- CUDA kernel and backend foundations for explicit Pascal, Volta, and Turing
  targets, stable device identity, health, capacity, registry metadata, and
  dynamically loaded NVRTC readiness;
- a generic CPU backend with topology, capability, health, capacity, a bounded
  priority worker pool, explicit backpressure, and conservative SIMD
  selection;
- ARM capability enrichment through Linux auxiliary-vector observation and
  injectable processor-identity parsing, without registering a second CPU
  backend;
- service lifecycle composition with callbacks outside the manager lock and a
  borrowed backend registry in service context;
- pure backend-neutral execution-policy evaluation;
- typed adaptive-state descriptors, validation, atomic transactions, and
  rollback;
- typed architecture proposals, isolated shadow validation, impact evidence,
  and a bounded versioned C ABI for future proposal producers.

These foundations do not activate production execution, mutate live
architecture, promote generated code, or replace the legacy robot loop.

## Public architecture specifications

- `docs/architecture/acs/` contains ACS-0000 through ACS-0009 as public Draft
  communication architecture.
- `docs/architecture/memory/` contains MEM-0000 through MEM-0010 as public
  Draft memory architecture. No MEM persistence implementation is present.
- `docs/architecture/immune/` contains IMM-0000 and IMM-0001 as public Draft
  immune architecture. No private detection logic or IMM implementation is
  present.
- Public BOOT documents are not yet committed. Bootstrap implementation remains
  reserved for `lane/bootstrap`.

## Build and test

Use a named build directory. On memory-constrained hosts, build serially:

```bash
cmake -S . -B build/local \
  -DPROMETHEUS_BUILD_TESTS=ON \
  -DPROMETHEUS_BUILD_BENCHMARKS=OFF \
  -DPROMETHEUS_BUILD_LEGACY_VISION=OFF \
  '-DPROMETHEUS_CUDA_ARCHITECTURES=61;70;75'

cmake --build build/local --parallel 1
ctest --test-dir build/local --output-on-failure
```

CUDA tests require a compatible host driver and GPU. ARM discovery tests use
injectable evidence on non-ARM hosts; validation on real AArch32, AArch64, and
SVE/SVE2 systems remains separate.

Read `AI_CONTEXT.md` before architecture changes and
`docs/CURRENT_STATE.md` for the integrated checkpoint record.
