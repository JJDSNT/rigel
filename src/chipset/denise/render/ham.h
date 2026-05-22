#ifndef RIGEL_DENISE_HAM_H
#define RIGEL_DENISE_HAM_H

#include <stdint.h>
#include "rigel/rigel_types.h"

/* Hold-and-Modify (HAM) mode — OCS HAM6, ECS/AGA HAM8.
 *
 * In HAM6 (6 planes, BPLCON0 HAM=1):
 *   Bits [5:4] = control field:
 *     00 → load color from palette[bits[3:0]]
 *     01 → hold previous pixel, modify blue  nibble to bits[3:0]
 *     10 → hold previous pixel, modify red   nibble to bits[3:0]
 *     11 → hold previous pixel, modify green nibble to bits[3:0]
 *
 * HAM8 works analogously with 8 bits, modifying byte channels.
 *
 * HAM is stateful per scanline: `prev_rgb` carries over from pixel to pixel.
 * It resets at each scanline start. */

/* Apply HAM6 decode for one pixel.
 * `plane_bits` is the 6-bit value from the 6 active planes.
 * `prev_rgb`   is the previous pixel's RGB (bits [23:0], no alpha).
 * `palette`    is the 32-entry palette (RGB24, no alpha).
 * Returns the decoded RGB24 for this pixel. */
rigel_u32 ham6_decode_pixel(uint8_t plane_bits,
                            rigel_u32 prev_rgb,
                            const rigel_u32 palette[32]);

/* HAM8 variant (8 planes; reserved for ECS/AGA expansion). */
rigel_u32 ham8_decode_pixel(uint8_t plane_bits,
                            rigel_u32 prev_rgb,
                            const rigel_u32 palette[256]);

#endif
