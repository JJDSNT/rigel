#ifndef PAULA_REGS_H
#define PAULA_REGS_H

#include "riegel/riegel_types.h"

bool paula_owns_reg(riegel_u32 addr);
riegel_u16 paula_read_reg(RiegelContext *ctx, riegel_u32 addr);
void paula_write_reg(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value);

#endif
