#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "$0")/lib-p01.sh"

[[ $# -eq 1 ]] || p01_fail 'usage: qemu-smoke.sh OUTPUT_DIR'
output_dir=$(p01_assert_output_directory "$1")
kernel="${output_dir}/artifacts/node-p01-bzImage"
initramfs="${output_dir}/artifacts/node-p01-initramfs.cpio.gz"
validation="${output_dir}/validation"
serial_log="${validation}/qemu-bios-direct.log"
result="${validation}/qemu-bios-direct.json"
mkdir -p -- "${validation}"

if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
    printf '%s\n' \
        '{"test":"p01_qemu_bios_direct","status":"unavailable","reason":"qemu-system-x86_64 not installed"}' \
        > "${result}"
    echo 'P01 QEMU smoke unavailable: qemu-system-x86_64 not installed' >&2
    exit 77
fi
p01_assert_regular_input "${kernel}"
p01_assert_regular_input "${initramfs}"
set +e
timeout --signal=TERM --kill-after=5s 45s qemu-system-x86_64 \
    -machine accel=tcg -cpu max -smp 1 -m 512M \
    -kernel "${kernel}" -initrd "${initramfs}" \
    -append 'console=ttyS0,115200n8 rdinit=/init loglevel=6 node.micro_os.hold_seconds=0 node.micro_os.terminal_action=poweroff node.micro_os.log_verbosity=1 node.micro_os.manifest=/etc/node-p01/p01-public-startup-v1.manifest node.micro_os.expected_boot_identity=node-p01-micro-os-v1' \
    -display none -serial stdio -monitor none -no-reboot \
    > "${serial_log}" 2>&1
qemu_status=$?
set -e

p01_validate_boot_log "${serial_log}" 'QEMU direct BIOS boot'
if [[ ${qemu_status} -ne 0 ]]; then
    p01_fail "QEMU returned status ${qemu_status} after evidence capture"
fi
printf '%s\n' \
    '{"test":"p01_qemu_bios_direct","status":"passed","acceleration":"tcg","claim":"kernel_initramfs_pair_only"}' \
    > "${result}"
echo 'P01 QEMU direct kernel/initramfs smoke passed'
