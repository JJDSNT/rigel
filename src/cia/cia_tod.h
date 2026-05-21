#ifndef CIA_TOD_H
#define CIA_TOD_H

typedef struct cia_tod_state {
    unsigned ticks;
} cia_tod_state_t;

void cia_tod_tick(cia_tod_state_t *tod);

#endif
