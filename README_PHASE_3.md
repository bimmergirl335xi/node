# Prometheus Kernel Phase 3

Phase 3 implements normalization as a reusable CUDA primitive.

## Added

- Layer normalization
- RMS normalization
- FP32 and FP64 paths
- Optional affine `gamma` and `beta`
- Optional per-row mean/inverse-scale outputs
- In-place and out-of-place execution
- Welford statistics
- Warp-per-row implementation
- Block-per-row implementation
- Automatic algorithm selection
- CPU-reference correctness tests
- Normalization benchmark executable
- Kepler-compatible shuffle path

## Apply

Extract the archive over the Prometheus repository root:

```bash
unzip -o prometheus_kernel_phase_3.zip -d .
```

This replaces:

- root `CMakeLists.txt`
- `src/kernels/CMakeLists.txt`

and fills the normalization placeholders.

## Build and test on P106/V100

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
Prometheus normalization CUDA kernels passed
```

## Benchmark

```bash
./build/benchmark_normalization_kernels \
  | tee normalization_benchmark.csv
```

The benchmark compares:

- automatic selection;
- warp-per-row;
- block-per-row;
- LayerNorm;
- RMSNorm;
- several row and feature dimensions.

## K6000 validation

Use the separate CUDA 11.4 / `sm_35` build tree:

```bash
rm -rf build-kepler

cmake \
  -S . \
  -B build-kepler \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DPROMETHEUS_CUDA_ARCHITECTURES="35" \
  -DPROMETHEUS_BUILD_TESTS=ON \
  -DPROMETHEUS_BUILD_BENCHMARKS=ON

cmake --build build-kepler --parallel --verbose

ctest \
  --test-dir build-kepler \
  --output-on-failure
```

## API behavior

The normalization launchers:

- use row-major contiguous tensors;
- allow `input == output`;
- accept optional `gamma` and `beta`;
- accept optional device statistics outputs;
- allocate no memory;
- synchronize no streams;
- return CUDA argument or launch errors;
- use population variance, matching standard LayerNorm behavior.

For RMSNorm, `mean_output` is intentionally omitted by the convenience wrapper.
The unified launcher writes zero as the center when a mean buffer is supplied.

## Notes

The Welford state provides stable mean and variance accumulation without using
the less stable `E[x^2] - E[x]^2` expression for LayerNorm.

RMSNorm derives mean square from:

```text
variance + mean^2
```

using the same Welford state.

## Commit suggestion

```bash
git switch -c kernels/phase-03-normalization

git add \
  CMakeLists.txt \
  src/kernels \
  tests/unit/kernels/test_normalization_kernels.cu \
  benchmarks/normalization_benchmark.cu

git commit -m "Add CUDA LayerNorm and RMSNorm kernels"
```

## Next phase

Phase 4 should implement temporal operations:

- temporal EMA;
- state decay;
- gated state update;
- prediction error;
- surprise magnitude;
- sequence masking;
- timestamp-aware decay;
- FP32 and FP64;
- stable stream-owned state;
- deterministic tests;
- benchmark-driven dispatch.

The temporal layer will then be built on the already tested activation,
reduction, dense, and normalization substrate.
