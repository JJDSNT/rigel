#!/usr/bin/env bash
#
# Regression test for the FPU work in patches/musashi/0002 and 0005.
#
# Generates a tiny m68k test ROM that exercises every opmode those patches add,
# assembles it with the cross toolchain, boots it in the harness as a 68040,
# and diffs the results it left in Chip RAM against values computed here.
#
# If an opmode is missing, Musashi takes an F-line trap — which patch 0004
# prints — so the test catches both "wrong answer" and "not implemented".
#
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
HARNESS="$ROOT/build-harness/rigel-harness"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

if [ ! -x "$HARNESS" ]; then
    echo "[fpu-test] harness not built; run ./run.sh build" >&2
    exit 1
fi

"$ROOT/scripts/m68k" --check >/dev/null || exit 1

echo "[fpu-test] generating fputest.S..."
python3 "$HERE/gen_fputest.py"

echo "[fpu-test] assembling..."
"$ROOT/scripts/m68k" vasmm68k_mot -Fbin -quiet \
    -o "$WORK/fputest.bin" "$HERE/fputest.S"

python3 - "$WORK/fputest.bin" "$WORK/fputest.rom" <<'PY'
import sys
data = open(sys.argv[1], "rb").read()
size = 256 * 1024
if len(data) > size:
    sys.exit("image is larger than 256K")
open(sys.argv[2], "wb").write(data + b"\x00" * (size - len(data)))
PY

echo "[fpu-test] running (--cpu 68040)..."
set +e
"$HARNESS" "$WORK/fputest.rom" --cpu 68040 --headless --frames 5 \
    --dump "1000:60:$WORK/results.bin" \
    > "$WORK/run.log" 2>&1
rc=$?
set -e

if [ "$rc" -ne 0 ]; then
    echo "[fpu-test] FAIL: harness exited $rc"
    cat "$WORK/run.log"
    exit 1
fi

# An unimplemented opcode traps rather than returning a wrong number, and the
# result buffer would then hold stale zeros that could pass by accident.
if grep -q "F-LINE-TRAP\|040FPU0-GATE-FAIL" "$WORK/run.log"; then
    echo "[fpu-test] FAIL: an FPU opcode trapped (unimplemented)"
    grep "F-LINE-TRAP\|040FPU0-GATE-FAIL" "$WORK/run.log"
    exit 1
fi

if [ ! -s "$WORK/results.bin" ]; then
    echo "[fpu-test] FAIL: no result dump — see below"
    cat "$WORK/run.log"
    exit 1
fi

echo "[fpu-test] verifying results..."
python3 "$HERE/check.py" "$WORK/results.bin" "$HERE/expected.json"

echo "[fpu-test] PASS — all opmodes correct"
