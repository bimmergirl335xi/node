#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
NODE_ROOT=$(cd -- "${SCRIPT_DIR}/.." && pwd -P)
BUILD_ROOT="${NODE_ROOT}/build"
CANDIDATE_DIR="${BUILD_ROOT}/p01-candidate"
FINAL_DIR="${BUILD_ROOT}/artifacts"
TEMP_ROOT="${BUILD_ROOT}/temp"
P01_DIR="${NODE_ROOT}/assembly/p01_boot"
DEFAULT_KERNEL_SOURCE=$(realpath -m -- "${NODE_ROOT}/../Linux-kernel-node-runtime")
KERNEL_SOURCE=${NODE_KERNEL_SOURCE:-${DEFAULT_KERNEL_SOURCE}}
KERNEL_REVISION=94515f3a7d4256a5062176b7d6ed0471938cd51a

node_build_fail() {
    echo "Node build error: $*" >&2
    return 1
}

node_require_regular_file() {
    [[ $# -eq 1 && -f $1 && ! -L $1 && -s $1 ]] ||
        node_build_fail "required build artifact is missing or empty: ${1:-missing}"
}

node_promote_artifacts() {
    if [[ $# -ne 5 ]]; then
        node_build_fail \
            'artifact promotion requires CANDIDATE_DIR FINAL_DIR TEMP_ROOT NODE_REVISION KERNEL_REVISION'
        return 1
    fi

    local candidate_dir=$1
    local final_dir=$2
    local temp_root=$3
    local node_revision=$4
    local kernel_revision=$5
    local candidate_artifacts="${candidate_dir}/artifacts"
    local kernel_build="${candidate_dir}/kernel-build"
    local candidate_iso="${candidate_artifacts}/node-p01-x86_64.iso"
    local candidate_kernel="${candidate_artifacts}/node-p01-bzImage"
    local candidate_initramfs="${candidate_artifacts}/node-p01-initramfs.cpio.gz"
    local candidate_config="${candidate_artifacts}/node-p01-kernel-config.sha256"
    local candidate_sums="${candidate_artifacts}/SHA256SUMS"
    local staging=''
    local iso_sha=''
    local kernel_sha=''
    local initramfs_sha=''
    local config_sha=''
    local timestamp=''
    local vmlinux_available=false
    local system_map_available=false
    local artifact=''

    [[ ${node_revision} =~ ^[0-9a-f]{40}$ ]] ||
        node_build_fail 'invalid Node Git revision for build metadata' || return 1
    [[ ${kernel_revision} =~ ^[0-9a-f]{40}$ ]] ||
        node_build_fail 'invalid kernel Git revision for build metadata' || return 1

    node_require_regular_file "${candidate_iso}" || return 1
    node_require_regular_file "${candidate_kernel}" || return 1
    node_require_regular_file "${candidate_initramfs}" || return 1
    node_require_regular_file "${candidate_config}" || return 1
    node_require_regular_file "${candidate_sums}" || return 1
    (
        cd -- "${candidate_artifacts}"
        sha256sum --check SHA256SUMS >/dev/null
    ) || {
        node_build_fail 'candidate artifact checksum validation failed'
        return 1
    }

    config_sha=$(tr -d '\r\n' < "${candidate_config}")
    [[ ${config_sha} =~ ^[0-9a-f]{64}$ ]] || {
        node_build_fail 'candidate kernel configuration digest is malformed'
        return 1
    }

    mkdir -p -- "${temp_root}"
    staging=$(mktemp -d "${temp_root}/node-artifacts.XXXXXX")
    if ! install -m 0644 "${candidate_iso}" "${staging}/node-current.iso" ||
       ! install -m 0644 "${candidate_kernel}" "${staging}/bzImage" ||
       ! install -m 0644 "${candidate_initramfs}" "${staging}/initramfs.cpio.gz"; then
        rm -rf -- "${staging}"
        node_build_fail 'failed to stage required development artifacts'
        return 1
    fi

    if [[ -f ${kernel_build}/vmlinux && ! -L ${kernel_build}/vmlinux &&
          -s ${kernel_build}/vmlinux ]]; then
        install -m 0644 "${kernel_build}/vmlinux" "${staging}/vmlinux"
        vmlinux_available=true
    fi
    if [[ -f ${kernel_build}/System.map && ! -L ${kernel_build}/System.map &&
          -s ${kernel_build}/System.map ]]; then
        install -m 0644 "${kernel_build}/System.map" "${staging}/System.map"
        system_map_available=true
    fi

    iso_sha=$(sha256sum -- "${staging}/node-current.iso" | awk '{print $1}')
    kernel_sha=$(sha256sum -- "${staging}/bzImage" | awk '{print $1}')
    initramfs_sha=$(sha256sum -- "${staging}/initramfs.cpio.gz" | awk '{print $1}')
    timestamp=$(date -u '+%Y-%m-%dT%H:%M:%SZ')

    printf '%s\n' "{
  \"schema\": \"node-development-build-info-v1\",
  \"profile\": \"p01-host-native\",
  \"node_git_commit\": \"${node_revision}\",
  \"kernel_git_commit\": \"${kernel_revision}\",
  \"kernel_config_sha256\": \"${config_sha}\",
  \"iso_sha256\": \"${iso_sha}\",
  \"kernel_sha256\": \"${kernel_sha}\",
  \"initramfs_sha256\": \"${initramfs_sha}\",
  \"build_timestamp_utc\": \"${timestamp}\",
  \"vmlinux_available\": ${vmlinux_available},
  \"system_map_available\": ${system_map_available}
}" > "${staging}/build-info.json"

    (
        cd -- "${staging}"
        find . -maxdepth 1 -type f ! -name SHA256SUMS -print0 |
            LC_ALL=C sort -z |
            xargs -0 -r sha256sum > SHA256SUMS
        sha256sum --check SHA256SUMS >/dev/null
    ) || {
        rm -rf -- "${staging}"
        node_build_fail 'staged development artifact validation failed'
        return 1
    }

    mkdir -p -- "${final_dir}"
    for artifact in bzImage initramfs.cpio.gz vmlinux System.map \
        SHA256SUMS build-info.json; do
        if [[ -f ${staging}/${artifact} ]]; then
            mv -f -- "${staging}/${artifact}" "${final_dir}/${artifact}"
        else
            rm -f -- "${final_dir}/${artifact}"
        fi
    done

    # Promote the ISO last. A failed build or incomplete staging operation can
    # never replace the previous known-good canonical boot artifact.
    mv -f -- "${staging}/node-current.iso" "${final_dir}/node-current.iso"
    rmdir -- "${staging}"

    echo "Node development ISO: ${final_dir}/node-current.iso"
    echo "Node development ISO SHA-256: ${iso_sha}"
}

usage() {
    cat >&2 <<'EOF'
usage: scripts/build-node.sh [--kernel-source PATH]

Build and validate the existing P01 host-native image, then safely promote the
current development artifacts beneath build/artifacts/. NODE_KERNEL_SOURCE may
also provide the external kernel checkout path.
EOF
    return 0
}

main() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --kernel-source)
                if [[ $# -lt 2 ]]; then
                    usage
                    return 64
                fi
                KERNEL_SOURCE=$2
                shift 2
                ;;
            -h|--help)
                usage
                return 0
                ;;
            *)
                usage
                return 64
                ;;
        esac
    done

    KERNEL_SOURCE=$(realpath -- "${KERNEL_SOURCE}") ||
        node_build_fail 'external kernel source path is unavailable'
    local node_revision
    node_revision=$(git -C "${NODE_ROOT}" rev-parse HEAD)

    mkdir -p -- "${BUILD_ROOT}"
    make -C "${P01_DIR}" \
        shell-syntax host-tests inspect boundaries \
        OUTPUT_DIR="${CANDIDATE_DIR}" \
        KERNEL_SOURCE="${KERNEL_SOURCE}" \
        KERNEL_REVISION="${KERNEL_REVISION}"

    node_promote_artifacts \
        "${CANDIDATE_DIR}" \
        "${FINAL_DIR}" \
        "${TEMP_ROOT}" \
        "${node_revision}" \
        "${KERNEL_REVISION}"
}

if [[ ${BASH_SOURCE[0]} == "$0" ]]; then
    main "$@"
fi
