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
    copper->waiting     = false;
    copper->enabled = false;
    copper->triggered = false;
    copper->fetch_pending = false;
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
