#include "agnus/agnus_state.h"

#include <stddef.h>

#include "chipset/chipset.h"
#include "core/rigel_context.h"
#include "domains/beam/beam_domain.h"
#include "domains/blitter/blitter_domain.h"
#include "domains/copper/copper_domain.h"
#include "domains/dma/dma_domain.h"

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

    rigel_beam_domain_reset(&agnus->beam);
    rigel_dma_domain_reset(&agnus->dma);
    rigel_copper_domain_reset(&agnus->copper);
    rigel_blitter_domain_reset(&agnus->blitter);
    bitplanes_set_depth(&agnus->bitplanes, 0);
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

    rigel_blitter_domain_step_dma(
        &ctx->chipset.agnus.blitter,
        rigel_agnus_blitter_memory(ctx),
        rigel_agnus_blitter_irq_sink(ctx),
        dma_slots
    );
}

void rigel_agnus_step(RigelContext *ctx, rigel_u32 cycles)
{
    RigelAgnus *agnus;
    rigel_u32 blitter_grants;

    if (ctx == NULL || cycles == 0) {
        return;
    }

    agnus = &ctx->chipset.agnus;

    rigel_beam_domain_step(&agnus->beam, (rigel_u16)cycles);
    rigel_dma_domain_sync_dmacon(&agnus->dma, agnus->dma.dmacon);
    rigel_copper_domain_step(&agnus->copper, cycles);
    blitter_grants = rigel_dma_domain_blitter_grants(&agnus->dma, cycles);
    rigel_agnus_blitter_step_dma(ctx, blitter_grants);
}
