# Prometheus Kernel Phase 5.1

Phase 5.1 adds auditable local learning mathematics.

## Added

- Hebbian update
- Oja update
- predictive-delta update
- FP32 and FP64
- sum or mean batch reduction
- weight decay
- update clipping
- final-weight clipping
- row masks
- individual weight masks
- shadow mode
- explicit apply mode
- update tensor output
- per-row L1 statistics
- per-row L2 statistics
- per-row maximum absolute update
- CPU-reference tests
- benchmark executable

## Safety Model

Shadow mode is selected with:

```cpp
apply_update = false;
```

The kernel still computes:

- the effective proposed delta;
- clipping effects;
- final-weight-bound effects;
- row update statistics.

The original weight buffer remains unchanged.

Live mutation requires:

```cpp
apply_update = true;
```

Higher-level services must eventually authorize that choice. The generic kernel
does not promote models or overwrite approved checkpoints.

## Apply

```bash
unzip -o prometheus_kernel_phase_5_1.zip -d .
```

Review documentation changes before committing if local edits exist.

## Build and Test

For the current Quadro RTX 4000 validation host, architecture 75 is sufficient:

```bash
rm -rf build

cmake \
  -S . \
  -B build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DPROMETHEUS_CUDA_ARCHITECTURES="75" \
  -DPROMETHEUS_BUILD_TESTS=ON \
  -DPROMETHEUS_BUILD_BENCHMARKS=ON

cmake --build build --parallel --verbose

ctest \
  --test-dir build \
  --output-on-failure
```

For a shared P106/V100 release build, use:

```bash
-DPROMETHEUS_CUDA_ARCHITECTURES="61;70"
```

Expected new result:

```text
Prometheus local learning CUDA kernels passed
```

## Benchmark

```bash
./build/benchmark_learning_kernels \
  | tee learning_benchmark.csv
```

The benchmark compares:

- Hebbian
- Oja
- predictive delta
- shadow mode
- applied mode
- multiple batch and matrix dimensions

## Rule Semantics

### Hebbian

```text
correlation = postsynaptic * presynaptic
```

### Oja

```text
correlation =
    postsynaptic * presynaptic
  - postsynaptic^2 * current_weight
```

### Predictive Delta

```text
correlation = prediction_error * presynaptic
```

After batch sum or mean:

```text
delta =
    learning_rate
  * (correlation - weight_decay * current_weight)
```

The delta and proposed final weight are then independently clipped when their
limits are non-zero.

## Commit Suggestion

```bash
git switch -c kernels/phase-05-1-local-learning

git add \
  CMakeLists.txt \
  src/kernels \
  tests/unit/kernels/test_learning_kernels.cu \
  benchmarks/learning_benchmark.cu \
  README.md \
  AI_CONTEXT.md \
  docs/CURRENT_STATE.md

git commit -m "Add shadow-safe local learning CUDA kernels"
```

## Next

Phase 5.2 adds optimizer and update-buffer operations.
