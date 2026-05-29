#include "denise/denise_state.h"

#include "rigel/rigel_custom.h"
#include "denise/debug/denise_trace.h"
#include "denise/output/framebuffer.h"
#include "denise/palette/palette.h"
#include "denise/registers/denise_registers.h"
#include "denise/render/compositor.h"
#include "denise/sprites/sprites_core.h"
#include "denise/video/display_window.h"
#include "agnus/timing/raster.h"
#include "agnus/timing/slot_scheduler.h"
#include "core/rigel_context.h"

void rigel_denise_reset(RigelDenise *denise)
{
    agnus_chip_rev_t chip_rev;

    if (denise == NULL) {
        return;
    }

    chip_rev = denise->chip_rev;
    rigel_denise_palette_reset(&denise->palette);
    denise->chip_rev = chip_rev;
    rigel_denise_sprites_reset(&denise->sprites);
    collision_reset(&denise->coll);
    rigel_denise_display_window_reset(&denise->video);
    rigel_denise_framebuffer_reset(&denise->output);
    denise->regs.bplcon0 = 0;
    denise->regs.bplcon1 = 0;
    denise->regs.bplcon2 = 0;
    denise->regs.bplcon3 = 0;
    denise->regs.diwstrt = 0;
    denise->regs.diwstop = 0;
    denise->regs.diwhigh = 0;
    denise->debug.frame_counter = 0;
    denise->debug.scanline_counter = 0;
    denise->debug.active_mode_flags = 0;
    denise->debug.last_color_index = 0;
    denise->debug.beam_hpos = 0;
    denise->debug.beam_vpos = 0;
    denise->debug.current_scanline = 0;
    denise->debug.current_pixel = 0;
    denise->debug.last_rgb32 = 0;
    denise->debug.visible_scanline = false;
}

void rigel_denise_set_chip_rev(RigelDenise *denise, agnus_chip_rev_t rev)
{
    if (denise == NULL) {
        return;
    }

    denise->chip_rev = rev;
    rigel_denise_display_window_update(denise);
}

void rigel_denise_step(RigelDenise *denise, const beam_state_t *beam, rigel_u32 cycles)
{
    if (denise == NULL) {
        return;
    }

    rigel_denise_compositor_tick(denise, beam, cycles);
    rigel_denise_trace_tick(&denise->debug, cycles);
    denise->debug.frame_counter = (rigel_u32)denise->output.frame_counter;
    denise->debug.beam_hpos = denise->output.beam_hpos;
    denise->debug.beam_vpos = denise->output.beam_vpos;
    denise->debug.current_scanline = denise->output.current_scanline;
    denise->debug.current_pixel = denise->output.current_pixel;
    denise->debug.last_rgb32 = denise->output.last_rgb;
    denise->debug.visible_scanline = denise->output.visible_scanline;
}

void rigel_denise_set_framebuffer_target(RigelDenise *denise, const rigel_framebuffer_target_t *target)
{
    if (denise == NULL) {
        return;
    }

    rigel_denise_framebuffer_set_target(&denise->output, target);
}

bool rigel_denise_owns_reg(rigel_u32 addr)
{
    return rigel_denise_registers_owns_reg(addr);
}

rigel_u16 rigel_denise_read_reg(RigelContext *ctx, rigel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    return rigel_denise_registers_read(&ctx->chipset.denise, addr);
}

void rigel_denise_write_reg(RigelContext *ctx, rigel_u32 addr, rigel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    rigel_denise_registers_write(&ctx->chipset.denise, addr, value);
    rigel_context_write_reg(ctx, addr, value);

    if (addr == RIGEL_REG_BPLCON0) {
        agnus_slot_scheduler_set_hires(
            &ctx->chipset.agnus.scheduler,
            (value & 0x8000u) != 0
        );
        agnus_slot_scheduler_set_depth(
            &ctx->chipset.agnus.scheduler,
            (value >> 12) & 0x7u
        );
        ctx->chipset.agnus.beam.lof_toggle = (value & 0x0004u) != 0u;
        if ((value & 0x0004u) == 0u) {
            ctx->chipset.agnus.beam.lof = 0u;
        }
    }

    if (addr == RIGEL_REG_DIWSTRT) {
        raster_set_diwstrt(&ctx->chipset.agnus.raster, value);
    }
    if (addr == RIGEL_REG_DIWSTOP) {
        raster_set_diwstop(&ctx->chipset.agnus.raster, value);
    }
}
