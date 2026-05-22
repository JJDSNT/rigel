#include "slot_scheduler.h"
#include "vblank.h"
#include "deadline.h"

#include "core/rigel_context.h"
#include "agnus/agnus_state.h"
#include "agnus/dma/dmacon.h"
#include "agnus/blitter/blitter.h"
#include "agnus/copper/copper_service.h"
#include "agnus/dma/sprite_dma.h"
#include "domains/beam/beam_domain.h"
#include "domains/dma/dma_domain.h"
#include "domains/copper/copper_domain.h"
#include "domains/blitter/blitter_domain.h"
#include "domains/disk/disk_domain.h"
#include "domains/audio/audio_domain.h"

/* =========================================================================
 * Internal: slot dispatch
 *
 * Called once per CCK for the slot owner. Domains own state; this is
 * purely a call-routing layer. All TODOs here point at missing domain
 * slot-step functions that need to be added as Approach C is implemented.
 * ========================================================================= */

static void dispatch_slot(agnus_slot_owner_t owner,
                          RigelContext *ctx,
                          rigel_u16 hpos)
{
    (void)hpos;

    switch (owner) {

    case AGNUS_SLOT_FREE:
    case AGNUS_SLOT_CPU:
        /* CPU has the bus — no DMA action. */
        break;

    case AGNUS_SLOT_REFRESH:
        /* Chip RAM self-refresh — transparent to software, no domain call. */
        break;

    case AGNUS_SLOT_DISK:
        /* TODO(slot_scheduler): rigel_disk_domain_step_slot(ctx) */
        break;

    case AGNUS_SLOT_AUDIO_0:
        /* TODO(slot_scheduler): rigel_audio_domain_step_slot(ctx, 0) */
        break;
    case AGNUS_SLOT_AUDIO_1:
        /* TODO(slot_scheduler): rigel_audio_domain_step_slot(ctx, 1) */
        break;
    case AGNUS_SLOT_AUDIO_2:
        /* TODO(slot_scheduler): rigel_audio_domain_step_slot(ctx, 2) */
        break;
    case AGNUS_SLOT_AUDIO_3:
        /* TODO(slot_scheduler): rigel_audio_domain_step_slot(ctx, 3) */
        break;

    case AGNUS_SLOT_SPRITE_0:
        /* TODO(slot_scheduler): agnus_sprite_dma_step(&ctx->chipset.agnus, 0, ...) */
        break;
    case AGNUS_SLOT_SPRITE_1:
        /* TODO(slot_scheduler): agnus_sprite_dma_step(&ctx->chipset.agnus, 1, ...) */
        break;
    case AGNUS_SLOT_SPRITE_2:
    case AGNUS_SLOT_SPRITE_3:
    case AGNUS_SLOT_SPRITE_4:
    case AGNUS_SLOT_SPRITE_5:
    case AGNUS_SLOT_SPRITE_6:
    case AGNUS_SLOT_SPRITE_7:
        /* TODO(slot_scheduler): sprite dispatch for channels 2-7 */
        break;

    case AGNUS_SLOT_BITPLANE:
        /* TODO(slot_scheduler): agnus_bitplane_fetch_step(ctx, plane_index) */
        break;

    case AGNUS_SLOT_COPPER:
        if (ctx) {
            rigel_copper_domain_step(
                &ctx->chipset.agnus.copper,
                &ctx->chipset.agnus.beam,
                &ctx->chipset.agnus.dma
            );
            rigel_copper_service_step_program(ctx);
        }
        break;

    case AGNUS_SLOT_BLITTER:
        if (ctx) {
            rigel_agnus_blitter_step_dma(ctx, 1);
        }
        break;
    }
}

/* =========================================================================
 * Table builder
 *
 * Assigns static slot owners from DMACON and line type.
 * Dynamic overrides (copper stealing FREE, blitter-nasty) are applied
 * per-step in agnus_slot_scheduler_step(), not baked here.
 * ========================================================================= */

static void fill_slot(agnus_slot_scheduler_t *sched,
                      rigel_u16 hpos, agnus_slot_owner_t owner)
{
    if (hpos < AGNUS_SLOTS_PER_LINE)
        sched->table[hpos] = owner;
}

void agnus_slot_scheduler_rebuild(agnus_slot_scheduler_t *sched, rigel_u16 vpos)
{
    int i;
    bool vbl = agnus_in_vbl_zone(vpos);

    /* Default: every slot is CPU-accessible */
    for (i = 0; i < AGNUS_SLOTS_PER_LINE; i++)
        sched->table[i] = AGNUS_SLOT_CPU;

    /* Refresh — always, regardless of DMACON or line type */
    fill_slot(sched, AGNUS_HPOS_REFRESH_0, AGNUS_SLOT_REFRESH);
    fill_slot(sched, AGNUS_HPOS_REFRESH_1, AGNUS_SLOT_REFRESH);
    fill_slot(sched, AGNUS_HPOS_REFRESH_2, AGNUS_SLOT_REFRESH);

    /* Disk DMA */
    if (dmacon_dsken(sched->dmacon)) {
        fill_slot(sched, AGNUS_HPOS_DISK_0, AGNUS_SLOT_DISK);
        fill_slot(sched, AGNUS_HPOS_DISK_1, AGNUS_SLOT_DISK);
    }

    /* Audio DMA */
    if (dmacon_auden(sched->dmacon, 0)) fill_slot(sched, AGNUS_HPOS_AUDIO_0, AGNUS_SLOT_AUDIO_0);
    if (dmacon_auden(sched->dmacon, 1)) fill_slot(sched, AGNUS_HPOS_AUDIO_1, AGNUS_SLOT_AUDIO_1);
    if (dmacon_auden(sched->dmacon, 2)) fill_slot(sched, AGNUS_HPOS_AUDIO_2, AGNUS_SLOT_AUDIO_2);
    if (dmacon_auden(sched->dmacon, 3)) fill_slot(sched, AGNUS_HPOS_AUDIO_3, AGNUS_SLOT_AUDIO_3);

    /* Sprite DMA — suppressed during VBL */
    if (!vbl && dmacon_spren(sched->dmacon)) {
        fill_slot(sched, AGNUS_HPOS_SPRITE_0_A, AGNUS_SLOT_SPRITE_0);
        fill_slot(sched, AGNUS_HPOS_SPRITE_0_B, AGNUS_SLOT_SPRITE_0);
        fill_slot(sched, AGNUS_HPOS_SPRITE_1_A, AGNUS_SLOT_SPRITE_1);
        fill_slot(sched, AGNUS_HPOS_SPRITE_1_B, AGNUS_SLOT_SPRITE_1);
        fill_slot(sched, AGNUS_HPOS_SPRITE_2_A, AGNUS_SLOT_SPRITE_2);
        fill_slot(sched, AGNUS_HPOS_SPRITE_2_B, AGNUS_SLOT_SPRITE_2);
        fill_slot(sched, AGNUS_HPOS_SPRITE_3_A, AGNUS_SLOT_SPRITE_3);
        fill_slot(sched, AGNUS_HPOS_SPRITE_3_B, AGNUS_SLOT_SPRITE_3);
        fill_slot(sched, AGNUS_HPOS_SPRITE_4_A, AGNUS_SLOT_SPRITE_4);
        fill_slot(sched, AGNUS_HPOS_SPRITE_4_B, AGNUS_SLOT_SPRITE_4);
        fill_slot(sched, AGNUS_HPOS_SPRITE_5_A, AGNUS_SLOT_SPRITE_5);
        fill_slot(sched, AGNUS_HPOS_SPRITE_5_B, AGNUS_SLOT_SPRITE_5);
        fill_slot(sched, AGNUS_HPOS_SPRITE_6_A, AGNUS_SLOT_SPRITE_6);
        fill_slot(sched, AGNUS_HPOS_SPRITE_6_B, AGNUS_SLOT_SPRITE_6);
        fill_slot(sched, AGNUS_HPOS_SPRITE_7_A, AGNUS_SLOT_SPRITE_7);
        fill_slot(sched, AGNUS_HPOS_SPRITE_7_B, AGNUS_SLOT_SPRITE_7);
    }

    /* Bitplane DMA — suppressed during VBL.
     * TODO(slot_scheduler): derive active range from DDFSTRT/DDFSTOP + BPLCON0 hires bit.
     * Currently uses a fixed approximation starting at AGNUS_HPOS_BITPLANE_START. */
    if (!vbl && dmacon_bplen(sched->dmacon)) {
        rigel_u16 h;
        for (h = AGNUS_HPOS_BITPLANE_START; h < AGNUS_SLOTS_PER_LINE - 4u; h += 2u)
            fill_slot(sched, h, AGNUS_SLOT_BITPLANE);
    }

    /* Non-CPU slots not assigned above become FREE (copper/blitter eligible).
     * Copper and blitter dynamic assignment is handled in agnus_slot_scheduler_step(). */
    for (i = 0; i < AGNUS_SLOTS_PER_LINE; i++) {
        if (sched->table[i] == AGNUS_SLOT_CPU) {
            /* Mark as FREE only if master DMA is enabled; otherwise CPU keeps it */
            if (dmacon_master(sched->dmacon))
                sched->table[i] = AGNUS_SLOT_FREE;
        }
    }

    sched->line_is_vbl  = vbl;
    sched->table_dirty  = false;
}

/* =========================================================================
 * Public API
 * ========================================================================= */

void agnus_slot_scheduler_init(agnus_slot_scheduler_t *sched)
{
    int i;
    sched->hpos          = 0;
    sched->dmacon        = 0;
    sched->line_is_vbl   = false;
    sched->table_dirty   = true;
    sched->copper_active = false;
    sched->blitter_nasty = false;
    sched->blitter_active = false;

    for (i = 0; i < AGNUS_SLOTS_PER_LINE; i++)
        sched->table[i] = AGNUS_SLOT_CPU;
}

void agnus_slot_scheduler_invalidate(agnus_slot_scheduler_t *sched, rigel_u16 dmacon)
{
    sched->dmacon       = dmacon;
    sched->table_dirty  = true;
}

void agnus_slot_scheduler_step(agnus_slot_scheduler_t *sched, RigelContext *ctx,
                               rigel_u16 line_clocks, rigel_u16 frame_lines)
{
    agnus_slot_owner_t owner;
    beam_state_t *beam = ctx ? &ctx->chipset.agnus.beam : NULL;
    rigel_u16 hpos;

    /* Derive dynamic flags from live state so queries stay accurate */
    if (ctx) {
        sched->blitter_active = blitter_is_busy(&ctx->chipset.agnus.blitter) != 0;
        sched->copper_active  = dmacon_copen(sched->dmacon);
        sched->blitter_nasty  = (sched->dmacon & DMACON_BLTPRI) != 0;
        sched->hpos           = beam->hpos;
        line_clocks           = beam->line_clocks;
    }

    /* Rebuild slot table if DMACON changed or a new line started */
    if (sched->table_dirty)
        agnus_slot_scheduler_rebuild(sched, beam ? beam->vpos : 0);

    hpos  = sched->hpos;
    owner = sched->table[hpos < AGNUS_SLOTS_PER_LINE ? hpos : 0];

    /* Dynamic slot overrides:
     *   Blitter steals FREE slots when active; also CPU slots when nasty.
     *   Copper steals FREE slots only when blitter is not competing. */
    if (sched->blitter_active) {
        if (owner == AGNUS_SLOT_FREE ||
            (sched->blitter_nasty && owner == AGNUS_SLOT_CPU))
            owner = AGNUS_SLOT_BLITTER;
    } else if (sched->copper_active && owner == AGNUS_SLOT_FREE) {
        owner = AGNUS_SLOT_COPPER;
    }

    dispatch_slot(owner, ctx, hpos);

    /* Advance beam by one CCK (beam is the canonical position; scheduler follows) */
    if (beam) {
        rigel_beam_domain_step(beam, 1);
        sched->hpos = beam->hpos;
        if (beam->hpos == 0)
            sched->table_dirty = true;  /* new line: VBL status may change */
    } else {
        (void)frame_lines;
        hpos = (rigel_u16)(hpos + 1u);
        if (hpos >= line_clocks) {
            hpos = 0;
            sched->table_dirty = true;
        }
        sched->hpos = hpos;
    }
}

void agnus_slot_scheduler_step_until(agnus_slot_scheduler_t *sched, RigelContext *ctx,
                                     rigel_u32 cycles,
                                     rigel_u16 line_clocks, rigel_u16 frame_lines)
{
    rigel_u32 i;
    for (i = 0; i < cycles; i++)
        agnus_slot_scheduler_step(sched, ctx, line_clocks, frame_lines);
}

rigel_u32 agnus_slot_scheduler_next_event(const agnus_slot_scheduler_t *sched,
                                           rigel_u16 line_clocks)
{
    rigel_u16 h;

    for (h = sched->hpos + 1u; h < line_clocks; h++) {
        agnus_slot_owner_t o = sched->table[h];
        if (o != AGNUS_SLOT_CPU && o != AGNUS_SLOT_FREE)
            return (rigel_u32)(h - sched->hpos);
    }

    /* No event before end of line — deadline is the line boundary */
    return (rigel_u32)(line_clocks - sched->hpos);
}

agnus_slot_owner_t agnus_slot_scheduler_current_owner(const agnus_slot_scheduler_t *sched)
{
    agnus_slot_owner_t o;

    if (sched->hpos >= AGNUS_SLOTS_PER_LINE)
        return AGNUS_SLOT_CPU;

    o = sched->table[sched->hpos];

    if (sched->blitter_active) {
        if (o == AGNUS_SLOT_FREE || (sched->blitter_nasty && o == AGNUS_SLOT_CPU))
            return AGNUS_SLOT_BLITTER;
    } else if (sched->copper_active && o == AGNUS_SLOT_FREE) {
        return AGNUS_SLOT_COPPER;
    }

    return o;
}

bool agnus_slot_scheduler_cpu_stall(const agnus_slot_scheduler_t *sched)
{
    agnus_slot_owner_t o = agnus_slot_scheduler_current_owner(sched);
    return o != AGNUS_SLOT_CPU && o != AGNUS_SLOT_FREE;
}
