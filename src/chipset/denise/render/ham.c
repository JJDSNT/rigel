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
    /* AGA HAM8: bits[7:6] = control, bits[5:0] = 6-bit channel value.
     * Modify: 6-bit value fills channel[7:2]; bits[1:0] replicate bits[7:6]
     * of the new channel (i.e., top 2 bits of the 6-bit field). */
    uint8_t  ctrl = (plane_bits >> 6) & 0x3u;
    uint8_t  v6   = (uint8_t)(plane_bits & 0x3Fu);
    uint8_t  val  = (uint8_t)((v6 << 2) | (v6 >> 4));  /* 6-bit → 8-bit with replication */

    switch (ctrl) {
    case 0: return palette[v6];                                              /* load */
    case 1: return (prev_rgb & 0xFFFF00u) | val;                             /* blue  */
    case 2: return (prev_rgb & 0x00FFFFu) | ((rigel_u32)val << 16);          /* red   */
    case 3: return (prev_rgb & 0xFF00FFu) | ((rigel_u32)val << 8);           /* green */
    default: return prev_rgb;
    }
}
