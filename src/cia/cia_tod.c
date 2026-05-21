#include "cia/cia_tod.h"

#include <stddef.h>

void cia_tod_tick(cia_tod_state_t *tod)
{
    if (tod == NULL) {
        return;
    }

    tod->ticks++;
}
