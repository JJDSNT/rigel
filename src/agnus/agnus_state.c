#include "agnus/agnus_state.h"

#include <stddef.h>

#include "core/riegel_context.h"
#include "chipset/chipset.h"
#include "riegel/riegel_custom.h"

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

BlitterMemory riegel_agnus_blitter_memory(RiegelContext *ctx)
{
    BlitterMemory mem;

    mem = ctx->config.chip_ram;
    return mem;
}

BlitterIrqSink riegel_agnus_blitter_irq_sink(RiegelContext *ctx)
{
    BlitterIrqSink sink;

    sink.opaque = ctx;
    sink.raise = riegel_agnus_raise_blitter_irq;
    return sink;
}

void riegel_agnus_blitter_step_dma(RiegelContext *ctx, riegel_u32 dma_slots)
{
    if (ctx == NULL || dma_slots == 0) {
        return;
    }

    blitter_step_dma(
        &ctx->chipset.agnus.blitter,
        riegel_agnus_blitter_memory(ctx),
        riegel_agnus_blitter_irq_sink(ctx),
        dma_slots
    );
}

void riegel_agnus_step(RiegelContext *ctx, riegel_u32 cycles)
{
    riegel_u16 dmacon;

    if (ctx == NULL || cycles == 0) {
        return;
    }

    dmacon = ctx->chipset.agnus.dmacon;

    if ((dmacon & RIEGEL_DMACON_DMAEN) == 0) {
        return;
    }

    if ((dmacon & RIEGEL_DMACON_BLTEN) == 0) {
        return;
    }

    riegel_agnus_blitter_step_dma(ctx, cycles);
}
