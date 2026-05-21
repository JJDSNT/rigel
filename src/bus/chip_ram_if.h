#ifndef CHIP_RAM_IF_H
#define CHIP_RAM_IF_H

#include "riegel/riegel_types.h"

riegel_u16 chip_ram_if_read16(riegel_u32 addr);
void chip_ram_if_write16(riegel_u32 addr, riegel_u16 value);

#endif
