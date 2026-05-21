#include "cia/cia_ports.h"

#include <stddef.h>

void cia_ports_reset(cia_ports_state_t *ports)
{
    if (ports == NULL) {
        return;
    }

    ports->pra = 0;
    ports->prb = 0;
}
