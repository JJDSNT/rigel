#ifndef RIGEL_KEYBOARD_H
#define RIGEL_KEYBOARD_H

#include <stdbool.h>

#include "rigel_types.h"

typedef struct RigelContext RigelContext;

/*
 * Inject an Amiga keyboard event.
 *
 * amiga_keycode: raw Amiga scancode, 7 bits (0x00–0x77).
 *   Common values: 0x41='A', 0x45='Esc', 0x60='Return', 0x40='Space', etc.
 *   See Amiga Hardware Reference Manual, Appendix E.
 *
 * pressed: true = key down, false = key up.
 *
 * Internally serialises the key code into CIA-B SDR using the Amiga keyboard
 * protocol (inverted, MSB first) and raises EXTER (IPL 6).
 * The host's interrupt handler will see RIGEL_EVENT_IRQ_CHANGED and can deliver
 * IPL 6 to the CPU; the CPU's interrupt handler reads CIA-B SDR to get the code.
 */
void rigel_keyboard_inject(RigelContext *ctx, rigel_u8 amiga_keycode, bool pressed);

#endif
