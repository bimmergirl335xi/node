#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "$0")/lib-p01.sh"

if [[ $# -ne 3 ]]; then
    p01_fail 'usage: build-kernel.sh OUTPUT_DIR KERNEL_SOURCE EXACT_REVISION'
fi
output_dir=$(p01_assert_output_directory "$1")
kernel_source=$(realpath -- "$2")
revision=$3
kernel_build="${output_dir}/kernel-build"
artifact_dir="${output_dir}/artifacts"
remote_url=''

p01_safe_token "${revision}"
[[ ${revision} == "${P01_KERNEL_REVISION}" ]] ||
    p01_fail "P01 requires reviewed kernel revision ${P01_KERNEL_REVISION}"
[[ -d ${kernel_source}/.git ]] || p01_fail 'kernel source must be an exact Git checkout'
remote_url=$(git -C "${kernel_source}" config --get remote.origin.url || true)
case "${remote_url}" in
    git@github.com:bimmergirl335xi/Linux-kernel-node-runtime.git|https://github.com/bimmergirl335xi/Linux-kernel-node-runtime.git) ;;
    *) p01_fail 'kernel origin does not match bimmergirl335xi/Linux-kernel-node-runtime' ;;
esac
[[ $(git -C "${kernel_source}" rev-parse HEAD) == "${revision}" ]] ||
    p01_fail 'kernel checkout HEAD does not match reviewed revision'
[[ -z $(git -C "${kernel_source}" status --porcelain --untracked-files=no) ]] ||
    p01_fail 'reviewed kernel source has tracked modifications'

p01_require_command make
p01_require_command sha256sum
mkdir -p -- "${kernel_build}" "${artifact_dir}"
make -C "${kernel_source}" O="${kernel_build}" ARCH=x86_64 x86_64_defconfig
"${kernel_source}/scripts/kconfig/merge_config.sh" -m -O "${kernel_build}" \
    "${kernel_build}/.config" \
    "${NODE_REPOSITORY}/assembly/ram_assembly_p0/config/common.config" \
    "${NODE_REPOSITORY}/assembly/ram_assembly_p0/config/x86_64.config" \
    "${NODE_REPOSITORY}/assembly/ram_assembly_p0/config/dell_wyse_5070.config"
make -C "${kernel_source}" O="${kernel_build}" ARCH=x86_64 olddefconfig
make -C "${kernel_source}" O="${kernel_build}" ARCH=x86_64 -j1 bzImage
install -m 0644 "${kernel_build}/arch/x86/boot/bzImage" \
    "${artifact_dir}/node-p01-bzImage"
sha256sum "${kernel_build}/.config" | awk '{print $1}' \
    > "${artifact_dir}/node-p01-kernel-config.sha256"
echo "P01 kernel candidate: ${artifact_dir}/node-p01-bzImage"
