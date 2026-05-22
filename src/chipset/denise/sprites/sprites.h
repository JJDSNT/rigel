#ifndef RIGEL_DENISE_SPRITES_H
#define RIGEL_DENISE_SPRITES_H

#include <stdint.h>
#include <stdbool.h>
#include "rigel/rigel_types.h"

/* Full sprite state — Denise's view of all 8 hardware sprites.
 *
 * Agnus fetches sprite words (SPRxDATA/DATB + SPRxPOS/CTL) and forwards them
 * here. Denise latches, interprets, and renders them during active display.
 *
 * Attached sprites: pairs (0,1), (2,3), (4,5), (6,7) may be attached to form
 * a 15+1-color sprite at the cost of losing one sprite slot per pair. */

#define DENISE_SPRITE_COUNT 8

typedef struct denise_sprite {
    rigel_u16 pos;    /* SPRxPOS — VSTART[7:0] | HSTART[8:1]              */
    rigel_u16 ctl;    /* SPRxCTL — VSTOP[7:0]  | HSTART[0] | VSTART[8]    */
    rigel_u16 data;   /* SPRxDATA — pixel data, plane A                    */
    rigel_u16 datb;   /* SPRxDATB — pixel data, plane B (2bpp from A+B)    */

    bool armed;       /* control words fetched; sprite may render this line */
    bool visible;     /* beam currently within vertical range               */
} denise_sprite_t;

typedef struct denise_sprites_state {
    denise_sprite_t sp[DENISE_SPRITE_COUNT];
    rigel_u16 active_mask;    /* bitmask: which sprites are armed this line  */
    rigel_u16 attached_mask;  /* bitmask: which odd sprites are attached     */
} denise_sprites_state_t;

void denise_sprites_reset(denise_sprites_state_t *s);

/* Called by Agnus DMA delivery — latch control or pixel words */
void denise_sprite_receive_ctrl(denise_sprites_state_t *s, unsigned sp,
                                rigel_u16 pos, rigel_u16 ctl);
void denise_sprite_receive_data(denise_sprites_state_t *s, unsigned sp,
                                rigel_u16 data, rigel_u16 datb);

/* Shift out the pixel value for `sp` at the current hpos.
 * Returns the 2-bit (normal) or 4-bit (attached) color offset, 0 = transparent. */
uint8_t denise_sprite_pixel(const denise_sprites_state_t *s, unsigned sp, rigel_u16 hpos);

/* Check if a sprite pair is attached (odd sprite index must be provided) */
bool denise_sprite_is_attached(const denise_sprites_state_t *s, unsigned odd_sp);

/* Compute horizontal start position from SPRxPOS/CTL */
rigel_u16 denise_sprite_hstart(const denise_sprite_t *sp);

/* Compute vertical start/stop from SPRxPOS/CTL */
rigel_u16 denise_sprite_vstart(const denise_sprite_t *sp);
rigel_u16 denise_sprite_vstop(const denise_sprite_t *sp);

#endif
