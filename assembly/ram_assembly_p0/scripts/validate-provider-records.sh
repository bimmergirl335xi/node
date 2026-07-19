#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=lib/p0-common.sh
source "${SCRIPT_DIR}/lib/p0-common.sh"
p0_require_ordinary_user

records_dir=""
host_validation=0
while (($# > 0)); do
    case "$1" in
        --records)
            (($# >= 2)) || p0_die "--records requires a path"
            records_dir="$2"
            shift 2
            ;;
        --host-validation)
            host_validation=1
            shift
            ;;
        *) p0_die "unknown argument: $1" ;;
    esac
done
[[ -n "$records_dir" ]] || p0_die \
    "usage: validate-provider-records.sh --records PATH [--host-validation]"
records_dir="$(p0_canonical_path "$records_dir")"
[[ -d "$records_dir" ]] || p0_die "provider-record directory unavailable: $records_dir"
if ((host_validation == 0)); then
    p0_assert_beneath "$records_dir" "${NODE_P0_WORKSPACE:?NODE_P0_WORKSPACE not exported}"
    p0_assert_tmpfs "$records_dir"
fi

required_files=(
    p0-conformance-authority-v1.json
    p0-conformance-operation.json
    asm-provider-operation.json
    candidate-kernel-output.json
    candidate-initramfs-output.json
)
for required_file in "${required_files[@]}"; do
    [[ -s "$records_dir/$required_file" ]] || p0_die \
        "required P0 provider record unavailable: $required_file"
    [[ "$(wc -c <"$records_dir/$required_file")" -le 32768 ]] || p0_die \
        "P0 provider record exceeds 32768-byte bound: $required_file"
done

grep -Fq '"authority_state": "AUTHORITY_NOT_REQUIRED"' \
    "$records_dir/p0-conformance-authority-v1.json"
grep -Fq '"operation_scope":"isolated_non_production_conformance"' \
    "$records_dir/p0-conformance-operation.json"
grep -Fq '"asm_provider_operation_identity"' \
    "$records_dir/p0-conformance-operation.json"
grep -Fq '"asm_provider_attempt_identity"' \
    "$records_dir/p0-conformance-operation.json"

provider_record="$records_dir/asm-provider-operation.json"
for provider_field in \
    p0_conformance_operation_identity \
    asm_provider_operation_identity \
    asm_provider_attempt_identity \
    provider_identity \
    toolchain_identity \
    reviewed_kernel_source_revision \
    configuration_references \
    resource_limits \
    resource_outcome \
    provider_outcome \
    cleanup_state \
    unresolved_effects; do
    grep -Fq "\"$provider_field\"" "$provider_record" || p0_die \
        "provider operation record lacks field: $provider_field"
done

for candidate_kind in kernel initramfs; do
    candidate_record="$records_dir/candidate-${candidate_kind}-output.json"
    for candidate_field in \
        candidate_output_identity \
        output_class \
        output_path_or_handle \
        digest \
        size_bytes \
        provider_validation_state \
        limitations \
        cleanup_state \
        unresolved_effects; do
        grep -Fq "\"$candidate_field\"" "$candidate_record" || p0_die \
            "$candidate_kind candidate record lacks field: $candidate_field"
    done
    grep -Eq '"value":"[0-9a-f]{64}"' "$candidate_record" || p0_die \
        "$candidate_kind candidate record has invalid digest"
    grep -Eq '"size_bytes":[1-9][0-9]*' "$candidate_record" || p0_die \
        "$candidate_kind candidate record has invalid size"
done

if grep -E '"(krn_artifact_acceptance|boot_scoped_artifact_acceptance|generation_membership|installation_eligibility|activation|recovery|readiness)"[[:space:]]*:' \
    "$provider_record" \
    "$records_dir/candidate-kernel-output.json" \
    "$records_dir/candidate-initramfs-output.json"; then
    p0_die "provider or candidate record contains a prohibited downstream claim"
fi

printf 'NODE_P0 RECORD provider and candidate-output contracts validated\n'
