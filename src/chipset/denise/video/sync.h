#ifndef RIGEL_DENISE_SYNC_H
#define RIGEL_DENISE_SYNC_H

#include <stdbool.h>
#include "rigel/rigel_types.h"

/* Sync signal generation — HSYNC and VSYNC pulses.
 *
 * On the classic Amiga, Denise generates composite sync for the monitor.
 * For Rigel's purposes these signals matter for:
 *   - Triggering RIGEL_EVENT_VBLANK at the correct beam position
 *   - Providing the host with frame cadence
 *   - Future: genlock / external sync support
 *
 * Sync timings are fixed by the video standard (PAL/NTSC). */

typedef struct sync_state {
    bool hsync_active;
    bool vsync_active;
    bool vblank;
} sync_state_t;

void sync_reset(sync_state_t *s);

/* Update sync signals based on beam position; returns true if vblank just started. */
bool sync_update(sync_state_t *s, rigel_u16 hpos, rigel_u16 vpos,
                 rigel_u16 frame_lines, rigel_u16 line_clocks);

#endif
