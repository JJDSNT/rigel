#include "paula/paula_regs.h"

#include "core/riegel_context.h"
#include "paula/disk.h"
#include "paula/paula_interrupts.h"
#include "riegel/riegel_custom.h"

bool paula_owns_reg(riegel_u32 addr)
{
    switch (addr) {
    case RIEGEL_REG_DSKDATR:
    case RIEGEL_REG_ADKCONR:
    case RIEGEL_REG_DSKBYTR:
    case RIEGEL_REG_DSKPTH:
    case RIEGEL_REG_DSKPTL:
    case RIEGEL_REG_DSKLEN:
    case RIEGEL_REG_DSKSYNC:
    case RIEGEL_REG_INTENA:
    case RIEGEL_REG_ADKCON:
    case RIEGEL_REG_INTREQ:
        return true;
    default:
        return false;
    }
}

riegel_u16 paula_read_reg(RiegelContext *ctx, riegel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    switch (addr) {
    case RIEGEL_REG_DSKDATR:
        return disk_read_dskdatr(&ctx->chipset.paula.disk);
    case RIEGEL_REG_ADKCONR:
        return ctx->chipset.paula.disk.adkcon;
    case RIEGEL_REG_DSKBYTR:
        return disk_read_dskbytr(&ctx->chipset.paula.disk);
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
    case RIEGEL_REG_DSKPTH:
        disk_write_dskpth(&ctx->chipset.paula.disk, value);
        riegel_context_write_reg(ctx, addr, value);
        break;
    case RIEGEL_REG_DSKPTL:
        disk_write_dskptl(&ctx->chipset.paula.disk, value);
        riegel_context_write_reg(ctx, addr, value);
        break;
    case RIEGEL_REG_DSKLEN:
        disk_write_dsklen(&ctx->chipset.paula.disk, value);
        riegel_context_write_reg(ctx, addr, ctx->chipset.paula.disk.dsklen);
        break;
    case RIEGEL_REG_DSKSYNC:
        disk_write_dsksync(&ctx->chipset.paula.disk, value);
        riegel_context_write_reg(ctx, addr, value);
        break;
    case RIEGEL_REG_INTENA:
        riegel_paula_interrupts_write_intena(&ctx->chipset.paula.interrupts, value);
        riegel_context_write_reg(
            ctx,
            addr,
            riegel_paula_interrupts_read_intena(&ctx->chipset.paula.interrupts)
        );
        break;
    case RIEGEL_REG_ADKCON:
        disk_write_adkcon(&ctx->chipset.paula.disk, value);
        riegel_context_write_reg(ctx, addr, ctx->chipset.paula.disk.adkcon);
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
