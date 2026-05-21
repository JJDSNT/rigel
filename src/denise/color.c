#include "denise/color.h"

riegel_u32 color_expand_12bit(riegel_u16 value)
{
    riegel_u32 r = (riegel_u32)((value >> 8) & 0x0f);
    riegel_u32 g = (riegel_u32)((value >> 4) & 0x0f);
    riegel_u32 b = (riegel_u32)(value & 0x0f);

    return (r << 20) | (r << 16) | (g << 12) | (g << 8) | (b << 4) | b;
}
