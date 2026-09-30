#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
REPO_ROOT=$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel)
ISO_PATH="${REPO_ROOT}/build/artifacts/node-current.iso"
LOG_DIR="${REPO_ROOT}/build/logs"
LOG_PATH="${LOG_DIR}/qemu-last-run.log"
EVENT_LOG_PATH="${LOG_DIR}/qemu-boot-events.jsonl"
TEMP_DIR="${REPO_ROOT}/build/temp"
QEMU_BIN=${NODE_QEMU_BIN:-qemu-system-x86_64}
TIMEOUT_SECONDS=${NODE_QEMU_TIMEOUT_SECONDS:-90}
DEBUG_MODE=${NODE_QEMU_DEBUG_MODE:-0}
GDB_PORT=${NODE_QEMU_GDB_PORT:-1234}
FIRMWARE=bios
MAX_EVENT_COUNT=256
MAX_EVENT_BYTES=262144

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

Outputs:
  build/logs/qemu-last-run.log
  build/logs/qemu-boot-events.jsonl
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
[[ ${DEBUG_MODE} == 0 || ${DEBUG_MODE} == 1 ]] ||
    fail 'NODE_QEMU_DEBUG_MODE must be 0 or 1'
if [[ ${DEBUG_MODE} == 1 ]]; then
    [[ ${GDB_PORT} =~ ^[0-9]{4,5}$ ]] &&
        (( GDB_PORT >= 1024 && GDB_PORT <= 65535 )) ||
        fail 'NODE_QEMU_GDB_PORT must be an integer from 1024 through 65535'
fi
command -v "${QEMU_BIN}" >/dev/null 2>&1 ||
    fail "QEMU executable not found: ${QEMU_BIN}"
[[ -f ${ISO_PATH} && ! -L ${ISO_PATH} && -s ${ISO_PATH} ]] ||
    fail "canonical ISO is missing or empty; run ./scripts/build-node.sh first: ${ISO_PATH}"

mkdir -p -- "${LOG_DIR}" "${TEMP_DIR}"

uefi_vars_copy=''
event_staging=''
cleanup() {
    [[ -z ${uefi_vars_copy} ]] || rm -f -- "${uefi_vars_copy}"
    [[ -z ${event_staging} ]] || rm -f -- "${event_staging}"
}
trap cleanup EXIT

event_staging=$(mktemp "${TEMP_DIR}/qemu-boot-events.XXXXXX.jsonl")
rm -f -- "${EVENT_LOG_PATH}"

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

if [[ ${DEBUG_MODE} == 1 ]]; then
    qemu_command+=(
        -S
        -gdb "tcp:127.0.0.1:${GDB_PORT}"
    )
fi

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
    if [[ ${DEBUG_MODE} == 1 ]]; then
        printf 'debug_mode: paused_gdb\n'
        printf 'gdb_endpoint: 127.0.0.1:%s\n' "${GDB_PORT}"
    fi
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

event_capture_status=0
if LC_ALL=C awk \
    -v max_count="${MAX_EVENT_COUNT}" \
    -v max_bytes="${MAX_EVENT_BYTES}" '
    BEGIN { status = 0 }
    {
        sub(/\r$/, "")
    }
    /^\{"record":"[A-Za-z0-9_]+"/ && /\}$/ {
        count++
        bytes += length($0) + 1
        if (count > max_count || bytes > max_bytes) {
            status = 2
            exit
        }
        print
    }
    END {
        if (status != 0) exit status
        if (count == 0) exit 3
    }
' "${LOG_PATH}" > "${event_staging}"; then
    chmod 0644 -- "${event_staging}"
    mv -f -- "${event_staging}" "${EVENT_LOG_PATH}"
    event_staging=''
else
    event_capture_status=$?
    rm -f -- "${event_staging}"
    event_staging=''
fi

event_count=0
if [[ ${event_capture_status} -eq 0 ]]; then
    event_count=$(wc -l < "${EVENT_LOG_PATH}")
fi
{
    printf 'structured_event_count: %s\n' "${event_count}"
    printf 'structured_event_capture_status: %s\n' "${event_capture_status}"
} | tee -a "${LOG_PATH}"

if [[ ${qemu_status} -ne 0 ]]; then
    fail "QEMU exited with status ${qemu_status}; boot log: ${LOG_PATH}"
fi
if [[ ${event_capture_status} -ne 0 ]]; then
    fail "structured boot-event capture failed with status ${event_capture_status}; boot log: ${LOG_PATH}"
fi

echo "Node QEMU boot log: ${LOG_PATH}"
echo "Node QEMU boot events: ${EVENT_LOG_PATH}"
