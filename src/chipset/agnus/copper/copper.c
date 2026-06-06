#include "agnus/copper/copper.h"

#include <stddef.h>

void copper_reset(copper_state_t *copper)
{
    if (copper == NULL) {
        return;
    }

    copper->cop1lc = 0;
    copper->cop2lc = 0;
    copper->program_counter = 0;
    copper->wait_hpos   = 0;
    copper->wait_vpos   = 0;
    copper->wait_vpmask = 0xFFu;
    copper->wait_hpmask = 0xFEu;
    copper->copcon      = 0;
    copper->waiting       = false;
    copper->wait_blitter  = false;
    copper->stopped_until_vbl = false;
    copper->enabled       = false;
    copper->triggered     = false;
    copper->event_latched = false;
    copper->fetch_pending = false;
}

void copper_vbl_reload(copper_state_t *copper)
{
    if (copper == NULL) {
        return;
    }

    if ((copper->cop1lc & 0x001ffffeu) == 0u) {
        copper->program_counter = 0u;
        copper->waiting         = false;
        copper->wait_blitter    = false;
        copper->stopped_until_vbl = false;
        copper->fetch_pending   = false;
        copper->triggered       = false;
        copper->event_latched   = false;
        return;
    }

    copper->program_counter = copper->cop1lc & 0x001ffffeu;
    copper->waiting         = false;
    copper->wait_blitter    = false;
    copper->stopped_until_vbl = false;
    copper->fetch_pending   = true;
    copper->triggered       = false;
    copper->event_latched   = false;
}

void copper_set_pointer_hi(rigel_u32 *ptr, rigel_u16 value)
{
    if (ptr == NULL) {
        return;
    }

    *ptr = (*ptr & 0x0000ffffu) | ((rigel_u32)value << 16);
}

void copper_set_pointer_lo(rigel_u32 *ptr, rigel_u16 value)
{
    if (ptr == NULL) {
        return;
    }

    *ptr = (*ptr & 0xffff0000u) | (rigel_u32)value;
}
