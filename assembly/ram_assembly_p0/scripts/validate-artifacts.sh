#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
# shellcheck source=lib/p0-common.sh
source "${SCRIPT_DIR}/lib/p0-common.sh"

"${SCRIPT_DIR}/preflight.sh" --stage build
p0_export_ram_environment

kernel="$P0_ARTIFACTS_DIR/node-p0-bzImage"
initramfs="$P0_ARTIFACTS_DIR/node-p0-initramfs.cpio.gz"
p0_verify_fixed_artifact "$kernel"
p0_verify_fixed_artifact "$initramfs"

(
    cd -- "$P0_ARTIFACTS_DIR"
    sha256sum --check kernel.sha256
    sha256sum --check initramfs.sha256
)

gzip -dc "$initramfs" | cpio -it --quiet >"$P0_RECORDS_DIR/initramfs.list"
for required_entry in \
    init \
    bin/p0_test_runner \
    etc/node-p0/tests.manifest \
    tests/test_monotonic_clock \
    tests/test_anonymous_memory \
    tests/test_proc_available \
    tests/test_sys_available \
    tests/test_fork_exec_wait \
    tests/test_timeout_payload \
    tests/test_intentional_failure \
    tests/test_signal_payload \
    tests/test_process_exit; do
    grep -Fxq "$required_entry" "$P0_RECORDS_DIR/initramfs.list" || \
        grep -Fxq "./$required_entry" "$P0_RECORDS_DIR/initramfs.list" || \
        p0_die "initramfs entry missing: $required_entry"
done

[[ -x "$P0_INITRAMFS_ROOT/init" ]] || p0_die "/init is not executable"
[[ -x "$P0_INITRAMFS_ROOT/bin/p0_test_runner" ]] || p0_die "test runner is not executable"
if find "$P0_INITRAMFS_ROOT" -type f \( -name sh -o -name bash -o -name busybox \) -print -quit | grep -q .; then
    p0_die "unexpected shell present in initramfs"
fi

"$P0_INITRAMFS_ROOT/bin/p0_test_runner" \
    --manifest "$P0_INITRAMFS_ROOT/etc/node-p0/tests.manifest" \
    --root "$P0_INITRAMFS_ROOT" \
    >"$P0_TEST_OUTPUT_DIR/host-manifest-validation.jsonl"

grep -Fq '"id":"timeout_enforcement"' "$P0_TEST_OUTPUT_DIR/host-manifest-validation.jsonl"
grep -Fq '"termination":"timeout"' "$P0_TEST_OUTPUT_DIR/host-manifest-validation.jsonl"
grep -Fq '"id":"intentional_semantic_failure"' "$P0_TEST_OUTPUT_DIR/host-manifest-validation.jsonl"
grep -Fq '"semantic":"fail"' "$P0_TEST_OUTPUT_DIR/host-manifest-validation.jsonl"
grep -Fq '"id":"launch_failure"' "$P0_TEST_OUTPUT_DIR/host-manifest-validation.jsonl"
grep -Fq '"termination":"launch_failure"' "$P0_TEST_OUTPUT_DIR/host-manifest-validation.jsonl"
grep -Fq '"id":"signal_termination"' "$P0_TEST_OUTPUT_DIR/host-manifest-validation.jsonl"
grep -Fq '"termination":"signal"' "$P0_TEST_OUTPUT_DIR/host-manifest-validation.jsonl"
grep -Fq '"id":"nonsemantic_process_exit"' "$P0_TEST_OUTPUT_DIR/host-manifest-validation.jsonl"
grep -Fq '"expected":"process_exit"' "$P0_TEST_OUTPUT_DIR/host-manifest-validation.jsonl"
grep -Fq '"outcome":"mechanisms_operated"' "$P0_TEST_OUTPUT_DIR/host-manifest-validation.jsonl"

git -C "$P0_ROOT_DIR/../.." diff --check
{
    sha256sum "$kernel"
    sha256sum "$initramfs"
    sha256sum "$P0_RECORDS_DIR/resolved-kernel.config"
    sha256sum "$P0_TEST_OUTPUT_DIR/host-manifest-validation.jsonl"
} >"$P0_RECORDS_DIR/artifact-validation-inputs.sha256"
kernel_digest="$(sha256sum "$kernel" | awk '{ print $1 }')"
kernel_size="$(stat -Lc '%s' -- "$kernel")"
initramfs_digest="$(sha256sum "$initramfs" | awk '{ print $1 }')"
initramfs_size="$(stat -Lc '%s' -- "$initramfs")"
p0_write_candidate_output_record kernel asm_candidate_kernel_image \
    "$kernel" "$kernel_digest" "$kernel_size" \
    provider_validated_for_p0_conformance_only 2 control_transfer_not_evaluated
p0_write_candidate_output_record initramfs asm_candidate_initramfs_image \
    "$initramfs" "$initramfs_digest" "$initramfs_size" \
    provider_validated_for_p0_conformance_only 2 control_transfer_not_evaluated
p0_write_provider_operation_record \
    completed_with_candidate_outputs pending_tmpfs_disposal \
    candidate_outputs_retained_control_transfer_not_evaluated
"${SCRIPT_DIR}/validate-provider-records.sh" --records "$P0_RECORDS_DIR"
p0_record candidate_output_validation satisfied \
    "kernel=provider_validated initramfs=provider_validated scope=p0_conformance_only"
p0_info "ASM provider records and candidate outputs validated for P0 conformance only"
