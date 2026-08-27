#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "$0")/lib-p01.sh"

[[ $# -eq 1 ]] || p01_fail 'usage: assemble-image-tree.sh OUTPUT_DIR'
output_dir=$(p01_assert_output_directory "$1")
artifact_dir="${output_dir}/artifacts"
image_tree="${output_dir}/image-tree"
kernel="${artifact_dir}/node-p01-bzImage"
initramfs="${artifact_dir}/node-p01-initramfs.cpio.gz"

p01_assert_regular_input "${kernel}"
p01_assert_regular_input "${initramfs}"
cmake -E rm -rf "${image_tree}"
mkdir -p -- "${image_tree}/boot/grub"
install -m 0644 "${kernel}" "${image_tree}/boot/node-p01-bzImage"
install -m 0644 "${initramfs}" "${image_tree}/boot/node-p01-initramfs.cpio.gz"
install -m 0644 "${P01_DIR}/config/grub.cfg" "${image_tree}/boot/grub/grub.cfg"
echo "P01 image tree staged: ${image_tree}"
