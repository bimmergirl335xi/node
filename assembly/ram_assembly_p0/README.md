# Node RAM Assembly P0

This directory contains a bounded, RAM-only mechanism proof for building and
entering a candidate Linux environment. It is intentionally narrower than
Node BOOT or a production assembly system.

The proof may establish only that:

- RAM-backed assembly mechanisms operated;
- a candidate kernel output was produced;
- a candidate initramfs output was produced;
- control reached the candidate environment;
- the bundled C tests executed.

It does **not** establish that Node was assembled, that an artifact or assembly
generation was accepted, that installation or activation completed, that
recovery completed, or that the normal runtime is ready.

## Safety boundary

All mutable build, source-checkout, tool-cache, temporary, test, evidence, and
artifact paths are redirected beneath `NODE_P0_WORKSPACE`. Every operational
script revalidates that its mutable paths are on `tmpfs`.

Strict mode refuses to proceed while any swap device or file is active. It
also refuses a workspace mounted read-only or with `noexec`. The proof never
writes `/boot`, runs `make install`, runs host `modules_install`, changes a
bootloader, modifies EFI variables, partitions media, installs packages, or
automatically executes `kexec -e`.

Run every P0 orchestration script as the ordinary user. Never invoke a complete
P0 script with `sudo` or `sudo -E`. When a leaf operation genuinely needs
privilege, the ordinary-user script prints its fully quoted command and all
inputs, then invokes only that operation through `sudo`, `/usr/bin/env -i`, an
explicit minimal `PATH` and locale, and a fully resolved system-binary path.

The only privileged leaf operations are:

- creation of the dedicated tmpfs mount-point directory, when absent;
- mounting that dedicated tmpfs with the ordinary user's numeric UID and GID;
- `kexec -l` after all inspection, validation, command-line construction, and
  hashing have completed as the ordinary user;
- `kexec -e` after ordinary-user revalidation and the explicit acknowledgement.

Swap changes are not implemented here. They remain a separate operator action;
P0 only reads `/proc/swaps` and refuses to continue while swap is active.

The default workspace is:

```text
/run/node-assembly-p0
```

On machines where `/run` is too small or mounted `noexec`, create a dedicated
tmpfs at that path. `create-ram-workspace.sh --mount` performs only that
volatile mount and refuses to replace an existing non-tmpfs mount.

## Dell Wyse 5070 profile direction

The first target is a Dell Wyse 5070. The configuration resolver does not
assume its exact SKU. It records the actual CPU and PCI evidence on the machine
that performs the build, starts from the running distribution configuration
when available, and applies three independently replaceable fragments:

```text
config/common.config
config/x86_64.config
config/dell_wyse_5070.config
```

The Wyse fragment favors the adaptable x86_64 baseline and built-in Intel i915,
simple framebuffer, input, storage, and common wired-network mechanisms. It is
a first-proof profile, not a permanent universal Node kernel profile.

## Required preinstalled tools

The proof installs nothing. The host must already provide the Linux kernel
build prerequisites, a C compiler capable of static linking, `git`, `cpio`,
`gzip`, `findmnt`, `sudo` for individual privilege boundaries, and `kexec` for
the load stage. The preflight script reports missing tools explicitly.

## Operator sequence

Set the desired tmpfs size for the target's installed RAM. Kernel compilation
normally needs several GiB; leave enough memory for the running system and the
failure-reporting reserve.

```sh
export NODE_P0_WORKSPACE=/run/node-assembly-p0
assembly/ram_assembly_p0/scripts/create-ram-workspace.sh \
  --mount --size-mib 6144
```

Disable swap through the machine's existing operator procedure, then verify
the strict build preflight. This project does not disable swap automatically.

```sh
assembly/ram_assembly_p0/scripts/preflight.sh --stage build
```

Copy an existing local checkout into RAM:

```sh
assembly/ram_assembly_p0/scripts/fetch-kernel-source.sh \
  --local /path/to/Linux-kernel-node-runtime \
  --ref <reviewed-commit>
```

Or clone the reviewed source directly into RAM:

```sh
assembly/ram_assembly_p0/scripts/fetch-kernel-source.sh \
  --url git@github.com:bimmergirl335xi/Linux-kernel-node-runtime.git \
  --ref <reviewed-commit>
```

An explicitly supplied Git source bundle is also accepted without vendoring
kernel source in this repository:

```sh
assembly/ram_assembly_p0/scripts/fetch-kernel-source.sh \
  --bundle /path/to/reviewed-kernel-source.bundle \
  --ref <reviewed-commit>
```

Resolve and verify the candidate configuration, build serially, then create
and inspect the initramfs:

```sh
assembly/ram_assembly_p0/scripts/configure-kernel.sh
assembly/ram_assembly_p0/scripts/build-kernel.sh
assembly/ram_assembly_p0/scripts/build-initramfs.sh
assembly/ram_assembly_p0/scripts/validate-artifacts.sh
```

`build-initramfs.sh` normally runs with no external component declaration. To
stage one structurally validated candidate manifest, supply exactly one input:

```sh
assembly/ram_assembly_p0/scripts/build-initramfs.sh \
  --external-component-manifest /exact/path/to/manifest
```

or set `NODE_EXTERNAL_COMPONENT_MANIFEST` before running the script. Supplying
both is rejected. The validated manifest is copied into RAM as inert candidate
data. PID 1 does not read or execute it, and validation grants no acceptance,
generation membership, authority, activation, or readiness.

Loading and executing are separate boundaries:

```sh
assembly/ram_assembly_p0/scripts/load-kexec.sh
assembly/ram_assembly_p0/scripts/execute-kexec.sh \
  --i-understand-control-transfers-now
```

`load-kexec.sh` loads only the fixed kernel and initramfs paths whose digests
were produced inside the verified tmpfs. All preparation runs as the ordinary
user; only the printed, fully resolved `kexec -l` leaf command is elevated. It
never executes the candidate.

`execute-kexec.sh` revalidates tmpfs, swap, digests, and the command line,
prints an immediate-transfer warning and exact clean-environment `kexec -e`
command, and requires the explicit acknowledgement argument shown above. Only
that final leaf command is elevated. Codex must never invoke it unattended.

## Candidate environment

The initramfs requires no shell. Static `/init` mounts devtmpfs, procfs, sysfs,
and a writable tmpfs work area, opens `/dev/console`, reports the running kernel,
launches the bounded C test runner, and halts or powers off after a bounded
observation period.

The manifest exercises:

- monotonic clock access;
- anonymous memory write/read;
- `/proc` availability;
- `/sys` availability;
- fork/exec/wait behavior;
- timeout enforcement through an intentional long-running payload;
- a deliberate semantic-failure payload;
- explicit launch-failure, signal-termination, and non-semantic-exit paths.

The timeout and semantic-failure entries are expected negative-path results.
They remain counted and visible as timeouts/failures, while the final mechanism
outcome records whether each declared result was observed as expected.

Additional already-built static C payloads can be copied into the RAM
initramfs without changing PID 1 or the runner. Set both
`NODE_P0_EXTRA_TESTS_DIR` and `NODE_P0_EXTRA_TESTS_MANIFEST` before
`build-initramfs.sh`. The builder bounds the addition to 16 executable files,
rejects unsafe names and dynamic interpreters, and copies the inputs into RAM
before packaging.

The default kernel command line sends output to both a conventional serial
console and the physical monitor, with `tty0` last so `/dev/console` resolves to
the monitor:

```text
console=ttyS0,115200n8 console=tty0 rdinit=/init loglevel=6 node.p0.hold_seconds=20
```

Set `NODE_P0_KERNEL_CMDLINE` before `load-kexec.sh` to override it. A hold value
is bounded by PID 1 to 600 seconds.

## Host-side validation

Repository-side validation does not perform a RAM-only kernel build or load
`kexec`. It compiles every C source statically with strict warnings, checks
shell syntax, packages and inspects a host initramfs, runs the exact manifest
parser/result engines, exercises the no-external-manifest path, rejects
malformed and incompatible external declarations, and proves the expected
timeout and intentional failure:

```sh
make -C assembly/ram_assembly_p0 validate
```

Generated host-validation files go to the repository's already ignored named
build tree. They are not P0 candidate outputs.
