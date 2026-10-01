# Node — Project Navigation

Use this routing map to find the owner of a task. It intentionally omits most
individual files and all generated output.

## Start here

- public overview and supported workflow: `README.md`
- agent operating rules: `AGENTS.md`
- current integrated implementation: `docs/CURRENT_STATE.md`
- durable architecture context: `AI_CONTEXT.md`
- build configuration: `CMakeLists.txt`, `cmake/`

## Boot and micro-OS

- overall assembly boundary: `assembly/README.md`
- permanent PID 1, startup manifest, and BOOT-owned first-boot CPU requirement
  evaluator: `assembly/micro_os/`
- P01 candidate image and QEMU proof: `assembly/p01_boot/`
- PID 1 sequencing contract: `assembly/init/`
- RAM-only kernel/initramfs proof: `assembly/ram_assembly_p0/`
- public profiles and requirements: `assembly/profiles/`,
  `assembly/requirements/`

Linux kernel source is external. Do not look for or vendor it here.

## Assembly providers and public interfaces

- external-component C ABI: `interfaces/`
- public manifest format: `assembly/manifests/public/`
- parser/validator providers: `assembly/providers/`
- assembly conformance tests: `assembly/tests/`

## CPU backend

- generic CPU identity, topology, capability, health, capacity, worker pool,
  and SIMD selection: `src/backends/cpu/`
- ARM-specific evidence enrichment: `src/backends/arm/`
- backend registry/contracts: `src/backends/`
- focused tests: `tests/unit/backends/`

## GPU and CUDA backend

- generic Linux PCI inventory and evidence correlation: `src/hardware/`
- CUDA backend, queues, registry, adapters, and worker: `src/backends/cuda/`
- CUDA kernels: `src/kernels/`
- hardware tests: `tests/unit/hardware/`
- backend and worker tests: `tests/unit/backends/`
- kernel tests: `tests/unit/kernels/`

CUDA-independent hardware questions should start in `src/hardware/`, not the
CUDA backend.

## Other hardware backends

- accelerator abstractions and early Hailo work: `src/backends/accelerator/`
- AMD, Phi, and storage backend areas: `src/backends/amd/`,
  `src/backends/phi/`, `src/backends/storage/`
- hardware profiles/configuration: `config/hardware_profiles/`

Treat empty or placeholder files as unimplemented.

## Runtime and services

- runtime/service lifecycle and shared state: `src/core/`
- ACS runtime-local foundation: `src/core/acs/`
- service implementations: `src/services/`
- scheduling/runtime helpers: `src/runtime/`
- runtime/core tests: `tests/unit/core/`

## Data, messaging, and mesh areas

- memory structures: `src/memory/`
- messaging and protocol: `src/messaging/`, `src/protocol/`
- mesh: `src/mesh/`
- storage: `src/storage/`
- diagnostics: `src/diagnostics/`
- security/provisioning: `src/security/`, `src/provisioning/`

Several of these areas remain partial or placeholder-level; confirm their state
in source and `docs/CURRENT_STATE.md` before relying on them.

## Public architecture

- ACS specifications: `docs/architecture/acs/`
- memory specifications: `docs/architecture/memory/`
- immune specifications: `docs/architecture/immune/`
- dependency boundaries: `docs/development/dependencies.md`

These specifications may describe planned behavior. They do not prove an
implementation exists.

## Legacy and migration reference

- current production robot behavior: `legacy/vision_swarm_11.cu`
- retained historical ARM handoff: `docs/handoffs/ARM_A1_CODEX_HANDOFF.md`

Historical handoffs are evidence, not current authority.

## Developer tooling

- general tools and probes: `tools/`
- shell helpers: `scripts/`
- Python utilities and the bounded five-node local VM/CPU-evidence controller:
  `python/`
- managed development-instance and private-link profiles:
  `config/virtual_nodes/`
- simulation code: `simulator/`
- test fixtures and failure scenarios: `tests/fixtures/`, `tests/failure/`

## Generated output

- all build products, CMake state, images, validation logs, and temporary
  staging: `build/`
- operator-local inputs: `.node-local/`
- private runtime material: `node-private-runtime/`

These paths are ignored and non-authoritative. `build/` may be deleted in full
and recreated by future tooling.
