#include "raster.h"
#include "agnus/agnus_config.h"

void raster_reset(raster_config_t *r, agnus_video_std_t std)
{
    r->std        = std;
    r->line_clocks = AGNUS_NTSC_LINE_CLOCKS;   /* same for PAL and NTSC in OCS */
    r->frame_lines = (std == AGNUS_VIDEO_PAL)
                     ? AGNUS_PAL_FRAME_LINES
                     : AGNUS_NTSC_FRAME_LINES;
    r->diwstrt = 0;
    r->diwstop = 0;
    r->ddfstrt = 0;
    r->ddfstop = 0;
}

void raster_set_diwstrt(raster_config_t *r, rigel_u16 val) { r->diwstrt = val; }
void raster_set_diwstop(raster_config_t *r, rigel_u16 val) { r->diwstop = val; }
void raster_set_ddfstrt(raster_config_t *r, rigel_u16 val) { r->ddfstrt = val; }
void raster_set_ddfstop(raster_config_t *r, rigel_u16 val) { r->ddfstop = val; }

/* Is (hpos, vpos) inside the display window defined by DIWSTRT/DIWSTOP?
 *
 * OCS DIWSTRT: bits[15:8] = VSTART, bits[7:0] = HSTART
 * OCS DIWSTOP: bits[15:8] = VSTOP,  bits[7:0] = HSTOP
 *
 * Horizontal: HSTART is 8-bit (0x00–0xFF).  HSTOP is also 8-bit but wraps:
 * if HSTOP < 0x80 the real stop is HSTOP + 0x100, covering the right side of
 * the screen that lies above 0xFF in CCK coordinates. */
bool raster_in_display_window(const raster_config_t *r, rigel_u16 hpos, rigel_u16 vpos)
{
    rigel_u16 vstart    = (r->diwstrt >> 8) & 0xFFu;
    rigel_u16 vstop     = (r->diwstop >> 8) & 0xFFu;
    rigel_u16 hstart    =  r->diwstrt & 0xFFu;
    rigel_u16 hstop_raw =  r->diwstop & 0xFFu;
    rigel_u16 hstop     = (hstop_raw < 0x80u) ? (hstop_raw + 0x100u) : hstop_raw;

    if (vpos < vstart || vpos >= vstop) return false;
    return hpos >= hstart && hpos < hstop;
}

/* Is hpos inside the bitplane data-fetch window (DDFSTRT..DDFSTOP)? */
bool raster_in_fetch_window(const raster_config_t *r, rigel_u16 hpos)
{
    rigel_u16 h = hpos & 0x00FEu;
    return h >= (r->ddfstrt & 0x00FEu) && h <= (r->ddfstop & 0x00FEu);
}
