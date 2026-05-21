#ifndef CIA_PORTS_H
#define CIA_PORTS_H

#include "riegel/riegel_types.h"

typedef struct cia_ports_state {
    riegel_u8 pra;
    riegel_u8 prb;
} cia_ports_state_t;

void cia_ports_reset(cia_ports_state_t *ports);

#endif
