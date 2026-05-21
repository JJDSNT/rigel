#include "agnus/agnus_state.h"

#include <stddef.h>

void riegel_agnus_reset(RiegelAgnus *agnus)
{
    if (agnus == NULL) {
        return;
    }

    agnus->dmacon = 0;
    blitter_reset(&agnus->blitter);
}
