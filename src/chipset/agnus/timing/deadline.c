#include "deadline.h"

void agnus_deadlines_reset(agnus_deadlines_t *d)
{
    d->blitter      = AGNUS_DEADLINE_UNKNOWN;
    d->beam_line_end = AGNUS_DEADLINE_UNKNOWN;
    d->vertb        = AGNUS_DEADLINE_UNKNOWN;
    d->copper_wait  = AGNUS_DEADLINE_UNKNOWN;
    d->audio        = AGNUS_DEADLINE_UNKNOWN;
    d->disk         = AGNUS_DEADLINE_UNKNOWN;
    d->slot         = AGNUS_DEADLINE_UNKNOWN;
}

rigel_u32 agnus_deadlines_min(const agnus_deadlines_t *d)
{
    rigel_u32 min = AGNUS_DEADLINE_UNKNOWN;

    if (d->blitter      < min) min = d->blitter;
    if (d->beam_line_end < min) min = d->beam_line_end;
    if (d->vertb        < min) min = d->vertb;
    if (d->copper_wait  < min) min = d->copper_wait;
    if (d->audio        < min) min = d->audio;
    if (d->disk         < min) min = d->disk;
    if (d->slot         < min) min = d->slot;

    return min;
}
