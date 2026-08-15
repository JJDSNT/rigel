#!/usr/bin/env bash
#
# End-to-end check of the development loop: assemble an m68k program, boot it
# in the harness, and verify what it did to the machine.
#
# This is the template for a chipset test. Copy it, change smoke.S, and check
# whatever the new program is supposed to prove.
#
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
HARNESS="$ROOT/build-harness/rigel-harness"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

PATTERN="ca5eb0a7"
MARKER=1000          # chip RAM address the program fills, hex

if [ ! -x "$HARNESS" ]; then
    echo "[smoke] harness not built; run ./run.sh build" >&2
    exit 1
fi

"$ROOT/scripts/m68k" --check >/dev/null || exit 1

echo "[smoke] assembling..."
"$ROOT/scripts/m68k" vasmm68k_mot -Fbin -quiet \
    -o "$WORK/smoke.bin" "$HERE/smoke.S"

# harness_load_kickstart takes 256K, 512K or 1 MB, so pad the image out to the
# smallest of those. The ROM sits at 0xFC0000 and mirrors through 0xF80000.
echo "[smoke] padding to 256K..."
python3 - "$WORK/smoke.bin" "$WORK/smoke.rom" <<'PY'
import sys
data = open(sys.argv[1], "rb").read()
size = 256 * 1024
if len(data) > size:
    sys.exit("image is larger than 256K")
open(sys.argv[2], "wb").write(data + b"\x00" * (size - len(data)))
PY

# Find the final spin loop rather than hardcoding its address: a literal
# goes stale the moment an instruction is added above it.
echo "[smoke] locating the spin loop..."
"$HARNESS" "$WORK/smoke.rom" --headless --frames 1 \
    --disasm 00fc0008:32 > "$WORK/dis.txt" 2>&1

if ! BREAK="$(python3 "$HERE/find_spin.py" "$WORK/dis.txt")"; then
    echo "[smoke] FAIL: could not find the spin loop in the disassembly"
    cat "$WORK/dis.txt"
    exit 1
fi
echo "[smoke] spin loop at \$$BREAK"

echo "[smoke] running..."
set +e
"$HARNESS" "$WORK/smoke.rom" \
    --headless --frames 5 \
    --break "$BREAK" \
    --dump "${MARKER}:40:$WORK/marker.bin" \
    --log video \
    > "$WORK/run.log" 2>&1
rc=$?
set -e

# Exit code 4 is "breakpoint hit", which is the success path here: it proves
# the program ran to its final loop rather than dying somewhere earlier.
if [ "$rc" -ne 4 ]; then
    echo "[smoke] FAIL: expected exit 4 (breakpoint), got $rc"
    cat "$WORK/run.log"
    exit 1
fi
echo "[smoke] breakpoint at \$$BREAK reached"

if ! grep -q "COLOR00" "$WORK/run.log"; then
    echo "[smoke] FAIL: no COLOR00 write was traced"
    cat "$WORK/run.log"
    exit 1
fi
echo "[smoke] COLOR00 writes traced"

echo "[smoke] verifying the Chip RAM pattern..."
python3 - "$WORK/marker.bin" "$PATTERN" <<'PY'
import struct, sys
data = open(sys.argv[1], "rb").read()
base = int(sys.argv[2], 16)
got = struct.unpack(">16I", data[:64])
want = tuple((base + i) & 0xFFFFFFFF for i in range(16))
if got != want:
    print("FAIL: chip RAM pattern mismatch")
    print("  want:", " ".join("%08x" % v for v in want[:4]), "...")
    print("  got :", " ".join("%08x" % v for v in got[:4]), "...")
    sys.exit(1)
print("[smoke] 16 longs verified at the marker address")
PY

echo "[smoke] PASS"
