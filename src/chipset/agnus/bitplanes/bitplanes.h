#ifndef RIGEL_AGNUS_BITPLANES_H
#define RIGEL_AGNUS_BITPLANES_H

typedef struct bitplanes_state {
    unsigned depth;
} bitplanes_state_t;

void bitplanes_set_depth(bitplanes_state_t *state, unsigned depth);

#endif
