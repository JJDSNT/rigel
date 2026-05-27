#ifndef RIGEL_AGNUS_TIMING_DEADLINE_H
#define RIGEL_AGNUS_TIMING_DEADLINE_H

#include "rigel/rigel_types.h"

/* Per-subsystem deadline contributions for rigel_get_next_deadline().
 *
 * Each Agnus subsystem computes "how many cycles until my next observable
 * event?" and deposits it here. The aggregate minimum is returned by
 * rigel_get_next_deadline().
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
     * Source: agnus_cycles_to_vertb() in timing/vblank.h. */
    rigel_u32 vertb;

    /* Copper: cycles until the copper's armed WAIT target is reached.
     * Source: rigel_copper_domain_cycles_to_wait() in domains/copper. */
    rigel_u32 copper_wait;

    /* Audio: cycles until the next audio period expiry (sample output).
     * Source: audio_cycles_to_next_event() — min period_counter across active channels. */
    rigel_u32 audio;

    /* Disk: cycles until fake-DMA countdown completion IRQ.
     * Source: disk_cycles_to_next_event() — countdown field when in countdown phase. */
    rigel_u32 disk;

    /* Slot scheduler: cycles until the next non-CPU/non-FREE DMA slot. */
    rigel_u32 slot;
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
