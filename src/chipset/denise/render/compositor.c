#include "denise/render/compositor.h"

#include <string.h>
#include <stdint.h>

#include "denise/output/framebuffer.h"
#include "denise/output/planar.h"
#include "denise/render/playfield.h"

static void compose_line(RigelDenise *denise)
{
    rigel_denise_output_state_t *output = &denise->output;
    unsigned depth = (denise->regs.bplcon0 >> 12) & 0x7u;
    rigel_u16 w, px;

    if (!output->visible_scanline || output->scanline_width == 0) return;

    /* Fill with background color (palette[0] = COLOR00). palette entries already carry alpha. */
    for (px = 0; px < output->scanline_width; px++)
        output->scanline_rgba[px] = denise->palette.rgb32[0];

    if (depth > 0 && depth <= 6 && output->plane_word_count > 0) {
        for (w = 0; w < output->plane_word_count; w++) {
            rigel_u16 block_words[6];
            uint8_t   pixels_chunky[16];
            unsigned  p;

            for (p = 0; p < 6; p++)
                block_words[p] = (p < depth) ? output->plane_words[p][w] : 0u;

            planar_to_chunky(block_words, depth, pixels_chunky);

            for (px = 0; px < 16; px++) {
                rigel_u16 x = (rigel_u16)(w * 16u + px);
                if (x < output->scanline_width)
                    output->scanline_rgba[x] =
                        denise->palette.rgb32[pixels_chunky[px] & 0x1Fu];
            }
        }
    }

    output->scanline_dirty = true;
}

void rigel_denise_compositor_tick(RigelDenise *denise, const beam_state_t *beam, rigel_u32 cycles)
{
    bool line_changed;
    bool was_visible;

    (void)cycles;

    if (denise == NULL || beam == NULL) {
        return;
    }

    line_changed = (denise->output.beam_vpos != beam->vpos) ||
                   (denise->output.frame_counter != beam->frame_count);
    was_visible = denise->output.visible_scanline;

    /* Compose the line that just ended using its accumulated plane data. */
    if (line_changed && was_visible) {
        compose_line(denise);
    }

    rigel_denise_framebuffer_sync_from_beam(denise, beam);

    /* When entering a visible scanline from a non-visible one, fill the
     * buffer now with the current palette so callers see a valid initial
     * state (mostly background color) before the line's bitplane data
     * accumulates and the end-of-line compose overwrites it. */
    if (line_changed && !was_visible && denise->output.visible_scanline) {
        compose_line(denise);
    }

    denise->output.last_rgb = denise->palette.rgb32[denise->debug.last_color_index & 31u];
}
