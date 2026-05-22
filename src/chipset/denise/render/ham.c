#include "ham.h"

rigel_u32 ham6_decode_pixel(uint8_t plane_bits,
                            rigel_u32 prev_rgb,
                            const rigel_u32 palette[32])
{
    uint8_t ctrl  = (plane_bits >> 4) & 0x3u;
    uint8_t value = (plane_bits & 0xFu) << 4;  /* expand nibble to byte */

    switch (ctrl) {
    case 0: /* load */
        return palette[plane_bits & 0x1Fu];
    case 1: /* modify blue */
        return (prev_rgb & 0xFFFF00u) | value;
    case 2: /* modify red */
        return (prev_rgb & 0x00FFFFu) | ((rigel_u32)value << 16);
    case 3: /* modify green */
        return (prev_rgb & 0xFF00FFu) | ((rigel_u32)value << 8);
    default:
        return prev_rgb;
    }
}

rigel_u32 ham8_decode_pixel(uint8_t plane_bits,
                            rigel_u32 prev_rgb,
                            const rigel_u32 palette[256])
{
    (void)plane_bits;
    (void)prev_rgb;
    (void)palette;
    /* TODO(render): HAM8 for ECS/AGA */
    return 0;
}
