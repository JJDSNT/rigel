#include "runtime/rigel_runtime.h"

#include <stddef.h>

void rigel_runtime_run_frame(RigelContext *ctx)
{
    if (ctx == NULL) {
        return;
    }

    rigel_step(ctx, 1);
}
