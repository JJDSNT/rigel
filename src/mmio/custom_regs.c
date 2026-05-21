#include "mmio/custom_regs.h"

#include "core/riegel_context.h"
#include "irq/intena.h"
#include "irq/intreq.h"

riegel_custom_domain_t riegel_custom_domain_for_reg(riegel_u32 addr)
{
    switch (addr) {
    case RIEGEL_REG_DMACON:
        return RIEGEL_DOMAIN_AGNUS;
    case RIEGEL_REG_INTENA:
    case RIEGEL_REG_INTREQ:
        return RIEGEL_DOMAIN_PAULA;
    case RIEGEL_REG_COLOR00:
        return RIEGEL_DOMAIN_DENISE;
    default:
        return RIEGEL_DOMAIN_UNKNOWN;
    }
}

riegel_u16 custom_regs_read16(RiegelContext *ctx, riegel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    switch (addr) {
    case RIEGEL_REG_INTENA:
        return ctx->intena;
    case RIEGEL_REG_INTREQ:
        return ctx->intreq;
    default:
        return riegel_context_read_reg(ctx, addr);
    }
}

void custom_regs_write16(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    switch (riegel_custom_domain_for_reg(addr)) {
    case RIEGEL_DOMAIN_PAULA:
        if (addr == RIEGEL_REG_INTENA) {
            ctx->intena = intena_apply_write(ctx->intena, value);
            riegel_context_write_reg(ctx, addr, ctx->intena);
            break;
        }

        if (addr == RIEGEL_REG_INTREQ) {
            ctx->intreq = intreq_apply_write(ctx->intreq, value);
            riegel_context_write_reg(ctx, addr, ctx->intreq);
            break;
        }

        riegel_context_write_reg(ctx, addr, value);
        break;
    case RIEGEL_DOMAIN_AGNUS:
    case RIEGEL_DOMAIN_DENISE:
    case RIEGEL_DOMAIN_UNKNOWN:
    default:
        riegel_context_write_reg(ctx, addr, value);
        break;
    }
}
