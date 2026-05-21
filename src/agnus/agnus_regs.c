#include "agnus/agnus_regs.h"

#include "core/riegel_context.h"
#include "riegel/riegel_custom.h"

static riegel_u16 riegel_apply_setclr(riegel_u16 current, riegel_u16 value)
{
    riegel_u16 mask = (riegel_u16)(value & 0x7fffU);

    if ((value & RIEGEL_SETCLR) != 0) {
        return (riegel_u16)(current | mask);
    }

    return (riegel_u16)(current & (riegel_u16)(~mask));
}

riegel_u16 agnus_read_reg(RiegelContext *ctx, riegel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    switch (addr) {
    case RIEGEL_REG_DMACON:
        return ctx->chipset.dmacon;
    default:
        return riegel_context_read_reg(ctx, addr);
    }
}

void agnus_write_reg(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    switch (addr) {
    case RIEGEL_REG_DMACON:
        ctx->chipset.dmacon = riegel_apply_setclr(ctx->chipset.dmacon, value);
        riegel_context_write_reg(ctx, addr, ctx->chipset.dmacon);
        break;
    default:
        riegel_context_write_reg(ctx, addr, value);
        break;
    }
}
