#ifndef RIGEL_MMIO_H
#define RIGEL_MMIO_H

#include "rigel_types.h"

rigel_u16 rigel_custom_read16(RigelContext *ctx, rigel_u32 addr);
void rigel_custom_write16(RigelContext *ctx, rigel_u32 addr, rigel_u16 value);

#endif
