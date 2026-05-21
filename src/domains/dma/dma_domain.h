#ifndef RIGEL_DMA_DOMAIN_H
#define RIGEL_DMA_DOMAIN_H

#include "agnus/dma.h"
#include "rigel/rigel_custom.h"
#include "rigel/rigel_types.h"

void rigel_dma_domain_reset(dma_state_t *dma);
rigel_u16 rigel_dma_domain_read_dmacon(const dma_state_t *dma);
void rigel_dma_domain_write_dmacon(dma_state_t *dma, rigel_u16 value);
void rigel_dma_domain_sync_dmacon(dma_state_t *dma, rigel_u16 dmacon);
rigel_u32 rigel_dma_domain_blitter_grants(const dma_state_t *dma, rigel_u32 cycles);

#endif
