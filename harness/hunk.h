#ifndef RIGEL_HARNESS_HUNK_H
#define RIGEL_HARNESS_HUNK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * AmigaOS hunk loader — the LoadSeg half of it.
 *
 * Emu68's bare-metal test programs (the Buddhabrot renderer, the SysInfo
 * benchmark) are ordinary AmigaOS executables that happen to open no
 * libraries: they talk to the hardware directly and print through Paula's
 * UART. Running them needs no operating system, only what LoadSeg does —
 * place each hunk in memory, zero the BSS, apply relocations, and jump to the
 * first one.
 *
 * That makes them useful to the harness as an alternative to a ROM: a real
 * m68k workload with no Kickstart involved.
 *
 * Ported from Emu68's src/HunkLoader.c; the segment layout and the relocation
 * walk follow it, so a program that runs there runs here.
 */

typedef struct hunk_image {
    uint32_t entry;       /* address to start executing at            */
    uint32_t base;        /* first byte the loader used               */
    uint32_t end;         /* one past the last byte used              */
    uint32_t hunk_count;
} hunk_image_t;

/*
 * Lay a hunk file out in the memory `write` addresses, starting at `load_addr`
 * and not passing `limit`.
 *
 * `write`/`read` access guest memory a byte at a time; the harness supplies
 * them so the loader does not need to know the memory map.
 *
 * Returns false and leaves a message in `err` (size `err_len`) if the file is
 * malformed or does not fit.
 */
bool hunk_load(const uint8_t *file, uint32_t file_size,
               uint32_t load_addr, uint32_t limit,
               void *ctx,
               void (*write8)(void *ctx, uint32_t addr, uint8_t value),
               uint8_t (*read8)(void *ctx, uint32_t addr),
               hunk_image_t *out,
               char *err, size_t err_len);

#endif
