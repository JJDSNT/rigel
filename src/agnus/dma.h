#ifndef DMA_H
#define DMA_H

#include <stdbool.h>

typedef struct dma_state {
    bool enabled;
} dma_state_t;

void dma_set_enabled(dma_state_t *dma, bool enabled);

#endif
