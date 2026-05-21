#ifndef PAULA_H
#define PAULA_H

#include <stdint.h>

#include "paula/paula_state.h"

typedef RigelPaula Paula;

void paula_irq_raise(Paula *paula, uint16_t mask);
void paula_irq_clear(Paula *paula, uint16_t mask);

#endif
