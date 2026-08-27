#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "$0")/lib-p01.sh"

[[ $# -eq 1 ]] || p01_fail 'usage: emit-candidate-records.sh OUTPUT_DIR'
output_dir=$(p01_assert_output_directory "$1")
artifact_dir="${output_dir}/artifacts"
record_dir="${output_dir}/records"
root="${output_dir}/initramfs-root"
mkdir -p -- "${record_dir}"

compiler_identity="gcc:$(gcc -dumpfullversion -dumpversion)+ld:$(ld -dumpversion)+make:$(make --version | awk 'NR == 1 {print $NF}')"
p01_safe_token "${compiler_identity}"
node_revision=$(git -C "${NODE_REPOSITORY}" rev-parse HEAD)
p01_safe_token "${node_revision}"
config_digest=$(tr -d '\n' < "${artifact_dir}/node-p01-kernel-config.sha256")
[[ ${config_digest} =~ ^[0-9a-f]{64}$ ]] || p01_fail 'invalid kernel configuration digest'

emit_record() {
    local record_name=$1
    local output_class=$2
    local artifact_path=$3
    local source_reference=$4
    local configuration_reference=$5
    local configuration_inputs=$6
    local limitations=$7
    local digest=''
    local size=0
    local state='candidate_produced'
    if [[ -f ${artifact_path} && ! -L ${artifact_path} ]]; then
        digest=$(p01_sha256 "${artifact_path}")
        size=$(p01_size "${artifact_path}")
    else
        state='unavailable'
    fi
    p01_safe_token "${record_name}"
    p01_safe_token "${output_class}"
    p01_safe_token "${source_reference}"
    p01_safe_token "${configuration_reference}"
    p01_safe_token "${configuration_inputs}"
    p01_safe_token "${limitations}"
    printf '%s\n' "{
  \"record_identity\": \"p01-candidate:${record_name}:1\",
  \"record_revision\": 1,
  \"provider_operation_identity\": \"p01-provider-operation:build:1\",
  \"provider_attempt_identity\": \"p01-provider-attempt:local:1\",
  \"output_class\": \"${output_class}\",
  \"output_state\": \"${state}\",
  \"artifact_name\": \"$(basename -- "${artifact_path}")\",
  \"repository_identity\": \"${P01_KERNEL_REPOSITORY}\",
  \"source_reference\": \"${source_reference}\",
  \"build_reference\": \"node-p01-build:1\",
  \"configuration_reference\": \"${configuration_reference}\",
  \"configuration_input_references\": \"${configuration_inputs}\",
  \"toolchain_identity\": \"${compiler_identity}\",
  \"sha256\": \"${digest}\",
  \"size_bytes\": ${size},
  \"validation_boundary\": \"p01_candidate_mechanism_only\",
  \"cleanup_state\": \"retained_in_named_build_directory\",
  \"limitation\": \"${limitations}\",
  \"unresolved_effects\": \"none_observed_no_device_write_attempted\",
  \"artifact_acceptance\": false,
  \"assembly_generation_membership\": false,
  \"installation_eligibility\": false,
  \"activation\": false,
  \"recovery\": false,
  \"runtime_readiness\": false
}" > "${record_dir}/${record_name}.json"
}

emit_record kernel-image asm_candidate_kernel_image \
    "${artifact_dir}/node-p01-bzImage" \
    "kernel-revision:${P01_KERNEL_REVISION}" \
    "kernel-config-sha256:${config_digest}" \
    "x86_64_defconfig+common.config+x86_64.config+dell_wyse_5070.config" \
    "dell-wyse-5070-first-target-not-physical-validation"
emit_record p01-initramfs asm_candidate_initramfs_image \
    "${artifact_dir}/node-p01-initramfs.cpio.gz" \
    "node-revision:${node_revision}" \
    "p01-initramfs-profile:1" \
    "tracked-micro-os-root+tracked-startup-manifest" \
    "x86_64-static-userspace"
emit_record micro-os-executable asm_candidate_micro_os_executable \
    "${root}/init" \
    "node-revision:${node_revision}" \
    "micro-os-profile:p01:1" \
    "node_init.c+node_p01_manifest.c" \
    "p01-conformance-only"
emit_record startup-manifest asm_candidate_startup_manifest \
    "${root}/etc/node-p01/p01-public-startup-v1.manifest" \
    "node-revision:${node_revision}" \
    "p01-startup-manifest:1" \
    "p01-public-startup-v1.manifest" \
    "not-a-boot-generation-manifest"
emit_record removable-media-image asm_candidate_removable_media_image \
    "${artifact_dir}/node-p01-x86_64.iso" \
    "kernel-revision:${P01_KERNEL_REVISION}" \
    "grub-hybrid-iso-profile:1" \
    "grub.cfg+kernel-image+p01-initramfs" \
    "bios-and-uefi-support-requires-host-inspection"

(
    cd -- "${artifact_dir}"
    find . -maxdepth 1 -type f ! -name SHA256SUMS -print0 | LC_ALL=C sort -z |
        xargs -0 -r sha256sum > SHA256SUMS
)
echo "P01 candidate records: ${record_dir}"
