#ifndef RIGEL_COPPER_DOMAIN_H
#define RIGEL_COPPER_DOMAIN_H

#include "agnus/copper/copper.h"
#include "agnus/beam.h"
#include "agnus/dma.h"
#include "rigel/rigel_types.h"

void rigel_copper_domain_reset(copper_state_t *copper);
void rigel_copper_domain_set_wait(copper_state_t *copper,
                                   rigel_u16 vpos, rigel_u16 hpos,
                                   rigel_u16 vpmask, rigel_u16 hpmask);
void rigel_copper_domain_jump1(copper_state_t *copper);
void rigel_copper_domain_jump2(copper_state_t *copper);
void rigel_copper_domain_vbl_reload(copper_state_t *copper);
void rigel_copper_domain_step(copper_state_t *copper, const beam_state_t *beam,
                              const dma_state_t *dma, bool blitter_busy);

/* Cycles until the armed WAIT condition fires from the given beam position.
 * Returns 0xFFFFFFFFu if the copper is not waiting or condition already satisfied. */
rigel_u32 rigel_copper_domain_cycles_to_wait(const copper_state_t *copper,
                                              const beam_state_t *beam);

#endif
