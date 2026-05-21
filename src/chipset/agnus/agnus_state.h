#ifndef RIGEL_AGNUS_STATE_H
#define RIGEL_AGNUS_STATE_H

#include "agnus/beam.h"
#include "agnus/copper.h"
#include "agnus/dma.h"
#include "agnus/blitter/blitter.h"
#include "rigel/rigel_types.h"

typedef struct RigelContext RigelContext;

typedef struct RigelAgnus {
    beam_state_t beam;
    dma_state_t dma;
    copper_state_t copper;
    BlitterState blitter;
} RigelAgnus;

void rigel_agnus_reset(RigelAgnus *agnus);
BlitterMemory rigel_agnus_blitter_memory(RigelContext *ctx);
BlitterIrqSink rigel_agnus_blitter_irq_sink(RigelContext *ctx);
void rigel_agnus_blitter_step_dma(RigelContext *ctx, rigel_u32 dma_slots);
void rigel_agnus_step(RigelContext *ctx, rigel_u32 cycles);

#endif
