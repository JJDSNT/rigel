#ifndef RIGEL_AGNUS_SLOT_SCHEDULER_H
#define RIGEL_AGNUS_SLOT_SCHEDULER_H

#include <stdbool.h>
#include "rigel/rigel_types.h"
#include "agnus/agnus_config.h"
#include "agnus/bitplanes/bitplane_pointers.h"
#include "agnus/dma/dmacon.h"
#include "agnus/dma/refresh_dma.h"

/* Agnus DMA slot scheduler — foundation of Approach C (dma_slot_timing.md).
 *
 * The Amiga chipset divides each scanline into 227 CCK (NTSC) fixed slots.
 * Agnus decides the owner of each slot deterministically from DMACON and beam
 * position. This is the authoritative source of:
 *
 *   - who has the chip bus at each CCK
 *   - when rigel_get_next_deadline() fires (next slot with a domain event)
 *   - whether the CPU would stall (bus state advisory)
 *
 * INTEGRATION PLAN (do not implement partially):
 *
 *   1. Beam model must be complete (line/frame wrapping, VBL zone). [timing/vblank.h]
 *   2. This scheduler becomes the inner loop of rigel_agnus_step():
 *        was:  rigel_agnus_step(ctx, cycles)  →  monolithic cycle advance
 *        will: agnus_slot_scheduler_step_until(sched, ctx, cycles)  →  slot loop
 *   3. Each domain gets a _step_slot() function called by dispatch_slot().
 *   4. rigel_get_next_deadline() delegates to agnus_slot_scheduler_next_event().
 *   5. rigel_get_bus_state() reads agnus_slot_scheduler_current_owner().
 *
 * OWNERSHIP RULE:
 *   The scheduler is orchestration only — it dispatches to domains.
 *   DMA state machines (counters, pointers, buffers) remain in their domains.
 *   The scheduler owns only the slot table and the current position. */

/* =========================================================================
 * Slot type enumeration
 * ========================================================================= */

typedef enum agnus_slot_owner {
    AGNUS_SLOT_FREE     = 0,  /* CPU or copper (if DMA enabled)            */
    AGNUS_SLOT_REFRESH,       /* chip RAM refresh — transparent, no domain */
    AGNUS_SLOT_DISK,          /* disk DMA → domains/disk                   */
    AGNUS_SLOT_AUDIO_0,       /* audio ch 0 → domains/audio                */
    AGNUS_SLOT_AUDIO_1,
    AGNUS_SLOT_AUDIO_2,
    AGNUS_SLOT_AUDIO_3,
    AGNUS_SLOT_SPRITE_0,      /* sprite DMA → agnus/dma/sprite_dma         */
    AGNUS_SLOT_SPRITE_1,
    AGNUS_SLOT_SPRITE_2,
    AGNUS_SLOT_SPRITE_3,
    AGNUS_SLOT_SPRITE_4,
    AGNUS_SLOT_SPRITE_5,
    AGNUS_SLOT_SPRITE_6,
    AGNUS_SLOT_SPRITE_7,
    AGNUS_SLOT_BITPLANE,      /* bitplane DMA → agnus/bitplanes/fetch      */
    AGNUS_SLOT_COPPER,        /* dynamic: copper steals FREE slots          */
    AGNUS_SLOT_BLITTER,       /* dynamic: blitter steals unused slots       */
    AGNUS_SLOT_CPU,           /* CPU owns this slot (no DMA contention)    */
} agnus_slot_owner_t;

/* =========================================================================
 * Hardware slot positions (CCK hpos within a scanline)
 *
 * Source: HRM Appendix A "DMA channel timing."
 * These are lores (lo-res) positions. Hires halves the bitplane fetch interval.
 * TODO(slot_scheduler): verify each position against hardware captures.
 * ========================================================================= */

enum {
    /* Refresh — chip RAM self-refresh, always present */
    AGNUS_HPOS_REFRESH_0   = 0x01,
    AGNUS_HPOS_REFRESH_1   = 0x03,
    AGNUS_HPOS_REFRESH_2   = 0x05,

    /* Disk DMA — two words per cycle, enabled by DSKEN */
    AGNUS_HPOS_DISK_0      = 0x07,   /* word 0 / sector header              */
    AGNUS_HPOS_DISK_1      = 0x09,   /* word 1                              */

    /* Audio DMA — one slot per channel, enabled per AUDxEN */
    AGNUS_HPOS_AUDIO_0     = 0x0B,
    AGNUS_HPOS_AUDIO_1     = 0x0D,
    AGNUS_HPOS_AUDIO_2     = 0x0F,
    AGNUS_HPOS_AUDIO_3     = 0x11,

    /* Sprite DMA — two slots per sprite (ctrl word + data words).
     * First slot = control (SPRxPOS/CTL), second = pixel data (SPRxDATA/DATB).
     * Only present on active scan lines (not VBL). Enabled by SPREN. */
    AGNUS_HPOS_SPRITE_0_A  = 0x15,   AGNUS_HPOS_SPRITE_0_B  = 0x17,
    AGNUS_HPOS_SPRITE_1_A  = 0x19,   AGNUS_HPOS_SPRITE_1_B  = 0x1B,
    AGNUS_HPOS_SPRITE_2_A  = 0x1D,   AGNUS_HPOS_SPRITE_2_B  = 0x1F,
    AGNUS_HPOS_SPRITE_3_A  = 0x21,   AGNUS_HPOS_SPRITE_3_B  = 0x23,
    AGNUS_HPOS_SPRITE_4_A  = 0x25,   AGNUS_HPOS_SPRITE_4_B  = 0x27,
    AGNUS_HPOS_SPRITE_5_A  = 0x29,   AGNUS_HPOS_SPRITE_5_B  = 0x2B,
    AGNUS_HPOS_SPRITE_6_A  = 0x2D,   AGNUS_HPOS_SPRITE_6_B  = 0x2F,
    AGNUS_HPOS_SPRITE_7_A  = 0x31,   AGNUS_HPOS_SPRITE_7_B  = 0x33,

    /* Bitplane DMA — begins at DDFSTRT, ends at DDFSTOP.
     * The exact start position depends on BPLCON0 (hires/lores) and DDFSTRT.
     * Only present on active scan lines within the vertical DIW. Enabled by BPLEN. */
    AGNUS_HPOS_BITPLANE_START = 0x35,
};

/* =========================================================================
 * Scheduler state
 * ========================================================================= */

typedef struct agnus_slot_scheduler {
    /* Static slot table — rebuilt when DMACON changes or line type changes.
     * One entry per CCK position. Indexed by hpos (0 .. AGNUS_SLOTS_PER_LINE-1). */
    agnus_slot_owner_t table[AGNUS_SLOTS_PER_LINE];
    rigel_u16 bitplane_slot_index[AGNUS_SLOTS_PER_LINE];

    /* Current position within the scanline */
    rigel_u16 hpos;

    /* Cached inputs used to build the current table */
    rigel_u16 dmacon;     /* DMACON value at last rebuild   */
    rigel_u16 ddfstrt;    /* bitplane fetch start hpos      */
    rigel_u16 ddfstop;    /* bitplane fetch stop hpos       */
    rigel_u16 depth;      /* BPU field from BPLCON0[14:12]  */
    bool      line_is_vbl; /* true = VBL line, sprite/bpl suppressed */

    /* Set when DMACON/DDF is written; clears after next rebuild */
    bool      table_dirty;

    /* Dynamic overrides applied per-step (not baked into table):
     *   copper_active  — copper may steal the next FREE slot
     *   blitter_nasty  — blitter also steals CPU slots (BLTCON0 bit 10) */
    bool      copper_active;
    bool      blitter_nasty;
    bool      blitter_active;

    /* Bitplane fetch cycling: which plane to fetch on the next BITPLANE slot */
    rigel_u16 fetch_plane_index;
    bool      bitplane_dma_this_line;
    rigel_u16 bitplane_words_this_line;
    rigel_u32 bitplane_line_base[BITPLANE_COUNT];
    bool      bitplane_line_base_valid;

    /* Hires mode (BPLCON0 bit 15): doubles bitplane fetch rate.
     * Must be kept in sync with BPLCON0 via agnus_slot_scheduler_set_hires(). */
    bool hires;

    /* Vertical DIW start line — bitplane DMA is suppressed for vpos < vstrt.
     * Updated via agnus_slot_scheduler_set_vstrt() when DIWSTRT is written. */
    rigel_u16 vstrt;
} agnus_slot_scheduler_t;

/* =========================================================================
 * API
 * ========================================================================= */

typedef struct RigelContext RigelContext;

void agnus_slot_scheduler_init(agnus_slot_scheduler_t *sched);

/* Call whenever DMACON is written — marks the table for rebuild. */
void agnus_slot_scheduler_invalidate(agnus_slot_scheduler_t *sched, rigel_u16 dmacon);

/* Call whenever DDFSTRT or DDFSTOP is written — marks the table for rebuild. */
void agnus_slot_scheduler_set_ddf(agnus_slot_scheduler_t *sched,
                                   rigel_u16 ddfstrt, rigel_u16 ddfstop);

/* Call whenever BPLCON0 BPU field [14:12] changes — updates cached depth. */
void agnus_slot_scheduler_set_depth(agnus_slot_scheduler_t *sched, rigel_u16 depth);

/* Call whenever BPLCON0 bit 15 (hires) changes — marks the table for rebuild. */
void agnus_slot_scheduler_set_hires(agnus_slot_scheduler_t *sched, bool hires);

/* Call whenever DIWSTRT is written — updates the vertical DIW start line. */
void agnus_slot_scheduler_set_vstrt(agnus_slot_scheduler_t *sched, rigel_u16 diwstrt);

/* Rebuild the slot table from the current DMACON and beam vpos.
 * Called automatically by step functions when table_dirty is set. */
void agnus_slot_scheduler_rebuild(agnus_slot_scheduler_t *sched,
                                  rigel_u16 vpos,
                                  const refresh_dma_state_t *refresh);

/* Step one CCK slot — dispatch to the owning domain, advance hpos.
 * Handles line wrap internally (hpos resets to 0 at line_clocks). */
void agnus_slot_scheduler_step(agnus_slot_scheduler_t *sched, RigelContext *ctx,
                               rigel_u16 line_clocks, rigel_u16 frame_lines);

/* Step until `cycles` CCKs have elapsed — the inner loop of rigel_agnus_step(). */
void agnus_slot_scheduler_step_until(agnus_slot_scheduler_t *sched, RigelContext *ctx,
                                     rigel_u32 cycles,
                                     rigel_u16 line_clocks, rigel_u16 frame_lines);

/* Return cycles until the next slot that carries a domain event.
 * Used by rigel_get_next_deadline(). Returns AGNUS_DEADLINE_UNKNOWN if none. */
rigel_u32 agnus_slot_scheduler_next_event(const agnus_slot_scheduler_t *sched,
                                           rigel_u16 line_clocks);

/* Return the owner of the current slot — for rigel_get_bus_state(). */
agnus_slot_owner_t agnus_slot_scheduler_current_owner(const agnus_slot_scheduler_t *sched);

/* True if the CPU would be denied the bus at the current slot
 * (owner != FREE && owner != CPU, or blitter_nasty with blitter active). */
bool agnus_slot_scheduler_cpu_stall(const agnus_slot_scheduler_t *sched);

#endif
