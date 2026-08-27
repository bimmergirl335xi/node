#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "$0")/lib-p01.sh"

[[ $# -eq 1 ]] || p01_fail 'usage: build-initramfs.sh OUTPUT_DIR'
output_dir=$(p01_assert_output_directory "$1")
root="${output_dir}/initramfs-root"
artifact_dir="${output_dir}/artifacts"
initramfs="${artifact_dir}/node-p01-initramfs.cpio.gz"

p01_require_command cpio
p01_require_command gzip
[[ -x ${root}/init ]] || p01_fail 'staged P01 /init is missing'
mkdir -p -- "${artifact_dir}"
(
    cd -- "${root}"
    find . -print0 | LC_ALL=C sort -z |
        cpio --null --create --quiet --format=newc --owner=0:0 --reproducible |
        gzip -9n > "${initramfs}"
)
p01_assert_regular_input "${initramfs}"
echo "P01 initramfs candidate: ${initramfs}"
