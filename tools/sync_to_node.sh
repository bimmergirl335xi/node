#!/usr/bin/env bash

set -euo pipefail

readonly script_directory="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly source_root="$(cd -- "${script_directory}/.." && pwd)"
readonly default_destination="$(cd -- "${source_root}/.." && pwd)/node"
readonly destination_root="${1:-${default_destination}}"

fail() {
    printf 'sync error: %s\n' "$*" >&2
    exit 1
}

publishable_path() {
    case "$1" in
        build | build/* | build-* | build-*/* | test_build | test_build/* | \
        data | data/* | downloads | downloads/* | logs | logs/* | runtime | runtime/* | \
        provisioning | provisioning/* | prometheus_phase_* | prometheus_phase_*/* | \
        *.csv | *.patch)
            return 1
            ;;
        *)
            return 0
            ;;
    esac
}

git -C "${source_root}" rev-parse --is-inside-work-tree >/dev/null 2>&1 ||
    fail "source is not a Git working tree: ${source_root}"
git -C "${destination_root}" rev-parse --is-inside-work-tree >/dev/null 2>&1 ||
    fail "destination is not a Git working tree: ${destination_root}"
[[ -z "$(git -C "${destination_root}" status --porcelain)" ]] ||
    fail "destination working tree must be clean"

declare -a files=()
mapfile -d '' -t files < <(git -C "${source_root}" ls-files -z)

# Stitches may be reviewed in prometheus_node before they are committed there.
# Include only source-shaped untracked files; build and runtime artifacts remain
# excluded even when the repository's ignore rules do not cover them yet.
while IFS= read -r -d '' path; do
    case "${path}" in
        docs/architecture/acs/* | tests/unit/backends/* | tools/*.sh)
            files+=("${path}")
            ;;
    esac
done < <(git -C "${source_root}" ls-files --others --exclude-standard -z)

declare -a changed_files=()
for path in "${files[@]}"; do
    publishable_path "${path}" || continue
    [[ -f "${source_root}/${path}" ]] ||
        fail "tracked source path is not a regular file: ${path}"

    if [[ -e "${destination_root}/${path}" ]] &&
       cmp -s -- "${source_root}/${path}" "${destination_root}/${path}"; then
        continue
    fi

    changed_files+=("${path}")
done

if [[ ${#changed_files[@]} -eq 0 ]]; then
    printf 'Already synchronized; no files need copying.\n'
    exit 0
fi

printf 'Source:      %s\n' "${source_root}"
printf 'Destination: %s\n' "${destination_root}"
printf 'Files to copy: %d\n\n' "${#changed_files[@]}"
printf '  %s\n' "${changed_files[@]}"
printf '\nCopy these files without deleting destination files? [y/N] '
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

printf '\nSynchronized %d file(s). Review with:\n' "${#changed_files[@]}"
printf '  git -C %q status --short\n' "${destination_root}"
