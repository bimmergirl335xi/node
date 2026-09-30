#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
REPO_ROOT=$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel)
ISO_PATH="${REPO_ROOT}/build/artifacts/node-current.iso"
LOG_DIR="${REPO_ROOT}/build/logs"
LOG_PATH="${LOG_DIR}/qemu-last-run.log"
TEMP_DIR="${REPO_ROOT}/build/temp"
QEMU_BIN=${NODE_QEMU_BIN:-qemu-system-x86_64}
TIMEOUT_SECONDS=${NODE_QEMU_TIMEOUT_SECONDS:-90}
FIRMWARE=bios

fail() {
    echo "Node QEMU error: $*" >&2
    return 1
}

usage() {
    cat <<'EOF'
usage: scripts/run-node-qemu.sh [--bios | --uefi]

Boot build/artifacts/node-current.iso in a disposable, headless QEMU guest.
BIOS is the default. UEFI requires a matching OVMF code and variable template.

Environment:
  NODE_QEMU_BIN              QEMU executable (default: qemu-system-x86_64)
  NODE_QEMU_TIMEOUT_SECONDS  bounded run timeout from 1 through 600 (default: 90)
  NODE_QEMU_OVMF_CODE        optional OVMF code image override for --uefi
  NODE_QEMU_OVMF_VARS        optional OVMF variable template override for --uefi
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --bios)
            FIRMWARE=bios
            ;;
        --uefi)
            FIRMWARE=uefi
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            usage >&2
            exit 64
            ;;
    esac
    shift
done

[[ ${TIMEOUT_SECONDS} =~ ^[1-9][0-9]{0,2}$ ]] &&
    (( TIMEOUT_SECONDS <= 600 )) ||
    fail 'NODE_QEMU_TIMEOUT_SECONDS must be an integer from 1 through 600'
command -v "${QEMU_BIN}" >/dev/null 2>&1 ||
    fail "QEMU executable not found: ${QEMU_BIN}"
[[ -f ${ISO_PATH} && ! -L ${ISO_PATH} && -s ${ISO_PATH} ]] ||
    fail "canonical ISO is missing or empty; run ./scripts/build-node.sh first: ${ISO_PATH}"

mkdir -p -- "${LOG_DIR}" "${TEMP_DIR}"

qemu_command=(
    "${QEMU_BIN}"
    -no-user-config
    -machine accel=tcg
    -cpu max
    -smp 1
    -m 512M
    -cdrom "${ISO_PATH}"
    -boot order=d
    -nic none
    -display none
    -serial stdio
    -monitor none
    -no-reboot
)

uefi_vars_copy=''
if [[ ${FIRMWARE} == uefi ]]; then
    ovmf_code=${NODE_QEMU_OVMF_CODE:-}
    ovmf_vars=${NODE_QEMU_OVMF_VARS:-}

    if [[ -n ${ovmf_code} || -n ${ovmf_vars} ]]; then
        [[ -n ${ovmf_code} && -n ${ovmf_vars} ]] ||
            fail 'both NODE_QEMU_OVMF_CODE and NODE_QEMU_OVMF_VARS are required'
    else
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
    fi

    [[ -f ${ovmf_code:-} && -f ${ovmf_vars:-} ]] ||
        fail 'matching OVMF code and variable-template files were not found'

    uefi_vars_copy=$(mktemp "${TEMP_DIR}/qemu-uefi-vars.XXXXXX.fd")
    trap 'rm -f -- "${uefi_vars_copy}"' EXIT
    cp -- "${ovmf_vars}" "${uefi_vars_copy}"
    chmod u+w -- "${uefi_vars_copy}"
    qemu_command+=(
        -drive "if=pflash,format=raw,readonly=on,file=${ovmf_code}"
        -drive "if=pflash,format=raw,file=${uefi_vars_copy}"
    )
fi

started_at=$(date -u '+%Y-%m-%dT%H:%M:%SZ')
{
    printf 'Node QEMU development boot\n'
    printf 'started_utc: %s\n' "${started_at}"
    printf 'firmware: %s\n' "${FIRMWARE}"
    printf 'iso: %s\n' "${ISO_PATH}"
    printf 'timeout_seconds: %s\n' "${TIMEOUT_SECONDS}"
    printf 'qemu_command:'
    printf ' %q' "${qemu_command[@]}"
    printf '\n--- boot output ---\n'
} > "${LOG_PATH}"

set +e
timeout --signal=TERM --kill-after=5s "${TIMEOUT_SECONDS}s" \
    "${qemu_command[@]}" 2>&1 | tee -a "${LOG_PATH}"
qemu_status=${PIPESTATUS[0]}
set -e

finished_at=$(date -u '+%Y-%m-%dT%H:%M:%SZ')
{
    printf '%s\n' '--- run result ---'
    printf 'finished_utc: %s\n' "${finished_at}"
    printf 'qemu_exit_status: %s\n' "${qemu_status}"
} | tee -a "${LOG_PATH}"

if [[ ${qemu_status} -ne 0 ]]; then
    fail "QEMU exited with status ${qemu_status}; boot log: ${LOG_PATH}"
fi

echo "Node QEMU boot log: ${LOG_PATH}"
