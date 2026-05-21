#ifndef DENISE_REGS_H
#define DENISE_REGS_H

#include "rigel/rigel_types.h"

rigel_u16 denise_read_reg(RigelContext *ctx, rigel_u32 addr);
void denise_write_reg(RigelContext *ctx, rigel_u32 addr, rigel_u16 value);

#endif
