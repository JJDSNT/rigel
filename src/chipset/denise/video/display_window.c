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
    rigel_u16 hwidth;
    rigel_u16 hscale;
    bool hires;
    bool ocs_diw;

    if (denise == NULL) {
        return;
    }

    /*
     * BPU=0 disables bitplane display.  Some software briefly clears BPU while
     * rewriting DIW/DDF/BPLCON0 during a visible boot animation; do not let
     * that transient blanking state resize the public frame geometry.  The
     * latest DIW/DIWHIGH values remain latched and will be applied as soon as
     * bitplanes are enabled again.
     */
    if (denise->regs.bplcon0 != 0u &&
        ((denise->regs.bplcon0 >> 12) & 0x7u) == 0u &&
        denise->video.width != 0u &&
        denise->video.height != 0u) {
        return;
    }

    ocs_diw = !(denise->chip_rev == AGNUS_REV_ECS && denise->regs.diwhigh != 0u);

    if (!ocs_diw) {
        rigel_u16 start_hi = (rigel_u16)(denise->regs.diwhigh & 0x00FFu);
        rigel_u16 stop_hi  = (rigel_u16)((denise->regs.diwhigh >> 8) & 0x00FFu);

        hstrt = (rigel_u16)((denise->regs.diwstrt & 0x00FFu) |
                            (((start_hi >> 3) & 0x7u) << 8));
        vstrt = (rigel_u16)(((denise->regs.diwstrt >> 8) & 0x00FFu) |
                            ((start_hi & 0x7u) << 8));
        hstop = (rigel_u16)((denise->regs.diwstop & 0x00FFu) |
                            (((stop_hi >> 3) & 0x7u) << 8));
        vstop = (rigel_u16)(((denise->regs.diwstop >> 8) & 0x00FFu) |
                            ((stop_hi & 0x7u) << 8));
    } else {
        hstrt = (rigel_u16)(denise->regs.diwstrt & 0x00FFu);
        vstrt = (rigel_u16)((denise->regs.diwstrt >> 8) & 0x00FFu);
        /* OCS DIWSTOP hpos is in the second half of the scanline [256..511].
         * Only bits[7:0] are stored; bit 8 is implied 1, so add 256. */
        hstop = (rigel_u16)((denise->regs.diwstop & 0x00FFu) + 256u);
        vstop = (rigel_u16)((denise->regs.diwstop >> 8) & 0x00FFu);
    }

    if (vstop <= vstrt) {
        vstop = (rigel_u16)(vstop + 256u);
    }

    if (hstop <= hstrt) {
        hstop = (rigel_u16)(hstop + 256u);
    }

    /*
     * Some ECS-era software touches DIWHIGH while still programming a normal
     * OCS-sized horizontal DIW.  Until programmable wide beams are modelled,
     * reject horizontal windows wider than the internal scanline buffer can
     * represent and fall back to the OCS hstop encoding.  This keeps
     * DIW=2c81/2cc1 hires at 640 pixels instead of producing impossible
     * frame widths such as 2176 pixels.
     */
    if (!ocs_diw && hstop > hstrt && (rigel_u16)(hstop - hstrt) > 512u) {
        hstrt = (rigel_u16)(denise->regs.diwstrt & 0x00FFu);
        hstop = (rigel_u16)((denise->regs.diwstop & 0x00FFu) + 256u);
        if (hstop <= hstrt) {
            hstop = (rigel_u16)(hstop + 256u);
        }
    }

    /* Workbench 1.3 can program DIWSTRT/DIWSTOP as 057e/40be.  A raw OCS
     * vertical decode gives only 59 lines, but the intended display is the
     * normal 256-line field that wraps through the next 8-bit VSTOP range.
     * ECS machines still need this path when old software does not program
     * DIWHIGH and therefore uses the OCS DIW encoding. */
    if (ocs_diw &&
        (rigel_u16)(vstop - vstrt) < 128u &&
        vstrt < 32u) {
        vstop = (rigel_u16)(vstrt + 256u);
    }

    /*
     * Guard: if the computed vertical window falls entirely outside the valid
     * raster range, the register values are a VBL "blanking" sequence (e.g.
     * the copper writing DIWSTRT=0xffff before clearing BPLCON0 depth).
     * Preserve the previous valid window state so the display keeps showing
     * the last good frame rather than flickering through invalid geometry.
     */
    if (vstrt >= (rigel_u16)RIGEL_DENISE_MAX_LINES ||
        vstop > (rigel_u16)RIGEL_DENISE_MAX_LINES)
        return;

    hires = (denise->regs.bplcon0 & 0x8000u) != 0u;
    hscale = hires ? 2u : 1u;
    hwidth = (hstop > hstrt) ? (rigel_u16)(hstop - hstrt) : 320u;

    denise->video.visible_x_start = (rigel_u16)(hstrt * hscale);
    denise->video.visible_y_start = vstrt;
    denise->video.visible_x_stop  =
        (rigel_u16)(denise->video.visible_x_start + (hwidth * hscale)); /* exclusive */
    denise->video.visible_y_stop  = vstop;   /* exclusive */

    width  = (rigel_u16)(hwidth * hscale);
    height = (vstop > vstrt) ? (rigel_u16)(vstop - vstrt) : 256u;

    denise->video.width  = width;
    denise->video.height = height;
}
