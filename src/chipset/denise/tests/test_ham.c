#include <stdio.h>
#include <assert.h>

#include "denise/render/ham.h"

/* HAM6 encoding: ctrl = plane_bits[5:4], value = plane_bits[3:0]
 *   00 → load palette[plane_bits[4:0]]
 *   01 → modify blue:  keep RG, replace B with value<<4
 *   10 → modify red:   keep GB, replace R with value<<4
 *   11 → modify green: keep RB, replace G with value<<4 */

int main(void)
{
    rigel_u32 palette[32] = {0};
    rigel_u32 px;

    palette[0] = 0x000000u;
    palette[1] = 0xFF0000u;   /* red */
    palette[2] = 0x00FF00u;   /* green */
    palette[3] = 0x0000FFu;   /* blue */

    /* --- ctrl=00: load from palette ---
     * plane_bits bits[5:4]=00 → palette[plane_bits & 0x1F] */
    px = ham6_decode_pixel(0x01u, 0x000000u, palette);
    assert(px == 0xFF0000u);

    px = ham6_decode_pixel(0x02u, 0xFF0000u, palette);
    assert(px == 0x00FF00u);

    px = ham6_decode_pixel(0x00u, 0x123456u, palette);
    assert(px == 0x000000u);

    /* --- ctrl=01: modify blue (bits[5:4]=01 → plane_bits 0x10..0x1F) ---
     * value = bits[3:0], expanded to byte as value<<4.
     * 0x1F: ctrl=01, nibble=0xF → blue = 0xF0 */
    px = ham6_decode_pixel(0x1Fu, 0xFF0000u, palette);
    assert((px & 0x0000FFu) == 0xF0u);
    assert((px & 0xFF0000u) == 0xFF0000u);
    assert((px & 0x00FF00u) == 0x000000u);

    /* 0x10: ctrl=01, nibble=0 → blue = 0x00; R+G unchanged */
    px = ham6_decode_pixel(0x10u, 0xAABBCCu, palette);
    assert((px & 0xFF0000u) == 0xAA0000u);
    assert((px & 0x00FF00u) == 0xBB00u);
    assert((px & 0x0000FFu) == 0x00u);

    /* --- ctrl=10: modify red (bits[5:4]=10 → plane_bits 0x20..0x2F) ---
     * 0x2A: ctrl=10, nibble=0xA → red = 0xA0 */
    px = ham6_decode_pixel(0x2Au, 0x00FFFFu, palette);
    assert((px & 0xFF0000u) == 0xA00000u);
    assert((px & 0x00FF00u) == 0xFF00u);
    assert((px & 0x0000FFu) == 0xFFu);

    /* --- ctrl=11: modify green (bits[5:4]=11 → plane_bits 0x30..0x3F) ---
     * 0x3A: ctrl=11, nibble=0xA → green = 0xA0 */
    px = ham6_decode_pixel(0x3Au, 0xFF00FFu, palette);
    assert((px & 0xFF0000u) == 0xFF0000u);
    assert((px & 0x00FF00u) == 0xA000u);
    assert((px & 0x0000FFu) == 0xFFu);

    /* --- HAM8: ctrl = plane_bits[7:6], value = plane_bits[5:0] → 8-bit via v6<<2|v6>>4 --- */
    rigel_u32 pal8[256] = {0};
    pal8[1] = 0xFF0000u;

    /* ctrl=00: load palette[v6=1] */
    px = ham8_decode_pixel(0x01u, 0x000000u, pal8);
    assert(px == 0xFF0000u);

    /* ctrl=01: modify blue, v6=0x3F → val=0xFF */
    px = ham8_decode_pixel(0x7Fu, 0xFF0000u, pal8);
    assert((px & 0x0000FFu) == 0xFFu);
    assert((px & 0xFF0000u) == 0xFF0000u);

    /* ctrl=10: modify red, v6=0x0F → val=0x3C */
    px = ham8_decode_pixel(0x8Fu, 0x00FFFFu, pal8);
    assert((px & 0xFF0000u) == 0x3C0000u);
    assert((px & 0x00FFFFu) == 0x00FFFFu);

    /* ctrl=11: modify green, v6=0x0F → val=0x3C */
    px = ham8_decode_pixel(0xCFu, 0xFF00FFu, pal8);
    assert((px & 0x00FF00u) == 0x3C00u);
    assert((px & 0xFF00FFu) == 0xFF00FFu);

    printf("test_ham: PASS\n");
    return 0;
}
