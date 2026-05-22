#include "denise/output/framebuffer.h"

void rigel_denise_framebuffer_reset(rigel_denise_output_state_t *output)
{
    if (output == NULL) {
        return;
    }

    output->current_scanline = 0;
    output->current_pixel = 0;
    output->last_rgb = 0;
}
