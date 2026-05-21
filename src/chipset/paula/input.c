#include "paula/input.h"

#include <stddef.h>

void input_reset(input_state_t *input)
{
    if (input == NULL) {
        return;
    }

    input->potgo = 0xffffu;
    input->joydat[0] = 0;
    input->joydat[1] = 0;
    input->pot_button_x[0] = 0;
    input->pot_button_x[1] = 0;
    input->pot_button_y[0] = 0;
    input->pot_button_y[1] = 0;
}

rigel_u16 input_read_joydat(const input_state_t *input, rigel_u32 port)
{
    if (input == NULL || port > 1u) {
        return 0;
    }

    return input->joydat[port];
}

rigel_u16 input_read_potdat(const input_state_t *input, rigel_u32 port)
{
    rigel_u16 value = 0xffffu;

    if (input == NULL || port > 1u) {
        return 0xffffu;
    }

    if (port == 0u) {
        if ((input->potgo & 0x0200u) == 0 && input->pot_button_x[0] != 0) {
            value &= 0xff00u;
        }
        if ((input->potgo & 0x0800u) == 0 && input->pot_button_y[0] != 0) {
            value &= 0x00ffu;
        }
        return value;
    }

    if ((input->potgo & 0x2000u) == 0 && input->pot_button_x[1] != 0) {
        value &= 0xff00u;
    }
    if ((input->potgo & 0x8000u) == 0 && input->pot_button_y[1] != 0) {
        value &= 0x00ffu;
    }

    return value;
}

rigel_u16 input_read_potgor(const input_state_t *input)
{
    rigel_u16 value = 0xff00u;

    if (input == NULL) {
        return value;
    }

    if ((input->potgo & 0x0200u) && (input->potgo & 0x0100u) && input->pot_button_x[0]) {
        value &= (rigel_u16)~0x0100u;
    }
    if ((input->potgo & 0x0800u) && (input->potgo & 0x0400u) && input->pot_button_y[0]) {
        value &= (rigel_u16)~0x0400u;
    }
    if ((input->potgo & 0x2000u) && (input->potgo & 0x1000u) && input->pot_button_x[1]) {
        value &= (rigel_u16)~0x1000u;
    }
    if ((input->potgo & 0x8000u) && (input->potgo & 0x4000u) && input->pot_button_y[1]) {
        value &= (rigel_u16)~0x4000u;
    }

    return value;
}

void input_write_potgo(input_state_t *input, rigel_u16 value)
{
    if (input == NULL) {
        return;
    }

    input->potgo = value;
}

void input_set_joydat(input_state_t *input, rigel_u32 port, rigel_u16 value)
{
    if (input == NULL || port > 1u) {
        return;
    }

    input->joydat[port] = value;
}

void input_set_pot_button_x(input_state_t *input, rigel_u32 port, int pressed)
{
    if (input == NULL || port > 1u) {
        return;
    }

    input->pot_button_x[port] = pressed ? 1u : 0u;
}

void input_set_pot_button_y(input_state_t *input, rigel_u32 port, int pressed)
{
    if (input == NULL || port > 1u) {
        return;
    }

    input->pot_button_y[port] = pressed ? 1u : 0u;
}
