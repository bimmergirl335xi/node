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

p01_validate_boot_log() {
    local log=$1
    local mode=$2

    p01_assert_regular_input "${log}"
    grep -Fq 'Linux version' "${log}" ||
        p01_fail "${mode} did not prove Linux kernel entry"
    grep -Fq '"record":"micro_os_boot_attempt"' "${log}" ||
        p01_fail "${mode} did not prove permanent PID 1 entry"
    grep -Fq '"subject":"volatile_filesystems","outcome":"established"' \
        "${log}" || p01_fail "${mode} did not prove volatile filesystem setup"
    grep -Fq '"outcome":"accepted_for_p01_structural_scope"' "${log}" ||
        p01_fail "${mode} did not prove manifest acceptance"
    grep -Fq '"subject":"same_stage_services","outcome":"overlap_observed"' \
        "${log}" || p01_fail "${mode} did not prove concurrent overlap"
    grep -Fq '"subject":"required_semantic_success","outcome":"semantic_success"' \
        "${log}" || p01_fail "${mode} did not prove required service semantics"
    grep -Fq '"subject":"optional_intentional_failure","outcome":"semantic_failure"' \
        "${log}" || p01_fail "${mode} did not prove optional bounded failure"
    grep -Fq '"subject":"timeout_probe","outcome":"timeout"' "${log}" ||
        p01_fail "${mode} did not prove timeout handling"
    grep -Fq '"subject":"pid1_children","outcome":"zombie_free"' "${log}" ||
        p01_fail "${mode} did not prove child reaping"
    grep -Fq '"outcome":"p01_terminal_state_reached"' "${log}" ||
        p01_fail "${mode} did not reach the P01 terminal state"
    grep -Fq '"outcome":"terminal_action_requested"' "${log}" ||
        p01_fail "${mode} did not prove bounded shutdown"
}
