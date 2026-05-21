#include "agnus/copper.h"

#include <stddef.h>

void copper_reset(copper_state_t *copper)
{
    if (copper == NULL) {
        return;
    }

    copper->program_counter = 0;
}
