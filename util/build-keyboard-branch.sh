#!/usr/bin/env bash
# Copyright 2026 bhe
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

usage() {
    cat >&2 <<'EOF'
usage: util/build-keyboard-branch.sh <keyboard> [keymap] [qmk-ref]

Build a keyboard from its own QMK branch in a disposable worktree. The
default ref is origin/<keyboard>; QMK_FIRMWARE_REF can override it.
EOF
    exit 2
}

if [[ $# -lt 1 || $# -gt 3 ]]; then
    usage
fi

source_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
keyboard=$1
keymap=${2:-default}
qmk_ref=${3:-${QMK_FIRMWARE_REF:-origin/$keyboard}}
keyboard_id=${keyboard//\//_}
keymap_id=${keymap//\//_}
submodule_source_root=${QMK_SUBMODULE_SOURCE_ROOT:-$source_root}
cache_root=${QMK_BRANCH_WORKTREE_ROOT:-"${TMPDIR:-/tmp}/qmk-branch-worktrees"}
artifact_dir=${QMK_ARTIFACT_DIR:-"$source_root/.build/${keyboard_id}_${keymap_id}-rust-qmk"}
target_dir=${RUST_QMK_TARGET_DIR:-"$source_root/.build/rust-target/${keyboard_id}_${keymap_id}"}

if ! qmk_revision=$(git -C "$source_root" rev-parse --verify "${qmk_ref}^{commit}"); then
    echo "QMK firmware ref is unavailable: $qmk_ref" >&2
    exit 1
fi

mkdir -p "$cache_root"
worktree=$(mktemp -d "$cache_root/qmk.XXXXXX")
rmdir "$worktree"

cleanup() {
    git -C "$source_root" worktree remove --force "$worktree" 2>/dev/null || true
    git -C "$source_root" worktree prune --expire now
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 129' HUP

git -C "$source_root" -c checkout.workers=0 \
    worktree add --quiet --detach "$worktree" "$qmk_revision"

# Reuse only initialized submodules whose revisions exactly match the selected
# branch. This avoids a network clone and prevents a build from silently using
# dependencies from a different keyboard branch.
while IFS= read -r submodule_path; do
    if [[ "${submodule_path#*/}" == */* ]]; then
        continue
    fi

    source_path="$submodule_source_root/$submodule_path"
    destination_path="$worktree/$submodule_path"
    if [[ ! -e "$source_path/.git" ]] || \
        ! git -C "$source_path" rev-parse --verify HEAD >/dev/null 2>&1; then
        echo "missing initialized QMK submodule: $source_path" >&2
        exit 1
    fi

    expected_revision=$(git -C "$worktree" ls-tree HEAD "$submodule_path" | awk '{print $3}')
    actual_revision=$(git -C "$source_path" rev-parse HEAD)
    if [[ "$actual_revision" != "$expected_revision" ]]; then
        echo "QMK submodule revision mismatch: $submodule_path" >&2
        echo "expected $expected_revision, found $actual_revision" >&2
        exit 1
    fi

    rmdir "$destination_path"
    ln -s "$source_path" "$destination_path"
done < <(git -C "$source_root" config --file .gitmodules --get-regexp path | awk '{print $2}')

branch_build="$worktree/keyboards/$keyboard/tools/build.sh"
if [[ ! -x "$branch_build" ]]; then
    echo "selected branch has no executable keyboard build contract: $branch_build" >&2
    exit 1
fi

(
    cd "$worktree"
    QMK_KEYBOARD="$keyboard" \
    QMK_KEYMAP="$keymap" \
    QMK_ARTIFACT_DIR="$artifact_dir" \
    RUST_QMK_TARGET_DIR="$target_dir" \
        "$branch_build"
)
