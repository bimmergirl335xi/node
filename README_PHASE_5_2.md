# Prometheus Kernel Phase 5.2

Phase 5.2 adds optimizer and persistent update-buffer operations.

## Build-Workflow Change

No `rm -rf` command is required. Use a new named build directory:

```text
build/phase-5-2
```

CMake writes generated files there and does not delete project source.

## Added

- signal accumulation
- SGD
- momentum
- RMSProp
- Adam with bias correction
- ascent and descent
- decoupled weight decay
- signal scaling/clipping
- delta clipping
- parameter clipping
- masks
- shadow mode
- explicit apply mode
- first and second moments
- optimizer-state reset
- step-counter reset/increment
- FP32 and FP64
- tests and benchmark

## Apply

```bash
unzip -o prometheus_kernel_phase_5_2.zip -d .
```

## Configure, Build, and Test

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

## Benchmark

```bash
./build/phase-5-2/benchmark_optimizer_kernels \
  | tee optimizer_benchmark.csv
```

## Phase 5.1 Integration

Phase 5.1 already produces an effective proposed update. To feed that directly
to Phase 5.2, use optimizer learning rate 1, signal scale 1, and weight decay 0.

The cleaner long-term path is to configure Phase 5.1 to emit an unscaled local
learning signal, then let Phase 5.2 own learning-rate scaling, optimizer state,
and final clipping.

## Safe Commit Order

```bash
git switch -c kernels/phase-05-2-optimizer
unzip -o prometheus_kernel_phase_5_2.zip -d .

cmake \
  -S . \
  -B build/phase-5-2 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DPROMETHEUS_CUDA_ARCHITECTURES="75" \
  -DPROMETHEUS_BUILD_TESTS=ON \
  -DPROMETHEUS_BUILD_BENCHMARKS=ON

cmake --build build/phase-5-2 --parallel
ctest --test-dir build/phase-5-2 --output-on-failure

git add \
  CMakeLists.txt \
  src/kernels \
  tests/unit/kernels/test_optimizer_kernels.cu \
  benchmarks/optimizer_benchmark.cu \
  README.md \
  AI_CONTEXT.md \
  docs/CURRENT_STATE.md

git commit -m "Add CUDA optimizer state and update buffers"
```

A failed test does not remove or damage source files. Fix the branch and test
again before committing.
