#include "agnus/beam.h"

#include <stddef.h>

void beam_step(beam_state_t *beam, riegel_u16 clocks)
{
    if (beam == NULL) {
        return;
    }

    beam->hpos = (riegel_u16)(beam->hpos + clocks);
}
