#include "bus/chip_ram_if.h"

riegel_u16 chip_ram_if_read16(riegel_u32 addr)
{
    (void)addr;
    return 0;
}

void chip_ram_if_write16(riegel_u32 addr, riegel_u16 value)
{
    (void)addr;
    (void)value;
}
