#!/usr/bin/env bash
#
# Boot AROS to its startup screen and check it got there.
#
# This is the one full-system test that can live in CI: AROS and its bootdisk
# are freely redistributable and tracked in media/, so it needs no Kickstart.
#
# It exercises nearly everything at once — CPU, Chip RAM, the 1 MB split ROM
# layout, CIA, floppy DMA and MFM decode, the interrupt path, Copper, bitplane
# DMA and Denise in hires.
#
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
HARNESS="$ROOT/build-harness/rigel-harness"
ROM="$ROOT/media/roms/aros.rom"
ADF="$ROOT/media/disks/aros-bootdisk-amiga-m68k.adf"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

for f in "$HARNESS" "$ROM" "$ADF"; do
    if [ ! -e "$f" ]; then
        echo "[aros] missing $f" >&2
        [ "$f" = "$HARNESS" ] && echo "[aros] run ./run.sh build" >&2
        exit 1
    fi
done

echo "[aros] booting..."
"$HARNESS" "$ROM" --adf "$ADF" \
    --headless --frames 2000 --chip 2048 --fast 8 \
    --screenshot "$WORK/boot.ppm" > "$WORK/run.log" 2>&1

fail() { echo "[aros] FAIL: $1"; tail -30 "$WORK/run.log"; exit 1; }

# Without boot media dosboot.resource waits forever, so exec never finishes
# InitCode. Reaching the end of it is the sharpest single signal that the whole
# startup ran.
grep -q "leave InitCode" "$WORK/run.log" || fail "exec never finished InitCode"
echo "[aros] exec completed InitCode"

grep -q "Software Failure" "$WORK/run.log" && fail "AROS reported a Software Failure"

for want in "graphics.library" "intuition.library" "trackdisk.device" "dosboot.resource"; do
    grep -q "InitResident.*$want" "$WORK/run.log" || fail "$want never initialised"
done
echo "[aros] every expected resident initialised"

# The startup screen is a hires Intuition window, so a boot that renders gets a
# wide frame with real content — a blank or lores one means it did not.
python3 - "$WORK/boot.ppm" <<'PY'
import sys
data = open(sys.argv[1], "rb").read()
parts = data.split(b"\n", 3)
w, h = (int(v) for v in parts[1].split())
px = parts[3]

if w < 640:
    sys.exit(f"FAIL: frame is {w}x{h}; the AROS screen is hires (>= 640 wide)")

colours = {px[i:i+3] for i in range(0, min(len(px), w * h * 3), 3)}
if len(colours) < 3:
    sys.exit(f"FAIL: frame has {len(colours)} distinct colours; nothing was drawn")

print(f"[aros] screen {w}x{h}, {len(colours)} distinct colours")
PY

echo "[aros] PASS"
