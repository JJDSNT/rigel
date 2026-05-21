#include "paula/paula_regs.h"

#include "core/riegel_context.h"
#include "irq/intena.h"
#include "irq/intreq.h"
#include "riegel/riegel_custom.h"

riegel_u16 paula_read_reg(RiegelContext *ctx, riegel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    switch (addr) {
    case RIEGEL_REG_INTENA:
        return ctx->chipset.intena;
    case RIEGEL_REG_INTREQ:
        return ctx->chipset.intreq;
    default:
        return riegel_context_read_reg(ctx, addr);
    }
}

void paula_write_reg(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    switch (addr) {
    case RIEGEL_REG_INTENA:
        ctx->chipset.intena = intena_apply_write(ctx->chipset.intena, value);
        riegel_context_write_reg(ctx, addr, ctx->chipset.intena);
        break;
    case RIEGEL_REG_INTREQ:
        ctx->chipset.intreq = intreq_apply_write(ctx->chipset.intreq, value);
        riegel_context_write_reg(ctx, addr, ctx->chipset.intreq);
        break;
    default:
        riegel_context_write_reg(ctx, addr, value);
        break;
    }
}
