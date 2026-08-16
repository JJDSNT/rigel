#!/usr/bin/env bash
#
# Run Copperline's cross-emulator timing test and compare the result.
#
# The test disables interrupts and DMA, times a battery of operations with
# CIA-A timer A, and streams 27 hex values out the serial port. Because the
# E-clock is the only clock involved, any emulator that models the CPU-cycle
# to E-clock ratio correctly reports the same numbers — so a row that
# disagrees names exactly which operation is timed wrong.
#
# This is a measurement, not a pass/fail on correctness: Rigel is a long way
# off on several rows and that is the point of having it. The gate is against
# tools/tests/timing/baseline.json, so a change that moves a number has to be
# deliberate.
#
#   tools/tests/timing/run.sh                 compare against the baseline
#   tools/tests/timing/run.sh --update        record the current numbers
#
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
HARNESS="$ROOT/build-harness/rigel-harness"
ADF="$ROOT/external/copperline/timing-test/timing-test.adf"
ROM="$ROOT/media/roms/KS20.rom"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

if [ ! -x "$HARNESS" ]; then
    echo "[timing] harness not built; run ./run.sh build" >&2
    exit 1
fi
TT="$ROOT/external/copperline/timing-test"

if [ ! -f "$TT/test.bin" ]; then
    echo "[timing] external/copperline is empty. Run:" >&2
    echo "    git submodule update --init external/copperline" >&2
    exit 1
fi

# The ADF is a build product and Copperline gitignores it, but the assembled
# boot block and test binary are committed, so it takes no assembler to make.
if [ ! -f "$ADF" ]; then
    echo "[timing] building the ADF..."
    python3 "$TT/make_adf.py" "$TT/boot.bin" "$TT/test.bin" "$ADF"
fi
if [ ! -f "$ROM" ]; then
    echo "[timing] needs a Kickstart 2.x at $ROM (not redistributable)" >&2
    exit 1
fi

# The machine the reference was measured on; see reference.json.
echo "[timing] running..."
"$HARNESS" "$ROM" --adf "$ADF" \
    --headless --frames 1500 \
    --cpu 68ec020 --chip 2048 --slow 512 --ecs \
    > "$WORK/run.log" 2>&1

if [ "${1:-}" = "--update" ]; then
    python3 "$HERE/compare.py" "$WORK/run.log" "$HERE/reference.json" \
        --write-baseline "$HERE/baseline.json"
else
    python3 "$HERE/compare.py" "$WORK/run.log" "$HERE/reference.json" \
        --baseline "$HERE/baseline.json"
fi
