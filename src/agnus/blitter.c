#include "agnus/blitter.h"

#include <stddef.h>

void blitter_start(blitter_state_t *blitter)
{
    if (blitter == NULL) {
        return;
    }

    blitter->busy = 1;
}
