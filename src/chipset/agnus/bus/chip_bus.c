#include "chip_bus.h"

rigel_u16 chip_bus_read16(rigel_chip_ram_if_t mem, rigel_u32 addr)
{
    /* TODO(bus): apply address mask based on configured chip revision */
    addr &= CHIP_BUS_ECS_MASK & ~1u;
    return mem.read16(mem.opaque, addr);
}

void chip_bus_write16(rigel_chip_ram_if_t mem, rigel_u32 addr, rigel_u16 val)
{
    addr &= CHIP_BUS_ECS_MASK & ~1u;
    mem.write16(mem.opaque, addr, val);
}
