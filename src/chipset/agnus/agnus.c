#include "agnus/agnus_state.h"

#include <stddef.h>

#include "chipset/chipset.h"
#include "agnus/blitter/blitter.h"
#include "agnus/bitplanes/bitplane_fetch.h"
#include "agnus/bitplanes/bitplane_pointers.h"
#include "agnus/dma/refresh_dma.h"
#include "agnus/dma/sprite_dma.h"
#include "agnus/timing/raster.h"
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
    bplpt_reset(&agnus->bplpt);
    bitplane_fetch_reset(&agnus->fetch);
    sprite_dma_reset(&agnus->sprite_dma);
    agnus_slot_scheduler_init(&agnus->scheduler);
    raster_reset(&agnus->raster, AGNUS_VIDEO_NTSC);
    agnus->beam.line_clocks = agnus->raster.line_clocks;
    agnus->beam.frame_lines = agnus->raster.frame_lines;
    refresh_dma_reset(&agnus->refresh);
}

void rigel_agnus_set_video_std(RigelAgnus *agnus, agnus_video_std_t std)
{
    if (agnus == NULL) {
        return;
    }

    raster_reset(&agnus->raster, std);
    agnus->beam.line_clocks = agnus->raster.line_clocks;
    agnus->beam.frame_lines = agnus->raster.frame_lines;
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

    if (ctx == NULL || cycles == 0) {
        return;
    }

    agnus = &ctx->chipset.agnus;

    rigel_dma_domain_sync_dmacon(&agnus->dma, agnus->dma.dmacon);

    /* Slot loop drives beam CCK-by-CCK, dispatches copper and blitter (Approach C) */
    agnus_slot_scheduler_step_until(&agnus->scheduler, ctx, cycles,
                                     agnus->raster.line_clocks, agnus->raster.frame_lines);
}
