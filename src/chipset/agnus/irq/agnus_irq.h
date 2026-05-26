#ifndef RIGEL_AGNUS_IRQ_H
#define RIGEL_AGNUS_IRQ_H

#include "rigel/rigel_types.h"

/* Agnus IRQ sources — Agnus raises INTREQ bits; Paula aggregates and delivers IPL.
 *
 * BLIT and COPER IRQs are raised through their respective domain sinks
 * (BlitterIrqSink callback, copper domain). Only VERTB is raised directly
 * by the slot scheduler on the beam position transition. */

#define AGNUS_INTB_BLIT   0x0040u
#define AGNUS_INTB_VERTB  0x0020u
#define AGNUS_INTB_COPER  0x0010u

typedef struct RigelContext RigelContext;

void agnus_irq_raise_vblank(RigelContext *ctx);

#endif
