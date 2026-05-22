#ifndef RIGEL_DENISE_H
#define RIGEL_DENISE_H

#include "denise/denise_state.h"
#include "rigel/rigel_types.h"

bool rigel_denise_owns_reg(rigel_u32 addr);
rigel_u16 rigel_denise_read_reg(RigelContext *ctx, rigel_u32 addr);
void rigel_denise_write_reg(RigelContext *ctx, rigel_u32 addr, rigel_u16 value);

#endif
