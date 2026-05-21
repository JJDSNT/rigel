#include "agnus/bitplanes.h"

#include <stddef.h>

void bitplanes_set_depth(bitplanes_state_t *state, unsigned depth)
{
    if (state == NULL) {
        return;
    }

    state->depth = depth;
}
