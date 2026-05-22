#ifndef RIGEL_DENISE_COLOR_REGS_H
#define RIGEL_DENISE_COLOR_REGS_H

#include "denise/denise_state.h"

rigel_u16 rigel_denise_color_regs_read(const RigelDenise *denise, rigel_u32 addr);
void rigel_denise_color_regs_write(RigelDenise *denise, rigel_u32 addr, rigel_u16 value);
rigel_u32 rigel_denise_color_expand_12bit(rigel_u16 value);

#endif
