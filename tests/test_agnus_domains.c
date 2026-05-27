#include "agnus/agnus_state.h"
#include "chipset/chipset.h"
#include "core/rigel_context.h"
#include "rigel/rigel.h"

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
        rigel_destroy(ctx);
        return 1;
    }

    result = rigel_step(ctx, (rigel_cycle_t)(RIGEL_BEAM_DEFAULT_LINE_CLOCKS * 26u + 3u));

    if ((result.events & RIGEL_EVENT_COPPER) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_step(ctx, 1);
    if (!rigel_denise_get_current_scanline(ctx, &scanline)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (!scanline.visible || !scanline.dirty || scanline.width == 0 || scanline.pixels_rgba == NULL) {
        rigel_destroy(ctx);
        return 1;
    }

    if (scanline.last_rgb32 == 0 || scanline.pixels_rgba[0] != scanline.last_rgb32) {
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

    rigel_agnus_step(ctx, 4);

    rigel_destroy(ctx);
    return 0;
}
