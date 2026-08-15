#!/usr/bin/env bash
#
# Build lide.rom from source with the m68k cross toolchain.
#
# lide.device is LIV2's IDE driver. The harness presents it as a Zorro II board
# so AmigaOS can see an HDF as a hard disk and an ISO as a CD-ROM: the ROM
# carries the autoconfig boot loader and the driver binary the guest loads.
#
# Produces external/lide.device/lide.rom, a 32 KB nibble-wide image.
#
#   scripts/build-lide-rom.sh [output_path]
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIDE_DIR="$ROOT/external/lide.device"
M68K="$ROOT/scripts/m68k"
NDK_INCLUDE=/opt/m68k-amigaos/m68k-amigaos/ndk-include
OUTPUT="${1:-$LIDE_DIR/lide.rom}"

if [ ! -f "$LIDE_DIR/Makefile" ]; then
    echo "error: external/lide.device is empty. Run:" >&2
    echo "    git submodule update --init external/lide.device" >&2
    exit 1
fi

# vasm alone is not enough here: step 1 needs the AmigaOS GCC and NDK headers,
# which only the Docker backend has.
if [ "$("$M68K" --check | cut -d: -f1)" != "docker" ]; then
    echo "error: building lide.rom needs the Amiga cross toolchain." >&2
    echo "    docker pull amigadev/crosstools:m68k-amigaos" >&2
    exit 1
fi

echo "[lide-rom] 1/4 building lide.device..."
# amigadev/crosstools' GCC 6.5.0b does not pull execbase.h and expansionbase.h
# in transitively the way LIV2's own amiga-gcc does, so force them in. CFLAGS
# is set inside the container because the wrapper passes no host environment.
"$M68K" bash -c \
    "CFLAGS='-include stdint.h -include exec/execbase.h -include libraries/expansionbase.h' \
     make -C '$LIDE_DIR' lide.device -s"

echo "[lide-rom] 2/4 assembling the boot loader..."
mkdir -p "$LIDE_DIR/bootrom/obj"
# -Fbin trips section-overlap errors on this source; produce hunk output and
# extract the raw bytes instead.
"$M68K" vasmm68k_mot -Fhunk -quiet -align \
    -DROM -DBYTEWIDE \
    -I"$NDK_INCLUDE" \
    -o "$LIDE_DIR/bootrom/obj/bootldr.hunk" \
    "$LIDE_DIR/bootrom/bootldr.S"
python3 "$ROOT/scripts/hunk_to_bin.py" \
    "$LIDE_DIR/bootrom/obj/bootldr.hunk" \
    "$LIDE_DIR/bootrom/obj/bootldr"

echo "[lide-rom] 3/4 nibble-encoding the boot loader..."
( cd "$LIDE_DIR/bootrom" && python3 mungerom.py )

echo "[lide-rom] 4/4 assembling the ROM image..."
python3 "$ROOT/scripts/make_lide_rom.py" "$LIDE_DIR" "$OUTPUT"

echo "[lide-rom] done: $OUTPUT ($(stat -c%s "$OUTPUT") bytes)"
