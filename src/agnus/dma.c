#include "agnus/dma.h"

#include <stddef.h>

void dma_set_enabled(dma_state_t *dma, bool enabled)
{
    if (dma == NULL) {
        return;
    }

    dma->enabled = enabled;
}
