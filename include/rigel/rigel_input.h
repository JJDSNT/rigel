#ifndef RIGEL_INPUT_H
#define RIGEL_INPUT_H

#include <stdbool.h>

#include "rigel_types.h"

void rigel_input_set_joydat(RigelContext *ctx, rigel_u32 port, rigel_u16 value);
void rigel_input_set_pot_button_x(RigelContext *ctx, rigel_u32 port, bool pressed);
void rigel_input_set_pot_button_y(RigelContext *ctx, rigel_u32 port, bool pressed);

#endif
