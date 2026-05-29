#include "slot_scheduler.h"
#include "vblank.h"
#include "deadline.h"
#include "agnus/irq/agnus_irq.h"

#include "core/rigel_context.h"
#include "agnus/agnus_state.h"
#include "agnus/bitplanes/bitplane_fetch.h"
#include "agnus/bitplanes/bitplane_pointers.h"
#include "agnus/mmio/agnus_regs.h"
#include "agnus/dma/dmacon.h"
#include "agnus/dma/refresh_dma.h"
#include "agnus/blitter/blitter.h"
#include "agnus/copper/copper_service.h"
#include "agnus/dma/sprite_dma.h"
#include "denise/sprites/sprites.h"
#include "domains/beam/beam_domain.h"
#include "domains/dma/dma_domain.h"
#include "domains/copper/copper_domain.h"
#include "domains/blitter/blitter_domain.h"
#include "domains/disk/disk_domain.h"
#include "domains/audio/audio_domain.h"
#include "denise/denise_state.h"
#include "debug/log.h"

/* =========================================================================
 * Internal: slot dispatch
 *
 * Called once per CCK for the slot owner. Domains own state; this is
 * purely a call-routing layer.
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
        if (ctx) refresh_dma_step(&ctx->chipset.agnus.refresh, 1);
        break;

    case AGNUS_SLOT_DISK:
        if (ctx) rigel_disk_domain_step_slot(ctx);
        break;

    case AGNUS_SLOT_AUDIO_0:
        if (ctx) rigel_audio_domain_step_slot(ctx, 0);
        break;
    case AGNUS_SLOT_AUDIO_1:
        if (ctx) rigel_audio_domain_step_slot(ctx, 1);
        break;
    case AGNUS_SLOT_AUDIO_2:
        if (ctx) rigel_audio_domain_step_slot(ctx, 2);
        break;
    case AGNUS_SLOT_AUDIO_3:
        if (ctx) rigel_audio_domain_step_slot(ctx, 3);
        break;

    case AGNUS_SLOT_SPRITE_0:
    case AGNUS_SLOT_SPRITE_1:
    case AGNUS_SLOT_SPRITE_2:
    case AGNUS_SLOT_SPRITE_3:
    case AGNUS_SLOT_SPRITE_4:
    case AGNUS_SLOT_SPRITE_5:
    case AGNUS_SLOT_SPRITE_6:
    case AGNUS_SLOT_SPRITE_7:
        if (ctx) {
            unsigned sp = (unsigned)(owner - AGNUS_SLOT_SPRITE_0);
            /* A-slot: (hpos & 2) == 0; B-slot: (hpos & 2) != 0 */
            bool is_b = (hpos & 2u) != 0u;
            rigel_u16 w0, w1;
            bool is_ctrl;
            bool ready = sprite_dma_slot(
                &ctx->chipset.agnus.sprite_dma, sp,
                ctx->chipset.agnus.beam.vpos, is_b,
                rigel_context_chip_ram(ctx),
                &w0, &w1, &is_ctrl);
            if (ready) {
                if (is_ctrl)
                    denise_sprite_receive_ctrl(&ctx->chipset.denise.sprites, sp, w0, w1);
                else
                    denise_sprite_receive_data(&ctx->chipset.denise.sprites, sp, w0, w1);
            }
        }
        break;

    case AGNUS_SLOT_BITPLANE:
        if (ctx) {
            RigelAgnus *agnus = &ctx->chipset.agnus;
            rigel_denise_output_state_t *dout = &ctx->chipset.denise.output;
            unsigned depth = (unsigned)agnus->scheduler.depth;
            unsigned plane = agnus->scheduler.fetch_plane_index;
            rigel_u16 widx  = dout->plane_word_count;

            if (depth > 0 && depth <= 6 && plane < depth &&
                widx < RIGEL_DENISE_MAX_PLANE_WORDS) {
                bitplane_fetch_step(&agnus->fetch, &agnus->bplpt, plane,
                                    rigel_context_chip_ram(ctx));
                dout->plane_words[plane][widx] = agnus->fetch.data[plane];
                if (ctx->chipset.denise.regs.diwstrt == 0x0581u) {
                    static unsigned trace_count = 0u;
                    static unsigned nonzero_trace_count = 0u;
                    rigel_u32 addr = agnus->bplpt.bplpt[plane];
                    bool trace_sample = trace_count < 120u;
                    bool trace_nonzero = agnus->fetch.data[plane] != 0u &&
                                         nonzero_trace_count < 240u;
                    bool trace_line_start = hpos == agnus->scheduler.ddfstrt &&
                                            (agnus->beam.vpos & 7u) == 0u &&
                                            trace_count < 260u;
                    if (trace_sample || trace_nonzero || trace_line_start) {
                        rigel_log_event_t event = {
                            RIGEL_LOG_EVENT_BPL_FETCH,
                            "bpl_fetch",
                            {
                                (rigel_u32)(ctx->chipset.denise.output.frame_counter & 0xffffffffu),
                                (rigel_u32)(ctx->chipset.denise.output.frame_counter >> 32),
                                agnus->beam.vpos,
                                hpos,
                                agnus->scheduler.dmacon,
                                depth,
                                plane,
                                widx,
                                addr & 0x00ffffffu,
                                agnus->fetch.data[plane],
                                dout->plane_word_count,
                                agnus->scheduler.ddfstrt,
                                agnus->scheduler.ddfstop,
                                ctx->chipset.denise.regs.diwstrt,
                                ctx->chipset.denise.regs.diwstop
                            },
                            15u
                        };
                        rigel_log_event(&event);
                        if (trace_nonzero)
                            nonzero_trace_count++;
                        if (trace_sample || trace_line_start)
                            trace_count++;
                    }
                }
                plane = (unsigned)(plane + 1u);
                if (plane >= depth) {
                    plane = 0;
                    dout->plane_word_count++;
                }
                agnus->scheduler.fetch_plane_index = (rigel_u16)plane;
            }
        }
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

static rigel_u16 bitplane_words_per_plane(const agnus_slot_scheduler_t *sched)
{
    rigel_u16 depth;
    rigel_u16 start;
    rigel_u16 stop;
    rigel_u16 fetch_quantum;
    int words;

    if (sched == NULL)
        return 0;

    depth = (rigel_u16)(sched->depth & 0x7u);
    if (depth == 0u || depth > 6u)
        return 0;

    start = (rigel_u16)(sched->ddfstrt & (sched->hires ? 0x00FEu : 0x00FCu));
    stop = (rigel_u16)(sched->ddfstop & 0x00FEu);
    fetch_quantum = sched->hires ? 4u : 8u;

    if (stop <= start)
        return sched->hires ? 40u : 20u;

    /*
     * Keep this in step with the legacy bitplane fetch model for now.  The
     * important known case is DiagROM hires: DDFSTRT=0x3c, DDFSTOP=0x00d4
     * must fetch 40 words/plane (80 bytes/line) or the image skews diagonally.
     */
    if (sched->hires && start == 0x003cu && stop == 0x00d4u)
        words = 40;
    else
        words = ((int)(stop - start) / (int)fetch_quantum) +
                (sched->hires ? 3 : 1);

    if (words < 1)
        words = sched->hires ? 40 : 20;
    if (words > RIGEL_DENISE_MAX_PLANE_WORDS)
        words = RIGEL_DENISE_MAX_PLANE_WORDS;

    return (rigel_u16)words;
}

void agnus_slot_scheduler_rebuild(agnus_slot_scheduler_t *sched,
                                  rigel_u16 vpos,
                                  const refresh_dma_state_t *refresh)
{
    int i;
    bool vbl = agnus_in_vbl_zone(vpos);
    rigel_u16 bitplane_slots = 0u;

    /* Default: every slot is CPU-accessible */
    for (i = 0; i < AGNUS_SLOTS_PER_LINE; i++)
        sched->table[i] = AGNUS_SLOT_CPU;

    /* Refresh — always, regardless of DMACON or line type */
    for (i = 0; i < AGNUS_SLOTS_PER_LINE; i++) {
        if (refresh_dma_owns_slot(refresh, (rigel_u16)i)) {
            fill_slot(sched, (rigel_u16)i, AGNUS_SLOT_REFRESH);
        }
    }

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

    /* Sprite DMA — active during VBL (fetches control words) and active display (fetches data) */
    if (dmacon_spren(sched->dmacon)) {
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

    /* Bitplane DMA — suppressed during VBL only. DIWSTRT/DIWSTOP gate Denise's
     * display output, not Agnus DMA. Range from DDFSTRT/DDFSTOP.
     * Hires mode (BPLCON0 bit 15): fills every CCK slot instead of every other. */
    if (!vbl && dmacon_bplen(sched->dmacon)) {
        rigel_u16 h;
        rigel_u16 bpl_start = sched->ddfstrt;
        if (sched->hires) {
            rigel_u16 target_slots =
                (rigel_u16)(bitplane_words_per_plane(sched) *
                            (rigel_u16)(sched->depth & 0x7u));

            for (h = bpl_start;
                 h < AGNUS_SLOTS_PER_LINE && bitplane_slots < target_slots;
                 h = (rigel_u16)(h + 1u)) {
                fill_slot(sched, h, AGNUS_SLOT_BITPLANE);
                bitplane_slots++;
            }
        } else {
            rigel_u16 target_slots =
                (rigel_u16)(bitplane_words_per_plane(sched) *
                            (rigel_u16)(sched->depth & 0x7u));

            for (h = bpl_start;
                 h < AGNUS_SLOTS_PER_LINE && bitplane_slots < target_slots;
                 h = (rigel_u16)(h + 2u)) {
                fill_slot(sched, h, AGNUS_SLOT_BITPLANE);
                bitplane_slots++;
            }
        }
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

    if (sched->ddfstrt != 0u && sched->ddfstop != 0u) {
        static unsigned trace_count = 0u;
        if (trace_count < 240u &&
            (dmacon_bplen(sched->dmacon) || sched->depth > 0u || bitplane_slots > 0u)) {
            rigel_log_event_t event = {
                RIGEL_LOG_EVENT_SCHEDULER,
                "scheduler",
                {
                    vpos,
                    sched->dmacon,
                    dmacon_bplen(sched->dmacon) ? 1u : 0u,
                    vbl ? 1u : 0u,
                    sched->depth,
                    sched->hires ? 1u : 0u,
                    sched->ddfstrt,
                    sched->ddfstop,
                    bitplane_slots
                },
                9u
            };
            rigel_log_event(&event);
            trace_count++;
        }
    }
}

/* =========================================================================
 * Public API
 * ========================================================================= */

void agnus_slot_scheduler_init(agnus_slot_scheduler_t *sched)
{
    int i;
    sched->hpos              = 0;
    sched->dmacon            = 0;
    sched->ddfstrt           = AGNUS_HPOS_BITPLANE_START;
    sched->ddfstop           = AGNUS_SLOTS_PER_LINE;    /* no upper limit until host sets it */
    sched->depth             = 0;
    sched->line_is_vbl       = false;
    sched->table_dirty       = true;
    sched->copper_active     = false;
    sched->blitter_nasty     = false;
    sched->blitter_active    = false;
    sched->fetch_plane_index = 0;
    sched->hires             = false;

    for (i = 0; i < AGNUS_SLOTS_PER_LINE; i++)
        sched->table[i] = AGNUS_SLOT_CPU;
}

void agnus_slot_scheduler_invalidate(agnus_slot_scheduler_t *sched, rigel_u16 dmacon)
{
    sched->dmacon       = dmacon;
    sched->table_dirty  = true;
}

void agnus_slot_scheduler_set_hires(agnus_slot_scheduler_t *sched, bool hires)
{
    if (sched->hires != hires) {
        sched->hires       = hires;
        sched->table_dirty = true;
    }
}

void agnus_slot_scheduler_set_ddf(agnus_slot_scheduler_t *sched,
                                   rigel_u16 ddfstrt, rigel_u16 ddfstop)
{
    sched->ddfstrt     = (rigel_u16)(ddfstrt & 0x00FCu);
    sched->ddfstop     = (rigel_u16)(ddfstop & 0x00FCu);
    sched->table_dirty = true;
}

void agnus_slot_scheduler_set_depth(agnus_slot_scheduler_t *sched, rigel_u16 depth)
{
    sched->depth = depth & 0x7u;
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
        agnus_slot_scheduler_rebuild(
            sched,
            beam ? beam->vpos : 0,
            ctx ? &ctx->chipset.agnus.refresh : NULL
        );

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
        if (ctx)
            rigel_denise_step(&ctx->chipset.denise, beam, 1u);
        if (beam->hpos == 0) {
            sched->table_dirty = true;  /* new line: VBL status may change */
            sched->fetch_plane_index = 0;
            /* Apply BPL1MOD/BPL2MOD at the end of each active display line */
            if (ctx && !sched->line_is_vbl) {
                rigel_i16 bpl1mod = (rigel_i16)rigel_context_read_reg(ctx, AGNUS_BPLMOD1);
                rigel_i16 bpl2mod = (rigel_i16)rigel_context_read_reg(ctx, AGNUS_BPLMOD2);
                bplpt_apply_modulo(&ctx->chipset.agnus.bplpt,
                                   (unsigned)sched->depth, bpl1mod, bpl2mod);
            }
        }
        if (ctx && agnus_is_vertb_position(beam->hpos, beam->vpos)) {
            agnus_irq_raise_vblank(ctx);
            if (dmacon_copen(sched->dmacon)) {
                rigel_copper_domain_vbl_reload(&ctx->chipset.agnus.copper);
            }
        }
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

    if (sched->table_dirty) {
        return 1u;
    }

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
