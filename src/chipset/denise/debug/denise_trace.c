#include "denise/debug/denise_trace.h"

void rigel_denise_trace_tick(rigel_denise_debug_state_t *debug, rigel_u32 cycles)
{
    if (debug == NULL) {
        return;
    }

    debug->scanline_counter += cycles;
}
