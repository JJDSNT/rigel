#include "denise/sprites/sprites_core.h"
#include "denise/sprites/sprites.h"

void rigel_denise_sprites_reset(denise_sprites_state_t *sprites)
{
    if (sprites == NULL) {
        return;
    }

    denise_sprites_reset(sprites);
}
