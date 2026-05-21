#include "denise/sprites.h"

#include <stddef.h>

void sprites_reset(sprites_state_t *sprites)
{
    if (sprites == NULL) {
        return;
    }

    sprites->active_mask = 0;
}
