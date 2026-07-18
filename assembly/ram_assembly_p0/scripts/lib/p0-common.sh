#!/usr/bin/env bash

if [[ -n "${NODE_P0_COMMON_LOADED:-}" ]]; then
    return 0
fi
readonly NODE_P0_COMMON_LOADED=1

set -euo pipefail
IFS=$'\n\t'
umask 077

readonly P0_SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly P0_ROOT_DIR="$(cd -- "${P0_SCRIPT_DIR}/../.." && pwd -P)"

p0_info() {
    printf 'NODE_P0 INFO %s\n' "$*"
}

p0_warn() {
    printf 'NODE_P0 WARN %s\n' "$*" >&2
}

p0_die() {
    printf 'NODE_P0 ERROR %s\n' "$*" >&2
    exit 1
}

p0_require_command() {
    command -v "$1" >/dev/null 2>&1 || p0_die "required command unavailable: $1"
}

p0_workspace_root() {
    printf '%s\n' "${NODE_P0_WORKSPACE:-/run/node-assembly-p0}"
}

p0_canonical_path() {
    realpath -m -- "$1"
}

p0_assert_beneath() {
    local candidate root
    candidate="$(p0_canonical_path "$1")"
    root="$(p0_canonical_path "$2")"
    case "$candidate" in
        "$root" | "$root"/*) ;;
        *) p0_die "path escapes RAM workspace: $candidate (root $root)" ;;
    esac
}

p0_mount_field() {
    local path field
    path="$1"
    field="$2"
    findmnt -T "$path" -n -o "$field" | head -n 1
}

p0_assert_tmpfs() {
    local path fs_type options
    path="$(p0_canonical_path "$1")"
    [[ -e "$path" ]] || p0_die "path does not exist: $path"
    fs_type="$(p0_mount_field "$path" FSTYPE)"
    options="$(p0_mount_field "$path" OPTIONS)"
    [[ "$fs_type" == "tmpfs" ]] || p0_die "path is not backed by tmpfs: $path ($fs_type)"
    case ",$options," in
        *,ro,*) p0_die "tmpfs is read-only: $path" ;;
    esac
    case ",$options," in
        *,noexec,*) p0_die "tmpfs is mounted noexec: $path" ;;
    esac
}

p0_assert_tmpfs_backing() {
    local path fs_type
    path="$(p0_canonical_path "$1")"
    [[ -e "$path" ]] || p0_die "path does not exist: $path"
    fs_type="$(p0_mount_field "$path" FSTYPE)"
    [[ "$fs_type" == "tmpfs" ]] || p0_die "path is not backed by tmpfs: $path ($fs_type)"
}

p0_assert_swap_disabled() {
    local active
    active="$(awk 'NR > 1 { count += 1 } END { print count + 0 }' /proc/swaps)"
    [[ "$active" == "0" ]] || p0_die "strict RAM-only mode refuses active swap ($active entries)"
}

p0_export_ram_environment() {
    local root
    root="$(p0_canonical_path "$(p0_workspace_root)")"
    p0_assert_tmpfs "$root"

    export NODE_P0_WORKSPACE="$root"
    export HOME="$root/home"
    export TMPDIR="$root/tmp"
    export TMP="$root/tmp"
    export TEMP="$root/tmp"
    export XDG_CACHE_HOME="$root/cache/xdg"
    export XDG_CONFIG_HOME="$root/config/xdg"
    export XDG_STATE_HOME="$root/state/xdg"
    export CCACHE_DISABLE=1
    export SCCACHE_DISABLE=1
    export KBUILD_OUTPUT="$root/kernel-build"

    export P0_SOURCE_DIR="$root/kernel-source"
    export P0_INPUTS_DIR="$root/inputs"
    export P0_INITRAMFS_ROOT="$root/initramfs-root"
    export P0_TEST_OUTPUT_DIR="$root/test-output"
    export P0_RECORDS_DIR="$root/records"
    export P0_ARTIFACTS_DIR="$root/artifacts"
    export P0_STAGING_DIR="$root/staging"
    export P0_QUARANTINE_DIR="$root/quarantine"
    export P0_FAILURE_RESERVE_DIR="$root/failure-reserve"

    local path
    for path in \
        "$HOME" "$TMPDIR" "$XDG_CACHE_HOME" "$XDG_CONFIG_HOME" \
        "$XDG_STATE_HOME" "$KBUILD_OUTPUT" "$P0_INPUTS_DIR" \
        "$P0_SOURCE_DIR" "$P0_INITRAMFS_ROOT" "$P0_TEST_OUTPUT_DIR" \
        "$P0_RECORDS_DIR" "$P0_ARTIFACTS_DIR" \
        "$P0_STAGING_DIR" "$P0_QUARANTINE_DIR" "$P0_FAILURE_RESERVE_DIR"; do
        p0_assert_beneath "$path" "$root"
        mkdir -p -- "$path"
    done
}

p0_json_escape() {
    local value
    value="$1"
    value="${value//\\/\\\\}"
    value="${value//\"/\\\"}"
    value="${value//$'\t'/\\t}"
    value="${value//$'\r'/\\r}"
    value="${value//$'\n'/\\n}"
    printf '%s' "$value"
}

p0_record() {
    local kind status detail records_file
    kind="$(p0_json_escape "$1")"
    status="$(p0_json_escape "$2")"
    detail="$(p0_json_escape "$3")"
    records_file="${P0_RECORDS_DIR:?P0_RECORDS_DIR not exported}/operations.jsonl"
    printf '{"record":"%s","status":"%s","detail":"%s"}\n' \
        "$kind" "$status" "$detail" >>"$records_file"
}

p0_require_ordinary_user() {
    [[ "${EUID}" -ne 0 ]] || p0_die \
        "run the P0 orchestration as an ordinary user; only printed leaf commands may use privilege"
}

p0_resolve_system_binary() {
    local candidate canonical mode
    for candidate in "$@"; do
        if [[ -f "$candidate" && -x "$candidate" ]]; then
            canonical="$(realpath -e -- "$candidate")"
            case "$canonical" in
                /bin/* | /sbin/* | /usr/bin/* | /usr/sbin/*)
                    mode="$(stat -Lc '%a' -- "$canonical")"
                    [[ "$mode" =~ ^[0-7]+$ ]] || continue
                    (((8#$mode & 0022) == 0)) || continue
                    printf '%s\n' "$canonical"
                    return 0
                    ;;
            esac
        fi
    done
    p0_die "required fixed-path system binary unavailable: $*"
}

_p0_print_exact_command() {
    local argument
    printf 'NODE_P0 PRIVILEGED_COMMAND'
    for argument in "$@"; do
        printf ' %q' "$argument"
    done
    printf '\n'
}

p0_print_privileged_command() {
    local sudo_binary env_binary
    local -a command
    sudo_binary="$(p0_resolve_system_binary /usr/bin/sudo /bin/sudo)"
    env_binary="$(p0_resolve_system_binary /usr/bin/env /bin/env)"
    command=(
        "$sudo_binary" -- "$env_binary" -i
        'PATH=/usr/sbin:/usr/bin:/sbin:/bin' 'LC_ALL=C' "$@"
    )
    _p0_print_exact_command "${command[@]}"
}

p0_run_privileged_command() {
    local sudo_binary env_binary
    local -a command
    sudo_binary="$(p0_resolve_system_binary /usr/bin/sudo /bin/sudo)"
    env_binary="$(p0_resolve_system_binary /usr/bin/env /bin/env)"
    command=(
        "$sudo_binary" -- "$env_binary" -i
        'PATH=/usr/sbin:/usr/bin:/sbin:/bin' 'LC_ALL=C' "$@"
    )

    _p0_print_exact_command "${command[@]}"
    "${command[@]}"
}

p0_verify_fixed_artifact() {
    local artifact
    artifact="$1"
    p0_assert_beneath "$artifact" "${NODE_P0_WORKSPACE:?}"
    p0_assert_tmpfs "$artifact"
    [[ -f "$artifact" ]] || p0_die "artifact missing: $artifact"
    [[ -s "$artifact" ]] || p0_die "artifact empty: $artifact"
}
