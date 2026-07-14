# Prometheus Kernel Phase 2

Phase 2 adds reusable dense operations and begins the multi-generation compute
strategy.

## Added operations

- FP32 dense matrix-vector and batched matrix-vector
- FP64 dense matrix-vector and batched matrix-vector
- `int32 × int32 → int64` exact accumulation
- Optional bias
- Fused floating-point activation
- Warp-per-output implementation
- Block-per-output implementation
- Runtime heuristic (`DenseAlgorithm::Auto`)
- Correctness tests against CPU reference results
- Dense benchmark executable
- Offline compute-hardware probe
- Kepler and Xeon Phi hardware profiles

## Important correction about large integers

The Quadro K6000 is a useful legacy compute device, especially for workloads
where its FP64 characteristics or available memory make it competitive.
However, arbitrary-precision integers are not the same as native `int64`.

This phase provides native integer dense accumulation. A later
`integer_kernels` family should implement multi-limb addition, multiplication,
comparison, modular arithmetic, and carry propagation for truly large integers.

## Apply

Copy this archive over the existing project root. It replaces the Phase 1
versions of:

- root `CMakeLists.txt`
- `src/kernels/CMakeLists.txt`
- `activation_kernels.cuh`
- `activation_kernels.cu`

It fills the previously placeholder dense files and adds tests and profiles.

## P106 and V100 build

Use the same working CUDA 12.x environment that passed Phase 1:

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
ctest --test-dir build --output-on-failure
```

Expected new test output:

```text
Prometheus dense CUDA kernels passed
```

Run the benchmark:

```bash
./build/benchmark_dense_kernels \
  | tee dense_benchmark.csv
```

## Quadro K6000 build

Build the K6000 release separately. Do not add `35` to a CUDA 12 build.

Use a CUDA 11.4 toolchain and the compatible legacy NVIDIA driver:

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
ctest --test-dir build-kepler --output-on-failure
```

This should become a distinct Forge release family:

```text
cuda_kepler/
cuda_pascal_volta/
cuda_modern/
```

## Probe the Xeon Phi server

Run:

```bash
python3 tools/probe_compute.py \
  --output phi_compute_probe.json
```

The JSON will help determine whether the ten devices are:

- Knights Corner PCIe coprocessors, which need a host-to-coprocessor service;
- Knights Landing/Knights Mill bootable processors, which can run a native
  Prometheus node with a specialized CPU-vector backend.

Do not build a Phi backend until the exact model and available software stack
are known.

## Commit suggestion

```bash
git switch -c kernels/phase-02-dense-multitype

git add \
  CMakeLists.txt \
  src/kernels \
  tests/unit/kernels \
  benchmarks \
  tools/probe_compute.py \
  config/hardware_profiles

git commit -m "Add multitype CUDA dense kernel foundation"
```

## Next phase

Phase 3 should implement normalization:

- layer normalization;
- RMS normalization;
- batched normalization;
- optional affine scale and bias;
- FP32 and FP64 paths;
- stable Welford statistics;
- deterministic reference tests;
- benchmark-based launch selection.

After normalization, temporal kernels can be developed against stable dense,
activation, and reduction primitives.
