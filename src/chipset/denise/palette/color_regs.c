#include "denise/palette/color_regs.h"

#include "rigel/rigel_custom.h"

rigel_u32 rigel_denise_color_expand_12bit(rigel_u16 value)
{
    rigel_u32 r = (rigel_u32)((value >> 8) & 0x0f);
    rigel_u32 g = (rigel_u32)((value >> 4) & 0x0f);
    rigel_u32 b = (rigel_u32)(value & 0x0f);

    return (r << 20) | (r << 16) | (g << 12) | (g << 8) | (b << 4) | b;
}

rigel_u16 rigel_denise_color_expand_rgb565(rigel_u16 value)
{
    rigel_u16 r = (rigel_u16)((value >> 8) & 0x0fu);
    rigel_u16 g = (rigel_u16)((value >> 4) & 0x0fu);
    rigel_u16 b = (rigel_u16)(value & 0x0fu);

    r = (rigel_u16)((r << 4) | r);
    g = (rigel_u16)((g << 4) | g);
    b = (rigel_u16)((b << 4) | b);

    return (rigel_u16)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

rigel_u16 rigel_denise_color_regs_read(const RigelDenise *denise, rigel_u32 addr)
{
    rigel_u32 index;

    if (denise == NULL || addr < RIGEL_REG_COLOR00 || addr > RIGEL_REG_COLOR31) {
        return 0;
    }

    index = (addr - RIGEL_REG_COLOR00) >> 1;
    return denise->regs.color[index];
}

void rigel_denise_color_regs_write(RigelDenise *denise, rigel_u32 addr, rigel_u16 value)
{
    rigel_u32 index;

    if (denise == NULL || addr < RIGEL_REG_COLOR00 || addr > RIGEL_REG_COLOR31) {
        return;
    }

    index = (addr - RIGEL_REG_COLOR00) >> 1;
    denise->regs.color[index] = value;
    denise->palette.rgb32[index] = rigel_denise_color_expand_12bit(value);
    denise->palette.rgb565[index] = rigel_denise_color_expand_rgb565(value);
    denise->debug.last_color_index = (rigel_u16)index;
}
