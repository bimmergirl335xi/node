#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=lib/p0-common.sh
source "${SCRIPT_DIR}/lib/p0-common.sh"

"${SCRIPT_DIR}/preflight.sh" --stage build
p0_export_ram_environment
[[ -d "$P0_SOURCE_DIR/.git" ]] || p0_die "RAM kernel source is unavailable"
[[ -f "$KBUILD_OUTPUT/.config" ]] || p0_die "resolved kernel configuration is unavailable"
"${SCRIPT_DIR}/verify-kernel-config.sh" "$KBUILD_OUTPUT/.config"

make -C "$P0_SOURCE_DIR" O="$KBUILD_OUTPUT" ARCH=x86_64 --jobs=1 bzImage \
    2>&1 | tee "$P0_RECORDS_DIR/kernel-build.log"

built_image="$KBUILD_OUTPUT/arch/x86/boot/bzImage"
[[ -s "$built_image" ]] || p0_die "kernel build did not produce bzImage"
install -m 0600 -- "$built_image" "$P0_ARTIFACTS_DIR/node-p0-bzImage"

git -C "$P0_SOURCE_DIR" diff --quiet || p0_die "kernel source worktree was modified"
git -C "$P0_SOURCE_DIR" diff --cached --quiet || p0_die "kernel source index was modified"

kernel_digest="$(sha256sum "$P0_ARTIFACTS_DIR/node-p0-bzImage" | awk '{ print $1 }')"
printf '%s  %s\n' "$kernel_digest" node-p0-bzImage >"$P0_ARTIFACTS_DIR/kernel.sha256"
p0_record kernel_build candidate_produced "sha256=$kernel_digest jobs=1"
p0_info "candidate kernel output produced: $P0_ARTIFACTS_DIR/node-p0-bzImage"
