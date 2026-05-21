#include "bus/rigel_bus.h"

#include <stddef.h>

void rigel_bus_touch(rigel_bus_t *bus, rigel_u32 addr)
{
    if (bus == NULL) {
        return;
    }

    bus->last_addr = addr;
}
