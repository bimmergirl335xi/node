# ARM Phase A1 — Codex Integration Handoff

## Purpose

This handoff packages generic ARM CPU capability, execution-profile, Linux auxiliary-vector, processor-identity, test, and diagnostic-probe work for integration into the moving Prometheus node tree.

The source branch is:

```text
agent/arm-a1-linux-auxv
```

The branch is intentionally isolated. Codex owns reconciliation with the current main tree, final file placement, shared CMake wiring, aggregate snapshot stitching, and conflict resolution.

## Architectural contract

ARM is an architecture family, not a Raspberry Pi product category and not a separately registered duplicate CPU backend.

The integrated design must preserve these boundaries:

- the generic CPU backend owns CPU identity, topology, health, capacity, lifecycle, and worker infrastructure;
- ARM code enriches the generic CPU snapshot with ARM-specific observations and execution profiles;
- the same physical CPU must not appear as both `CpuBackend` and a second `ArmBackend`;
- Hailo, IMX500, cameras, GPUs, NPUs, and other accelerators are optional independent resources;
- no permanent visual, audio, cognitive, or machine-level master is introduced;
- hardware capability does not prove that a typed Prometheus operation adapter exists;
- processor implementer and part numbers are observational metadata, not dispatch authority;
- missing observations remain explicit unknown, unavailable, partial, or failed states;
- core discovery code remains silent and headless-safe; human-readable output belongs in probes;
- robot behavior and legacy vision paths remain unchanged.

## Branch-owned files

### Capability and profile vocabulary

```text
src/backends/arm/arm_capabilities.hpp
src/backends/arm/arm_capabilities.cpp
```

Provides:

- AArch32 and AArch64 execution-state vocabulary;
- ARM capability provenance;
- capability-derived common execution profiles;
- ARM-specific feature coverage layered on generic `CpuFeatureCoverage`;
- explicit query status and issues;
- conservative profile selection from common feature coverage.

### Linux auxiliary-vector provider

```text
src/backends/arm/arm_linux_auxv.hpp
src/backends/arm/arm_linux_auxv.cpp
```

Provides:

- injected/raw `AT_HWCAP` and `AT_HWCAP2` observation seam;
- AArch64 Advanced SIMD, FP16, dot-product, I8MM, BF16, crypto, atomics, SVE, and SVE2 decoding;
- conservative AArch32 VFP, NEON, crypto, and CRC decoding;
- runtime SVE vector-length observation through `prctl(PR_SVE_GET_VL)`;
- explicit preservation of unknown states when an auxiliary-vector word was not observed.

### Processor identity provider

```text
src/backends/arm/arm_processor_identity.hpp
src/backends/arm/arm_processor_identity.cpp
```

Provides:

- injectable `/proc/cpuinfo` parser;
- read-only Linux `/proc/cpuinfo` provider;
- implementer, architecture, variant, part, and revision observations;
- grouping of target logical processors by observed signature;
- heterogeneous processor-signature support;
- missing, duplicate, malformed, and incomplete record issues;
- raw hexadecimal fallback for unknown implementers and parts.

### Tests

```text
tests/unit/backends/test_arm_capabilities.cpp
tests/unit/backends/test_arm_linux_auxv.cpp
tests/unit/backends/test_arm_processor_identity.cpp
```

Expected CTest names:

```text
prometheus_arm_capabilities
prometheus_arm_linux_auxv
prometheus_arm_processor_identity
```

### Diagnostic probe

```text
tools/arm_capability_probe.cpp
```

The probe:

1. queries generic CPU topology;
2. queries generic CPU capabilities;
3. enriches the snapshot with ARM capabilities;
4. reports the common ARM execution profile and every feature coverage state;
5. queries and reports processor-signature groups;
6. treats a non-ARM host as a valid not-applicable diagnostic result;
7. performs no affinity, lifecycle, worker, scheduling, accelerator, or robot changes.

## Required Codex stitching

### 1. Reconcile branch history

The branch and `main` have advanced independently. Integrate by file intent, not by blindly replacing newer shared files.

Prefer cherry-picking or copying the ARM-owned files, then manually reconciling the two shared CMake files:

```text
src/backends/arm/CMakeLists.txt
tests/unit/backends/CMakeLists.txt
```

Do not overwrite unrelated newer CPU, CUDA, runtime, test, or tool wiring.

### 2. Build the ARM library

The ARM capability target must compile:

```text
arm_capabilities.cpp
arm_linux_auxv.cpp
arm_processor_identity.cpp
```

It must link publicly or privately as appropriate against the generic CPU foundation that provides:

```text
cpu_topology
cpu_capabilities
CpuFeatureCoverage
CpuSupportState
```

### 3. Keep capability and identity providers separate

Integration checkpoint I-001 keeps processor-identity discovery separate from
`query_arm_capabilities()`. The capability query continues to describe ISA and
operating-system exposure; the identity provider retains its own detailed issue
codes and partial-result status. This avoids making observational processor
signatures look like instruction-dispatch authority and avoids expanding the
capability API into a full discovery aggregate.

The diagnostic probe is the current higher discovery layer: it calls both
providers with the same target logical processor IDs and presents their results
side by side. A future production discovery layer may compose the two results
without changing either provider's semantics. The former unconditional
`identity_unavailable` placeholder is therefore not emitted by the capability
query.

### 4. Preserve heterogeneous semantics

`ArmProcessorIdentitySnapshot::heterogeneous` means multiple complete observed signatures exist among current target logical processors.

This is not by itself proof of heterogeneous instruction coverage. Safe execution remains governed by common `CpuFeatureCoverage` and the common execution profile.

### 5. Wire the probe into the project tool layout

Use the project’s existing tool convention. A minimal target is equivalent to:

```cmake
add_executable(prometheus_arm_capability_probe EXCLUDE_FROM_ALL
    "${PROJECT_SOURCE_DIR}/tools/arm_capability_probe.cpp"
)

target_link_libraries(prometheus_arm_capability_probe
    PRIVATE
        prometheus_arm_capabilities
        prometheus_cpu_foundation
)

target_compile_features(
    prometheus_arm_capability_probe PRIVATE cxx_std_17)
```

Add include paths only through target interfaces where possible. Do not spread private source-directory knowledge into unrelated targets.

The probe is diagnostic and should not be started as a production service.

### 6. Do not register a duplicate ARM backend

Do not add ARM capability discovery to the backend registry as a second physical CPU resource.

The final shape remains:

```text
CpuBackend
  + generic CPU topology/health/capacity/lifecycle
  + ARM architecture capabilities and profiles when architecture is ARM

Independent optional backends
  + CUDA / future GPU backends
  + Hailo
  + IMX500
  + future accelerators
```

## Validation already completed

On the `curiosity` x86-64 validation host, repository-level CMake and CTest passed:

```text
prometheus_arm_capabilities
prometheus_arm_linux_auxv
```

Result:

```text
2/2 tests passed
100% tests passed
```

The processor-identity parser and heterogeneous grouping tests passed in an isolated strict-warning C++17 build.

## Required integration validation

Run after Codex stitching:

```bash
cmake --build <build-dir> \
  --target \
    test_arm_capabilities \
    test_arm_linux_auxv \
    test_arm_processor_identity \
    prometheus_arm_capability_probe \
  --parallel 1

ctest \
  --test-dir <build-dir> \
  --output-on-failure \
  -R 'prometheus_arm_(capabilities|linux_auxv|processor_identity)'
```

Expected:

```text
3/3 tests passed
100% tests passed
```

Run the probe on x86-64. It should report `not_arm` / unsupported architecture as a successful diagnostic outcome and must not attempt ARM identity parsing.

Run the probe on at least one AArch64 Linux node. Confirm:

- execution state is `aarch64`;
- target logical processor IDs match the generic CPU target set;
- `advanced_simd` reflects HWCAP ASIMD;
- profile selection is conservative;
- identity groups correspond to `/proc/cpuinfo` records;
- missing fields appear as issues, not zero-valued healthy claims;
- no camera, accelerator, or Raspberry Pi assumptions appear in output.

When available, also validate on:

- heterogeneous ARM hardware;
- an ARM server with multiple packages or NUMA nodes;
- AArch32 Linux;
- AArch64 with SVE/SVE2;
- ARM hardware with a discrete GPU, confirming CPU and GPU resources remain independently represented.

## Acceptance checklist

- [x] ARM CPU is represented once through the generic CPU backend.
- [x] Auxiliary-vector observations compile on non-ARM Linux.
- [x] Missing HWCAP/HWCAP2 remains unknown rather than unsupported.
- [x] Identity parser retains only target logical processors.
- [x] Multiple complete signatures produce multiple groups.
- [x] Processor signatures do not select instruction implementations.
- [x] Capability and identity providers preserve their separate partial states.
- [x] All three ARM tests pass in the integrated tree.
- [x] Probe builds and runs on x86-64.
- [ ] Probe runs on real AArch64 hardware.
- [x] No Hailo, IMX500, camera, Raspberry Pi, or GPU assumption is introduced.
- [x] No worker-pool, scheduling, backend-registry, or robot behavior regression is introduced.

## Phase boundary

Completion of this handoff closes ARM Phase A1 implementation scope.

The next phase should begin ARM operation-dispatch foundations: scalar fallback, runtime NEON selection, typed operation descriptors, alignment requirements, and proof-based execution readiness. It should not begin by assuming any particular board, attached accelerator, or permanent cognitive role.
