#include "core/rigel_timing.h"

rigel_u64 rigel_timing_advance(rigel_u64 current, rigel_u32 delta)
{
    return current + delta;
}
