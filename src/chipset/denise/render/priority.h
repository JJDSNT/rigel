#ifndef RIGEL_DENISE_PRIORITY_H
#define RIGEL_DENISE_PRIORITY_H

#include <stdint.h>
#include <stdbool.h>

/* Sprite/playfield priority resolution (BPLCON2).
 *
 * BPLCON2 layout:
 *   Bits [2:0]  PF1P — playfield 1 priority vs sprites (0 = below all, 4 = above sp0-5)
 *   Bits [5:3]  PF2P — playfield 2 priority vs sprites
 *   Bit  6      KILLEHB — disable EHB (ECS)
 *   Bit  9      RDRAM — (ECS)
 *
 * Sprite priority ordering (fixed): SP0 > SP1 > SP2 > SP3 > SP4 > SP5 > SP6 > SP7
 * Playfield priority is set relative to that ordering via PF1P/PF2P.
 *
 * Result: the color index of the winning layer, or 0 (transparent/background). */

typedef struct priority_result {
    uint8_t color_index;  /* final resolved color index (0 = background) */
    bool    from_sprite;  /* true if a sprite won */
    uint8_t sprite_num;   /* winning sprite index (valid if from_sprite)  */
} priority_result_t;

priority_result_t denise_priority_resolve(
    uint8_t pf_color,          /* playfield resolved color index (0 = transparent) */
    bool    is_pf2,            /* true if color came from PF2 (dual playfield)     */
    const uint8_t sprite_px[8],/* sprite pixels (0 = transparent)                 */
    uint16_t bplcon2
);

#endif
