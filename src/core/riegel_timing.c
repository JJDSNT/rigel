#include "core/riegel_timing.h"

riegel_u64 riegel_timing_advance(riegel_u64 current, riegel_u32 delta)
{
    return current + delta;
}
