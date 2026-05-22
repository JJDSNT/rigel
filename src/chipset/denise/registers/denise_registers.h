#ifndef RIGEL_DENISE_REGISTERS_H
#define RIGEL_DENISE_REGISTERS_H

#include "denise/denise_state.h"
#include "rigel/rigel_types.h"

bool rigel_denise_registers_owns_reg(rigel_u32 addr);
rigel_u16 rigel_denise_registers_read(RigelDenise *denise, rigel_u32 addr);
void rigel_denise_registers_write(RigelDenise *denise, rigel_u32 addr, rigel_u16 value);

#endif
