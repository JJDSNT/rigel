#ifndef BEAM_H
#define BEAM_H

#include "rigel/rigel_types.h"

typedef struct beam_state {
    rigel_u16 hpos;
    rigel_u16 vpos;
} beam_state_t;

void beam_step(beam_state_t *beam, rigel_u16 clocks);

#endif
