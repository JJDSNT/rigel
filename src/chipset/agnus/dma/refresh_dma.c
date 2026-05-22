#include "refresh_dma.h"

/* TODO(dma): implement refresh slot accounting */

void refresh_dma_reset(refresh_dma_state_t *r)
{
    r->pending_cycles = 0;
}

void refresh_dma_step(refresh_dma_state_t *r, rigel_u32 cycles)
{
    (void)r;
    (void)cycles;
    /* TODO */
}
