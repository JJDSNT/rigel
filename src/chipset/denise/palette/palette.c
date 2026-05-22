#include "denise/palette/palette.h"

void rigel_denise_palette_reset(rigel_denise_palette_state_t *palette)
{
    rigel_u32 i;

    if (palette == NULL) {
        return;
    }

    for (i = 0; i < 32; ++i) {
        palette->rgb32[i] = 0;
    }
}
