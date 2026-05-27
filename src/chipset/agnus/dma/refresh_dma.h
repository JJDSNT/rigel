#ifndef RIGEL_AGNUS_DMA_REFRESH_H
#define RIGEL_AGNUS_DMA_REFRESH_H

#include <stdbool.h>

#include "rigel/rigel_types.h"

/* Chip RAM refresh — Agnus issues one refresh access every ~10 CCKs.
 * These consume DMA slots but produce no data visible to software. */

typedef struct refresh_dma_state {
    rigel_u32 pending_cycles;
    rigel_u16 slot_hpos[3];
    rigel_u8 slot_count;
} refresh_dma_state_t;

void refresh_dma_reset(refresh_dma_state_t *r);
void refresh_dma_step(refresh_dma_state_t *r, rigel_u32 cycles);
bool refresh_dma_owns_slot(const refresh_dma_state_t *r, rigel_u16 hpos);

#endif
