#ifndef RIGEL_INPUT_DOMAIN_H
#define RIGEL_INPUT_DOMAIN_H

#include "paula/input.h"
#include "rigel/rigel_custom.h"

bool rigel_input_domain_owns_reg(rigel_u32 addr);
void rigel_input_domain_reset(input_state_t *input);
rigel_u16 rigel_input_domain_read_reg(const input_state_t *input, rigel_u32 addr);
void rigel_input_domain_write_reg(input_state_t *input, rigel_u32 addr, rigel_u16 value);

#endif
