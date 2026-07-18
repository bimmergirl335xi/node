#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=lib/p0-common.sh
source "${SCRIPT_DIR}/lib/p0-common.sh"

"${SCRIPT_DIR}/preflight.sh" --stage build
p0_export_ram_environment
[[ -d "$P0_SOURCE_DIR/.git" ]] || p0_die "RAM kernel source is unavailable"
[[ -x "$P0_SOURCE_DIR/scripts/kconfig/merge_config.sh" ]] || p0_die "kernel merge_config.sh unavailable"

config_input_dir="$P0_INPUTS_DIR/kernel-config"
mkdir -p -- "$config_input_dir"

if [[ -r /proc/config.gz ]]; then
    gzip -dc /proc/config.gz >"$config_input_dir/host.config"
    base_description=/proc/config.gz
elif [[ -r "/boot/config-$(uname -r)" ]]; then
    cp -- "/boot/config-$(uname -r)" "$config_input_dir/host.config"
    base_description="/boot/config-$(uname -r)"
else
    make -C "$P0_SOURCE_DIR" O="$KBUILD_OUTPUT" ARCH=x86_64 x86_64_defconfig
    cp -- "$KBUILD_OUTPUT/.config" "$config_input_dir/host.config"
    base_description=x86_64_defconfig
fi

cp -- "$P0_ROOT_DIR/config/common.config" "$config_input_dir/common.config"
cp -- "$P0_ROOT_DIR/config/x86_64.config" "$config_input_dir/x86_64.config"
cp -- "$P0_ROOT_DIR/config/dell_wyse_5070.config" "$config_input_dir/dell_wyse_5070.config"
cp -- /proc/cpuinfo "$P0_RECORDS_DIR/hardware-cpuinfo.txt"
if command -v lspci >/dev/null 2>&1; then
    lspci -nn -k >"$P0_RECORDS_DIR/hardware-pci.txt"
else
    printf 'lspci unavailable; PCI evidence not collected\n' >"$P0_RECORDS_DIR/hardware-pci.txt"
fi

cp -- "$config_input_dir/host.config" "$KBUILD_OUTPUT/.config"
(
    cd -- "$TMPDIR"
    "$P0_SOURCE_DIR/scripts/kconfig/merge_config.sh" -m -O "$KBUILD_OUTPUT" \
        "$KBUILD_OUTPUT/.config" \
        "$config_input_dir/common.config" \
        "$config_input_dir/x86_64.config" \
        "$config_input_dir/dell_wyse_5070.config"
)
make -C "$P0_SOURCE_DIR" O="$KBUILD_OUTPUT" ARCH=x86_64 olddefconfig
"${SCRIPT_DIR}/verify-kernel-config.sh" "$KBUILD_OUTPUT/.config"

cp -- "$KBUILD_OUTPUT/.config" "$P0_RECORDS_DIR/resolved-kernel.config"
resolved_digest="$(sha256sum "$KBUILD_OUTPUT/.config" | awk '{ print $1 }')"
printf '%s\n' "$resolved_digest" >"$P0_RECORDS_DIR/resolved-kernel-config.sha256"
p0_record kernel_config resolved "base=$base_description sha256=$resolved_digest profile=dell-wyse-5070-p0"
p0_info "resolved kernel configuration: $KBUILD_OUTPUT/.config"
