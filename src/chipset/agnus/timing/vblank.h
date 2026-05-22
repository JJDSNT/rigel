#ifndef RIGEL_AGNUS_TIMING_VBLANK_H
#define RIGEL_AGNUS_TIMING_VBLANK_H

#include <stdbool.h>
#include "rigel/rigel_types.h"
#include "agnus/beam.h"

/* Vertical blank zone — Agnus timing model.
 *
 * Two distinct concepts that the codebase currently conflates:
 *
 *   a) Denise display window (DIWSTRT/DIWSTOP) — when Denise outputs pixels.
 *      `visible_y_start/stop` in `beam_state_t` reflects this.
 *
 *   b) Agnus VBL zone — when Agnus suppresses bitplane/sprite DMA and raises VERTB.
 *      This is a fixed hardware zone, NOT derived from DIWSTOP.
 *
 * On NTSC (262 lines): VBL zone = lines 0–25 (26 lines).
 *                       VERTB IRQ fires at vpos=0, hpos=1.
 * On PAL  (312 lines): VBL zone = lines 0–25 (same 26 lines).
 *                       VERTB IRQ fires at vpos=0, hpos=1.
 *
 * The current `beam_in_vblank()` returns (vpos > visible_y_stop) which is the
 * end-of-frame region, not the start-of-frame VBL zone where VERTB fires.
 * This header provides the correct Agnus-side VBL model. */

enum {
    AGNUS_VBL_LINE_START  = 0,    /* first VBL line (start of new frame)    */
    AGNUS_VBL_LINE_END    = 25,   /* last VBL line; active DMA begins at 26 */
    AGNUS_VERTB_VPOS      = 0,    /* vpos where VERTB IRQ fires             */
    AGNUS_VERTB_HPOS      = 1,    /* hpos where VERTB IRQ fires             */
};

/* Returns true if `vpos` is inside the VBL zone (Agnus DMA inhibit + VERTB territory).
 * This is independent of the Denise display window. */
static inline bool agnus_in_vbl_zone(rigel_u16 vpos)
{
    return vpos <= AGNUS_VBL_LINE_END;
}

/* Returns true if this is exactly the beam position where VERTB IRQ should fire. */
static inline bool agnus_is_vertb_position(rigel_u16 hpos, rigel_u16 vpos)
{
    return vpos == AGNUS_VERTB_VPOS && hpos == AGNUS_VERTB_HPOS;
}

/* Returns true if DMA channels that are suppressed during VBL (bitplanes, sprites)
 * should be inhibited at the given `vpos`. Disk, audio, copper, and blitter continue
 * to run during VBL. */
static inline bool agnus_vbl_inhibits_dma(rigel_u16 vpos)
{
    return vpos <= AGNUS_VBL_LINE_END;
}

/* Compute cycles from the current beam position to the next VERTB trigger.
 * Returns 0 if the beam is already at the VERTB position. */
rigel_u32 agnus_cycles_to_vertb(const beam_state_t *beam);

#endif
