#ifndef RIGEL_AGNUS_BLITTER_FILL_H
#define RIGEL_AGNUS_BLITTER_FILL_H

#include <stdint.h>
#include <stdbool.h>
#include "blitter_types.h"

/* Blitter area-fill mode — inclusive or exclusive fill.
 *
 * Fill is applied right-to-left across each word of D-channel output.
 * A carry bit propagates between words. The fill function does not
 * modify A/B/C channels — only the D output word. */

uint16_t blitter_fill_word(uint16_t word, BlitterFillMode mode, bool *carry);

#endif
