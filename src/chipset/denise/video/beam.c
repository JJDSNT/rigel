#include "beam.h"

void denise_beam_reset(denise_beam_t *b)
{
    b->hpos      = 0;
    b->vpos      = 0;
    b->in_window = false;
}

void denise_beam_sync(denise_beam_t *b, rigel_u16 hpos, rigel_u16 vpos)
{
    b->hpos = hpos;
    b->vpos = vpos;
}

void denise_beam_advance(denise_beam_t *b)
{
    b->hpos++;
}
