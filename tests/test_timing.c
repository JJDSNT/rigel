#include "agnus/agnus_state.h"
#include "core/rigel_context.h"
#include "rigel/rigel.h"

static int check_beam_projection(rigel_video_std_t video_std)
{
    static const rigel_cycle_t deltas[] = {
        0u, 1u, 2u, 113u, 226u, 227u, 228u, 1000u, 65537u
    };
    rigel_config_t cfg = { 0 };
    RigelContext *ctx;

    cfg.video_std = video_std;
    ctx = rigel_create(&cfg);
    if (ctx == NULL)
        return 1;

    for (rigel_u32 i = 0; i < (rigel_u32)(sizeof(deltas) / sizeof(deltas[0])); i++) {
        rigel_beam_geometry_t geometry = rigel_get_beam_geometry(ctx);
        rigel_u16 projected_vpos;
        rigel_u16 projected_hpos;

        if (!rigel_beam_position_at(&geometry, geometry.time + deltas[i],
                                    &projected_vpos, &projected_hpos)) {
            rigel_destroy(ctx);
            return 1;
        }

        (void)rigel_step(ctx, deltas[i]);
        if (projected_vpos != ctx->chipset.agnus.beam.vpos ||
            projected_hpos != ctx->chipset.agnus.beam.hpos) {
            rigel_destroy(ctx);
            return 1;
        }
    }

    rigel_destroy(ctx);
    return 0;
}

int main(void)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);
    RigelContext *pal_ctx;
    rigel_u32 clock_hz;
    rigel_u32 line_cycles;
    rigel_u32 frame_cycles;
    rigel_u64 frame_us;
    rigel_cycle_t deadline;
    rigel_cycle_t observable_deadline;
    rigel_beam_geometry_t geometry;
    rigel_beam_geometry_t future_geometry;
    rigel_u16 vpos;
    rigel_u16 hpos;

    if (check_beam_projection(RIGEL_VIDEO_NTSC) != 0 ||
        check_beam_projection(RIGEL_VIDEO_PAL) != 0) {
        return 1;
    }

    if (ctx == NULL) {
        return 1;
    }

    clock_hz = rigel_get_clock_hz(ctx);
    line_cycles = rigel_get_line_cycles(ctx);
    frame_cycles = rigel_get_frame_cycles(ctx);
    frame_us = rigel_cycles_to_us(frame_cycles, clock_hz);

    if (clock_hz != 7093790u) {
        rigel_destroy(ctx);
        return 1;
    }

    if (line_cycles != 227u || frame_cycles != (227u * 262u)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (frame_us == 0 || rigel_us_to_cycles(frame_us, clock_hz) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    if (rigel_us_to_cycles(1000000u, clock_hz) != clock_hz) {
        rigel_destroy(ctx);
        return 1;
    }

    /* VERTB fires at vpos=0, hpos=1 (one CCK from reset) */
    rigel_agnus_step(ctx, 1);
    if ((rigel_get_intreq(ctx) & 0x0020u) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_agnus_step(ctx, 1);
    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_DSKEN
    );
    deadline = rigel_get_next_deadline(ctx);
    if (deadline != rigel_get_time(ctx) + 1u) {
        rigel_destroy(ctx);
        return 1;
    }

    /* The slot-exact deadline is one CCK away, but the host-observable API is
     * allowed to cross internal DMA slots processed inside rigel_step(). */
    observable_deadline = rigel_get_next_observable_deadline(ctx);
    if (observable_deadline <= rigel_get_time(ctx)) {
        rigel_destroy(ctx);
        return 1;
    }

    geometry = rigel_get_beam_geometry(ctx);
    if (geometry.time != rigel_get_time(ctx) ||
        geometry.vpos != ctx->chipset.agnus.beam.vpos ||
        geometry.hpos != ctx->chipset.agnus.beam.hpos ||
        !rigel_beam_position_at(&geometry,
                                geometry.time + geometry.line_clocks + 7u,
                                &vpos, &hpos) ||
        vpos != (rigel_u16)((geometry.vpos + 1u) % geometry.frame_lines) ||
        hpos != (rigel_u16)((geometry.hpos + 7u) % geometry.line_clocks)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (!rigel_beam_position_at(
            &geometry,
            geometry.time +
                ((rigel_cycle_t)(geometry.frame_lines - geometry.vpos) *
                 geometry.line_clocks) - geometry.hpos + 3u,
            &vpos, &hpos) ||
        vpos != 0u || hpos != 3u) {
        rigel_destroy(ctx);
        return 1;
    }

    future_geometry = geometry;
    future_geometry.time = geometry.time + 1u;
    if (rigel_beam_position_at(&future_geometry, geometry.time, &vpos, &hpos) ||
        rigel_beam_position_at(NULL, geometry.time, &vpos, &hpos) ||
        rigel_beam_position_at(&geometry, geometry.time, NULL, &hpos)) {
        rigel_destroy(ctx);
        return 1;
    }

    geometry.lof_toggle = 1u;
    if (rigel_beam_position_at(&geometry, geometry.time, &vpos, &hpos)) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);

    cfg.video_std = RIGEL_VIDEO_PAL;
    pal_ctx = rigel_create(&cfg);
    if (pal_ctx == NULL) {
        return 1;
    }
    geometry = rigel_get_beam_geometry(pal_ctx);
    if (geometry.frame_lines == 262u ||
        !rigel_beam_position_at(
            &geometry,
            geometry.time +
                (rigel_cycle_t)geometry.line_clocks * geometry.frame_lines + 5u,
            &vpos, &hpos) ||
        vpos != 0u || hpos != 5u) {
        rigel_destroy(pal_ctx);
        return 1;
    }
    rigel_destroy(pal_ctx);
    return 0;
}
