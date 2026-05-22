#include "ehb.h"

rigel_u32 ehb_resolve_color(uint8_t plane_bits, const rigel_u32 palette[32])
{
    if (plane_bits & 0x20u) {
        /* EHB set: half brightness of base color */
        rigel_u32 base = palette[plane_bits & 0x1Fu];
        uint8_t r = ((base >> 16) & 0xFFu) >> 1;
        uint8_t g = ((base >>  8) & 0xFFu) >> 1;
        uint8_t b = ( base        & 0xFFu) >> 1;
        return ((rigel_u32)r << 16) | ((rigel_u32)g << 8) | b;
    }
    return palette[plane_bits & 0x1Fu];
}
