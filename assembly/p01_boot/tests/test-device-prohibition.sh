#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: $0 LIB_P01" >&2
    exit 64
fi
source "$1"

for prohibited in /dev/sda /dev/nvme0n1 /dev/mmcblk0 /boot/node.iso /sys/firmware/efi; do
    if bash -c 'source "$1"; p01_assert_output_directory "$2"' \
        p01-prohibition "$1" "${prohibited}" >/dev/null 2>&1; then
        echo "prohibited path accepted: ${prohibited}" >&2
        exit 1
    fi
done
valid="${NODE_REPOSITORY}/build/p01-device-prohibition-test"
[[ $(p01_assert_output_directory "${valid}") == "${valid}" ]]
echo 'P01 device-write prohibition test passed'
