# Current State

This is a snapshot of the integrated repository, not a development diary.
Historical validation details remain available in Git history and retained
architecture/handoff documents.

## Current repository baseline

The normalization branch `dev` was created directly from `main` commit
`49bbba3d27c05e952307569668cea31aec98335f`. At branch creation,
`lane/runtime` resolved to the same commit, so no branch merge was needed.

That baseline combines the public runtime/ACS foundation, CPU and ARM work,
GPU inventory/evidence/execution slices, RAM assembly work, and the P01
micro-OS/image path. The cleanup on `dev` changes repository organization and
documentation only; it does not alter runtime behavior.

## Current boot path

The implemented candidate path is:

```text
selected external Linux kernel
    -> static C11 /init
    -> bounded volatile filesystems
    -> fixed P01 startup manifest
    -> supervised public proof services
    -> serial/JSONL evidence and bounded terminal action
```

`assembly/p01_boot/` builds candidate kernel/initramfs artifacts and, when the
required host tooling is available, an x86_64 GRUB hybrid ISO. It contains
inspection and direct/ISO QEMU validation paths. Missing GRUB, xorriso, or OVMF
support is reported as unavailable rather than passed.

The selected kernel source is external and pinned by the P01 tooling. Nothing
in the current public path installs to a host, writes removable media, changes
EFI variables, or grants an artifact production authority.

## Current micro-OS state

`assembly/micro_os/` contains the permanent P01 PID 1 foundation. It:

- mounts devtmpfs, procfs, sysfs, `/run`, and a bounded result tmpfs;
- parses only bounded `node.micro_os.*` options;
- validates the exact tracked startup manifest before launching services;
- uses deterministic stages, explicit dependencies, bounded concurrency,
  deadlines, restart limits, process groups, `signalfd`, and complete reaping;
- supervises exact executables under `/node/services/` without shell or `PATH`
  discovery;
- emits human-readable and JSONL evidence to console/serial channels.

The included services are conformance probes. They are not production CPU,
GPU, ACS, network, or application providers.

## Current assembly state

`assembly/ram_assembly_p0/` is a bounded tmpfs-only mechanism proof. It can
select an exact external kernel revision, resolve tracked configuration
fragments, build candidate kernel/initramfs outputs, validate records, and
prepare explicit kexec boundaries. It does not establish BOOT acceptance,
generation membership, installation, activation, recovery, or runtime
readiness.

The public external-component ABI and parser are in `interfaces/` and
`assembly/providers/`. A declaration may be absent or structurally validated;
neither outcome launches a component or grants trust, acceptance, authority, or
readiness. Public-only assembly remains a complete supported case.

## Current CPU backend state

The generic CPU backend provides:

- node-relative identity and Linux topology observations;
- configured, online, and process-allowed CPU accounting;
- x86 and ARM capability evidence with explicit partial/unknown states;
- utilization, frequency, load, memory, pressure, and thermal observations;
- health and capacity summaries plus advisory execution groups;
- a bounded priority worker pool with backpressure, task handles,
  cancellation, counters, restart, exception containment, and bounded shutdown
  waiting;
- conservative scalar/x86/ARM SIMD selection.

ARM auxiliary-vector and `/proc/cpuinfo` parsing enrich this backend. No second
ARM backend is registered. CPU affinity, NUMA execution-group pool collection,
cross-group work stealing, typed CPU kernels, and production CPU dispatch are
not implemented.

## Current GPU backend state

The generic Linux PCI inventory observes GPU-like hardware and driver binding
without CUDA. The evidence-correlation layer keeps physical hardware, kernel
driver, compiler/toolkit, runtime, CUDA-visible identity, binary coverage,
backend readiness, and admission separate.

The CUDA path includes stable UUID identity, capability/health observation,
device pooling, metadata-first kernel registration, bounded queues, typed
adapter registration, and a device-local worker. The worker currently proves a
small synthetic FP32 path and owns its stream/event/adapter cleanup on the
device thread.

This is not a production scheduler. General payload/memory-lease resolution,
automatic retry or migration, cross-device work stealing, production model
memory, MIG control, and broad neural execution are absent. CUDA remains
optional and an empty visible-device set is valid.

## Current hardware discovery state

Discovery is bounded and observational. Stable CPU/GPU identities are distinct
from logical CPU numbers and CUDA ordinals. Linux PCI evidence distinguishes
unbound, bound, and unknown driver states; CUDA evidence independently reports
runtime visibility and compatibility. Presence never implies readiness or
admission.

The repository contains early Hailo discovery/backend material, but HailoRT
loading and model execution are not established. IMX500, AMD GPU, Xeon Phi,
and other accelerator paths remain incomplete or placeholder-level.

## Current ACS state

`src/core/acs/` implements the ACS-I001 public runtime-local foundation:

- bounded identities, evidence, authority, and condition vocabulary;
- immutable descriptors and deterministic bounded registry snapshots;
- separately versioned lifecycle, operational, and enforcement transitions;
- bounded transition history and idempotency handling;
- pure metadata-only, non-reserving admission evaluation;
- transport-neutral binding and attachment snapshots, a bounded typed signal
  envelope, and pure structural validation that composes current registry,
  lifecycle, and admission evidence without treating validation as acceptance.

The public Draft ACS-0000 through ACS-0009 specifications are retained under
`docs/architecture/acs/`. The transport-facing additions define contracts only:
they perform no serialization, I/O, reservation, lifecycle mutation, or signal
delivery. Live connections, transport, network discovery, authentication
providers, resource reservation, persistence, descriptor removal, and a public
ACS C ABI are not implemented.

## Current kernel relationship

Linux kernel source and acceptance remain external to this repository. Node
records the selected source/revision/configuration and treats produced images as
candidates until the owning authority accepts them. The first P01 profile uses
tracked common, x86_64, and Dell Wyse 5070 configuration fragments; it is not a
universal hardware profile.

## Current testing and validation state

The repository contains CMake/CTest coverage for CPU, ARM, CUDA, hardware,
runtime, ACS, and assembly components plus P0/P01-specific validation tooling.
Earlier checkpoints recorded passing CPU-only, CUDA-enabled, repeated stress,
strict-warning, sanitizer, and QEMU runs on their documented hosts.

No build, test, benchmark, or QEMU boot was run during this repository
normalization. Current-machine validation is therefore `not run`; earlier
records should be treated as historical evidence only. `build/` was removed as
disposable generated state and will be recreated by future build tooling.

## Current development environment

The primary workflow is Linux-hosted and headless-friendly. CMake drives host
components, named directories beneath `build/` contain generated state, and
QEMU is the intended pre-hardware boot environment. Serial output is the
primary boot diagnostic surface. Docker is not required for normal development.

`scripts/build-node.sh` is the normalized host-native boot build entry point.
It orchestrates the existing P01 builder and its non-QEMU validation, keeps all
generated repository state beneath disposable `build/`, and safely promotes a
complete current ISO to `build/artifacts/node-current.iso`. It also exposes the
current kernel, initramfs, checksums, build identity, and existing kernel debug
artifacts when available. A failed or partial build does not replace the last
successfully promoted ISO; the exact Linux kernel source remains an external
checkout.

`scripts/run-node-qemu.sh` boots only that canonical ISO in a bounded,
headless TCG guest. It defaults to BIOS and optionally uses OVMF with a
disposable variable-store copy for UEFI. The guest has one vCPU, 512 MiB of
memory, serial stdio, no network interface, no virtual disk, and no persistent
VM state. The latest timestamped command, boot transcript, and exit status are
stored under `build/logs/qemu-last-run.log`. The launcher preserves the full
serial transcript and separately captures the micro-OS's existing bounded
JSON records at `build/logs/qemu-boot-events.jsonl`. Capture is limited to 256
events and 256 KiB; it observes the established P01 lifecycle without adding
new PID 1, runtime, or acceptance behavior.

`scripts/debug-node-qemu.sh` is the opt-in debug entry point. It reuses the
normal launcher and canonical ISO, requires the exposed `vmlinux`, starts QEMU
paused, and binds the GDB remote interface only to loopback. Attaching and
continuing preserves the same serial transcript, structured-event capture,
bounded timeout, firmware choices, and guest shutdown behavior. Normal QEMU
boot remains unpaused and exposes no debugger.

`python/node_lab.py` provides a bounded two-instance layer around those same
launchers. The tracked `node-001` and `node-002` profiles declare firmware,
vCPU count, memory, unique loopback debug settings, deterministic local MACs,
and reciprocal network peers. The controller can list, inspect, start, stop,
restart, and observe either instance; generated state and node-local
serial/JSONL logs remain disposable under `build/virtual/NODE/`. Stop
operations require a recorded PID plus matching process-start and QEMU command
identity, preventing a stale PID from authorizing a signal. Status reports
observed process and boot evidence only; it does not infer service health or
runtime readiness. The guests share one unprivileged QEMU Unix-datagram
point-to-point Ethernet segment using `virtio-net-pci`; no host bridge, TAP,
NAT, internet path, or persistent network state is created. The kernel already
contains the required virtio networking support. Guest IP configuration and
peer connectivity proof remain absent, as do orchestration, storage, ACS, and
a general virtual-node platform.

`main` is stable/public, `dev` is active integration, and specialized lanes own
subsystem work before integration. See `AGENTS.md` for branch rules.

## Known incomplete areas

- production Node service graph and runtime activation;
- migration of production robot behavior out of the legacy CUDA program;
- CPU affinity/NUMA orchestration and typed CPU kernel execution;
- general GPU payload and memory-lease execution;
- HailoRT, IMX500, AMD GPU, and Xeon Phi production backends;
- live ACS transport, discovery, authentication, persistence, and reservation;
- MEM persistence and IMM implementation;
- production BOOT acceptance, installation, recovery, and media workflows;
- real-hardware coverage across supported ARM and heterogeneous targets;
- externally selected project license.

## Next major development direction

The next implementation milestone must be authorized independently. The
current boundaries point toward deeper bounded CPU/GPU execution, live but
governed ACS integration, and promotion of the P01 candidate path into a
defined BOOT acceptance model. None should bypass explicit evidence, ownership,
authority, CPU-only support, or public/private separation.
