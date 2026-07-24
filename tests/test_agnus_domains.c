#include "agnus/agnus_state.h"
#include "agnus/blitter/blitter.h"
#include "agnus/mmio/agnus_regs.h"
#include "agnus/timing/slot_scheduler.h"
#include "chipset/chipset.h"
#include "core/rigel_context.h"
#include "rigel/rigel.h"

#include <stdio.h>

static unsigned count_bitplane_slots(const agnus_slot_scheduler_t *sched)
{
    unsigned count = 0;

    for (unsigned i = 0; i < AGNUS_SLOTS_PER_LINE; ++i) {
        if (sched->table[i] == AGNUS_SLOT_BITPLANE) {
            count++;
        }
    }

    return count;
}

static int test_cpu_resume_targets_first_free_slot(void)
{
    agnus_slot_scheduler_t sched = { 0 };

    for (unsigned i = 0; i < AGNUS_SLOTS_PER_LINE; ++i)
        sched.table[i] = AGNUS_SLOT_CPU;

    sched.hpos = 20u;
    sched.table[20] = AGNUS_SLOT_BITPLANE;
    sched.table[21] = AGNUS_SLOT_SPRITE_0;
    sched.table[22] = AGNUS_SLOT_FREE;
    sched.table[23] = AGNUS_SLOT_BITPLANE;

    if (!agnus_slot_scheduler_cpu_stall(&sched) ||
        agnus_slot_scheduler_cpu_resume_in(&sched,
                                           AGNUS_SLOTS_PER_LINE) != 2u) {
        fprintf(stderr, "CPU resume did not target first free slot\n");
        return 1;
    }

    sched.hpos = 22u;
    sched.copper_active = true;
    sched.copper_request = false;
    if (agnus_slot_scheduler_cpu_stall(&sched)) {
        fprintf(stderr, "sleeping Copper incorrectly stalled CPU\n");
        return 1;
    }
    sched.copper_request = true;
    if (!agnus_slot_scheduler_cpu_stall(&sched)) {
        fprintf(stderr, "pending Copper fetch did not own free slot\n");
        return 1;
    }

    return 0;
}

static int expect_bitplane_slot(const agnus_slot_scheduler_t *sched,
                                rigel_u16 hpos,
                                rigel_u16 logical)
{
    return hpos < AGNUS_SLOTS_PER_LINE &&
           sched->table[hpos] == AGNUS_SLOT_BITPLANE &&
           sched->bitplane_slot_index[hpos] == logical;
}

static void write_bplpt(RigelContext *ctx, unsigned plane, rigel_u32 addr)
{
    rigel_u32 reg = RIGEL_REG_BPL1PTH + (rigel_u32)(plane * 4u);

    rigel_custom_write16(ctx, reg, (rigel_u16)((addr >> 16) & 0xffffu));
    rigel_custom_write16(ctx, reg + 2u, (rigel_u16)(addr & 0xffffu));
}

static int expect_lores_bitplane_layout(RigelChipset *chipset,
                                        rigel_u16 depth,
                                        rigel_u16 ddfstrt,
                                        rigel_u16 words)
{
    static const rigel_u16 lores_plane_order[6] = {
        7u, 3u, 5u, 1u, 6u, 2u
    };
    rigel_u16 expected_slots = (rigel_u16)(words * depth);

    if (count_bitplane_slots(&chipset->agnus.scheduler) != expected_slots) {
        fprintf(stderr,
                "unexpected lores slot count: depth=%u got=%u expected=%u\n",
                depth,
                count_bitplane_slots(&chipset->agnus.scheduler),
                expected_slots);
        return 0;
    }

    for (rigel_u16 plane = 0u; plane < depth; ++plane) {
        if (!expect_bitplane_slot(&chipset->agnus.scheduler,
                                  (rigel_u16)(ddfstrt +
                                      lores_plane_order[plane]),
                                  plane)) {
            fprintf(stderr,
                    "missing first lores bitplane slot: depth=%u plane=%u\n",
                    depth,
                    plane);
            return 0;
        }
    }

    if (!expect_bitplane_slot(
            &chipset->agnus.scheduler,
            (rigel_u16)(ddfstrt + 8u + lores_plane_order[0]),
            depth) ||
        !expect_bitplane_slot(
            &chipset->agnus.scheduler,
            (rigel_u16)(ddfstrt + (words - 1u) * 8u +
                        lores_plane_order[depth - 1u]),
            (rigel_u16)(expected_slots - 1u))) {
        fprintf(stderr, "unexpected lores logical slot mapping: depth=%u\n", depth);
        return 0;
    }

    return 1;
}

static int expect_lores_bitplane_advance(RigelContext *ctx,
                                         RigelChipset *chipset,
                                         rigel_u16 depth,
                                         rigel_u16 ddfstrt,
                                         rigel_u16 ddfstop,
                                         rigel_u16 words)
{
    const rigel_u32 base[6] = {
        0x0040u, 0x0060u, 0x0080u, 0x00a0u, 0x00c0u, 0x00e0u
    };

    rigel_reset(ctx);
    rigel_custom_write16(ctx, RIGEL_REG_DIWSTRT, 0x5881u);
    rigel_custom_write16(ctx, RIGEL_REG_DIWSTOP, 0x00c1u);
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTRT, ddfstrt);
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTOP, ddfstop);
    rigel_custom_write16(ctx, RIGEL_REG_BPLCON0, (rigel_u16)((depth << 12u) | 0x0001u));
    for (unsigned plane = 0u; plane < 6u; ++plane) {
        write_bplpt(ctx, plane, base[plane]);
    }
    rigel_custom_write16(ctx, AGNUS_BPLMOD1, 0x0000u);
    rigel_custom_write16(ctx, AGNUS_BPLMOD2, 0x0000u);
    rigel_custom_write16(ctx, RIGEL_REG_DMACON,
                         RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BPLEN);

    chipset->agnus.beam.hpos = 0u;
    chipset->agnus.beam.vpos = 88u;
    chipset->agnus.scheduler.hpos = 0u;
    chipset->agnus.scheduler.table_dirty = true;
    rigel_agnus_step(ctx, RIGEL_BEAM_DEFAULT_LINE_CLOCKS);

    if (chipset->agnus.beam.hpos != 0u || chipset->agnus.beam.vpos != 89u) {
        fprintf(stderr,
                "unexpected beam after lores line: depth=%u v=%u h=%u\n",
                depth,
                chipset->agnus.beam.vpos,
                chipset->agnus.beam.hpos);
        return 0;
    }

    for (unsigned plane = 0u; plane < 6u; ++plane) {
        rigel_u32 expected = base[plane];
        if (plane < depth) {
            expected += (rigel_u32)words * 2u;
        }
        if (chipset->agnus.bplpt.bplpt[plane] != expected) {
            fprintf(stderr,
                    "unexpected lores pointer advance: depth=%u plane=%u got=%06x expected=%06x\n",
                    depth,
                    plane,
                    chipset->agnus.bplpt.bplpt[plane],
                    expected);
            return 0;
        }
    }

    return 1;
}

static rigel_u16 test_chip_ram_read16(void *opaque, rigel_u32 addr)
{
    rigel_u16 *ram = (rigel_u16 *)opaque;
    return ram[(addr & 0x1ffu) >> 1];
}

static void test_chip_ram_write16(void *opaque, rigel_u32 addr, rigel_u16 value)
{
    rigel_u16 *ram = (rigel_u16 *)opaque;
    ram[(addr & 0x1ffu) >> 1] = value;
}

static int test_bplcon0_depth_change_invalidates_slots(RigelContext *ctx,
                                                       RigelChipset *chipset)
{
    rigel_reset(ctx);
    rigel_custom_write16(ctx, RIGEL_REG_DIWSTRT, 0x2981u);
    rigel_custom_write16(ctx, RIGEL_REG_DIWSTOP, 0x38c0u);
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTRT, 0x0038u);
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTOP, 0x00d0u);
    rigel_custom_write16(ctx, RIGEL_REG_BPLCON0, 0x1200u);
    rigel_custom_write16(ctx, RIGEL_REG_DMACON,
                         RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BPLEN);

    agnus_slot_scheduler_rebuild(&chipset->agnus.scheduler,
                                 96u,
                                 &chipset->agnus.refresh);
    if (count_bitplane_slots(&chipset->agnus.scheduler) != 20u ||
        chipset->agnus.scheduler.table_dirty) {
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_BPLCON0, 0x4200u);
    if (!chipset->agnus.scheduler.table_dirty) {
        fprintf(stderr, "BPLCON0 depth change did not dirty bitplane slots\n");
        return 1;
    }

    agnus_slot_scheduler_rebuild(&chipset->agnus.scheduler,
                                 96u,
                                 &chipset->agnus.refresh);
    if (count_bitplane_slots(&chipset->agnus.scheduler) != 80u ||
        !expect_bitplane_slot(&chipset->agnus.scheduler, 0x003fu, 0u) ||
        !expect_bitplane_slot(&chipset->agnus.scheduler, 0x003bu, 1u) ||
        !expect_bitplane_slot(&chipset->agnus.scheduler, 0x003du, 2u) ||
        !expect_bitplane_slot(&chipset->agnus.scheduler, 0x0039u, 3u)) {
        fprintf(stderr, "unexpected slots after BPLCON0 depth change\n");
        return 1;
    }

    return 0;
}

int main(void)
{
    rigel_config_t cfg = { 0 };
    rigel_config_t pal_cfg = { 0 };
    rigel_u16 chip_ram[256] = { 0 };
    RigelContext *ctx;
    RigelContext *pal_ctx;
    RigelChipset *chipset;
    rigel_step_result_t result;
    rigel_denise_scanline_t scanline;

    chip_ram[0x40u >> 1] = RIGEL_REG_COLOR00;
    chip_ram[0x42u >> 1] = 0x0f00u;
    chip_ram[0x44u >> 1] = 0x0011u;  /* WAIT: vpos=0, hpos=0, bit0=1 */
    chip_ram[0x46u >> 1] = 0x0000u;  /* mask (bit0=0 = WAIT not SKIP) */
    cfg.chip_ram.opaque = chip_ram;
    cfg.chip_ram.read16 = test_chip_ram_read16;
    cfg.chip_ram.write16 = test_chip_ram_write16;
    if (test_cpu_resume_targets_first_free_slot() != 0)
        return 1;

    ctx = rigel_create(&cfg);

    if (ctx == NULL) {
        return 1;
    }

    chipset = &ctx->chipset;
    if (chipset == NULL) {
        rigel_destroy(ctx);
        return 1;
    }

    if (chipset->agnus.raster.std != AGNUS_VIDEO_NTSC ||
        chipset->agnus.raster.frame_lines != chipset->agnus.beam.frame_lines ||
        chipset->agnus.raster.line_clocks != chipset->agnus.beam.line_clocks) {
        rigel_destroy(ctx);
        return 1;
    }

    pal_cfg.video_std = RIGEL_VIDEO_PAL;
    pal_ctx = rigel_create(&pal_cfg);
    if (pal_ctx == NULL ||
        pal_ctx->chipset.agnus.raster.std != AGNUS_VIDEO_PAL ||
        pal_ctx->chipset.agnus.raster.frame_lines != AGNUS_PAL_FRAME_LINES ||
        pal_ctx->chipset.agnus.beam.frame_lines != AGNUS_PAL_FRAME_LINES) {
        if (pal_ctx != NULL) rigel_destroy(pal_ctx);
        rigel_destroy(ctx);
        return 1;
    }
    rigel_reset(pal_ctx);
    if (pal_ctx->chipset.agnus.raster.std != AGNUS_VIDEO_PAL ||
        pal_ctx->chipset.agnus.beam.frame_lines != AGNUS_PAL_FRAME_LINES) {
        rigel_destroy(pal_ctx);
        rigel_destroy(ctx);
        return 1;
    }
    rigel_destroy(pal_ctx);

    rigel_agnus_step(ctx, 6);
    if (chipset->agnus.beam.hpos != 6 ||
        chipset->agnus.refresh.pending_cycles != 3u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_agnus_step(ctx, (rigel_u32)(RIGEL_BEAM_DEFAULT_LINE_CLOCKS - 6));
    if (chipset->agnus.beam.hpos != 0 || chipset->agnus.beam.vpos != 1) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BLTEN
    );

    rigel_agnus_step(ctx, 1);

    if (!chipset->agnus.dma.enabled || !chipset->agnus.dma.blitter_enabled) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_DMACON, RIGEL_DMACON_BLTEN);
    rigel_agnus_step(ctx, 1);

    if (!chipset->agnus.dma.enabled || chipset->agnus.dma.blitter_enabled) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_reset(ctx);
    result = rigel_step(
        ctx,
        (rigel_cycle_t)(RIGEL_BEAM_DEFAULT_LINE_CLOCKS * RIGEL_BEAM_DEFAULT_FRAME_LINES)
    );

    if ((result.events & RIGEL_EVENT_FRAME_READY) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    if ((result.events & RIGEL_EVENT_HBLANK) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_COP1LCH, 0x0000);
    rigel_custom_write16(ctx, RIGEL_REG_COP1LCL, 0x0040);
    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_COPEN
    );
    rigel_custom_write16(ctx, RIGEL_REG_COPJMP1, 0);
    {
        unsigned i;
        result.events = RIGEL_EVENT_NONE;
        for (i = 0; i < RIGEL_BEAM_DEFAULT_LINE_CLOCKS; i++) {
            result = rigel_step(ctx, 1);
            if (result.events & RIGEL_EVENT_COPPER) break;
        }
    }

    if ((result.events & RIGEL_EVENT_COPPER) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    /* After VBL reload at (0,1) the copper re-executes from COP1LC=0x40;
     * pc is 0x44 after that second MOVE completes. */
    if (chipset->agnus.copper.program_counter != 0x44u) {
        rigel_destroy(ctx);
        return 1;
    }

    if (rigel_custom_read16(ctx, RIGEL_REG_COLOR00) != 0x0f00u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_reset(ctx);
    chip_ram[0x40u >> 1] = AGNUS_BLTSIZE;
    chip_ram[0x42u >> 1] = 0x0001u;
    rigel_custom_write16(ctx, RIGEL_REG_COP1LCH, 0x0000);
    rigel_custom_write16(ctx, RIGEL_REG_COP1LCL, 0x0040);
    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_COPEN
    );
    rigel_custom_write16(ctx, RIGEL_REG_COPJMP1, 0);
    {
        unsigned i;
        result.events = RIGEL_EVENT_NONE;
        for (i = 0; i < RIGEL_BEAM_DEFAULT_LINE_CLOCKS; i++) {
            result = rigel_step(ctx, 1);
            if (result.events & RIGEL_EVENT_COPPER) break;
        }
    }
    {
        const rigel_u16 bltsize = rigel_custom_read16(ctx, AGNUS_BLTSIZE);
        const int blitter_busy = blitter_is_busy(&chipset->agnus.blitter);
        if (!chipset->agnus.copper.stopped_until_vbl ||
            bltsize != 0u ||
            blitter_busy != 0) {
            fprintf(stderr,
                    "unexpected protected copper blitter write result: "
                    "events=%08x stopped=%d bltsize=%04x busy=%d\n",
                    result.events,
                    chipset->agnus.copper.stopped_until_vbl,
                    bltsize,
                    blitter_busy);
            rigel_destroy(ctx);
            return 1;
        }
    }

    rigel_reset(ctx);
    chip_ram[0x40u >> 1] = 0x1a05u;  /* WAIT: vpos=26, hpos=4, bit0=1 */
    chip_ram[0x42u >> 1] = 0x0000u;  /* mask: bit0=0 = WAIT not SKIP */
    chip_ram[0x44u >> 1] = RIGEL_REG_COLOR00;
    chip_ram[0x46u >> 1] = 0x00f0u;
    rigel_custom_write16(ctx, RIGEL_REG_COP1LCH, 0x0000);
    rigel_custom_write16(ctx, RIGEL_REG_COP1LCL, 0x0040);
    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_COPEN
    );
    rigel_custom_write16(ctx, RIGEL_REG_COPJMP1, 0);
    result = rigel_step(ctx, 1);

    if ((result.events & RIGEL_EVENT_COPPER) != 0) {
        fprintf(stderr, "unexpected immediate copper event: events=%08x\n", result.events);
        rigel_destroy(ctx);
        return 1;
    }

    result = rigel_step(ctx, (rigel_cycle_t)(RIGEL_BEAM_DEFAULT_LINE_CLOCKS * 26u + 3u));

    if ((result.events & RIGEL_EVENT_COPPER) == 0) {
        fprintf(stderr, "missing waited copper event: events=%08x\n", result.events);
        rigel_destroy(ctx);
        return 1;
    }

    rigel_step(ctx, 1);
    if (!rigel_denise_get_current_scanline(ctx, &scanline)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (!scanline.visible || !scanline.dirty || scanline.width == 0 || scanline.pixels_rgba == NULL) {
        fprintf(stderr,
                "unexpected scanline state: visible=%d dirty=%d width=%u pixels=%p\n",
                scanline.visible,
                scanline.dirty,
                scanline.width,
                (void *)scanline.pixels_rgba);
        rigel_destroy(ctx);
        return 1;
    }

    if (scanline.last_rgb32 == 0 || scanline.pixels_rgba[0] != scanline.last_rgb32) {
        fprintf(stderr,
                "unexpected scanline pixel: last=%08x first=%08x\n",
                scanline.last_rgb32,
                scanline.pixels_rgba[0]);
        rigel_destroy(ctx);
        return 1;
    }

    /* DDFSTRT/DDFSTOP MMIO writes update the slot scheduler DDF range */
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTRT, 0x0050u);
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTOP, 0x00c0u);

    if (chipset->agnus.raster.ddfstrt != 0x0050u ||
        chipset->agnus.raster.ddfstop != 0x00c0u ||
        chipset->agnus.scheduler.ddfstrt != 0x0050u ||
        chipset->agnus.scheduler.ddfstop != 0x00c0u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_DIWSTRT, 0x2c81u);
    rigel_custom_write16(ctx, RIGEL_REG_DIWSTOP, 0x2cc1u);
    if (chipset->agnus.raster.diwstrt != 0x2c81u ||
        chipset->agnus.raster.diwstop != 0x2cc1u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_BPLCON0, 0xb200u);
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTRT, 0x003cu);
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTOP, 0x00d4u);
    rigel_custom_write16(ctx, RIGEL_REG_DMACON,
                         RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BPLEN);
    agnus_slot_scheduler_rebuild(&chipset->agnus.scheduler,
                                 44u,
                                 &chipset->agnus.refresh);
    if (count_bitplane_slots(&chipset->agnus.scheduler) != 120u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_BPLCON0, 0x2302u);
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTRT, 0x0038u);
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTOP, 0x00d0u);
    agnus_slot_scheduler_rebuild(&chipset->agnus.scheduler,
                                 44u,
                                 &chipset->agnus.refresh);
    if (count_bitplane_slots(&chipset->agnus.scheduler) != 40u) {
        rigel_destroy(ctx);
        return 1;
    }

    if (test_bplcon0_depth_change_invalidates_slots(ctx, chipset) != 0) {
        rigel_destroy(ctx);
        return 1;
    }

    for (rigel_u16 depth = 1u; depth <= 6u; ++depth) {
        const rigel_u16 ddfstrt = 0x0030u;
        const rigel_u16 ddfstop = 0x00d0u;
        const rigel_u16 words = 21u;

        rigel_custom_write16(ctx, RIGEL_REG_BPLCON0, (rigel_u16)((depth << 12u) | 0x0001u));
        rigel_custom_write16(ctx, RIGEL_REG_DDFSTRT, ddfstrt);
        rigel_custom_write16(ctx, RIGEL_REG_DDFSTOP, ddfstop);
        rigel_custom_write16(ctx, RIGEL_REG_DMACON,
                             RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BPLEN);
        agnus_slot_scheduler_rebuild(&chipset->agnus.scheduler,
                                     88u,
                                     &chipset->agnus.refresh);
        if (!expect_lores_bitplane_layout(chipset, depth, ddfstrt, words) ||
            !expect_lores_bitplane_advance(ctx, chipset, depth, ddfstrt, ddfstop, words)) {
            rigel_destroy(ctx);
            return 1;
        }
    }

    rigel_agnus_step(ctx, 4);

    rigel_destroy(ctx);
    return 0;
}
