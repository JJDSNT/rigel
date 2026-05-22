#ifndef RIGEL_DENISE_COLLISIONS_H
#define RIGEL_DENISE_COLLISIONS_H

#include <stdint.h>
#include "rigel/rigel_types.h"

/* Sprite/playfield collision detection — CLXCON / CLXDAT.
 *
 * Hardware detects collisions per pixel clock and accumulates them in CLXDAT.
 * CLXDAT is a read-only sticky register; once a bit is set it stays set until
 * the host reads it (read clears).
 *
 * CLXCON controls which sprite/playfield pairs participate:
 *   Bits [13:12] — MVBP (playfield bits that must match for collision enable)
 *   Bits [11:6]  — enable playfield collision per sprite channel
 *   Bits [5:0]   — enable sprite–sprite collision per pair
 *
 * CLXDAT result bits:
 *   Bit  0  — odd  sprites collided with each other
 *   Bit  1  — even sprites collided with each other
 *   Bit  2  — odd  sprites with PF1
 *   Bit  3  — even sprites with PF1
 *   Bit  4  — odd  sprites with PF2
 *   Bit  5  — even sprites with PF2
 *   Bit  6  — PF1 collided with PF2 */

typedef struct collision_state {
    rigel_u16 clxcon;
    rigel_u16 clxdat;  /* sticky — accumulates until read */
} collision_state_t;

void collision_reset(collision_state_t *c);

/* Called each pixel clock with the current pixel's active layers.
 * `sprite_mask` — bitmask of sprites with a non-transparent pixel here
 * `pf1_active`  — playfield 1 has a non-transparent pixel
 * `pf2_active`  — playfield 2 has a non-transparent pixel */
void collision_check_pixel(collision_state_t *c,
                           uint8_t sprite_mask,
                           bool pf1_active, bool pf2_active);

/* Read CLXDAT and clear it (CLXDAT resets on read). */
rigel_u16 collision_read_clxdat(collision_state_t *c);

void collision_write_clxcon(collision_state_t *c, rigel_u16 val);

#endif
