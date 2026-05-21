#include "paula/paula_regs.h"

#include "core/rigel_context.h"
#include "paula/disk.h"
#include "paula/paula_interrupts.h"
#include "rigel/rigel_custom.h"

bool paula_owns_reg(rigel_u32 addr)
{
    switch (addr) {
    case RIGEL_REG_DSKDATR:
    case RIGEL_REG_ADKCONR:
    case RIGEL_REG_DSKBYTR:
    case RIGEL_REG_DSKPTH:
    case RIGEL_REG_DSKPTL:
    case RIGEL_REG_DSKLEN:
    case RIGEL_REG_DSKSYNC:
    case RIGEL_REG_INTENA:
    case RIGEL_REG_ADKCON:
    case RIGEL_REG_INTREQ:
        return true;
    default:
        return false;
    }
}

rigel_u16 paula_read_reg(RigelContext *ctx, rigel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    switch (addr) {
    case RIGEL_REG_DSKDATR:
        return disk_read_dskdatr(&ctx->chipset.paula.disk);
    case RIGEL_REG_ADKCONR:
        return ctx->chipset.paula.disk.adkcon;
    case RIGEL_REG_DSKBYTR:
        return disk_read_dskbytr(&ctx->chipset.paula.disk);
    case RIGEL_REG_INTENA:
        return rigel_paula_interrupts_read_intena(&ctx->chipset.paula.interrupts);
    case RIGEL_REG_INTREQ:
        return rigel_paula_interrupts_read_intreq(&ctx->chipset.paula.interrupts);
    default:
        return rigel_context_read_reg(ctx, addr);
    }
}

void paula_write_reg(RigelContext *ctx, rigel_u32 addr, rigel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    switch (addr) {
    case RIGEL_REG_DSKPTH:
        disk_write_dskpth(&ctx->chipset.paula.disk, value);
        rigel_context_write_reg(ctx, addr, value);
        break;
    case RIGEL_REG_DSKPTL:
        disk_write_dskptl(&ctx->chipset.paula.disk, value);
        rigel_context_write_reg(ctx, addr, value);
        break;
    case RIGEL_REG_DSKLEN:
        disk_write_dsklen(&ctx->chipset.paula.disk, value);
        rigel_context_write_reg(ctx, addr, ctx->chipset.paula.disk.dsklen);
        break;
    case RIGEL_REG_DSKSYNC:
        disk_write_dsksync(&ctx->chipset.paula.disk, value);
        rigel_context_write_reg(ctx, addr, value);
        break;
    case RIGEL_REG_INTENA:
        rigel_paula_interrupts_write_intena(&ctx->chipset.paula.interrupts, value);
        rigel_context_write_reg(
            ctx,
            addr,
            rigel_paula_interrupts_read_intena(&ctx->chipset.paula.interrupts)
        );
        break;
    case RIGEL_REG_ADKCON:
        disk_write_adkcon(&ctx->chipset.paula.disk, value);
        rigel_context_write_reg(ctx, addr, ctx->chipset.paula.disk.adkcon);
        break;
    case RIGEL_REG_INTREQ:
        rigel_paula_interrupts_write_intreq(&ctx->chipset.paula.interrupts, value);
        rigel_context_write_reg(
            ctx,
            addr,
            rigel_paula_interrupts_read_intreq(&ctx->chipset.paula.interrupts)
        );
        break;
    default:
        rigel_context_write_reg(ctx, addr, value);
        break;
    }
}
