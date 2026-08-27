#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "$0")/lib-p01.sh"

[[ $# -eq 1 ]] || p01_fail 'usage: validate-boundaries.sh OUTPUT_DIR'
output_dir=$(p01_assert_output_directory "$1")

private_pattern='/ho''me/|BEGIN (RSA |EC |OPENSSH )?PRIVATE KEY|pass''word=|to''ken='
if grep -R -n -E "${private_pattern}" \
    "${P01_DIR}" "${output_dir}/records"; then
    p01_fail 'private identifier or persistent local path found'
fi
if grep -R -n -E '(^|[[:space:]])(sudo|dd|parted|fdisk|sfdisk|gdisk|mkfs(\.|[[:space:]])|efibootmgr)([[:space:]]|$)' \
    "${P01_DIR}/scripts"; then
    p01_fail 'prohibited device or host-boot mutation command found'
fi
if bash -c 'source "$1"; p01_assert_output_directory /dev/sda' \
    p01-device-test "${P01_DIR}/scripts/lib-p01.sh" >/dev/null 2>&1; then
    p01_fail 'device output path was unexpectedly accepted'
fi
if bash -c 'source "$1"; p01_assert_output_directory /boot/node-p01.iso' \
    p01-boot-test "${P01_DIR}/scripts/lib-p01.sh" >/dev/null 2>&1; then
    p01_fail 'host boot path was unexpectedly accepted'
fi
echo 'P01 private/path/device-write boundary validation passed'
