#include "refresh_dma.h"

/* OCS chip RAM refresh — Agnus inserts one refresh slot roughly every 10 CCKs.
 * The slot scheduler already places these at fixed hpos positions; this module
 * only accounts for the consumed cycles (useful for diagnostics). */

void refresh_dma_reset(refresh_dma_state_t *r)
{
    r->pending_cycles = 0;
}

void refresh_dma_step(refresh_dma_state_t *r, rigel_u32 cycles)
{
    r->pending_cycles += cycles;
}
