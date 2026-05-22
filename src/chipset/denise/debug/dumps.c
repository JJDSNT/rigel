#include "dumps.h"

#include <stdio.h>

void denise_dump_regs(const RigelDenise *denise)
{
    if (!denise) return;
    fprintf(stderr,
            "DENISE REGS: BPLCON0=$%04X BPLCON1=$%04X BPLCON2=$%04X "
            "DIWSTRT=$%04X DIWSTOP=$%04X\n",
            denise->regs.bplcon0, denise->regs.bplcon1, denise->regs.bplcon2,
            denise->regs.diwstrt, denise->regs.diwstop);
}

void denise_dump_palette(const RigelDenise *denise)
{
    if (!denise) return;
    fprintf(stderr, "PALETTE:\n");
    for (unsigned i = 0; i < 32; i++) {
        fprintf(stderr, "  COLOR%02u = $%04X (rgb32=$%08X)\n",
                i, denise->regs.color[i], denise->palette.rgb32[i]);
    }
}

void denise_dump_sprites(const RigelDenise *denise)
{
    if (!denise) return;
    fprintf(stderr, "SPRITES: active=$%04X attached=$%04X\n",
            denise->sprites.active_mask, denise->sprites.attached_mask);
    /* TODO(debug): dump per-sprite pos/ctl once denise_sprites_state_t is expanded */
}

void denise_dump_video(const RigelDenise *denise)
{
    if (!denise) return;
    fprintf(stderr,
            "VIDEO: %ux%u visible=[%u..%u, %u..%u] "
            "hpos=%u vpos=%u frame=%llu\n",
            denise->video.width, denise->video.height,
            denise->video.visible_x_start, denise->video.visible_x_stop,
            denise->video.visible_y_start, denise->video.visible_y_stop,
            denise->output.beam_hpos, denise->output.beam_vpos,
            (unsigned long long)denise->output.frame_counter);
}
