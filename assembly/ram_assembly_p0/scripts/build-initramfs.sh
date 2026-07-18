#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=lib/p0-common.sh
source "${SCRIPT_DIR}/lib/p0-common.sh"

manifest_argument=""
while (($# > 0)); do
    case "$1" in
        --external-component-manifest)
            (($# >= 2)) || p0_die "--external-component-manifest requires a path"
            [[ -z "$manifest_argument" ]] || p0_die \
                "--external-component-manifest may be supplied only once"
            manifest_argument="$2"
            shift 2
            ;;
        *) p0_die "unknown argument: $1" ;;
    esac
done

manifest_environment="${NODE_EXTERNAL_COMPONENT_MANIFEST:-}"
if [[ -n "$manifest_argument" && -n "$manifest_environment" ]]; then
    p0_die "external component manifest is ambiguous: use argument or environment, not both"
fi
selected_manifest="${manifest_argument:-$manifest_environment}"

"${SCRIPT_DIR}/preflight.sh" --stage build
p0_export_ram_environment

if find "$P0_INITRAMFS_ROOT" -mindepth 1 -print -quit | grep -q .; then
    p0_die "initramfs root is not empty; use a fresh RAM workspace"
fi

"${SCRIPT_DIR}/compile-payloads.sh" --output "$P0_INITRAMFS_ROOT"
install -D -m 0644 -- "$P0_ROOT_DIR/manifests/tests.manifest" \
    "$P0_INITRAMFS_ROOT/etc/node-p0/tests.manifest"

manifest_validator="$P0_INITRAMFS_ROOT/bin/node_component_manifest_validator"
if [[ -n "$selected_manifest" ]]; then
    selected_manifest="$(p0_canonical_path "$selected_manifest")"
    [[ -f "$selected_manifest" ]] || p0_die \
        "external component manifest unavailable: $selected_manifest"
    NODE_EXTERNAL_COMPONENT_MANIFEST= \
        "$manifest_validator" --manifest "$selected_manifest"
    install -D -m 0444 -- "$selected_manifest" \
        "$P0_INITRAMFS_ROOT/etc/node/components/candidate.manifest"
    p0_record external_component_manifest candidate_validated \
        "source=explicit manifest=$selected_manifest activation=not_requested"
else
    NODE_EXTERNAL_COMPONENT_MANIFEST= "$manifest_validator"
    p0_record external_component_manifest not_supplied \
        "public_only=true activation=not_requested"
fi

if [[ -n "${NODE_P0_EXTRA_TESTS_MANIFEST:-}" || -n "${NODE_P0_EXTRA_TESTS_DIR:-}" ]]; then
    [[ -n "${NODE_P0_EXTRA_TESTS_MANIFEST:-}" && -n "${NODE_P0_EXTRA_TESTS_DIR:-}" ]] || \
        p0_die "extra tests require both NODE_P0_EXTRA_TESTS_MANIFEST and NODE_P0_EXTRA_TESTS_DIR"
    extra_manifest="$(p0_canonical_path "$NODE_P0_EXTRA_TESTS_MANIFEST")"
    extra_tests_dir="$(p0_canonical_path "$NODE_P0_EXTRA_TESTS_DIR")"
    [[ -f "$extra_manifest" ]] || p0_die "extra test manifest unavailable: $extra_manifest"
    [[ -d "$extra_tests_dir" ]] || p0_die "extra test directory unavailable: $extra_tests_dir"
    extra_count=0
    while IFS= read -r -d '' extra_test; do
        extra_name="$(basename -- "$extra_test")"
        [[ "$extra_name" =~ ^[A-Za-z0-9_.-]{1,64}$ ]] || p0_die \
            "unsafe extra test filename: $extra_name"
        ((extra_count += 1))
        ((extra_count <= 16)) || p0_die "extra prebuilt test limit exceeded (16)"
        install -m 0755 -- "$extra_test" "$P0_INITRAMFS_ROOT/tests/$extra_name"
        if readelf -l "$P0_INITRAMFS_ROOT/tests/$extra_name" | grep -q INTERP; then
            p0_die "extra test is dynamically linked: $extra_name"
        fi
    done < <(find "$extra_tests_dir" -maxdepth 1 -type f -perm /111 -print0)
    ((extra_count > 0)) || p0_die "extra test directory contains no executable files"
    cat -- "$extra_manifest" >>"$P0_INITRAMFS_ROOT/etc/node-p0/tests.manifest"
fi

initramfs="$P0_ARTIFACTS_DIR/node-p0-initramfs.cpio.gz"
(
    cd -- "$P0_INITRAMFS_ROOT"
    find . -print0 | LC_ALL=C sort -z | \
        cpio --null --create --quiet --format=newc --owner=0:0 --reproducible | \
        gzip -9n >"$initramfs"
)

[[ -s "$initramfs" ]] || p0_die "initramfs output is empty"
initramfs_digest="$(sha256sum "$initramfs" | awk '{ print $1 }')"
printf '%s  %s\n' "$initramfs_digest" node-p0-initramfs.cpio.gz \
    >"$P0_ARTIFACTS_DIR/initramfs.sha256"
p0_record initramfs_build candidate_produced "sha256=$initramfs_digest shell=absent"
p0_info "candidate initramfs output produced: $initramfs"
