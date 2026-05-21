#ifndef AGNUS_REGS_H
#define AGNUS_REGS_H

#include "rigel/rigel_types.h"

bool agnus_owns_reg(rigel_u32 addr);
rigel_u16 agnus_read_reg(RigelContext *ctx, rigel_u32 addr);
void agnus_write_reg(RigelContext *ctx, rigel_u32 addr, rigel_u16 value);

#endif
