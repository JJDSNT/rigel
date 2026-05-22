#ifndef RIGEL_DENISE_PLANAR_H
#define RIGEL_DENISE_PLANAR_H

#include <stdint.h>
#include "rigel/rigel_types.h"

/* Planar-to-chunky (P2C) conversion — Amiga planar pixel format → pixel indices.
 *
 * The Amiga stores bitplanes as separate word streams. For each 16-pixel group:
 *   plane[0] word → bit 0 of each pixel index
 *   plane[1] word → bit 1 of each pixel index
 *   ...
 *   plane[5] word → bit 5 of each pixel index
 *
 * The result is 16 pixel indices, each 0–63 (6-plane max).
 *
 * This is the hot path for bitplane rendering and should be SIMD-friendly.
 * `num_planes` must be 1–6; planes beyond `num_planes` contribute 0. */

void planar_to_chunky(const rigel_u16 plane_words[6], unsigned num_planes,
                      uint8_t pixels_out[16]);

/* Single-pixel variant — extract pixel index at bit position `bit` (0=MSB, 15=LSB). */
uint8_t planar_pixel_at(const rigel_u16 plane_words[6], unsigned num_planes, unsigned bit);

#endif
