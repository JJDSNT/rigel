#include "rigel/rigel_denise.h"

#include "core/rigel_context.h"

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
