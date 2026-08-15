#ifndef RIGEL_HARNESS_EXPANSION_SHIM_H
#define RIGEL_HARNESS_EXPANSION_SHIM_H

/*
 * The ATA and ATAPI sources are carried over from Bellatrix essentially
 * unchanged, so they still include "support.h" and call kprintf(). This
 * supplies both without dragging anything else along, which keeps the diff
 * against the original small enough to re-sync if those files ever move.
 */

#include <stdint.h>
#include <stdio.h>

#define kprintf(...) fprintf(stderr, __VA_ARGS__)

/* The carried-over sources gate their env-var trace on a host that has
 * getenv(). Bellatrix spelled that BELLATRIX_HARNESS; the harness always has
 * it, and a bare-metal host would set this to 0. */
#ifndef RIGEL_HARNESS_HAS_ENV
#define RIGEL_HARNESS_HAS_ENV 1
#endif

#define BE32(x) __builtin_bswap32(x)
#define BE16(x) __builtin_bswap16(x)

#endif
