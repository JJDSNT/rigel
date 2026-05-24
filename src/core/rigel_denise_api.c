#include "rigel/rigel_denise.h"

#include <string.h>

#include "core/rigel_context.h"
#include "chipset/denise/denise_state.h"

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

    if (ctx == NULL || out == NULL || y >= RIGEL_DENISE_MAX_LINES) {
        return false;
    }

    denise = &ctx->chipset.denise;
    x0     = denise->video.visible_x_start;

    out->frame_counter = denise->output.frame_counter;
    out->y             = y;
    out->width         = (rigel_u16)(denise->video.visible_x_stop - x0);
    out->pixels_rgba   = &denise->output.frame_rgba[y][x0];
    out->last_rgb32    = 0;
    out->visible       = (y >= denise->video.visible_y_start &&
                          y <  denise->video.visible_y_stop);
    out->dirty         = false;
    return true;
}

bool rigel_get_frame(const RigelContext *ctx, rigel_frame_t *frame)
{
    const RigelDenise *denise;
    rigel_u16 y0;
    rigel_u16 x0;

    if (ctx == NULL || frame == NULL) {
        return false;
    }

    denise = &ctx->chipset.denise;
    y0 = denise->video.visible_y_start;
    x0 = denise->video.visible_x_start;

    frame->width       = (rigel_u32)(denise->video.visible_x_stop - x0);
    frame->height      = (rigel_u32)(denise->video.visible_y_stop  - y0);
    frame->pitch       = (rigel_u32)(RIGEL_DENISE_MAX_SCANLINE_PIXELS * sizeof(rigel_u32));
    frame->frame_count = denise->output.frame_counter;
    frame->pixels      = (y0 < RIGEL_DENISE_MAX_LINES)
                             ? &denise->output.frame_rgba[y0][x0]
                             : &denise->output.frame_rgba[0][0];
    frame->flags       = (rigel_frame_flags_t)denise->output.completed_flags;
    frame->delta.full_redraw = false;
    (void)memcpy(frame->delta.dirty_lines, denise->output.completed_dirty,
                 sizeof(frame->delta.dirty_lines));
    return true;
}
