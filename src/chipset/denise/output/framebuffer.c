#include "denise/output/framebuffer.h"

#include <string.h>
#include <stdint.h>

void rigel_denise_framebuffer_reset(rigel_denise_output_state_t *output)
{
    if (output == NULL) {
        return;
    }

    output->frame_counter = 0;
    output->beam_hpos = 0;
    output->beam_vpos = 0;
    output->current_scanline = 0;
    output->current_pixel = 0;
    output->scanline_width = 0;
    output->last_rgb = 0;
    (void)memset(output->scanline_rgba, 0, sizeof(output->scanline_rgba));
    (void)memset(output->plane_words, 0, sizeof(output->plane_words));
    output->plane_word_count = 0;
    output->visible_scanline = false;
    output->scanline_dirty = false;
    output->frame_dirty = true;
}

void rigel_denise_framebuffer_sync_from_beam(RigelDenise *denise, const beam_state_t *beam)
{
    rigel_denise_output_state_t *output;
    bool frame_changed;
    bool line_changed;

    if (denise == NULL || beam == NULL) {
        return;
    }

    output = &denise->output;
    frame_changed = output->frame_counter != beam->frame_count;
    line_changed = frame_changed || output->beam_vpos != beam->vpos;

    if (line_changed) {
        /* plane_words cleared here for the new line; compositor manages scanline_rgba and dirty */
        (void)memset(output->plane_words, 0, sizeof(output->plane_words));
        output->plane_word_count = 0;
    }

    output->frame_counter = beam->frame_count;
    output->beam_hpos = beam->hpos;
    output->beam_vpos = beam->vpos;
    output->frame_dirty = frame_changed;
    output->visible_scanline = beam_in_visible_area(beam);
    output->scanline_width = denise->video.width;

    if (output->visible_scanline) {
        output->current_scanline = (rigel_u16)(beam->vpos - beam->visible_y_start);
        output->current_pixel = beam->hpos;
        if (output->current_pixel > output->scanline_width) {
            output->current_pixel = output->scanline_width;
        }
    } else {
        output->current_scanline = beam->vpos;
        output->current_pixel = 0;
    }
}
