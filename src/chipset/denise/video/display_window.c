#include "denise/video/display_window.h"

void rigel_denise_display_window_reset(rigel_denise_video_state_t *video)
{
    if (video == NULL) {
        return;
    }

    video->width = 320;
    video->height = 256;
    video->visible_x_start = 0;
    video->visible_x_stop = 319;
    video->visible_y_start = 0;
    video->visible_y_stop = 255;
}

void rigel_denise_display_window_update(RigelDenise *denise)
{
    if (denise == NULL) {
        return;
    }

    denise->video.visible_x_start = (rigel_u16)(denise->regs.diwstrt & 0x00ffu);
    denise->video.visible_y_start = (rigel_u16)((denise->regs.diwstrt >> 8) & 0x00ffu);
    denise->video.visible_x_stop = (rigel_u16)(denise->regs.diwstop & 0x00ffu);
    denise->video.visible_y_stop = (rigel_u16)((denise->regs.diwstop >> 8) & 0x00ffu);
}
