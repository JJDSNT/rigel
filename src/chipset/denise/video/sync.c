#include "sync.h"

/* HSYNC: lines 0-2 (approx), VSYNC: frames 0-3 lines (approx).
 * TODO(video): use precise hardware timings from the HRM. */

void sync_reset(sync_state_t *s)
{
    s->hsync_active = false;
    s->vsync_active = false;
    s->vblank       = false;
}

bool sync_update(sync_state_t *s, rigel_u16 hpos, rigel_u16 vpos,
                 rigel_u16 frame_lines, rigel_u16 line_clocks)
{
    bool vblank_started = false;

    s->hsync_active = (hpos >= line_clocks - 8);
    s->vsync_active = (vpos < 3);

    bool new_vblank = (vpos < 26);  /* approx. vblank region */
    if (new_vblank && !s->vblank)
        vblank_started = true;
    s->vblank = new_vblank;

    (void)frame_lines;
    return vblank_started;
}
