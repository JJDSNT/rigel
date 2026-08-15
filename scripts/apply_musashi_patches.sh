#!/usr/bin/env bash
#
# Apply Rigel's Musashi patches to the external/musashi submodule.
#
# The submodule tracks upstream kstenerud/Musashi unmodified; every local
# change lives in patches/musashi/*.patch so it stays reviewable and can be
# rebased when the submodule pin moves. See patches/musashi/README.md.
#
# Usage:
#   scripts/apply_musashi_patches.sh            apply (idempotent)
#   scripts/apply_musashi_patches.sh --check    report status, change nothing
#   scripts/apply_musashi_patches.sh --revert   restore pristine upstream
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MUSASHI="$ROOT/external/musashi"
PATCHES="$ROOT/patches/musashi"

mode="apply"
case "${1:-}" in
    --check)  mode="check" ;;
    --revert) mode="revert" ;;
    "")       ;;
    *) echo "usage: $0 [--check|--revert]" >&2; exit 2 ;;
esac

if [ ! -f "$MUSASHI/m68kcpu.c" ]; then
    echo "error: external/musashi is empty. Run:" >&2
    echo "    git submodule update --init external/musashi" >&2
    exit 1
fi

cd "$MUSASHI"

if [ "$mode" = "revert" ]; then
    git checkout -- .
    echo "external/musashi restored to pristine upstream."
    exit 0
fi

rc=0
applied=0
skipped=0

for p in "$PATCHES"/*.patch; do
    name="$(basename "$p")"
    if patch -p1 --dry-run --silent --force --reverse <"$p" >/dev/null 2>&1; then
        printf '  %-45s already applied\n' "$name"
        skipped=$((skipped + 1))
        continue
    fi
    if ! patch -p1 --dry-run --silent --force <"$p" >/dev/null 2>&1; then
        printf '  %-45s DOES NOT APPLY\n' "$name"
        rc=1
        continue
    fi
    if [ "$mode" = "check" ]; then
        printf '  %-45s not applied (would apply cleanly)\n' "$name"
        rc=1
        continue
    fi
    patch -p1 --silent --force <"$p"
    printf '  %-45s applied\n' "$name"
    applied=$((applied + 1))
done

if [ "$mode" = "check" ]; then
    [ $rc -eq 0 ] && echo "All Musashi patches are applied."
    exit $rc
fi

echo "Musashi: $applied applied, $skipped already present."
exit $rc
