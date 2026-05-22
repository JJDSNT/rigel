#include "domains/beam/beam_domain.h"

#include <stddef.h>

void rigel_beam_domain_reset(beam_state_t *beam)
{
    beam_reset(beam);
}

void rigel_beam_domain_step(beam_state_t *beam, rigel_u16 clocks)
{
    beam_step(beam, clocks);
}
