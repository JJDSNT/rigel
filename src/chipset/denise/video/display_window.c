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
    rigel_u16 width;
    rigel_u16 height;

    if (denise == NULL) {
        return;
    }

    rigel_u16 vstop_raw;
    rigel_u16 vstop;

    denise->video.visible_x_start = (rigel_u16)(denise->regs.diwstrt & 0x00ffu);
    denise->video.visible_y_start = (rigel_u16)((denise->regs.diwstrt >> 8) & 0x00ffu);
    denise->video.visible_x_stop  = (rigel_u16)(denise->regs.diwstop & 0x00ffu);

    /* OCS Agnus extends DIWSTOP vertical byte: bit 7 clear → bit 8 is set (line 256+).
     * This makes WB1.3 DIWSTOP vpos=0x2C decode to line 300 (0x12C) rather than 44. */
    vstop_raw = (rigel_u16)((denise->regs.diwstop >> 8) & 0xFFu);
    vstop = (vstop_raw & 0x80u) ? vstop_raw : (rigel_u16)(vstop_raw | 0x100u);
    denise->video.visible_y_stop = vstop;

    width = 320;
    if (denise->video.visible_x_stop >= denise->video.visible_x_start) {
        width = (rigel_u16)(denise->video.visible_x_stop - denise->video.visible_x_start + 1u);
    }

    height = 256;
    if (denise->video.visible_y_stop >= denise->video.visible_y_start) {
        height = (rigel_u16)(denise->video.visible_y_stop - denise->video.visible_y_start + 1u);
    }

    denise->video.width = width;
    denise->video.height = height;
}
