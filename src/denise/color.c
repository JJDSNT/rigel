#include "denise/color.h"

rigel_u32 color_expand_12bit(rigel_u16 value)
{
    rigel_u32 r = (rigel_u32)((value >> 8) & 0x0f);
    rigel_u32 g = (rigel_u32)((value >> 4) & 0x0f);
    rigel_u32 b = (rigel_u32)(value & 0x0f);

    return (r << 20) | (r << 16) | (g << 12) | (g << 8) | (b << 4) | b;
}
