#include <stdio.h>
#include <assert.h>

#include "denise/render/dualpf.h"

/* TODO(tests): verify dual playfield plane splitting and priority
 *   - 6-plane word → correct pf1_index (bits 0,2,4)
 *   - 6-plane word → correct pf2_index (bits 1,3,5)
 *   - transparent pixels (index=0) per playfield
 *   - priority resolve: PF2 over PF1, background */

int main(void)
{
    dualpf_result_t r;

    /* plane_bits = 0b101010 → PF1 bits = 0, PF2 bits = 7 */
    r = dualpf_decode(0b101010u);
    assert(r.pf1_index == 0);   /* transparent */
    assert(r.pf2_index == 8 + 7);

    /* plane_bits = 0b010101 → PF1 bits = 7, PF2 bits = 0 */
    r = dualpf_decode(0b010101u);
    assert(r.pf1_index == 7);
    assert(r.pf2_index == 0);   /* transparent */

    printf("test_dualpf: PASS\n");
    return 0;
}
