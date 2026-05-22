#ifndef RIGEL_AGNUS_COPPER_REGS_H
#define RIGEL_AGNUS_COPPER_REGS_H

#include "rigel/rigel_types.h"

bool rigel_copper_regs_owns_reg(rigel_u32 addr);
rigel_u16 rigel_copper_regs_read(RigelContext *ctx, rigel_u32 addr);
void rigel_copper_regs_write(RigelContext *ctx, rigel_u32 addr, rigel_u16 value);

#endif
