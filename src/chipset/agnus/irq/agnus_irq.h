#ifndef RIGEL_AGNUS_IRQ_H
#define RIGEL_AGNUS_IRQ_H

#include "rigel/rigel_types.h"

/* Agnus IRQ sources — Agnus raises INTREQ bits; Paula aggregates and delivers IPL.
 *
 * Agnus-generated sources (INTREQ bits):
 *   Bit 9  (0x0200)  EXTER — not Agnus
 *   Bit 8  (0x0100)  INTEN — not Agnus
 *   Bit 6  (0x0040)  BLIT  — blitter done
 *   Bit 5  (0x0020)  VERTB — vertical blank
 *   Bit 4  (0x0010)  COPER — copper (COPJMP1/2 or illegal MOVE)
 *
 * Agnus calls rigel_chipset_raise_irq_source() with the relevant INTB_ bitmask. */

#define AGNUS_INTB_BLIT   0x0040u
#define AGNUS_INTB_VERTB  0x0020u
#define AGNUS_INTB_COPER  0x0010u

typedef struct RigelContext RigelContext;

void agnus_irq_raise_blitter_done(RigelContext *ctx);
void agnus_irq_raise_vblank(RigelContext *ctx);
void agnus_irq_raise_copper(RigelContext *ctx);

#endif
