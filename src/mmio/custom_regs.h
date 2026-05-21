#ifndef CUSTOM_REGS_H
#define CUSTOM_REGS_H

#include "riegel/riegel_custom.h"
#include "riegel/riegel_types.h"

riegel_u16 custom_regs_read16(RiegelContext *ctx, riegel_u32 addr);
void custom_regs_write16(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value);

#endif
