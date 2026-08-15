#!/usr/bin/env bash
#
# Build the ODFileSystem handler (m68k) with the cross toolchain.
#
# ODFS is the ISO-9660 filesystem handler. The LIDE board presents an ISO as an
# ATAPI CD-ROM, but AmigaOS still needs a handler to mount it — that handler is
# this binary, served from the board's second ROM bank at offset 0x10000.
#
# Produces external/ODFileSystem/build/amiga/ODFileSystem.
#
#   scripts/build-odfs.sh [output_path]
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ODFS_DIR="$ROOT/external/ODFileSystem"
M68K="$ROOT/scripts/m68k"
COMPAT_HDR="$ROOT/scripts/odfs_crosstools_compat.h"
OUTPUT="${1:-$ODFS_DIR/build/amiga/ODFileSystem}"

if [ ! -f "$ODFS_DIR/Makefile" ]; then
    echo "error: external/ODFileSystem is empty. Run:" >&2
    echo "    git submodule update --init external/ODFileSystem" >&2
    exit 1
fi

if [ "$("$M68K" --check | cut -d: -f1)" != "docker" ]; then
    echo "error: building ODFS needs the Amiga cross toolchain." >&2
    echo "    docker pull amigadev/crosstools:m68k-amigaos" >&2
    exit 1
fi

# ISO-9660 with Rock Ridge and Joliet is all an Amiga CD needs. UDF, HFS and
# CDDA stay off: they roughly double the binary, and the board ROM bank has
# 64 KB of byte-wide space to fit it in.
FEATURES="\
 FEATURE_ISO9660=1 \
 FEATURE_ROCK_RIDGE=1 \
 FEATURE_JOLIET=1 \
 FEATURE_MULTISESSION=1 \
 FEATURE_UDF=0 \
 FEATURE_HFS=0 \
 FEATURE_HFSPLUS=0 \
 FEATURE_CDDA=0"

# -Werror is dropped because the crosstools NDK headers have minor
# incompatibilities with what ODFS expects, and the compat header patches
# UtilityBase and TD_READ64 back in.
CFLAGS="-Os -m68000 -mtune=68020-60 -msoft-float -noixemul -nostartfiles \
 -Wall -Wextra -Wno-error \
 -Wstrict-prototypes -Wmissing-prototypes \
 -Wno-array-bounds -Wno-unused-parameter \
 -DAMIGA \
 -DODFS_AMIGA_DATE=\\\"rigel\\\" \
 -DODFS_GIT_VERSION=\\\"embedded\\\" \
 -DODFS_SERIAL_DEBUG=0 \
 -DODFS_FEATURE_LOG=0 \
 -DODFS_FEATURE_ISO9660=1 \
 -DODFS_FEATURE_ROCK_RIDGE=1 \
 -DODFS_FEATURE_JOLIET=1 \
 -DODFS_FEATURE_MULTISESSION=1 \
 -DODFS_FEATURE_UDF=0 \
 -DODFS_FEATURE_HFS=0 \
 -DODFS_FEATURE_HFSPLUS=0 \
 -DODFS_FEATURE_CDDA=0 \
 -DODFS_FEATURE_CACHE_BLOCK=1 \
 -DODFS_FEATURE_CACHE_META=0 \
 -DODFS_FEATURE_CACHE_STREAM=0 \
 -include $COMPAT_HDR"

echo "[odfs] building..."
# Always from scratch: a stale object built with different feature flags links
# into a binary that looks fine and behaves like a regression.
rm -rf "$ODFS_DIR/build/amiga"
"$M68K" bash -c \
    "make -C '$ODFS_DIR' amiga CFLAGS='$CFLAGS' $FEATURES SERIAL_DEBUG=0 AMIGA_BUILD=build/amiga 2>&1"

BUILT="$ODFS_DIR/build/amiga/ODFileSystem"
if [ ! -f "$BUILT" ]; then
    echo "[odfs] FAILED: $BUILT was not produced" >&2
    exit 1
fi

echo "[odfs] built: $BUILT ($(wc -c < "$BUILT") bytes)"

if [ "$OUTPUT" != "$BUILT" ]; then
    cp "$BUILT" "$OUTPUT"
    echo "[odfs] copied to $OUTPUT"
fi
