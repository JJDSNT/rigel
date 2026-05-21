#include "denise/denise_regs.h"

#include "core/riegel_context.h"

riegel_u16 denise_read_reg(RiegelContext *ctx, riegel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    return riegel_context_read_reg(ctx, addr);
}

void denise_write_reg(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    riegel_context_write_reg(ctx, addr, value);
}
