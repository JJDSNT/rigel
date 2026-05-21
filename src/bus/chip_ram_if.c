#include "bus/chip_ram_if.h"

rigel_u16 chip_ram_if_read16(rigel_u32 addr)
{
    (void)addr;
    return 0;
}

void chip_ram_if_write16(rigel_u32 addr, rigel_u16 value)
{
    (void)addr;
    (void)value;
}
