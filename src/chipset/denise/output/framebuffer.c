#include "denise/output/framebuffer.h"

#include <string.h>
#include <stdint.h>

#include "simd/rigel_simd.h"

static void copy_visible_line_to_target(const RigelDenise *denise)
{
    const rigel_denise_output_state_t *output;
    const rigel_framebuffer_target_t *target;
    rigel_u16 raster_y;
    rigel_u32 row;
    rigel_u32 width;
    rigel_u16 x0;
    rigel_u8 *dst;

    if (denise == NULL) {
        return;
    }

    output = &denise->output;
    if (!output->has_write_target) {
        return;
    }

    target = &output->write_target;
    raster_y = output->beam_vpos;
    if (raster_y < denise->video.visible_y_start ||
        raster_y >= denise->video.visible_y_stop) {
        return;
    }

    row = (rigel_u32)(raster_y - denise->video.visible_y_start);
    if (row >= target->height) {
        return;
    }

    width = denise->video.width;
    if (width > target->width) {
        width = target->width;
    }

    x0 = denise->video.visible_x_start;
    dst = (rigel_u8 *)target->pixels + row * target->pitch;

    if (target->format == RIGEL_PIXEL_RGB565) {
        if (target->little_endian) {
            rigel_simd_copy_u16_to_le(dst, &output->scanline_rgb565[x0], width);
        } else {
            rigel_simd_copy_u16((rigel_u16 *)dst, &output->scanline_rgb565[x0], width);
        }
    } else {
        (void)memcpy(dst,
                     &output->scanline_rgba[x0],
                     width * sizeof(rigel_u32));
    }
}

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
    (void)memset(output->scanline_rgb565, 0, sizeof(output->scanline_rgb565));
    (void)memset(output->frame_rgba, 0, sizeof(output->frame_rgba));
    (void)memset(output->frame_rgb565, 0, sizeof(output->frame_rgb565));
    output->front_idx = 0;
    (void)memset(output->plane_words, 0, sizeof(output->plane_words));
    output->plane_word_count = 0;
    output->ddfstrt_lores = 0;
    output->visible_scanline = false;
    output->scanline_dirty = false;
    output->frame_dirty = true;
    output->beam_lof = 0;
    output->beam_lof_toggle = 0;
    output->pending_flags   = 0;
    output->completed_flags = 0;
    (void)memset(output->pending_dirty,   0, sizeof(output->pending_dirty));
    (void)memset(output->completed_dirty, 0, sizeof(output->completed_dirty));
    output->pending_full_redraw = false;
    output->completed_full_redraw = false;
    output->has_write_target = false;
    (void)memset(&output->write_target, 0, sizeof(output->write_target));
}

void rigel_denise_framebuffer_set_target(rigel_denise_output_state_t *output,
                                         const rigel_framebuffer_target_t *target)
{
    if (output == NULL) {
        return;
    }

    output->has_write_target = false;
    (void)memset(&output->write_target, 0, sizeof(output->write_target));

    if (target == NULL || target->pixels == NULL ||
        target->width == 0u || target->height == 0u || target->pitch == 0u) {
        return;
    }

    if (target->format != RIGEL_PIXEL_RGB565 &&
        target->format != RIGEL_PIXEL_RGBA8888) {
        return;
    }

    if (target->format == RIGEL_PIXEL_RGB565 &&
        target->pitch < target->width * sizeof(rigel_u16)) {
        return;
    }

    if (target->format == RIGEL_PIXEL_RGBA8888 &&
        target->pitch < target->width * sizeof(rigel_u32)) {
        return;
    }

    output->write_target = *target;
    output->has_write_target = true;
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
        if (output->beam_vpos < RIGEL_DENISE_MAX_LINES) {
            rigel_u8 back_idx = (rigel_u8)(1u ^ output->front_idx);
            (void)memcpy(output->frame_rgba[1u ^ output->front_idx][output->beam_vpos],
                         output->scanline_rgba,
                         sizeof(output->scanline_rgba));
            (void)memcpy(output->frame_rgb565[back_idx][output->beam_vpos],
                         output->scanline_rgb565,
                         sizeof(output->scanline_rgb565));
            copy_visible_line_to_target(denise);
        }
        (void)memset(output->plane_words, 0, sizeof(output->plane_words));
        output->plane_word_count = 0;
    }

    /* At frame boundary: snapshot metadata, then make the back buffer visible.
     * Swap runs after writing the last scanline so the front buffer is complete. */
    if (frame_changed) {
        if (output->beam_lof_toggle) {
            output->pending_flags |= output->beam_lof
                ? (rigel_u32)RIGEL_FRAME_INTERLACE_ODD
                : (rigel_u32)RIGEL_FRAME_INTERLACE_EVEN;
        }
        output->completed_flags = output->pending_flags;
        (void)memcpy(output->completed_dirty, output->pending_dirty,
                     sizeof(output->completed_dirty));
        output->completed_full_redraw = output->pending_full_redraw;
        output->pending_flags = 0;
        (void)memset(output->pending_dirty, 0, sizeof(output->pending_dirty));
        output->pending_full_redraw = false;
        output->front_idx ^= 1u;
    }

    output->frame_counter = beam->frame_count;
    output->beam_hpos = beam->hpos;
    output->beam_vpos = beam->vpos;
    output->beam_lof = beam->lof;
    output->beam_lof_toggle = beam->lof_toggle;
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
