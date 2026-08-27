#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "$0")/lib-p01.sh"

[[ $# -eq 1 ]] || p01_fail 'usage: qemu-bios-iso-smoke.sh OUTPUT_DIR'
output_dir=$(p01_assert_output_directory "$1")
image="${output_dir}/artifacts/node-p01-x86_64.iso"
validation="${output_dir}/validation"
result="${validation}/qemu-bios-iso.json"
serial_log="${validation}/qemu-bios-iso.log"
mkdir -p -- "${validation}"

if ! command -v qemu-system-x86_64 >/dev/null 2>&1 || [[ ! -f ${image} ]]; then
    printf '%s\n' \
        '{"test":"p01_qemu_bios_iso","status":"unavailable","reason":"QEMU or candidate ISO unavailable"}' \
        > "${result}"
    echo 'P01 BIOS ISO smoke unavailable: QEMU or candidate ISO unavailable' >&2
    exit 77
fi
set +e
timeout --signal=TERM --kill-after=5s 60s qemu-system-x86_64 \
    -machine accel=tcg -cpu max -smp 1 -m 512M \
    -cdrom "${image}" -boot d \
    -display none -serial stdio -monitor none -no-reboot \
    > "${serial_log}" 2>&1
qemu_status=$?
set -e
[[ ${qemu_status} -eq 0 ]] || p01_fail "BIOS ISO QEMU returned ${qemu_status}"
p01_validate_boot_log "${serial_log}" 'QEMU BIOS ISO boot'
printf '%s\n' \
    '{"test":"p01_qemu_bios_iso","status":"passed","acceleration":"tcg","firmware":"seabios"}' \
    > "${result}"
echo 'P01 QEMU BIOS ISO smoke passed'
