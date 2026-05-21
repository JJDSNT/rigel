#ifndef SPRITES_H
#define SPRITES_H

typedef struct sprites_state {
    unsigned active_mask;
} sprites_state_t;

void sprites_reset(sprites_state_t *sprites);

#endif
