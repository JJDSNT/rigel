#ifndef RIGEL_BLITTER_DOMAIN_H
#define RIGEL_BLITTER_DOMAIN_H

#include "agnus/blitter/blitter.h"
#include "rigel/rigel_types.h"

bool rigel_blitter_domain_owns_reg(rigel_u32 addr);
void rigel_blitter_domain_reset(BlitterState *blitter);
rigel_u16 rigel_blitter_domain_read_reg(const BlitterState *blitter, rigel_u32 addr);
void rigel_blitter_domain_write_reg(BlitterState *blitter, rigel_u32 addr, rigel_u16 value);
void rigel_blitter_domain_step_dma(
    BlitterState *blitter,
    BlitterMemory memory,
    BlitterIrqSink irq,
    rigel_u32 dma_slots
);

#endif
