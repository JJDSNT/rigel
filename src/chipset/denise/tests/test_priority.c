#include <stdio.h>
#include <assert.h>

#include "denise/render/priority.h"

/* TODO(tests): verify priority resolution matches HRM table
 *   - all sprites transparent → playfield wins
 *   - all playfield transparent → lowest-numbered sprite wins
 *   - PF1P=4: PF1 above sprites 0-5
 *   - PF2P=0: PF2 below all sprites
 *   - sprite 0 beats sprite 7 when both non-transparent */

int main(void)
{
    uint8_t sprite_px[8] = {0};
    priority_result_t r;

    /* No sprites, playfield colour 5 → playfield wins */
    r = denise_priority_resolve(5, false, sprite_px, 0x0000u);
    assert(!r.from_sprite);
    assert(r.color_index == 5);

    /* Sprite 0 = colour 17, playfield transparent → sprite 0 wins */
    sprite_px[0] = 17;
    r = denise_priority_resolve(0, false, sprite_px, 0x0000u);
    assert(r.from_sprite);
    assert(r.sprite_num  == 0);
    assert(r.color_index == 17);

    printf("test_priority: PASS (stub — priority ordering not yet enforced)\n");
    return 0;
}
