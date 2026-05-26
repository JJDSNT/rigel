#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "denise/render/priority.h"

int main(void)
{
    uint8_t sprite_px[8];
    priority_result_t r;

    /* --- all sprites transparent, playfield opaque → playfield wins --- */
    memset(sprite_px, 0, sizeof(sprite_px));
    r = denise_priority_resolve(5, false, sprite_px, 0x0000u);
    assert(!r.from_sprite);
    assert(r.color_index == 5);

    /* --- all transparent (both PF and sprites) → background (0) --- */
    r = denise_priority_resolve(0, false, sprite_px, 0x0000u);
    assert(!r.from_sprite);
    assert(r.color_index == 0);

    /* --- sprite 0 opaque, playfield transparent → sprite 0 wins --- */
    sprite_px[0] = 17;
    r = denise_priority_resolve(0, false, sprite_px, 0x0000u);
    assert(r.from_sprite);
    assert(r.sprite_num  == 0);
    assert(r.color_index == 17);

    /* --- sprite 0 opaque, PF1 opaque, PF1P=0 → sprite pair 0 + 0 < 4 → sprite wins --- */
    r = denise_priority_resolve(5, false, sprite_px, 0x0000u);
    assert(r.from_sprite);
    assert(r.sprite_num  == 0);
    assert(r.color_index == 17);

    /* --- sprite 0 opaque, PF1 opaque, PF1P=4 → pair 0 + 4 = 4, not < 4 → PF wins --- */
    r = denise_priority_resolve(5, false, sprite_px, 0x0004u);
    assert(!r.from_sprite);
    assert(r.color_index == 5);

    /* --- sprite 6 (pair 3) vs PF1P=0 → 3+0 < 4 → sprite wins --- */
    memset(sprite_px, 0, sizeof(sprite_px));
    sprite_px[6] = 28;
    r = denise_priority_resolve(5, false, sprite_px, 0x0000u);
    assert(r.from_sprite);
    assert(r.sprite_num  == 6);
    assert(r.color_index == 28);

    /* --- sprite 6 (pair 3) vs PF1P=1 → 3+1 = 4, not < 4 → PF wins --- */
    r = denise_priority_resolve(5, false, sprite_px, 0x0001u);
    assert(!r.from_sprite);
    assert(r.color_index == 5);

    /* --- sprite 0 and sprite 7 both opaque → sprite 0 wins (lower number) --- */
    memset(sprite_px, 0, sizeof(sprite_px));
    sprite_px[0] = 17;
    sprite_px[7] = 31;
    r = denise_priority_resolve(0, false, sprite_px, 0x0000u);
    assert(r.from_sprite);
    assert(r.sprite_num  == 0);
    assert(r.color_index == 17);

    /* --- PF2 path: sprite 0 vs PF2P=4 → pair 0 + 4 = 4, not < 4 → PF2 wins --- */
    memset(sprite_px, 0, sizeof(sprite_px));
    sprite_px[0] = 17;
    r = denise_priority_resolve(9, true, sprite_px, 0x0020u);
    assert(!r.from_sprite);
    assert(r.color_index == 9);

    printf("test_priority: PASS\n");
    return 0;
}
