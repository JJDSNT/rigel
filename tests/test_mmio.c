#include "rigel/rigel.h"

int main(void)
{
    rigel_config_t cfg = { 0 };
    rigel_config_t ecs_cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);
    RigelContext *ecs_ctx;
    rigel_denise_video_desc_t video;

    if (ctx == NULL) {
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x0f00);

    if (rigel_custom_read16(ctx, RIGEL_REG_COLOR00) != 0x0f00) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_DMACON, RIGEL_SETCLR | RIGEL_DMACON_DMAEN);
    if (rigel_custom_read16(ctx, RIGEL_REG_DMACON) != RIGEL_DMACON_DMAEN) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_DMACON, RIGEL_DMACON_DMAEN);
    if (rigel_custom_read16(ctx, RIGEL_REG_DMACON) != 0x0000) {
        rigel_destroy(ctx);
        return 1;
    }

    if ((rigel_custom_read16(ctx, RIGEL_REG_VPOSR) & 0x7f00u) != 0x1000u) {
        rigel_destroy(ctx);
        return 1;
    }

    if (rigel_custom_read16(ctx, RIGEL_REG_DENISEID) != 0x0000u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_BEAMCON0, 0x0020u);
    if (rigel_custom_read16(ctx, RIGEL_REG_BEAMCON0) != 0x0000u) {
        rigel_destroy(ctx);
        return 1;
    }

    ecs_cfg.chipset_model = RIGEL_CHIPSET_ECS;
    ecs_cfg.video_std = RIGEL_VIDEO_PAL;
    ecs_ctx = rigel_create(&ecs_cfg);
    if (ecs_ctx == NULL) {
        rigel_destroy(ctx);
        return 1;
    }

    if ((rigel_custom_read16(ecs_ctx, RIGEL_REG_VPOSR) & 0x7f00u) != 0x2000u ||
        rigel_custom_read16(ecs_ctx, RIGEL_REG_DENISEID) != 0x00fcu) {
        rigel_destroy(ecs_ctx);
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ecs_ctx, RIGEL_REG_BEAMCON0, 0x0000u);
    if (rigel_custom_read16(ecs_ctx, RIGEL_REG_BEAMCON0) != 0x0000u ||
        rigel_get_frame_cycles(ecs_ctx) != (227u * 262u)) {
        rigel_destroy(ecs_ctx);
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ecs_ctx, RIGEL_REG_BEAMCON0, 0x0020u);
    if (rigel_custom_read16(ecs_ctx, RIGEL_REG_BEAMCON0) != 0x0020u ||
        rigel_get_frame_cycles(ecs_ctx) != (227u * 312u)) {
        rigel_destroy(ecs_ctx);
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ecs_ctx, RIGEL_REG_DIWSTRT, 0x2c81u);
    rigel_custom_write16(ecs_ctx, RIGEL_REG_DIWSTOP, 0x2cc1u);
    rigel_custom_write16(ecs_ctx, RIGEL_REG_DIWHIGH, 0x0900u);
    if (!rigel_denise_get_video_desc(ecs_ctx, &video) ||
        video.visible_x_start != 0x0081u ||
        video.visible_x_stop != 0x01c1u ||
        video.visible_y_start != 0x002cu ||
        video.visible_y_stop != 0x012cu ||
        video.display_width != 320u ||
        video.display_height != 256u) {
        rigel_destroy(ecs_ctx);
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ecs_ctx);
    rigel_destroy(ctx);
    return 0;
}
