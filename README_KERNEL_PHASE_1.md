# Prometheus CUDA Kernel Phase 1

This patch makes the root CMake project build a real CUDA static library and
implements the first two reusable kernel groups:

- `activation_kernels.*`
- `reduction_kernels.*`

It intentionally does not migrate the dense, normalization, temporal, or
learning kernels yet. Those will be layered on top of this API after the
foundation passes on both Pascal and Volta hardware.

## Apply

Copy the files into the matching paths in the Prometheus repository. Existing
placeholder files at those paths should be replaced.

## Configure

For the P106-100 and Tesla V100 nodes:

```bash
rm -rf build

cmake \
  -S . \
  -B build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DPROMETHEUS_CUDA_ARCHITECTURES="61;70" \
  -DPROMETHEUS_BUILD_TESTS=ON
```

## Build

```bash
cmake --build build --parallel --verbose
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

Run the test executable directly when you want its console output:

```bash
./build/test_kernel_foundation
```

Depending on the selected CMake generator, the executable may instead be under
a configuration subdirectory such as `build/RelWithDebInfo/`.

## Optional legacy build

After the working CUDA prototype has been copied to
`legacy/vision_swarm_11.cu`:

```bash
cmake \
  -S . \
  -B build \
  -DPROMETHEUS_BUILD_LEGACY_VISION=ON
```

The legacy program remains a separate target and does not yet use the new
kernel library.

## Important behavior

The public launch functions:

- accept a `cudaStream_t`;
- do not call `cudaDeviceSynchronize`;
- return CUDA argument or launch errors;
- permit in-place activation;
- require caller-owned reduction workspace;
- avoid allocating memory in the inference hot path.

The next phase will implement:

1. dense FP32 GEMV with bias and optional fused activation;
2. vectorized and warp-oriented dispatch paths;
3. layer normalization using the reduction primitives;
4. CPU-reference correctness tests;
5. per-GPU benchmarking and launch-profile selection.
