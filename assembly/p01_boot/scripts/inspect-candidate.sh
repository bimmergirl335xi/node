#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "$0")/lib-p01.sh"

[[ $# -eq 1 ]] || p01_fail 'usage: inspect-candidate.sh OUTPUT_DIR'
output_dir=$(p01_assert_output_directory "$1")
artifact_dir="${output_dir}/artifacts"
root="${output_dir}/initramfs-root"
validation="${output_dir}/validation"
mkdir -p -- "${validation}"

gzip -dc "${artifact_dir}/node-p01-initramfs.cpio.gz" |
    cpio -it --quiet > "${validation}/initramfs.list"
for member in init etc/node-p01/p01-public-startup-v1.manifest \
    node/services/identity_probe node/services/volatile_filesystem_probe \
    node/services/concurrent_delay_a node/services/concurrent_delay_b \
    node/services/required_semantic_success \
    node/services/optional_intentional_failure node/services/timeout_probe \
    node/services/signal_termination_probe \
    node/services/cpu_assembly_decision \
    node/services/cpu_runtime_assembly \
    node/candidates/cpu-runtime-x86_64-avx2 \
    node/candidates/cpu-runtime-x86_64-avx2.provenance \
    node/services/acs_reference_transport; do
    grep -Eq "^(\\./)?${member}$" "${validation}/initramfs.list" ||
        p01_fail "initramfs member missing: ${member}"
done

runtime_candidate="${root}/node/candidates/cpu-runtime-x86_64-avx2"
runtime_provenance="${root}/node/candidates/cpu-runtime-x86_64-avx2.provenance"
runtime_sha256=$(sed -n 's/^artifact_sha256=//p' "${runtime_provenance}")
runtime_size=$(sed -n 's/^artifact_size=//p' "${runtime_provenance}")
[[ ${runtime_sha256} =~ ^[0-9a-f]{64}$ ]] ||
    p01_fail 'CPU runtime provenance digest is malformed'
[[ ${runtime_size} =~ ^[1-9][0-9]*$ && ${runtime_size} -le 4194304 ]] ||
    p01_fail 'CPU runtime provenance size is invalid'
[[ $(p01_sha256 "${runtime_candidate}") == "${runtime_sha256}" ]] ||
    p01_fail 'CPU runtime candidate digest validation failed'
[[ $(p01_size "${runtime_candidate}") == "${runtime_size}" ]] ||
    p01_fail 'CPU runtime candidate size validation failed'
grep -Fxq 'profile=avx2' "${runtime_provenance}" ||
    p01_fail 'CPU runtime candidate profile is invalid'
grep -Fxq 'compile_location=host-canonical-image-build' \
    "${runtime_provenance}" ||
    p01_fail 'CPU runtime candidate compilation location is invalid'

while IFS= read -r executable; do
    file -- "${executable}" | grep -Fq 'statically linked' ||
        p01_fail "dynamic executable in P01 root: ${executable}"
    if readelf -l -- "${executable}" | grep -Fq 'INTERP'; then
        p01_fail "interpreter segment in P01 executable: ${executable}"
    fi
done < <(find "${root}" -type f -perm -0100 -print | LC_ALL=C sort)

grep -Fq 'console=ttyS0,115200n8 console=tty0 rdinit=/init' \
    "${P01_DIR}/config/grub.cfg"
grep -Fq 'node.micro_os.manifest=/etc/node-p01/p01-public-startup-v1.manifest' \
    "${P01_DIR}/config/grub.cfg"

if [[ -f ${artifact_dir}/node-p01-x86_64.iso ]]; then
    p01_require_command xorriso
    xorriso -indev "${artifact_dir}/node-p01-x86_64.iso" \
        -find / -type f -exec echo \
        > "${validation}/iso.list" 2>&1
    grep -Fq '/boot/grub/grub.cfg' "${validation}/iso.list"
    grep -Fq '/boot/node-p01-bzImage' "${validation}/iso.list"
    grep -Fq '/boot/node-p01-initramfs.cpio.gz' "${validation}/iso.list"
    xorriso -indev "${artifact_dir}/node-p01-x86_64.iso" \
        -report_el_torito plain > "${validation}/iso-boot-report.txt" 2>&1
    bios=false
    uefi=false
    if grep -Fq 'BIOS' "${validation}/iso-boot-report.txt"; then bios=true; fi
    if grep -Eq 'UEFI|EFI' "${validation}/iso-boot-report.txt"; then uefi=true; fi
    printf '%s\n' \
        "{\"test\":\"p01_iso_firmware_inspection\",\"status\":\"inspected\",\"bios_entry\":${bios},\"uefi_entry\":${uefi}}" \
        > "${validation}/iso-firmware.json"
fi

(
    cd -- "${artifact_dir}"
    sha256sum --check SHA256SUMS
)
echo 'P01 candidate inspection passed'
