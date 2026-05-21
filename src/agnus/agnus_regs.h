#ifndef AGNUS_REGS_H
#define AGNUS_REGS_H

#include "riegel/riegel_types.h"

bool agnus_owns_reg(riegel_u32 addr);
riegel_u16 agnus_read_reg(RiegelContext *ctx, riegel_u32 addr);
void agnus_write_reg(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value);

#endif
