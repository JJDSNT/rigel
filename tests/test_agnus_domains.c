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
    rigel_u16 chip_ram[256] = { 0 };
    RigelContext *ctx;
    RigelChipset *chipset;
    rigel_step_result_t result;
    rigel_denise_scanline_t scanline;

    chip_ram[0x40u >> 1] = 0x0f00u;
    chip_ram[0x42u >> 1] = RIGEL_REG_COLOR00;
    chip_ram[0x44u >> 1] = 0x0010u;
    chip_ram[0x46u >> 1] = 0x0001u;
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

    rigel_agnus_step(ctx, 4);
    if (chipset->agnus.beam.hpos != 4) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_agnus_step(ctx, (rigel_u32)(RIGEL_BEAM_DEFAULT_LINE_CLOCKS - 4));
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
    result = rigel_step(ctx, 1);

    if ((result.events & RIGEL_EVENT_COPPER) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    if (chipset->agnus.copper.program_counter != 0x44u) {
        rigel_destroy(ctx);
        return 1;
    }

    if (rigel_custom_read16(ctx, RIGEL_REG_COLOR00) != 0x0f00u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_reset(ctx);
    chip_ram[0x40u >> 1] = 0x1a04u;
    chip_ram[0x42u >> 1] = 0x0001u;
    chip_ram[0x44u >> 1] = 0x00f0u;
    chip_ram[0x46u >> 1] = RIGEL_REG_COLOR00;
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

    if (chipset->agnus.scheduler.ddfstrt != 0x0050u ||
        chipset->agnus.scheduler.ddfstop != 0x00c0u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_agnus_step(ctx, 4);

    rigel_destroy(ctx);
    return 0;
}
