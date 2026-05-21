#include "riegel/riegel.h"

#include "agnus/agnus_state.h"
#include "agnus/blitter.h"

int main(void)
{
    riegel_config_t cfg = { 0 };
    RiegelContext *ctx = riegel_create(&cfg);
    BlitterIrqSink sink;
    BlitterState blitter;

    if (ctx == NULL) {
        return 1;
    }

    riegel_custom_write16(ctx, RIEGEL_REG_INTENA, 0xc001);
    riegel_custom_write16(ctx, RIEGEL_REG_INTREQ, 0x8001);

    if (riegel_get_intena(ctx) != 0x4001 || riegel_get_intreq(ctx) != 0x0001) {
        riegel_destroy(ctx);
        return 1;
    }

    if (riegel_get_ipl(ctx) == 0) {
        riegel_destroy(ctx);
        return 1;
    }

    riegel_reset(ctx);
    sink = riegel_agnus_blitter_irq_sink(ctx);
    blitter_init(&blitter);
    blitter_force_finish(&blitter, sink);

    if ((riegel_get_intreq(ctx) & 0x0040u) == 0) {
        riegel_destroy(ctx);
        return 1;
    }

    riegel_destroy(ctx);
    return 0;
}
