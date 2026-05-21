#include "bus/riegel_bus.h"

#include <stddef.h>

void riegel_bus_touch(riegel_bus_t *bus, riegel_u32 addr)
{
    if (bus == NULL) {
        return;
    }

    bus->last_addr = addr;
}
