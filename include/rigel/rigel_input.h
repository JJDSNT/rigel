#ifndef RIGEL_INPUT_H
#define RIGEL_INPUT_H

#include <stdbool.h>

#include "rigel_types.h"

/*
 * Set joystick/mouse directional data (JOY0DAT / JOY1DAT).
 * port: 0 = port A (mouse/joy0), 1 = port B (joy1).
 * value: bits 15-8 = Y counter, bits 7-0 = X counter (8-bit quadrature).
 */
void rigel_input_set_joydat(RigelContext *ctx, rigel_u32 port, rigel_u16 value);

/*
 * POT buttons — right mouse button (port 0) and second joystick button (port 1).
 * Mapped to POTGO/POTGOR (Paula side).
 */
void rigel_input_set_pot_button_x(RigelContext *ctx, rigel_u32 port, bool pressed);
void rigel_input_set_pot_button_y(RigelContext *ctx, rigel_u32 port, bool pressed);

/*
 * Primary fire button (left mouse button / joystick fire).
 * port 0 → CIA-A PRA bit 6 (/FIR0, active low).
 * port 1 → CIA-A PRA bit 7 (/FIR1, active low).
 * Requires CIA to be in the chipset (always true after rigel_create).
 */
void rigel_input_set_fire(RigelContext *ctx, rigel_u32 port, bool pressed);

#endif
