#ifndef RIGEL_AGNUS_MMIO_H
#define RIGEL_AGNUS_MMIO_H

#include "rigel/rigel_types.h"

bool rigel_agnus_mmio_owns_reg(rigel_u32 addr);
rigel_u16 rigel_agnus_mmio_read(RigelContext *ctx, rigel_u32 addr);
void rigel_agnus_mmio_write(RigelContext *ctx, rigel_u32 addr, rigel_u16 value);

/* VPOSR bits [14:8]: Agnus chip identification (rev + video standard). */
struct RigelAgnus;
rigel_u16 agnus_vposr_chip_id(const struct RigelAgnus *agnus);

#endif
