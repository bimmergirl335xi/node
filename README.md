# Prometheus Node

Prometheus Node is the evolving runtime for a distributed cognitive mesh.

The robot remains the embodied system and retains its existing sensor,
microcontroller, timing, and safety architecture. Prometheus Node replaces the
current monolithic node-side program with a portable runtime that can discover
hardware, expose capabilities, host services, exchange typed messages, and
participate in distributed learning and memory.

Prometheus is the robot's internal name and project codename. A separate public
name may be selected before public release.

## Current Development Focus

Confirmed kernel phases:

1. Activation and reduction
2. Multitype dense operations
3. LayerNorm and RMSNorm
4. Temporal state, prediction error, and surprise
5.1 Shadow-safe local learning mathematics

Current phase:

5.2 Optimizer state and accumulated update processing

The compute foundation includes FP32, FP64, native fixed-width integer dense
accumulation, asynchronous launch APIs, CPU-reference tests, per-device
benchmarks, shadow learning, and explicit apply paths.

## Learning Safety Boundary

Learning mathematics and model deployment policy are separate.

Learning and optimizer kernels can run in shadow mode. Shadow mode calculates
proposed changes without modifying weights or optimizer state. Applying changes
is explicit. Candidate promotion, provenance, validation, checkpointing,
distribution, and rollback belong above the kernel layer.

## Safe Build Workflow

Use a new build directory for each phase. No source directory needs to be
deleted:

```bash
cmake \
  -S . \
  -B build/phase-5-2 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DPROMETHEUS_CUDA_ARCHITECTURES="75" \
  -DPROMETHEUS_BUILD_TESTS=ON \
  -DPROMETHEUS_BUILD_BENCHMARKS=ON

cmake --build build/phase-5-2 --parallel --verbose

ctest \
  --test-dir build/phase-5-2 \
  --output-on-failure
```

For the shared P106/V100 build, use architecture list `"61;70"`.

## Current Kernel Tests

```text
kernel_foundation
dense_kernels
normalization_kernels
temporal_kernels
learning_kernels
optimizer_kernels
```

See `AI_CONTEXT.md` before architectural changes and
`docs/CURRENT_STATE.md` for the current checkpoint.
