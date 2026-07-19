#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 5 ]]; then
    echo "usage: $0 ROOT INIT PUBLIC_MANIFEST REQUIRED_FAILURE_MANIFEST LAUNCH_FAILURE_MANIFEST" >&2
    exit 64
fi

root=$1
init=$2
public_manifest=$3
required_failure_manifest=$4
launch_failure_manifest=$5
output_dir="${root}/host-results"
mkdir -p "${root}/run/node-p01-results" "${output_dir}"

"${init}" --host-root "${root}" --manifest "${public_manifest}" \
    > "${output_dir}/public.out" 2>&1

grep -F '"outcome":"accepted_for_p01_structural_scope"' \
    "${output_dir}/public.out"
grep -F '"subject":"same_stage_services","outcome":"overlap_observed"' \
    "${output_dir}/public.out"
grep -F '"subject":"timeout_probe","outcome":"timeout"' \
    "${output_dir}/public.out"
grep -F '"subject":"signal_termination_probe","outcome":"signal_termination"' \
    "${output_dir}/public.out"
grep -F '"subject":"optional_intentional_failure","outcome":"semantic_failure"' \
    "${output_dir}/public.out"
grep -F '"subject":"pid1_children","outcome":"zombie_free"' \
    "${output_dir}/public.out"
grep -F '"outcome":"p01_terminal_state_reached"' "${output_dir}/public.out"

if "${init}" --host-root "${root}" --manifest "${required_failure_manifest}" \
    > "${output_dir}/required-failure.out" 2>&1; then
    echo "required service failure unexpectedly returned zero" >&2
    exit 1
fi
grep -F '"subject":"blocked_later_service","outcome":"cancellation"' \
    "${output_dir}/required-failure.out"
grep -F '"outcome":"required_service_failure"' \
    "${output_dir}/required-failure.out"

if "${init}" --host-root "${root}" --manifest "${launch_failure_manifest}" \
    > "${output_dir}/launch-failure.out" 2>&1; then
    echo "launch failure unexpectedly returned zero" >&2
    exit 1
fi
grep -F '"subject":"missing_executable","outcome":"launch_failure"' \
    "${output_dir}/launch-failure.out"

if "${init}" --host-root "${root}" --manifest "${public_manifest}" \
    --cmdline 'node.micro_os.hold_seconds=unbounded' \
    > "${output_dir}/malformed-cmdline.out" 2>&1; then
    echo "malformed kernel command line unexpectedly returned zero" >&2
    exit 1
fi
grep -F '"subject":"node.micro_os","outcome":"manifest_rejection"' \
    "${output_dir}/malformed-cmdline.out"

echo "P01 supervisor host tests passed"
