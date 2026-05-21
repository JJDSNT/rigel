#ifndef RIEGEL_PAULA_STATE_H
#define RIEGEL_PAULA_STATE_H

#include "paula/paula_interrupts.h"

typedef struct RiegelPaula {
    RiegelPaulaInterrupts interrupts;
} RiegelPaula;

void riegel_paula_reset(RiegelPaula *paula);

#endif
