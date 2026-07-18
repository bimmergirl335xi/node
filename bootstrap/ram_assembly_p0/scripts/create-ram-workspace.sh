#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=lib/p0-common.sh
source "${SCRIPT_DIR}/lib/p0-common.sh"

mount_requested=0
size_mib="${NODE_P0_TMPFS_SIZE_MIB:-6144}"

while (($# > 0)); do
    case "$1" in
        --mount)
            mount_requested=1
            shift
            ;;
        --size-mib)
            (($# >= 2)) || p0_die "--size-mib requires a value"
            size_mib="$2"
            shift 2
            ;;
        *) p0_die "unknown argument: $1" ;;
    esac
done

[[ "$size_mib" =~ ^[0-9]+$ ]] || p0_die "tmpfs size must be an integer MiB value"
((size_mib >= 1024 && size_mib <= 65536)) || p0_die "tmpfs size must be between 1024 and 65536 MiB"

p0_require_command findmnt
p0_require_command head
p0_require_command realpath

workspace="$(p0_canonical_path "$(p0_workspace_root)")"

if [[ -e "$workspace" ]] && findmnt -M "$workspace" >/dev/null 2>&1; then
    existing_type="$(findmnt -M "$workspace" -n -o FSTYPE | head -n 1)"
    [[ "$existing_type" == "tmpfs" ]] || p0_die "refusing existing non-tmpfs mount: $workspace"
elif ((mount_requested)); then
    p0_require_root
    p0_require_command mount
    mkdir -p -- "$workspace"
    mount -t tmpfs -o "rw,exec,nosuid,nodev,mode=0700,size=${size_mib}M" \
        node-p0-tmpfs "$workspace"
else
    [[ -d "$workspace" ]] || p0_die "workspace absent; use --mount or create it on tmpfs: $workspace"
fi

p0_assert_tmpfs "$workspace"
p0_export_ram_environment

mkdir -p -- "$P0_SOURCE_DIR" "$P0_INITRAMFS_ROOT"
chmod 0700 -- "$NODE_P0_WORKSPACE" "$P0_FAILURE_RESERVE_DIR"

mount_target="$(p0_mount_field "$workspace" TARGET)"
mount_options="$(p0_mount_field "$workspace" OPTIONS)"
p0_record workspace established "target=$mount_target options=$mount_options size_mib=$size_mib"

p0_info "verified RAM workspace: $workspace"
p0_info "export NODE_P0_WORKSPACE='$workspace'"
p0_info "swap must be disabled before strict preflight or build"
