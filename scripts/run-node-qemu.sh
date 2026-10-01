#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
REPO_ROOT=$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel)
ISO_PATH="${REPO_ROOT}/build/artifacts/node-current.iso"
BUILD_ROOT="${REPO_ROOT}/build"
LOG_DIR_INPUT=${NODE_QEMU_LOG_DIR:-${BUILD_ROOT}/logs}
LOG_DIR=$(realpath -m -- "${LOG_DIR_INPUT}")
LOG_PATH="${LOG_DIR}/qemu-last-run.log"
EVENT_LOG_PATH="${LOG_DIR}/qemu-boot-events.jsonl"
TEMP_DIR="${BUILD_ROOT}/temp"
QEMU_BIN=${NODE_QEMU_BIN:-qemu-system-x86_64}
TIMEOUT_SECONDS=${NODE_QEMU_TIMEOUT_SECONDS:-90}
NODE_MODE=${NODE_QEMU_MODE:-conformance}
DEBUG_MODE=${NODE_QEMU_DEBUG_MODE:-0}
GDB_PORT=${NODE_QEMU_GDB_PORT:-1234}
NODE_ID=${NODE_QEMU_NODE_ID:-}
VCPUS=${NODE_QEMU_VCPUS:-1}
CPU_SOCKETS=${NODE_QEMU_CPU_SOCKETS:-1}
CPU_CORES=${NODE_QEMU_CPU_CORES:-${VCPUS}}
CPU_THREADS=${NODE_QEMU_CPU_THREADS:-1}
MEMORY_MB=${NODE_QEMU_MEMORY_MB:-512}
NETWORK_ENABLED=${NODE_QEMU_NETWORK_ENABLED:-0}
NETWORK_MAC=${NODE_QEMU_NETWORK_MAC:-}
NETWORK_MULTICAST_ADDRESS=${NODE_QEMU_NETWORK_MULTICAST_ADDRESS:-}
NETWORK_MULTICAST_PORT=${NODE_QEMU_NETWORK_MULTICAST_PORT:-}
NETWORK_LOCAL_ADDRESS=${NODE_QEMU_NETWORK_LOCAL_ADDRESS:-}
ACS_IPV4_ADDRESS=${NODE_QEMU_ACS_IPV4_ADDRESS:-}
ACS_UDP_PORT=${NODE_QEMU_ACS_UDP_PORT:-}
ACS_PEERS_A=${NODE_QEMU_ACS_PEERS_A:-}
ACS_PEERS_B=${NODE_QEMU_ACS_PEERS_B:-}
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
                              or 0 for explicit managed lab residency
  NODE_QEMU_MODE             conformance or lab (default: conformance)
  NODE_QEMU_OVMF_CODE        optional OVMF code image override for --uefi
  NODE_QEMU_OVMF_VARS        optional OVMF variable template override for --uefi

The managed-node controller also supplies bounded internal environment values
for its per-node log directory, process identity, resource limits, profile
metadata, and loopback-confined multicast-datagram LAN attachment.

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

[[ ${NODE_MODE} == conformance || ${NODE_MODE} == lab ]] ||
    fail 'NODE_QEMU_MODE must be conformance or lab'
if [[ ${NODE_MODE} == lab && ${TIMEOUT_SECONDS} == 0 ]]; then
    :
else
    [[ ${TIMEOUT_SECONDS} =~ ^[1-9][0-9]{0,2}$ ]] &&
        (( TIMEOUT_SECONDS <= 600 )) ||
        fail 'NODE_QEMU_TIMEOUT_SECONDS must be 1 through 600, or 0 in lab mode'
fi
[[ ${DEBUG_MODE} == 0 || ${DEBUG_MODE} == 1 ]] ||
    fail 'NODE_QEMU_DEBUG_MODE must be 0 or 1'
[[ ${VCPUS} =~ ^[1-9][0-9]?$ ]] && (( VCPUS <= 16 )) ||
    fail 'NODE_QEMU_VCPUS must be an integer from 1 through 16'
for topology_value in "${CPU_SOCKETS}" "${CPU_CORES}" "${CPU_THREADS}"; do
    [[ ${topology_value} =~ ^[1-9][0-9]?$ ]] && (( topology_value <= 16 )) ||
        fail 'QEMU CPU topology fields must be integers from 1 through 16'
done
(( CPU_SOCKETS * CPU_CORES * CPU_THREADS == VCPUS )) ||
    fail 'NODE_QEMU_VCPUS must equal sockets * cores * threads'
[[ ${MEMORY_MB} =~ ^[1-9][0-9]{2,4}$ ]] &&
    (( MEMORY_MB >= 128 && MEMORY_MB <= 8192 )) ||
    fail 'NODE_QEMU_MEMORY_MB must be an integer from 128 through 8192'
[[ -z ${NODE_ID} || ${NODE_ID} =~ ^[a-z0-9][a-z0-9-]{0,31}$ ]] ||
    fail 'NODE_QEMU_NODE_ID must be a lowercase identifier of at most 32 characters'
[[ ${NETWORK_ENABLED} == 0 || ${NETWORK_ENABLED} == 1 ]] ||
    fail 'NODE_QEMU_NETWORK_ENABLED must be 0 or 1'
case "${LOG_DIR}" in
    "${BUILD_ROOT}"/*) ;;
    *) fail 'NODE_QEMU_LOG_DIR must resolve beneath the repository build directory' ;;
esac
if [[ ${NETWORK_ENABLED} == 1 ]]; then
    [[ -n ${NODE_ID} ]] ||
        fail 'managed network attachment requires NODE_QEMU_NODE_ID'
    [[ ${NETWORK_MAC} =~ ^02:([0-9a-f]{2}:){4}[0-9a-f]{2}$ ]] ||
        fail 'NODE_QEMU_NETWORK_MAC must be a lowercase locally administered MAC'
    [[ ${NETWORK_MULTICAST_ADDRESS} =~ ^23[0-9](\.[0-9]{1,3}){3}$ ]] ||
        fail 'NODE_QEMU_NETWORK_MULTICAST_ADDRESS must be an IPv4 multicast address'
    [[ ${NETWORK_LOCAL_ADDRESS} =~ ^127(\.[0-9]{1,3}){3}$ ]] ||
        fail 'NODE_QEMU_NETWORK_LOCAL_ADDRESS must be an IPv4 loopback address'
    [[ ${ACS_IPV4_ADDRESS} =~ ^10\.77\.0\.[1-9][0-9]{0,2}$ ]] ||
        fail 'NODE_QEMU_ACS_IPV4_ADDRESS must be a 10.77.0.0/24 host address'
    for port_value in "${NETWORK_MULTICAST_PORT}" "${ACS_UDP_PORT}"; do
        [[ ${port_value} =~ ^[0-9]{4,5}$ ]] &&
            (( port_value >= 1024 && port_value <= 65535 )) ||
            fail 'managed network ports must be integers from 1024 through 65535'
    done
    for peer_part in "${ACS_PEERS_A}" "${ACS_PEERS_B}"; do
        [[ -n ${peer_part} && ${#peer_part} -le 63 && ${peer_part} != *,* ]] ||
            fail 'managed ACS peer profile parts must be nonempty and at most 63 bytes'
        [[ ${peer_part} =~ ^[a-z0-9-]+@10\.77\.0\.[0-9]+(\;[a-z0-9-]+@10\.77\.0\.[0-9]+)*$ ]] ||
            fail 'managed ACS peer profile parts must contain node@IPv4 entries'
    done
fi
if [[ ${NODE_MODE} == lab && ( -z ${NODE_ID} || ${NETWORK_ENABLED} != 1 ) ]]; then
    fail 'lab mode requires an explicit managed node identity and network profile'
fi
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
    -smp "cpus=${VCPUS},sockets=${CPU_SOCKETS},cores=${CPU_CORES},threads=${CPU_THREADS}"
    -m "${MEMORY_MB}M"
    -cdrom "${ISO_PATH}"
    -boot order=d
    -display none
    -serial stdio
    -monitor none
    -no-reboot
)

if [[ ${NETWORK_ENABLED} == 1 ]]; then
    network_backend="dgram,id=node_lab_net,remote.type=inet"
    network_backend+=",remote.host=${NETWORK_MULTICAST_ADDRESS}"
    network_backend+=",remote.port=${NETWORK_MULTICAST_PORT},local.type=inet"
    network_backend+=",local.host=${NETWORK_LOCAL_ADDRESS}"
    network_backend+=",local.port=${NETWORK_MULTICAST_PORT}"
    qemu_command+=(
        -netdev "${network_backend}"
        -device "virtio-net-pci,netdev=node_lab_net,mac=${NETWORK_MAC}"
    )
else
    qemu_command+=(
        -nic none
    )
fi

if [[ -n ${NODE_ID} ]]; then
    smbios_profile="type=1,manufacturer=${ACS_PEERS_A}"
    smbios_profile+=",product=Node-Development-VM-${NODE_MODE}"
    smbios_profile+=",version=acs-profile-v1-port-${ACS_UDP_PORT}"
    smbios_profile+=",serial=${NODE_ID},sku=${ACS_IPV4_ADDRESS}"
    smbios_profile+=",family=${ACS_PEERS_B}"
    qemu_command+=(
        -name "guest=${NODE_ID},process=${NODE_ID}"
        -smbios "${smbios_profile}"
    )
fi

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
    printf 'vcpus: %s\n' "${VCPUS}"
    printf 'cpu_topology: sockets=%s cores=%s threads=%s\n' \
        "${CPU_SOCKETS}" "${CPU_CORES}" "${CPU_THREADS}"
    printf 'memory_mb: %s\n' "${MEMORY_MB}"
    printf 'node_mode: %s\n' "${NODE_MODE}"
    if [[ -n ${NODE_ID} ]]; then
        printf 'managed_node_id: %s\n' "${NODE_ID}"
    fi
    if [[ ${NETWORK_ENABLED} == 1 ]]; then
        printf 'network_backend: loopback_multicast_dgram_lan\n'
        printf 'network_mac: %s\n' "${NETWORK_MAC}"
        printf 'network_multicast: %s:%s\n' \
            "${NETWORK_MULTICAST_ADDRESS}" "${NETWORK_MULTICAST_PORT}"
        printf 'network_local_address: %s\n' "${NETWORK_LOCAL_ADDRESS}"
        printf 'acs_ipv4_address: %s\n' "${ACS_IPV4_ADDRESS}"
        printf 'acs_udp_port: %s\n' "${ACS_UDP_PORT}"
        printf 'acs_peers: %s;%s\n' "${ACS_PEERS_A}" "${ACS_PEERS_B}"
    else
        printf 'network_backend: none\n'
    fi
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
if [[ ${TIMEOUT_SECONDS} == 0 ]]; then
    "${qemu_command[@]}" 2>&1 | tee -a "${LOG_PATH}"
    qemu_status=${PIPESTATUS[0]}
else
    timeout --signal=TERM --kill-after=5s "${TIMEOUT_SECONDS}s" \
        "${qemu_command[@]}" 2>&1 | tee -a "${LOG_PATH}"
    qemu_status=${PIPESTATUS[0]}
fi
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
