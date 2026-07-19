#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=lib/p0-common.sh
source "${SCRIPT_DIR}/lib/p0-common.sh"

"${SCRIPT_DIR}/preflight.sh" --stage build
p0_export_ram_environment
[[ -d "$P0_SOURCE_DIR/.git" ]] || p0_die "RAM kernel source is unavailable"
[[ -x "$P0_SOURCE_DIR/scripts/kconfig/merge_config.sh" ]] || p0_die "kernel merge_config.sh unavailable"
provider_timeout_seconds="$(p0_provider_timeout_seconds)"
kernel_cc="$(realpath -e -- "$(command -v "${NODE_P0_KERNEL_CC:-gcc}")")"
kernel_ld="$(realpath -e -- "$(command -v "${NODE_P0_KERNEL_LD:-ld}")")"
kernel_make="$(realpath -e -- "$(command -v make)")"

config_input_dir="$P0_INPUTS_DIR/kernel-config"
mkdir -p -- "$config_input_dir"

if [[ -r /proc/config.gz ]]; then
    gzip -dc /proc/config.gz >"$config_input_dir/host.config"
    base_description=/proc/config.gz
elif [[ -r "/boot/config-$(uname -r)" ]]; then
    cp -- "/boot/config-$(uname -r)" "$config_input_dir/host.config"
    base_description="/boot/config-$(uname -r)"
else
    timeout --signal=TERM --kill-after=30s "${provider_timeout_seconds}s" \
        "$kernel_make" -C "$P0_SOURCE_DIR" O="$KBUILD_OUTPUT" \
        ARCH=x86_64 CC="$kernel_cc" LD="$kernel_ld" x86_64_defconfig
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
    timeout --signal=TERM --kill-after=30s "${provider_timeout_seconds}s" \
        "$P0_SOURCE_DIR/scripts/kconfig/merge_config.sh" -m -O "$KBUILD_OUTPUT" \
        "$KBUILD_OUTPUT/.config" \
        "$config_input_dir/common.config" \
        "$config_input_dir/x86_64.config" \
        "$config_input_dir/dell_wyse_5070.config"
)
timeout --signal=TERM --kill-after=30s "${provider_timeout_seconds}s" \
    "$kernel_make" -C "$P0_SOURCE_DIR" O="$KBUILD_OUTPUT" \
    ARCH=x86_64 CC="$kernel_cc" LD="$kernel_ld" olddefconfig
"${SCRIPT_DIR}/verify-kernel-config.sh" "$KBUILD_OUTPUT/.config"

cp -- "$KBUILD_OUTPUT/.config" "$P0_RECORDS_DIR/resolved-kernel.config"
resolved_digest="$(sha256sum "$KBUILD_OUTPUT/.config" | awk '{ print $1 }')"
printf '%s\n' "$resolved_digest" >"$P0_RECORDS_DIR/resolved-kernel-config.sha256"
base_digest="$(sha256sum "$config_input_dir/host.config" | awk '{ print $1 }')"
common_digest="$(sha256sum "$config_input_dir/common.config" | awk '{ print $1 }')"
x86_digest="$(sha256sum "$config_input_dir/x86_64.config" | awk '{ print $1 }')"
wyse_digest="$(sha256sum "$config_input_dir/dell_wyse_5070.config" | awk '{ print $1 }')"
printf '%s\n' \
    "{\"schema\":\"node.p0.configuration-references.v1\",\"record_revision\":1,\"p0_conformance_operation_identity\":\"$(p0_json_escape "$P0_CONFORMANCE_OPERATION_ID")\",\"base_configuration\":{\"reference\":\"$(p0_json_escape "$base_description")\",\"ram_handle\":\"$(p0_json_escape "$config_input_dir/host.config")\",\"sha256\":\"$base_digest\"},\"public_fragments\":[{\"reference\":\"config/common.config\",\"sha256\":\"$common_digest\"},{\"reference\":\"config/x86_64.config\",\"sha256\":\"$x86_digest\"},{\"reference\":\"config/dell_wyse_5070.config\",\"sha256\":\"$wyse_digest\"}],\"resolved_configuration\":{\"ram_handle\":\"$(p0_json_escape "$KBUILD_OUTPUT/.config")\",\"sha256\":\"$resolved_digest\"},\"semantic_status\":\"provider_configuration_input_only\"}" \
    >"$P0_RECORDS_DIR/configuration-references.json"
printf '%s\n' \
    "{\"schema\":\"node.p0.toolchain-identity.v1\",\"record_revision\":1,\"provider_identity\":\"$P0_ASM_PROVIDER_ID\",\"toolchain_scope\":\"kernel_configuration_and_build\",\"compiler_path\":\"$(p0_json_escape "$kernel_cc")\",\"compiler_version\":\"$(p0_json_escape "$($kernel_cc --version | head -n 1)")\",\"linker_path\":\"$(p0_json_escape "$kernel_ld")\",\"linker_version\":\"$(p0_json_escape "$($kernel_ld --version | head -n 1)")\",\"build_tool_path\":\"$(p0_json_escape "$kernel_make")\",\"build_tool_version\":\"$(p0_json_escape "$($kernel_make --version | head -n 1)")\"}" \
    >"$P0_RECORDS_DIR/kernel-toolchain-identity.json"
p0_record kernel_config resolved "base=$base_description sha256=$resolved_digest profile=dell-wyse-5070-p0"
p0_info "resolved kernel configuration: $KBUILD_OUTPUT/.config"
