# P01 bootable micro-OS candidate

P01B builds ordinary files beneath a named ignored `build/` directory. It
never accepts a device path, invokes a media writer, requires `sudo`, updates a
host bootloader, modifies EFI variables, or mutates an installation target.

The selected first format is an x86_64 GRUB hybrid ISO when the host provides
both `grub-mkrescue` and `xorriso`. GRUB's emitted BIOS and UEFI structures are
claimed only after the resulting ISO is inspected and the corresponding QEMU
boot is observed. Missing tools produce machine-readable `unavailable`
results, never a fabricated pass.

## Exact kernel boundary

The kernel source remains outside this repository:

```text
repository: git@github.com:bimmergirl335xi/Linux-kernel-node-runtime.git
revision:   94515f3a7d4256a5062176b7d6ed0471938cd51a
```

The builder requires an exact clean checkout at that revision, resolves an
x86_64 configuration from `x86_64_defconfig` plus the tracked P0 common,
x86_64, and Dell Wyse 5070 fragments, builds only `bzImage` with `-j1`, and
records the resolved configuration and artifact digests. The source is not
vendored or copied into the image.

## Non-privileged build

From the Node repository root:

```sh
make -C assembly/p01_boot validate \
  OUTPUT_DIR="$PWD/build/p01-candidate" \
  KERNEL_SOURCE=/exact/path/to/Linux-kernel-node-runtime \
  KERNEL_REVISION=94515f3a7d4256a5062176b7d6ed0471938cd51a
```

The stages build and inspect the permanent micro-OS, initramfs, reviewed
kernel, image tree, candidate ISO when supported, digests, five independent
candidate records, direct TCG smoke evidence, and optional OVMF evidence.

The direct QEMU proof uses the kernel/initramfs pair and therefore proves BIOS
firmware-independent kernel entry rather than GRUB BIOS boot. BIOS bootloader
support requires ISO inspection and a QEMU ISO boot; UEFI support additionally
requires OVMF. Those distinctions remain explicit in validation records.

## Output and authority boundary

Expected outputs are retained below `OUTPUT_DIR`:

```text
artifacts/node-p01-bzImage
artifacts/node-p01-initramfs.cpio.gz
artifacts/node-p01-x86_64.iso       (when image tools are available)
artifacts/SHA256SUMS
records/*.json
validation/*.json
validation/*.log
```

All are ASM candidate outputs under
`contracts/p01-conformance-authority-v1.json`. Digests, inspection, and QEMU
boot do not create artifact acceptance, assembly-generation membership,
installation eligibility, activation, recovery, or runtime readiness.

Physical removable-media writing is operator-only and deliberately absent
from every P01 script. Any future write must use an operator-reviewed stable
device identity and must never be inferred from a transient `/dev/sdX` name.
