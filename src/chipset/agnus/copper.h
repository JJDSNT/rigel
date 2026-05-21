#ifndef COPPER_H
#define COPPER_H

typedef struct copper_state {
    unsigned program_counter;
} copper_state_t;

void copper_reset(copper_state_t *copper);

#endif
