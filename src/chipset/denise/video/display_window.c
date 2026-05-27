#include "denise/video/display_window.h"

void rigel_denise_display_window_reset(rigel_denise_video_state_t *video)
{
    if (video == NULL) {
        return;
    }

    video->width = 320;
    video->height = 256;
    video->visible_x_start = 0;
    video->visible_x_stop = 320;
    video->visible_y_start = 0;
    video->visible_y_stop = 256;
}

void rigel_denise_display_window_update(RigelDenise *denise)
{
    rigel_u16 hstrt, hstop, vstrt, vstop, width, height;

    if (denise == NULL) {
        return;
    }

    hstrt = (rigel_u16)(denise->regs.diwstrt & 0x00FFu);
    vstrt = (rigel_u16)((denise->regs.diwstrt >> 8) & 0x00FFu);
    /* DIWSTOP hpos is always in the second half of the scanline [256..511].
     * Only bits[7:0] are stored; bit 8 is implied 1, so add 256. */
    hstop = (rigel_u16)((denise->regs.diwstop & 0x00FFu) + 256u);
    vstop = (rigel_u16)((denise->regs.diwstop >> 8) & 0x00FFu);

    if (vstop <= vstrt) {
        vstop = (rigel_u16)(vstop + 256u);
    }

    if (hstop <= hstrt) {
        hstop = (rigel_u16)(hstop + 256u);
    }

    denise->video.visible_x_start = hstrt;
    denise->video.visible_y_start = vstrt;
    denise->video.visible_x_stop  = hstop;   /* exclusive */
    denise->video.visible_y_stop  = vstop;   /* exclusive */

    width  = (hstop > hstrt) ? (rigel_u16)(hstop - hstrt) : 320u;
    height = (vstop > vstrt) ? (rigel_u16)(vstop - vstrt) : 256u;

    denise->video.width  = width;
    denise->video.height = height;
}
