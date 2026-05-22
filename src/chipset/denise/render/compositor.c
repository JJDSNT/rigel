#include "denise/render/compositor.h"

#include "denise/output/framebuffer.h"
#include "denise/render/playfield.h"

void rigel_denise_compositor_tick(RigelDenise *denise, const beam_state_t *beam, rigel_u32 cycles)
{
    rigel_u16 limit;
    rigel_u16 i;

    (void)cycles;

    if (denise == NULL) {
        return;
    }

    rigel_denise_framebuffer_sync_from_beam(denise, beam);
    denise->output.last_rgb = denise->palette.rgb32[denise->debug.last_color_index & 31u];

    if (!denise->output.visible_scanline || denise->output.scanline_width == 0) {
        return;
    }

    limit = denise->output.current_pixel;
    if (limit > denise->output.scanline_width) {
        limit = denise->output.scanline_width;
    }
    if (limit > RIGEL_DENISE_MAX_SCANLINE_PIXELS) {
        limit = RIGEL_DENISE_MAX_SCANLINE_PIXELS;
    }

    for (i = 0; i < limit; ++i) {
        denise->output.scanline_rgba[i] = denise->output.last_rgb;
    }
    denise->output.scanline_dirty = limit != 0;
}
