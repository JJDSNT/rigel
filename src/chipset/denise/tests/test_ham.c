#include <stdio.h>
#include <assert.h>

#include "denise/render/ham.h"

/* TODO(tests): full HAM6 test cases
 *   - load (ctrl=00): verify palette lookup
 *   - modify blue  (ctrl=01): verify only blue channel changes
 *   - modify red   (ctrl=10): verify only red channel changes
 *   - modify green (ctrl=11): verify only green channel changes
 *   - hold: verify prev_rgb passes through when value unchanged */

int main(void)
{
    /* Palette: color 0 = black, color 1 = red (#FF0000 expanded from #F00) */
    rigel_u32 palette[32] = {0};
    palette[0] = 0x000000u;
    palette[1] = 0xFF0000u;

    /* Load color 1 (ctrl=00, value=1) */
    rigel_u32 px = ham6_decode_pixel(0x01u, 0x000000u, palette);
    assert(px == 0xFF0000u);

    /* Modify blue: ctrl=01, value=0xF → blue channel = 0xF0 */
    px = ham6_decode_pixel(0x1Fu, 0xFF0000u, palette);
    assert((px & 0x0000FFu) == 0xF0u);
    assert((px & 0xFF0000u) == 0xFF0000u);

    printf("test_ham: PASS\n");
    return 0;
}
