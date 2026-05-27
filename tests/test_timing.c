#include "agnus/agnus_state.h"
#include "rigel/rigel.h"

int main(void)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);
    rigel_u32 clock_hz;
    rigel_u32 line_cycles;
    rigel_u32 frame_cycles;
    rigel_u64 frame_us;

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

    rigel_destroy(ctx);
    return 0;
}
