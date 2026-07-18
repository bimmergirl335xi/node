#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=lib/p0-common.sh
source "${SCRIPT_DIR}/lib/p0-common.sh"

[[ $# -eq 1 && "$1" == "--i-understand-control-transfers-now" ]] || p0_die \
    "explicit acknowledgement required: --i-understand-control-transfers-now"

"${SCRIPT_DIR}/preflight.sh" --stage execute
p0_export_ram_environment

kernel="$P0_ARTIFACTS_DIR/node-p0-bzImage"
initramfs="$P0_ARTIFACTS_DIR/node-p0-initramfs.cpio.gz"
cmdline_file="$P0_RECORDS_DIR/kernel-command-line.txt"
loaded_inputs="$P0_RECORDS_DIR/kexec-loaded-inputs.sha256"
p0_verify_fixed_artifact "$kernel"
p0_verify_fixed_artifact "$initramfs"
p0_assert_beneath "$cmdline_file" "$NODE_P0_WORKSPACE"
p0_assert_tmpfs "$cmdline_file"
[[ -s "$cmdline_file" ]] || p0_die "recorded kernel command line unavailable; run load-kexec.sh first"
[[ -s "$loaded_inputs" ]] || p0_die "loaded-input record unavailable; run load-kexec.sh first"
(
    cd -- "$P0_ARTIFACTS_DIR"
    sha256sum --check kernel.sha256
    sha256sum --check initramfs.sha256
)
sha256sum --check "$loaded_inputs"

kernel_cmdline="$(cat -- "$cmdline_file")"
printf '\nNODE_P0 IMMEDIATE CONTROL TRANSFER WARNING\n' >&2
printf 'kernel: %s\n' "$kernel" >&2
printf 'initramfs: %s\n' "$initramfs" >&2
printf 'command line: %s\n' "$kernel_cmdline" >&2
printf 'No persistent installation or bootloader mutation will be performed.\n\n' >&2

p0_record kexec_execute attempting "explicit_operator_acknowledgement=true"
sync -f "$P0_RECORDS_DIR/operations.jsonl" 2>/dev/null || true
kexec -e
p0_die "kexec returned unexpectedly"
