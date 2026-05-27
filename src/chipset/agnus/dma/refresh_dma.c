#include "refresh_dma.h"

/* OCS chip RAM refresh slots consume bus time but produce no software-visible
 * data. The scheduler asks this domain which hpos values are refresh slots. */

void refresh_dma_reset(refresh_dma_state_t *r)
{
    if (r == 0) {
        return;
    }

    r->pending_cycles = 0;
    r->slot_count = 3;
    r->slot_hpos[0] = 0x01;
    r->slot_hpos[1] = 0x03;
    r->slot_hpos[2] = 0x05;
}

void refresh_dma_step(refresh_dma_state_t *r, rigel_u32 cycles)
{
    if (r == 0) {
        return;
    }

    r->pending_cycles += cycles;
}

bool refresh_dma_owns_slot(const refresh_dma_state_t *r, rigel_u16 hpos)
{
    rigel_u8 i;

    if (r == 0) {
        return hpos == 0x01u || hpos == 0x03u || hpos == 0x05u;
    }

    for (i = 0; i < r->slot_count; ++i) {
        if (r->slot_hpos[i] == hpos) {
            return true;
        }
    }

    return false;
}
