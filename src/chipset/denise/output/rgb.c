#include "rgb.h"

void rgb_expand_palette(const rigel_u16 color_regs[32], rigel_u32 rgba_out[32])
{
    for (unsigned i = 0; i < 32; i++)
        rgba_out[i] = rgb12_to_rgba32(color_regs[i]);
}
