#ifndef RIGEL_COPPER_DOMAIN_H
#define RIGEL_COPPER_DOMAIN_H

#include "agnus/copper.h"
#include "rigel/rigel_types.h"

void rigel_copper_domain_reset(copper_state_t *copper);
void rigel_copper_domain_step(copper_state_t *copper, rigel_u32 cycles);

#endif
