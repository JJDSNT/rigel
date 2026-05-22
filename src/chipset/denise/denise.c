#include "denise/denise_state.h"

#include "denise/debug/denise_trace.h"
#include "denise/output/framebuffer.h"
#include "denise/palette/palette.h"
#include "denise/registers/denise_registers.h"
#include "denise/render/compositor.h"
#include "denise/sprites/sprites_core.h"
#include "denise/video/display_window.h"
#include "core/rigel_context.h"

void rigel_denise_reset(RigelDenise *denise)
{
    if (denise == NULL) {
        return;
    }

    rigel_denise_palette_reset(&denise->palette);
    rigel_denise_sprites_reset(&denise->sprites);
    rigel_denise_display_window_reset(&denise->video);
    rigel_denise_framebuffer_reset(&denise->output);
    denise->regs.bplcon0 = 0;
    denise->regs.bplcon1 = 0;
    denise->regs.bplcon2 = 0;
    denise->regs.diwstrt = 0;
    denise->regs.diwstop = 0;
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
}
