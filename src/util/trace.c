#include "util/trace.h"

static bool g_trace_enabled;

void riegel_trace_set_enabled(bool enabled)
{
    g_trace_enabled = enabled;
}

bool riegel_trace_is_enabled(void)
{
    return g_trace_enabled;
}
