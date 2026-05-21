#ifndef RIGEL_PAULA_INPUT_H
#define RIGEL_PAULA_INPUT_H

#include "rigel/rigel_types.h"

typedef struct input_state {
    rigel_u16 potgo;
    rigel_u16 joydat[2];
    rigel_u8 pot_button_x[2];
    rigel_u8 pot_button_y[2];
} input_state_t;

void input_reset(input_state_t *input);
rigel_u16 input_read_joydat(const input_state_t *input, rigel_u32 port);
rigel_u16 input_read_potdat(const input_state_t *input, rigel_u32 port);
rigel_u16 input_read_potgor(const input_state_t *input);
void input_write_potgo(input_state_t *input, rigel_u16 value);
void input_set_joydat(input_state_t *input, rigel_u32 port, rigel_u16 value);
void input_set_pot_button_x(input_state_t *input, rigel_u32 port, int pressed);
void input_set_pot_button_y(input_state_t *input, rigel_u32 port, int pressed);

#endif
