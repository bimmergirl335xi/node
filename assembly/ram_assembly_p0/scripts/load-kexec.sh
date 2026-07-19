#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=lib/p0-common.sh
source "${SCRIPT_DIR}/lib/p0-common.sh"

"${SCRIPT_DIR}/preflight.sh" --stage load
p0_export_ram_environment

kernel="$P0_ARTIFACTS_DIR/node-p0-bzImage"
initramfs="$P0_ARTIFACTS_DIR/node-p0-initramfs.cpio.gz"
validation_inputs="$P0_RECORDS_DIR/artifact-validation-inputs.sha256"
kexec_binary="$(p0_resolve_system_binary /usr/sbin/kexec /sbin/kexec /usr/bin/kexec)"
p0_verify_fixed_artifact "$kernel"
p0_verify_fixed_artifact "$initramfs"
[[ -s "$validation_inputs" ]] || p0_die "artifact-validation record unavailable; run validate-artifacts.sh first"
(
    cd -- "$P0_ARTIFACTS_DIR"
    sha256sum --check kernel.sha256
    sha256sum --check initramfs.sha256
)
sha256sum --check "$validation_inputs"

default_cmdline='console=ttyS0,115200n8 console=tty0 rdinit=/init loglevel=6 node.p0.hold_seconds=20'
kernel_cmdline="${NODE_P0_KERNEL_CMDLINE:-$default_cmdline}"
[[ ${#kernel_cmdline} -le 2048 ]] || p0_die "kernel command line exceeds 2048 bytes"
case "$kernel_cmdline" in
    *$'\n'* | *$'\r'*) p0_die "kernel command line contains a line break" ;;
esac
printf '%s\n' "$kernel_cmdline" >"$P0_RECORDS_DIR/kernel-command-line.txt"

p0_info "loading candidate kernel into RAM only"
p0_info "kernel: $kernel"
p0_info "initramfs: $initramfs"
p0_info "command line: $kernel_cmdline"
p0_info "privileged input operation=kexec-load executable=$kexec_binary"
p0_info "privileged input kernel=$kernel"
p0_info "privileged input initramfs=$initramfs"
p0_info "privileged input command_line=$kernel_cmdline"
p0_run_privileged_command "$kexec_binary" -l "$kernel" \
    "--initrd=$initramfs" "--command-line=$kernel_cmdline"
{
    sha256sum "$kernel"
    sha256sum "$initramfs"
    sha256sum "$P0_RECORDS_DIR/kernel-command-line.txt"
} >"$P0_RECORDS_DIR/kexec-loaded-inputs.sha256"
p0_write_control_transfer_record loaded_not_executed \
    candidate_kernel_and_initramfs_loaded_in_memory
p0_record kexec_load satisfied "kernel=$kernel initramfs=$initramfs"
p0_info "candidate loaded; control has NOT transferred"
p0_info "explicit ordinary-user execute command: $P0_ROOT_DIR/scripts/execute-kexec.sh --i-understand-control-transfers-now"
