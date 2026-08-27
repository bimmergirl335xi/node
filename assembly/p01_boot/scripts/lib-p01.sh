#!/usr/bin/env bash
set -euo pipefail

P01_SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
P01_DIR=$(cd -- "${P01_SCRIPT_DIR}/.." && pwd -P)
NODE_REPOSITORY=$(cd -- "${P01_DIR}/../.." && pwd -P)
P01_KERNEL_REPOSITORY='git@github.com:bimmergirl335xi/Linux-kernel-node-runtime.git'
P01_KERNEL_REVISION='94515f3a7d4256a5062176b7d6ed0471938cd51a'

p01_fail() {
    echo "P01 error: $*" >&2
    exit 1
}

p01_require_command() {
    command -v "$1" >/dev/null 2>&1 || p01_fail "required command unavailable: $1"
}

p01_realpath_m() {
    realpath -m -- "$1"
}

p01_assert_output_directory() {
    local candidate
    local build_root
    [[ $# -eq 1 ]] || p01_fail 'output-directory validation requires one path'
    [[ $1 == /* ]] || p01_fail 'output directory must be absolute'
    candidate=$(p01_realpath_m "$1")
    build_root=$(p01_realpath_m "${NODE_REPOSITORY}/build")
    case "${candidate}/" in
        "${build_root}/"*) ;;
        *) p01_fail 'P01 output must remain beneath the repository build directory' ;;
    esac
    case "${candidate}" in
        /dev|/dev/*|/boot|/boot/*|/sys|/sys/*|/proc|/proc/*)
            p01_fail 'device, boot, kernel-state, and process-state paths are prohibited'
            ;;
    esac
    printf '%s\n' "${candidate}"
}

p01_assert_regular_input() {
    [[ $# -eq 1 && -f $1 && ! -L $1 ]] ||
        p01_fail "expected a regular non-symlink input: ${1:-missing}"
}

p01_sha256() {
    p01_assert_regular_input "$1"
    sha256sum -- "$1" | awk '{print $1}'
}

p01_size() {
    p01_assert_regular_input "$1"
    stat -c '%s' -- "$1"
}

p01_safe_token() {
    [[ $1 =~ ^[A-Za-z0-9._:/+@-]+$ ]] || p01_fail "unsafe record token: $1"
}
