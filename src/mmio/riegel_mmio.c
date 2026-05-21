#include "riegel/riegel_mmio.h"

#include "mmio/custom_regs.h"

riegel_u16 riegel_custom_read16(RiegelContext *ctx, riegel_u32 addr)
{
    return custom_regs_read16(ctx, addr);
}

void riegel_custom_write16(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value)
{
    custom_regs_write16(ctx, addr, value);
}
