#include "agnus/agnus_state.h"

#include <stddef.h>

#include "core/rigel_context.h"
#include "chipset/chipset.h"
#include "rigel/rigel_custom.h"

static void rigel_agnus_raise_blitter_irq(void *opaque, uint16_t mask)
{
    RigelContext *ctx = (RigelContext *)opaque;

    if (ctx == NULL) {
        return;
    }

    rigel_chipset_raise_irq_source(&ctx->chipset, mask);
}

void rigel_agnus_reset(RigelAgnus *agnus)
{
    if (agnus == NULL) {
        return;
    }

    agnus->dmacon = 0;
    blitter_reset(&agnus->blitter);
}

BlitterMemory rigel_agnus_blitter_memory(RigelContext *ctx)
{
    BlitterMemory mem;

    mem = ctx->config.chip_ram;
    return mem;
}

BlitterIrqSink rigel_agnus_blitter_irq_sink(RigelContext *ctx)
{
    BlitterIrqSink sink;

    sink.opaque = ctx;
    sink.raise = rigel_agnus_raise_blitter_irq;
    return sink;
}

void rigel_agnus_blitter_step_dma(RigelContext *ctx, rigel_u32 dma_slots)
{
    if (ctx == NULL || dma_slots == 0) {
        return;
    }

    blitter_step_dma(
        &ctx->chipset.agnus.blitter,
        rigel_agnus_blitter_memory(ctx),
        rigel_agnus_blitter_irq_sink(ctx),
        dma_slots
    );
}

void rigel_agnus_step(RigelContext *ctx, rigel_u32 cycles)
{
    rigel_u16 dmacon;

    if (ctx == NULL || cycles == 0) {
        return;
    }

    dmacon = ctx->chipset.agnus.dmacon;

    if ((dmacon & RIGEL_DMACON_DMAEN) == 0) {
        return;
    }

    if ((dmacon & RIGEL_DMACON_BLTEN) == 0) {
        return;
    }

    rigel_agnus_blitter_step_dma(ctx, cycles);
}
