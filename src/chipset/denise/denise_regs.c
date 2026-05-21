#include "denise/denise_regs.h"

#include "core/rigel_context.h"

rigel_u16 denise_read_reg(RigelContext *ctx, rigel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    return rigel_context_read_reg(ctx, addr);
}

void denise_write_reg(RigelContext *ctx, rigel_u32 addr, rigel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    rigel_context_write_reg(ctx, addr, value);
}
