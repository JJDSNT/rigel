#ifndef DMA_H
#define DMA_H

#include <stdbool.h>
#include "rigel/rigel_types.h"

typedef struct dma_state {
    rigel_u16 dmacon;
    bool enabled;
    bool blitter_enabled;
} dma_state_t;

void dma_set_enabled(dma_state_t *dma, bool enabled);
void dma_set_dmacon(dma_state_t *dma, rigel_u16 dmacon);

#endif
