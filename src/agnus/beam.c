#include "agnus/beam.h"

#include <stddef.h>

void beam_step(beam_state_t *beam, rigel_u16 clocks)
{
    if (beam == NULL) {
        return;
    }

    beam->hpos = (rigel_u16)(beam->hpos + clocks);
}
