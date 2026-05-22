#include "agnus_dump.h"

#include <stdio.h>

void agnus_dump_regs(const RigelAgnus *agnus)
{
    if (!agnus) return;
    agnus_dump_beam(&agnus->beam);
    agnus_dump_dma(&agnus->dma);
    agnus_dump_copper(&agnus->copper);
}

void agnus_dump_dma(const dma_state_t *dma)
{
    if (!dma) return;
    fprintf(stderr, "DMA: DMACON=$%04X enabled=%d blitter=%d\n",
            dma->dmacon, dma->enabled, dma->blitter_enabled);
}

void agnus_dump_copper(const copper_state_t *copper)
{
    if (!copper) return;
    fprintf(stderr,
            "COPPER: pc=$%06X cop1=$%06X cop2=$%06X "
            "wait=%d vp=%d hp=%d enabled=%d\n",
            copper->program_counter, copper->cop1lc, copper->cop2lc,
            copper->waiting, copper->wait_vpos, copper->wait_hpos,
            copper->enabled);
}

void agnus_dump_beam(const beam_state_t *beam)
{
    if (!beam) return;
    fprintf(stderr, "BEAM: hpos=%d vpos=%d frame=%llu\n",
            beam->hpos, beam->vpos, (unsigned long long)beam->frame_count);
}
