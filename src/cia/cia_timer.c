#include "cia/cia_timer.h"

#include <stddef.h>

void cia_timer_reload(cia_timer_state_t *timer)
{
    if (timer == NULL) {
        return;
    }

    timer->counter = timer->latch;
}
