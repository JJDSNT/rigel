#include "rigel/rigel.h"

int main(void)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);
    rigel_denise_video_desc_t video;
    rigel_denise_debug_state_t debug;
    rigel_denise_scanline_t scanline;
    rigel_u32 frame_cycles = 227u * 262u;

    if (ctx == NULL) {
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x0f00);
    rigel_custom_write16(ctx, 0x08e, 0x2c81);
    rigel_custom_write16(ctx, 0x090, 0x2cc1);

    if (rigel_custom_read16(ctx, RIGEL_REG_COLOR00) != 0x0f00) {
        rigel_destroy(ctx);
        return 1;
    }

    if (!rigel_denise_get_video_desc(ctx, &video)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (video.display_width != 65 || video.display_height != 1) {
        rigel_destroy(ctx);
        return 1;
    }

    if (video.visible_x_start != 0x81 || video.visible_x_stop != 0xc1) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_step(ctx, 4);

    if (!rigel_denise_get_debug_state(ctx, &debug)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (debug.scanline_counter == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    if (debug.last_rgb32 == 0 || debug.visible_scanline) {
        rigel_destroy(ctx);
        return 1;
    }

    if (!rigel_denise_get_current_scanline(ctx, &scanline)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (scanline.width != video.display_width || scanline.visible) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_step(ctx, frame_cycles);

    if (!rigel_denise_get_debug_state(ctx, &debug)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (debug.frame_counter != 1 || debug.beam_vpos != 0 || debug.beam_hpos != 4) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}
