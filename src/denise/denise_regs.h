#ifndef DENISE_REGS_H
#define DENISE_REGS_H

#include "riegel/riegel_types.h"

riegel_u16 denise_read_reg(RiegelContext *ctx, riegel_u32 addr);
void denise_write_reg(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value);

#endif
