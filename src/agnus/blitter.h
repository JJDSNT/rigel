#ifndef BLITTER_H
#define BLITTER_H

typedef struct blitter_state {
    unsigned busy;
} blitter_state_t;

void blitter_start(blitter_state_t *blitter);

#endif
