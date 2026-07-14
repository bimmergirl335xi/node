#!/usr/bin/env bash

set -euo pipefail

readonly DEFAULT_REPOSITORY="https://github.com/bimmergirl335xi/node.git"

usage() {
    cat <<'EOF'
Usage:
  tools/sync_node_branch.sh <branch> [path ...]

Examples:
  tools/sync_node_branch.sh agent/arm-a1-linux-auxv
  tools/sync_node_branch.sh main docs/architecture/acs

With a feature branch and no paths, the script imports only files changed by
that branch relative to node/main. For main, provide one or more explicit
paths. Changes are previewed and require confirmation before copying.

Environment:
  PROMETHEUS_NODE_SOURCE_REPOSITORY  Override the source repository URL.
EOF
}

fail() {
    printf 'sync error: %s\n' "$*" >&2
    exit 1
}

allowed_path() {
    case "$1" in
        docs/architecture/acs | docs/architecture/acs/* | src/backends | src/backends/* | tests/unit/backends | tests/unit/backends/*)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

if [[ $# -lt 1 ]] || [[ "${1:-}" == "-h" ]] || [[ "${1:-}" == "--help" ]]; then
    usage
    [[ $# -ge 1 ]] && exit 0
    exit 2
fi

readonly branch="$1"
shift

readonly script_directory="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly destination_root="$(cd -- "${script_directory}/.." && pwd)"
readonly repository="${PROMETHEUS_NODE_SOURCE_REPOSITORY:-${DEFAULT_REPOSITORY}}"
readonly temporary_root="$(mktemp -d -t prometheus-node-sync.XXXXXXXX)"
readonly source_root="${temporary_root}/source"

cleanup() {
    rm -rf -- "${temporary_root}"
}
trap cleanup EXIT

git -C "${destination_root}" rev-parse --is-inside-work-tree >/dev/null 2>&1 ||
    fail "destination is not a Git working tree"

printf 'Downloading %s from %s...\n' "${branch}" "${repository}"
git clone --quiet --single-branch --branch "${branch}" "${repository}" "${source_root}" ||
    fail "could not download source branch '${branch}'"

declare -a requested_paths=("$@")
declare -a files=()

if [[ ${#requested_paths[@]} -eq 0 ]]; then
    [[ "${branch}" != "main" ]] ||
        fail "syncing main requires at least one explicit path"

    git -C "${source_root}" fetch --quiet origin \
        main:refs/remotes/origin/main

    mapfile -t files < <(
        git -C "${source_root}" diff --name-only --diff-filter=ACMRT \
            origin/main...HEAD
    )
else
    for requested_path in "${requested_paths[@]}"; do
        [[ "${requested_path}" != /* ]] ||
            fail "absolute source paths are not allowed: ${requested_path}"
        [[ "${requested_path}" != *".."* ]] ||
            fail "parent path components are not allowed: ${requested_path}"
        allowed_path "${requested_path}" ||
            fail "path is outside the sync allowlist: ${requested_path}"

        if [[ -f "${source_root}/${requested_path}" ]]; then
            files+=("${requested_path}")
        elif [[ -d "${source_root}/${requested_path}" ]]; then
            while IFS= read -r source_file; do
                files+=("${source_file#"${source_root}/"}")
            done < <(find "${source_root}/${requested_path}" -type f -print | sort)
        else
            fail "source path does not exist on '${branch}': ${requested_path}"
        fi
    done
fi

[[ ${#files[@]} -gt 0 ]] || fail "no source changes were selected"

declare -a changed_files=()
for path in "${files[@]}"; do
    allowed_path "${path}" || fail "branch changed a path outside the sync allowlist: ${path}"
    [[ -f "${source_root}/${path}" ]] ||
        fail "deletions are not imported automatically: ${path}"

    if [[ -e "${destination_root}/${path}" ]]; then
        if cmp -s -- "${source_root}/${path}" "${destination_root}/${path}"; then
            continue
        fi
        if [[ -n "$(git -C "${destination_root}" status --porcelain -- "${path}")" ]]; then
            fail "destination has uncommitted changes: ${path}"
        fi
    fi
    changed_files+=("${path}")
done

if [[ ${#changed_files[@]} -eq 0 ]]; then
    printf 'Already synchronized; no files need copying.\n'
    exit 0
fi

printf '\nProposed synchronization:\n'
for path in "${changed_files[@]}"; do
    if [[ -e "${destination_root}/${path}" ]]; then
        printf '\n--- MODIFY %s ---\n' "${path}"
        diff -u -- "${destination_root}/${path}" "${source_root}/${path}" || true
    else
        printf '\n--- ADD %s ---\n' "${path}"
    fi
done

printf '\nCopy %d file(s) into %s? [y/N] ' \
    "${#changed_files[@]}" "${destination_root}"
read -r answer
case "${answer}" in
    y | Y | yes | YES)
        ;;
    *)
        printf 'No files copied.\n'
        exit 0
        ;;
esac

for path in "${changed_files[@]}"; do
    mkdir -p -- "$(dirname -- "${destination_root}/${path}")"
    cp --preserve=mode,timestamps -- "${source_root}/${path}" \
        "${destination_root}/${path}"
done

printf '\nSynchronized files:\n'
printf '  %s\n' "${changed_files[@]}"
printf '\nReview with:\n  git status --short -- %s\n' \
    "${changed_files[*]}"
