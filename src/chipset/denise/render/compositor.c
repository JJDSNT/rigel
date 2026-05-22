#include "denise/render/compositor.h"

#include "denise/output/framebuffer.h"
#include "denise/render/playfield.h"

void rigel_denise_compositor_tick(RigelDenise *denise, rigel_u32 cycles)
{
    if (denise == NULL) {
        return;
    }

    denise->output.current_pixel = (rigel_u16)(denise->output.current_pixel + cycles);
    denise->output.last_rgb = denise->palette.rgb32[denise->debug.last_color_index & 31u];
}
