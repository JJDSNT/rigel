#ifndef RIGEL_AGNUS_DMA_SPRITE_H
#define RIGEL_AGNUS_DMA_SPRITE_H

#include "rigel/rigel_types.h"

/* Sprite DMA — Agnus fetches sprite control words and pixel data.
 * Denise receives the fetched data and handles composition and collision.
 * Agnus owns: SPRxPT pointers, slot timing, DMA fetch.
 * Denise owns: SPRxPOS/CTL/DATA/DATB interpretation, pixel output. */

#define SPRITE_DMA_CHANNELS 8

typedef struct sprite_dma_channel {
    rigel_u32 ptr;      /* SPRxPTH/SPRxPTL — current fetch pointer */
    bool      armed;    /* set after control words fetched          */
} sprite_dma_channel_t;

typedef struct sprite_dma_state {
    sprite_dma_channel_t sp[SPRITE_DMA_CHANNELS];
} sprite_dma_state_t;

void sprite_dma_reset(sprite_dma_state_t *s);
void sprite_dma_set_ptr_hi(sprite_dma_state_t *s, unsigned sp, rigel_u16 val);
void sprite_dma_set_ptr_lo(sprite_dma_state_t *s, unsigned sp, rigel_u16 val);

/* Called per slot pair during hblank; fetches control or pixel words.
 * Returns the two fetched words via out_w0/out_w1 (for forwarding to Denise). */
void sprite_dma_step(sprite_dma_state_t *s, unsigned sp,
                     rigel_chip_ram_if_t mem,
                     rigel_u16 *out_w0, rigel_u16 *out_w1);

#endif
