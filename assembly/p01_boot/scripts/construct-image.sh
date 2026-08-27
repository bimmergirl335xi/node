#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "$0")/lib-p01.sh"

[[ $# -eq 1 ]] || p01_fail 'usage: construct-image.sh OUTPUT_DIR'
output_dir=$(p01_assert_output_directory "$1")
image_tree="${output_dir}/image-tree"
artifact_dir="${output_dir}/artifacts"
image="${artifact_dir}/node-p01-x86_64.iso"
availability="${output_dir}/validation/image-build.json"
mkdir -p -- "${artifact_dir}" "${output_dir}/validation"

if ! command -v grub-mkrescue >/dev/null 2>&1 ||
   ! command -v xorriso >/dev/null 2>&1; then
    printf '%s\n' \
        '{"test":"p01_image_build","status":"unavailable","reason":"grub-mkrescue and xorriso are required"}' \
        > "${availability}"
    echo 'P01 image build unavailable: grub-mkrescue and xorriso are required' >&2
    exit 77
fi
[[ -f ${image_tree}/boot/grub/grub.cfg ]] || p01_fail 'image tree is incomplete'
grub-mkrescue -o "${image}" "${image_tree}"
p01_assert_regular_input "${image}"
printf '%s\n' \
    '{"test":"p01_image_build","status":"candidate_produced","format":"grub_hybrid_iso"}' \
    > "${availability}"
echo "P01 removable-media image candidate: ${image}"
