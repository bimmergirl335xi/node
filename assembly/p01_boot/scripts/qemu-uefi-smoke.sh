#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "$0")/lib-p01.sh"

[[ $# -eq 1 ]] || p01_fail 'usage: qemu-uefi-smoke.sh OUTPUT_DIR'
output_dir=$(p01_assert_output_directory "$1")
image="${output_dir}/artifacts/node-p01-x86_64.iso"
validation="${output_dir}/validation"
result="${validation}/qemu-uefi.json"
serial_log="${validation}/qemu-uefi.log"
variable_state="${validation}/qemu-uefi-vars.fd"
mkdir -p -- "${validation}"

ovmf_code=''
ovmf_vars=''
while IFS='|' read -r code_candidate vars_candidate; do
    if [[ -f ${code_candidate} && -f ${vars_candidate} ]]; then
        ovmf_code=${code_candidate}
        ovmf_vars=${vars_candidate}
        break
    fi
done <<'EOF'
/usr/share/OVMF/OVMF_CODE_4M.fd|/usr/share/OVMF/OVMF_VARS_4M.fd
/usr/share/OVMF/OVMF_CODE.fd|/usr/share/OVMF/OVMF_VARS.fd
/usr/share/edk2/x64/OVMF_CODE.fd|/usr/share/edk2/x64/OVMF_VARS.fd
/usr/share/qemu/OVMF_CODE.fd|/usr/share/qemu/OVMF_VARS.fd
EOF
if ! command -v qemu-system-x86_64 >/dev/null 2>&1 ||
   [[ -z ${ovmf_code} || ! -f ${image} ]]; then
    printf '%s\n' \
        '{"test":"p01_qemu_uefi","status":"unavailable","reason":"QEMU, OVMF, or candidate ISO unavailable"}' \
        > "${result}"
    echo 'P01 UEFI smoke unavailable: QEMU, OVMF, or candidate ISO unavailable' >&2
    exit 77
fi
cp -- "${ovmf_vars}" "${variable_state}"
chmod u+w -- "${variable_state}"
set +e
timeout --signal=TERM --kill-after=5s 60s qemu-system-x86_64 \
    -machine accel=tcg -cpu max -smp 1 -m 512M \
    -drive "if=pflash,format=raw,readonly=on,file=${ovmf_code}" \
    -drive "if=pflash,format=raw,file=${variable_state}" \
    -cdrom "${image}" -boot d \
    -display none -serial stdio -monitor none -no-reboot \
    > "${serial_log}" 2>&1
qemu_status=$?
set -e
[[ ${qemu_status} -eq 0 ]] || p01_fail "UEFI QEMU returned ${qemu_status}"
p01_validate_boot_log "${serial_log}" 'QEMU UEFI ISO boot'
printf '%s\n' \
    '{"test":"p01_qemu_uefi","status":"passed","acceleration":"tcg"}' \
    > "${result}"
echo 'P01 QEMU UEFI smoke passed'
