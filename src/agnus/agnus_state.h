#ifndef RIEGEL_AGNUS_STATE_H
#define RIEGEL_AGNUS_STATE_H

#include "agnus/blitter/blitter.h"
#include "riegel/riegel_types.h"

typedef struct RiegelAgnus {
    riegel_u16 dmacon;
    BlitterState blitter;
} RiegelAgnus;

void riegel_agnus_reset(RiegelAgnus *agnus);

#endif
