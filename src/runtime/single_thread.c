#include "runtime/single_thread.h"

#include <stddef.h>

#include "runtime/rigel_runtime.h"

void single_thread_pump(RigelContext *ctx, unsigned frames)
{
    unsigned i;

    if (ctx == NULL) {
        return;
    }

    for (i = 0; i < frames; ++i) {
        rigel_runtime_run_frame(ctx);
    }
}
