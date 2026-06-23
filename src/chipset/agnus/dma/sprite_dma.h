#ifndef RIGEL_AGNUS_DMA_SPRITE_H
#define RIGEL_AGNUS_DMA_SPRITE_H

#include "rigel/rigel_types.h"
#include "rigel/rigel_config.h"

/* Sprite DMA — Agnus fetches sprite control words and pixel data.
 * Denise receives the fetched data and handles composition and collision.
 * Agnus owns: SPRxPT pointers, slot timing, DMA fetch.
 * Denise owns: SPRxPOS/CTL/DATA/DATB interpretation, pixel output. */

#define SPRITE_DMA_CHANNELS 8

typedef struct sprite_dma_channel {
    rigel_u32 base_ptr;     /* SPRxPTH/SPRxPTL — frame reload pointer    */
    rigel_u32 ptr;          /* SPRxPTH/SPRxPTL — current fetch pointer   */
    bool      armed;        /* control words fetched; vstart/vstop valid  */
    bool      terminated;   /* zero control pair seen; idle until reload  */
    bool      fetch_ctrl;   /* A-slot decided: fetching ctrl (not data)   */
    rigel_u16 vstart;       /* cached from last SPRxPOS fetch             */
    rigel_u16 vstop;        /* cached from last SPRxCTL fetch             */
    rigel_u16 w0;           /* word latched in A-slot, consumed in B-slot */
} sprite_dma_channel_t;

typedef struct sprite_dma_state {
    sprite_dma_channel_t sp[SPRITE_DMA_CHANNELS];
} sprite_dma_state_t;

void sprite_dma_reset(sprite_dma_state_t *s);
void sprite_dma_set_ptr_hi(sprite_dma_state_t *s, unsigned sp, rigel_u16 val);
void sprite_dma_set_ptr_lo(sprite_dma_state_t *s, unsigned sp, rigel_u16 val);
void sprite_dma_frame_start(sprite_dma_state_t *s);

/* Process one DMA slot (A or B) for sprite channel `sp`.
 * is_b: false = A-slot (latch first word), true = B-slot (latch + deliver).
 * vpos: current scanline, used to decide control vs data fetch.
 * On B-slot returns true and fills *out_w0, *out_w1, *out_is_ctrl.
 * On A-slot returns false (*out_* not written). */
bool sprite_dma_slot(sprite_dma_state_t *s, unsigned sp,
                     rigel_u16 vpos, bool is_b,
                     rigel_chip_ram_if_t mem,
                     rigel_u16 *out_w0, rigel_u16 *out_w1,
                     bool *out_is_ctrl);

#endif
