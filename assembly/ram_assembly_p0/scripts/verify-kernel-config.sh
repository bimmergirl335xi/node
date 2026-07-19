#!/usr/bin/env bash
set -euo pipefail

[[ "${EUID}" -ne 0 ]] || {
    printf '%s\n' 'run kernel configuration inspection as an ordinary user' >&2
    exit 1
}

if (($# != 1)); then
    printf 'usage: verify-kernel-config.sh /path/to/.config\n' >&2
    exit 2
fi

config_file="$1"
[[ -f "$config_file" ]] || {
    printf 'missing kernel configuration: %s\n' "$config_file" >&2
    exit 1
}

required_symbols=(
    CONFIG_64BIT
    CONFIG_X86_64
    CONFIG_BLK_DEV_INITRD
    CONFIG_RD_GZIP
    CONFIG_BINFMT_ELF
    CONFIG_DEVTMPFS
    CONFIG_PROC_FS
    CONFIG_SYSFS
    CONFIG_TMPFS
    CONFIG_TTY
    CONFIG_VT
    CONFIG_VT_CONSOLE
    CONFIG_PCI
    CONFIG_INPUT
    CONFIG_HIGH_RES_TIMERS
    CONFIG_KEXEC
    CONFIG_DRM
    CONFIG_DRM_I915
    CONFIG_DRM_SIMPLEDRM
    CONFIG_FRAMEBUFFER_CONSOLE
    CONFIG_SERIAL_8250
    CONFIG_SERIAL_8250_CONSOLE
)

missing=0
for symbol in "${required_symbols[@]}"; do
    if ! grep -Fxq "${symbol}=y" "$config_file"; then
        printf 'required built-in kernel symbol unresolved: %s\n' "$symbol" >&2
        missing=1
    fi
done

((missing == 0)) || exit 1
printf 'NODE_P0 CONFIG required built-in mechanisms satisfied\n'
