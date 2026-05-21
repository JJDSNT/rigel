#include "runtime/riegel_runtime.h"

#include <stddef.h>

#include "agnus/agnus_state.h"

void riegel_runtime_run_frame(RiegelContext *ctx)
{
    if (ctx == NULL) {
        return;
    }

    riegel_step(ctx, 1);
    riegel_agnus_step(ctx, 1);
}
