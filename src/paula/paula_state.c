#include "paula/paula_state.h"

#include <stddef.h>

void riegel_paula_reset(RiegelPaula *paula)
{
    if (paula == NULL) {
        return;
    }

    riegel_paula_interrupts_reset(&paula->interrupts);
}
