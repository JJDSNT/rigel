#include <stdio.h>
#include <assert.h>

#include "denise/render/dualpf.h"

int main(void)
{
    dualpf_result_t r;
    uint8_t win;

    /* --- plane splitting ---
     * Odd planes (bits 0,2,4) → PF1; even planes (bits 1,3,5) → PF2.
     * A non-zero PF2 index has 8 added; PF1 index is returned as-is (1-7). */

    /* 0b101010: bits 0,2,4 = 0 → PF1 transparent; bits 1,3,5 = 1,1,1 → PF2 = 7+8 */
    r = dualpf_decode(0b101010u);
    assert(r.pf1_index == 0);
    assert(r.pf2_index == 15);

    /* 0b010101: bits 0,2,4 = 1,1,1 → PF1 = 7; bits 1,3,5 = 0 → PF2 transparent */
    r = dualpf_decode(0b010101u);
    assert(r.pf1_index == 7);
    assert(r.pf2_index == 0);

    /* 0b000000: both transparent */
    r = dualpf_decode(0u);
    assert(r.pf1_index == 0);
    assert(r.pf2_index == 0);

    /* 0b111111: all planes set → PF1 = 7, PF2 = 15 */
    r = dualpf_decode(0b111111u);
    assert(r.pf1_index == 7);
    assert(r.pf2_index == 15);

    /* 0b000001: only bit0 → PF1 = 1, PF2 transparent */
    r = dualpf_decode(0b000001u);
    assert(r.pf1_index == 1);
    assert(r.pf2_index == 0);

    /* 0b000010: only bit1 → PF1 transparent, PF2 = 8+1 = 9 */
    r = dualpf_decode(0b000010u);
    assert(r.pf1_index == 0);
    assert(r.pf2_index == 9);

    /* --- priority resolution --- */

    /* both transparent → background (0) */
    r.pf1_index = 0; r.pf2_index = 0;
    win = dualpf_priority_resolve(&r, 0x0000u);
    assert(win == 0);

    /* only PF1 → PF1 wins regardless of PFxP */
    r.pf1_index = 3; r.pf2_index = 0;
    win = dualpf_priority_resolve(&r, 0x0000u);
    assert(win == 3);

    /* only PF2 → PF2 wins */
    r.pf1_index = 0; r.pf2_index = 12;
    win = dualpf_priority_resolve(&r, 0x0000u);
    assert(win == 12);

    /* both opaque, PF2PRI clear → PF1 wins.  PF1P/PF2P only affect sprites. */
    r.pf1_index = 3; r.pf2_index = 12;
    win = dualpf_priority_resolve(&r, 0x0020u);
    assert(win == 3);

    /* both opaque, PF2PRI set → PF2 wins. */
    win = dualpf_priority_resolve(&r, 0x0040u);
    assert(win == 12);

    printf("test_dualpf: PASS\n");
    return 0;
}
