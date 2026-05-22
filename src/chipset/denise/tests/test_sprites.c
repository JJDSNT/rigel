#include <stdio.h>
#include <assert.h>

#include "denise/sprites/sprites.h"

/* TODO(tests): implement sprite unit tests
 *   - hstart/vstart/vstop decoding from SPRxPOS/CTL
 *   - pixel shifting at correct hpos
 *   - attached sprite 4-bit index assembly
 *   - collision detection via CLXCON/CLXDAT */

int main(void)
{
    denise_sprites_state_t sprites;
    denise_sprites_reset(&sprites);

    /* Placeholder: verify reset state */
    for (unsigned i = 0; i < DENISE_SPRITE_COUNT; i++) {
        assert(!sprites.sp[i].armed);
        assert(!sprites.sp[i].visible);
    }
    assert(sprites.active_mask   == 0);
    assert(sprites.attached_mask == 0);

    printf("test_sprites: PASS (stub)\n");
    return 0;
}
