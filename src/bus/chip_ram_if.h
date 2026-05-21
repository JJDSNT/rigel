#ifndef CHIP_RAM_IF_H
#define CHIP_RAM_IF_H

#include "rigel/rigel_types.h"

rigel_u16 chip_ram_if_read16(rigel_u32 addr);
void chip_ram_if_write16(rigel_u32 addr, rigel_u16 value);

#endif
