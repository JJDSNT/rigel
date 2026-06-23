#include "rigel/rigel_denise.h"

#include <string.h>

#include "core/rigel_context.h"
#include "chipset/denise/denise_state.h"

static void rigel_denise_frame_x_bounds(const RigelDenise *denise,
                                        rigel_u16 *x0,
                                        rigel_u16 *x1)
{
    rigel_u16 start = denise->video.visible_x_start;
    rigel_u16 stop = denise->video.visible_x_stop;

    if (stop > (rigel_u16)RIGEL_DENISE_MAX_SCANLINE_PIXELS)
        stop = (rigel_u16)RIGEL_DENISE_MAX_SCANLINE_PIXELS;
    if (start > stop)
        start = 0u;

    /*
     * Export a monitor-like viewport, not the whole internal HPOS range.  Large
     * DIW starts such as WB1.3 hires (x=252) otherwise expose a huge blank band
     * before the playfield.  Keep a small left border so sprites that sit just
     * outside the DIW, including the hardware mouse pointer at the left edge,
     * remain visible.
     */
    if (start > 128u) {
        start = (rigel_u16)(start - 32u);
    } else {
        start = 0u;
    }

    *x0 = start;
    *x1 = stop;
}

bool rigel_denise_get_video_desc(const RigelContext *ctx, rigel_denise_video_desc_t *desc)
{
    const RigelDenise *denise;

    if (ctx == NULL || desc == NULL) {
        return false;
    }

    denise = &ctx->chipset.denise;
    desc->display_width = denise->video.width;
    desc->display_height = denise->video.height;
    desc->visible_x_start = denise->video.visible_x_start;
    desc->visible_x_stop = denise->video.visible_x_stop;
    desc->visible_y_start = denise->video.visible_y_start;
    desc->visible_y_stop = denise->video.visible_y_stop;
    return true;
}

bool rigel_denise_get_debug_state(const RigelContext *ctx, rigel_denise_debug_state_t *state)
{
    if (ctx == NULL || state == NULL) {
        return false;
    }

    *state = ctx->chipset.denise.debug;
    return true;
}

bool rigel_denise_get_current_scanline(const RigelContext *ctx, rigel_denise_scanline_t *scanline)
{
    const RigelDenise *denise;

    if (ctx == NULL || scanline == NULL) {
        return false;
    }

    denise = &ctx->chipset.denise;
    scanline->frame_counter = denise->output.frame_counter;
    scanline->y = denise->output.current_scanline;
    scanline->width = denise->output.scanline_width;
    scanline->pixels_rgba = denise->output.scanline_rgba;
    scanline->last_rgb32 = denise->output.last_rgb;
    scanline->visible = denise->output.visible_scanline;
    scanline->dirty = denise->output.scanline_dirty;
    return true;
}

bool rigel_get_scanline(const RigelContext *ctx, rigel_u16 y, rigel_denise_scanline_t *out)
{
    const RigelDenise *denise;
    rigel_u16 x0;
    rigel_u16 x1;

    if (ctx == NULL || out == NULL || y >= RIGEL_DENISE_MAX_LINES) {
        return false;
    }

    denise = &ctx->chipset.denise;
    rigel_denise_frame_x_bounds(denise, &x0, &x1);

    out->frame_counter = denise->output.frame_counter;
    out->y             = y;
    out->width         = (rigel_u16)(x1 - x0);
    out->pixels_rgba   = &denise->output.frame_rgba[denise->output.front_idx][y][x0];
    out->last_rgb32    = 0;
    out->visible       = (y >= denise->video.visible_y_start &&
                          y <  denise->video.visible_y_stop);
    out->dirty         = false;
    return true;
}

bool rigel_get_frame(const RigelContext *ctx, rigel_frame_t *frame)
{
    const RigelDenise *denise;
    rigel_pixel_format_t format;
    rigel_u16 y0;
    rigel_u16 x0;
    rigel_u16 x1;

    if (ctx == NULL || frame == NULL) {
        return false;
    }

    denise = &ctx->chipset.denise;
    format = ctx->config.pixel_format;
    y0 = denise->video.visible_y_start;
    rigel_denise_frame_x_bounds(denise, &x0, &x1);

    /*
     * Guard: if visible_y_start is outside the valid raster range (e.g. because
     * the VBL copper wrote DIWSTRT=0xffff+DIWHIGH=0x00ff before BPLCON0 was reset
     * to depth=0, which makes the ECS decode produce vstrt=2047), the subtraction
     * visible_y_stop - y0 would wrap to a huge uint32.  Return an empty frame.
     */
    if (y0 >= (rigel_u16)RIGEL_DENISE_MAX_LINES ||
        denise->video.visible_y_stop <= y0 ||
        x1 <= x0) {
        frame->width       = 0u;
        frame->height      = 0u;
        frame->pitch       = 0u;
        frame->frame_count = denise->output.frame_counter;
        frame->format      = format;
        frame->pixels      = NULL;
        frame->flags       = (rigel_frame_flags_t)denise->output.completed_flags;
        frame->delta.full_redraw = denise->output.completed_full_redraw;
        (void)memcpy(frame->delta.dirty_lines, denise->output.completed_dirty,
                     sizeof(frame->delta.dirty_lines));
        return true;
    }

    frame->width       = (rigel_u32)(x1 - x0);
    frame->height      = (rigel_u32)(denise->video.visible_y_stop  - y0);
    frame->frame_count = denise->output.frame_counter;
    frame->format      = format;
    if (format == RIGEL_PIXEL_RGB565) {
        frame->pitch = (rigel_u32)(RIGEL_DENISE_MAX_SCANLINE_PIXELS * sizeof(rigel_u16));
        frame->pixels = (y0 < RIGEL_DENISE_MAX_LINES)
                            ? &denise->output.frame_rgb565[denise->output.front_idx][y0][x0]
                            : &denise->output.frame_rgb565[denise->output.front_idx][0][0];
    } else {
        frame->pitch = (rigel_u32)(RIGEL_DENISE_MAX_SCANLINE_PIXELS * sizeof(rigel_u32));
        frame->pixels = (y0 < RIGEL_DENISE_MAX_LINES)
                            ? &denise->output.frame_rgba[denise->output.front_idx][y0][x0]
                            : &denise->output.frame_rgba[denise->output.front_idx][0][0];
    }
    frame->flags       = (rigel_frame_flags_t)denise->output.completed_flags;
    frame->delta.full_redraw = denise->output.completed_full_redraw;
    (void)memcpy(frame->delta.dirty_lines, denise->output.completed_dirty,
                 sizeof(frame->delta.dirty_lines));
    return true;
}
