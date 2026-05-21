#include "debug/trace.h"

static bool g_trace_enabled;

void rigel_trace_set_enabled(bool enabled)
{
    g_trace_enabled = enabled;
}

bool rigel_trace_is_enabled(void)
{
    return g_trace_enabled;
}
