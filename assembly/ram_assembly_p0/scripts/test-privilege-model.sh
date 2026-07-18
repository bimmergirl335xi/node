#!/usr/bin/env bash
set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly P0_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
# shellcheck source=lib/p0-common.sh
source "${SCRIPT_DIR}/lib/p0-common.sh"
p0_require_ordinary_user

if grep -R -nE '^[[:space:]]*sudo[[:space:]]+-E' "$P0_DIR"; then
    p0_die "whole-environment sudo remains in the P0 implementation"
fi

if grep -R -nE '^[[:space:]]*(sudo|mount|kexec)([[:space:]]|$)' "$SCRIPT_DIR"; then
    p0_die "direct privileged command bypasses the narrow invocation helper"
fi

rendered="$(p0_print_privileged_command /usr/bin/true --example 'value with space')"
[[ "$rendered" == NODE_P0\ PRIVILEGED_COMMAND* ]] || \
    p0_die "privileged command is not visibly labelled"
[[ "$rendered" == *" /usr/bin/env -i "* || "$rendered" == *" /bin/env -i "* ]] || \
    p0_die "privileged command does not clear the inherited environment"
[[ "$rendered" == *" PATH=/usr/sbin:/usr/bin:/sbin:/bin "* ]] || \
    p0_die "privileged command lacks its explicit minimal PATH"
[[ "$rendered" == *" LC_ALL=C "* ]] || \
    p0_die "privileged command lacks its explicit locale"
[[ "$rendered" == *"value\\ with\\ space"* ]] || \
    p0_die "privileged command inputs are not shell-quoted"

grep -Fq 'p0_run_privileged_command "$kexec_binary" -l' \
    "$SCRIPT_DIR/load-kexec.sh"
grep -Fq 'p0_run_privileged_command "$kexec_binary" -e' \
    "$SCRIPT_DIR/execute-kexec.sh"
grep -Fq -- '--i-understand-control-transfers-now' \
    "$SCRIPT_DIR/execute-kexec.sh"
grep -Fq 'p0_require_ordinary_user' "$SCRIPT_DIR/preflight.sh"
grep -Fq 'p0_require_ordinary_user' "$SCRIPT_DIR/create-ram-workspace.sh"

printf 'NODE_P0 PRIVILEGE ordinary-user orchestration and narrow leaf commands validated\n'
