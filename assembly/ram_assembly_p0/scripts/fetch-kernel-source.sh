#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=lib/p0-common.sh
source "${SCRIPT_DIR}/lib/p0-common.sh"

local_source=""
source_bundle=""
remote_url=""
kernel_ref=""

while (($# > 0)); do
    case "$1" in
        --local)
            (($# >= 2)) || p0_die "--local requires a path"
            local_source="$2"
            shift 2
            ;;
        --url)
            (($# >= 2)) || p0_die "--url requires a URL"
            remote_url="$2"
            shift 2
            ;;
        --bundle)
            (($# >= 2)) || p0_die "--bundle requires a path"
            source_bundle="$2"
            shift 2
            ;;
        --ref)
            (($# >= 2)) || p0_die "--ref requires an immutable reviewed revision"
            kernel_ref="$2"
            shift 2
            ;;
        *) p0_die "unknown argument: $1" ;;
    esac
done

[[ -n "$kernel_ref" ]] || p0_die "--ref is required"
source_count=0
[[ -n "$local_source" ]] && ((source_count += 1))
[[ -n "$source_bundle" ]] && ((source_count += 1))
[[ -n "$remote_url" ]] && ((source_count += 1))
((source_count == 1)) || p0_die "select exactly one of --local, --bundle, or --url"

"${SCRIPT_DIR}/preflight.sh" --stage source
p0_export_ram_environment

if [[ -e "$P0_SOURCE_DIR/.git" ]] || find "$P0_SOURCE_DIR" -mindepth 1 -print -quit | grep -q .; then
    p0_die "RAM kernel-source destination is not empty: $P0_SOURCE_DIR"
fi
rmdir -- "$P0_SOURCE_DIR"

if [[ -n "$local_source" ]]; then
    local_source="$(p0_canonical_path "$local_source")"
    [[ -d "$local_source/.git" ]] || p0_die "local source is not a Git checkout: $local_source"
    p0_assert_tmpfs "$NODE_P0_WORKSPACE"
    git clone --no-hardlinks --no-checkout -- "$local_source" "$P0_SOURCE_DIR"
    source_description="local:$local_source"
elif [[ -n "$source_bundle" ]]; then
    source_bundle="$(p0_canonical_path "$source_bundle")"
    [[ -f "$source_bundle" ]] || p0_die "kernel source bundle unavailable: $source_bundle"
    git bundle verify "$source_bundle" >/dev/null
    git clone --no-checkout -- "$source_bundle" "$P0_SOURCE_DIR"
    source_description="bundle:$source_bundle"
else
    git clone --no-checkout -- "$remote_url" "$P0_SOURCE_DIR"
    source_description="url:$remote_url"
fi

git -C "$P0_SOURCE_DIR" checkout --detach -- "$kernel_ref"
kernel_revision="$(git -C "$P0_SOURCE_DIR" rev-parse HEAD)"
git -C "$P0_SOURCE_DIR" diff --quiet
git -C "$P0_SOURCE_DIR" diff --cached --quiet

printf '%s\n' "$kernel_revision" >"$P0_RECORDS_DIR/kernel-source-revision.txt"
printf '%s\n' "$source_description" >"$P0_RECORDS_DIR/kernel-source-origin.txt"
p0_record kernel_source obtained "revision=$kernel_revision source=$source_description"
p0_info "kernel source copied into RAM at revision: $kernel_revision"
