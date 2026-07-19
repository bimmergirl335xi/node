#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly P0_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
readonly ASSEMBLY_DIR="$(cd -- "${P0_DIR}/.." && pwd -P)"
readonly REPOSITORY_DIR="$(cd -- "${ASSEMBLY_DIR}/.." && pwd -P)"

[[ "${EUID}" -ne 0 ]] || {
    printf '%s\n' 'run payload compilation as an ordinary user' >&2
    exit 1
}

output_root=""
host_validation=0

while (($# > 0)); do
    case "$1" in
        --output)
            (($# >= 2)) || {
                printf '%s\n' '--output requires a path' >&2
                exit 2
            }
            output_root="$2"
            shift 2
            ;;
        --host-validation)
            host_validation=1
            shift
            ;;
        *)
            printf 'unknown argument: %s\n' "$1" >&2
            exit 2
            ;;
    esac
done

[[ -n "$output_root" ]] || {
    printf '%s\n' 'usage: compile-payloads.sh --output PATH [--host-validation]' >&2
    exit 2
}

if ((host_validation == 0)); then
    # shellcheck source=lib/p0-common.sh
    source "${SCRIPT_DIR}/lib/p0-common.sh"
    p0_export_ram_environment
    p0_assert_beneath "$output_root" "$NODE_P0_WORKSPACE"
    p0_assert_tmpfs "$output_root"
fi

cc_command="${CC:-cc}"
command -v "$cc_command" >/dev/null 2>&1 || {
    printf 'C compiler unavailable: %s\n' "$cc_command" >&2
    exit 1
}
cc_path="$(realpath -e -- "$(command -v "$cc_command")")"

mkdir -p -- \
    "$output_root/bin" \
    "$output_root/dev" \
    "$output_root/etc/node-p0" \
    "$output_root/proc" \
    "$output_root/run" \
    "$output_root/sys" \
    "$output_root/tests" \
    "$output_root/tmp"

readonly -a cflags=(
    -std=c11
    -D_GNU_SOURCE
    -Os
    -static
    -fno-common
    -fstack-protector-strong
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

"$cc_command" "${cflags[@]}" "$P0_DIR/src/p0_init.c" -o "$output_root/init"
"$cc_command" "${cflags[@]}" "$P0_DIR/src/p0_test_runner.c" \
    -o "$output_root/bin/p0_test_runner"
"$cc_command" "${cflags[@]}" \
    -I"$REPOSITORY_DIR/interfaces" \
    -I"$ASSEMBLY_DIR/providers" \
    "$ASSEMBLY_DIR/providers/external_component_manifest.c" \
    "$ASSEMBLY_DIR/providers/manifest_validator_main.c" \
    -o "$output_root/bin/node_component_manifest_validator"

for source_file in "$P0_DIR"/src/tests/*.c; do
    test_name="$(basename -- "$source_file" .c)"
    "$cc_command" "${cflags[@]}" "$source_file" -o "$output_root/tests/$test_name"
done

chmod 0755 -- \
    "$output_root/init" \
    "$output_root/bin/p0_test_runner" \
    "$output_root/bin/node_component_manifest_validator" \
    "$output_root/tests/"*

for executable in \
    "$output_root/init" \
    "$output_root/bin/p0_test_runner" \
    "$output_root/bin/node_component_manifest_validator" \
    "$output_root/tests/"*; do
    if readelf -l "$executable" | grep -q 'INTERP'; then
        printf 'payload is dynamically linked: %s\n' "$executable" >&2
        exit 1
    fi
done

if ((host_validation == 0)); then
    printf '%s\n' \
        "{\"schema\":\"node.p0.toolchain-identity.v1\",\"record_revision\":1,\"provider_identity\":\"$P0_ASM_PROVIDER_ID\",\"toolchain_scope\":\"initramfs_static_payloads\",\"compiler_path\":\"$(p0_json_escape "$cc_path")\",\"compiler_version\":\"$(p0_json_escape "$($cc_path --version | head -n 1)")\",\"compile_mode\":\"strict_c11_static\"}" \
        >"$P0_RECORDS_DIR/payload-toolchain-identity.json"
fi

printf 'NODE_P0 PAYLOAD static strict C compilation passed\n'
