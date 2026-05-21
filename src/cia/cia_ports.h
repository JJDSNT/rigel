#ifndef CIA_PORTS_H
#define CIA_PORTS_H

#include "rigel/rigel_types.h"

typedef struct cia_ports_state {
    rigel_u8 pra;
    rigel_u8 prb;
} cia_ports_state_t;

void cia_ports_reset(cia_ports_state_t *ports);

#endif
