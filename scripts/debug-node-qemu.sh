#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
REPO_ROOT=$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel)
RUNNER="${SCRIPT_DIR}/run-node-qemu.sh"
ISO_PATH="${REPO_ROOT}/build/artifacts/node-current.iso"
VMLINUX_PATH="${REPO_ROOT}/build/artifacts/vmlinux"
GDB_PORT=${NODE_QEMU_GDB_PORT:-1234}
TIMEOUT_SECONDS=${NODE_QEMU_TIMEOUT_SECONDS:-600}
FIRMWARE_ARGUMENT=--bios

fail() {
    echo "Node QEMU debug error: $*" >&2
    return 1
}

usage() {
    cat <<'EOF'
usage: scripts/debug-node-qemu.sh [--bios | --uefi] [--port PORT]

Start the canonical Node ISO paused in QEMU with a loopback-only GDB endpoint.
The default endpoint is 127.0.0.1:1234 and the default bounded timeout is 600
seconds. NODE_QEMU_TIMEOUT_SECONDS may shorten that timeout. A GDB client is
required separately; this launcher does not install or start one.

In a second terminal:
  gdb build/artifacts/vmlinux
  (gdb) target remote 127.0.0.1:1234
  (gdb) continue
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --bios)
            FIRMWARE_ARGUMENT=--bios
            shift
            ;;
        --uefi)
            FIRMWARE_ARGUMENT=--uefi
            shift
            ;;
        --port)
            [[ $# -ge 2 ]] || {
                usage >&2
                exit 64
            }
            GDB_PORT=$2
            shift 2
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
done

[[ ${GDB_PORT} =~ ^[0-9]{4,5}$ ]] &&
    (( GDB_PORT >= 1024 && GDB_PORT <= 65535 )) ||
    fail 'PORT must be an integer from 1024 through 65535'
[[ -f ${ISO_PATH} && ! -L ${ISO_PATH} && -s ${ISO_PATH} ]] ||
    fail "canonical ISO is missing or empty; run ./scripts/build-node.sh first: ${ISO_PATH}"
[[ -f ${VMLINUX_PATH} && ! -L ${VMLINUX_PATH} && -s ${VMLINUX_PATH} ]] ||
    fail "kernel symbols are missing or empty; run ./scripts/build-node.sh first: ${VMLINUX_PATH}"

printf 'Node QEMU debug endpoint: 127.0.0.1:%s\n' "${GDB_PORT}"
printf 'Node kernel symbols: %s\n' "${VMLINUX_PATH}"
printf 'Attach from another terminal, then continue execution:\n'
printf '  gdb %q\n' "${VMLINUX_PATH}"
printf '  (gdb) target remote 127.0.0.1:%s\n' "${GDB_PORT}"
printf '  (gdb) continue\n'

NODE_QEMU_DEBUG_MODE=1 \
NODE_QEMU_GDB_PORT="${GDB_PORT}" \
NODE_QEMU_TIMEOUT_SECONDS="${TIMEOUT_SECONDS}" \
    exec "${RUNNER}" "${FIRMWARE_ARGUMENT}"
