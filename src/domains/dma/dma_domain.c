#include "domains/dma/dma_domain.h"

#include <stddef.h>

static rigel_u16 rigel_dma_apply_setclr(rigel_u16 current, rigel_u16 value)
{
    rigel_u16 mask = (rigel_u16)(value & 0x7fffU);

    if ((value & RIGEL_SETCLR) != 0) {
        return (rigel_u16)(current | mask);
    }

    return (rigel_u16)(current & (rigel_u16)(~mask));
}

void rigel_dma_domain_reset(dma_state_t *dma)
{
    if (dma == NULL) {
        return;
    }

    dma->dmacon = 0;
    dma->enabled = false;
    dma->blitter_enabled = false;
}

rigel_u16 rigel_dma_domain_read_dmacon(const dma_state_t *dma)
{
    if (dma == NULL) {
        return 0;
    }

    return dma->dmacon;
}

void rigel_dma_domain_write_dmacon(dma_state_t *dma, rigel_u16 value)
{
    if (dma == NULL) {
        return;
    }

    dma_set_dmacon(dma, rigel_dma_apply_setclr(dma->dmacon, value));
    rigel_dma_domain_sync_dmacon(dma, dma->dmacon);
}

void rigel_dma_domain_sync_dmacon(dma_state_t *dma, rigel_u16 dmacon)
{
    if (dma == NULL) {
        return;
    }

    dma_set_dmacon(dma, dmacon);
    dma_set_enabled(dma, (dmacon & RIGEL_DMACON_DMAEN) != 0);
    dma->blitter_enabled = (dmacon & RIGEL_DMACON_BLTEN) != 0;
}

rigel_u32 rigel_dma_domain_blitter_grants(const dma_state_t *dma, rigel_u32 cycles)
{
    if (dma == NULL || !dma->enabled || !dma->blitter_enabled) {
        return 0;
    }

    return cycles;
}
