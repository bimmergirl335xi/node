#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=lib/p0-common.sh
source "${SCRIPT_DIR}/lib/p0-common.sh"
p0_require_ordinary_user

stage=build
if (($# > 0)); then
    [[ "$1" == "--stage" && $# -eq 2 ]] || p0_die "usage: preflight.sh --stage source|build|load|execute"
    stage="$2"
fi

case "$stage" in
    source | build | load | execute) ;;
    *) p0_die "invalid preflight stage: $stage" ;;
esac

for command_name in awk du findmnt grep head realpath sed sha256sum stat timeout; do
    p0_require_command "$command_name"
done

p0_export_ram_environment
p0_assert_swap_disabled

[[ "$(uname -m)" == "x86_64" ]] || p0_die "P0 currently supports only x86_64"
[[ -r /proc/meminfo ]] || p0_die "/proc/meminfo unavailable"
[[ -r /proc/cpuinfo ]] || p0_die "/proc/cpuinfo unavailable"

available_kib="$(awk '/^MemAvailable:/ { print $2; exit }' /proc/meminfo)"
[[ "$available_kib" =~ ^[0-9]+$ ]] || p0_die "unable to determine available memory"
minimum_mib="${NODE_P0_MIN_AVAILABLE_MIB:-3072}"
[[ "$minimum_mib" =~ ^[0-9]+$ ]] || p0_die "NODE_P0_MIN_AVAILABLE_MIB must be an integer"
((minimum_mib >= 1024 && minimum_mib <= 65536)) || p0_die \
    "minimum available RAM must be between 1024 and 65536 MiB"
((available_kib >= minimum_mib * 1024)) || p0_die \
    "insufficient available RAM: $((available_kib / 1024)) MiB; require ${minimum_mib} MiB"
workspace_size_bytes="$(p0_mount_size_bytes "$NODE_P0_WORKSPACE")"
[[ "$workspace_size_bytes" =~ ^[0-9]+$ ]] || p0_die "unable to determine tmpfs size"
((workspace_size_bytes >= 1024 * 1024 * 1024 && \
   workspace_size_bytes <= 65536 * 1024 * 1024)) || p0_die \
    "tmpfs size is outside the P0 contract range of 1024 to 65536 MiB"
provider_timeout_seconds="$(p0_provider_timeout_seconds)"

if [[ "$stage" == "source" || "$stage" == "build" ]]; then
    for command_name in \
        bash bc bison cpio file flex gcc git gzip ld make objcopy openssl \
        perl readelf sort timeout; do
        p0_require_command "$command_name"
    done
fi

for mutable_path in \
    "$HOME" "$TMPDIR" "$TMP" "$TEMP" "$XDG_CACHE_HOME" \
    "$XDG_CONFIG_HOME" "$XDG_STATE_HOME" "$KBUILD_OUTPUT" \
    "$P0_SOURCE_DIR" "$P0_INPUTS_DIR" "$P0_INITRAMFS_ROOT" \
    "$P0_TEST_OUTPUT_DIR" "$P0_RECORDS_DIR" "$P0_ARTIFACTS_DIR" \
    "$P0_STAGING_DIR" "$P0_QUARANTINE_DIR" "$P0_FAILURE_RESERVE_DIR"; do
    p0_assert_beneath "$mutable_path" "$NODE_P0_WORKSPACE"
done

printf '%s\n' "$(uname -a)" >"$P0_RECORDS_DIR/host-uname.txt"
cp -- /proc/meminfo "$P0_RECORDS_DIR/host-meminfo.txt"
cp -- /proc/swaps "$P0_RECORDS_DIR/host-swaps.txt"
p0_record preflight satisfied \
    "stage=$stage available_mib=$((available_kib / 1024)) workspace_bytes=$workspace_size_bytes provider_timeout_seconds=$provider_timeout_seconds"
p0_info "strict preflight satisfied for stage: $stage"
