#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "$0")/lib-p01.sh"

[[ $# -eq 1 ]] || p01_fail 'usage: build-micro-os.sh OUTPUT_DIR'
output_dir=$(p01_assert_output_directory "$1")
node_build="${output_dir}/node-build"
root="${output_dir}/initramfs-root"

p01_require_command cmake
p01_require_command env
p01_require_command file
p01_require_command nm
p01_require_command readelf
p01_require_command sha256sum
p01_require_command stat
p01_require_command tail
p01_require_command timeout
mkdir -p -- "${output_dir}"
cmake -S "${NODE_REPOSITORY}" -B "${node_build}" \
    -DPROMETHEUS_BUILD_TESTS=ON \
    -DPROMETHEUS_BUILD_BENCHMARKS=OFF \
    -DPROMETHEUS_BUILD_LEGACY_VISION=OFF \
    -DPROMETHEUS_ENABLE_CUDA=OFF
runtime_build_environment="${output_dir}/cpu-runtime-build-environment"
runtime_build_log="${output_dir}/cpu-runtime-build.log"
mkdir -p -- "${runtime_build_environment}/home" \
    "${runtime_build_environment}/tmp"
cmake_command=$(realpath -e -- "$(command -v cmake)")
timeout_command=$(realpath -e -- "$(command -v timeout)")
if ! env -i PATH=/usr/bin:/bin LC_ALL=C LANG=C \
    HOME="${runtime_build_environment}/home" \
    TMPDIR="${runtime_build_environment}/tmp" \
    CMAKE_BUILD_PARALLEL_LEVEL=1 MAKEFLAGS=-j1 \
    "${timeout_command}" --signal=TERM --kill-after=5s 120s \
    "${cmake_command}" --build "${node_build}" --parallel 1 --target \
        node_p01_cpu_runtime_candidate \
        node_p01_cpu_runtime_provenance \
        > "${runtime_build_log}" 2>&1; then
    if [[ $(stat -c '%s' -- "${runtime_build_log}") -gt 262144 ]]; then
        tail -c 262144 -- "${runtime_build_log}" \
            > "${runtime_build_log}.bounded"
        mv -f -- "${runtime_build_log}.bounded" "${runtime_build_log}"
    fi
    tail -n 80 -- "${runtime_build_log}" >&2
    p01_fail 'bounded CPU runtime candidate build failed'
fi
if [[ $(stat -c '%s' -- "${runtime_build_log}") -gt 262144 ]]; then
    tail -c 262144 -- "${runtime_build_log}" > "${runtime_build_log}.bounded"
    mv -f -- "${runtime_build_log}.bounded" "${runtime_build_log}"
fi
cmake --build "${node_build}" --parallel 1 --target \
    node_p01_init \
    node_p01_manifest_tests \
    node_cpu_assembly_decision_tests \
    node_cpu_runtime_assembly_tests \
    node_p01_identity_probe \
    node_p01_volatile_filesystem_probe \
    node_p01_concurrent_delay_a \
    node_p01_concurrent_delay_b \
    node_p01_required_semantic_success \
    node_p01_optional_intentional_failure \
    node_p01_timeout_probe \
    node_p01_signal_termination_probe \
    node_p01_cpu_assembly_decision \
    node_p01_cpu_runtime_assembly \
    node_p01_acs_reference_transport

cmake -E rm -rf "${root}"
mkdir -p -- "${root}/dev" "${root}/proc" "${root}/sys" \
    "${root}/run" "${root}/etc/node-p01" "${root}/node/services" \
    "${root}/node/candidates"
install -m 0755 "${node_build}/assembly/p01-root/init" "${root}/init"
for service in identity_probe volatile_filesystem_probe concurrent_delay_a \
    concurrent_delay_b required_semantic_success optional_intentional_failure \
    timeout_probe signal_termination_probe cpu_assembly_decision \
    cpu_runtime_assembly acs_reference_transport; do
    install -m 0755 "${node_build}/assembly/p01-root/node/services/${service}" \
        "${root}/node/services/${service}"
done
runtime_candidate="${root}/node/candidates/cpu-runtime-x86_64-avx2"
install -m 0555 \
    "${node_build}/assembly/p01-root/node/candidates/cpu-runtime-x86_64-avx2" \
    "${runtime_candidate}"
install -m 0444 \
    "${node_build}/assembly/p01-root/node/candidates/cpu-runtime-x86_64-avx2.provenance" \
    "${root}/node/candidates/cpu-runtime-x86_64-avx2.provenance"
runtime_size=$(stat -c '%s' -- "${runtime_candidate}")
[[ ${runtime_size} =~ ^[1-9][0-9]*$ && ${runtime_size} -le 4194304 ]] ||
    p01_fail 'CPU runtime candidate exceeds its 4 MiB bound'
runtime_sha256=$(p01_sha256 "${runtime_candidate}")
[[ ${runtime_sha256} =~ ^[0-9a-f]{64}$ ]] ||
    p01_fail 'CPU runtime candidate digest is malformed'
nm -g --defined-only "${runtime_candidate}" |
    grep -Eq ' [TW] node_cpu_runtime_conformance_v1$' ||
    p01_fail 'CPU runtime candidate ABI symbol is missing'
grep -Fxq "artifact_sha256=${runtime_sha256}" \
    "${root}/node/candidates/cpu-runtime-x86_64-avx2.provenance" ||
    p01_fail 'generated CPU runtime provenance digest does not match'
grep -Fxq "artifact_size=${runtime_size}" \
    "${root}/node/candidates/cpu-runtime-x86_64-avx2.provenance" ||
    p01_fail 'generated CPU runtime provenance size does not match'
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
