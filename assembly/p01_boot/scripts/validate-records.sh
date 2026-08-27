#!/usr/bin/env bash
set -euo pipefail
source "$(dirname -- "$0")/lib-p01.sh"

[[ $# -eq 1 ]] || p01_fail 'usage: validate-records.sh OUTPUT_DIR'
output_dir=$(p01_assert_output_directory "$1")
record_dir="${output_dir}/records"
expected=(kernel-image p01-initramfs micro-os-executable startup-manifest removable-media-image)
for name in "${expected[@]}"; do
    record="${record_dir}/${name}.json"
    p01_assert_regular_input "${record}"
    python3 -m json.tool "${record}" >/dev/null
    grep -Fq '"artifact_acceptance": false' "${record}"
    grep -Fq '"assembly_generation_membership": false' "${record}"
    grep -Fq '"installation_eligibility": false' "${record}"
    grep -Fq '"runtime_readiness": false' "${record}"
done
echo 'P01 candidate-record validation passed'
