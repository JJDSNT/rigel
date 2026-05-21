#ifndef CIA_TIMER_H
#define CIA_TIMER_H

#include "rigel/rigel_types.h"

typedef struct cia_timer_state {
    rigel_u16 latch;
    rigel_u16 counter;
} cia_timer_state_t;

void cia_timer_reload(cia_timer_state_t *timer);

#endif
