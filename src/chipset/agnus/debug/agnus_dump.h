#ifndef RIGEL_AGNUS_DUMP_H
#define RIGEL_AGNUS_DUMP_H

#include "agnus/agnus_state.h"

/* Agnus register and state dump — for debugging and test assertions.
 * Writes to stderr. Safe to call at any point during emulation. */

void agnus_dump_regs(const RigelAgnus *agnus);
void agnus_dump_dma(const dma_state_t *dma);
void agnus_dump_copper(const copper_state_t *copper);
void agnus_dump_beam(const beam_state_t *beam);

#endif
