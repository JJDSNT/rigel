#include "rigel/rigel.h"

#include "agnus/agnus_state.h"
#include "agnus/blitter.h"

int main(void)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);
    BlitterIrqSink sink;
    BlitterState blitter;

    if (ctx == NULL) {
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_INTENA, 0xc001);
    rigel_custom_write16(ctx, RIGEL_REG_INTREQ, 0x8001);

    if (rigel_get_intena(ctx) != 0x4001 || rigel_get_intreq(ctx) != 0x0001) {
        rigel_destroy(ctx);
        return 1;
    }

    if (rigel_get_ipl(ctx) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_reset(ctx);
    sink = rigel_agnus_blitter_irq_sink(ctx);
    blitter_init(&blitter);
    blitter_force_finish(&blitter, sink);

    if ((rigel_get_intreq(ctx) & 0x0040u) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}
