#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "$0")/lib-p01.sh"

[[ $# -eq 1 ]] || p01_fail 'usage: build-micro-os.sh OUTPUT_DIR'
output_dir=$(p01_assert_output_directory "$1")
node_build="${output_dir}/node-build"
root="${output_dir}/initramfs-root"

p01_require_command cmake
p01_require_command file
p01_require_command readelf
mkdir -p -- "${output_dir}"
cmake -S "${NODE_REPOSITORY}" -B "${node_build}" \
    -DPROMETHEUS_BUILD_TESTS=ON \
    -DPROMETHEUS_BUILD_BENCHMARKS=OFF \
    -DPROMETHEUS_BUILD_LEGACY_VISION=OFF \
    -DPROMETHEUS_ENABLE_CUDA=OFF
cmake --build "${node_build}" --parallel 1 --target \
    node_p01_init \
    node_p01_manifest_tests \
    node_p01_identity_probe \
    node_p01_volatile_filesystem_probe \
    node_p01_concurrent_delay_a \
    node_p01_concurrent_delay_b \
    node_p01_required_semantic_success \
    node_p01_optional_intentional_failure \
    node_p01_timeout_probe \
    node_p01_signal_termination_probe

cmake -E rm -rf "${root}"
mkdir -p -- "${root}/dev" "${root}/proc" "${root}/sys" \
    "${root}/run" "${root}/etc/node-p01" "${root}/node/services"
install -m 0755 "${node_build}/assembly/p01-root/init" "${root}/init"
for service in identity_probe volatile_filesystem_probe concurrent_delay_a \
    concurrent_delay_b required_semantic_success optional_intentional_failure \
    timeout_probe signal_termination_probe; do
    install -m 0755 "${node_build}/assembly/p01-root/node/services/${service}" \
        "${root}/node/services/${service}"
done
install -m 0644 \
    "${NODE_REPOSITORY}/assembly/micro_os/manifests/p01-public-startup-v1.manifest" \
    "${root}/etc/node-p01/p01-public-startup-v1.manifest"

while IFS= read -r executable; do
    file -- "${executable}" | grep -Fq 'statically linked' ||
        p01_fail "micro-OS executable is not static: ${executable}"
    if readelf -l -- "${executable}" | grep -Fq 'INTERP'; then
        p01_fail "micro-OS executable has a dynamic interpreter: ${executable}"
    fi
done < <(find "${root}" -type f -perm -0100 -print | LC_ALL=C sort)

echo "P01 micro-OS root staged: ${root}"
