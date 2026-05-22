#ifndef RIGEL_DENISE_RGB_H
#define RIGEL_DENISE_RGB_H

#include <stdint.h>
#include "rigel/rigel_types.h"

/* Color format conversion — Amiga 12-bit RGB → 32-bit RGBA.
 *
 * OCS palette registers encode color as 0x0RGB (4 bits per channel, 12-bit).
 * This module expands to 0xRRGGBBAA (8 bits per channel, alpha = 0xFF).
 *
 * The expansion doubles each nibble: e.g. 0xF → 0xFF, 0x8 → 0x88.
 * This is the standard Amiga color expansion; it matches the hardware analog
 * output levels. ECS can produce 18-bit color (6 bits per channel). */

/* Expand a 12-bit OCS color register value to 32-bit RGBA (alpha = 0xFF). */
static inline rigel_u32 rgb12_to_rgba32(rigel_u16 color12)
{
    uint8_t r = (uint8_t)(((color12 >> 8) & 0xFu) * 0x11u);
    uint8_t g = (uint8_t)(((color12 >> 4) & 0xFu) * 0x11u);
    uint8_t b = (uint8_t)(( color12       & 0xFu) * 0x11u);
    return ((rigel_u32)r << 24) | ((rigel_u32)g << 16) | ((rigel_u32)b << 8) | 0xFFu;
}

/* Expand the full 32-color palette at once. */
void rgb_expand_palette(const rigel_u16 color_regs[32], rigel_u32 rgba_out[32]);

#endif
