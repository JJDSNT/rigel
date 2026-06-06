#include "domains/copper/copper_domain.h"

#include <stddef.h>

enum {
    RIGEL_DMACON_COPEN = 0x0080u,
    RIGEL_DMACON_DMAEN = 0x0200u
};

/* Sentinel: no deadline contribution */
#define COPPER_DEADLINE_NONE ((rigel_u32)0xFFFFFFFFu)

void rigel_copper_domain_reset(copper_state_t *copper)
{
    copper_reset(copper);
}

void rigel_copper_domain_set_wait(copper_state_t *copper,
                                   rigel_u16 vpos, rigel_u16 hpos,
                                   rigel_u16 vpmask, rigel_u16 hpmask)
{
    if (copper == NULL) {
        return;
    }

    copper->wait_vpos   = vpos;
    copper->wait_hpos   = hpos;
    copper->wait_vpmask = vpmask;
    copper->wait_hpmask = hpmask;
    copper->waiting     = true;
}

void rigel_copper_domain_jump1(copper_state_t *copper)
{
    if (copper == NULL) {
        return;
    }

    copper->program_counter = copper->cop1lc;
    copper->triggered     = false;
    copper->event_latched = false;
    copper->fetch_pending = true;
    copper->waiting       = false;
}

void rigel_copper_domain_jump2(copper_state_t *copper)
{
    if (copper == NULL) {
        return;
    }

    copper->program_counter = copper->cop2lc;
    copper->triggered     = false;
    copper->event_latched = false;
    copper->fetch_pending = true;
    copper->waiting       = false;
}

void rigel_copper_domain_vbl_reload(copper_state_t *copper)
{
    copper_vbl_reload(copper);
}

rigel_u32 rigel_copper_domain_cycles_to_wait(const copper_state_t *copper,
                                              const beam_state_t *beam)
{
    rigel_u32 current, target, frame_total;

    if (copper == NULL || beam == NULL || !copper->waiting)
        return COPPER_DEADLINE_NONE;

    if (copper_beam_cmp(beam->vpos, beam->hpos,
                        copper->wait_vpos, copper->wait_hpos,
                        copper->wait_vpmask, copper->wait_hpmask))
        return 0;

    /* Full-mask fast path: exact CCK count to wait target */
    if (copper->wait_vpmask == 0xFFu && copper->wait_hpmask == 0xFEu) {
        current = (rigel_u32)beam->vpos * beam->line_clocks + beam->hpos;
        target  = (rigel_u32)copper->wait_vpos * beam->line_clocks + copper->wait_hpos;
        if (target > current)
            return target - current;
        /* Target is in the next frame */
        frame_total = (rigel_u32)beam->frame_lines * beam->line_clocks;
        return frame_total - current + target;
    }

    /* Partial mask: conservative — re-check at the next line boundary */
    return (rigel_u32)(beam->line_clocks - beam->hpos);
}

void rigel_copper_domain_step(copper_state_t *copper, const beam_state_t *beam,
                              const dma_state_t *dma, bool blitter_busy)
{
    bool enabled;

    if (copper == NULL || beam == NULL || dma == NULL) {
        return;
    }

    copper->triggered = false;
    enabled = (dma->dmacon & (RIGEL_DMACON_DMAEN | RIGEL_DMACON_COPEN))
        == (RIGEL_DMACON_DMAEN | RIGEL_DMACON_COPEN);
    copper->enabled = enabled;

    if (!enabled || !copper->waiting) {
        return;
    }
    if (copper->wait_blitter && blitter_busy) {
        return;
    }

    if (copper_beam_cmp(beam->vpos, beam->hpos,
                        copper->wait_vpos, copper->wait_hpos,
                        copper->wait_vpmask, copper->wait_hpmask)) {
        copper->waiting         = false;
        copper->wait_blitter    = false;
        copper->program_counter += 4u;
        copper->triggered       = true;
        copper->event_latched   = true;
        copper->fetch_pending   = true;
    }
}
