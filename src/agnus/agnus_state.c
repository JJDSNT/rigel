#include "agnus/agnus_state.h"

#include <stddef.h>

#include "core/riegel_context.h"
#include "chipset/chipset.h"

static void riegel_agnus_raise_blitter_irq(void *opaque, uint16_t mask)
{
    RiegelContext *ctx = (RiegelContext *)opaque;

    if (ctx == NULL) {
        return;
    }

    riegel_chipset_raise_intreq(&ctx->chipset, mask);
}

void riegel_agnus_reset(RiegelAgnus *agnus)
{
    if (agnus == NULL) {
        return;
    }

    agnus->dmacon = 0;
    blitter_reset(&agnus->blitter);
}

BlitterIrqSink riegel_agnus_blitter_irq_sink(RiegelContext *ctx)
{
    BlitterIrqSink sink;

    sink.opaque = ctx;
    sink.raise = riegel_agnus_raise_blitter_irq;
    return sink;
}
