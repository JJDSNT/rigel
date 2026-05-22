#include "denise/sprites/sprites_core.h"

void rigel_denise_sprites_reset(rigel_denise_sprite_state_t *sprites)
{
    if (sprites == NULL) {
        return;
    }

    sprites->active_mask = 0;
    sprites->attached_mask = 0;
}
