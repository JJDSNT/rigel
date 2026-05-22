#ifndef PAULA_H
#define PAULA_H

#include <stdint.h>

#include "paula/paula_state.h"
#include "rigel/rigel_types.h"

typedef RigelPaula Paula;

bool paula_owns_reg(rigel_u32 addr);
rigel_u16 paula_read_reg(RigelContext *ctx, rigel_u32 addr);
void paula_write_reg(RigelContext *ctx, rigel_u32 addr, rigel_u16 value);

void paula_irq_raise(Paula *paula, uint16_t mask);
void paula_irq_clear(Paula *paula, uint16_t mask);

#endif
