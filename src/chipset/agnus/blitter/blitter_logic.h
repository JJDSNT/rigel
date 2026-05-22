#ifndef RIGEL_AGNUS_BLITTER_LOGIC_H
#define RIGEL_AGNUS_BLITTER_LOGIC_H

#include <stdint.h>

/* Blitter copy-mode logic unit — applies the 8-bit minterm LUT.
 *
 * The minterm selects the output bit for each combination of A, B, C:
 *   bit 7 → A=1, B=1, C=1
 *   bit 6 → A=1, B=1, C=0
 *   ...
 *   bit 0 → A=0, B=0, C=0
 *
 * Operates word-wide: each bit position is independent. */

uint16_t blitter_logic_minterm(uint8_t minterm,
                               uint16_t a, uint16_t b, uint16_t c);

#endif
