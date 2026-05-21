#ifndef RIGEL_BUS_H
#define RIGEL_BUS_H

#include "rigel/rigel_types.h"

typedef struct rigel_bus {
    rigel_u32 last_addr;
} rigel_bus_t;

void rigel_bus_touch(rigel_bus_t *bus, rigel_u32 addr);

#endif
