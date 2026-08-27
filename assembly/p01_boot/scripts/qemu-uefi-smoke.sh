#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "$0")/lib-p01.sh"

[[ $# -eq 1 ]] || p01_fail 'usage: qemu-uefi-smoke.sh OUTPUT_DIR'
output_dir=$(p01_assert_output_directory "$1")
image="${output_dir}/artifacts/node-p01-x86_64.iso"
validation="${output_dir}/validation"
result="${validation}/qemu-uefi.json"
mkdir -p -- "${validation}"

ovmf=''
for candidate in /usr/share/OVMF/OVMF_CODE.fd \
    /usr/share/edk2/x64/OVMF_CODE.fd \
    /usr/share/qemu/OVMF_CODE.fd; do
    if [[ -f ${candidate} ]]; then ovmf=${candidate}; break; fi
done
if ! command -v qemu-system-x86_64 >/dev/null 2>&1 ||
   [[ -z ${ovmf} || ! -f ${image} ]]; then
    printf '%s\n' \
        '{"test":"p01_qemu_uefi","status":"unavailable","reason":"QEMU, OVMF, or candidate ISO unavailable"}' \
        > "${result}"
    echo 'P01 UEFI smoke unavailable: QEMU, OVMF, or candidate ISO unavailable' >&2
    exit 77
fi
set +e
timeout --signal=TERM --kill-after=5s 60s qemu-system-x86_64 \
    -machine accel=tcg -cpu max -smp 1 -m 512M \
    -bios "${ovmf}" -cdrom "${image}" \
    -display none -serial stdio -monitor none -no-reboot \
    > "${validation}/qemu-uefi.log" 2>&1
qemu_status=$?
set -e
[[ ${qemu_status} -eq 0 ]] || p01_fail "UEFI QEMU returned ${qemu_status}"
grep -Fq '"outcome":"p01_terminal_state_reached"' \
    "${validation}/qemu-uefi.log" || p01_fail 'UEFI boot did not reach P01 terminal state'
printf '%s\n' \
    '{"test":"p01_qemu_uefi","status":"passed","acceleration":"tcg"}' \
    > "${result}"
echo 'P01 QEMU UEFI smoke passed'
