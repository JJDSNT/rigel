#ifndef RIEGEL_AGNUS_STATE_H
#define RIEGEL_AGNUS_STATE_H

#include "agnus/blitter/blitter.h"
#include "riegel/riegel_types.h"

typedef struct RiegelAgnus {
    riegel_u16 dmacon;
    BlitterState blitter;
} RiegelAgnus;

void riegel_agnus_reset(RiegelAgnus *agnus);
BlitterMemory riegel_agnus_blitter_memory(RiegelContext *ctx);
BlitterIrqSink riegel_agnus_blitter_irq_sink(RiegelContext *ctx);
void riegel_agnus_blitter_step_dma(RiegelContext *ctx, riegel_u32 dma_slots);
void riegel_agnus_step(RiegelContext *ctx, riegel_u32 cycles);

#endif
