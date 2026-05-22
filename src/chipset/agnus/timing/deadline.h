#ifndef RIGEL_AGNUS_TIMING_DEADLINE_H
#define RIGEL_AGNUS_TIMING_DEADLINE_H

#include "rigel/rigel_types.h"

/* Per-subsystem deadline contributions for rigel_get_next_deadline().
 *
 * The current implementation in core/rigel.c contains:
 *   "TODO: each domain should expose its next wake time; aggregate here"
 *
 * This header is the resolution of that TODO. Each Agnus subsystem computes
 * "how many cycles until my next observable event?" and deposits it here.
 * The aggregate minimum becomes the return value of rigel_get_next_deadline().
 *
 * AGNUS_DEADLINE_UNKNOWN means the subsystem has no pending event and should
 * not constrain the deadline. The aggregate ignores UNKNOWN entries. */

#define AGNUS_DEADLINE_UNKNOWN  ((rigel_u32)0xFFFFFFFFu)

typedef struct agnus_deadlines {
    /* Blitter: cycles_remaining until blitter finishes.
     * Source: BlitterState.cycles_remaining (already exists). */
    rigel_u32 blitter;

    /* Beam: cycles until end of current scanline (line boundary event).
     * Source: beam_cycles_until_line_end() (already exists). */
    rigel_u32 beam_line_end;

    /* VERTB: cycles until the next vertical blank IRQ trigger point (vpos=0, hpos=1).
     * Source: agnus_cycles_to_vertb() in timing/vblank.h.
     * TODO: wire up in core/rigel.c once vblank.h is integrated. */
    rigel_u32 vertb;

    /* Copper: cycles until the copper's armed WAIT target is reached.
     * Source: copper_domain — needs a "cycles_to_wait_target(beam, copper)" helper.
     * TODO: implement in domains/copper once beam model is complete. */
    rigel_u32 copper_wait;

    /* Audio: cycles until the next audio DMA slot grant (period expiry).
     * Source: audio domain per-channel period counters.
     * TODO: implement in domains/audio. */
    rigel_u32 audio;

    /* Disk: cycles until the next disk DMA word transfer.
     * Source: disk domain DMA active flag + MFM bit rate.
     * TODO: implement in domains/disk. */
    rigel_u32 disk;
} agnus_deadlines_t;

/* Initialize all entries to UNKNOWN. */
void agnus_deadlines_reset(agnus_deadlines_t *d);

/* Return the minimum non-UNKNOWN deadline, or AGNUS_DEADLINE_UNKNOWN if all unknown. */
rigel_u32 agnus_deadlines_min(const agnus_deadlines_t *d);

/* Convenience: clamp a candidate to the current minimum (for incremental updates). */
static inline void agnus_deadlines_update(agnus_deadlines_t *d,
                                          rigel_u32 *field,
                                          rigel_u32 cycles)
{
    (void)d;
    if (cycles < *field)
        *field = cycles;
}

#endif
