#include "domains/copper/copper_domain.h"

#include <stddef.h>

enum {
    RIGEL_DMACON_COPEN = 0x0080u,
    RIGEL_DMACON_DMAEN = 0x0200u
};

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
    copper->triggered = false;
    copper->fetch_pending = true;
    copper->waiting = false;
}

void rigel_copper_domain_jump2(copper_state_t *copper)
{
    if (copper == NULL) {
        return;
    }

    copper->program_counter = copper->cop2lc;
    copper->triggered = false;
    copper->fetch_pending = true;
    copper->waiting = false;
}

void rigel_copper_domain_step(copper_state_t *copper, const beam_state_t *beam, const dma_state_t *dma)
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

    if (copper_beam_cmp(beam->vpos, beam->hpos,
                        copper->wait_vpos, copper->wait_hpos,
                        copper->wait_vpmask, copper->wait_hpmask)) {
        copper->waiting       = false;
        copper->program_counter += 4u;
        copper->triggered     = true;
        copper->fetch_pending = true;
    }
}
