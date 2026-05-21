#ifndef RIEGEL_BUS_H
#define RIEGEL_BUS_H

#include "riegel/riegel_types.h"

typedef struct riegel_bus {
    riegel_u32 last_addr;
} riegel_bus_t;

void riegel_bus_touch(riegel_bus_t *bus, riegel_u32 addr);

#endif
