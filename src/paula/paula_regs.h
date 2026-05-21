#ifndef PAULA_REGS_H
#define PAULA_REGS_H

#include "rigel/rigel_types.h"

bool paula_owns_reg(rigel_u32 addr);
rigel_u16 paula_read_reg(RigelContext *ctx, rigel_u32 addr);
void paula_write_reg(RigelContext *ctx, rigel_u32 addr, rigel_u16 value);

#endif
