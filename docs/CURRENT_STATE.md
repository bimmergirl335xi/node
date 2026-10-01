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
    -> explicit conformance terminal action or scoped resident lab transition
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
- invokes the existing generic CPU providers on every boot and evaluates their
  bounded evidence through a BOOT-owned CPU-only component-set decision;
- emits human-readable and JSONL evidence to console/serial channels;
- selects explicit `conformance` or `lab` behavior without changing the image;
- remains the permanent PID 1 while supervising resident CPU and ACS services
  in lab mode, including bounded transition and shutdown handling.

The CPU decision records observation, normalized capabilities, selected SIMD
profile, `cpu.runtime=required`, and `gpu.runtime=not_required` as distinct
facts. For the current x86_64/AVX2 development profile, the canonical serial
image build produces one static CPU conformance candidate with bounded
provenance. Every guest independently consumes its exported decision,
materializes the candidate into volatile `/run`, validates its SHA-256, size,
ELF/ABI/profile properties, launches it for the current boot, and runs a
deterministic AVX2 probe through the existing bounded CPU thread pool after an
explicit test-generation-only BOOT acceptance. Conformance mode retains the
bounded terminal P01 path. In lab mode the accepted candidate replaces only
its assembly-service child process, retains a one-worker bounded pool, and
answers a distinct post-transition execution probe while the permanent PID 1
continues supervision. PID 1 admits that transition only after required
startup, compatible CPU decision, candidate validation/activation, initial
probe, and local ACS readiness are all observed; explicit mode additionally
requires its configured-peer exchange. The resulting readiness
claim is limited to the current boot, x86_64/AVX2 candidate, public ACS
reference transport, and lab profile. It adds no durable installation, reboot
continuation, GPU readiness, MEM/storage readiness, or production-wide
readiness claim.

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
  lifecycle, and admission evidence without treating validation as acceptance;
- a versioned, deterministic reference envelope representation bounded to
  8 KiB, with at most 256 inline bytes and four provenance references;
- a volatile, bounded participant-observation store using canonical
  `ParticipantId`, with direct/hint kinds, observed/stale/conflict states,
  monotonic current-boot timestamps, explicit capacity rejection, and no
  descriptor-graph mutation; and
- a separate version-1 pre-relationship discovery codec bounded to 1 KiB and
  eight hints for presence, response, hint-query, and hint-response messages.

The public Draft ACS-0000 through ACS-0009 specifications are retained under
`docs/architecture/acs/`. The managed development image supports two narrow
reference modes. Explicit mode uses configured registry declarations, real
lifecycle transitions, advisory admission, active peer-specific
binding/attachment snapshots, bounded UDP serialization, and receiver-side
validation across the isolated five-node lab LAN. Discovery mode requires zero
configured remote peers, binds guest-local multicast `239.77.0.1:39002`, and
continuously maintains bounded direct/hinted observations with a two-second
presence interval and six-second stale threshold. Linux's current boot ID is
hashed into a non-cryptographic restart epoch. Discovery initializes and may
remain resident with zero remote participants; observations never create
relationships, connections, attachments, capabilities, trust, or authority.
Neither mode is production-secure, authenticated, routed, persistent, private
ACS behavior, or a public ACS C ABI.

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

The ACS-001A validation on the development host passed all nine focused public
ACS tests and the focused Python profile/controller tests. A three-node lab
booted with zero configured remote peers: all three reached scoped runtime
readiness, remained under PID 1, and directly observed both others. After
node-003 stopped, node-001 and node-002 remained ready and marked it stale;
after restart both emitted stale-to-observed rediscovery and node-003 observed
both survivors. The existing five-node explicit mode then retained 4/4 valid
peers per node (20/20 directed paths), and all five passed CPU activation,
resident transition, and post-transition probes.

DEV-004A validation scaled the same discovery path to ten concurrently resident
managed guests. All ten reached ACS and runtime readiness, activated their
profile-driven CPU runtime, passed the post-transition probe, and directly
observed the other nine participants for 90 directed volatile observations.
The original heterogeneous topology remained intact, and nodes 006–010 ran as
one-vCPU guests with 256 MiB each. Stopping node-007 made its observations stale
without degrading the surviving runtimes; restart restored its nine direct
observations and produced stale-to-observed rediscovery on sampled survivors.
The controller now reports discovery evidence separately from retained
explicit-peer requirements. These observations created no trust, canonical
relationship, connection authority, mesh route, or persistent membership; the
five-node explicit mode remains the deterministic conformance path.

DEV-004B extends the tracked host-side lab configuration to 25 discovery-mode
profiles. Nodes 011–025 use one vCPU, 256 MiB, and no explicit peers. A complete
25-node run would yield 24 direct observations per Node and 600 directed
volatile observations fleet-wide; that runtime scale result has not yet been
validated and is not claimed here.

The earlier DEV-003C validation on the development host rebuilt the canonical
image serially, passed all 39 host CTest cases and the strict-warning build,
reconfirmed the direct BIOS conformance boot and poweroff path, and exercised
all five managed guests in resident lab mode. All five reached scoped runtime
readiness, passed their post-transition CPU probes and complete ACS exchange,
remained healthy through a measured resident interval, isolated a single-node
stop, and accepted that node's fresh restart and ACS rejoin. These are bounded
development results for the current host and image, not production authority.

DEV-003D host validation passed the focused GDB/MI controller tests and all 39
existing CTest cases, attached the five paused managed guests on distinct
loopback endpoints, and exercised broadcast, single-node, and explicit-set
control. It observed attributed asynchronous stops, hit a stable relocated
kernel symbol independently on all five nodes, contained one disconnected
guest, remained usable after resident transition, and left every remaining VM
running when the debugger frontend exited.

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

`python/node_gdb_fleet.py` is the host-side interactive frontend for managed
debug guests. It reuses `node_lab.py` profile discovery and exact managed-VM
identity checks, accepts all configured profiles or an explicit subset, and
caps a fleet at 50 sessions. Each selected node receives one owned GDB MI3
subprocess using the single canonical `build/artifacts/vmlinux`; a reader
thread continuously consumes bounded MI records and updates explicit starting,
connecting, stopped, running, disconnected, exited, error, or unknown state.
Commands may target all nodes, one persistent selection, or a small explicit
node set, and failures remain per-node. Debugger exit reaps owned GDB processes
without stopping QEMU. The controller adds no guest agent, guest runtime
behavior, remote listener, per-node symbols, or persistent debugger state.

`python/node_lab.py` provides a bounded twenty-five-instance layer around those same
launchers. The tracked `node-001` through `node-025` profiles declare firmware,
explicit socket/core/thread topology and vCPU count, memory, unique loopback
debug settings, deterministic local MAC and
IPv4 addresses and a default ACS mode. The original five profiles retain their
explicit conformance peer sets, while the twenty scale profiles require no configured
remote peers. The controller can list, inspect,
start, stop, restart, and observe individual instances, and can start, stop, or
restart the profile set concurrently; generated state and node-local
serial/JSONL logs remain disposable under `build/virtual/NODE/`. Stop
operations require a recorded PID plus matching process-start and QEMU command
identity, preventing a stale PID from authorizing a signal. `--acs-mode
discovery` passes no remote peers; `--acs-mode explicit` retains the complete
peer set and its original SMBIOS v1 representation. Status reports
observed process and structured boot/runtime evidence only. `runtime-status`
and `runtime-summary` require explicit transition, readiness, resident CPU/ACS,
and post-transition-probe records; they do not infer missing health. The guests
share one unprivileged QEMU datagram-multicast
Ethernet segment bound to host loopback using `virtio-net-pci`; no host bridge, TAP,
NAT, internet path, or persistent network state is created. The kernel already
contains the required virtio networking support. A managed profile selector is
passed explicitly through bounded QEMU DMI metadata; the static conformance
service creates separate binding, attachment, admission, send/receive, signal,
and correlation evidence for each of the four peers per node. It uses profile
addresses `10.77.0.1/24` through `10.77.0.5/24`, UDP port `39001`, and attempts
the complete 20-direction matrix. DMI, MAC, and IP
data select only this declared development profile and do not prove logical
identity, trust, or authority. `node_lab.py acs-events NODE` surfaces the
resulting public evidence without making a policy judgment, while
`node_lab.py discovery NODE` filters bounded discovery transitions. The guest
discovery group and port are distinct from both the host-side QEMU Ethernet
multicast mechanism and the explicit ACS unicast port. These remain disposable
reference transports, not a cluster or ACS control plane.

Managed start/restart commands accept explicit `conformance` and `lab` modes
and explicit/discovery ACS modes;
conformance remains the default. Both use the one canonical ISO. Lab mode keeps
CPU and ACS children under permanent PID 1 supervision after a pure bounded
transition evaluation. ACS now launches in `platform_observation` after local
identity/filesystem readiness, before CPU assembly and activation; discovery
mode marks local readiness immediately after its socket and multicast
membership are active. An unexpected
essential-child exit is observed as runtime degradation. The host controller's
normal stop remains exact-identity QEMU termination: PID 1 contains a bounded
signal-driven resident shutdown path, but this phase adds no guest control
protocol and therefore does not claim that host-initiated stops exercise it.

`node_lab.py cpu-info NODE` reads the bounded node-local CPU JSONL records and
reports only explicit observations and decisions. `cpu-summary` compares actual
package, core, logical-processor, SMT, allowed-CPU, resident-worker, and probe
evidence across the managed fleet without inferring missing evidence. The
resident pool uses one bounded worker per freshly observed process-allowed CPU,
including the valid one-worker fallback. CPU inspection remains
separate from runtime selection, assembly, activation, and readiness.
`cpu-runtime NODE` and `cpu-runtime-summary` similarly expose the distinct
build request, candidate, validation, activation, and probe outcomes. They do
not convert successful conformance execution into Node runtime readiness. Only
the explicit lab transition produces the scoped current-boot readiness record.

`main` is stable/public, `dev` is active integration, and specialized lanes own
subsystem work before integration. See `AGENTS.md` for branch rules.

## Known incomplete areas

- production Node service graph, restart policy, and general runtime activation;
- migration of production robot behavior out of the legacy CUDA program;
- CPU affinity/NUMA orchestration and typed CPU kernel execution;
- general GPU payload and memory-lease execution;
- HailoRT, IMX500, AMD GPU, and Xeon Phi production backends;
- production ACS transport, authenticated or wide-area discovery, secure
  sessions, persistence, and reservation;
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
