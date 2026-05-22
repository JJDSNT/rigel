#include "blitter_fill.h"

uint16_t blitter_fill_word(uint16_t word, BlitterFillMode mode, bool *carry)
{
    uint16_t result = 0;
    bool c = *carry;
    for (int bit = 15; bit >= 0; bit--) {
        bool pixel = (word >> bit) & 1u;
        if (pixel) c = !c;
        bool out;
        if (mode == BLITTER_FILL_INCLUSIVE)
            out = c;
        else
            out = c && !pixel;  /* exclusive: fill between edges, not on them */
        if (out) result |= (uint16_t)(1u << bit);
    }
    *carry = c;
    return result;
}
