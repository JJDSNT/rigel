#include "blitter_logic.h"

uint16_t blitter_logic_minterm(uint8_t minterm, uint16_t a, uint16_t b, uint16_t c)
{
    uint16_t result = 0;
    for (int bit = 0; bit < 16; bit++) {
        unsigned ab = ((a >> bit) & 1u);
        unsigned bb = ((b >> bit) & 1u);
        unsigned cb = ((c >> bit) & 1u);
        unsigned idx = (ab << 2) | (bb << 1) | cb;
        if ((minterm >> idx) & 1u)
            result |= (uint16_t)(1u << bit);
    }
    return result;
}
