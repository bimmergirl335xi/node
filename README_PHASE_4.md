# Prometheus Kernel Phase 4

Phase 4 adds persistent temporal-state operations.

## Added

- temporal EMA
- per-stream EMA coefficients
- stream masks
- timestamp-aware exponential half-life decay
- per-stream elapsed time
- gated updates
- per-element gates
- fused previous-state capture, decay, and gated update
- residual prediction error
- absolute prediction error
- squared prediction error
- uncertainty-scaled surprise
- state copy
- selective reset
- stream masking
- FP32 and FP64
- correctness tests
- fused-versus-split benchmark

## Apply

```bash
unzip -o prometheus_kernel_phase_4.zip -d .
```

Review documentation changes before committing if the repository README already
contains custom material.

## Build and Test

```bash
rm -rf build

cmake \
  -S . \
  -B build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DPROMETHEUS_CUDA_ARCHITECTURES="61;70" \
  -DPROMETHEUS_BUILD_TESTS=ON \
  -DPROMETHEUS_BUILD_BENCHMARKS=ON

cmake --build build --parallel --verbose

ctest \
  --test-dir build \
  --output-on-failure
```

Expected new result:

```text
Prometheus temporal CUDA kernels passed
```

## Benchmark

```bash
./build/benchmark_temporal_kernels \
  | tee temporal_benchmark.csv
```

The benchmark compares one fused temporal update with three separate launches:

```text
copy state
decay state
gated update
```

The fused implementation should generally reduce launch and memory-traffic
overhead, but Prometheus should preserve both forms because separate operations
remain useful for experimentation and unusual model graphs.

## Temporal Semantics

The preferred persistent-state operation is:

```text
previous = state
decayed = state * 2^(-elapsed / half_life)
state = decayed + gate * (observation - decayed)
```

This makes wall-clock or Pico-derived elapsed time explicit. A delayed frame
therefore does not receive the same temporal weighting as an on-time frame.

## Missing Streams

A zero stream-mask entry prevents a stream state from being changed. In the
fused operation, `previous_state` still captures the prior value before the
update is skipped. This preserves an auditable state transition boundary.

## Surprise

```text
surprise = abs(observed - predicted) /
           max(abs(uncertainty), epsilon)
```

When no uncertainty tensor is supplied, surprise is simply absolute error.

A later service may reduce the surprise vector to a scalar using the already
implemented reduction kernels.

## Commit Suggestion

```bash
git switch -c kernels/phase-04-temporal

git add \
  CMakeLists.txt \
  src/kernels \
  tests/unit/kernels/test_temporal_kernels.cu \
  benchmarks/temporal_benchmark.cu \
  README.md \
  AI_CONTEXT.md \
  docs/CURRENT_STATE.md

git commit -m "Add persistent temporal CUDA kernels"
```

## Next Phase

Phase 5 will implement reusable learning primitives while keeping deployment
and promotion policy outside the kernels.
