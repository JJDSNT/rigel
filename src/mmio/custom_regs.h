#ifndef CUSTOM_REGS_H
#define CUSTOM_REGS_H

#include "rigel/rigel_custom.h"
#include "rigel/rigel_types.h"

rigel_u16 custom_regs_read16(RigelContext *ctx, rigel_u32 addr);
void custom_regs_write16(RigelContext *ctx, rigel_u32 addr, rigel_u16 value);

#endif
