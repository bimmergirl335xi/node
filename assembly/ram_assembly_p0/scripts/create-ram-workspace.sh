#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=lib/p0-common.sh
source "${SCRIPT_DIR}/lib/p0-common.sh"
p0_require_ordinary_user

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
    workspace_parent="$(dirname -- "$workspace")"
    [[ -d "$workspace_parent" ]] || p0_die "workspace parent is absent: $workspace_parent"
    p0_assert_tmpfs_backing "$workspace_parent"
    if [[ -e "$workspace" ]]; then
        [[ -d "$workspace" ]] || p0_die "workspace exists and is not a directory: $workspace"
        [[ -z "$(find "$workspace" -mindepth 1 -maxdepth 1 -print -quit)" ]] || \
            p0_die "refusing to mount over a non-empty workspace: $workspace"
    fi

    owner_uid="$(id -u)"
    owner_gid="$(id -g)"
    install_binary="$(p0_resolve_system_binary /usr/bin/install /bin/install)"
    mount_binary="$(p0_resolve_system_binary /usr/bin/mount /bin/mount)"
    mount_options="rw,exec,nosuid,nodev,mode=0700,uid=${owner_uid},gid=${owner_gid},size=${size_mib}M"

    p0_info "privileged input operation=create-mount-point target=$workspace uid=$owner_uid gid=$owner_gid"
    p0_run_privileged_command "$install_binary" -d -m 0700 \
        -o "$owner_uid" -g "$owner_gid" "$workspace"
    p0_info "privileged input operation=mount-tmpfs source=node-p0-tmpfs target=$workspace options=$mount_options"
    p0_run_privileged_command "$mount_binary" -t tmpfs -o "$mount_options" \
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
