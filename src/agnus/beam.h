#ifndef BEAM_H
#define BEAM_H

#include "riegel/riegel_types.h"

typedef struct beam_state {
    riegel_u16 hpos;
    riegel_u16 vpos;
} beam_state_t;

void beam_step(beam_state_t *beam, riegel_u16 clocks);

#endif
