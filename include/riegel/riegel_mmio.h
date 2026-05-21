#ifndef RIEGEL_MMIO_H
#define RIEGEL_MMIO_H

#include "riegel_types.h"

riegel_u16 riegel_custom_read16(RiegelContext *ctx, riegel_u32 addr);
void riegel_custom_write16(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value);

#endif
