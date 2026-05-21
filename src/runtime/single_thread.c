#include "runtime/single_thread.h"

#include <stddef.h>

#include "runtime/riegel_runtime.h"

void single_thread_pump(RiegelContext *ctx, unsigned frames)
{
    unsigned i;

    if (ctx == NULL) {
        return;
    }

    for (i = 0; i < frames; ++i) {
        riegel_runtime_run_frame(ctx);
    }
}
