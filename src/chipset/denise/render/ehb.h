#ifndef RIGEL_DENISE_EHB_H
#define RIGEL_DENISE_EHB_H

#include <stdint.h>
#include "rigel/rigel_types.h"

/* Extra-Half-Brite (EHB) mode — 6-plane, 64 colors.
 *
 * Color indices 0–31 use the normal palette.
 * Color indices 32–63 are palette[index - 32] at half brightness (each RGB
 * channel right-shifted by 1). Bit 5 of `plane_bits` selects the EHB set.
 *
 * EHB is only active when: 6 planes, HAM=0, DUALFW=0, EHB not killed
 * (BPLCON2 KILLEHB=0 on ECS). */

/* Resolve a 6-bit EHB color index to an RGB24 value.
 * `palette` must contain 32 entries (base colors only; EHB entries are derived). */
rigel_u32 ehb_resolve_color(uint8_t plane_bits, const rigel_u32 palette[32]);

#endif
