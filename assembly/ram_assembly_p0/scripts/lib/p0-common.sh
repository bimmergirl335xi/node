#!/usr/bin/env bash

if [[ -n "${NODE_P0_COMMON_LOADED:-}" ]]; then
    return 0
fi
readonly NODE_P0_COMMON_LOADED=1

set -euo pipefail
IFS=$'\n\t'
umask 077

readonly P0_SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly P0_ROOT_DIR="$(cd -- "${P0_SCRIPT_DIR}/../.." && pwd -P)"
readonly P0_CONFORMANCE_CONTRACT_ID="node.ram-assembly-p0.conformance-authority"
readonly P0_CONFORMANCE_CONTRACT_REVISION=1
readonly P0_ASM_PROVIDER_ID="node.public.ram-assembly-p0.asm-provider"
readonly P0_ASM_PROVIDER_REVISION=1

p0_info() {
    printf 'NODE_P0 INFO %s\n' "$*"
}

p0_warn() {
    printf 'NODE_P0 WARN %s\n' "$*" >&2
}

p0_die() {
    printf 'NODE_P0 ERROR %s\n' "$*" >&2
    exit 1
}

p0_require_command() {
    command -v "$1" >/dev/null 2>&1 || p0_die "required command unavailable: $1"
}

p0_workspace_root() {
    printf '%s\n' "${NODE_P0_WORKSPACE:-/run/node-assembly-p0}"
}

p0_canonical_path() {
    realpath -m -- "$1"
}

p0_assert_beneath() {
    local candidate root
    candidate="$(p0_canonical_path "$1")"
    root="$(p0_canonical_path "$2")"
    case "$candidate" in
        "$root" | "$root"/*) ;;
        *) p0_die "path escapes RAM workspace: $candidate (root $root)" ;;
    esac
}

p0_mount_field() {
    local path field
    path="$1"
    field="$2"
    findmnt -T "$path" -n -o "$field" | head -n 1
}

p0_mount_size_bytes() {
    findmnt -b -T "$1" -n -o SIZE | head -n 1
}

p0_assert_tmpfs() {
    local path fs_type options
    path="$(p0_canonical_path "$1")"
    [[ -e "$path" ]] || p0_die "path does not exist: $path"
    fs_type="$(p0_mount_field "$path" FSTYPE)"
    options="$(p0_mount_field "$path" OPTIONS)"
    [[ "$fs_type" == "tmpfs" ]] || p0_die "path is not backed by tmpfs: $path ($fs_type)"
    case ",$options," in
        *,ro,*) p0_die "tmpfs is read-only: $path" ;;
    esac
    case ",$options," in
        *,noexec,*) p0_die "tmpfs is mounted noexec: $path" ;;
    esac
}

p0_assert_tmpfs_backing() {
    local path fs_type
    path="$(p0_canonical_path "$1")"
    [[ -e "$path" ]] || p0_die "path does not exist: $path"
    fs_type="$(p0_mount_field "$path" FSTYPE)"
    [[ "$fs_type" == "tmpfs" ]] || p0_die "path is not backed by tmpfs: $path ($fs_type)"
}

p0_assert_swap_disabled() {
    local active
    active="$(awk 'NR > 1 { count += 1 } END { print count + 0 }' /proc/swaps)"
    [[ "$active" == "0" ]] || p0_die "strict RAM-only mode refuses active swap ($active entries)"
}

p0_provider_timeout_seconds() {
    local value
    value="${NODE_P0_PROVIDER_TIMEOUT_SECONDS:-21600}"
    [[ "$value" =~ ^[0-9]+$ ]] || p0_die \
        "NODE_P0_PROVIDER_TIMEOUT_SECONDS must be an integer"
    ((value >= 600 && value <= 86400)) || p0_die \
        "provider timeout must be between 600 and 86400 seconds"
    printf '%s\n' "$value"
}

p0_read_context() {
    local context_file key value
    context_file="${P0_RECORDS_DIR:?P0_RECORDS_DIR not exported}/operation-context.env"
    [[ -f "$context_file" ]] || p0_die "P0 operation context is unavailable"

    P0_CONFORMANCE_OPERATION_ID=""
    P0_ASM_PROVIDER_OPERATION_ID=""
    P0_ASM_PROVIDER_ATTEMPT_ID=""
    while IFS='=' read -r key value; do
        case "$key" in
            P0_CONFORMANCE_OPERATION_ID) P0_CONFORMANCE_OPERATION_ID="$value" ;;
            P0_ASM_PROVIDER_OPERATION_ID) P0_ASM_PROVIDER_OPERATION_ID="$value" ;;
            P0_ASM_PROVIDER_ATTEMPT_ID) P0_ASM_PROVIDER_ATTEMPT_ID="$value" ;;
            *) p0_die "invalid P0 operation-context key: $key" ;;
        esac
    done <"$context_file"

    local identity
    for identity in \
        "$P0_CONFORMANCE_OPERATION_ID" \
        "$P0_ASM_PROVIDER_OPERATION_ID" \
        "$P0_ASM_PROVIDER_ATTEMPT_ID"; do
        [[ "$identity" =~ ^[A-Za-z0-9._-]{1,128}$ ]] || p0_die \
            "invalid P0 operation identity"
    done
    export P0_CONFORMANCE_OPERATION_ID
    export P0_ASM_PROVIDER_OPERATION_ID
    export P0_ASM_PROVIDER_ATTEMPT_ID
}

p0_initialize_conformance_context() {
    local context_file contract_copy token operation_id provider_operation_id
    local provider_attempt_id
    context_file="$P0_RECORDS_DIR/operation-context.env"
    contract_copy="$P0_RECORDS_DIR/p0-conformance-authority-v1.json"

    if [[ ! -e "$context_file" ]]; then
        [[ -r /proc/sys/kernel/random/uuid ]] || p0_die \
            "kernel UUID source unavailable for P0 operation identity"
        IFS= read -r token </proc/sys/kernel/random/uuid
        token="${token//-/}"
        [[ "$token" =~ ^[0-9a-f]{32}$ ]] || p0_die \
            "invalid kernel UUID for P0 operation identity"
        operation_id="node.p0.conformance.${token}"
        provider_operation_id="node.asm.p0.provider-operation.${token}"
        provider_attempt_id="node.asm.p0.provider-attempt.${token}.1"
        {
            printf 'P0_CONFORMANCE_OPERATION_ID=%s\n' "$operation_id"
            printf 'P0_ASM_PROVIDER_OPERATION_ID=%s\n' "$provider_operation_id"
            printf 'P0_ASM_PROVIDER_ATTEMPT_ID=%s\n' "$provider_attempt_id"
        } >"$context_file"
        chmod 0600 -- "$context_file"
    fi

    if [[ ! -e "$contract_copy" ]]; then
        cp -- "$P0_ROOT_DIR/contracts/p0-conformance-authority-v1.json" \
            "$contract_copy"
        chmod 0400 -- "$contract_copy"
    fi
    p0_read_context

    local operation_json
    operation_json="$P0_RECORDS_DIR/p0-conformance-operation.json"
    if [[ ! -e "$operation_json" ]]; then
        printf '%s\n' \
            "{\"schema\":\"node.p0.conformance-operation.v1\",\"record_revision\":1,\"p0_conformance_operation_identity\":\"$(p0_json_escape "$P0_CONFORMANCE_OPERATION_ID")\",\"asm_provider_operation_identity\":\"$(p0_json_escape "$P0_ASM_PROVIDER_OPERATION_ID")\",\"asm_provider_attempt_identity\":\"$(p0_json_escape "$P0_ASM_PROVIDER_ATTEMPT_ID")\",\"owning_contract_identity\":\"$P0_CONFORMANCE_CONTRACT_ID\",\"owning_contract_revision\":$P0_CONFORMANCE_CONTRACT_REVISION,\"authority_state\":\"AUTHORITY_NOT_REQUIRED\",\"operation_scope\":\"isolated_non_production_conformance\",\"retention\":\"tmpfs_only\",\"cleanup_state\":\"pending_tmpfs_disposal\",\"unresolved_effects\":[\"candidate_outputs_not_yet_evaluated\"],\"semantic_absences\":[\"boot_recovery_plan\",\"boot_semantic_operation\",\"accepted_krn_artifact\",\"boot_scoped_artifact_acceptance\",\"assembly_generation\",\"installed_instance\",\"activation_result\",\"recovery_result\",\"runtime_readiness_result\"]}" \
            >"$operation_json"
    fi
}

p0_export_ram_environment() {
    local root
    root="$(p0_canonical_path "$(p0_workspace_root)")"
    p0_assert_tmpfs "$root"

    export NODE_P0_WORKSPACE="$root"
    export HOME="$root/home"
    export TMPDIR="$root/tmp"
    export TMP="$root/tmp"
    export TEMP="$root/tmp"
    export XDG_CACHE_HOME="$root/cache/xdg"
    export XDG_CONFIG_HOME="$root/config/xdg"
    export XDG_STATE_HOME="$root/state/xdg"
    export CCACHE_DISABLE=1
    export SCCACHE_DISABLE=1
    export KBUILD_OUTPUT="$root/kernel-build"

    export P0_SOURCE_DIR="$root/kernel-source"
    export P0_INPUTS_DIR="$root/inputs"
    export P0_INITRAMFS_ROOT="$root/initramfs-root"
    export P0_TEST_OUTPUT_DIR="$root/test-output"
    export P0_RECORDS_DIR="$root/records"
    export P0_ARTIFACTS_DIR="$root/artifacts"
    export P0_STAGING_DIR="$root/staging"
    export P0_QUARANTINE_DIR="$root/quarantine"
    export P0_FAILURE_RESERVE_DIR="$root/failure-reserve"

    local path
    for path in \
        "$HOME" "$TMPDIR" "$XDG_CACHE_HOME" "$XDG_CONFIG_HOME" \
        "$XDG_STATE_HOME" "$KBUILD_OUTPUT" "$P0_INPUTS_DIR" \
        "$P0_SOURCE_DIR" "$P0_INITRAMFS_ROOT" "$P0_TEST_OUTPUT_DIR" \
        "$P0_RECORDS_DIR" "$P0_ARTIFACTS_DIR" \
        "$P0_STAGING_DIR" "$P0_QUARANTINE_DIR" "$P0_FAILURE_RESERVE_DIR"; do
        p0_assert_beneath "$path" "$root"
        mkdir -p -- "$path"
    done
    p0_initialize_conformance_context
}

p0_json_escape() {
    local value
    value="$1"
    value="${value//\\/\\\\}"
    value="${value//\"/\\\"}"
    value="${value//$'\t'/\\t}"
    value="${value//$'\r'/\\r}"
    value="${value//$'\n'/\\n}"
    printf '%s' "$value"
}

p0_record() {
    local kind status detail records_file
    kind="$(p0_json_escape "$1")"
    status="$(p0_json_escape "$2")"
    detail="$(p0_json_escape "$3")"
    records_file="${P0_RECORDS_DIR:?P0_RECORDS_DIR not exported}/operations.jsonl"
    printf '{"record":"%s","status":"%s","detail":"%s"}\n' \
        "$kind" "$status" "$detail" >>"$records_file"
}

p0_write_candidate_output_record() {
    local output_kind output_class output_path digest size_bytes validation_state
    local record_revision unresolved_effect output_identity record_path
    output_kind="$1"
    output_class="$2"
    output_path="$(p0_canonical_path "$3")"
    digest="$4"
    size_bytes="$5"
    validation_state="$6"
    record_revision="$7"
    unresolved_effect="$8"

    case "$output_kind" in kernel | initramfs) ;;
        *) p0_die "invalid P0 candidate-output kind: $output_kind" ;;
    esac
    case "$output_class" in asm_candidate_kernel_image | asm_candidate_initramfs_image) ;;
        *) p0_die "invalid P0 candidate-output class: $output_class" ;;
    esac
    [[ "$digest" =~ ^[0-9a-f]{64}$ ]] || p0_die "invalid candidate-output digest"
    [[ "$size_bytes" =~ ^[1-9][0-9]*$ ]] || p0_die "invalid candidate-output size"
    [[ "$record_revision" =~ ^[1-9][0-9]*$ ]] || p0_die \
        "invalid candidate-output record revision"
    p0_assert_beneath "$output_path" "$NODE_P0_WORKSPACE"
    p0_assert_tmpfs "$output_path"
    output_identity="node.p0.candidate.${output_kind}.sha256.${digest}"
    record_path="$P0_RECORDS_DIR/candidate-${output_kind}-output.json"

    printf '%s\n' \
        "{\"schema\":\"node.asm.candidate-output.p0.v1\",\"record_revision\":$record_revision,\"p0_conformance_operation_identity\":\"$(p0_json_escape "$P0_CONFORMANCE_OPERATION_ID")\",\"asm_provider_operation_identity\":\"$(p0_json_escape "$P0_ASM_PROVIDER_OPERATION_ID")\",\"asm_provider_attempt_identity\":\"$(p0_json_escape "$P0_ASM_PROVIDER_ATTEMPT_ID")\",\"provider_identity\":\"$P0_ASM_PROVIDER_ID\",\"provider_revision\":$P0_ASM_PROVIDER_REVISION,\"candidate_output_identity\":\"$output_identity\",\"output_class\":\"$output_class\",\"output_path_or_handle\":\"$(p0_json_escape "$output_path")\",\"digest\":{\"algorithm\":\"sha256\",\"value\":\"$digest\"},\"size_bytes\":$size_bytes,\"provider_validation_state\":\"$(p0_json_escape "$validation_state")\",\"limitations\":[\"authority_not_required_disposable_output\",\"not_a_krn_artifact_acceptance\",\"not_boot_scoped_artifact_acceptance\",\"not_a_generation_member\",\"not_installation_eligible\",\"not_activatable\",\"not_propagatable\",\"not_recovery_material\",\"not_runtime_ready\",\"not_normal_node_or_production_material\"],\"cleanup_state\":\"pending_tmpfs_disposal\",\"unresolved_effects\":[\"$(p0_json_escape "$unresolved_effect")\"]}" \
        >"$record_path"
}

p0_write_provider_operation_record() {
    local outcome cleanup_state unresolved_effect source_revision resolved_config_digest
    local toolchain_digest workspace_limit_bytes workspace_used_bytes available_kib
    local timeout_seconds record_path
    outcome="$1"
    cleanup_state="$2"
    unresolved_effect="$3"
    [[ -s "$P0_RECORDS_DIR/kernel-source-revision.txt" ]] || p0_die \
        "reviewed kernel-source revision record unavailable"
    [[ -s "$P0_RECORDS_DIR/resolved-kernel-config.sha256" ]] || p0_die \
        "resolved configuration digest unavailable"
    [[ -s "$P0_RECORDS_DIR/kernel-toolchain-identity.json" ]] || p0_die \
        "kernel toolchain identity unavailable"
    [[ -s "$P0_RECORDS_DIR/payload-toolchain-identity.json" ]] || p0_die \
        "payload toolchain identity unavailable"
    [[ -s "$P0_RECORDS_DIR/configuration-references.json" ]] || p0_die \
        "configuration references unavailable"

    source_revision="$(<"$P0_RECORDS_DIR/kernel-source-revision.txt")"
    resolved_config_digest="$(<"$P0_RECORDS_DIR/resolved-kernel-config.sha256")"
    [[ "$source_revision" =~ ^[0-9a-f]{40,64}$ ]] || p0_die \
        "invalid reviewed kernel-source revision record"
    [[ "$resolved_config_digest" =~ ^[0-9a-f]{64}$ ]] || p0_die \
        "invalid resolved configuration digest"
    toolchain_digest="$({
        sha256sum "$P0_RECORDS_DIR/kernel-toolchain-identity.json"
        sha256sum "$P0_RECORDS_DIR/payload-toolchain-identity.json"
    } | sha256sum | awk '{ print $1 }')"
    workspace_limit_bytes="$(p0_mount_size_bytes "$NODE_P0_WORKSPACE")"
    workspace_used_bytes="$(du -sb -- "$NODE_P0_WORKSPACE" | awk '{ print $1 }')"
    available_kib="$(awk '/^MemAvailable:/ { print $2; exit }' /proc/meminfo)"
    timeout_seconds="$(p0_provider_timeout_seconds)"
    [[ "$workspace_limit_bytes" =~ ^[0-9]+$ ]] || p0_die "invalid tmpfs size result"
    [[ "$workspace_used_bytes" =~ ^[0-9]+$ ]] || p0_die "invalid tmpfs use result"
    [[ "$available_kib" =~ ^[0-9]+$ ]] || p0_die "invalid RAM availability result"
    record_path="$P0_RECORDS_DIR/asm-provider-operation.json"

    printf '%s\n' \
        "{\"schema\":\"node.asm.provider-operation.p0.v1\",\"record_revision\":1,\"p0_conformance_operation_identity\":\"$(p0_json_escape "$P0_CONFORMANCE_OPERATION_ID")\",\"asm_provider_operation_identity\":\"$(p0_json_escape "$P0_ASM_PROVIDER_OPERATION_ID")\",\"asm_provider_attempt_identity\":\"$(p0_json_escape "$P0_ASM_PROVIDER_ATTEMPT_ID")\",\"provider_identity\":\"$P0_ASM_PROVIDER_ID\",\"provider_revision\":$P0_ASM_PROVIDER_REVISION,\"toolchain_identity\":\"sha256:$toolchain_digest\",\"toolchain_references\":[\"$(p0_json_escape "$P0_RECORDS_DIR/kernel-toolchain-identity.json")\",\"$(p0_json_escape "$P0_RECORDS_DIR/payload-toolchain-identity.json")\"],\"reviewed_kernel_source_revision\":\"$source_revision\",\"configuration_references\":\"$(p0_json_escape "$P0_RECORDS_DIR/configuration-references.json")\",\"resolved_configuration_digest\":\"sha256:$resolved_config_digest\",\"resource_limits\":{\"tmpfs_workspace_bytes\":$workspace_limit_bytes,\"provider_invocation_timeout_seconds\":$timeout_seconds,\"kernel_build_jobs\":1,\"candidate_output_limit\":2,\"test_entry_limit\":32,\"per_test_timeout_max_ms\":60000,\"extra_prebuilt_test_limit\":16,\"control_transfer_limit\":1},\"resource_outcome\":{\"tmpfs_used_bytes\":$workspace_used_bytes,\"available_ram_kib\":$available_kib,\"active_swap_entries\":0},\"provider_outcome\":\"$(p0_json_escape "$outcome")\",\"candidate_output_references\":[\"$(p0_json_escape "$P0_RECORDS_DIR/candidate-kernel-output.json")\",\"$(p0_json_escape "$P0_RECORDS_DIR/candidate-initramfs-output.json")\"],\"cleanup_state\":\"$(p0_json_escape "$cleanup_state")\",\"unresolved_effects\":[\"$(p0_json_escape "$unresolved_effect")\"],\"limitations\":[\"provider_result_only\",\"no_boot_semantic_operation\",\"no_artifact_acceptance\",\"no_generation_membership\",\"no_installation\",\"no_activation\",\"no_recovery\",\"no_readiness\"]}" \
        >"$record_path"
}

p0_write_control_transfer_record() {
    local state unresolved_effect
    state="$1"
    unresolved_effect="$2"
    printf '%s\n' \
        "{\"schema\":\"node.p0.control-transfer-observation.v1\",\"record_revision\":1,\"p0_conformance_operation_identity\":\"$(p0_json_escape "$P0_CONFORMANCE_OPERATION_ID")\",\"asm_provider_operation_identity\":\"$(p0_json_escape "$P0_ASM_PROVIDER_OPERATION_ID")\",\"state\":\"$(p0_json_escape "$state")\",\"semantic_scope\":\"p0_conformance_only\",\"unresolved_effects\":[\"$(p0_json_escape "$unresolved_effect")\"],\"activation_result\":\"not_created\",\"recovery_result\":\"not_created\",\"runtime_readiness_result\":\"not_created\"}" \
        >"$P0_RECORDS_DIR/control-transfer-observation.json"
}

p0_require_ordinary_user() {
    [[ "${EUID}" -ne 0 ]] || p0_die \
        "run the P0 orchestration as an ordinary user; only printed leaf commands may use privilege"
}

p0_resolve_system_binary() {
    local candidate canonical mode
    for candidate in "$@"; do
        if [[ -f "$candidate" && -x "$candidate" ]]; then
            canonical="$(realpath -e -- "$candidate")"
            case "$canonical" in
                /bin/* | /sbin/* | /usr/bin/* | /usr/sbin/*)
                    mode="$(stat -Lc '%a' -- "$canonical")"
                    [[ "$mode" =~ ^[0-7]+$ ]] || continue
                    (((8#$mode & 0022) == 0)) || continue
                    printf '%s\n' "$canonical"
                    return 0
                    ;;
            esac
        fi
    done
    p0_die "required fixed-path system binary unavailable: $*"
}

_p0_print_exact_command() {
    local argument
    printf 'NODE_P0 PRIVILEGED_COMMAND'
    for argument in "$@"; do
        printf ' %q' "$argument"
    done
    printf '\n'
}

p0_print_privileged_command() {
    local sudo_binary env_binary
    local -a command
    sudo_binary="$(p0_resolve_system_binary /usr/bin/sudo /bin/sudo)"
    env_binary="$(p0_resolve_system_binary /usr/bin/env /bin/env)"
    command=(
        "$sudo_binary" -- "$env_binary" -i
        'PATH=/usr/sbin:/usr/bin:/sbin:/bin' 'LC_ALL=C' "$@"
    )
    _p0_print_exact_command "${command[@]}"
}

p0_run_privileged_command() {
    local sudo_binary env_binary
    local -a command
    sudo_binary="$(p0_resolve_system_binary /usr/bin/sudo /bin/sudo)"
    env_binary="$(p0_resolve_system_binary /usr/bin/env /bin/env)"
    command=(
        "$sudo_binary" -- "$env_binary" -i
        'PATH=/usr/sbin:/usr/bin:/sbin:/bin' 'LC_ALL=C' "$@"
    )

    _p0_print_exact_command "${command[@]}"
    "${command[@]}"
}

p0_verify_fixed_artifact() {
    local artifact
    artifact="$1"
    p0_assert_beneath "$artifact" "${NODE_P0_WORKSPACE:?}"
    p0_assert_tmpfs "$artifact"
    [[ -f "$artifact" ]] || p0_die "artifact missing: $artifact"
    [[ -s "$artifact" ]] || p0_die "artifact empty: $artifact"
}
