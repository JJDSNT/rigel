#include "paula/paula_regs.h"

#include "core/riegel_context.h"
#include "paula/paula_interrupts.h"
#include "riegel/riegel_custom.h"

riegel_u16 paula_read_reg(RiegelContext *ctx, riegel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    switch (addr) {
    case RIEGEL_REG_INTENA:
        return riegel_paula_interrupts_read_intena(&ctx->chipset.paula.interrupts);
    case RIEGEL_REG_INTREQ:
        return riegel_paula_interrupts_read_intreq(&ctx->chipset.paula.interrupts);
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
        riegel_paula_interrupts_write_intena(&ctx->chipset.paula.interrupts, value);
        riegel_context_write_reg(
            ctx,
            addr,
            riegel_paula_interrupts_read_intena(&ctx->chipset.paula.interrupts)
        );
        break;
    case RIEGEL_REG_INTREQ:
        riegel_paula_interrupts_write_intreq(&ctx->chipset.paula.interrupts, value);
        riegel_context_write_reg(
            ctx,
            addr,
            riegel_paula_interrupts_read_intreq(&ctx->chipset.paula.interrupts)
        );
        break;
    default:
        riegel_context_write_reg(ctx, addr, value);
        break;
    }
}
