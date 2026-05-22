#ifndef RIGEL_AGNUS_MMIO_H
#define RIGEL_AGNUS_MMIO_H

#include "rigel/rigel_types.h"

bool rigel_agnus_mmio_owns_reg(rigel_u32 addr);
rigel_u16 rigel_agnus_mmio_read(RigelContext *ctx, rigel_u32 addr);
void rigel_agnus_mmio_write(RigelContext *ctx, rigel_u32 addr, rigel_u16 value);

#endif
