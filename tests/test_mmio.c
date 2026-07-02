#include "rigel/rigel.h"
#include "core/rigel_context.h"

typedef struct chip_ram_trace {
    rigel_u32 last_read_addr;
    rigel_u32 last_write_addr;
} chip_ram_trace_t;

static rigel_u16 trace_chip_ram_read16(void *opaque, rigel_u32 addr)
{
    chip_ram_trace_t *trace = (chip_ram_trace_t *)opaque;

    trace->last_read_addr = addr;
    return 0;
}

static void trace_chip_ram_write16(void *opaque, rigel_u32 addr, rigel_u16 value)
{
    chip_ram_trace_t *trace = (chip_ram_trace_t *)opaque;

    (void)value;
    trace->last_write_addr = addr;
}

int main(void)
{
    rigel_config_t cfg = { 0 };
    rigel_config_t ecs_cfg = { 0 };
    chip_ram_trace_t ocs_trace = { 0 };
    chip_ram_trace_t ecs_trace = { 0 };
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
    /* V8 (bit 8) + STOP H8 (bit 13) — H8 lives at bit 13 per ECS DIWHIGH */
    rigel_custom_write16(ecs_ctx, RIGEL_REG_DIWHIGH, 0x2100u);
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

    rigel_custom_write16(ecs_ctx, RIGEL_REG_BPLCON0, 0x9000u);
    rigel_custom_write16(ecs_ctx, RIGEL_REG_DIWHIGH, 0x2000u);
    if (!rigel_denise_get_video_desc(ecs_ctx, &video) ||
        video.visible_x_start != 0x0102u ||
        video.visible_x_stop != 0x0382u ||
        video.visible_y_start != 0x002cu ||
        video.visible_y_stop != 0x012cu ||
        video.display_width != 640u ||
        video.display_height != 256u) {
        rigel_destroy(ecs_ctx);
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ecs_ctx, RIGEL_REG_BPLCON0, 0x0201u);
    rigel_custom_write16(ecs_ctx, RIGEL_REG_DIWHIGH, 0x0000u);
    rigel_custom_write16(ecs_ctx, RIGEL_REG_DIWSTRT, 0x0181u);
    rigel_custom_write16(ecs_ctx, RIGEL_REG_DIWSTOP, 0x0281u);
    /* BPU=0 keeps the previous (hires DIWHIGH) geometry latched */
    if (!rigel_denise_get_video_desc(ecs_ctx, &video) ||
        video.visible_x_start != 0x0102u ||
        video.visible_x_stop != 0x0382u ||
        video.display_width != 640u ||
        video.display_height != 256u) {
        rigel_destroy(ecs_ctx);
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ecs_ctx, RIGEL_REG_DIWSTRT, 0x2c81u);
    rigel_custom_write16(ecs_ctx, RIGEL_REG_DIWSTOP, 0x2cc1u);
    rigel_custom_write16(ecs_ctx, RIGEL_REG_DIWHIGH, 0x2100u);
    rigel_custom_write16(ecs_ctx, RIGEL_REG_BPLCON0, 0xc205u);
    if (!rigel_denise_get_video_desc(ecs_ctx, &video) ||
        video.visible_x_start != 0x0102u ||
        video.visible_x_stop != 0x0382u ||
        video.display_width != 640u ||
        video.display_height != 256u) {
        rigel_destroy(ecs_ctx);
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ecs_ctx);
    rigel_destroy(ctx);

    cfg = (rigel_config_t){ 0 };
    cfg.chip_ram_size = 0x00100000u;
    cfg.chip_ram.opaque = &ocs_trace;
    cfg.chip_ram.read16 = trace_chip_ram_read16;
    cfg.chip_ram.write16 = trace_chip_ram_write16;
    ctx = rigel_create(&cfg);
    if (ctx == NULL) {
        return 1;
    }
    {
        rigel_chip_ram_if_t mem = rigel_context_chip_ram(ctx);
        (void)mem.read16(mem.opaque, 0x00080000u);
        mem.write16(mem.opaque, 0x00080002u, 0x1234u);
    }
    if (ocs_trace.last_read_addr != 0x00000000u ||
        ocs_trace.last_write_addr != 0x00000002u) {
        rigel_destroy(ctx);
        return 1;
    }
    rigel_destroy(ctx);

    ecs_cfg = (rigel_config_t){ 0 };
    ecs_cfg.chipset_model = RIGEL_CHIPSET_ECS;
    ecs_cfg.chip_ram_size = 0x00100000u;
    ecs_cfg.chip_ram.opaque = &ecs_trace;
    ecs_cfg.chip_ram.read16 = trace_chip_ram_read16;
    ecs_cfg.chip_ram.write16 = trace_chip_ram_write16;
    ecs_ctx = rigel_create(&ecs_cfg);
    if (ecs_ctx == NULL) {
        return 1;
    }
    {
        rigel_chip_ram_if_t mem = rigel_context_chip_ram(ecs_ctx);
        (void)mem.read16(mem.opaque, 0x00080000u);
        mem.write16(mem.opaque, 0x00100002u, 0x1234u);
    }
    if (ecs_trace.last_read_addr != 0x00080000u ||
        ecs_trace.last_write_addr != 0x00000002u) {
        rigel_destroy(ecs_ctx);
        return 1;
    }
    rigel_destroy(ecs_ctx);

    return 0;
}
