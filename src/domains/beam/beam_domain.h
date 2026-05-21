#ifndef RIGEL_BEAM_DOMAIN_H
#define RIGEL_BEAM_DOMAIN_H

#include "agnus/beam.h"

void rigel_beam_domain_reset(beam_state_t *beam);
void rigel_beam_domain_step(beam_state_t *beam, rigel_u16 clocks);

#endif
