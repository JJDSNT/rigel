#include "rigel/rigel_mmio.h"

#include "mmio/custom_regs.h"

rigel_u16 rigel_custom_read16(RigelContext *ctx, rigel_u32 addr)
{
    return custom_regs_read16(ctx, addr);
}

void rigel_custom_write16(RigelContext *ctx, rigel_u32 addr, rigel_u16 value)
{
    custom_regs_write16(ctx, addr, value);
}
